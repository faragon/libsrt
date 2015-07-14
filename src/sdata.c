/*
 * sdata.c
 *
 * Dynamic size data handling. Helper functions, not part of the API.
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */ 

#include "sdata.h"
#include "scommon.h"

/*
 * Allocation heuristic configuration
 *
 * SD_GROW_CACHE_MIN_ELEMS (e.g. 1024): number of elements that can be asumed
 * is going to be cheap to be copied because of being covered by data cache
 * fetch + write back.
 * SD_GROW_PCT_CACHED (e.g. 25%): if in the case of cached region, when not
 * having available space, allocate at least 25% the current allocated memory.
 * SD_GROW_PCT_NONCACHED (e.g. 100%): similar to the previous case, but when
 * elements are over SD_GROW_CACHE_MIN_ELEMS, increase the allocation in 100%.
 */

#define SD_GROW_CACHE_MIN_ELEMS	4096
#define SD_GROW_PCT_CACHED	  25
#define SD_GROW_PCT_NONCACHED	  50

/*
 * Internal functions
 */

size_t sd_alloc_size_to_mt_size(const size_t a_size, const struct sd_conf *f)
{
	ASSERT_RETURN_IF(!f, 0);
	return a_size <= S_ALLOC_SMALL ? f->metainfo_small : f->metainfo_full;
}

sbool_t sd_alloc_size_to_is_big(const size_t s, const struct sd_conf *f)
{
	return s <= f->alloc_small ? S_FALSE : S_TRUE;
}

static size_t sd_size_to_alloc_size(const size_t header_size,
	const size_t elem_size, const size_t size, const struct sd_conf *f)
{
	ASSERT_RETURN_IF(!f, 0);
	if (elem_size > 0 && size > 0)
		return header_size + size * elem_size;
	return size * f->fixed_elem_size +
		 (size <= f->range_small && f->metainfo_small > 0 ?
		  f->metainfo_small : f->metainfo_full);
}

size_t sd_get_size(const sd_t *d)
{
	RETURN_IF(!d, 0);
	return d->is_full ? ((const struct SData_Full *)d)->size :
		(size_t)((d->size_h << 8) | ((struct SData_Small *)d)->size_l);
}

size_t sd_get_alloc_size(const sd_t *d)
{
	return d->is_full ? ((struct SData_Full *)d)->alloc_size :
		(size_t)((d->alloc_size_h << 8) |
			 ((struct SData_Small *)d)->alloc_size_l);
}

void sd_set_size(sd_t *d, const size_t size)
{
	if (d) {
		if (d->is_full) {
			((struct SData_Full *)d)->size = size;
		} else {
			d->size_h = (size & 0x100) ? 1 : 0;
			((struct SData_Small *)d)->size_l = size & 0xff;
		}
	}
}

void sd_set_alloc_size(sd_t *d, const size_t alloc_size)
{
	if (d) {
		if (d->is_full) {
			((struct SData_Full *)d)->alloc_size = alloc_size;
		} else {
			d->alloc_size_h = (alloc_size & 0x100) ? 1 : 0;
			((struct SData_Small *)d)->alloc_size_l =
							alloc_size & 0xff;
		}
	}
}

/*
 * Allocation
 */

sd_t *sd_alloc(const size_t header_size, const size_t elem_size,
	       const size_t initial_reserve, const struct sd_conf *f)
{
	size_t alloc_size = sd_size_to_alloc_size(header_size, elem_size,
						  initial_reserve, f);
	sd_t *d = (sd_t *)__sd_malloc(alloc_size);
	if (d) {
		f->sx_reset(d, alloc_size, S_FALSE);
		S_PROFILE_ALLOC_CALL;
	} else {
		S_ERROR("not enough memory");
		d = f->sx_void;
	}
	return d;
}

sd_t *sd_alloc_into_ext_buf(void *buffer, const size_t buffer_size,
			    const struct sd_conf *f)
{
	size_t metainfo_sz_req = sd_alloc_size_to_mt_size(buffer_size, f);
	S_ASSERT(buffer && buffer_size >= metainfo_sz_req);
	if (!buffer || buffer_size < metainfo_sz_req)
		return NULL;    /* not enough memory */
	sd_t *d = (sd_t *)buffer;
	if (f->sx_reset)
		f->sx_reset(d, buffer_size, S_TRUE);
	else
		sd_reset(d, S_TRUE, buffer_size, S_TRUE);
	return d;
}

void sd_free(sd_t **d)
{
	if (d && *d) {
		if (!(*d)->ext_buffer)
			free(*d);
		*d = NULL;
	}
}

void sd_free_va(const size_t elems, sd_t **first, va_list ap)
{
	sd_free(first);
	if (elems == 1)
		return;
	size_t i = 1;
        for (; i < elems; i++)
                sd_free((sd_t **)va_arg(ap, sd_t **));
}

void sd_reset(sd_t *d, const sbool_t use_big_struct,
	      const size_t alloc_size, const sbool_t ext_buf)
{
	if (d) {
		d->is_full = use_big_struct;
		d->ext_buffer = ext_buf;
		if (d->is_full) {
			d->size_h = 0;
			d->alloc_size_h = 0;
		}
		sd_reset_alloc_errors(d);
		sd_set_size(d, 0);
		sd_set_alloc_size(d, alloc_size);
		d->other1 = 0;
		d->other2 = 0;
		d->other3 = 0;
	}
}

/* Acknowledgements: similar to git's strbuf_grow */
size_t sd_grow(sd_t **d, const size_t extra_size, const struct sd_conf *f)
{
	ASSERT_RETURN_IF(!d, 0);
	size_t size = *d ? sd_get_size(*d) : 0;
	ASSERT_RETURN_IF(s_size_t_overflow(size, extra_size), 0);
	size_t new_size = sd_reserve(d, size + extra_size, f);
	return new_size >= (size + extra_size) ? (new_size - size) : 0;
}

static size_t sd_resize_aux(sd_t **d, size_t max_size, const struct sd_conf *f)
{
	ASSERT_RETURN_IF(!d || !f, 0);
	if (*d && *d != f->sx_void) {
		const size_t current_max_size = f->sx_get_max_size(*d);
		if (current_max_size < max_size) {
#ifdef SD_ENABLE_HEURISTIC_GROW
			size_t grow_pct = max_size < SD_GROW_CACHE_MIN_ELEMS ?
				SD_GROW_PCT_CACHED : SD_GROW_PCT_NONCACHED;
			size_t inc = max_size > 10000 ?
					(max_size / 100) * grow_pct :
					(max_size * grow_pct) / 100;
			/*
			 * On 32-bit systems, over 2GB allocations the
			 * heuristic increment will be in 16MB steps
			 */
			if (s_size_t_overflow(max_size, inc))
				inc = 16 * 1024 * 1024;
			if (!s_size_t_overflow(max_size, inc))
				max_size += inc;
#endif
		}
		if (max_size == 0) /* BEHAVIOR: minimum alloc size: 1 */
			max_size = 1;
		if ((*d)->ext_buffer) {
			S_ERROR("out of memore on fixed-size "
				 "allocated space");
			sd_set_alloc_errors(*d);
			return current_max_size;
		}
		size_t header_size, elem_size;
		header_size = f->header_size;
		elem_size = !f->elem_size_off ? 0 :
			    s_load_size_t((void *)*d, f->elem_size_off);
		size_t new_alloc_size = sd_size_to_alloc_size(
						header_size, elem_size,
						max_size, f);
		sd_t *d1 = (sd_t *)__sd_realloc(*d, new_alloc_size);
		if (!d1) {
			S_ERROR("not enough memory (realloc error)");
			sd_set_alloc_errors(*d);
			return current_max_size;
		}
		*d = d1;
		S_PROFILE_ALLOC_CALL;
		if (f->sx_reconfig && current_max_size < max_size) {
			size_t mt1 = sd_alloc_size_to_mt_size(
					sd_get_alloc_size(*d), f);
			size_t mt2 = sd_alloc_size_to_mt_size(
					new_alloc_size, f);
			if (mt1 >= mt2) /* Current type is enough */
				sd_set_alloc_size(*d, new_alloc_size);
			else /* Structure rewrite required */
				f->sx_reconfig(*d, new_alloc_size, mt2);
		} else {
			sd_set_alloc_size(*d, new_alloc_size);
		}
	} else {
		*d = f->sx_alloc(max_size);
	}
	return *d ? f->sx_get_max_size(*d) : 0;
}

size_t sd_reserve(sd_t **d, size_t max_size, const struct sd_conf *f)
{
	ASSERT_RETURN_IF(!d || !f, 0);
	if (*d && *d != f->sx_void) {
		if (f->sx_get_max_size(*d) < max_size)
			return sd_resize_aux(d, max_size, f);
	} else {
		*d = f->sx_alloc(max_size);
	}
	return *d ? f->sx_get_max_size(*d) : 0;
}

sd_t *sd_shrink(sd_t **d, const struct sd_conf *f)
{
	ASSERT_RETURN_IF(!d, f->sx_void);
	RETURN_IF(!*d, f->sx_check(d));
	size_t csize = sd_get_size(*d);
	if (*d && csize < f->sx_get_max_size(*d) &&
	    !(*d)->ext_buffer) { /* shrink only for heap-allocated */
		/* non-trivial resize structure (full->small switch) */
		if (f->sx_dup && f->range_small > 0 &&
		    csize <= S_ALLOC_SMALL && (*d)->is_full) {
			sd_t *d2 = f->sx_dup(*d);
			if  (d2) {
				sd_free(d);
				*d = d2;
			}
		} else { /* generic fast resize (via realloc) */
			sd_resize_aux(d, csize, f);
		}
	}
	return f->sx_check(d);
}


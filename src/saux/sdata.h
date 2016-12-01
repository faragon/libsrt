#ifndef SDATA_H
#define SDATA_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * sdata.h
 *
 * Dynamic size data handling.
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 *
 * Observations:
 * - This is not intended for direct use, but as base for
 *   other types (ss_t, sv_t, st_t, sm_t, etc.)
 */

#include "scommon.h"

/*
 * Allocation heuristic configuration
 *
 * Define SD_ENABLE_HEURISTIC_GROWTH for enabling pre-allocation heuristics.
 */

#ifndef SD_DISABLE_HEURISTIC_GROWTH
#define SD_ENABLE_HEURISTIC_GROWTH
#endif

/*
 * Macros
 */

#define SD_BUILDFUNCS_COMMON(pfix, tail_bytes)				       \
	S_INLINE pfix##_t *pfix##_shrink(pfix##_t **c) {		       \
		return (pfix##_t *)sd_shrink((sd_t **)c, tail_bytes);	       \
	}								       \
	S_INLINE sbool_t pfix##_empty(const pfix##_t *c) {		       \
		return pfix##_size(c) == 0 ? S_TRUE : S_FALSE;		       \
	}								       \
	S_INLINE void pfix##_set_alloc_errors(pfix##_t *c) {		       \
		sd_set_alloc_errors((sd_t *)c);				       \
	}								       \
	S_INLINE void pfix##_reset_alloc_errors(pfix##_t *c) {		       \
		sd_reset_alloc_errors((sd_t *)c);			       \
	}								       \
	S_INLINE sbool_t pfix##_alloc_errors(pfix##_t *c) {		       \
		return sd_alloc_errors((sd_t *)c);			       \
	}

#define SD_BUILDFUNCS_ST(pfix, stpfix)					       \
	S_INLINE size_t pfix##_size(const pfix##_t *c) {		       \
		return stpfix##_size((const sd_t *)c);			       \
	}								       \
	S_INLINE void pfix##_set_size(pfix##_t *c, const size_t s) {	       \
		stpfix##_set_size((sd_t *)c, s);			       \
	}								       \
	S_INLINE size_t pfix##_max_size(const pfix##_t *c) {		       \
		return stpfix##_max_size((const sd_t *)c);		       \
	}								       \
	S_INLINE size_t pfix##_alloc_size(const pfix##_t *c) {		       \
		return stpfix##_alloc_size((const sd_t *)c);		       \
	}								       \
	S_INLINE size_t pfix##_get_buffer_size(const pfix##_t *c) {	       \
		return stpfix##_size((const sd_t *)c) *			       \
		       stpfix##_elem_size((const sd_t *)c);		       \
	}								       \
	S_INLINE void *pfix##_elem_addr(pfix##_t *c, const size_t pos) {       \
		return stpfix##_elem_addr((sd_t *)c, pos);		       \
	}								       \
	S_INLINE const void *pfix##_elem_addr_r(const pfix##_t *c,	       \
						const size_t pos) {	       \
		return stpfix##_elem_addr_r((const sd_t *)c, pos);	       \
	}								       \
	S_INLINE size_t pfix##_capacity(const pfix##_t *c) {		       \
		return stpfix##_max_size((const sd_t *)c);		       \
	}								       \
	S_INLINE size_t pfix##_capacity_left(const pfix##_t *c) {	       \
		return stpfix##_max_size((const sd_t *)c) -		       \
		       stpfix##_size((const sd_t *)c);			       \
	}								       \
	S_INLINE size_t pfix##_len(const pfix##_t *c) {			       \
		return stpfix##_size((const sd_t *)c);			       \
	}								       \

#define SD_BUILDFUNCS_ST2(pfix, stpfix)					       \
	S_INLINE char *pfix##_get_buffer(pfix##_t *c) {			       \
		return stpfix##_get_buffer((sd_t *)c);			       \
	}								       \
	S_INLINE const char *pfix##_get_buffer_r(const pfix##_t *c) {	       \
		return stpfix##_get_buffer_r((const sd_t *)c);		       \
	}								       \

#define SD_BUILDFUNCS_DYN_ST(pfix, tail_bytes)				       \
	SD_BUILDFUNCS_ST(pfix, sdx)					       \
	SD_BUILDFUNCS_COMMON(pfix, tail_bytes)

#define SD_BUILDFUNCS_FULL_ST(pfix, tail_bytes)				       \
	SD_BUILDFUNCS_ST(pfix, sd)					       \
	SD_BUILDFUNCS_ST2(pfix, sd)					       \
	SD_BUILDFUNCS_COMMON(pfix, tail_bytes)				       \
	S_INLINE size_t pfix##_grow(pfix##_t **c, const size_t extra_elems) {  \
		return sd_grow((sd_t **)c, extra_elems, tail_bytes);	       \
	}								       \
	S_INLINE size_t pfix##_reserve(pfix##_t **c, const size_t max_elems) { \
		return sd_reserve((sd_t **)c, max_elems, tail_bytes);	       \
	}								       \

#define SD_FREE_AUX(pfix)						       \
	S_INLINE void pfix##_free_aux(pfix##_t **c, ...) {		       \
		va_list ap;						       \
		va_start(ap, c);					       \
		sd_free_va((sd_t **)c, ap);				       \
		va_end(ap);						       \
	}

#define SD_BUILDFUNCS_DYN(pfix, tail_bytes)				       \
	SD_BUILDFUNCS_DYN_ST(pfix, tail_bytes)				       \
	SD_FREE_AUX(pfix)

#define SD_BUILDFUNCS_FULL(pfix, tail_bytes)				       \
	SD_BUILDFUNCS_FULL_ST(pfix, tail_bytes)				       \
	SD_FREE_AUX(pfix)

/*
 * Data structures and types
 */

enum SDataStMode
{
	SData_Full	= 0,
	SData_DynFull	= 1,
	SData_DynSmall	= 2,
	SData_VoidData	= 3
};

struct SDataFlags /* 1-byte structure */
{
	/* Dynamic vs fixed memory flag
	 *
	 * 0: using dynamic memory. Implemented via malloc/realloc/free.
	 * 1: stack or external buffer (fixed-size). Allocation and
	 *    de-allocation is done externally (or implicit, e.g. stack)
	 */
	unsigned char ext_buffer : 1;

	/* Allocation error status flag
	 *
	 * 0: no allocation errors.
	 * 1: allocation errors found. Those errors happen when e.g.
	 *    increasing container size (add, concatenate, etc.) and not
	 *    being possible to request more memory. For erasing this flag
	 *    a call to sd_reset_alloc_errors() is required. This flag is
	 *    just informative, i.e. being active does not change operation
	 *    behavior.
	 */
	unsigned char alloc_errors : 1;

	/* Struct mode
	 *
	 * SData_Full(0): full mode (header_size >= sizeof(struct SDataFull))
	 * SData_DynFull(1): dynamic, full mode (header_size >=
	 *					 sizeof(struct SDataFull))
	 * SData_DynSmall(2): dynamic, small mode (header_size ==
	 *					   sizeof(struct SDataSmall))
	 * SData_VoidData(3): void data, not suitable for any operation. The
	 * intention for for this is constant data containing a default object
	 * (e.g. empty string for avoding returning null pointers)
	 */
	unsigned char st_mode : 2;

	/*
	 * Type-specific flags
	 */
	unsigned char flag1 : 1;
	unsigned char flag2 : 1;
	unsigned char flag3 : 1;
	unsigned char flag4 : 1;
};

struct SDataSmall /* 4-byte structure */
{
	/* Implicit values:
	 * elem_size: 1
	 * header_size = sizeof(SDataSmall)
	 * sub_type: 0
	 */

	struct SDataFlags f;

	/*
	* Number of elements
	*/
	uint8_t size;

	/*
	* Maximum number of elements
	*/
	uint8_t max_size;

	/*
	* Type-specific configuration
	*/
	uint8_t aux;
};

struct SDataFull /* 16-byte structure for 32-bit compiler, 32-byte for 64-bit */
{
	struct SDataFlags f;

	/*
	 * Header size: struct SData size plus additional header from type
	 * build on top of it.
	 */
	uint8_t header_size;

	/*
	 * Type-specific configuration
	 */
	uint8_t sub_type;

	/*
	 * Type element size
	 */
	size_t elem_size;

	/*
	 * Number of elements
	 */
	size_t size;

	/*
	 * Maximum number of elements
	 */
	size_t max_size;
};

typedef struct SDataFull sd_t; /* "Hidden" structure (accessors are provided) */

#define EMPTY_SDataFlags	{ 1, 1, 3, 0, 0, 0, 0 }
#define EMPTY_SDataSmall	{ EMPTY_SDataFlags, 0, 0, 0 }
#define EMPTY_SDataFull		{ EMPTY_SDataFlags, 0, 0, 0, 0, 0 }

extern sd_t *sd_void;

/*
 * Functions
 */

S_INLINE void sd_set_alloc_errors(sd_t *d)
{
	if (d)
		d->f.alloc_errors = 1;
}

S_INLINE void sd_reset_alloc_errors(sd_t *d)
{
	if (d)
		d->f.alloc_errors = 0;
}

S_INLINE sbool_t sd_alloc_errors(const sd_t *d)
{
	return (!d || d->f.alloc_errors) ? S_TRUE : S_FALSE;
}

S_INLINE size_t sd_size(const sd_t *d)
{
	RETURN_IF(!d, 0);
	return d->size;
}

S_INLINE void sd_set_size(sd_t *d, const size_t size)
{
	if (d)
		d->size = size;
}

S_INLINE void sd_set_max_size(sd_t *d, const size_t max_size)
{
	if (d)
		d->max_size = max_size;
}

S_INLINE size_t sd_max_size(const sd_t *d)
{
	RETURN_IF(!d, 0);
	return d->max_size;
}

S_INLINE char *sd_get_buffer(sd_t *d)
{
	return !d ? NULL : (char *)d + d->header_size;
}

S_INLINE const char *sd_get_buffer_r(const sd_t *d)
{
	return !d ? NULL : (const char *)d + d->header_size;
}

S_INLINE void *sd_elem_addr(sd_t *d, const size_t pos)
{
	return !d ? NULL : ((char *)d + d->header_size +
					d->elem_size * pos);
}

S_INLINE const void *sd_elem_addr_r(const sd_t *d, const size_t pos)
{
	return !d ? NULL : ((const char *)d + d->header_size +
					      d->elem_size * pos);
}

S_INLINE size_t sd_elem_size(const sd_t *d)
{
	RETURN_IF(!d, 0);
	return d->elem_size;
}

S_INLINE size_t sd_alloc_size(const sd_t *d)
{
	RETURN_IF(!d, 0);
	return d->header_size + d->elem_size * d->max_size;
}

S_INLINE uint8_t sdx_szt_to_u8_sat(size_t v)
{
	return v <= 255 ? (uint8_t)v : 255;
}

S_INLINE sbool_t sdx_full_st(const sd_t *d)
{
	return d && d->f.st_mode <= SData_DynFull ? S_TRUE : S_FALSE;
}

S_INLINE sbool_t sdx_dyn_st(const sd_t *d)
{
	RETURN_IF(!d, S_FALSE);
	return d->f.st_mode == SData_DynFull || d->f.st_mode == SData_DynSmall ?
	       S_TRUE : S_FALSE;
}

S_INLINE void sdx_set_max_size(sd_t *d, const size_t max_size)
{
	if (d) {
		if (sdx_full_st(d))
			d->max_size = max_size;
		else
			((struct SDataSmall *)d)->max_size =
						sdx_szt_to_u8_sat(max_size);
	}
}

S_INLINE size_t sdx_size(const sd_t *d)
{
	RETURN_IF(!d, 0);
	if (sdx_full_st(d))
		return sd_size(d);
	return ((const struct SDataSmall *)d)->size;
}

S_INLINE void sdx_set_size(sd_t *d, const size_t size)
{
	if (d) {
		if (sdx_full_st(d))
			sd_set_size(d, size);
		else
			((struct SDataSmall *)d)->size =
						sdx_szt_to_u8_sat(size);
	}
}

S_INLINE size_t sdx_max_size(const sd_t *d)
{
	RETURN_IF(!d, 0);
	if (sdx_full_st(d))
		return sd_max_size(d);
	return ((const struct SDataSmall *)d)->max_size;
}

S_INLINE char *sdx_get_buffer(sd_t *d)
{
	return	!d ? NULL :
		(char *)d + (sdx_full_st(d) ? d->header_size :
					     sizeof(struct SDataSmall));
}

S_INLINE const char *sdx_get_buffer_r(const sd_t *d)
{
	return	!d ? NULL :
		(const char *)d + (sdx_full_st(d) ? d->header_size :
						   sizeof(struct SDataSmall));
}

S_INLINE void *sdx_elem_addr(sd_t *d, const size_t pos)
{
	return !d ? NULL : (char *)d +
		(sdx_full_st(d) ? d->header_size + d->elem_size * pos :
		sizeof(struct SDataSmall) + pos);	/* BEHAVIOR */
}

S_INLINE const void *sdx_elem_addr_r(const sd_t *d, const size_t pos)
{
	return !d ? NULL : (const char *)d +
		(sdx_full_st(d) ? d->header_size + d->elem_size * pos :
		sizeof(struct SDataSmall) + pos);	/* BEHAVIOR */
}

S_INLINE uint8_t sdx_header_size(const sd_t *d)
{
	RETURN_IF(!d, 0);
	RETURN_IF(sdx_full_st(d), d->header_size);
	return sizeof(struct SDataSmall); /* Implicit value */
}

S_INLINE size_t sdx_elem_size(const sd_t *d)
{
	RETURN_IF(!d, 0);
	RETURN_IF(sdx_full_st(d), d->elem_size);
	return 1; /* Implicit value */
}

S_INLINE size_t sdx_alloc_size(const sd_t *d)
{
	return sdx_header_size(d) + sdx_elem_size(d) * sdx_max_size(d);
}

/* Checks if structure container switch is required
 * -1: full to small
 * 0: no changes
 * 1: small to full
 */
S_INLINE int sdx_chk_st_change(const sd_t *d, size_t new_max_size)
{
	RETURN_IF(!d || !sdx_dyn_st(d), 0);
	size_t curr_max_size = sdx_max_size(d);
	return	(curr_max_size <= 255 && new_max_size > 255) ||
		(curr_max_size > 255 && new_max_size <= 255) ?
		(new_max_size > curr_max_size ? 1 : -1) : 0;
}

S_INLINE size_t sd_alloc_size_raw(uint8_t header_size, size_t elem_size, size_t size, const sbool_t dyn_st)
{
	if (dyn_st && size < 255) {
		S_ASSERT(elem_size == 1);
		header_size = sizeof(struct SDataSmall);
	}
	return header_size + elem_size * size;
}

S_INLINE size_t sd_compute_max_elems(size_t buffer_size, size_t header_size, size_t elem_size)
{
	return header_size < buffer_size ? 0 :
		(buffer_size - header_size) / elem_size;
}

void sd_set_alloc_size(sd_t *d, const size_t alloc_size);
sd_t *sd_alloc(const uint8_t header_size, const size_t elem_size, const size_t initial_reserve, const sbool_t dyn_st, const size_t extra_tail_bytes);
sd_t *sd_alloc_into_ext_buf(void *buffer, const size_t max_size, const uint8_t header_size, const size_t elem_size, const sbool_t dyn_st);
void sd_free(sd_t **d);
void sd_free_va(sd_t **first, va_list ap);
void sd_reset(sd_t *d, const uint8_t header_size, const size_t elem_size, const size_t max_size, const sbool_t ext_buf, const sbool_t dyn_st);
size_t sd_grow(sd_t **d, const size_t extra_size, const size_t extra_tail_bytes);
size_t sd_reserve(sd_t **d, size_t max_size, const size_t extra_tail_bytes);
size_t sdx_reserve(sd_t **d, size_t max_size, uint8_t full_header_size, const size_t extra_tail_bytes);
sd_t *sd_shrink(sd_t **d, const size_t extra_tail_bytes);

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif	/* SDATA_H */

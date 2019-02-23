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
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 *
 * Observations:
 * - This is not intended for direct use, but as base for
 *   other types (srt_string, srt_vector, srt_tree, srt_map, etc.)
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

#define SD_BUILDFUNCS_COMMON(pfix, t, tail_bytes)                              \
	S_INLINE t *pfix##_shrink(t **c)                                       \
	{                                                                      \
		return (t *)sd_shrink((srt_data **)c, tail_bytes);             \
	}                                                                      \
	S_INLINE srt_bool pfix##_empty(const t *c)                             \
	{                                                                      \
		return pfix##_size(c) == 0 ? S_TRUE : S_FALSE;                 \
	}                                                                      \
	S_INLINE void pfix##_set_alloc_errors(t *c)                            \
	{                                                                      \
		sd_set_alloc_errors((srt_data *)c);                            \
	}                                                                      \
	S_INLINE void pfix##_reset_alloc_errors(t *c)                          \
	{                                                                      \
		sd_reset_alloc_errors((srt_data *)c);                          \
	}                                                                      \
	S_INLINE srt_bool pfix##_alloc_errors(t *c)                            \
	{                                                                      \
		return sd_alloc_errors((srt_data *)c);                         \
	}

#define SD_BUILDFUNCS_ST(pfix, t, stpfix)                                      \
	S_INLINE size_t pfix##_size(const t *c)                                \
	{                                                                      \
		return stpfix##_size((const srt_data *)c);                     \
	}                                                                      \
	S_INLINE void pfix##_set_size(t *c, size_t s)                          \
	{                                                                      \
		stpfix##_set_size((srt_data *)c, s);                           \
	}                                                                      \
	S_INLINE size_t pfix##_max_size(const t *c)                            \
	{                                                                      \
		return stpfix##_max_size((const srt_data *)c);                 \
	}                                                                      \
	S_INLINE size_t pfix##_alloc_size(const t *c)                          \
	{                                                                      \
		return stpfix##_alloc_size((const srt_data *)c);               \
	}                                                                      \
	S_INLINE size_t pfix##_get_buffer_size(const t *c)                     \
	{                                                                      \
		return stpfix##_size((const srt_data *)c)                      \
		       * stpfix##_elem_size((const srt_data *)c);              \
	}                                                                      \
	S_INLINE void *pfix##_elem_addr(t *c, size_t pos)                      \
	{                                                                      \
		return stpfix##_elem_addr((srt_data *)c, pos);                 \
	}                                                                      \
	S_INLINE const void *pfix##_elem_addr_r(const t *c, size_t pos)        \
	{                                                                      \
		return stpfix##_elem_addr_r((const srt_data *)c, pos);         \
	}                                                                      \
	S_INLINE size_t pfix##_capacity(const t *c)                            \
	{                                                                      \
		return stpfix##_max_size((const srt_data *)c);                 \
	}                                                                      \
	S_INLINE size_t pfix##_capacity_left(const t *c)                       \
	{                                                                      \
		return stpfix##_max_size((const srt_data *)c)                  \
		       - stpfix##_size((const srt_data *)c);                   \
	}                                                                      \
	S_INLINE size_t pfix##_len(const t *c)                                 \
	{                                                                      \
		return stpfix##_size((const srt_data *)c);                     \
	}

#define SD_BUILDFUNCS_ST2(pfix, t, stpfix)                                     \
	S_INLINE uint8_t *pfix##_get_buffer(t *c)                              \
	{                                                                      \
		return stpfix##_get_buffer((srt_data *)c);                     \
	}                                                                      \
	S_INLINE const uint8_t *pfix##_get_buffer_r(const t *c)                \
	{                                                                      \
		return stpfix##_get_buffer_r((const srt_data *)c);             \
	}

#define SD_BUILDFUNCS_DYN_ST(pfix, t, tail_bytes)                              \
	SD_BUILDFUNCS_ST(pfix, t, sdx)                                         \
	SD_BUILDFUNCS_COMMON(pfix, t, tail_bytes)

#define SD_BUILDFUNCS_FULL_ST(pfix, t, tail_bytes)                             \
	SD_BUILDFUNCS_ST(pfix, t, sd)                                          \
	SD_BUILDFUNCS_ST2(pfix, t, sd)                                         \
	SD_BUILDFUNCS_COMMON(pfix, t, tail_bytes)                              \
	S_INLINE size_t pfix##_grow(t **c, size_t extra_elems)                 \
	{                                                                      \
		return sd_grow((srt_data **)c, extra_elems, tail_bytes);       \
	}                                                                      \
	S_INLINE size_t pfix##_reserve(t **c, size_t max_elems)                \
	{                                                                      \
		return sd_reserve((srt_data **)c, max_elems, tail_bytes);      \
	}

#define SD_FREE_AUX(pfix, t)                                                   \
	S_INLINE void pfix##_free_aux(t **c, ...)                              \
	{                                                                      \
		va_list ap;                                                    \
		va_start(ap, c);                                               \
		sd_free_va((srt_data **)c, ap);                                \
		va_end(ap);                                                    \
	}

#define SD_BUILDFUNCS_DYN(pfix, t, tail_bytes)                                 \
	SD_BUILDFUNCS_DYN_ST(pfix, t, tail_bytes)                              \
	SD_FREE_AUX(pfix, t)

#define SD_BUILDFUNCS_FULL(pfix, t, tail_bytes)                                \
	SD_BUILDFUNCS_FULL_ST(pfix, t, tail_bytes)                             \
	SD_FREE_AUX(pfix, t)

/*
 * Data structures and types
 */

enum SDataStMode {
	SData_Full = 0,
	SData_DynFull = 1,
	SData_DynSmall = 2,
	SData_VoidData = 3
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

struct SDataFull /* 20-byte structure (32-bit compiler), 40-byte (64-bit c.) */
{
	struct SDataFlags f;

	/*
	 * Type-specific configuration
	 */
	uint8_t sub_type;

	/*
	 * Header size: struct SData size plus additional header from type
	 * build on top of it.
	 */
	size_t header_size;

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

typedef struct SDataFull srt_data; /* Opaque structure (accessors are provided) */

#define EMPTY_SDataFlags	{ 1, 1, 3, 0, 0, 0, 0 }
#define EMPTY_SDataSmall	{ EMPTY_SDataFlags, 0, 0, 0 }
#define EMPTY_SDataFull		{ EMPTY_SDataFlags, 0, 0, 0, 0, 0 }

extern srt_data *sd_void;

/*
 * Functions
 */

S_INLINE void sd_set_alloc_errors(srt_data *d)
{
	if (d)
		d->f.alloc_errors = 1;
}

S_INLINE void sd_reset_alloc_errors(srt_data *d)
{
	if (d)
		d->f.alloc_errors = 0;
}

S_INLINE srt_bool sd_alloc_errors(const srt_data *d)
{
	return (!d || d->f.alloc_errors) ? S_TRUE : S_FALSE;
}

S_INLINE size_t sd_size(const srt_data *d)
{
	RETURN_IF(!d, 0);
	return d->size;
}

S_INLINE void sd_set_size(srt_data *d, size_t size)
{
	if (d)
		d->size = size;
}

S_INLINE void sd_set_max_size(srt_data *d, size_t max_size)
{
	if (d)
		d->max_size = max_size;
}

S_INLINE size_t sd_max_size(const srt_data *d)
{
	RETURN_IF(!d, 0);
	return d->max_size;
}

S_INLINE uint8_t *sd_get_buffer(srt_data *d)
{
	return !d ? NULL : (uint8_t *)d + d->header_size;
}

S_INLINE const uint8_t *sd_get_buffer_r(const srt_data *d)
{
	return !d ? NULL : (const uint8_t *)d + d->header_size;
}

S_INLINE void *sd_elem_addr(srt_data *d, size_t pos)
{
	return !d ? NULL : ((char *)d + d->header_size + d->elem_size * pos);
}

S_INLINE const void *sd_elem_addr_r(const srt_data *d, size_t pos)
{
	return !d ? NULL
		  : ((const char *)d + d->header_size + d->elem_size * pos);
}

S_INLINE size_t sd_elem_size(const srt_data *d)
{
	RETURN_IF(!d, 0);
	return d->elem_size;
}

S_INLINE size_t sd_alloc_size(const srt_data *d)
{
	RETURN_IF(!d, 0);
	return d->header_size + d->elem_size * d->max_size;
}

S_INLINE uint8_t sdx_szt_to_u8_sat(size_t v)
{
	return v <= 255 ? (uint8_t)v : 255;
}

S_INLINE srt_bool sdx_full_st(const srt_data *d)
{
	return d && d->f.st_mode <= SData_DynFull ? S_TRUE : S_FALSE;
}

S_INLINE srt_bool sdx_dyn_st(const srt_data *d)
{
	RETURN_IF(!d, S_FALSE);
	return d->f.st_mode == SData_DynFull || d->f.st_mode == SData_DynSmall
		       ? S_TRUE
		       : S_FALSE;
}

S_INLINE void sdx_set_max_size(srt_data *d, size_t max_size)
{
	if (d) {
		if (sdx_full_st(d))
			d->max_size = max_size;
		else
			((struct SDataSmall *)d)->max_size =
				sdx_szt_to_u8_sat(max_size);
	}
}

S_INLINE size_t sdx_size(const srt_data *d)
{
	RETURN_IF(!d, 0);
	if (sdx_full_st(d))
		return sd_size(d);
	return ((const struct SDataSmall *)d)->size;
}

S_INLINE void sdx_set_size(srt_data *d, size_t size)
{
	if (d) {
		if (sdx_full_st(d))
			sd_set_size(d, size);
		else
			((struct SDataSmall *)d)->size =
				sdx_szt_to_u8_sat(size);
	}
}

S_INLINE size_t sdx_max_size(const srt_data *d)
{
	RETURN_IF(!d, 0);
	if (sdx_full_st(d))
		return sd_max_size(d);
	return ((const struct SDataSmall *)d)->max_size;
}

S_INLINE uint8_t *sdx_get_buffer(srt_data *d)
{
	return !d ? NULL
		  : (uint8_t *)d
			       + (sdx_full_st(d) ? d->header_size
						 : sizeof(struct SDataSmall));
}

S_INLINE const uint8_t *sdx_get_buffer_r(const srt_data *d)
{
	return !d ? NULL
		  : (const uint8_t *)d
			       + (sdx_full_st(d) ? d->header_size
						 : sizeof(struct SDataSmall));
}

S_INLINE void *sdx_elem_addr(srt_data *d, size_t pos)
{
	return !d ? NULL
		  : (char *)d
			       + (sdx_full_st(d)
					  ? d->header_size + d->elem_size * pos
					  : sizeof(struct SDataSmall)
						    + pos); /* BEHAVIOR */
}

S_INLINE const void *sdx_elem_addr_r(const srt_data *d, size_t pos)
{
	return !d ? NULL
		  : (const char *)d
			       + (sdx_full_st(d)
					  ? d->header_size + d->elem_size * pos
					  : sizeof(struct SDataSmall)
						    + pos); /* BEHAVIOR */
}

S_INLINE size_t sdx_header_size(const srt_data *d)
{
	RETURN_IF(!d, 0);
	RETURN_IF(sdx_full_st(d), d->header_size);
	return sizeof(struct SDataSmall); /* Implicit value */
}

S_INLINE size_t sdx_elem_size(const srt_data *d)
{
	RETURN_IF(!d, 0);
	RETURN_IF(sdx_full_st(d), d->elem_size);
	return 1; /* Implicit value */
}

S_INLINE size_t sdx_alloc_size(const srt_data *d)
{
	return sdx_header_size(d) + sdx_elem_size(d) * sdx_max_size(d);
}

/* Checks if structure container switch is required
 * -1: full to small
 * 0: no changes
 * 1: small to full
 */
S_INLINE int sdx_chk_st_change(const srt_data *d, size_t new_max_size)
{
	size_t curr_max_size;
	RETURN_IF(!d || !sdx_dyn_st(d), 0);
	curr_max_size = sdx_max_size(d);
	return (curr_max_size <= 255 && new_max_size > 255)
			       || (curr_max_size > 255 && new_max_size <= 255)
		       ? (new_max_size > curr_max_size ? 1 : -1)
		       : 0;
}

S_INLINE size_t sd_alloc_size_raw(size_t header_size, size_t elem_size,
				  size_t size, srt_bool dyn_st)
{
	S_ASSERT(header_size != 0);
	RETURN_IF(!header_size, 0);
	if (dyn_st && size < 255) {
		S_ASSERT(elem_size == 1);
		header_size = sizeof(struct SDataSmall);
	}
	return header_size + elem_size * size;
}

S_INLINE size_t sd_compute_max_elems(size_t buffer_size, size_t header_size,
				     size_t elem_size)
{
	return header_size < buffer_size
		       ? 0
		       : (buffer_size - header_size) / elem_size;
}

void sd_set_alloc_size(srt_data *d, size_t alloc_size);
srt_data *sd_alloc(size_t header_size, size_t elem_size, size_t initial_reserve, srt_bool dyn_st, size_t extra_tail_bytes);
srt_data *sd_alloc_into_ext_buf(void *buffer, size_t max_size, size_t header_size, size_t elem_size, srt_bool dyn_st);
void sd_free(srt_data **d);
void sd_free_va(srt_data **first, va_list ap);
void sd_reset(srt_data *d, size_t header_size, size_t elem_size, size_t max_size, srt_bool ext_buf, srt_bool dyn_st);
size_t sd_grow(srt_data **d, size_t extra_size, size_t extra_tail_bytes);
size_t sd_reserve(srt_data **d, size_t max_size, size_t extra_tail_bytes);
size_t sdx_reserve(srt_data **d, size_t max_size, size_t full_header_size, size_t extra_tail_bytes);
srt_data *sd_shrink(srt_data **d, size_t extra_tail_bytes);

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* SDATA_H */

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
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 *
 * Observations:
 * - This is not intended for direct use, but as base for 
 *   sstring, svector, stree, smap.
 */

#include "scommon.h"

/*
 * Allocation heuristic configuration
 *
 * Define SD_ENABLE_HEURISTIC_GROW for enabling pre-allocation heuristics.
 */

#define SD_ENABLE_HEURISTIC_GROW

/*
 * Macros
 */

#define S_ALLOC_SMALL		511
#define S_ALLOC_FULL		((size_t)~0)

#define SD_BUILDPROTS(pfix)						\
	void pfix##_free_aux(const size_t nargs, pfix##_t **c, ...);	\
	size_t pfix##_grow(pfix##_t **c, const size_t extra_elems);	\
	size_t pfix##_reserve(pfix##_t **c, const size_t max_elems);	\
	pfix##_t *pfix##_shrink(pfix##_t **c);				\
	size_t pfix##_size(const pfix##_t *c);				\
	void pfix##_set_size(pfix##_t *c, const size_t ss);		\
	size_t pfix##_len(const pfix##_t *c);

#define SD_BUILDFUNCS(pfix)						\
	void pfix##_free_aux(const size_t nargs, pfix##_t **c, ...) {	\
		va_list ap;						\
		va_start(ap, c);					\
		sd_free_va(nargs, (sd_t **)c, ap);			\
		va_end(ap);						\
	}								\
	size_t pfix##_grow(pfix##_t **c, const size_t extra_elems) {	\
		ASSERT_RETURN_IF(!c || *c == pfix##_void, 0);		\
		return sd_grow((sd_t **)c, extra_elems, &pfix##f);	\
	}								\
	size_t pfix##_reserve(pfix##_t **c, const size_t max_elems) {	\
		ASSERT_RETURN_IF(!c || *c == pfix##_void, 0);		\
		return sd_reserve((sd_t **)c, max_elems, &pfix##f);	\
	}								\
	pfix##_t *pfix##_shrink(pfix##_t **c) {				\
		return (pfix##_t *)sd_shrink((sd_t **)c, &pfix##f);	\
	}								\
	size_t pfix##_size(const pfix##_t *c) {				\
		return sd_get_size((const sd_t *)c);			\
	}								\
	void pfix##_set_size(pfix##_t *c, const size_t s) {		\
		sd_set_size((sd_t *)c, s);				\
	}								\
	size_t pfix##_len(const pfix##_t *c) {				\
		return sd_get_size((const sd_t *)c);				\
	}								\

/*
 * Artificial memory allocation limits (tests/debug)
 */

#if defined(SD_MAX_MALLOC_SIZE) && !defined(SD_DEBUG_ALLOC)
static void *__sd_malloc(size_t size)
{
	return size <= SD_MAX_MALLOC_SIZE ? malloc(size) : NULL;
}
static void *__sd_calloc(size_t nmemb, size_t size)
{
	return size <= SD_MAX_MALLOC_SIZE ? calloc(nmemb, size) : NULL;
}
static void *__sd_realloc(void *ptr, size_t size)
{
	return size <= SD_MAX_MALLOC_SIZE ? realloc(ptr, size) : NULL;
}
#elif defined(SD_DEBUG_ALLOC)
#ifndef SD_MAX_MALLOC_SIZE
#define SD_MAX_MALLOC_SIZE ((size_t)-1)
#endif
static void *__sd_alloc_check(void *p, size_t size, char *from_label)
{
	fprintf(stderr, "[libsrt %s @%s] alloc size: " FMT_ZU "\n",
		(p ? "OK" : "ERROR"), from_label, size);
	return p;
}
static void *__sd_malloc(size_t size)
{
	void *p = size <= SD_MAX_MALLOC_SIZE ? malloc(size) : NULL;
	return __sd_alloc_check(p, size, "malloc");
}
static void *__sd_calloc(size_t nmemb, size_t size)
{
	void *p = size <= SD_MAX_MALLOC_SIZE ? calloc(nmemb, size) : NULL;
	return __sd_alloc_check(p, nmemb * size, "calloc");
}
static void *__sd_realloc(void *ptr, size_t size)
{
	void *p = size <= SD_MAX_MALLOC_SIZE ? realloc(ptr, size) : NULL;
	return __sd_alloc_check(p, size, "realloc");
}
#else
#define __sd_malloc malloc
#define __sd_calloc calloc
#define __sd_realloc realloc
#endif
#define __sd_free free

/*
 * Generic data structures
 */

struct SData
{
	unsigned char is_full : 1; /* 0: small container, 1 : full container */
	unsigned char ext_buffer : 1;
	unsigned char size_h : 1;
	unsigned char alloc_size_h : 1;
	unsigned char alloc_errors : 1;
	unsigned char other1 : 1;
	unsigned char other2 : 1;
	unsigned char other3 : 1;
};

struct SData_Small
{
        struct SData d;
        unsigned char size_l, alloc_size_l;
};

struct SData_Full
{
	struct SData d;
	unsigned char unused[3];
        size_t size, alloc_size;
};

typedef struct SData sd_t; /* "Hidden" structure (accessors are provided) */

struct sd_conf
{
	size_t (*sx_get_max_size)(const sd_t *);
	void (*sx_reset)(sd_t *s, const size_t alloc_size, const sbool_t ext_buf);
	void (*sx_reconfig)(sd_t *, size_t new_alloc_size, const size_t new_mt_size);
	sd_t *(*sx_alloc)(const size_t initial_reserve);
	sd_t *(*sx_dup)(const sd_t *src);
	sd_t *(*sx_check)(sd_t **s);
	sd_t *sx_void;
	size_t alloc_small;
	size_t range_small;
	size_t metainfo_small;
	size_t metainfo_full;
	/* Fixed size elements (ss_t): */
	size_t fixed_elem_size;
	/* Variable size objects (sv_t): */
	size_t header_size;
	size_t elem_size_off;
};

#define EMPTY_SData_IF0 { 0, 1, 0, 0, 1, 0, 0, 0 }
#define EMPTY_SData_IF1 { 1, 1, 0, 0, 1, 0, 0, 0 }
#define EMPTY_SData_Small(alloc_size) { EMPTY_SData_IF0, 0, alloc_size }
#define EMPTY_SData_Full(alloc_size) { EMPTY_SData_IF1, { 0, 0, 0 }, 0, alloc_size }

/*
 * Functions
 */

S_INLINE void sd_set_alloc_errors(sd_t *d)
{
	if (d)
		d->alloc_errors = 1;
}

S_INLINE void sd_reset_alloc_errors(sd_t *d)
{
	if (d)
		d->alloc_errors = 0;
}

size_t sd_alloc_size_to_mt_size(const size_t a_size, const struct sd_conf *f);
sbool_t sd_alloc_size_to_is_big(const size_t s, const struct sd_conf *f);
size_t sd_get_size(const sd_t *s);
size_t sd_get_alloc_size(const sd_t *d);
void sd_set_size(sd_t *d, const size_t size);
void sd_set_alloc_size(sd_t *d, const size_t alloc_size);
sd_t *sd_alloc(const size_t header_size, const size_t elem_size, const size_t initial_reserve, const struct sd_conf *f);
sd_t *sd_alloc_into_ext_buf(void *buffer, const size_t buffer_size, const struct sd_conf *f);
void sd_free(sd_t **d);
void sd_free_va(const size_t elems, sd_t **first, va_list ap);
void sd_reset(sd_t *d, const sbool_t use_big_struct, const size_t alloc_size, const sbool_t ext_buf);
size_t sd_grow(sd_t **d, const size_t extra_size, const struct sd_conf *f);
size_t sd_reserve(sd_t **d, size_t max_size, const struct sd_conf *f);
sd_t *sd_shrink(sd_t **d, const struct sd_conf *f);

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif	/* SDATA_H */


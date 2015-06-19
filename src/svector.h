#ifndef SVECTOR_H
#define SVECTOR_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * svector.h
 *
 * Vector handling.
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */ 

#include "sdata.h"

/*
 * Macros
 */

#define SDV_HEADER_SIZE sizeof(struct SVector)
#define SV_SIZE_TO_ALLOC_SIZE(elems, elem_size) \
	(SDV_HEADER_SIZE + elem_size * elems)

/*
 * Structures
 */

enum eSV_Type
{
	SV_FIRST,
	SV_I8 = SV_FIRST,
	SV_U8,
	SV_I16,
	SV_U16,
	SV_I32,
	SV_U32,
	SV_I64,
	SV_U64,
	SV_GEN,
	SV_LAST = SV_GEN
};

struct SVector
{
	struct SData_Full df;
	char sv_type;
	size_t elem_size;
	size_t aux, aux2;	 /* Used by derived types */
};

typedef struct SVector sv_t; /* "Hidden" structure (accessors are provided) */

#define EMPTY_SV { EMPTY_SData_Full(sizeof(struct SVector)), SV_U8, 0 }

/*
 * Variable argument functions
 */

#ifdef S_USE_VA_ARGS
#define sv_free(...) sv_free_aux(S_NARGS_SVPW(__VA_ARGS__), __VA_ARGS__)
#define sv_cat(v, ...) sv_cat_aux(v, S_NARGS_SVR(__VA_ARGS__), __VA_ARGS__)
#define sv_push(v, ...) sv_push_aux(v, S_NARGS_R(__VA_ARGS__), __VA_ARGS__)
#else
#define sv_free(...)
#define sv_cat(v, ...) NULL
#define sv_push(...) S_FALSE
#endif

/*
 * Allocation
 */

#define sv_alloca(elem_size, num_elems)					\
	sv_alloc_raw(							\
		SV_GEN, elem_size, S_TRUE,				\
		alloca(SV_SIZE_TO_ALLOC_SIZE(num_elems,	elem_size)),	\
		SV_SIZE_TO_ALLOC_SIZE(num_elems, elem_size))
#define sv_alloca_t(type, num_elems)					      \
	sv_alloc_raw(							      \
		type, sv_elem_size(type), S_TRUE,			      \
		alloca(SV_SIZE_TO_ALLOC_SIZE(num_elems, sv_elem_size(type))), \
		SV_SIZE_TO_ALLOC_SIZE(num_elems, sv_elem_size(type)))
sv_t *sv_alloc_raw(const enum eSV_Type t, const size_t elem_size, const sbool_t ext_buf, void *buffer, const size_t buffer_size);
sv_t *sv_alloc(const size_t elem_size, const size_t initial_num_elems_reserve);
sv_t *sv_alloc_t(const enum eSV_Type t, const size_t initial_num_elems_reserve);

SD_BUILDPROTS(sv)

/*
 * Accessors
 */

size_t sv_capacity(const sv_t *v);
size_t sv_len_left(const sv_t *v);
sbool_t sv_set_len(sv_t *v, const size_t elems);
void *sv_get_buffer(sv_t *v);
const void *sv_get_buffer_r(const sv_t *v);
size_t sv_get_buffer_size(const sv_t *v);
size_t sv_elem_size(const enum eSV_Type t);

/*
 * Allocation from other sources: "dup"
 */

sv_t *sv_dup(const sv_t *src);
sv_t *sv_dup_erase(const sv_t *src, const size_t off, const size_t n);
sv_t *sv_dup_resize(const sv_t *src, const size_t n);

/*
 * Assignment
 */

sv_t *sv_cpy(sv_t **v, const sv_t *src);
sv_t *sv_cpy_erase(sv_t **v, const sv_t *src, const size_t off, const size_t n);
sv_t *sv_cpy_resize(sv_t **v, const sv_t *src, const size_t n);

/*
 * Append
 */

sv_t *sv_cat_aux(sv_t **v, const size_t nargs, const sv_t *v1, ...);
sv_t *sv_cat_erase(sv_t **v, const sv_t *src, const size_t off, const size_t n);
sv_t *sv_cat_resize(sv_t **v, const sv_t *src, const size_t n);

/*
 * Transformation
 */

sv_t *sv_erase(sv_t **v, const size_t off, const size_t n);
sv_t *sv_resize(sv_t **v, const size_t n);

/*
 * Search
 */

size_t sv_find(const sv_t *v, const size_t off, const void *target);
size_t sv_find_i(const sv_t *v, const size_t off, const sint_t target);
size_t sv_find_u(const sv_t *v, const size_t off, const suint_t target);

/*
 * Compare
 */

int sv_ncmp(const sv_t *v1, const size_t v1off, const sv_t *v2, const size_t v2off, const size_t n);

/*
 * Vector "at": element access to given position
 */

const void *sv_at(const sv_t *v, const size_t index);
sint_t sv_i_at(const sv_t *v, const size_t index);
suint_t sv_u_at(const sv_t *v, const size_t index);

/*
 * Vector "push": add element in the last position
 */

sbool_t sv_push_raw(sv_t **v, const void *src, const size_t n);
size_t sv_push_aux(sv_t **v, const size_t nargs, const void *c1, ...);
sbool_t sv_push_i(sv_t **v, const sint_t c);
sbool_t sv_push_u(sv_t **v, const suint_t c);

/*
 * Vector "pop": extract element from last position
 */

void *sv_pop(sv_t *v);
sint_t sv_pop_i(sv_t *v);
suint_t sv_pop_u(sv_t *v);

/*
 * Functions intended for helping compiler optimization
 */

static size_t __sv_get_max_size(const sv_t *v)
{
	return (v->df.alloc_size - SDV_HEADER_SIZE) / v->elem_size;
}

static const void *__sv_get_buffer_r(const sv_t *v)
{
	return (const void *)(((const char *)v) + SDV_HEADER_SIZE);
}

static void *__sv_get_buffer(sv_t *v)
{
	return (void *)(((char *)v) + SDV_HEADER_SIZE);
}

#ifdef __cplusplus
};      /* extern "C" { */
#endif
#endif	/* SVECTOR_H */


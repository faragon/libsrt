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

/*
#API: |Allocate typed vector (stack)|Vector type: SV_I8/SV_U8/SV_I16/SV_U16/SV_I32/SV_U32/SV_I64/SV_U64; space preallocated to store n elements|vector|O(1)|
sv_t *sv_alloca_t(const enum eSV_Type t, const size_t initial_num_elems_reserve)

#API: |Allocate generic vector (stack)|element size; space preallocated to store n elements|vector|O(1)|
sv_t *sv_alloca(const size_t elem_size, const size_t initial_num_elems_reserve)
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

/* #API: |Allocate generic vector (heap)|element size; space preallocated to store n elements|vector|O(1)| */
sv_t *sv_alloc(const size_t elem_size, const size_t initial_num_elems_reserve);

/* #API: |Allocate typed vector (heap)|Vector type: SV_I8/SV_U8/SV_I16/SV_U16/SV_I32/SV_U32/SV_I64/SV_U64; space preallocated to store n elements|vector|O(1)| */
sv_t *sv_alloc_t(const enum eSV_Type t, const size_t initial_num_elems_reserve);

SD_BUILDPROTS(sv)

/*
#API: |Free one or more vectors (heap)|vector;more vectors (optional)||O(1)|
void sv_free(sv_t **c, ...)

#API: |Ensure space for extra elements|vector;number of extra eelements|extra size allocated|O(1)|
size_t sv_grow(sv_t **c, const size_t extra_elems)

#API: |Ensure space for elements|vector;absolute element reserve|reserved elements|O(1)|
size_t sv_reserve(sv_t **c, const size_t max_elems)

#API: |Free unused space|vector|same vector (optional usage)|O(1)|
sv_t *sv_shrink(sv_t **c)

#API: |Get vector size|vector|vector bytes used in UTF8 format|O(1)|
size_t sv_size(const sv_t *c)

#API: |Set vector size (bytes used in UTF8 format)|vector;set vector number of elements||O(1)|
void sv_set_size(sv_t *c, const size_t s)

#API: |Equivalent to sv_size|vector|Number of bytes (UTF-8 vector length)|O(1)|
size_t sv_len(const sv_t *c)

#API: |Allocate vector (stack)|space preallocated to store n elements|allocated vector|O(1)|
sv_t *sv_alloca(const size_t initial_reserve)
*/

/*
 * Accessors
 */

/* #API: |Allocated space|vector|current allocated space (vector elements)|O(1)| */
size_t sv_capacity(const sv_t *v);

/* #API: |Preallocated space left|vector|allocated space left|O(1)| */
size_t sv_len_left(const sv_t *v);

/* #API: |Explicit set length (intended for external I/O raw acccess)|vector;new length||O(1)| */
sbool_t sv_set_len(sv_t *v, const size_t elems);

/* #API: |Get string buffer read-only access|string|pointer to the insternal string buffer (UTF-8 or raw data)|O(1)| */
void *sv_get_buffer(sv_t *v);

/* #API: |Get string buffer access|string|pointer to the insternal string buffer (UTF-8 or raw data)|O(1)| */
const void *sv_get_buffer_r(const sv_t *v);

/* #API: |Get buffer size|vector|Number of bytes in use for current vector elements|O(1)| */
size_t sv_get_buffer_size(const sv_t *v);

/* #API: |Get vector type element size|vector type|Element size (bytes)|O(1)| */
size_t sv_elem_size(const enum eSV_Type t);

/*
 * Allocation from other sources: "dup"
 */

/* #API: |Duplicate vector|vector|output vector|O(n)| */
sv_t *sv_dup(const sv_t *src);

/* #API: |Duplicate vector portion|vector; offset start; number of elements to take|output vector|O(n)| */
sv_t *sv_dup_erase(const sv_t *src, const size_t off, const size_t n);

/* #API: |Duplicate vector with resize operation|vector; size for the output|output vector|O(n)| */
sv_t *sv_dup_resize(const sv_t *src, const size_t n);

/*
 * Assignment
 */

/* #API: |Overwrite vector with a vector copy|output vector; input vector|output vector reference (optional usage)|O(n)| */
sv_t *sv_cpy(sv_t **v, const sv_t *src);

/* #API: |Overwrite vector with input vector copy applying a erase operation|output vector; input vector; input vector erase start offset; number of elements to erase|output vector reference (optional usage)|O(n)| */
sv_t *sv_cpy_erase(sv_t **v, const sv_t *src, const size_t off, const size_t n);

/* #API: |Overwrite vector with input vector copy plus resize operation|output vector; input vector; number of elements of input vector|output vector reference (optional usage)|O(n)| */
sv_t *sv_cpy_resize(sv_t **v, const sv_t *src, const size_t n);

/*
 * Append
 */

/*
#API: |Concatenate vectors to vector|vector; first vector to be concatenated; optional: N additional vectors to be concatenated|output vector reference (optional usage)|O(n)|
sv_t *sv_cat(sv_t **v, const sv_t *v1, ...)
*/
sv_t *sv_cat_aux(sv_t **v, const size_t nargs, const sv_t *v1, ...);

/* #API: |Concatenate vector with erase operation|output vector; input vector; input vector offset for erase start; erase element count|output vector reference (optional usage)|O(n)| */
sv_t *sv_cat_erase(sv_t **v, const sv_t *src, const size_t off, const size_t n);

/* #API: |Concatenate vector with input vector copy plus resize operation|output vector; input vector; number of elements of input vector|output vector reference (optional usage)|O(n)| */
sv_t *sv_cat_resize(sv_t **v, const sv_t *src, const size_t n);

/*
 * Transformation
 */

/* #API: |Erase portion of a vector|input/output vector; element offset where to start the cut; number of elements to be cut|output vector reference (optional usage)|O(n)| */
sv_t *sv_erase(sv_t **v, const size_t off, const size_t n);

/* #API: |Resize vector|input/output vector; new size|output vector reference (optional usage)|O(n)| */
sv_t *sv_resize(sv_t **v, const size_t n);

/*
 * Search
 */

/* #API: |Find value in vector (generic data)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)| */
size_t sv_find(const sv_t *v, const size_t off, const void *target);

/* #API: |Find value in vector (integer)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)| */
size_t sv_find_i(const sv_t *v, const size_t off, const sint_t target);

/* #API: |Find value in vector (unsigned integer)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)| */
size_t sv_find_u(const sv_t *v, const size_t off, const suint_t target);

/*
 * Compare
 */

/* #API: |Compare two vectors|vector #1; vector #1 offset start; vector #2; vector #2 start; compare size|0: equals; < 0 if a < b; > 0 if a > b|O(n)| */
int sv_ncmp(const sv_t *v1, const size_t v1off, const sv_t *v2, const size_t v2off, const size_t n);

/*
 * Vector "at": element access to given position
 */

/* #API: |Vector random access (generic data)|vector; location|NULL: not found; != NULL: element reference|O(1)| */
const void *sv_at(const sv_t *v, const size_t index);

/* #API: |Vector random access (integer)|vector; location|Element value|O(1)| */
sint_t sv_i_at(const sv_t *v, const size_t index);

/* #API: |Vector random access (unsigned integer)|vector; location|Element value|O(1)| */
suint_t sv_u_at(const sv_t *v, const size_t index);

/*
 * Vector "push": add element in the last position
 */

/* #API: |Push/add element (generic data)|vector; data source; number of elements|S_TRUE: added OK; S_FALSE: not enough memory|O(1)| */
sbool_t sv_push_raw(sv_t **v, const void *src, const size_t n);

/*
#API: |Push/add multiple elements (generic data)|vector; new element to be added; more elements to be added (optional)|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|
sbool_t sv_push(sv_t **v, const void *c1, ...)
*/
size_t sv_push_aux(sv_t **v, const size_t nargs, const void *c1, ...);

/* #API: |Push/add element (integer)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)| */
sbool_t sv_push_i(sv_t **v, const sint_t c);

/* #API: |Push/add element (unsigned integer)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)| */
sbool_t sv_push_u(sv_t **v, const suint_t c);

/*
 * Vector "pop": extract element from last position
 */

/* #API: |Pop/extract element (generic data)|vector|Element reference|O(1)| */
void *sv_pop(sv_t *v);

/* #API: |Pop/extract element (integer)|vector|Integer element|O(1)| */
sint_t sv_pop_i(sv_t *v);

/* #API: |Pop/extract element (unsigned integer)|vector|Integer element|O(1)| */
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


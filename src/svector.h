#ifndef SVECTOR_H
#define SVECTOR_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * svector.h
 *
 * #SHORTDOC vector handling
 *
 * #DOC Vector handling functions for managing integer data and
 * #DOC generic data.
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */ 

#include "saux/sdata.h"

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
	SV_LAST_INT = SV_U64,
	SV_GEN
};

typedef int (*sv_cmp_t)(const void *a, const void *b);

struct SVector
{
	struct SDataFull d;
	union {
		sv_cmp_t cmpf;	/* compare function */
		uintptr_t cnt;	/* counter (for derivative types, e.g. sb_t) */
	} vx;
};

typedef struct SVector sv_t; /* "Hidden" structure (accessors are provided) */

#define EMPTY_SV { EMPTY_SData(sizeof(sv_t)), NULL }

/*
 * Variable argument functions
 */

#ifdef S_USE_VA_ARGS
#define sv_free(...) sv_free_aux(__VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#define sv_cat(v, ...) sv_cat_aux(v, __VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#define sv_push(v, ...) sv_push_aux(v, __VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#else
#define sv_free(...)
#define sv_cat(v, ...) NULL
#define sv_push(...) S_FALSE
#endif

/*
 * Allocation
 */

/*
#API: |Allocate typed vector (stack)|Vector type: SV_I8/SV_U8/SV_I16/SV_U16/SV_I32/SV_U32/SV_I64/SV_U64; space preallocated to store n elements|vector|O(1)|1;2|
sv_t *sv_alloca_t(const enum eSV_Type t, const size_t initial_num_elems_reserve)

#API: |Allocate generic vector (stack)|element size; space preallocated to store n elements; compare function (used for sorting, pass NULL for none)|vector|O(1)|1;2|
sv_t *sv_alloca(const size_t elem_size, const size_t initial_num_elems_reserve, const sv_cmp_t f)
*/
#define sv_alloca(elem_size, max_size, cmp_f)				\
	sv_alloc_raw(SV_GEN, S_TRUE,					\
		     alloca(sd_alloc_size_raw(sizeof(sv_t), elem_size,	\
					      max_size, S_FALSE)),	\
		     elem_size, max_size, cmp_f)
#define sv_alloca_t(type, max_size)					       \
	sv_alloc_raw(type, S_TRUE,					       \
		     alloca(sd_alloc_size_raw(sizeof(sv_t), sv_elem_size(type),\
			    max_size, S_FALSE)),			       \
		     sv_elem_size(type), max_size, NULL)

sv_t *sv_alloc_raw(const enum eSV_Type t, const sbool_t ext_buf, void *buffer, const size_t elem_size, const size_t max_size, const sv_cmp_t f);

/* #API: |Allocate generic vector (heap)|element size; space preallocated to store n elements; compare function (used for sorting, pass NULL for none)|vector|O(1)|1;2| */
sv_t *sv_alloc(const size_t elem_size, const size_t initial_num_elems_reserve, const sv_cmp_t f);

/* #API: |Allocate typed vector (heap)|Vector type: SV_I8/SV_U8/SV_I16/SV_U16/SV_I32/SV_U32/SV_I64/SV_U64; space preallocated to store n elements|vector|O(1)|1;2| */
sv_t *sv_alloc_t(const enum eSV_Type t, const size_t initial_num_elems_reserve);

SD_BUILDFUNCS_FULL(sv, 0)

/*
#API: |Allocate vector (stack)|space preallocated to store n elements|allocated vector|O(1)|1;2|
sv_t *sv_alloca(const size_t initial_reserve)

#API: |Free one or more vectors (heap)|vector;more vectors (optional)|-|O(1)|1;2|
void sv_free(sv_t **v, ...)

#API: |Ensure space for extra elements|vector;number of extra eelements|extra size allocated|O(1)|1;2|
size_t sv_grow(sv_t **v, const size_t extra_elems)

#API: |Ensure space for elements|vector;absolute element reserve|reserved elements|O(1)|1;2|
size_t sv_reserve(sv_t **v, const size_t max_elems)

#API: |Free unused space|vector|same vector (optional usage)|O(1)|1;2|
sv_t *sv_shrink(sv_t **v)

#API: |Get vector size|vector|vector number of elements|O(1)|1;2|
size_t sv_size(const sv_t *v)

#NOTAPI: |Set vector size (number of vector elements)|vector;set vector number of elements|-|O(1)|1;2|
void sv_set_size(sv_t *v, const size_t s)

#API: |Equivalent to sv_size|vector|Number of vector elements|O(1)|1;2|
size_t sv_len(const sv_t *v)

#API: |Allocated space|vector|current allocated space (vector elements)|O(1)|1;2|
size_t sv_capacity(const sv_t *v);

#API: |Preallocated space left|vector|allocated space left|O(1)|1;2|
size_t sv_capacity_left(const sv_t *v);

#API: |Tells if a vector is empty (zero elements)|vector|S_TRUE: empty vector; S_FALSE: not empty|O(1)|1;2|
sbool_t sv_empty(const sv_t *v)

#API: |Get vector buffer access|vector|pointer to the internal vector buffer (raw data)|O(1)|1;2|
char *sv_get_buffer(sv_t *v);

#API: |Get vector buffer access (read-only)|vector|pointer to the internal vector buffer (raw data)|O(1)|1;2|
const char *sv_get_buffer_r(const sv_t *v);

#API: |Get vector buffer size|vector|Number of bytes in use for storing all vector elements|O(1)|1;2|
size_t sv_get_buffer_size(const sv_t *v);
*/

/*
 * Accessors
 */

/* #API: |Get vector type element size|vector type|Element size (bytes)|O(1)|1;2| */
uint8_t sv_elem_size(const enum eSV_Type t);

/*
 * Allocation from other sources: "dup"
 */

/* #API: |Duplicate vector|vector|output vector|O(n)|1;!| */
sv_t *sv_dup(const sv_t *src);

/* #API: |Duplicate vector portion|vector; offset start; number of elements to take|output vector|O(n)|1;2| */
sv_t *sv_dup_erase(const sv_t *src, const size_t off, const size_t n);

/* #API: |Duplicate vector with resize operation|vector; size for the output|output vector|O(n)|1;2| */
sv_t *sv_dup_resize(const sv_t *src, const size_t n);

/* #API: |Reset/clean vector (keeping vector type)|vector|-|O(1)|1;2| */
void sv_clear(sv_t *v);


/*
 * Assignment
 */

/* #API: |Overwrite vector with a vector copy|output vector; input vector|output vector reference (optional usage)|O(n)|1;2| */
sv_t *sv_cpy(sv_t **v, const sv_t *src);

/* #API: |Overwrite vector with input vector copy applying a erase operation|output vector; input vector; input vector erase start offset; number of elements to erase|output vector reference (optional usage)|O(n)|1;2| */
sv_t *sv_cpy_erase(sv_t **v, const sv_t *src, const size_t off, const size_t n);

/* #API: |Overwrite vector with input vector copy plus resize operation|output vector; input vector; number of elements of input vector|output vector reference (optional usage)|O(n)|1;2| */
sv_t *sv_cpy_resize(sv_t **v, const sv_t *src, const size_t n);

/*
 * Append
 */

/*
#API: |Concatenate vectors to vector|vector; first vector to be concatenated; optional: N additional vectors to be concatenated|output vector reference (optional usage)|O(n)|1;2|
sv_t *sv_cat(sv_t **v, const sv_t *v1, ...)
*/
sv_t *sv_cat_aux(sv_t **v, const sv_t *v1, ...);

/* #API: |Concatenate vector with erase operation|output vector; input vector; input vector offset for erase start; erase element count|output vector reference (optional usage)|O(n)|1;2| */
sv_t *sv_cat_erase(sv_t **v, const sv_t *src, const size_t off, const size_t n);

/* #API: |Concatenate vector with input vector copy plus resize operation|output vector; input vector; number of elements of input vector|output vector reference (optional usage)|O(n)|1;2| */
sv_t *sv_cat_resize(sv_t **v, const sv_t *src, const size_t n);

/*
 * Transformation
 */

/* #API: |Erase portion of a vector|input/output vector; element offset where to start the cut; number of elements to be cut|output vector reference (optional usage)|O(n)|1;2| */
sv_t *sv_erase(sv_t **v, const size_t off, const size_t n);

/* #API: |Resize vector|input/output vector; new size|output vector reference (optional usage)|O(n)|1;2| */
sv_t *sv_resize(sv_t **v, const size_t n);

/* #API: |Sort vector|input/output vector|output vector reference (optional usage)|relies on libc "qsort" implementation, e.g. glibc implements introsort (O(n log n)), musl does smoothsort (O(n log n)), etc.|1;2| */
sv_t *sv_sort(sv_t *v);

/*
 * Search
 */

/* #API: |Find value in vector (generic data)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find(const sv_t *v, const size_t off, const void *target);

/* #API: |Find value in vector (integer)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find_i(const sv_t *v, const size_t off, const int64_t target);

/* #API: |Find value in vector (unsigned integer)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find_u(const sv_t *v, const size_t off, const uint64_t target);

/*
 * Compare
 */

/* #API: |Compare two vectors [TODO: this may be removed, as it is unnecesary complex]|vector #1; vector #1 offset start; vector #2; vector #2 start; compare size|0: equals; < 0 if a < b; > 0 if a > b|O(n)|1;2| */
int sv_ncmp(const sv_t *v1, const size_t v1off, const sv_t *v2, const size_t v2off, const size_t n);

/* #API: |Compare two elements from same vector|vector; element 'a' offset; element :b' offset|0: equals; < 0 if a < b; > 0 if a > b|O(n)|1;2| */
int sv_cmp(const sv_t *v, const size_t a_off, const size_t b_off);

/*
 * Vector "at": element access to given position
 */

/* #API: |Vector random access (generic data)|vector; location|NULL: not found; != NULL: element reference|O(1)|1;2| */
const void *sv_at(const sv_t *v, const size_t index);

/* #API: |Vector random access (integer)|vector; location|Element value|O(1)|1;2| */
int64_t sv_at_i(const sv_t *v, const size_t index);

/* #API: |Vector random access (unsigned integer)|vector; location|Element value|O(1)|1;2| */
uint64_t sv_at_u(const sv_t *v, const size_t index);

/*
 * Vector "set": set element value at given position
 */

/* #API: |Vector random access write (generic data)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
sbool_t sv_set(sv_t **v, const size_t index, const void *value);

/* #API: |Vector random access write (integer)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
sbool_t sv_set_i(sv_t **v, const size_t index, int64_t value);

/* #API: |Vector random access write (unsigned integer)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
sbool_t sv_set_u(sv_t **v, const size_t index, uint64_t value);

/*
 * Vector "push": add element in the last position
 */

/* #API: |Push/add element (generic data)|vector; data source; number of elements|S_TRUE: added OK; S_FALSE: not enough memory|O(n)|1;2| */
sbool_t sv_push_raw(sv_t **v, const void *src, const size_t n);

/*
#API: |Push/add multiple elements (generic data)|vector; new element to be added; more elements to be added (optional)|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2|
sbool_t sv_push(sv_t **v, const void *c1, ...)
*/
size_t sv_push_aux(sv_t **v, const void *c1, ...);

/* #API: |Push/add element (integer)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2| */
sbool_t sv_push_i(sv_t **v, const int64_t c);

/* #API: |Push/add element (unsigned integer)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2| */
sbool_t sv_push_u(sv_t **v, const uint64_t c);

/*
 * Vector "pop": extract element from last position
 */

/* #API: |Pop/extract element (generic data)|vector|Element reference|O(1)|1;2| */
void *sv_pop(sv_t *v);

/* #API: |Pop/extract element (integer)|vector|Integer element|O(1)|1;2| */
int64_t sv_pop_i(sv_t *v);

/* #API: |Pop/extract element (unsigned integer)|vector|Integer element|O(1)|1;2| */
uint64_t sv_pop_u(sv_t *v);

/*
 * Functions intended for helping compiler optimization
 */

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif	/* SVECTOR_H */


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
 * #DOC
 * #DOC Supported vector types (enum eSV_Type):
 * #DOC
 * #DOC
 * #DOC SV_I8: int8_t
 * #DOC
 * #DOC SV_U8: uint8_t
 * #DOC
 * #DOC SV_I16: int16_t
 * #DOC
 * #DOC SV_U16: uint16_t
 * #DOC
 * #DOC SV_I32: int32_t
 * #DOC
 * #DOC SV_U32: uint32_t
 * #DOC
 * #DOC SV_I64: int64_t
 * #DOC
 * #DOC SV_U64: uint64_t
 * #DOC
 * #DOC SV_F: float
 * #DOC
 * #DOC SV_D: double
 * #DOC
 * #DOC SV_GEN: generic-data (user defined)
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "saux/sdata.h"

/*
 * Structures
 */

enum eSV_Type {
	SV_I8 = 0,
	SV_U8,
	SV_I16,
	SV_U16,
	SV_I32,
	SV_U32,
	SV_I64,
	SV_U64,
	SV_F,
	SV_D,
	SV_LAST_NUM = SV_D,
	SV_GEN,
	SV_NumTypes
};

typedef int (*srt_vector_cmp)(const void *a, const void *b);

struct SVector {
	struct SDataFull d;
	union {
		srt_vector_cmp cmpf; /* compare function */
		uintptr_t cnt;       /* counter (for derivative types, e.g.
					srt_bitset) */
	} vx;
};

typedef struct SVector
	srt_vector; /* Opaque structure (accessors are provided) */

#define EMPTY_SV { EMPTY_SData(sizeof(srt_vector)), NULL }

/*
 * Variable argument functions
 */

#ifdef S_USE_VA_ARGS
#define sv_free(...) sv_free_aux(__VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#define sv_cat(v, ...) sv_cat_aux(v, __VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#define sv_push(v, ...) sv_push_aux(v, __VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#else
#define sv_free(v) sv_free_aux(v, S_INVALID_PTR_VARG_TAIL)
#define sv_cat(v, a) sv_cat_aux(v, a, S_INVALID_PTR_VARG_TAIL)
#define sv_push(v, a) sv_push_aux(v, a, S_INVALID_PTR_VARG_TAIL)
#endif

/*
 * Allocation
 */

#define sv_alloca(elem_size, max_size, cmp_f)                                  \
	sv_alloc_raw(SV_GEN, S_TRUE,                                           \
		     s_alloca(sd_alloc_size_raw(sizeof(srt_vector), elem_size, \
						max_size, S_FALSE)),           \
		     elem_size, max_size, cmp_f)
#define sv_alloca_t(type, max_size)                                            \
	sv_alloc_raw(type, S_TRUE,                                             \
		     s_alloca(sd_alloc_size_raw(sizeof(srt_vector),            \
						sv_elem_size(type), max_size,  \
						S_FALSE)),                     \
		     sv_elem_size(type), max_size, NULL)

srt_vector *sv_alloc_raw(enum eSV_Type t, srt_bool ext_buf,
			 void *buffer, size_t elem_size,
			 size_t max_size, const srt_vector_cmp f);

/* #API: |Allocate SV_GEN vector (heap)|element size; space preallocated to store n elements; compare function (used for sorting, pass NULL for none)|vector|O(1)|1;2| */
srt_vector *sv_alloc(size_t elem_size, size_t initial_num_elems_reserve, const srt_vector_cmp f);

/* #API: |Allocate typed vector (heap)|Vector type; space preallocated to store n elements|vector|O(1)|1;2| */
srt_vector *sv_alloc_t(enum eSV_Type t, size_t initial_num_elems_reserve);

SD_BUILDFUNCS_FULL(sv, srt_vector, 0)

/*
#API: |Allocate typed vector (stack)|Vector type; space preallocated to store n elements|vector|O(1)|1;2|
srt_vector *sv_alloca_t(enum eSV_Type t, size_t initial_num_elems_reserve)

#API: |Allocate SV_GEN vector (stack)|space preallocated to store n elements|allocated vector|O(1)|1;2|
srt_vector *sv_alloca(size_t initial_reserve)

#API: |Free one or more vectors (heap)|vector;more vectors (optional)|-|O(1)|1;2|
void sv_free(srt_vector **v, ...)

#API: |Ensure space for extra elements|vector;number of extra eelements|extra size allocated|O(1)|1;2|
size_t sv_grow(srt_vector **v, size_t extra_elems)

#API: |Ensure space for elements|vector;absolute element reserve|reserved elements|O(1)|1;2|
size_t sv_reserve(srt_vector **v, size_t max_elems)

#API: |Free unused space|vector|same vector (optional usage)|O(1)|1;2|
srt_vector *sv_shrink(srt_vector **v)

#API: |Get vector size|vector|vector number of elements|O(1)|1;2|
size_t sv_size(const srt_vector *v)

#NOTAPI: |Set vector size (number of vector elements)|vector;set vector number of elements|-|O(1)|1;2|
void sv_set_size(srt_vector *v, size_t s)

#API: |Equivalent to sv_size|vector|Number of vector elements|O(1)|1;2|
size_t sv_len(const srt_vector *v)

#API: |Allocated space|vector|current allocated space (vector elements)|O(1)|1;2|
size_t sv_capacity(const srt_vector *v);

#API: |Preallocated space left|vector|allocated space left|O(1)|1;2|
size_t sv_capacity_left(const srt_vector *v);

#API: |Tells if a vector is empty (zero elements)|vector|S_TRUE: empty vector; S_FALSE: not empty|O(1)|1;2|
srt_bool sv_empty(const srt_vector *v)

#API: |Get vector buffer access|vector|pointer to the internal vector buffer (raw data)|O(1)|1;2|
char *sv_get_buffer(srt_vector *v);

#API: |Get vector buffer access (read-only)|vector|pointer to the internal vector buffer (raw data)|O(1)|1;2|
const char *sv_get_buffer_r(const srt_vector *v);

#API: |Get vector buffer size|vector|Number of bytes in use for storing all vector elements|O(1)|1;2|
size_t sv_get_buffer_size(const srt_vector *v);
*/

/*
 * Accessors
 */

/* #API: |Get vector type element size|vector type|Element size (bytes)|O(1)|1;2| */
uint8_t sv_elem_size(enum eSV_Type t);

/*
 * Allocation from other sources: "dup"
 */

/* #API: |Duplicate vector|vector|output vector|O(n)|1;2| */
srt_vector *sv_dup(const srt_vector *src);

/* #API: |Duplicate vector portion|vector; offset start; number of elements to take|output vector|O(n)|1;2| */
srt_vector *sv_dup_erase(const srt_vector *src, size_t off, size_t n);

/* #API: |Duplicate vector with resize operation|vector; size for the output|output vector|O(n)|1;2| */
srt_vector *sv_dup_resize(const srt_vector *src, size_t n);

/* #API: |Reset/clean vector (keeping vector type)|vector|-|O(1)|1;2| */
void sv_clear(srt_vector *v);

/*
 * Assignment
 */

/* #API: |Overwrite vector with a vector copy|output vector; input vector|output vector reference (optional usage)|O(n)|1;2| */
srt_vector *sv_cpy(srt_vector **v, const srt_vector *src);

/* #API: |Overwrite vector with input vector copy applying a erase operation|output vector; input vector; input vector erase start offset; number of elements to erase|output vector reference (optional usage)|O(n)|1;2| */
srt_vector *sv_cpy_erase(srt_vector **v, const srt_vector *src, size_t off, size_t n);

/* #API: |Overwrite vector with input vector copy plus resize operation|output vector; input vector; number of elements of input vector|output vector reference (optional usage)|O(n)|1;2| */
srt_vector *sv_cpy_resize(srt_vector **v, const srt_vector *src, size_t n);

/*
 * Append
 */

/*
#API: |Concatenate vectors to vector|vector; first vector to be concatenated; optional: N additional vectors to be concatenated|output vector reference (optional usage)|O(n)|1;2|
srt_vector *sv_cat(srt_vector **v, const srt_vector *v1, ...)
*/
srt_vector *sv_cat_aux(srt_vector **v, const srt_vector *v1, ...);

/* #API: |Concatenate vector with erase operation|output vector; input vector; input vector offset for erase start; erase element count|output vector reference (optional usage)|O(n)|1;2| */
srt_vector *sv_cat_erase(srt_vector **v, const srt_vector *src, size_t off, size_t n);

/* #API: |Concatenate vector with input vector copy plus resize operation|output vector; input vector; number of elements of input vector|output vector reference (optional usage)|O(n)|1;2| */
srt_vector *sv_cat_resize(srt_vector **v, const srt_vector *src, size_t n);

/*
 * Transformation
 */

/* #API: |Erase portion of a vector|input/output vector; element offset where to start the cut; number of elements to be cut|output vector reference (optional usage)|O(n)|1;2| */
srt_vector *sv_erase(srt_vector **v, size_t off, size_t n);

/* #API: |Resize vector|input/output vector; new size|output vector reference (optional usage)|O(n)|1;2| */
srt_vector *sv_resize(srt_vector **v, size_t n);

/* #API: |Sort vector|input/output vector|output vector reference (optional usage)|relies on libc "qsort" implementation, e.g. glibc implements introsort (O(n log n)), musl does smoothsort (O(n log n)), etc.|1;2| */
srt_vector *sv_sort(srt_vector *v);

/*
 * Search
 */

/* #API: |Find value in vector (SV_GEN)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find(const srt_vector *v, size_t off, const void *target);

/* #API: |Find value in vector (SV_I8)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find_i8(const srt_vector *v, size_t off, int8_t target);

/* #API: |Find value in vector (SV_U8)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find_u8(const srt_vector *v, size_t off, uint8_t target);

/* #API: |Find value in vector (SV_I16)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find_i16(const srt_vector *v, size_t off, int16_t target);

/* #API: |Find value in vector (SV_U16)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find_u16(const srt_vector *v, size_t off, uint16_t target);

/* #API: |Find value in vector (SV_I32)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find_i32(const srt_vector *v, size_t off, int32_t target);

/* #API: |Find value in vector (SV_U32)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find_u32(const srt_vector *v, size_t off, uint32_t target);

/* #API: |Find value in vector (SV_I64)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find_i64(const srt_vector *v, size_t off, int64_t target);

/* #API: |Find value in vector (SV_U64)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find_u64(const srt_vector *v, size_t off, uint64_t target);

/* #API: |Find value in vector (SV_F)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find_f(const srt_vector *v, size_t off, float target);

/* #API: |Find value in vector (SV_D)|vector; search offset start; target to be located|offset: >=0 found; S_NPOS: not found|O(n)|1;2| */
size_t sv_find_d(const srt_vector *v, size_t off, double target);

/*
 * Compare
 */

/* #API: |Compare two vectors [TODO: this may be removed, as it is unnecesary complex]|vector #1; vector #1 offset start; vector #2; vector #2 start; compare size|0: equals; < 0 if a < b; > 0 if a > b|O(n)|1;2| */
int sv_ncmp(const srt_vector *v1, size_t v1off, const srt_vector *v2, size_t v2off, size_t n);

/* #API: |Compare two elements from same vector|vector; element 'a' offset; element :b' offset|0: equals; < 0 if a < b; > 0 if a > b|O(n)|1;2| */
int sv_cmp(const srt_vector *v, size_t a_off, size_t b_off);

/*
 * Vector "at": element access to given position
 */

/* #API: |Vector random access (SV_GEN)|vector; location|NULL: not found; != NULL: element reference|O(1)|1;2| */
const void *sv_at(const srt_vector *v, size_t index);

/* #API: |Vector random access (SV_I8)|vector; location|Element value|O(1)|1;2| */
int8_t sv_at_i8(const srt_vector *v, size_t index);

/* #API: |Vector random access (SV_U8)|vector; location|Element value|O(1)|1;2| */
uint8_t sv_at_u8(const srt_vector *v, size_t index);

/* #API: |Vector random access (SV_I16)|vector; location|Element value|O(1)|1;2| */
int16_t sv_at_i16(const srt_vector *v, size_t index);

/* #API: |Vector random access (SV_U16)|vector; location|Element value|O(1)|1;2| */
uint16_t sv_at_u16(const srt_vector *v, size_t index);

/* #API: |Vector random access (SV_I32)|vector; location|Element value|O(1)|1;2| */
int32_t sv_at_i32(const srt_vector *v, size_t index);

/* #API: |Vector random access (SV_U32)|vector; location|Element value|O(1)|1;2| */
uint32_t sv_at_u32(const srt_vector *v, size_t index);

/* #API: |Vector random access (SV_I64)|vector; location|Element value|O(1)|1;2| */
int64_t sv_at_i64(const srt_vector *v, size_t index);

/* #API: |Vector random access (SV_U64)|vector; location|Element value|O(1)|1;2| */
uint64_t sv_at_u64(const srt_vector *v, size_t index);

/* #API: |Vector random access (SV_F)|vector; location|Element value|O(1)|1;2| */
float sv_at_f(const srt_vector *v, size_t index);

/* #API: |Vector random access (SV_D)|vector; location|Element value|O(1)|1;2| */
double sv_at_d(const srt_vector *v, size_t index);

/*
 * Vector "set": set element value at given position
 */

/* #API: |Vector random access write (SV_GEN)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_set(srt_vector **v, size_t index, const void *value);

/* #API: |Vector random access write (SV_I8)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_set_i8(srt_vector **v, size_t index, int8_t value);

/* #API: |Vector random access write (SV_U8)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_set_u8(srt_vector **v, size_t index, uint8_t value);

/* #API: |Vector random access write (SV_I16)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_set_i16(srt_vector **v, size_t index, int16_t value);

/* #API: |Vector random access write (SV_U16)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_set_u16(srt_vector **v, size_t index, uint16_t value);

/* #API: |Vector random access write (SV_I32)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_set_i32(srt_vector **v, size_t index, int32_t value);

/* #API: |Vector random access write (SV_U32)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_set_u32(srt_vector **v, size_t index, uint32_t value);

/* #API: |Vector random access write (SV_I64)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_set_i64(srt_vector **v, size_t index, int64_t value);

/* #API: |Vector random access write (SV_U64)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_set_u64(srt_vector **v, size_t index, uint64_t value);

/* #API: |Vector random access write (SV_F)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_set_f(srt_vector **v, size_t index, float value);

/* #API: |Vector random access write (SV_D)|vector; location; value|S_TRUE: OK, S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_set_d(srt_vector **v, size_t index, double value);

/*
 * Vector "push": add element in the last position
 */

/* #API: |Push/add element|vector; data source; number of elements|S_TRUE: added OK; S_FALSE: not enough memory|O(n)|1;2| */
srt_bool sv_push_raw(srt_vector **v, const void *src, size_t n);

/*
#API: |Push/add multiple elements (SV_GEN)|vector; new element to be added; more elements to be added (optional)|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2|
srt_bool sv_push(srt_vector **v, const void *c1, ...)
*/
size_t sv_push_aux(srt_vector **v, const void *c1, ...);

/* #API: |Push/add element (SV_I8)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_push_i8(srt_vector **v, int8_t c);

/* #API: |Push/add element (SV_U8)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_push_u8(srt_vector **v, uint8_t c);

/* #API: |Push/add element (SV_I16)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_push_i16(srt_vector **v, int16_t c);

/* #API: |Push/add element (SV_U16)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_push_u16(srt_vector **v, uint16_t c);

/* #API: |Push/add element (SV_I32)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_push_i32(srt_vector **v, int32_t c);

/* #API: |Push/add element (SV_U32)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_push_u32(srt_vector **v, uint32_t c);

/* #API: |Push/add element (SV_I64)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_push_i64(srt_vector **v, int64_t c);

/* #API: |Push/add element (SV_U64)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_push_u64(srt_vector **v, uint64_t c);

/* #API: |Push/add element (SV_F)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_push_f(srt_vector **v, float c);

/* #API: |Push/add element (SV_D)|vector; data source|S_TRUE: added OK; S_FALSE: not enough memory|O(1)|1;2| */
srt_bool sv_push_d(srt_vector **v, double c);

/*
 * Vector "pop": extract element from last position
 */

/* #API: |Pop/extract element (SV_GEN)|vector|Element reference|O(1)|1;2| */
void *sv_pop(srt_vector *v);

/* #API: |Pop/extract element (SV_I8)|vector|Integer element|O(1)|1;2| */
int8_t sv_pop_i8(srt_vector *v);

/* #API: |Pop/extract element (SV_U8)|vector|Integer element|O(1)|1;2| */
uint8_t sv_pop_u8(srt_vector *v);

/* #API: |Pop/extract element (SV_I16)|vector|Integer element|O(1)|1;2| */
int16_t sv_pop_i16(srt_vector *v);

/* #API: |Pop/extract element (SV_U16)|vector|Integer element|O(1)|1;2| */
uint16_t sv_pop_u16(srt_vector *v);

/* #API: |Pop/extract element (SV_I32)|vector|Integer element|O(1)|1;2| */
int32_t sv_pop_i32(srt_vector *v);

/* #API: |Pop/extract element (SV_U32)|vector|Integer element|O(1)|1;2| */
uint32_t sv_pop_u32(srt_vector *v);

/* #API: |Pop/extract element (SV_I64)|vector|Integer element|O(1)|1;2| */
int64_t sv_pop_i64(srt_vector *v);

/* #API: |Pop/extract element (SV_U64)|vector|Integer element|O(1)|1;2| */
uint64_t sv_pop_u64(srt_vector *v);

/* #API: |Pop/extract element (SV_F)|vector|Integer element|O(1)|1;2| */
float sv_pop_f(srt_vector *v);

/* #API: |Pop/extract element (SV_D)|vector|Integer element|O(1)|1;2| */
double sv_pop_d(srt_vector *v);

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* SVECTOR_H */


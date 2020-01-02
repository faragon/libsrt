#ifndef SHSET_H
#define SHSET_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * shset.h
 *
 * #SHORTDOC hash set handling (key-only storage)
 *
 * #DOC Set functions handle key-only storage, which is implemented as a
 * #DOC hash table (O(n), with O(1) amortized time complexity for insert/
 * #DOC count/delete).
 * #DOC
 * #DOC
 * #DOC Supported set modes (enum eSHS_Type):
 * #DOC
 * #DOC
 * #DOC 	SHS_I32: int32_t key
 * #DOC
 * #DOC 	SHS_U32: uint32_t key
 * #DOC
 * #DOC 	SHS_I: int64_t key
 * #DOC
 * #DOC 	SHS_F: float (single-precision floating point) key
 * #DOC
 * #DOC 	SHS_D: double (double-precision floating point) key
 * #DOC
 * #DOC 	SHS_S: string key
 * #DOC
 * #DOC
 * #DOC Callback types for the shs_itp_*() functions:
 * #DOC
 * #DOC
 * #DOC	typedef srt_bool (*srt_hset_it_i32)(int32_t k, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hset_it_u32)(uint32_t k, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hset_it_i)(int64_t k, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hset_it_f)(float k, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hset_it_d)(double k, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hset_it_s)(const srt_string *, void *context);
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "shmap.h"

/*
 * Structures and types
 */

enum eSHS_Type {
	SHS_I32 = SHM0_I32,
	SHS_U32 = SHM0_U32,
	SHS_I = SHM0_I,
	SHS_F = SHM0_F,
	SHS_D = SHM0_D,
	SHS_S = SHM0_S
};

typedef srt_hmap srt_hset;

/*
 * Allocation
 */

#define shs_alloca shm_alloca
/*
#API: |Allocate hash set (stack)|set type; initial reserve|map|O(n)|1;2|
srt_hset *shs_alloca(enum eSHS_Type t, size_t n);
*/

/* #API: |Allocate hash set (heap)|set type; initial reserve|hash set|O(n)|1;2| */
S_INLINE srt_hset *shs_alloc(enum eSHS_Type t, size_t init_size)
{
	return shm_alloc_aux((int)t, init_size);
}

/* #API: |Ensure space for extra elements|hash set;number of extra elements|extra size allocated|O(1)|1;2| */
S_INLINE size_t shs_grow(srt_hset **hs, size_t extra_elems)
{
	return shm_grow(hs, extra_elems);
}

/* #API: |Ensure space for elements|hash set;absolute element reserve|reserved elements|O(1)|1;2| */
S_INLINE size_t shs_reserve(srt_hset **hs, size_t max_elems)
{
	return shm_reserve(hs, max_elems);
}

/* #API: |Make the hmap use the minimum possible memory|hash set|hash set reference (optional usage)|O(1) for allocators using memory remap; O(n) for naive allocators|1;2| */
S_INLINE srt_hset *shs_shrink(srt_hset **hs)
{
	return shm_shrink(hs);
}

/* #API: |Get hmap size|map|hash set number of elements|O(1)|1;2| */
S_INLINE size_t shs_size(const srt_hset *hs)
{
	return shm_size(hs);
}

/* #API: |Get hmap size|map|hash set current max number of elements|O(1)|1;2| */
S_INLINE size_t shs_max_size(const srt_hset *hs)
{
	return shm_max_size(hs);
}

/* #API: |Allocated space|hash set|current allocated space (vector elements)|O(1)|1;2| */
S_INLINE size_t shs_capacity(const srt_hset *hs)
{
	return shm_capacity(hs);
}

/* #API: |Preallocated space left|hash set|allocated space left|O(1)|1;2| */
S_INLINE size_t shs_capacity_left(const srt_hset *hs)
{
	return shm_capacity_left(hs);
}

/* #API: |Tells if a hash set is empty (zero elements)|hash set|S_TRUE: empty; S_FALSE: not empty|O(1)|1;2| */
S_INLINE srt_bool shs_empty(const srt_hset *hs)
{
	return shm_empty(hs);
}

/* #API: |Duplicate hash set|input hash setoutput hash set|O(n)|1;2| */
S_INLINE srt_hset *shs_dup(const srt_hset *src)
{
	return shm_dup(src);
}

/* #API: |Clear/reset map (keeping map type)|hash set||O(1) for simple maps, O(n) for maps having nodes with strings|1;2| */
S_INLINE void shs_clear(srt_hset *hs)
{
	shm_clear(hs);
}

/*
#API: |Free one or more hash sets|hash set; more hash sets (optional)|-|O(1) for simple dmaps, O(n) for dmaps having nodes with strings|1;2|
void shs_free(srt_hset **hs, ...)
*/
#ifdef S_USE_VA_ARGS
#define shs_free(...) shm_free_aux(__VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#else
#define shs_free(hs) shm_free_aux(hs, S_INVALID_PTR_VARG_TAIL)
#endif

/*
 * Copy
 */

/* #API: |Overwrite map with a map copy|output map; input hash setoutput map reference (optional usage)|O(n)|1;2| */
S_INLINE srt_hset *shs_cpy(srt_hset **hs, const srt_hset *src)
{
	return shm_cpy(hs, src);
}

/*
 * Existence check
 */

/* #API: |Map element count/check (SHS_U32)|hash set; key|S_TRUE: element found; S_FALSE: not in the hash set | O(n), O(1) average amortized|1;2| */
S_INLINE size_t shs_count_u32(const srt_hset *hs, uint32_t k)
{
	return shm_count_u32(hs, k);
}

/* #API: |Map element count/check (SHS_I32)|hash set; key|S_TRUE: element found; S_FALSE: not in the hash set | O(n), O(1) average amortized|1;2| */
S_INLINE size_t shs_count_i32(const srt_hset *hs, int32_t k)
{
	return shm_count_i32(hs, k);
}

/* #API: |Map element count/check (SHS_I)|hash set; key|S_TRUE: element found; S_FALSE: not in the hash set | O(n), O(1) average amortized|1;2| */
S_INLINE size_t shs_count_i(const srt_hset *hs, int64_t k)
{
	return shm_count_i(hs, k);
}

/* #API: |Map element count/check (SHS_F)|hash set; key|S_TRUE: element found; S_FALSE: not in the hash set | O(n), O(1) average amortized|1;2| */
S_INLINE size_t shs_count_f(const srt_hset *hs, float k)
{
	return shm_count_f(hs, k);
}

/* #API: |Map element count/check (SHS_D)|hash set; key|S_TRUE: element found; S_FALSE: not in the hash set | O(n), O(1) average amortized|1;2| */
S_INLINE size_t shs_count_d(const srt_hset *hs, double k)
{
	return shm_count_d(hs, k);
}

/* #API: |Map element count/check (SHS_S)|hash set; key|S_TRUE: element found; S_FALSE: not in the hash set | O(n), O(1) average amortized|1;2| */
S_INLINE size_t shs_count_s(const srt_hset *hs, const srt_string *k)
{
	return shm_count_s(hs, k);
}

/*
 * Insert
 */

/* #API: |Insert into hash set (SHS_I32)|hash set; key|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
S_INLINE srt_bool shs_insert_i32(srt_hset **hs, int32_t k)
{
	return shm_insert_i32(hs, k);
}

/* #API: |Insert into hash set (SHS_U32)|hash set; key|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
S_INLINE srt_bool shs_insert_u32(srt_hset **hs, uint32_t k)
{
	return shm_insert_u32(hs, k);
}

/* #API: |Insert into hash set (SHS_I)|hash set; key|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
S_INLINE srt_bool shs_insert_i(srt_hset **hs, int64_t k)
{
	return shm_insert_i(hs, k);
}

/* #API: |Insert into hash set (SHS_F)|hash set; key|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
S_INLINE srt_bool shs_insert_f(srt_hset **hs, float k)
{
	return shm_insert_f(hs, k);
}

/* #API: |Insert into hash set (SHS_D)|hash set; key|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
S_INLINE srt_bool shs_insert_d(srt_hset **hs, double k)
{
	return shm_insert_d(hs, k);
}

/* #API: |Insert into hash set (SHS_S)|hash set; key|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
S_INLINE srt_bool shs_insert_s(srt_hset **hs, const srt_string *k)
{
	return shm_insert_s(hs, k);
}

/*
 * Delete
 */

/* #API: |Delete map element (SHS_U32)|hash set; int32_t key|S_TRUE: found and deleted; S_FALSE: not found|O(n), O(1) average amortized|1;2| */
S_INLINE srt_bool shs_delete_u32(srt_hset *hs, uint32_t k)
{
	return shm_delete_u32(hs, k);
}

/* #API: |Delete map element (SHS_I32)|hash set; int32_t key|S_TRUE: found and deleted; S_FALSE: not found|O(n), O(1) average amortized|1;2| */
S_INLINE srt_bool shs_delete_i32(srt_hset *hs, int32_t k)
{
	return shm_delete_i32(hs, k);
}

/* #API: |Delete map element (SHS_I)|hash set; int64_t key|S_TRUE: found and deleted; S_FALSE: not found|O(n), O(1) average amortized|1;2| */
S_INLINE srt_bool shs_delete_i(srt_hset *hs, int64_t k)
{
	return shm_delete_i(hs, k);
}

/* #API: |Delete map element (SHS_F)|hash set; float key|S_TRUE: found and deleted; S_FALSE: not found|O(n), O(1) average amortized|1;2| */
S_INLINE srt_bool shs_delete_f(srt_hset *hs, float k)
{
	return shm_delete_f(hs, k);
}

/* #API: |Delete map element (SHS_D)|hash set; double key|S_TRUE: found and deleted; S_FALSE: not found|O(n), O(1) average amortized|1;2| */
S_INLINE srt_bool shs_delete_d(srt_hset *hs, double k)
{
	return shm_delete_d(hs, k);
}

/* #API: |Delete map element (SHS_S)|hash set; string key|S_TRUE: found and deleted; S_FALSE: not found|O(n), O(1) average amortized|1;2| */
S_INLINE srt_bool shs_delete_s(srt_hset *hs, const srt_string *k)
{
	return shm_delete_s(hs, k);
}

/*
 * Enumeration
 */

/* #API: |Enumerate set keys (SHS_I32)|hash set; element, 0 to n - 1|int32_t|O(1)|1;2| */
S_INLINE int32_t shs_it_i32(const srt_hset *hs, size_t i)
{
	return shm_it_i32_k(hs, i);
}

/* #API: |Enumerate set keys (SHS_U32)|hash set; element, 0 to n - 1|uint32_t|O(1)|1;2| */
S_INLINE uint32_t shs_it_u32(const srt_hset *hs, size_t i)
{
	return shm_it_u32_k(hs, i);
}

/* #API: |Enumerate set keys (SHS_I)|hash set; element, 0 to n - 1|int64_t|O(1)|1;2| */
S_INLINE int64_t shs_it_i(const srt_hset *hs, size_t i)
{
	return shm_it_i_k(hs, i);
}

/* #API: |Enumerate set keys (SHS_F)|hash set; element, 0 to n - 1|int64_t|O(1)|0;1| */
S_INLINE float shs_it_f(const srt_hset *hs, size_t i)
{
	return shm_it_f_k(hs, i);
}

/* #API: |Enumerate set keys (SHS_D)|hash set; element, 0 to n - 1|int64_t|O(1)|0;1| */
S_INLINE double shs_it_d(const srt_hset *hs, size_t i)
{
	return shm_it_d_k(hs, i);
}

/* #API: |Enumerate set keys (SHS_S)|hash set; element, 0 to n - 1|string|O(1)|1;2| */
S_INLINE const srt_string *shs_it_s(const srt_hset *hs, size_t i)
{
	return shm_it_s_k(hs, i);
}

/*
 * Enumeration, with callback helper
 */

typedef srt_bool (*srt_hset_it_i32)(int32_t k, void *context);
typedef srt_bool (*srt_hset_it_u32)(uint32_t k, void *context);
typedef srt_bool (*srt_hset_it_i)(int64_t k, void *context);
typedef srt_bool (*srt_hset_it_f)(float k, void *context);
typedef srt_bool (*srt_hset_it_d)(double k, void *context);
typedef srt_bool (*srt_hset_it_s)(const srt_string *, void *context);

/* #API: |Enumerate set elements in portions (SHS_I32)|set; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shs_itp_i32(const srt_hset *s, size_t begin, size_t end, srt_hset_it_i32 f, void *context);

/* #API: |Enumerate set elements in portions (SHS_U32)|set; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shs_itp_u32(const srt_hset *s, size_t begin, size_t end, srt_hset_it_u32 f, void *context);

/* #API: |Enumerate set elements in portions (SHS_I)|set; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shs_itp_i(const srt_hset *s, size_t begin, size_t end, srt_hset_it_i f, void *context);

/* #API: |Enumerate set elements in portions (SHS_F)|set; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shs_itp_f(const srt_hset *s, size_t begin, size_t end, srt_hset_it_f f, void *context);

/* #API: |Enumerate set elements in portions (SHS_D)|set; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shs_itp_d(const srt_hset *s, size_t begin, size_t end, srt_hset_it_d f, void *context);

/* #API: |Enumerate set elements in portions (SHS_S)|set; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shs_itp_s(const srt_hset *s, size_t begin, size_t end, srt_hset_it_s f, void *context);

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* #ifndef SHSET_H */


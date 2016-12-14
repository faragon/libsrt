#ifndef SMSET_H
#define SMSET_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * smset.h
 *
 * #SHORTDOC set handling (key-only storage)
 *
 * #DOC Set functions handle key-only storage, which is implemented as a
 * #DOC Red-Black tree (O(n log n) maximum complexity for insert/read/delete).
 * #DOC
 * #DOC
 * #DOC Supported set modes (enum eSMS_Type):
 * #DOC
 * #DOC
 * #DOC 	SMS_I32: 32-bit integer key
 * #DOC
 * #DOC 	SMS_U32: 32-bit unsigned int key
 * #DOC
 * #DOC 	SMS_I: 64-bit int key
 * #DOC
 * #DOC 	SMS_S: string key
 * #DOC
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "sstring.h"
#include "smap.h"

/*
 * Structures
 */

enum eSMS_Type
{
	SMS_I	= SM0_I,
	SMS_I32	= SM0_I32,
	SMS_U32	= SM0_U32,
	SMS_S	= SM0_S
};

typedef sm_t sms_t;	/* "Hidden" structure (accessors are provided) */
			/* (set is implemented over key-only map)      */

typedef sbool_t (*sms_it_i32_t)(int32_t k, void *context);
typedef sbool_t (*sms_it_u32_t)(uint32_t k, void *context);
typedef sbool_t (*sms_it_i_t)(int64_t k, void *context);
typedef sbool_t (*sms_it_s_t)(const ss_t *, void *context);

/*
 * Allocation
 */

/*
#API: |Allocate set (stack)|set type; initial reserve|set|O(1)|1;2|
sms_t *sms_alloca(const enum eSMS_Type t, const size_t n);
*/
#define sms_alloca(type, max_size)					\
	sms_alloc_raw(type, S_TRUE,					\
		      alloca(sd_alloc_size_raw(sizeof(sms_t),		\
			     sm_elem_size(type), max_size, S_FALSE)),	\
		     sm_elem_size(type), max_size)

S_INLINE sms_t *sms_alloc_raw(const enum eSMS_Type t, const sbool_t ext_buf, void *buffer, const size_t elem_size, const size_t max_size)
{
	return sm_alloc_raw0((enum eSM_Type0)t, ext_buf, buffer, elem_size, max_size);
}

/* #API: |Allocate set (heap)|set type; initial reserve|set|O(1)|1;2| */
S_INLINE sms_t *sms_alloc(const enum eSMS_Type t, const size_t initial_num_elems_reserve)
{
        return sm_alloc0((enum eSM_Type0)t, initial_num_elems_reserve);
}

/* #API: |Duplicate set|input set|output set|O(n)|1;2| */
S_INLINE sms_t *sms_dup(const sms_t *src)
{
	return sm_dup(src);
}

/* #API: |Reset/clean set (keeping set type)|set|-|O(1) for simple sets, O(n) for sets having nodes with strings|1;2| */
S_INLINE void sms_clear(sms_t *s)
{
	sm_clear(s);
}

/*
#API: |Free one or more sets (heap)|set; more sets (optional)|-|O(1) for simple sets, O(n) for string sets|1;2|
void sms_free(sm_t **s, ...)
*/
#define sms_free(...) sm_free_aux(__VA_ARGS__, S_INVALID_PTR_VARG_TAIL)

SD_BUILDFUNCS_FULL_ST(sms, 0)

/*
#API: |Ensure space for extra elements|set;number of extra elements|extra size allocated|O(1)|1;2|
size_t sms_grow(sms_t **s, const size_t extra_elems)

#API: |Ensure space for elements|set;absolute element reserve|reserved elements|O(1)|1;2|
size_t sms_reserve(sms_t **s, const size_t max_elems)

#API: |Make the set use the minimum possible memory|set|set reference (optional usage)|O(1) for allocators using memory reset; O(n) for naive allocators|1;2|
sms_t *sms_shrink(sms_t **s);

#API: |Get set size|set|Set number of elements|O(1)|1;2|
size_t sms_size(const sms_t *s);

#API: |Allocated space|set|current allocated space (vector elements)|O(1)|1;2|
size_t sms_capacity(const sms_t *s);

#API: |Preallocated space left|set|allocated space left|O(1)|1;2|
size_t sms_capacity_left(const sms_t *s);

#API: |Tells if a set is empty (zero elements)|set|S_TRUE: empty vector; S_FALSE: not empty|O(1)|1;2|
sbool_t sms_empty(const sms_t *s)
*/

/*
 * Copy
 */

/* #API: |Overwrite set with a set copy|output set; input set|output set reference (optional usage)|O(n)|1;2| */
S_INLINE sms_t *sms_cpy(sms_t **s, const sms_t *src)
{
	RETURN_IF(!s, NULL);
	if (*s)
		sm_clear(*s);
	RETURN_IF(!src || src->d.sub_type < SMS_I || src->d.sub_type > SMS_S, *s);
	return sm_cpy(s, src);
}

/*
 * Existence check
 */

/* #API: |Set element count/check|set; 32-bit unsigned integer key|S_TRUE: element found; S_FALSE: not in the set|O(log n)|1;2| */
S_INLINE sbool_t sms_count_u(const sms_t *s, const uint32_t k)
{
	return sm_count_u(s, k);
}

/* #API: |Set element count/check|set; integer key|S_TRUE: element found; S_FALSE: not in the set|O(log n)|1;2| */
S_INLINE sbool_t sms_count_i(const sms_t *s, const int64_t k)
{
	return sm_count_i(s, k);
}

/* #API: |Set element count/check|set; string key|S_TRUE: element found; S_FALSE: not in the set|O(log n)|1;2| */
S_INLINE sbool_t sms_count_s(const sms_t *s, const ss_t *k)
{
	return sm_count_s(s, k);
}

/*
 * Insert
 */

/* #API: |Insert into int32-int32 set|set; key|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
S_INLINE sbool_t sms_insert_i32(sms_t **s, const int32_t k)
{
        RETURN_IF(!s || (*s)->d.sub_type != SMS_I32, S_FALSE);
        struct SMapi n;
        n.k = k;
        return st_insert((st_t **)s, (const stn_t *)&n);
}

/* #API: |Insert into uint32-uint32 set|set; key|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
S_INLINE sbool_t sms_insert_u32(sms_t **s, const uint32_t k)
{
        RETURN_IF(!s || (*s)->d.sub_type != SMS_U32, S_FALSE);
        struct SMapu n;
        n.k = k;
        return st_insert((st_t **)s, (const stn_t *)&n);
}

/* #API: |Insert into int-int set|set; key|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
S_INLINE sbool_t sms_insert_i(sms_t **s, const int64_t k)
{
        RETURN_IF(!s || (*s)->d.sub_type != SMS_I, S_FALSE);
        struct SMapI n;
        n.k = k;
        return st_insert((st_t **)s, (const stn_t *)&n);
}

/* #API: |Insert into string-string set|set; key|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
S_INLINE sbool_t sms_insert_s(sms_t **s, const ss_t *k)
{
        RETURN_IF(!s  || (*s)->d.sub_type != SMS_S, S_FALSE);
        struct SMapS n;
#if 1 /* workaround */
	SMStrSet(&n.k, k);
	sbool_t ins_ok = st_insert((st_t **)s, (const stn_t *)&n);
	if (!ins_ok)
		SMStrFree(&n.k);
	return ins_ok;
#else
	/* TODO: rw_add_SMS_S */
        n.k = k;
	return st_insert_rw((st_t **)m, (const stn_t *)&n, rw_add_SMS_S);
#endif
}

/*
 * Delete
 */

/* #API: |Delete set element|set; integer key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
S_INLINE sbool_t sms_delete_i(sms_t *s, const int64_t k)
{
	return sm_delete_i(s, k);
}

/* #API: |Delete set element|set; string key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
S_INLINE sbool_t sms_delete_s(sms_t *s, const ss_t *k)
{
	return sm_delete_s(s, k);
}

/*
 * Enumeration
 */

/* #API: |Enumerate set elements in a given key range|set; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sms_itr_i32(const sms_t *s, int32_t key_min, int32_t key_max, sms_it_i32_t f, void *context);

/* #API: |Enumerate set elements in a given key range|set; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sms_itr_u32(const sms_t *s, uint32_t key_min, uint32_t key_max, sms_it_u32_t f, void *context);

/* #API: |Enumerate set elements in a given key range|set; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sms_itr_i(const sms_t *s, int64_t key_min, int64_t key_max, sms_it_i_t f, void *context);

/* #API: |Enumerate set elements in a given key range|set; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sms_itr_s(const sms_t *s, const ss_t *key_min, const ss_t *key_max, sms_it_s_t f, void *context);

/*
 * Unordered enumeration is inlined in order to get almost as fast
 * as array access after compiler optimization.
 */

/* #API: |Enumerate int32 set|set; element, 0 to n - 1|int32_t|O(1)|1;2|
int32_t sms_it_i32(const sms_t *s, stndx_t i);
*/
#define sms_it_i32(s, i) sm_it_i32_k(s, i)

/* #API: |Enumerate uint32 set|set; element, 0 to n - 1|uint32_t|O(1)|1;2|
uint32_t sms_it_u32(const sms_t *s, stndx_t i);
*/
#define sms_it_u32(s, i) sm_it_u32_k(s, i)

/* #API: |Enumerate int64 set|set; element, 0 to n - 1|int64_t|O(1)|1;2|
int32_t sms_it_i32(const sms_t *s, stndx_t i);
*/
#define sms_it_i(s, i) sm_it_i_k(s, i)

/* #API: |Enumerate string set|set; element, 0 to n - 1|string|O(1)|1;2|
int32_t sms_it_s(const sms_t *s, stndx_t i);
*/
#define sms_it_s(s, i) sm_it_s_k(s, i)

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* #ifndef SMSET_H */


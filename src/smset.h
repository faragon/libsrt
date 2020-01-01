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
 * #DOC 	SMS_I32: int32_t key
 * #DOC
 * #DOC 	SMS_U32: uint32_t key
 * #DOC
 * #DOC 	SMS_I: int64_t key
 * #DOC
 * #DOC 	SMS_F: float (single-precision floating point) key
 * #DOC
 * #DOC 	SMS_D: double (double-precision floating point) key
 * #DOC
 * #DOC 	SMS_S: string key
 * #DOC
 * #DOC
 * #DOC Callback types for the sm_itr_*() functions:
 * #DOC
 * #DOC
 * #DOC	typedef srt_bool (*srt_set_it_i32)(int32_t k, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_set_it_u32)(uint32_t k, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_set_it_i)(int64_t k, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_set_it_f)(float k, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_set_it_d)(double k, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_set_it_s)(const srt_string *, void *context);
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "smap.h"
#include "sstring.h"

/*
 * Structures
 */

enum eSMS_Type {
	SMS_I = SM0_I,
	SMS_I32 = SM0_I32,
	SMS_U32 = SM0_U32,
	SMS_F = SM0_F,
	SMS_D = SM0_D,
	SMS_S = SM0_S
};

typedef srt_map srt_set; /* Opaque structure (accessors are provided) */
			 /* (set is implemented over key-only map)    */

typedef srt_bool (*srt_set_it_i32)(int32_t k, void *context);
typedef srt_bool (*srt_set_it_u32)(uint32_t k, void *context);
typedef srt_bool (*srt_set_it_i)(int64_t k, void *context);
typedef srt_bool (*srt_set_it_f)(float k, void *context);
typedef srt_bool (*srt_set_it_d)(double k, void *context);
typedef srt_bool (*srt_set_it_s)(const srt_string *, void *context);

/*
 * Allocation
 */

/*
#API: |Allocate set (stack)|set type; initial reserve|set|O(1)|1;2|
srt_set *sms_alloca(enum eSMS_Type t, size_t n);
*/
#define sms_alloca(type, max_size)                                             \
	sms_alloc_raw(type, S_TRUE,                                            \
		      s_alloca(sd_alloc_size_raw(sizeof(srt_set),              \
						 sm_elem_size(type), max_size, \
						 S_FALSE)),                    \
		      sm_elem_size(type), max_size)

S_INLINE srt_set *sms_alloc_raw(enum eSMS_Type t, srt_bool ext_buf,
				void *buffer, size_t elem_size, size_t max_size)
{
	return sm_alloc_raw0((enum eSM_Type0)t, ext_buf, buffer, elem_size,
			     max_size);
}

/* #API: |Allocate set (heap)|set type; initial reserve|set|O(1)|1;2| */
S_INLINE srt_set *sms_alloc(enum eSMS_Type t, size_t initial_num_elems_reserve)
{
	return sm_alloc0((enum eSM_Type0)t, initial_num_elems_reserve);
}

/* #API: |Duplicate set|input set|output set|O(n)|1;2| */
S_INLINE srt_set *sms_dup(const srt_set *src)
{
	return sm_dup(src);
}

/* #API: |Reset/clean set (keeping set type)|set|-|O(1) for simple sets, O(n) for sets having nodes with strings|1;2| */
S_INLINE void sms_clear(srt_set *s)
{
	sm_clear(s);
}

/*
#API: |Free one or more sets (heap)|set; more sets (optional)|-|O(1) for simple sets, O(n) for string sets|1;2|
void sms_free(srt_map **s, ...)
*/
#ifdef S_USE_VA_ARGS
#define sms_free(...) sm_free_aux(__VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#else
#define sms_free(m) sm_free_aux(m, S_INVALID_PTR_VARG_TAIL)
#endif

SD_BUILDFUNCS_FULL_ST(sms, srt_set, 0)

/*
#API: |Ensure space for extra elements|set;number of extra elements|extra size allocated|O(1)|1;2|
size_t sms_grow(srt_set **s, size_t extra_elems)

#API: |Ensure space for elements|set;absolute element reserve|reserved elements|O(1)|1;2|
size_t sms_reserve(srt_set **s, size_t max_elems)

#API: |Make the set use the minimum possible memory|set|set reference (optional usage)|O(1) for allocators using memory reset; O(n) for naive allocators|1;2|
srt_set *sms_shrink(srt_set **s);

#API: |Get set size|set|Set number of elements|O(1)|1;2|
size_t sms_size(const srt_set *s);

#API: |Allocated space|set|current allocated space (vector elements)|O(1)|1;2|
size_t sms_capacity(const srt_set *s);

#API: |Preallocated space left|set|allocated space left|O(1)|1;2|
size_t sms_capacity_left(const srt_set *s);

#API: |Tells if a set is empty (zero elements)|set|S_TRUE: empty vector; S_FALSE: not empty|O(1)|1;2|
srt_bool sms_empty(const srt_set *s)
*/

/*
 * Copy
 */

/* #API: |Overwrite set with a set copy|output set; input set|output set reference (optional usage)|O(n)|1;2| */
S_INLINE srt_set *sms_cpy(srt_set **s, const srt_set *src)
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

/* #API: |Set element count/check (SMS_U32)|set; key|S_TRUE: element found; S_FALSE: not in the set|O(log n)|1;2| */
S_INLINE size_t sms_count_u32(const srt_set *s, uint32_t k)
{
	return sm_count_u32(s, k);
}

/* #API: |Set element count/check (SMS_I32)|set; key|S_TRUE: element found; S_FALSE: not in the set|O(log n)|1;2| */
S_INLINE size_t sms_count_i32(const srt_set *s, int32_t k)
{
	return sm_count_i32(s, k);
}

/* #API: |Set element count/check (SMS_I)|set; key|S_TRUE: element found; S_FALSE: not in the set|O(log n)|1;2| */
S_INLINE size_t sms_count_i(const srt_set *s, int64_t k)
{
	return sm_count_i(s, k);
}

/* #API: |Set element count/check (SMS_F)|set; key|S_TRUE: element found; S_FALSE: not in the set|O(log n)|1;2| */
S_INLINE size_t sms_count_f(const srt_set *s, float k)
{
	return sm_count_f(s, k);
}

/* #API: |Set element count/check (SMS_D)|set; key|S_TRUE: element found; S_FALSE: not in the set|O(log n)|1;2| */
S_INLINE size_t sms_count_d(const srt_set *s, double k)
{
	return sm_count_d(s, k);
}

/* #API: |Set element count/check (SMS_S)|set; key|S_TRUE: element found; S_FALSE: not in the set|O(log n)|1;2| */
S_INLINE size_t sms_count_s(const srt_set *s, const srt_string *k)
{
	return sm_count_s(s, k);
}

/*
 * Insert
 */

/* #API: |Insert element (SMS_I32)|set; key|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
S_INLINE srt_bool sms_insert_i32(srt_set **s, int32_t k)
{
	struct SMapi n;
	RETURN_IF(!s || (*s)->d.sub_type != SMS_I32, S_FALSE);
	n.k = k;
	return st_insert((srt_tree **)s, (const srt_tnode *)&n);
}

/* #API: |Insert element (SMS_U32)|set; key|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
S_INLINE srt_bool sms_insert_u32(srt_set **s, uint32_t k)
{
	struct SMapu n;
	RETURN_IF(!s || (*s)->d.sub_type != SMS_U32, S_FALSE);
	n.k = k;
	return st_insert((srt_tree **)s, (const srt_tnode *)&n);
}

/* #API: |Insert element (SMS_I)|set; key|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
S_INLINE srt_bool sms_insert_i(srt_set **s, int64_t k)
{
	struct SMapI n;
	RETURN_IF(!s || (*s)->d.sub_type != SMS_I, S_FALSE);
	n.k = k;
	return st_insert((srt_tree **)s, (const srt_tnode *)&n);
}

/* #API: |Insert element (SMS_F)|set; key|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
S_INLINE srt_bool sms_insert_f(srt_set **s, float k)
{
	struct SMapF n;
	RETURN_IF(!s || (*s)->d.sub_type != SMS_F, S_FALSE);
	n.k = k;
	return st_insert((srt_tree **)s, (const srt_tnode *)&n);
}

/* #API: |Insert element (SMS_D)|set; key|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
S_INLINE srt_bool sms_insert_d(srt_set **s, double k)
{
	struct SMapD n;
	RETURN_IF(!s || (*s)->d.sub_type != SMS_D, S_FALSE);
	n.k = k;
	return st_insert((srt_tree **)s, (const srt_tnode *)&n);
}

/* #API: |Insert element (SMS_S)|set; key|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
S_INLINE srt_bool sms_insert_s(srt_set **s, const srt_string *k)
{
	struct SMapS n;
	srt_bool ins_ok;
	RETURN_IF(!s || (*s)->d.sub_type != SMS_S, S_FALSE);
#if 1 /* workaround */
	sso1_set(&n.k, k);
	ins_ok = st_insert((srt_tree **)s, (const srt_tnode *)&n);
	if (!ins_ok)
		sso1_free(&n.k);
	return ins_ok;
#else
	/* TODO: rw_add_SMS_S */
	n.k = k;
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n,
			    rw_add_SMS_S);
#endif
}

/*
 * Delete
 */

/* #API: |Delete element (SMS_I32)|set; key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
S_INLINE srt_bool sms_delete_i32(srt_set *s, int32_t k)
{
	return sm_delete_i32(s, k);
}

/* #API: |Delete element (SMS_U32)|set; key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
S_INLINE srt_bool sms_delete_u32(srt_set *s, uint32_t k)
{
	return sm_delete_u32(s, k);
}

/* #API: |Delete element (SMS_I)|set; key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
S_INLINE srt_bool sms_delete_i(srt_set *s, int64_t k)
{
	return sm_delete_i(s, k);
}

/* #API: |Delete element (SMS_F)|set; key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
S_INLINE srt_bool sms_delete_f(srt_set *s, float k)
{
	return sm_delete_f(s, k);
}

/* #API: |Delete element (SMS_D)|set; key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
S_INLINE srt_bool sms_delete_d(srt_set *s, double k)
{
	return sm_delete_d(s, k);
}

/* #API: |Delete element (SMS_S)|set; key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
S_INLINE srt_bool sms_delete_s(srt_set *s, const srt_string *k)
{
	return sm_delete_s(s, k);
}

/*
 * Enumeration
 */

/* #API: |Enumerate elements in a given key range (SMS_I32)|set; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sms_itr_i32(const srt_set *s, int32_t key_min, int32_t key_max, srt_set_it_i32 f, void *context);

/* #API: |Enumerate elements in a given key range (SMS_U32)|set; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sms_itr_u32(const srt_set *s, uint32_t key_min, uint32_t key_max, srt_set_it_u32 f, void *context);

/* #API: |Enumerate elements in a given key range (SMS_I)|set; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sms_itr_i(const srt_set *s, int64_t key_min, int64_t key_max, srt_set_it_i f, void *context);

/* #API: |Enumerate elements in a given key range (SMS_F)|set; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sms_itr_f(const srt_set *s, float key_min, float key_max, srt_set_it_f f, void *context);

/* #API: |Enumerate elements in a given key range (SMS_D)|set; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sms_itr_d(const srt_set *s, double key_min, double key_max, srt_set_it_d f, void *context);

/* #API: |Enumerate elements in a given key range (SMS_S)|set; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sms_itr_s(const srt_set *s, const srt_string *key_min, const srt_string *key_max, srt_set_it_s f, void *context);

/*
 * Unordered enumeration is inlined in order to get almost as fast
 * as array access after compiler optimization.
 */

/* #API: |Enumerate elements (SMS_I32)|set; element, 0 to n - 1|int32_t|O(1)|1;2|
int32_t sms_it_i32(const srt_set *s, srt_tndx i);
*/
#define sms_it_i32(s, i) sm_it_i32_k(s, i)

/* #API: |Enumerate elements (SMS_U32)|set; element, 0 to n - 1|uint32_t|O(1)|1;2|
uint32_t sms_it_u32(const srt_set *s, srt_tndx i);
*/
#define sms_it_u32(s, i) sm_it_u32_k(s, i)

/* #API: |Enumerate elements (SMS_I)|set; element, 0 to n - 1|int64_t|O(1)|1;2|
int32_t sms_it_i(const srt_set *s, srt_tndx i);
*/
#define sms_it_i(s, i) sm_it_i_k(s, i)

/* #API: |Enumerate elements (SMS_F)|set; element, 0 to n - 1|float|O(1)|1;2|
float sms_it_f(const srt_set *s, srt_tndx i);
*/
#define sms_it_f(s, i) sm_it_f_k(s, i)

/* #API: |Enumerate elements (SMS_D)|set; element, 0 to n - 1|double|O(1)|1;2|
double sms_it_d(const srt_set *s, srt_tndx i);
*/
#define sms_it_d(s, i) sm_it_d_k(s, i)

/* #API: |Enumerate elements (SMS_S)|set; element, 0 to n - 1|string|O(1)|1;2|
int32_t sms_it_s(const srt_set *s, srt_tndx i);
*/
#define sms_it_s(s, i) sm_it_s_k(s, i)

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* #ifndef SMSET_H */

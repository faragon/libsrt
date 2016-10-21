#ifndef SMAP_H
#define SMAP_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * smap.h
 *
 * #SHORTDOC map handling (key-value storage)
 *
 * #DOC Map functions handle key-value storage, whici is implemented as a
 * #DOC Red-Black tree (O(n log n) maximum complexity for insert/read/delete).
 * #DOC
 * #DOC
 * #DOC Supported key/value modes:
 * #DOC
 * #DOC
 * #DOC 	SM_I32I32: 32-bit integer key, 32-bit integer value
 * #DOC
 * #DOC 	SM_U32U32: 32-bit unsigned int key, 32-bit unsigned int value
 * #DOC
 * #DOC 	SM_IntInt: 64-bit int key, 64-bit int value
 * #DOC
 * #DOC 	SM_IntStr: 64-bit int key, string value
 * #DOC
 * #DOC 	SM_IntPtr: 64-bit int key, pointer value
 * #DOC
 * #DOC 	SM_StrInt: 64-bit string key, 64-bit int value
 * #DOC
 * #DOC 	SM_StrStr: string key, string value
 * #DOC
 * #DOC 	SM_StrPtr: string key, pointer value
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "sstring.h"
#include "svector.h"
#include "saux/stree.h"

/*
 * Structures
 */

enum eSM_Type
{
	SM_I32I32,
	SM_U32U32,
	SM_IntInt,
	SM_IntStr,
	SM_IntPtr,
	SM_StrInt,
	SM_StrStr,
	SM_StrPtr,
	SM_TotalTypes
};

struct SMapIx { stn_t n; int64_t k; };
struct SMapSx { stn_t n; ss_t *k; };
struct SMapSX { stn_t n; const ss_t *k; };
struct SMapii { stn_t n; int32_t k, v; };
struct SMapuu { stn_t n; uint32_t k, v; };
struct SMapII { struct SMapIx x; int64_t v; };
struct SMapIS { struct SMapIx x; ss_t *v; };
struct SMapIP { struct SMapIx x; const void *v; };
struct SMapSI { struct SMapSx x; int64_t v; };
struct SMapSS { struct SMapSx x; ss_t *v; };
struct SMapSP { struct SMapSx x; const void *v; };

typedef st_t sm_t;	/* "Hidden" structure (accessors are provided) */
			/* (map is immplemented as a tree)	       */

typedef sbool_t (*sm_enum_ii32_t)(int32_t k, int32_t v, void *context);
typedef sbool_t (*sm_enum_uu32_t)(uint32_t k, uint32_t v, void *context);
typedef sbool_t (*sm_enum_ii_t)(int64_t k, int64_t v, void *context);
typedef sbool_t (*sm_enum_is_t)(int64_t k, const ss_t *, void *context);
typedef sbool_t (*sm_enum_ip_t)(int64_t k, const void *, void *context);
typedef sbool_t (*sm_enum_si_t)(const ss_t *, int64_t v, void *context);
typedef sbool_t (*sm_enum_ss_t)(const ss_t *, const ss_t *, void *context);
typedef sbool_t (*sm_enum_sp_t)(const ss_t *, const void *, void *context);

/*
 * Allocation
 */

/*
#API: |Allocate map (stack)|map type; initial reserve|map|O(1)|1;2|
sm_t *sm_alloca(const enum eSM_Type t, const size_t n);
*/
#define sm_alloca(type, max_size)						\
	sm_alloc_raw(type, S_TRUE,						\
		     alloca(sd_alloc_size(sizeof(sm_t), sm_elem_size(type),	\
					  max_size, S_FALSE)),			\
		     sm_elem_size(type), max_size)

sm_t *sm_alloc_raw(const enum eSM_Type t, const sbool_t ext_buf, void *buffer, const size_t elem_size, const size_t max_size);

/* #API: |Allocate map (heap)|map type; initial reserve|map|O(1)|1;2| */
sm_t *sm_alloc(const enum eSM_Type t, const size_t initial_num_elems_reserve);

/* #API: |Make the map use the minimum possible memory|map|map reference (optional usage)|O(1) for allocators using memory remap; O(n) for naive allocators|1;2|
sm_t *sm_shrink(sm_t **m);
*/

/* #API: |Get map node size from map type|map type|bytes required for storing a single node|O(1)|1;2| */
S_INLINE uint8_t sm_elem_size(const enum eSM_Type t)
{
	switch (t) {
	case SM_I32I32:	return sizeof(struct SMapii);
	case SM_U32U32:	return sizeof(struct SMapuu);
	case SM_IntInt:	return sizeof(struct SMapII);
	case SM_IntStr:	return sizeof(struct SMapIS);
	case SM_IntPtr: return sizeof(struct SMapIP);
	case SM_StrInt:	return sizeof(struct SMapSI);
	case SM_StrStr:	return sizeof(struct SMapSS);
	case SM_StrPtr: return sizeof(struct SMapSP);
	default: break;
	}
	return 0;
}

/* #API: |Duplicate map|input map|output map|O(n)|1;2| */
sm_t *sm_dup(const sm_t *src);

/* #API: |Reset/clean map (keeping map type)|map|S_TRUE: OK, S_FALSE: invalid map|O(1) for simple maps, O(n) for maps having nodes with strings|0;2| */
sbool_t sm_reset(sm_t *m);

/*
#API: |Free one or more maps (heap)|map; more maps (optional)|-|O(1) for simple maps, O(n) for maps having nodes with strings|1;2|
void sm_free(sm_t **m, ...)
*/
#define sm_free(...) sm_free_aux(__VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
void sm_free_aux(sm_t **m, ...);

SD_BUILDFUNCS_FULL_ST(sm)

/*
#API: |Ensure space for extra elements|map;number of extra elements|extra size allocated|O(1)|0;2|
size_t sm_grow(sm_t **m, const size_t extra_elems)

#API: |Ensure space for elements|map;absolute element reserve|reserved elements|O(1)|0;2|
size_t sm_reserve(sm_t **m, const size_t max_elems)

#API: |Make the map use the minimum possible memory|map|map reference (optional usage)|O(1) for allocators using memory remap; O(n) for naive allocators|1;2|
sm_t *sm_shrink(sm_t **m);

#API: |Get map size|map|Map number of elements|O(1)|1;2|
size_t sm_size(const sm_t *m);

#API: |Allocated space|map|current allocated space (vector elements)|O(1)|1;2|
size_t sm_capacity(const sm_t *m);

#API: |Preallocated space left|map|allocated space left|O(1)|1;2|
size_t sm_capacity_left(const sm_t *m);

#API: |Tells if a map is empty (zero elements)|map|S_TRUE: empty vector; S_FALSE: not empty|O(1)|1;2|
sbool_t sm_empty(const sm_t *m)
*/

/*
 * Copy
 */

/* #API: |Overwrite map with a map copy|output map; input map|output map reference (optional usage)|O(n)|0;2| */
sm_t *sm_cpy(sm_t **m, const sm_t *src);

/*
 * Random access
 */

/* #API: |Access to int32-int32 map|map; int32 key|int32|O(log n)|1;2| */
int32_t sm_at_ii32(const sm_t *m, const int32_t k);

/* #API: |Access to uint32-uint32 map|map; uint32 key|uint32|O(log n)|1;2| */
uint32_t sm_at_uu32(const sm_t *m, const uint32_t k);

/* #API: |Access to integer-interger map|map; integer key|integer|O(log n)|1;2| */
int64_t sm_at_ii(const sm_t *m, const int64_t k);

/* #API: |Access to integer-string map|map; integer key|string|O(log n)|1;2| */
const ss_t *sm_at_is(const sm_t *m, const int64_t k);

/* #API: |Access to integer-pointer map|map; integer key|pointer|O(log n)|1;2| */
const void *sm_at_ip(const sm_t *m, const int64_t k);

/* #API: |Access to string-integer map|map; string key|integer|O(log n)|1;2| */
int64_t sm_at_si(const sm_t *m, const ss_t *k);

/* #API: |Access to string-string map|map; string key|string|O(log n)|1;2| */
const ss_t *sm_at_ss(const sm_t *m, const ss_t *k);

/* #API: |Access to string-pointer map|map; string key|pointer|O(log n)|1;2| */
const void *sm_at_sp(const sm_t *m, const ss_t *k);

/*
 * Existence check
 */

/* #API: |Map element count/check|map; 32-bit unsigned integer key|S_TRUE: element found; S_FALSE: not in the map|O(log n)|1;2| */
sbool_t sm_count_u(const sm_t *m, const uint32_t k);

/* #API: |Map element count/check|map; integer key|S_TRUE: element found; S_FALSE: not in the map|O(log n)|1;2| */
sbool_t sm_count_i(const sm_t *m, const int64_t k);

/* #API: |Map element count/check|map; string key|S_TRUE: element found; S_FALSE: not in the map|O(log n)|1;2| */
sbool_t sm_count_s(const sm_t *m, const ss_t *k);

/*
 * Insert
 */

/* #API: |Insert into int32-int32 map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
sbool_t sm_insert_ii32(sm_t **m, const int32_t k, const int32_t v);

/* #API: |Insert into uint32-uint32 map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
sbool_t sm_insert_uu32(sm_t **m, const uint32_t k, const uint32_t v);

/* #API: |Insert into int-int map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
sbool_t sm_insert_ii(sm_t **m, const int64_t k, const int64_t v);

/* #API: |Insert into int-string map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
sbool_t sm_insert_is(sm_t **m, const int64_t k, const ss_t *v);

/* #API: |Insert into int-pointer map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
sbool_t sm_insert_ip(sm_t **m, const int64_t k, const void *v);

/* #API: |Insert into string-int map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
sbool_t sm_insert_si(sm_t **m, const ss_t *k, const int64_t v);

/* #API: |Insert into string-string map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
sbool_t sm_insert_ss(sm_t **m, const ss_t *k, const ss_t *v);

/* #API: |Insert into string-pointer map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
sbool_t sm_insert_sp(sm_t **m, const ss_t *k, const void *v);

/* #API: |Increment value into int32-int32 map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|0;2| */
sbool_t sm_inc_ii32(sm_t **m, const int32_t k, const int32_t v);

/* #API: |Increment into uint32-uint32 map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|0;2| */
sbool_t sm_inc_uu32(sm_t **m, const uint32_t k, const uint32_t v);

/* #API: |Increment into int-int map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|0;2| */
sbool_t sm_inc_ii(sm_t **m, const int64_t k, const int64_t v);

/* #API: |Increment into string-int map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|0;2| */
sbool_t sm_inc_si(sm_t **m, const ss_t *k, const int64_t v);

/*
 * Delete
 */

/* #API: |Delete map element|map; integer key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|0;2| */
sbool_t sm_delete_i(sm_t *m, const int64_t k);

/* #API: |Delete map element|map; string key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|0;2| */
sbool_t sm_delete_s(sm_t *m, const ss_t *k);

/*
 * Enumeration / export data
 */

#if 0
/* #NOTAPI: |Enumerate map elements using callback (in-order traverse)|map; traverse function; traverse function context|Elements processed|O(n); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|0;2| */
ssize_t sm_enum_inorder(const sm_t *m, st_traverse f, void *context);
#endif

/* #API: |Sort map to vector|map; output vector for keys; output vector for values|Number of map elements|O(n)|1;2| */
ssize_t sm_sort_to_vectors(const sm_t *m, sv_t **kv, sv_t **vv);

/*
 * Inlined functions
 */

#define S_SM_ENUM_AUX_K(NT, m, i, n_k, def_k)		\
	const NT *n = (const NT *)st_enum_r(m, i);	\
	RETURN_IF(!n, def_k);				\
	return n_k

#define S_SM_ENUM_AUX_V(NT, m, i, n_v, def_v)		\
	const NT *n = (const NT *)st_enum_r(m, i);	\
	RETURN_IF(!n, def_v);				\
	return n_v

/* #API: |Enumerate int32-int32 map keys|map; element, 0 to n - 1|sbool_t|O(log n)|0;1| */
S_INLINE int32_t sm_enum_i32_k(const sm_t *m, const stndx_t i)
{
	S_SM_ENUM_AUX_K(struct SMapii, m, i, n->k, 0);
}

/* #API: |Enumerate int32-int32 map values|map; element, 0 to n - 1|sbool_t|O(log n)|0;1| */
S_INLINE int32_t sm_enum_i32_v(const sm_t *m, const stndx_t i)
{
	S_SM_ENUM_AUX_V(struct SMapii, m, i, n->v, 0);
}

/* #API: |Enumerate uint32-uint32 map keys|map; element, 0 to n - 1|S_INLINE sbool_t|O(log n)|0;1| */
S_INLINE uint32_t sm_enum_u32_k(const sm_t *m, const stndx_t i)
{
	S_SM_ENUM_AUX_K(struct SMapuu, m, i, n->k, 0);
}

/* #API: |Enumerate uint32-uint32 map values|map; element, 0 to n - 1|S_INLINE sbool_t|O(log n)|0;1| */
S_INLINE uint32_t sm_enum_u32_v(const sm_t *m, const stndx_t i)
{
	S_SM_ENUM_AUX_V(struct SMapuu, m, i, n->v, 0);
}

/* #API: |Enumerate integer-interger map keys|map; element, 0 to n - 1|S_INLINE sbool_t|O(log n)|0;1| */
S_INLINE int64_t sm_enum_i_k(const sm_t *m, const stndx_t i)
{
	S_SM_ENUM_AUX_K(struct SMapIx, m, i, n->k, 0);
}

/* #API: |Enumerate integer-interger map values|map; element, 0 to n - 1|S_INLINE sbool_t|O(log n)|0;1| */
S_INLINE int64_t sm_enum_ii_v(const sm_t *m, const stndx_t i)
{
	S_SM_ENUM_AUX_V(struct SMapII, m, i, n->v, 0);
}

/* #API: |Enumerate integer-string map values|map; element, 0 to n - 1|S_INLINE sbool_t|O(log n)|0;1| */
S_INLINE const ss_t *sm_enum_is_v(const sm_t *m, const stndx_t i)
{
	S_SM_ENUM_AUX_V(struct SMapIS, m, i, n->v, ss_void);
}

/* #API: |Enumerate integer-pointer map values|map; element, 0 to n - 1|S_INLINE sbool_t|O(log n)|0;1| */
S_INLINE const void *sm_enum_ip_v(const sm_t *m, const stndx_t i)
{
	S_SM_ENUM_AUX_V(struct SMapIP, m, i, n->v, NULL);
}

/* #API: |Enumerate string-* map keys|map; element, 0 to n - 1|S_INLINE sbool_t|O(log n)|0;1| */
S_INLINE const ss_t *sm_enum_s_k(const sm_t *m, const stndx_t i)
{
	S_SM_ENUM_AUX_K(struct SMapSx, m, i, n->k, ss_void);
}

/* #API: |Enumerate string-integer map values|map; element, 0 to n - 1|S_INLINE sbool_t|O(log n)|0;1| */
S_INLINE int64_t sm_enum_si_v(const sm_t *m, const stndx_t i)
{
	S_SM_ENUM_AUX_V(struct SMapSI, m, i, n->v, 0);
}

/* #API: |Enumerate string-string map values|map; element, 0 to n - 1|S_INLINE sbool_t|O(log n)|0;1| */
S_INLINE const ss_t *sm_enum_ss_v(const sm_t *m, const stndx_t i)
{
	S_SM_ENUM_AUX_V(struct SMapSS, m, i, n->v, ss_void);
}

/* #API: |Enumerate string-pointer map|map; element, 0 to n - 1|S_INLINE sbool_t|O(log n)|0;1| */
S_INLINE const void *sm_enum_sp_v(const sm_t *m, const stndx_t i)
{
	S_SM_ENUM_AUX_V(struct SMapSP, m, i, n->v, NULL);
}

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* #ifndef SMAP_H */


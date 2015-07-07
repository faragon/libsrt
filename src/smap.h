#ifndef SMAP_H
#define SMAP_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * smap.h
 *
 * Map handling
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "stree.h"
#include "svector.h"
#include "sstring.h"

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
        SM_LAST = SM_StrStr,
	SM_TotalTypes
};

struct SMapIx { stn_t n; sint_t k; };
struct SMapSx { stn_t n; ss_t *k; };
struct SMapSX { stn_t n; const ss_t *k; };
struct SMapii { stn_t n; sint32_t k, v; };
struct SMapuu { stn_t n; suint32_t k, v; };
struct SMapII { struct SMapIx x; sint_t v; };
struct SMapIS { struct SMapIx x; ss_t *v; };
struct SMapIP { struct SMapIx x; const void *v; };
struct SMapSI { struct SMapSx x; sint_t v; };
struct SMapSS { struct SMapSx x; ss_t *v; };
struct SMapSP { struct SMapSx x; const void *v; };

typedef st_t sm_t;	/* "Hidden" structure (accessors are provided) */
			/* (map is immplemented as a tree)	       */

/*
 * Allocation
 */

/*
#API: |Allocate map (stack)|map type; initial reserve|map|O(1)|
sm_t *sm_alloca(const enum eSM_Type t, const size_t n);
*/
#define sm_alloca(type, num_elems)					\
	sm_alloc_raw(							\
	type, S_TRUE,							\
	alloca(ST_SIZE_TO_ALLOC_SIZE(num_elems, sm_elem_size(type))),	\
	ST_SIZE_TO_ALLOC_SIZE(num_elems, sm_elem_size(type)))
sm_t *sm_alloc_raw(const enum eSM_Type t, const sbool_t ext_buf, void *buffer, const size_t buffer_size);

/* #API: |Allocate map (heap)|map type; initial reserve|map|O(1)| */
sm_t *sm_alloc(const enum eSM_Type t, const size_t initial_num_elems_reserve);


/* #API: |Make the map use the minimum possible memory|map|map reference (optional usage)|O(1) for allocators using memory remap; O(n) for naive allocators| */
sm_t *sm_shrink(sm_t **s);

/* #API: |Get map node size from map type|map type|bytes required for storing a single node|O(1)| */
size_t sm_elem_size(const enum eSM_Type t);

/* #API: |Duplicate map|input map|output map|O(n)| */
sm_t *sm_dup(const sm_t *src);

/* #API: |Reset/clean map (keeping map type)|map|S_TRUE: OK, S_FALSE: invalid map|O(1) for simple maps, O(n) for maps having nodes with strings| */
sbool_t sm_reset(sm_t *m);

/* #API: |Set map integer and string defaults|map; integer default; string default||O(1)| */
void sm_set_defaults(sm_t *m, const size_t i_def_v, const ss_t *s_def_v);

/*
#API: |Free one or more maps (heap)|map; more maps (optional)||O(1) for simple maps, O(n) for maps having nodes with strings|
void sm_free(sm_t **m, ...)
*/
#define sm_free(...) sm_free_aux(S_NARGS_STPW(__VA_ARGS__), __VA_ARGS__)
void sm_free_aux(const size_t nargs, sm_t **s, ...);

/*
 * Accessors
 */

/* #API: |Get map size|map|Map number of elements|O(1)| */
size_t sm_size(const sm_t *m);

/*
 * Random access
 */

/* #API: |Access to int32-int32 map|map; int32 key|int32|O(log n)| */
sint32_t sm_ii32_at(const sm_t *m, const sint32_t k);

/* #API: |Access to uint32-uint32 map|map; uint32 key|uint32|O(log n)| */
suint32_t sm_uu32_at(const sm_t *m, const suint32_t k);

/* #API: |Access to integer-interger map|map; integer key|integer|O(log n)| */
sint_t sm_ii_at(const sm_t *m, const sint_t k);

/* #API: |Access to integer-string map|map; integer key|string|O(log n)| */
const ss_t *sm_is_at(const sm_t *m, const sint_t k);

/* #API: |Access to integer-pointer map|map; integer key|pointer|O(log n)| */
const void *sm_ip_at(const sm_t *m, const sint_t k);

/* #API: |Access to string-integer map|map; string key|integer|O(log n)| */
sint_t sm_si_at(const sm_t *m, const ss_t *k);

/* #API: |Access to string-string map|map; string key|string|O(log n)| */
const ss_t *sm_ss_at(const sm_t *m, const ss_t *k);

/* #API: |Access to string-pointer map|map; string key|pointer|O(log n)| */
const void *sm_sp_at(const sm_t *m, const ss_t *k);

/*
 * Existence check
 */

/* #API: |Map element count/check|map; 32-bit unsigned integer key|S_TRUE: element found; S_FALSE: not in the map|O(log n)| */
sbool_t sm_u_count(const sm_t *m, const suint32_t k);

/* #API: |Map element count/check|map; integer key|S_TRUE: element found; S_FALSE: not in the map|O(log n)| */
sbool_t sm_i_count(const sm_t *m, const sint_t k);

/* #API: |Map element count/check|map; string key|S_TRUE: element found; S_FALSE: not in the map|O(log n)| */
sbool_t sm_s_count(const sm_t *m, const ss_t *k);

/*
 * Insert
 */

/* #API: |Insert into int32-int32 map|map; key; value||O(log n)| */
sbool_t sm_ii32_insert(sm_t **m, const sint32_t k, const sint32_t v);

/* #API: |Insert into uint32-uint32 map|map; key; value||O(log n)| */
sbool_t sm_uu32_insert(sm_t **m, const suint32_t k, const suint32_t v);

/* #API: |Insert into int-int map|map; key; value||O(log n)| */
sbool_t sm_ii_insert(sm_t **m, const sint_t k, const sint_t v);

/* #API: |Insert into int-string map|map; key; value||O(log n)| */
sbool_t sm_is_insert(sm_t **m, const sint_t k, const ss_t *v);

/* #API: |Insert into int-pointer map|map; key; value||O(log n)| */
sbool_t sm_ip_insert(sm_t **m, const sint_t k, const void *v);

/* #API: |Insert into string-int map|map; key; value||O(log n)| */
sbool_t sm_si_insert(sm_t **m, const ss_t *k, const sint_t v);

/* #API: |Insert into string-string map|map; key; value||O(log n)| */
sbool_t sm_ss_insert(sm_t **m, const ss_t *k, const ss_t *v);

/* #API: |Insert into string-pointer map|map; key; value||O(log n)| */
sbool_t sm_sp_insert(sm_t **m, const ss_t *k, const void *v);

/*
 * Delete
 */

/* #API: |Delete map element|map; integer key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)| */
sbool_t sm_i_delete(sm_t *m, const sint_t k);

/* #API: |Delete map element|map; string key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)| */
sbool_t sm_s_delete(sm_t *m, const ss_t *k);

/*
 * Enumeration / export data
 */

/* #API: |Enumerate map elements (unordered)|map; element, 0 to n - 1, being n the number of elements|Element offset (0..n-1)|O(1)| */
const stn_t *sm_enum(const sm_t *m, const stndx_t i);

/* #API: |Enumerate map elements using callback (in-order traverse)|map; traverse function; traverse function context|Elements processed|O(n)| */
ssize_t sm_inorder_enum(const sm_t *m, st_traverse f, void *context);

/* #API: |Sort map to vector|map; output vector for keys; output vector for values|Number of map elements|O(n)| */
ssize_t sm_sort_to_vectors(const sm_t *m, sv_t **kv, sv_t **vv);

#ifdef __cplusplus
}; /* extern "C" { */
#endif

#endif /* #ifndef SMAP_H */


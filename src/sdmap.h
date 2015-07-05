#ifndef SDMAP_H
#define SDMAP_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * sdmap.h
 *
 * Distributed map handling (same-process clustering)
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "smap.h"

/*
 * Structures and types
 */

typedef struct S_DMap sdm_t;
typedef size_t (*sdm_i_hash_t)(const sdm_t *dm, const sint_t k);
typedef size_t (*sdm_s_hash_t)(const sdm_t *dm, const ss_t *k);

struct S_DMap
{
	size_t nmaps;
	sdm_i_hash_t ih;
	sdm_s_hash_t sh;
	sm_t *maps[1];
};

/*
 * Allocation
 */

/* #API: |Allocate distributed map (heap)|map type; number of submaps; initial reserve|map|O(1)| */
sdm_t *sdm_alloc(const enum eSM_Type t, const size_t number_of_submaps, const size_t initial_num_elems_reserve);

/* #API: |Make the dynamic map use the minimum possible memory|map||O(1) for allocators using memory remap; O(n) for naive allocators| */
void sdm_shrink(sdm_t **s);

/* #API: |Duplicate distributed map|input map|output map|O(n)| */
sdm_t *sdm_dup(const sdm_t *src);

/* #API: |Reset/clean distributed map (keeping map type)|map|S_TRUE: OK, S_FALSE: invalid map|O(1) for simple maps, O(n) for maps having nodes with strings| */
sbool_t sdm_reset(sdm_t *m);

/*
#API: |Free one or more distributed maps|map; more distributed maps (optional)||O(1) for simple dmaps, O(n) for dmaps having nodes with strings|
void sdm_free(sdm_t **dm, ...)
*/
#define sdm_free(...) sdm_free_aux(S_NARGS_SDMPW(__VA_ARGS__), __VA_ARGS__)
void sdm_free_aux(const size_t nargs, sdm_t **s, ...);

/*
 * Routing
 */

/* #API: |Set routing functions|map; routing function for integer keys (if NULL, a "fair" hash will be used); routing function for string keys (if NULL, also a "fair" hash will be used)||O(1)| */
void sdm_set_routing(sdm_t *dm, sdm_i_hash_t irf, sdm_s_hash_t srf);

/* #API: |Get route to subtree (integer key)|dmap; key|subtree id|O(1)| */
size_t sdm_i_route(const sdm_t *dm, const sint_t k);

/* #API: |Get route to subtree (string key)|dmap; key|subtree id|O(1)| */
size_t sdm_s_route(const sdm_t *dm, const ss_t *k);

/*
 * Accessors
 */

/* #API: |Get number of submaps|dmap|Number of submaps|O(1)| */
size_t sdm_size(const sdm_t *m);

/* #API: |Get submap pointer vector|dmap|map vector|O(1)| */
sm_t **sdm_submaps(sdm_t *m);

/* #API: |Get submap pointer vector (read-only)|dmap|map vector|O(1)| */
const sm_t **sdm_submaps_r(const sdm_t *m);

#ifdef __cplusplus
}; /* extern "C" { */
#endif

#endif /* #ifndef SDMAP_H */


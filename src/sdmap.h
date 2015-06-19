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

sdm_t *sdm_alloc(const enum eSM_Type t, const size_t number_of_submaps, const size_t initial_num_elems_reserve);
void sdm_free_aux(const size_t nargs, sdm_t **s, ...);
void sdm_shrink(sdm_t **s);
sdm_t *sdm_dup(const sdm_t *src);
sbool_t sdm_reset(sdm_t *m);

#define sdm_free(...) sdm_free_aux(S_NARGS_SDMPW(__VA_ARGS__), __VA_ARGS__)

/*
 * Routing
 */

void sdm_set_routing(sdm_t *dm, sdm_i_hash_t irf, sdm_s_hash_t srf);
size_t sdm_i_route(const sdm_t *dm, const sint_t k);
size_t sdm_s_route(const sdm_t *dm, const ss_t *k);

/*
 * Accessors
 */

size_t sdm_size(const sdm_t *m);
sm_t **sdm_submaps(sdm_t *m);
const sm_t **sdm_submaps_r(const sdm_t *m);

#ifdef __cplusplus
}; /* extern "C" { */
#endif

#endif /* #ifndef SDMAP_H */


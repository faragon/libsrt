#ifndef SDMAP_H
#define SDMAP_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * sdmap.h   ** incomplete, work in progress **
 *
 * Distributed map handling (same-process clustering)
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "smap.h"

/*
 * Structures and types
 */

typedef void(*sdm_i_hash_t)(const sint_t k);
typedef void(*sdm_u_hash_t)(const suint32_t k);
typedef void(*sdm_s_hash_t)(const ss_t *k);

struct S_DMap
{
	size_t nmaps;
	sdm_i_hash_t ih;
	sdm_u_hash_t uh;
	sdm_s_hash_t sh;
	sm_t *maps[1];
};

typedef struct S_DMap sdm_t;

/*
 * Allocation
 */

sdm_t *sdm_alloc(const enum eSM_Type t, const size_t number_of_submaps, const size_t initial_num_elems_reserve);
void sdm_free_aux(const size_t nargs, sdm_t **s, ...);
void sdm_shrink_to_fit(sdm_t **s);
sdm_t *sdm_dup(const sdm_t *src);
sbool_t sdm_reset(sdm_t *m);

/* SD_BUILDPROTS(st) */

#define sdm_free(...) sdm_free_aux(S_NARGS_STPW(__VA_ARGS__), __VA_ARGS__)

/*
 * Hash function change (default one is "fair"/uniform)
 */

void sdm_set_i_hash(sdm_t *m, sdm_i_hash_t f);
void sdm_set_u_hash(sdm_t *m, sdm_u_hash_t f);
void sdm_set_s_hash(sdm_t *m, sdm_s_hash_t f);
void sdm_set_hash_defaults(sdm_t *m);

/*
 * Key to submap routing
 */

int sdm_u_route(const sdm_t *m, const suint32_t k);
int sdm_i_route(const sdm_t *m, const sint_t k);
int sdm_s_route(const sdm_t *m, const ss_t *k);

/*
 * Accessors
 */

size_t sdm_size(const sdm_t *m);
sm_t **sdm_submaps(const sdm_t *m);

#ifdef __cplusplus
}; /* extern "C" { */
#endif

#endif /* #ifndef SDMAP_H */


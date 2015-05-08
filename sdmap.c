/*
 * sdmap.c
 *
 * Distributed map handling (same-process clustering)
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */ 

#include "sdmap.h"
#include "scommon.h"

/*
 * Macros
 */

#ifndef SDM_DEF_S_HASH_MAX_SIZE
#define SDM_DEF_S_HASH_MAX_SIZE 16
#endif


#define SDM_FUN_DMAP(CHK, REF, FUN, FPFREF)		\
	if (CHK) {					\
		size_t i = 0, n = sdm_size(REF);	\
		for (; i < n; i++)			\
			FUN(FPFREF->maps[i]);		\
	}
#define SDM_FUN_PPDMAP(m, FUN)	SDM_FUN_DMAP(m && *m, *m, FUN, &(*m))
#define SDM_FUN_PDMAP(m, FUN)	SDM_FUN_DMAP(m, m, FUN, m)

/*
 * Internal functions
 */

static size_t sdm_default_i_hash(const sdm_t *dm, const sint_t k)
{
	S_ASSERT(dm && dm->nmaps);
	return dm ? (k % dm->nmaps) : 0;
}

static size_t sdm_default_s_hash(const sdm_t *dm, const ss_t *k)
{
	S_ASSERT(dm && k && dm->nmaps);
	/*
	 * Simple routing hash for strings: there is no need for string
	 * full scan, as we just need to route to a subtree and not a
	 * hash map bucket. That's the reason of scanning just the first
	 * 16 bytes of the string. If not enough for you, you can use a
	 * custom routing function (or pass a bigger value through the
	 * Makefile).
	 */
	const unsigned h = ss_csum32(k, SDM_DEF_S_HASH_MAX_SIZE);
	return dm && k ? ((h / 2 + h) % dm->nmaps) : 0;
}

/*
 * Allocation
 */

/* #API: |Allocate distributed map|map type; initial reserve|map|O(1)| */

sdm_t *sdm_alloc(const enum eSM_Type t, const size_t nsubmaps, const size_t initial_reserve)
{
	ASSERT_RETURN_IF(nsubmaps < 1, NULL);
	size_t alloc_size = sizeof(sdm_t) + sizeof(sm_t *) * (nsubmaps - 1);
	sdm_t *dm = (sdm_t *)malloc(alloc_size);
	ASSERT_RETURN_IF(!dm, NULL);
	memset(dm, 0, alloc_size);
	size_t i = 0, nelems = (initial_reserve / nsubmaps) + 1;
	for (; i < nsubmaps; i++) {
		if (!(dm->maps[i] = sm_alloc(t, nelems)))
			break;	/* Allocation error */
	}
	if (i != nsubmaps) { /* Handle allocation error */
		for (i = 0; i < nsubmaps; i++) {
			if (dm->maps[i])
				free(dm->maps[i]);
			else
				break;
		}
		free(dm);
		dm = NULL;
	} else {
		/* Set routing defaults */
		sdm_set_routing(dm, NULL, NULL);
		dm->nmaps = nsubmaps;
	}
	return dm;
}

/*
#API: |Free one or more distributed maps|map; more distributed maps (optional)||O(1) for simple dmaps, O(n) for dmaps having nodes with strings|
void sdm_free(sdm_t **dm, ...)
*/

static void sdm_free_aux1(sdm_t **dm)
{
	if (dm && *dm) {
		SDM_FUN_PPDMAP(dm, sm_free);
		free(*dm);
		*dm = NULL;
	}
}

void sdm_free_aux(const size_t nargs, sdm_t **dm, ...)
{
	va_list ap;
	va_start(ap, dm);
	if (dm)
		sdm_free_aux1(dm);
	if (nargs > 1) {
		size_t i = 1;
		for (; i < nargs; i++)
			sdm_free_aux1(va_arg(ap, sdm_t **));
	}
	va_end(ap);
}

/* #API: |Make the dynamic map use the minimum possible memory|map||O(1) for allocators using memory remap; O(n) for naive allocators| */

void sdm_shrink_to_fit(sdm_t **dm)
{
        SDM_FUN_PPDMAP(dm, sm_shrink_to_fit);
}

/* #API: |Duplicate distributed map|input map|output map|O(n)| */

sdm_t *sdm_dup(const sdm_t *src)
{
	ASSERT_RETURN_IF(!src, NULL);
	size_t nsubmaps = sdm_size(src);
	ASSERT_RETURN_IF(!nsubmaps, NULL);
	const sm_t **maps = sdm_submaps_r(src);
	ASSERT_RETURN_IF(!maps, NULL);
	ASSERT_RETURN_IF(!maps[0], NULL);
	size_t nmaps = sdm_size(src);
	enum eSM_Type t = (enum eSM_Type)maps[0]->f.type;
	ASSERT_RETURN_IF(t < SM_FIRST || t >SM_LAST, NULL);
	size_t alloc_size = sizeof(sdm_t) + sizeof(sm_t *) * (nsubmaps - 1);
	sdm_t *dm = (sdm_t *)malloc(alloc_size);
	ASSERT_RETURN_IF(!dm, NULL);
	size_t i = 0;
	for (; i < nsubmaps; i++ ) {
		if (!(dm->maps[i] = sm_dup(maps[i])))
			break; /* Allocation error */
	}
	if (i != nsubmaps) { /* Handle allocation error */
		for (; i < nsubmaps; i++ ) {
			if (dm->maps[i])
				sm_free(&dm->maps[i]);
			else
				break;
		}
		free(dm);
		dm = NULL;
	}
	return dm;
}

/* #API: |Reset/clean distributed map (keeping map type)|map|S_TRUE: OK, S_FALSE: invalid map|O(1) for simple maps, O(n) for maps having nodes with strings| */

sbool_t sdm_reset(sdm_t *dm)
{
	sbool_t res_ok = S_TRUE;
	SDM_FUN_PDMAP(dm, res_ok &= sm_reset);
	return res_ok;
}

/*
 * Routing
 */

/* #API: |Set routing functions|map; routing function for integer keys (if NULL, a "fair" hash will be used); routing function for string keys (if NULL, also a "fair" hash will be used)||O(1)| */

void sdm_set_routing(sdm_t *dm, sdm_i_hash_t irf, sdm_s_hash_t srf)
{
	if (dm) {
		dm->ih = irf ? irf : sdm_default_i_hash;
		dm->sh = srf ? srf : sdm_default_s_hash;
	}
}

/* #API: |Get route to subtree (integer key)|dmap; key|subtree id|O(1)| */

size_t sdm_i_route(const sdm_t *dm, const sint_t k)
{
	S_ASSERT(dm);
	return dm ? dm->ih(dm, k) : 0;
}

/* #API: |Get route to subtree (string key)|dmap; key|subtree id|O(1)| */

size_t sdm_s_route(const sdm_t *dm, const ss_t *k)
{
	S_ASSERT(dm);
	return dm ? dm->sh(dm, k) : 0;
}

/*
 * Accessors
 */

/* #API: |Get number of submaps|dmap|Number of submaps|O(1)| */

size_t sdm_size(const sdm_t *dm)
{
	S_ASSERT(dm);
	return dm ? dm->nmaps : 0;
}

/* #API: |Get submap pointer vector|dmap|map vector|O(1)| */

sm_t **sdm_submaps(sdm_t *dm)
{
	return (sm_t **)sdm_submaps_r(dm);
}

/* #API: |Get submap pointer vector (read-only)|dmap|map vector|O(1)| */

const sm_t **sdm_submaps_r(const sdm_t *dm)
{
	S_ASSERT(dm);
	return dm ? (const sm_t **)dm->maps : NULL;
}


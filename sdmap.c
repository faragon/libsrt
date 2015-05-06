/*
 * sdmap.c   ** incomplete, work in progress **
 *
 * Distributed map handling (same-process clustering)
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */ 

#include "sdmap.h"
#include "scommon.h"

/*
 * Internal functions
 */

/* TODO
   - Add default hashing functions ("fair"/uniform)
 */

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
	}
	return dm;
}

/*
#API: |Free one or more distributed maps|map; more distributed maps (optional)||O(1) for simple dmaps, O(n) for dmaps having nodes with strings|
void sdm_free(sdm_t **m, ...)
*/

#define SDM_FUN_MAP(m, FUN)			\
	if (m && (*m)) {			\
		size_t nelems = sdm_size(*m);	\
		size_t i = 0;			\
		for (; i < nelems; i++ )	\
			FUN(&(*m)->maps[i]);	\
	}

static void sdm_free_aux1(sdm_t **m)
{
	SDM_FUN_MAP(m, sm_free);
}

void sdm_free_aux(const size_t nargs, sdm_t **m, ...)
{
	va_list ap;
	va_start(ap, m);
	if (m)
		sdm_free_aux1(m);
	if (nargs > 1) {
		size_t i = 1;
		for (; i < nargs; i++)
			sdm_free_aux1(va_arg(ap, sdm_t **));
	}
	va_end(ap);
}

/* #API: |Make the dynamic map use the minimum possible memory|map||O(1) for allocators using memory remap; O(n) for naive allocators| */

void sdm_shrink_to_fit(sdm_t **m)
{
        SDM_FUN_MAP(m, sm_shrink_to_fit);
}

/* #API: |Duplicate distributed map|input map|output map|O(n)| */

sdm_t *sdm_dup(const sdm_t *src)
{
	ASSERT_RETURN_IF(!src, NULL);
	size_t nsubmaps = sdm_size(src);
	ASSERT_RETURN_IF(!nsubmaps, NULL);
	sm_t **maps = sdm_submaps(src);
	ASSERT_RETURN_IF(!maps, NULL);
	ASSERT_RETURN_IF(!maps[0], NULL);
	size_t nmaps = sdm_size(src);
	enum eSM_Type t = maps[0]->f.type;
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

sbool_t sdm_reset(sdm_t *m)
{
#if 1
	return S_FALSE;
#else
	sbool_t res_ok = S_TRUE;
        SDM_FUN_MAP(&m, res_ok &&= sm_reset);
	return res_ok;
#endif
}

/*
 * Hash function change (default one is "fair"/uniform)
 */

/* #API: ||||| */

void sdm_set_i_hash(sdm_t *m, sdm_i_hash_t f)
{
	/* TODO */
}

/* #API: ||||| */

void sdm_set_u_hash(sdm_t *m, sdm_u_hash_t f)
{
	/* TODO */
}

/* #API: ||||| */

void sdm_set_s_hash(sdm_t *m, sdm_s_hash_t f)
{
	/* TODO */
}

/* #API: ||||| */

void sdm_set_hash_defaults(sdm_t *m)
{
	/* TODO */
}

/*
 * Key to submap routing
 */

/* #API: ||||| */

int sdm_u_route(const sdm_t *m, const suint32_t k)
{
	return -1; /* TODO */
}

/* #API: ||||| */

int sdm_i_route(const sdm_t *m, const sint_t k)
{
	return -1; /* TODO */
}

/* #API: ||||| */

int sdm_s_route(const sdm_t *m, const ss_t *k)
{
	return -1; /* TODO */
}

/*
 * Accessors
 */

size_t sdm_size(const sdm_t *m)
{
	return 0; /* TODO */
}

/* #API: ||||| */

sm_t **sdm_submaps(const sdm_t *m)
{
	return 0; /* TODO */
}



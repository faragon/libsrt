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
        SM_FIRST,
	SM_I32I32 = SM_FIRST,
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

#define sm_alloca(type, num_elems)					\
	sm_alloc_raw(							\
	type, S_TRUE,							\
	num_elems,							\
	alloca(ST_SIZE_TO_ALLOC_SIZE(num_elems, sm_elem_size(type))),	\
	ST_SIZE_TO_ALLOC_SIZE(num_elems, sm_elem_size(type)))
sm_t *sm_alloc_raw(const enum eSM_Type t, const sbool_t ext_buf,
		  const size_t num_elems, void *buffer,
		  const size_t buffer_size);
sm_t *sm_alloc(const enum eSM_Type t, const size_t initial_num_elems_reserve);
void sm_free_aux(const size_t nargs, sm_t **s, ...);
sm_t *sm_shrink(sm_t **s);
size_t sm_elem_size(const enum eSM_Type t);
sm_t *sm_dup(const sm_t *src);
sbool_t sm_reset(sm_t *m);
void sm_set_defaults(sm_t *m, const size_t i_def_v, const ss_t *s_def_v);

#define sm_free(...) sm_free_aux(S_NARGS_STPW(__VA_ARGS__), __VA_ARGS__)

/*
 * Accessors
 */

size_t sm_size(const sm_t *m);

/*
 * Random access
 */

sint32_t sm_ii32_at(const sm_t *m, const sint32_t k);
suint32_t sm_uu32_at(const sm_t *m, const suint32_t k);
sint_t sm_ii_at(const sm_t *m, const sint_t k);
const ss_t *sm_is_at(const sm_t *m, const sint_t k);
const void *sm_ip_at(const sm_t *m, const sint_t k);
sint_t sm_si_at(const sm_t *m, const ss_t *k);
const ss_t *sm_ss_at(const sm_t *m, const ss_t *k);
const void *sm_sp_at(const sm_t *m, const ss_t *k);

/*
 * Existence check
 */

sbool_t sm_u_count(const sm_t *m, const suint32_t k);
sbool_t sm_i_count(const sm_t *m, const sint_t k);
sbool_t sm_s_count(const sm_t *m, const ss_t *k);

/*
 * Insert
 */

sbool_t sm_ii32_insert(sm_t **m, const sint32_t k, const sint32_t v);
sbool_t sm_uu32_insert(sm_t **m, const suint32_t k, const suint32_t v);
sbool_t sm_ii_insert(sm_t **m, const sint_t k, const sint_t v);
sbool_t sm_is_insert(sm_t **m, const sint_t k, const ss_t *v);
sbool_t sm_ip_insert(sm_t **m, const sint_t k, const void *v);
sbool_t sm_si_insert(sm_t **m, const ss_t *k, const sint_t v);
sbool_t sm_ss_insert(sm_t **m, const ss_t *k, const ss_t *v);
sbool_t sm_sp_insert(sm_t **m, const ss_t *k, const void *v);

/*
 * Delete
 */

sbool_t sm_i_delete(sm_t *m, const sint_t k);
sbool_t sm_s_delete(sm_t *m, const ss_t *k);

/*
 * Enumeration / export data
 */

const stn_t *sm_enum(const sm_t *m, const stndx_t i);
ssize_t sm_inorder_enum(const sm_t *m, st_traverse f, void *context);
ssize_t sm_sort_to_vectors(const sm_t *m, sv_t **kv, sv_t **vv);

#ifdef __cplusplus
}; /* extern "C" { */
#endif

#endif /* #ifndef SMAP_H */


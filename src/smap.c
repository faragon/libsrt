/*
 * smap.c
 *
 * Map handling.
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "smap.h"
#include "saux/scommon.h"

/*
 * Internal constants
 */

struct SMapCtx {
	enum eSV_Type sort_kt, sort_vt;
	st_traverse sort_tr;
	srt_tree_callback delete_callback;
	srt_cmp cmpf;
};

/*
 * Internal functions
 */

#define BUILD_SMAP_CMP(FN, T)                                                  \
	static int FN(const T *a, const T *b)                                  \
	{                                                                      \
		return a->k > b->k ? 1 : a->k < b->k ? -1 : 0;                 \
	}

BUILD_SMAP_CMP(cmp_i, struct SMapi)
BUILD_SMAP_CMP(cmp_u, struct SMapu)
BUILD_SMAP_CMP(cmp_I, struct SMapI)
BUILD_SMAP_CMP(cmp_F, struct SMapF)
BUILD_SMAP_CMP(cmp_D, struct SMapD)

static int cmp_s(const struct SMapS *a, const struct SMapS *b)
{
	return ss_cmp(sso_get((const srt_stringo *)&a->k),
		      sso_get((const srt_stringo *)&b->k));
}

#define BUILD_SMAP_RW_INC(FN, T)                                               \
	static void FN(srt_tnode *node, const srt_tnode *new_data,             \
		       srt_bool existing)                                      \
	{                                                                      \
		if (existing)                                                  \
			((T *)node)->v += ((const T *)new_data)->v;            \
		else                                                           \
			((T *)node)->v = ((const T *)new_data)->v;             \
		((T *)node)->x.k = ((const T *)new_data)->x.k;                 \
	}

BUILD_SMAP_RW_INC(rw_inc_SM_II32, struct SMapii)
BUILD_SMAP_RW_INC(rw_inc_SM_UU32, struct SMapuu)
BUILD_SMAP_RW_INC(rw_inc_SM_II, struct SMapII)
BUILD_SMAP_RW_INC(rw_inc_SM_FF, struct SMapFF)
BUILD_SMAP_RW_INC(rw_inc_SM_DD, struct SMapDD)

static void rw_add_SM_S(srt_tnode *node, const srt_tnode *new_data,
			srt_bool existing)
{
	struct SMapS *n = (struct SMapS *)node;
	const struct SMapS *m = (const struct SMapS *)new_data;
	if (!existing)
		sso1_set(&n->k, sso_get((const srt_stringo *)&m->k));
	else
		sso1_update(&n->k, sso_get((const srt_stringo *)&m->k));
}

#define BUILD_SMAP_RW_ADD_SX(FN, T)                                            \
	static void FN(srt_tnode *node, const srt_tnode *new_data,             \
		       srt_bool existing)                                      \
	{                                                                      \
		T *n = (T *)node;                                              \
		const T *m = (const T *)new_data;                              \
		rw_add_SM_S(node, new_data, existing);                         \
		n->v = m->v;                                                   \
	}

BUILD_SMAP_RW_ADD_SX(rw_add_SM_SI, struct SMapSI)
BUILD_SMAP_RW_ADD_SX(rw_add_SM_SD, struct SMapSD)
BUILD_SMAP_RW_ADD_SX(rw_add_SM_SP, struct SMapSP)

static void rw_add_SM_SS(srt_tnode *node, const srt_tnode *new_data,
			 srt_bool existing)
{
	struct SMapSS *n = (struct SMapSS *)node;
	const struct SMapSS *m = (const struct SMapSS *)new_data;
	if (!existing)
		sso_set(&n->s, sso_get(&m->s), sso_get_s2(&m->s));
	else
		sso_update(&n->s, sso_get(&m->s), sso_get_s2(&m->s));
}

#define BUILD_SMAP_RW_ADD_XS(FN, T)                                            \
	static void FN(srt_tnode *node, const srt_tnode *new_data,             \
		       srt_bool existing)                                      \
	{                                                                      \
		T *n = (T *)node;                                              \
		const T *nd = (const T *)new_data;                             \
		n->x.k = nd->x.k;                                              \
		if (!existing)                                                 \
			sso1_set(&n->v, sso1_get(&nd->v));                     \
		else                                                           \
			sso1_update(&n->v, sso1_get(&nd->v));                  \
	}

BUILD_SMAP_RW_ADD_XS(rw_add_SM_IS, struct SMapIS)
BUILD_SMAP_RW_ADD_XS(rw_add_SM_DS, struct SMapDS)

#define BUILD_SMAP_RW_INC_Sx(FN, T, ADDF)                                      \
	static void FN(srt_tnode *node, const srt_tnode *new_data,             \
		       srt_bool existing)                                      \
	{                                                                      \
		if (!existing)                                                 \
			ADDF(node, new_data, existing);                        \
		else                                                           \
			((T *)node)->v += ((const T *)new_data)->v;            \
	}

/* clang-format off */
BUILD_SMAP_RW_INC_Sx(rw_inc_SM_SI, struct SMapSI, rw_add_SM_SI)
BUILD_SMAP_RW_INC_Sx(rw_inc_SM_SD, struct SMapSD, rw_add_SM_SD)
/* clang-format on */

#define BUILD_DELETE_XS(FN, T)                                                 \
	static void FN(void *node)                                             \
	{                                                                      \
		sso1_free(&((T *)node)->v);                                    \
	}

	/* clang-format off */

BUILD_DELETE_XS(aux_is_delete, struct SMapIS)
BUILD_DELETE_XS(aux_ds_delete, struct SMapDS)

static void aux_sx_delete(void *node)
{
	sso1_free(&((struct SMapS *)node)->k);
}

static void aux_ss_delete(void *node)
{
	sso_free(&((struct SMapSS *)node)->s);
}

/* clang-format on */

struct SV2X {
	srt_vector *kv, *vv;
};

#define BUILD_SORT_K(FN, T, PUSHK)                                             \
	static int FN(struct STraverseParams *tp)                              \
	{                                                                      \
		const T *cn = (const T *)get_node_r(tp->t, tp->c);             \
		if (cn) {                                                      \
			struct SV2X *v2x = (struct SV2X *)tp->context;         \
			PUSHK(&v2x->kv, cn->k);                                \
		}                                                              \
		return 0;                                                      \
	}

BUILD_SORT_K(aux_i32_sort, struct SMapi, sv_push_i32)
BUILD_SORT_K(aux_u32_sort, struct SMapu, sv_push_u32)
BUILD_SORT_K(aux_i_sort, struct SMapI, sv_push_i64)
BUILD_SORT_K(aux_f_sort, struct SMapF, sv_push_f)
BUILD_SORT_K(aux_d_sort, struct SMapD, sv_push_d)

static int aux_s_sort(struct STraverseParams *tp)
{
	const struct SMapS *cn = (const struct SMapS *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push(&v2x->kv, sso1_get(&cn->k));
	}
	return 0;
}

#define BUILD_SORT_KV(FN, T, PUSHK, PUSHV)                                     \
	static int FN(struct STraverseParams *tp)                              \
	{                                                                      \
		const T *cn = (const T *)get_node_r(tp->t, tp->c);             \
		if (cn) {                                                      \
			struct SV2X *v2x = (struct SV2X *)tp->context;         \
			PUSHK(&v2x->kv, cn->x.k);                              \
			PUSHV(&v2x->vv, cn->v);                                \
		}                                                              \
		return 0;                                                      \
	}

BUILD_SORT_KV(aux_ii32_sort, struct SMapii, sv_push_i32, sv_push_i32)
BUILD_SORT_KV(aux_uu32_sort, struct SMapuu, sv_push_u32, sv_push_u32)
BUILD_SORT_KV(aux_ii_sort, struct SMapII, sv_push_i64, sv_push_i64)
BUILD_SORT_KV(aux_ff_sort, struct SMapFF, sv_push_f, sv_push_f)
BUILD_SORT_KV(aux_dd_sort, struct SMapDD, sv_push_d, sv_push_d)
BUILD_SORT_KV(aux_ip_sort, struct SMapIP, sv_push_i64, sv_push)
BUILD_SORT_KV(aux_dp_sort, struct SMapDP, sv_push_d, sv_push)

#define BUILD_xS_SORT(FN, T, PUSHK)                                            \
	static int FN(struct STraverseParams *tp)                              \
	{                                                                      \
		const T *cn = (const T *)get_node_r(tp->t, tp->c);             \
		if (cn) {                                                      \
			struct SV2X *v2x = (struct SV2X *)tp->context;         \
			PUSHK(&v2x->kv, cn->x.k);                              \
			sv_push(&v2x->vv, sso1_get(&cn->v));                   \
		}                                                              \
		return 0;                                                      \
	}

BUILD_xS_SORT(aux_is_sort, struct SMapIS, sv_push_i64)
	BUILD_xS_SORT(aux_ds_sort, struct SMapDS, sv_push_d)

#define BUILD_Sx_SORT(FN, T, PUSHV)                                            \
	static int FN(struct STraverseParams *tp)                              \
	{                                                                      \
		const T *cn = (const T *)get_node_r(tp->t, tp->c);             \
		if (cn) {                                                      \
			struct SV2X *v2x = (struct SV2X *)tp->context;         \
			sv_push(&v2x->kv, sso1_get(&cn->x.k));                 \
			PUSHV(&v2x->vv, cn->v);                                \
		}                                                              \
		return 0;                                                      \
	}

	/* clang-format off */

BUILD_Sx_SORT(aux_si_sort, struct SMapSI, sv_push_i64)
BUILD_Sx_SORT(aux_sd_sort, struct SMapSD, sv_push_d)

static int aux_ss_sort(struct STraverseParams *tp)
{
	/* clang-format on */
	const struct SMapSS *cn =
		(const struct SMapSS *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push(&v2x->kv, sso_get(&cn->s));
		sv_push(&v2x->vv, sso_get_s2(&cn->s));
	}
	return 0;
}

static int aux_sp_sort(struct STraverseParams *tp)
{
	const struct SMapSP *cn =
		(const struct SMapSP *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push(&v2x->kv, sso1_get(&cn->x.k));
		sv_push(&v2x->vv, cn->v);
	}
	return 0;
}

const struct SMapCtx sm_ctx[SM0_NumTypes] = {
	{SV_I32, SV_I32, aux_ii32_sort, NULL, (srt_cmp)cmp_i}, /*SM0_II32*/
	{SV_U32, SV_U32, aux_uu32_sort, NULL, (srt_cmp)cmp_u}, /*SM0_UU32*/
	{SV_I64, SV_I64, aux_ii_sort, NULL, (srt_cmp)cmp_I},   /*SM0_II*/
	{SV_I64, SV_GEN, aux_is_sort, aux_is_delete, (srt_cmp)cmp_I}, /*SM0_IS*/
	{SV_I64, SV_GEN, aux_ip_sort, NULL, (srt_cmp)cmp_I},	  /*SM0_IP*/
	{SV_GEN, SV_I64, aux_si_sort, aux_sx_delete, (srt_cmp)cmp_s}, /*SM0_SI*/
	{SV_GEN, SV_GEN, aux_ss_sort, aux_ss_delete, (srt_cmp)cmp_s}, /*SM0_SS*/
	{SV_GEN, SV_GEN, aux_sp_sort, aux_sx_delete, (srt_cmp)cmp_s}, /*SM0_SP*/
	{SV_I64, SV_I64, aux_i_sort, NULL, (srt_cmp)cmp_I},	   /*SM0_I*/
	{SV_I32, SV_I32, aux_i32_sort, NULL, (srt_cmp)cmp_i},	/*SM0_I32*/
	{SV_U32, SV_U32, aux_u32_sort, NULL, (srt_cmp)cmp_u},	/*SM0_U32*/
	{SV_GEN, SV_GEN, aux_s_sort, aux_sx_delete, (srt_cmp)cmp_s}, /*SM0_S*/
	{SV_F, SV_F, aux_f_sort, NULL, (srt_cmp)cmp_F},		     /*SM0_F*/
	{SV_D, SV_D, aux_d_sort, NULL, (srt_cmp)cmp_D},		     /*SM0_D*/
	{SV_F, SV_F, aux_ff_sort, NULL, (srt_cmp)cmp_F},	     /*SM0_FF*/
	{SV_D, SV_D, aux_dd_sort, NULL, (srt_cmp)cmp_D},	     /*SM0_DD*/
	{SV_D, SV_GEN, aux_dp_sort, NULL, (srt_cmp)cmp_D},	   /*SM0_DP*/
	{SV_D, SV_GEN, aux_ds_sort, aux_ds_delete, (srt_cmp)cmp_D},  /*SM0_DS*/
	{SV_GEN, SV_D, aux_sd_sort, aux_sx_delete, (srt_cmp)cmp_s}}; /*SM0_SD*/

S_INLINE srt_cmp type2cmpf(enum eSM_Type0 t)
{
	return t < SM0_NumTypes ? sm_ctx[t].cmpf : NULL;
}

S_INLINE srt_bool sm_chk_t(const srt_map *m, int t)
{
	return m && m->d.sub_type == t ? S_TRUE : S_FALSE;
}

S_INLINE srt_bool sm_chk_sx(const srt_map *m)
{
	return sm_chk_t(m, SM0_SS) || sm_chk_t(m, SM0_SI) || sm_chk_t(m, SM0_SP)
	       || sm_chk_t(m, SM0_S) || sm_chk_t(m, SM0_SD);
}

S_INLINE srt_bool sm_chk_ix(const srt_map *m)
{
	return sm_chk_t(m, SM0_II) || sm_chk_t(m, SM0_I) || sm_chk_t(m, SM0_IS)
	       || sm_chk_t(m, SM0_IP);
}

S_INLINE srt_bool sm_chk_i32x(const srt_map *m)
{
	return sm_chk_t(m, SM0_I32) || sm_chk_t(m, SM0_II32);
}

S_INLINE srt_bool sm_chk_u32x(const srt_map *m)
{
	return sm_chk_t(m, SM0_U32) || sm_chk_t(m, SM0_UU32);
}

S_INLINE srt_bool sm_chk_fx(const srt_map *m)
{
	return sm_chk_t(m, SM0_F) || sm_chk_t(m, SM0_FF);
}

S_INLINE srt_bool sm_chk_dx(const srt_map *m)
{
	return sm_chk_t(m, SM0_D) || sm_chk_t(m, SM0_DD) || sm_chk_t(m, SM0_DS)
	       || sm_chk_t(m, SM0_DP);
}

SM_ENUM_INORDER_XX(sm_itr_ii32, srt_map_it_ii32, SM_II32, int32_t,
		   cmp_ni_i((const struct SMapi *)cn, kmin),
		   cmp_ni_i((const struct SMapi *)cn, kmax),
		   f(((const struct SMapi *)cn)->k,
		     ((const struct SMapii *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_uu32, srt_map_it_uu32, SM_UU32, uint32_t,
		   cmp_nu_u((const struct SMapu *)cn, kmin),
		   cmp_nu_u((const struct SMapu *)cn, kmax),
		   f(((const struct SMapu *)cn)->k,
		     ((const struct SMapuu *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_ii, srt_map_it_ii, SM_II, int64_t,
		   cmp_nI_I((const struct SMapI *)cn, kmin),
		   cmp_nI_I((const struct SMapI *)cn, kmax),
		   f(((const struct SMapI *)cn)->k,
		     ((const struct SMapII *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_ff, srt_map_it_ff, SM_FF, float,
		   cmp_nF_F((const struct SMapF *)cn, kmin),
		   cmp_nF_F((const struct SMapF *)cn, kmax),
		   f(((const struct SMapF *)cn)->k,
		     ((const struct SMapFF *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_dd, srt_map_it_dd, SM_DD, double,
		   cmp_nD_D((const struct SMapD *)cn, kmin),
		   cmp_nD_D((const struct SMapD *)cn, kmax),
		   f(((const struct SMapD *)cn)->k,
		     ((const struct SMapDD *)cn)->v, context))

SM_ENUM_INORDER_XX(
	sm_itr_is, srt_map_it_is, SM_IS, int64_t,
	cmp_nI_I((const struct SMapI *)cn, kmin),
	cmp_nI_I((const struct SMapI *)cn, kmax),
	f(((const struct SMapI *)cn)->k,
	  sso_get((const srt_stringo *)&((const struct SMapIS *)cn)->v),
	  context))

SM_ENUM_INORDER_XX(
	sm_itr_ds, srt_map_it_ds, SM_DS, double,
	cmp_nD_D((const struct SMapD *)cn, kmin),
	cmp_nD_D((const struct SMapD *)cn, kmax),
	f(((const struct SMapD *)cn)->k,
	  sso_get((const srt_stringo *)&((const struct SMapDS *)cn)->v),
	  context))

SM_ENUM_INORDER_XX(sm_itr_ip, srt_map_it_ip, SM_IP, int64_t,
		   cmp_nI_I((const struct SMapI *)cn, kmin),
		   cmp_nI_I((const struct SMapI *)cn, kmax),
		   f(((const struct SMapI *)cn)->k,
		     ((const struct SMapIP *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_dp, srt_map_it_dp, SM_DP, double,
		   cmp_nD_D((const struct SMapD *)cn, kmin),
		   cmp_nD_D((const struct SMapD *)cn, kmax),
		   f(((const struct SMapD *)cn)->k,
		     ((const struct SMapDP *)cn)->v, context))

SM_ENUM_INORDER_XX(
	sm_itr_si, srt_map_it_si, SM_SI, const srt_string *,
	cmp_ns_s((const struct SMapS *)cn, kmin),
	cmp_ns_s((const struct SMapS *)cn, kmax),
	f(sso_get((const srt_stringo *)&((const struct SMapS *)cn)->k),
	  ((const struct SMapSI *)cn)->v, context))

SM_ENUM_INORDER_XX(
	sm_itr_sd, srt_map_it_sd, SM_SD, const srt_string *,
	cmp_ns_s((const struct SMapS *)cn, kmin),
	cmp_ns_s((const struct SMapS *)cn, kmax),
	f(sso_get((const srt_stringo *)&((const struct SMapS *)cn)->k),
	  ((const struct SMapSD *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_ss, srt_map_it_ss, SM_SS, const srt_string *,
		   cmp_ns_s((const struct SMapS *)cn, kmin),
		   cmp_ns_s((const struct SMapS *)cn, kmax),
		   f(sso_get(&((const struct SMapSS *)cn)->s),
		     sso_get_s2(&((const struct SMapSS *)cn)->s), context))

SM_ENUM_INORDER_XX(
	sm_itr_sp, srt_map_it_sp, SM_SP, const srt_string *,
	cmp_ns_s((const struct SMapS *)cn, kmin),
	cmp_ns_s((const struct SMapS *)cn, kmax),
	f(sso_get((const srt_stringo *)&((const struct SMapS *)cn)->k),
	  ((const struct SMapSP *)cn)->v, context))

/*
 * Allocation
 */

srt_map *sm_alloc_raw0(enum eSM_Type0 t, srt_bool ext_buf, void *buffer,
		       size_t elem_size, size_t max_size)
{
	srt_map *m;
	RETURN_IF(!buffer || !max_size, NULL);
	m = (srt_map *)st_alloc_raw(type2cmpf(t), ext_buf, buffer, elem_size,
				    max_size);
	if (m)
		m->d.sub_type = (uint8_t)t;
	return m;
}

srt_map *sm_alloc0(enum eSM_Type0 t, size_t init_size)
{
	srt_map *m = (srt_map *)st_alloc(type2cmpf(t), sm_elem_size((int)t),
					 init_size);
	if (m)
		m->d.sub_type = (uint8_t)t;
	return m;
}

void sm_free_aux(srt_map **m, ...)
{
	va_list ap;
	srt_map **next;
	va_start(ap, m);
	next = m;
	while (!s_varg_tail_ptr_tag(next)) { /* last element tag */
		if (next) {
			sm_clear(*next); /* release associated dyn. memory */
			sd_free((srt_data **)next);
		}
		next = (srt_map **)va_arg(ap, srt_map **);
	}
	va_end(ap);
}

srt_map *sm_dup(const srt_map *src)
{
	srt_map *m = NULL;
	return sm_cpy(&m, src);
}

void sm_clear(srt_map *m)
{
	int t;
	if (!m || !m->d.size)
		return;
	t = m->d.sub_type;
	srt_tree_callback delete_callback =
		t < SM0_NumTypes ? sm_ctx[t].delete_callback : NULL;
	if (delete_callback) { /* deletion of dynamic memory elems */
		srt_tndx i = 0;
		for (; i < (srt_tndx)m->d.size; i++) {
			srt_tnode *n = st_enum(m, i);
			delete_callback(n);
		}
	}
	st_set_size((srt_tree *)m, 0);
}

/*
 * Copy
 */

srt_map *sm_cpy(srt_map **m, const srt_map *src)
{
	srt_tndx i;
	enum eSM_Type0 t;
	size_t ss, src_buf_size;
	RETURN_IF(!m || !src, NULL); /* BEHAVIOR */
	t = (enum eSM_Type0)src->d.sub_type;
	ss = sm_size(src);
	src_buf_size = src->d.elem_size * src->d.size;
	RETURN_IF(ss > ST_NDX_MAX, NULL); /* BEHAVIOR */
	if (*m) {
		sm_clear(*m);
		if (!sm_chk_t(*m, (int)t)) {
			/*
			 * Case of changing map type, reusing allocated memory,
			 * but changing container configuration.
			 */
			size_t raw_size = (*m)->d.elem_size * (*m)->d.max_size,
			       new_max_size = raw_size / src->d.elem_size;
			(*m)->d.elem_size = src->d.elem_size;
			(*m)->d.max_size = new_max_size;
			(*m)->cmp_f = src->cmp_f;
			(*m)->d.sub_type = src->d.sub_type;
		}
		sm_reserve(m, ss);
	} else {
		*m = sm_alloc0(t, ss);
		RETURN_IF(!*m, NULL); /* BEHAVIOR: allocation error */
	}
	RETURN_IF(sm_max_size(*m) < ss, *m); /* BEHAVIOR: not enough space */
	/*
	 * Bulk tree copy: tree structure can be copied as is, because of
	 * of using indexes instead of pointers.
	 */
	memcpy(sm_get_buffer(*m), sm_get_buffer_r(src), src_buf_size);
	sm_set_size(*m, ss);
	(*m)->root = src->root;
	/*
	 * Copy elements using external dynamic memory (string data)
	 */
	switch (t) {
	case SM0_IS:
		for (i = 0; i < ss; i++) {
			const struct SMapIS *ms =
				(const struct SMapIS *)st_enum_r(src, i);
			struct SMapIS *mt = (struct SMapIS *)st_enum(*m, i);
			sso1_set(&mt->v, sso1_get(&ms->v));
		}
		break;
	case SM0_DS:
		for (i = 0; i < ss; i++) {
			const struct SMapDS *ms =
				(const struct SMapDS *)st_enum_r(src, i);
			struct SMapDS *mt = (struct SMapDS *)st_enum(*m, i);
			sso1_set(&mt->v, sso1_get(&ms->v));
		}
		break;
	case SM0_S:
	case SM0_SI:
	case SM0_SD:
	case SM0_SP:
		for (i = 0; i < ss; i++) {
			const struct SMapS *ms =
				(const struct SMapS *)st_enum_r(src, i);
			struct SMapS *mt = (struct SMapS *)st_enum(*m, i);
			sso1_set(&mt->k, sso1_get(&ms->k));
		}
		break;
	case SM0_SS:
		for (i = 0; i < ss; i++) {
			const struct SMapSS *ms =
				(const struct SMapSS *)st_enum_r(src, i);
			struct SMapSS *mt = (struct SMapSS *)st_enum(*m, i);
			sso_set(&mt->s, sso_get(&ms->s), sso_get_s2(&ms->s));
		}
		break;
	default:
		/* no additional action required */
		break;
	}
	return *m;
}

	/*
	 * Random access
	 */

#define BUILD_SM_AT(FN, ID, TS, TK, TV)                                        \
	TV FN(const srt_map *m, TK k)                                          \
	{                                                                      \
		TS n;                                                          \
		const TS *nr;                                                  \
		RETURN_IF(!sm_chk_t(m, ID), 0);                                \
		n.x.k = k;                                                     \
		nr = (const TS *)st_locate(m, (const srt_tnode *)&n);          \
		return nr ? nr->v : 0; /* BEHAVIOR */                          \
	}

BUILD_SM_AT(sm_at_ii32, SM_II32, struct SMapii, int32_t, int32_t)
BUILD_SM_AT(sm_at_uu32, SM_UU32, struct SMapuu, uint32_t, uint32_t)
BUILD_SM_AT(sm_at_ii, SM_II, struct SMapII, int64_t, int64_t)
BUILD_SM_AT(sm_at_ff, SM_FF, struct SMapFF, float, float)
BUILD_SM_AT(sm_at_dd, SM_DD, struct SMapDD, double, double)
BUILD_SM_AT(sm_at_ip, SM_IP, struct SMapIP, int64_t, const void *)
BUILD_SM_AT(sm_at_dp, SM_DP, struct SMapDP, double, const void *)

#define BUILD_SM_AT_XS(FN, ID, TS, TK)                                         \
	const srt_string *FN(const srt_map *m, TK k)                           \
	{                                                                      \
		TS n;                                                          \
		const TS *nr;                                                  \
		RETURN_IF(!sm_chk_t(m, ID), ss_void);                          \
		n.x.k = k;                                                     \
		nr = (const TS *)st_locate(m, (const srt_tnode *)&n);          \
		return nr ? sso_get((const srt_stringo *)&nr->v) : ss_void;    \
	}

BUILD_SM_AT_XS(sm_at_is, SM_IS, struct SMapIS, int64_t)
BUILD_SM_AT_XS(sm_at_ds, SM_DS, struct SMapDS, double)

#define BUILD_SM_AT_SX(FN, ID, TS, TV, DEF_VAL)                                \
	TV FN(const srt_map *m, const srt_string *k)                           \
	{                                                                      \
		TS n;                                                          \
		const TS *nr;                                                  \
		RETURN_IF(!sm_chk_t(m, ID), DEF_VAL);                          \
		sso1_setref(&n.x.k, k);                                        \
		nr = (const TS *)st_locate(m, &n.x.n);                         \
		return nr ? nr->v : DEF_VAL; /* BEHAVIOR */                    \
	}

BUILD_SM_AT_SX(sm_at_si, SM_SI, struct SMapSI, int64_t, 0)
BUILD_SM_AT_SX(sm_at_sd, SM_SD, struct SMapSD, double, 0)
BUILD_SM_AT_SX(sm_at_sp, SM_SP, struct SMapSP, const void *, NULL)

const srt_string *sm_at_ss(const srt_map *m, const srt_string *k)
{
	struct SMapSS n;
	const struct SMapSS *nr;
	RETURN_IF(!sm_chk_t(m, SM_SS), ss_void);
	sso_setref(&n.s, k, NULL);
	nr = (const struct SMapSS *)st_locate(m, &n.n);
	return nr ? sso_get_s2(&nr->s) : ss_void;
}

	/*
	 * Existence check
	 */

#define BUILD_SM_COUNT(FN, CHK, TS, TK)                                        \
	size_t FN(const srt_map *m, TK k)                                      \
	{                                                                      \
		TS n;                                                          \
		RETURN_IF(!(CHK), S_FALSE);                                    \
		n.k = k;                                                       \
		return st_locate(m, (const srt_tnode *)&n) ? 1 : 0;            \
	}

BUILD_SM_COUNT(sm_count_i32, sm_chk_i32x(m), struct SMapi, int32_t)
BUILD_SM_COUNT(sm_count_u32, sm_chk_u32x(m), struct SMapu, uint32_t)
BUILD_SM_COUNT(sm_count_i, sm_chk_ix(m), struct SMapI, int64_t)
BUILD_SM_COUNT(sm_count_f, sm_chk_fx(m), struct SMapF, float)
BUILD_SM_COUNT(sm_count_d, sm_chk_dx(m), struct SMapD, double)

size_t sm_count_s(const srt_map *m, const srt_string *k)
{
	struct SMapS n;
	RETURN_IF(!sm_chk_sx(m), S_FALSE);
	sso1_setref(&n.k, k);
	return st_locate(m, (const srt_tnode *)&n) ? 1 : 0;
}

/*
 * Insert
 */

S_INLINE srt_bool sm_insert_ii32_aux(srt_map **m, int32_t k, int32_t v,
				     srt_tree_rewrite rw_f)
{
	struct SMapii n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_II32), S_FALSE);
	n.x.k = k;
	n.v = v;
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n, rw_f);
}

srt_bool sm_insert_ii32(srt_map **m, int32_t k, int32_t v)
{
	return sm_insert_ii32_aux(m, k, v, NULL);
}

srt_bool sm_inc_ii32(srt_map **m, int32_t k, int32_t v)
{
	return sm_insert_ii32_aux(m, k, v, rw_inc_SM_II32);
}

S_INLINE srt_bool sm_insert_uu32_aux(srt_map **m, uint32_t k, uint32_t v,
				     srt_tree_rewrite rw_f)
{
	struct SMapuu n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_UU32), S_FALSE);
	n.x.k = k;
	n.v = v;
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n, rw_f);
}

srt_bool sm_insert_uu32(srt_map **m, uint32_t k, uint32_t v)
{
	return sm_insert_uu32_aux(m, k, v, NULL);
}

srt_bool sm_inc_uu32(srt_map **m, uint32_t k, uint32_t v)
{
	return sm_insert_uu32_aux(m, k, v, rw_inc_SM_UU32);
}

S_INLINE srt_bool sm_insert_ii_aux(srt_map **m, int64_t k, int64_t v,
				   srt_tree_rewrite rw_f)
{
	struct SMapII n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_II), S_FALSE);
	n.x.k = k;
	n.v = v;
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n, rw_f);
}

srt_bool sm_insert_ii(srt_map **m, int64_t k, int64_t v)
{
	return sm_insert_ii_aux(m, k, v, NULL);
}

srt_bool sm_inc_ii(srt_map **m, int64_t k, int64_t v)
{
	return sm_insert_ii_aux(m, k, v, rw_inc_SM_II);
}

S_INLINE srt_bool sm_insert_ff_aux(srt_map **m, float k, float v,
				   srt_tree_rewrite rw_f)
{
	struct SMapFF n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_FF), S_FALSE);
	n.x.k = k;
	n.v = v;
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n, rw_f);
}

srt_bool sm_insert_ff(srt_map **m, float k, float v)
{
	return sm_insert_ff_aux(m, k, v, NULL);
}

srt_bool sm_inc_ff(srt_map **m, float k, float v)
{
	return sm_insert_ff_aux(m, k, v, rw_inc_SM_FF);
}

S_INLINE srt_bool sm_insert_dd_aux(srt_map **m, double k, double v,
				   srt_tree_rewrite rw_f)
{
	struct SMapDD n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_DD), S_FALSE);
	n.x.k = k;
	n.v = v;
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n, rw_f);
}

srt_bool sm_insert_dd(srt_map **m, double k, double v)
{
	return sm_insert_dd_aux(m, k, v, NULL);
}

srt_bool sm_inc_dd(srt_map **m, double k, double v)
{
	return sm_insert_dd_aux(m, k, v, rw_inc_SM_DD);
}

srt_bool sm_insert_is(srt_map **m, int64_t k, const srt_string *v)
{
	struct SMapIS n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_IS), S_FALSE);
	n.x.k = k;
	sso1_setref(&n.v, v);
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n,
			    rw_add_SM_IS);
}

srt_bool sm_insert_ds(srt_map **m, double k, const srt_string *v)
{
	struct SMapDS n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_DS), S_FALSE);
	n.x.k = k;
	sso1_setref(&n.v, v);
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n,
			    rw_add_SM_DS);
}

srt_bool sm_insert_ip(srt_map **m, int64_t k, const void *v)
{
	struct SMapIP n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_IP), S_FALSE);
	n.x.k = k;
	n.v = v;
	return st_insert((srt_tree **)m, (const srt_tnode *)&n);
}

srt_bool sm_insert_dp(srt_map **m, double k, const void *v)
{
	struct SMapDP n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_DP), S_FALSE);
	n.x.k = k;
	n.v = v;
	return st_insert((srt_tree **)m, (const srt_tnode *)&n);
}

S_INLINE srt_bool sm_insert_si_aux(srt_map **m, const srt_string *k, int64_t v,
				   srt_tree_rewrite rw_f)
{
	srt_bool r;
	struct SMapSI n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_SI), S_FALSE);
	sso1_setref(&n.x.k, k);
	n.v = v;
	r = st_insert_rw((srt_tree **)m, (const srt_tnode *)&n, rw_f);
	return r;
}

srt_bool sm_insert_si(srt_map **m, const srt_string *k, int64_t v)
{
	return sm_insert_si_aux(m, k, v, rw_add_SM_SI);
}

srt_bool sm_inc_si(srt_map **m, const srt_string *k, int64_t v)
{
	return sm_insert_si_aux(m, k, v, rw_inc_SM_SI);
}

S_INLINE srt_bool sm_insert_sd_aux(srt_map **m, const srt_string *k, double v,
				   srt_tree_rewrite rw_f)
{
	srt_bool r;
	struct SMapSD n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_SD), S_FALSE);
	sso1_setref(&n.x.k, k);
	n.v = v;
	r = st_insert_rw((srt_tree **)m, (const srt_tnode *)&n, rw_f);
	return r;
}

srt_bool sm_insert_sd(srt_map **m, const srt_string *k, double v)
{
	return sm_insert_sd_aux(m, k, v, rw_add_SM_SD);
}

srt_bool sm_inc_sd(srt_map **m, const srt_string *k, double v)
{
	return sm_insert_sd_aux(m, k, v, rw_inc_SM_SD);
}

srt_bool sm_insert_ss(srt_map **m, const srt_string *k, const srt_string *v)
{
	struct SMapSS n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_SS), S_FALSE);
	sso_setref(&n.s, k, v);
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n,
			    rw_add_SM_SS);
}

srt_bool sm_insert_sp(srt_map **m, const srt_string *k, const void *v)
{
	struct SMapSP n;
	RETURN_IF(!m || !sm_chk_t(*m, SM0_SP), S_FALSE);
	sso1_setref(&n.x.k, k);
	n.v = v;
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n,
			    rw_add_SM_SP);
}

/*
 * Delete
 */

srt_bool sm_delete_i32(srt_map *m, int32_t k)
{
	struct SMapi n;
	RETURN_IF(!sm_chk_i32x(m), S_FALSE);
	n.k = k;
	return st_delete(m, (const srt_tnode *)&n, NULL);
}

srt_bool sm_delete_u32(srt_map *m, uint32_t k)
{
	struct SMapu n;
	RETURN_IF(!sm_chk_u32x(m), S_FALSE);
	n.k = k;
	return st_delete(m, (const srt_tnode *)&n, NULL);
}

srt_bool sm_delete_i(srt_map *m, int64_t k)
{
	struct SMapI n;
	RETURN_IF(!sm_chk_ix(m), S_FALSE);
	n.k = k;
	return st_delete(m, (const srt_tnode *)&n,
			 sm_chk_t(m, SM0_IS) ? aux_is_delete : NULL);
}

srt_bool sm_delete_f(srt_map *m, float k)
{
	struct SMapF n;
	RETURN_IF(!sm_chk_fx(m), S_FALSE);
	n.k = k;
	return st_delete(m, (const srt_tnode *)&n, NULL);
}

srt_bool sm_delete_d(srt_map *m, double k)
{
	struct SMapD n;
	RETURN_IF(!sm_chk_dx(m), S_FALSE);
	n.k = k;
	return st_delete(m, (const srt_tnode *)&n,
			 sm_chk_t(m, SM0_DS) ? aux_ds_delete : NULL);
}

srt_bool sm_delete_s(srt_map *m, const srt_string *k)
{
	srt_tree_callback callback = NULL;
	struct SMapS sx;
	RETURN_IF(!m, S_FALSE);
	sso1_setref(&sx.k, k);
	if (sm_chk_t(m, SM0_SS))
		callback = aux_ss_delete;
	else if (sm_chk_sx(m))
		callback = aux_sx_delete;
	else
		return S_FALSE;
	return st_delete(m, (const srt_tnode *)&sx, callback);
}

/*
 * Enumeration / export data
 */

ssize_t sm_sort_to_vectors(const srt_map *m, srt_vector **kv, srt_vector **vv)
{
	ssize_t r;
	struct SV2X v2x;
	uint8_t t;
	enum eSV_Type kt, vt;
	st_traverse traverse_f = NULL;
	RETURN_IF(!m || !kv || !vv || m->d.sub_type >= SM0_NumTypes, 0);
	v2x.kv = *kv;
	v2x.vv = *vv;
	t = m->d.sub_type;
	kt = sm_ctx[t].sort_kt;
	vt = sm_ctx[t].sort_vt;
	traverse_f = sm_ctx[t].sort_tr;
	if (v2x.kv) {
		if (v2x.kv->d.sub_type != kt)
			sv_free(&v2x.kv);
		else
			sv_reserve(&v2x.kv, m->d.size);
	}
	if (v2x.vv) {
		if (v2x.vv->d.sub_type != vt)
			sv_free(&v2x.vv);
		else
			sv_reserve(&v2x.vv, m->d.size);
	}
	if (!v2x.kv)
		v2x.kv = sv_alloc_t(kt, m->d.size);
	if (!v2x.vv)
		v2x.vv = sv_alloc_t(vt, m->d.size);
	r = st_traverse_inorder((const srt_tree *)m, traverse_f, (void *)&v2x);
	*kv = v2x.kv;
	*vv = v2x.vv;
	return r;
}

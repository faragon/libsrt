/*
 * smap.c
 *
 * Map handling.
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */ 

#include "smap.h"
#include "saux/scommon.h"

/*
 * Internal functions
 */

static int cmp_i(const struct SMapi *a, const struct SMapi *b)
{
	return a->k > b->k ? 1 : a->k < b->k ? -1 : 0;
}

static int cmp_u(const struct SMapu *a, const struct SMapu *b)
{
	return a->k > b->k ? 1 : a->k < b->k ? -1 : 0;
}

static int cmp_I(const struct SMapI *a, const struct SMapI *b)
{
	return a->k > b->k ? 1 : a->k < b->k ? -1 : 0;
}

static int cmp_s(const struct SMapS *a, const struct SMapS *b)
{
	return ss_cmp(SMStrGet(&a->k), SMStrGet(&b->k));
}

static void rw_inc_SM_II32(stn_t *node, const stn_t *new_data,
			   const sbool_t existing)
{
	if (existing)
		((struct SMapii *)node)->v += ((const struct SMapii *)new_data)->v;
}

static void rw_inc_SM_UU32(stn_t *node, const stn_t *new_data,
			   const sbool_t existing)
{
	if (existing)
		((struct SMapuu *)node)->v += ((const struct SMapuu *)new_data)->v;
}

static void rw_inc_SM_II(stn_t *node, const stn_t *new_data,
			 const sbool_t existing)
{
	if (existing)
		((struct SMapII *)node)->v += ((const struct SMapII *)new_data)->v;
}

static void rw_add_SM_SP(stn_t *node, const stn_t *new_data,
			 const sbool_t existing)
{
	struct SMapSP *n = (struct SMapSP *)node;
	const struct SMapSP *m = (const struct SMapSP *)new_data;
	if (!existing)
		SMStrSet(&n->x.k, SMStrGet(&m->x.k));
	else
		SMStrUpdate(&n->x.k, SMStrGet(&m->x.k));
}

static void rw_add_SM_SS(stn_t *node, const stn_t *new_data,
			 const sbool_t existing)
{
	struct SMapSS *n = (struct SMapSS *)node;
	const struct SMapSS *m = (const struct SMapSS *)new_data;
	if (!existing) {
		SMStrSet(&n->x.k, SMStrGet(&m->x.k));
		SMStrSet(&n->v, SMStrGet(&m->v));
	} else {
		SMStrUpdate(&n->x.k, SMStrGet(&m->x.k));
		SMStrUpdate(&n->v, SMStrGet(&m->v));
	}
}

static void rw_add_SM_SI(stn_t *node, const stn_t *new_data,
			 const sbool_t existing)
{
	struct SMapSI *n = (struct SMapSI *)node;
	const struct SMapSI *m = (const struct SMapSI *)new_data;
	if (!existing)
		SMStrSet(&n->x.k, SMStrGet(&m->x.k));
	else
		SMStrUpdate(&n->x.k, SMStrGet(&m->x.k));
}

static void rw_inc_SM_SI(stn_t *node, const stn_t *new_data,
			 const sbool_t existing)
{
	if (!existing)
		rw_add_SM_SI(node, new_data, existing);
	else
		((struct SMapSI *)node)->v += ((const struct SMapSI *)new_data)->v;
}

static void aux_is_delete(void *node)
{
	SMStrFree(&((struct SMapIS *)node)->v);
}

static void aux_sx_delete(void *node)
{
	SMStrFree(&((struct SMapS *)node)->k);
}

static void aux_ss_delete(void *node)
{
	SMStrFree(&((struct SMapSS *)node)->x.k);
	SMStrFree(&((struct SMapSS *)node)->v);
}

struct SV2X { sv_t *kv, *vv; };

static int aux_ii32_sort(struct STraverseParams *tp)
{
	const struct SMapii *cn = (const struct SMapii *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push_i(&v2x->kv, cn->x.k);
		sv_push_i(&v2x->vv, cn->v);
	}
	return 0;
}

static int aux_uu32_sort(struct STraverseParams *tp)
{
	const struct SMapuu *cn = (const struct SMapuu *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push_u(&v2x->kv, cn->x.k);
		sv_push_u(&v2x->vv, cn->v);
	}
	return 0;
}
static int aux_ii_sort(struct STraverseParams *tp)
{
	const struct SMapII *cn = (const struct SMapII *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push_i(&v2x->kv, cn->x.k);
		sv_push_i(&v2x->vv, cn->v);
	}
	return 0;
}

static int aux_is_ip_sort(struct STraverseParams *tp)
{
	const struct SMapIP *cn = (const struct SMapIP *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push_i(&v2x->kv, cn->x.k);
		sv_push(&v2x->vv, &cn->v);
	}
	return 0;
}

static int aux_si_sort(struct STraverseParams *tp)
{
	const struct SMapII *cn = (const struct SMapII *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push(&v2x->kv, &cn->x.k);
		sv_push_i(&v2x->vv, cn->v);
	}
	return 0;
}

static int aux_sp_ss_sort(struct STraverseParams *tp)
{
	const struct SMapII *cn = (const struct SMapII *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push(&v2x->kv, &cn->x.k);
		sv_push(&v2x->vv, &cn->v);
	}
	return 0;
}

static st_cmp_t type2cmpf(const enum eSM_Type0 t)
{
	switch (t) {
	case SM0_U32: case SM0_UU32:
		return (st_cmp_t)cmp_u;
	case SM0_I32: case SM0_II32:
		return (st_cmp_t)cmp_i;
	case SM0_I: case SM0_II: case SM0_IS: case SM0_IP:
		return (st_cmp_t)cmp_I;
	case SM0_S: case SM0_SI: case SM0_SS: case SM0_SP:
		return (st_cmp_t)cmp_s;
	}
	return NULL;
}

S_INLINE sbool_t sm_chk_Ix(const sm_t *m)
{
	RETURN_IF(!m, S_FALSE);
	int t = m->d.sub_type;
	return t == SM0_II || t == SM0_IS || t == SM0_IP || t == SM0_I ?
	       S_TRUE : S_FALSE;
}

S_INLINE sbool_t sm_chk_sx(const sm_t *m)
{
	RETURN_IF(!m, S_FALSE);
	int t = m->d.sub_type;
	return t == SM0_SS || t == SM0_SI || t == SM0_SP || t == SM0_S ?
	       S_TRUE : S_FALSE;
}

S_INLINE sbool_t sm_chk_t(const sm_t *m, int t)
{
	return m && m->d.sub_type == t ? S_TRUE : S_FALSE;
}

S_INLINE sbool_t sm_i32_range(const int64_t k)
{
	return k >= SINT32_MIN && k <= SINT32_MAX ? S_TRUE : S_FALSE;
}

#ifdef _MSC_VER /* supress alloca() warning */
#pragma warning(disable: 6255)
#endif

SM_ENUM_INORDER_XX(sm_itr_ii32, sm_it_ii32_t, SM_II32, int32_t,
		   cmp_ni_i((const struct SMapi *)cn, kmin),
		   cmp_ni_i((const struct SMapi *)cn, kmax),
		   f(((const struct SMapi *)cn)->k,
		     ((const struct SMapii *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_uu32, sm_it_uu32_t, SM_UU32, uint32_t,
		   cmp_nu_u((const struct SMapu *)cn, kmin),
		   cmp_nu_u((const struct SMapu *)cn, kmax),
		   f(((const struct SMapu *)cn)->k,
		     ((const struct SMapuu *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_ii, sm_it_ii_t, SM_II, int64_t,
		   cmp_nI_I((const struct SMapI *)cn, kmin),
		   cmp_nI_I((const struct SMapI *)cn, kmax),
		   f(((const struct SMapI *)cn)->k,
		     ((const struct SMapII *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_is, sm_it_is_t, SM_IS, int64_t,
		   cmp_nI_I((const struct SMapI *)cn, kmin),
		   cmp_nI_I((const struct SMapI *)cn, kmax),
		   f(((const struct SMapI *)cn)->k,
		     SMStrGet(&((const struct SMapIS *)cn)->v), context))

SM_ENUM_INORDER_XX(sm_itr_ip, sm_it_ip_t, SM_IP, int64_t,
		   cmp_nI_I((const struct SMapI *)cn, kmin),
		   cmp_nI_I((const struct SMapI *)cn, kmax),
		   f(((const struct SMapI *)cn)->k,
		     ((const struct SMapIP *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_si, sm_it_si_t, SM_SI, const ss_t *,
		   cmp_ns_s((const struct SMapS *)cn, kmin),
		   cmp_ns_s((const struct SMapS *)cn, kmax),
		   f(SMStrGet(&((const struct SMapS *)cn)->k),
		     ((const struct SMapSI *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_ss, sm_it_ss_t, SM_SS, const ss_t *,
		   cmp_ns_s((const struct SMapS *)cn, kmin),
		   cmp_ns_s((const struct SMapS *)cn, kmax),
		   f(SMStrGet(&((const struct SMapS *)cn)->k),
		     SMStrGet(&((const struct SMapSS *)cn)->v), context))

SM_ENUM_INORDER_XX(sm_itr_sp, sm_it_sp_t, SM_SP, const ss_t *,
		   cmp_ns_s((const struct SMapS *)cn, kmin),
		   cmp_ns_s((const struct SMapS *)cn, kmax),
		   f(SMStrGet(&((const struct SMapS *)cn)->k),
		     ((const struct SMapSP *)cn)->v, context))

/*
 * Allocation
 */

sm_t *sm_alloc_raw0(const enum eSM_Type0 t, const sbool_t ext_buf, void *buffer,
		   const size_t elem_size, const size_t max_size)
{
	RETURN_IF(!buffer || !max_size, NULL);
	sm_t *m = (sm_t *)st_alloc_raw(type2cmpf(t), ext_buf, buffer, elem_size,
				       max_size);
	m->d.sub_type = t;
	return m;
}

sm_t *sm_alloc0(const enum eSM_Type0 t, const size_t init_size)
{
	sm_t *m = (sm_t *)st_alloc(type2cmpf(t), sm_elem_size(t), init_size);
	m->d.sub_type = t;
	return m;
}

void sm_free_aux(sm_t **m, ...)
{
	va_list ap;
	va_start(ap, m);
	sm_t **next = m;
	while (!s_varg_tail_ptr_tag(next)) { /* last element tag */
		if (next) {
			sm_clear(*next); /* release associated dynamic memory */
			sd_free((sd_t **)next);
		}
		next = (sm_t **)va_arg(ap, sm_t **);
	}
	va_end(ap);
}

sm_t *sm_dup(const sm_t *src)
{
	sm_t *m = NULL;
	return sm_cpy(&m, src);
}

void sm_clear(sm_t *m)
{
	if (!m || !m->d.size)
		return;
	stn_callback_t delete_callback = NULL;
	switch (m->d.sub_type) {
	case SM0_IS:
		delete_callback = aux_is_delete;
		break;
	case SM0_SS:
		delete_callback = aux_ss_delete;
		break;
	case SM0_S: case SM0_SI: case SM0_SP:
		delete_callback = aux_sx_delete;
		break;
	}
	if (delete_callback) {	/* deletion of dynamic memory elems */
		stndx_t i = 0;
		for (; i < (stndx_t)m->d.size; i++) {
			stn_t *n = st_enum(m, i);
			delete_callback(n);
		}
	}
	st_set_size((st_t *)m, 0);
}

/*
 * Copy
 */

sm_t *sm_cpy(sm_t **m, const sm_t *src)
{
	RETURN_IF(!m || !src, NULL); /* BEHAVIOR */
	const enum eSM_Type0 t = (enum eSM_Type0)src->d.sub_type;
	size_t ss = sm_size(src),
	       src_buf_size = src->d.elem_size * src->d.size;
	RETURN_IF(ss > ST_NDX_MAX, NULL); /* BEHAVIOR */
	if (*m) {
		sm_clear(*m);
		if (!sm_chk_t(*m, t)) {
			/*
			 * Case of changing map type, reusing allocated memory,
			 * but changing container configuration.
			 */
			size_t raw_space = (*m)->d.elem_size * (*m)->d.max_size,
			       new_max_size = raw_space / src->d.elem_size;
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
	stndx_t i;
	switch (t) {
	case SM0_IS:
		for (i = 0; i < ss; i++) {
			const struct SMapIS *ms = (const struct SMapIS *)st_enum_r(src, i);
			struct SMapIS *mt = (struct SMapIS *)st_enum(*m, i);
			SMStrSet(&mt->v, SMStrGet(&ms->v));
		}
		break;
	case SM0_S: case SM0_SI: case SM0_SP:
		for (i = 0; i < ss; i++) {
			const struct SMapS *ms = (const struct SMapS *)st_enum_r(src, i);
			struct SMapS *mt = (struct SMapS *)st_enum(*m, i);
			SMStrSet(&mt->k, SMStrGet(&ms->k));
		}
		break;
	case SM0_SS:
		for (i = 0; i < ss; i++) {
			const struct SMapSS *ms = (const struct SMapSS *)st_enum_r(src, i);
			struct SMapSS *mt = (struct SMapSS *)st_enum(*m, i);
			SMStrSet(&mt->x.k, SMStrGet(&ms->x.k));
			SMStrSet(&mt->v, SMStrGet(&ms->v));
		}
		break;
	case SM0_II32: case SM0_UU32: case SM0_II: case SM0_IP:
	case SM0_I: case SM0_I32: case SM0_U32:
		/* no additional action required */
		break;
	}
	return *m;
}

/*
 * Random access
 */

int32_t sm_at_ii32(const sm_t *m, const int32_t k)
{
	RETURN_IF(!sm_chk_t(m, SM_II32), 0);
	struct SMapii n;
	n.x.k = k;
	const struct SMapii *nr =
			(const struct SMapii *)st_locate(m, (const stn_t *)&n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

uint32_t sm_at_uu32(const sm_t *m, const uint32_t k)
{
	RETURN_IF(!sm_chk_t(m, SM_UU32), 0);
	struct SMapuu n;
	n.x.k = k;
	const struct SMapuu *nr =
			(const struct SMapuu *)st_locate(m, (const stn_t *)&n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

int64_t sm_at_ii(const sm_t *m, const int64_t k)
{
	RETURN_IF(!sm_chk_t(m, SM_II), 0);
	struct SMapII n;
	n.x.k = k;
	const struct SMapII *nr =
			(const struct SMapII *)st_locate(m, (const stn_t *)&n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

const ss_t *sm_at_is(const sm_t *m, const int64_t k)
{
	RETURN_IF(!sm_chk_t(m, SM_IS), ss_void);
	struct SMapIS n;
	n.x.k = k;
	const struct SMapIS *nr =
			(const struct SMapIS *)st_locate(m, (const stn_t *)&n);
	return nr ? SMStrGet(&nr->v) : 0; /* BEHAVIOR */
}

const void *sm_at_ip(const sm_t *m, const int64_t k)
{
	RETURN_IF(!sm_chk_t(m, SM_IP), NULL);
	struct SMapIP n;
	n.x.k = k;
	const struct SMapIP *nr =
			(const struct SMapIP *)st_locate(m, (const stn_t *)&n);
	return nr ? nr->v : NULL;
}

int64_t sm_at_si(const sm_t *m, const ss_t *k)
{
	RETURN_IF(!sm_chk_t(m, SM_SI), 0);
	struct SMapSI n;
	SMStrSetRef(&n.x.k, k);
	const struct SMapSI *nr =
			(const struct SMapSI *)st_locate(m, &n.x.n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

const ss_t *sm_at_ss(const sm_t *m, const ss_t *k)
{
	RETURN_IF(!sm_chk_t(m, SM_SS), ss_void);
	struct SMapSS n;
	SMStrSetRef(&n.x.k, k);
	const struct SMapSS *nr =
			(const struct SMapSS *)st_locate(m, &n.x.n);
	return nr ? SMStrGet(&nr->v) : ss_void;
}

const void *sm_at_sp(const sm_t *m, const ss_t *k)
{
	RETURN_IF(!sm_chk_t(m, SM_SP), NULL);
	struct SMapSP n;
	SMStrSetRef(&n.x.k, k);
	const struct SMapSP *nr =
			(const struct SMapSP *)st_locate(m, &n.x.n);
	return nr ? nr->v : NULL;
}

/*
 * Existence check
 */

sbool_t sm_count_u(const sm_t *m, const uint32_t k)
{
	ASSERT_RETURN_IF(!sm_chk_t(m, SM0_UU32) && !sm_chk_t(m, SM0_U32), S_FALSE);
	struct SMapuu n;
	n.x.k = k;
	return st_locate(m, (const stn_t *)&n) ? S_TRUE : S_FALSE;
}

sbool_t sm_count_i(const sm_t *m, const int64_t k)
{
	ASSERT_RETURN_IF(!m, S_FALSE);
	const stn_t *n;
	struct SMapI n1;
	struct SMapi n2;
	if (sm_chk_Ix(m)) {
		n1.k = k;
		n = (const stn_t *)&n1;
	} else {
		RETURN_IF((!sm_chk_t(m, SM0_II32) && !sm_chk_t(m, SM0_I32)) ||
			  !sm_i32_range(k), S_FALSE);
		n2.k = (int32_t)k;
		n = (const stn_t *)&n2;
	}
	return st_locate(m, n) ? S_TRUE : S_FALSE;
}

sbool_t sm_count_s(const sm_t *m, const ss_t *k)
{
	ASSERT_RETURN_IF(!sm_chk_sx(m), S_FALSE);
	struct SMapS n;
	SMStrSetRef(&n.k, k);
	return st_locate(m, (const stn_t *)&n) ? S_TRUE : S_FALSE;
}

/*
 * Insert
 */

S_INLINE sbool_t sm_insert_ii32_aux(sm_t **m, const int32_t k,
				    const int32_t v, const st_rewrite_t rw_f)
{
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_II32), S_FALSE);
	struct SMapii n;
	n.x.k = k;
	n.v = v;
	return st_insert_rw((st_t **)m, (const stn_t *)&n, rw_f);
}

sbool_t sm_insert_ii32(sm_t **m, const int32_t k, const int32_t v)
{
	return sm_insert_ii32_aux(m, k, v, NULL);
}

sbool_t sm_inc_ii32(sm_t **m, const int32_t k, const int32_t v)
{
	return sm_insert_ii32_aux(m, k, v, rw_inc_SM_II32);
}

S_INLINE sbool_t sm_insert_uu32_aux(sm_t **m, const uint32_t k,
				    const uint32_t v, const st_rewrite_t rw_f)
{
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_UU32), S_FALSE);
	struct SMapuu n;
	n.x.k = k;
	n.v = v;
	return st_insert_rw((st_t **)m, (const stn_t *)&n, rw_f);
}

sbool_t sm_insert_uu32(sm_t **m, const uint32_t k, const uint32_t v)
{
	return sm_insert_uu32_aux(m, k, v, NULL);
}

sbool_t sm_inc_uu32(sm_t **m, const uint32_t k, const uint32_t v)
{
	return sm_insert_uu32_aux(m, k, v, rw_inc_SM_UU32);
}

S_INLINE sbool_t sm_insert_ii_aux(sm_t **m, const int64_t k,
			          const int64_t v, const st_rewrite_t rw_f)
{
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_II), S_FALSE);
	struct SMapII n;
	n.x.k = k;
	n.v = v;
	return st_insert_rw((st_t **)m, (const stn_t *)&n, rw_f);
}

sbool_t sm_insert_ii(sm_t **m, const int64_t k, const int64_t v)
{
	return sm_insert_ii_aux(m, k, v, NULL);
}

sbool_t sm_inc_ii(sm_t **m, const int64_t k, const int64_t v)
{
	return sm_insert_ii_aux(m, k, v, rw_inc_SM_II);
}

sbool_t sm_insert_is(sm_t **m, const int64_t k, const ss_t *v)
{
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_IS), S_FALSE);
	struct SMapIS n;
	n.x.k = k;
#if 1 /* workaround */
	SMStrSet(&n.v, v);
	sbool_t ins_ok = st_insert((st_t **)m, (const stn_t *)&n);
	if (!ins_ok)
		SMStrFree(&n.v);
	return ins_ok;
#else
	/* TODO: rw_add_SM_IS */
	n.v = v;
	return st_insert_rw((st_t **)m, (const stn_t *)&n, rw_add_SM_IS);
#endif
}

sbool_t sm_insert_ip(sm_t **m, const int64_t k, const void *v)
{
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_IP), S_FALSE);
	struct SMapIP n;
	n.x.k = k;
	n.v = v;
	return st_insert((st_t **)m, (const stn_t *)&n);
}

S_INLINE sbool_t sm_insert_si_aux(sm_t **m, const ss_t *k,
				  const int64_t v, const st_rewrite_t rw_f)
{
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_SI), S_FALSE);
	struct SMapSI n;
	SMStrSetRef(&n.x.k, k);
	n.v = v;
	sbool_t r = st_insert_rw((st_t **)m, (const stn_t *)&n, rw_f);
	return r;
}

sbool_t sm_insert_si(sm_t **m, const ss_t *k, const int64_t v)
{
	return sm_insert_si_aux(m, k, v, rw_add_SM_SI);
}

sbool_t sm_inc_si(sm_t **m, const ss_t *k, const int64_t v)
{
	return sm_insert_si_aux(m, k, v, rw_inc_SM_SI);
}

sbool_t sm_insert_ss(sm_t **m, const ss_t *k, const ss_t *v)
{
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_SS), S_FALSE);
	struct SMapSS n;
	SMStrSetRef(&n.x.k, k);
	SMStrSetRef(&n.v, v);
	return st_insert_rw((st_t **)m, (const stn_t *)&n, rw_add_SM_SS);
}

sbool_t sm_insert_sp(sm_t **m, const ss_t *k, const void *v)
{
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_SP), S_FALSE);
	struct SMapSP n;
	SMStrSetRef(&n.x.k, k);
	n.v = v;
	return st_insert_rw((st_t **)m, (const stn_t *)&n, rw_add_SM_SP);
}

/*
 * Delete
 */

sbool_t sm_delete_i(sm_t *m, const int64_t k)
{
	struct SMapI n_i64;
	struct SMapi n_i32;
	struct SMapu n_u32;
	const stn_t *n;
	stn_callback_t callback = NULL;
	switch (m->d.sub_type) {
	case SM0_I32: case SM0_II32:
		RETURN_IF(!sm_i32_range(k), S_FALSE);
		n_i32.k = (int32_t)k;
		n = (const stn_t *)&n_i32;
		break;
	case SM0_U32: case SM0_UU32:
		RETURN_IF(k > SUINT32_MAX, S_FALSE);
		n_u32.k = (uint32_t)k;
		n = (const stn_t *)&n_u32;
		break;
	case SM0_IS:
		callback = aux_is_delete;
		/* don't break */
	case SM0_I: case SM0_II: case SM0_IP:
		n_i64.k = k;
		n = (const stn_t *)&n_i64;
		break;
	default:
		return S_FALSE;
	}
	return st_delete(m, n, callback);
}

sbool_t sm_delete_s(sm_t *m, const ss_t *k)
{
	stn_callback_t callback = NULL;
	struct SMapS sx;
	SMStrSetRef(&sx.k, k);
	switch (m->d.sub_type) {
		case SM0_SS:
			callback = aux_ss_delete;
			break;
		case SM0_S: case SM0_SP: case SM0_SI:
			callback = aux_sx_delete;
			break;
		default:
			return S_FALSE;
	}
	return st_delete(m, (const stn_t *)&sx, callback);
}

/*
 * Enumeration / export data
 */

ssize_t sm_sort_to_vectors(const sm_t *m, sv_t **kv, sv_t **vv)
{
	RETURN_IF(!m || !kv || !vv, 0);
	struct SV2X v2x = { *kv, *vv };
	st_traverse traverse_f = NULL;
	enum eSV_Type kt, vt;
	switch (m->d.sub_type) {
	case SM_II32:
		kt = vt = SV_I32;
		break;
	case SM_UU32:
		kt = vt = SV_U32;
		break;
	case SM_II: case SM_IS: case SM_IP:
		kt = SV_I64;
		vt = sm_chk_t(m, SM_II) ? SV_I64 : SV_GEN;
		break;
	case SM_SI: case SM_SS: case SM_SP:
		kt = SV_GEN;
		vt = sm_chk_t(m, SM_SI) ? SV_I64 : SV_GEN;
		break;
	default: return 0; /* BEHAVIOR: invalid type */
	}
	if (v2x.kv) {
		if (v2x.kv->d.sub_type != (uint8_t)kt)
			sv_free(&v2x.kv);
		else
			sv_reserve(&v2x.kv, m->d.size);
	}
	if (v2x.vv) {
		if (v2x.vv->d.sub_type != (uint8_t)vt)
			sv_free(&v2x.vv);
		else
			sv_reserve(&v2x.vv, m->d.size);
	}
	if (!v2x.kv)
		v2x.kv = sv_alloc_t(kt, m->d.size);
	if (!v2x.vv)
		v2x.vv = sv_alloc_t(vt, m->d.size);
	switch (m->d.sub_type) {
	case SM_II32: traverse_f = aux_ii32_sort; break;
	case SM_UU32: traverse_f = aux_uu32_sort; break;
	case SM_II: traverse_f = aux_ii_sort; break;
	case SM_IS: case SM_IP: traverse_f = aux_is_ip_sort; break;
	case SM_SI: traverse_f = aux_si_sort; break;
	case SM_SS: case SM_SP: traverse_f = aux_sp_ss_sort; break;
	}
	ssize_t r = st_traverse_inorder((const st_t *)m, traverse_f,
					(void *)&v2x);
	*kv = v2x.kv;
	*vv = v2x.vv;
	return r;
}

#ifdef S_ENABLE_SM_STRING_OPTIMIZATION

void SMStrUpdate_unsafe(union SMStr *sstr, const ss_t *s)
{
	if (sstr->t == SMStr_Indirect)
		ss_cpy(&sstr->i.s, s);
	else {
		const size_t ss = ss_size(s);
		if (ss <= SMStrMaxSize) {
			ss_t *s_out = (ss_t *)sstr->d.s_raw;
			ss_alloc_into_ext_buf(s_out, SMStrMaxSize);
			sstr->t = SMStr_Direct;
			ss_cpy(&s_out, s);
		} else {
			sstr->t = SMStr_Indirect;
			sstr->i.s = ss_dup(s);
		}
	}
}

#endif


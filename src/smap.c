/*
 * smap.c
 *
 * Map handling.
 *
 * Copyright (c) 2015-2018 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
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

static void rw_inc_SM_II32(srt_tnode *node, const srt_tnode *new_data,
			   const srt_bool existing)
{
	if (existing)
		((struct SMapii *)node)->v +=
			((const struct SMapii *)new_data)->v;
}

static void rw_inc_SM_UU32(srt_tnode *node, const srt_tnode *new_data,
			   const srt_bool existing)
{
	if (existing)
		((struct SMapuu *)node)->v +=
			((const struct SMapuu *)new_data)->v;
}

static void rw_inc_SM_II(srt_tnode *node, const srt_tnode *new_data,
			 const srt_bool existing)
{
	if (existing)
		((struct SMapII *)node)->v +=
			((const struct SMapII *)new_data)->v;
}

static void rw_add_SM_SP(srt_tnode *node, const srt_tnode *new_data,
			 const srt_bool existing)
{
	struct SMapSP *n = (struct SMapSP *)node;
	const struct SMapSP *m = (const struct SMapSP *)new_data;
	if (!existing)
		SMStrSet(&n->x.k, SMStrGet(&m->x.k));
	else
		SMStrUpdate(&n->x.k, SMStrGet(&m->x.k));
}

static void rw_add_SM_SS(srt_tnode *node, const srt_tnode *new_data,
			 const srt_bool existing)
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

static void rw_add_SM_SI(srt_tnode *node, const srt_tnode *new_data,
			 const srt_bool existing)
{
	struct SMapSI *n = (struct SMapSI *)node;
	const struct SMapSI *m = (const struct SMapSI *)new_data;
	if (!existing)
		SMStrSet(&n->x.k, SMStrGet(&m->x.k));
	else
		SMStrUpdate(&n->x.k, SMStrGet(&m->x.k));
}

static void rw_inc_SM_SI(srt_tnode *node, const srt_tnode *new_data,
			 const srt_bool existing)
{
	if (!existing)
		rw_add_SM_SI(node, new_data, existing);
	else
		((struct SMapSI *)node)->v +=
			((const struct SMapSI *)new_data)->v;
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

struct SV2X {
	srt_vector *kv, *vv;
};

static int aux_ii32_sort(struct STraverseParams *tp)
{
	const struct SMapii *cn =
		(const struct SMapii *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push_i(&v2x->kv, cn->x.k);
		sv_push_i(&v2x->vv, cn->v);
	}
	return 0;
}

static int aux_uu32_sort(struct STraverseParams *tp)
{
	const struct SMapuu *cn =
		(const struct SMapuu *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push_u(&v2x->kv, cn->x.k);
		sv_push_u(&v2x->vv, cn->v);
	}
	return 0;
}

static int aux_ii_sort(struct STraverseParams *tp)
{
	const struct SMapII *cn =
		(const struct SMapII *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push_i(&v2x->kv, cn->x.k);
		sv_push_i(&v2x->vv, cn->v);
	}
	return 0;
}

static int aux_is_ip_sort(struct STraverseParams *tp)
{
	const struct SMapIP *cn =
		(const struct SMapIP *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push_i(&v2x->kv, cn->x.k);
		sv_push(&v2x->vv, &cn->v);
	}
	return 0;
}

static int aux_si_sort(struct STraverseParams *tp)
{
	const struct SMapII *cn =
		(const struct SMapII *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push(&v2x->kv, &cn->x.k);
		sv_push_i(&v2x->vv, cn->v);
	}
	return 0;
}

static int aux_sp_ss_sort(struct STraverseParams *tp)
{
	const struct SMapII *cn =
		(const struct SMapII *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push(&v2x->kv, &cn->x.k);
		sv_push(&v2x->vv, &cn->v);
	}
	return 0;
}

static srt_cmp type2cmpf(const enum eSM_Type0 t)
{
	switch (t) {
	case SM0_U32:
	case SM0_UU32:
		return (srt_cmp)cmp_u;
	case SM0_I32:
	case SM0_II32:
		return (srt_cmp)cmp_i;
	case SM0_I:
	case SM0_II:
	case SM0_IS:
	case SM0_IP:
		return (srt_cmp)cmp_I;
	case SM0_S:
	case SM0_SI:
	case SM0_SS:
	case SM0_SP:
		return (srt_cmp)cmp_s;
	}
	return NULL;
}

S_INLINE srt_bool sm_chk_Ix(const srt_map *m)
{
	int t;
	RETURN_IF(!m, S_FALSE);
	t = m->d.sub_type;
	return t == SM0_II || t == SM0_IS || t == SM0_IP || t == SM0_I
		       ? S_TRUE
		       : S_FALSE;
}

S_INLINE srt_bool sm_chk_sx(const srt_map *m)
{
	int t;
	RETURN_IF(!m, S_FALSE);
	t = m->d.sub_type;
	return t == SM0_SS || t == SM0_SI || t == SM0_SP || t == SM0_S
		       ? S_TRUE
		       : S_FALSE;
}

S_INLINE srt_bool sm_chk_t(const srt_map *m, int t)
{
	return m && m->d.sub_type == t ? S_TRUE : S_FALSE;
}

S_INLINE srt_bool sm_i32_range(const int64_t k)
{
	return k >= INT32_MIN && k <= INT32_MAX ? S_TRUE : S_FALSE;
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

SM_ENUM_INORDER_XX(sm_itr_is, srt_map_it_is, SM_IS, int64_t,
		   cmp_nI_I((const struct SMapI *)cn, kmin),
		   cmp_nI_I((const struct SMapI *)cn, kmax),
		   f(((const struct SMapI *)cn)->k,
		     SMStrGet(&((const struct SMapIS *)cn)->v), context))

SM_ENUM_INORDER_XX(sm_itr_ip, srt_map_it_ip, SM_IP, int64_t,
		   cmp_nI_I((const struct SMapI *)cn, kmin),
		   cmp_nI_I((const struct SMapI *)cn, kmax),
		   f(((const struct SMapI *)cn)->k,
		     ((const struct SMapIP *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_si, srt_map_it_si, SM_SI, const srt_string *,
		   cmp_ns_s((const struct SMapS *)cn, kmin),
		   cmp_ns_s((const struct SMapS *)cn, kmax),
		   f(SMStrGet(&((const struct SMapS *)cn)->k),
		     ((const struct SMapSI *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_ss, srt_map_it_ss, SM_SS, const srt_string *,
		   cmp_ns_s((const struct SMapS *)cn, kmin),
		   cmp_ns_s((const struct SMapS *)cn, kmax),
		   f(SMStrGet(&((const struct SMapS *)cn)->k),
		     SMStrGet(&((const struct SMapSS *)cn)->v), context))

SM_ENUM_INORDER_XX(sm_itr_sp, srt_map_it_sp, SM_SP, const srt_string *,
		   cmp_ns_s((const struct SMapS *)cn, kmin),
		   cmp_ns_s((const struct SMapS *)cn, kmax),
		   f(SMStrGet(&((const struct SMapS *)cn)->k),
		     ((const struct SMapSP *)cn)->v, context))

/*
 * Allocation
 */

srt_map *sm_alloc_raw0(const enum eSM_Type0 t, const srt_bool ext_buf,
		       void *buffer, const size_t elem_size,
		       const size_t max_size)
{
	srt_map *m;
	RETURN_IF(!buffer || !max_size, NULL);
	m = (srt_map *)st_alloc_raw(type2cmpf(t), ext_buf, buffer, elem_size,
				    max_size);
	m->d.sub_type = t;
	return m;
}

srt_map *sm_alloc0(const enum eSM_Type0 t, const size_t init_size)
{
	srt_map *m =
		(srt_map *)st_alloc(type2cmpf(t), sm_elem_size(t), init_size);
	m->d.sub_type = t;
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
	srt_tree_callback delete_callback = NULL;
	if (!m || !m->d.size)
		return;
	switch (m->d.sub_type) {
	case SM0_IS:
		delete_callback = aux_is_delete;
		break;
	case SM0_SS:
		delete_callback = aux_ss_delete;
		break;
	case SM0_S:
	case SM0_SI:
	case SM0_SP:
		delete_callback = aux_sx_delete;
		break;
	}
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
		if (!sm_chk_t(*m, t)) {
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
			SMStrSet(&mt->v, SMStrGet(&ms->v));
		}
		break;
	case SM0_S:
	case SM0_SI:
	case SM0_SP:
		for (i = 0; i < ss; i++) {
			const struct SMapS *ms =
				(const struct SMapS *)st_enum_r(src, i);
			struct SMapS *mt = (struct SMapS *)st_enum(*m, i);
			SMStrSet(&mt->k, SMStrGet(&ms->k));
		}
		break;
	case SM0_SS:
		for (i = 0; i < ss; i++) {
			const struct SMapSS *ms =
				(const struct SMapSS *)st_enum_r(src, i);
			struct SMapSS *mt = (struct SMapSS *)st_enum(*m, i);
			SMStrSet(&mt->x.k, SMStrGet(&ms->x.k));
			SMStrSet(&mt->v, SMStrGet(&ms->v));
		}
		break;
	case SM0_II32:
	case SM0_UU32:
	case SM0_II:
	case SM0_IP:
	case SM0_I:
	case SM0_I32:
	case SM0_U32:
		/* no additional action required */
		break;
	}
	return *m;
}

/*
 * Random access
 */

int32_t sm_at_ii32(const srt_map *m, const int32_t k)
{
	struct SMapii n;
	const struct SMapii *nr;
	RETURN_IF(!sm_chk_t(m, SM_II32), 0);
	n.x.k = k;
	nr = (const struct SMapii *)st_locate(m, (const srt_tnode *)&n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

uint32_t sm_at_uu32(const srt_map *m, const uint32_t k)
{
	struct SMapuu n;
	const struct SMapuu *nr;
	RETURN_IF(!sm_chk_t(m, SM_UU32), 0);
	n.x.k = k;
	nr = (const struct SMapuu *)st_locate(m, (const srt_tnode *)&n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

int64_t sm_at_ii(const srt_map *m, const int64_t k)
{
	struct SMapII n;
	const struct SMapII *nr;
	RETURN_IF(!sm_chk_t(m, SM_II), 0);
	n.x.k = k;
	nr = (const struct SMapII *)st_locate(m, (const srt_tnode *)&n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

const srt_string *sm_at_is(const srt_map *m, const int64_t k)
{
	struct SMapIS n;
	const struct SMapIS *nr;
	RETURN_IF(!sm_chk_t(m, SM_IS), ss_void);
	n.x.k = k;
	nr = (const struct SMapIS *)st_locate(m, (const srt_tnode *)&n);
	return nr ? SMStrGet(&nr->v) : 0; /* BEHAVIOR */
}

const void *sm_at_ip(const srt_map *m, const int64_t k)
{
	struct SMapIP n;
	const struct SMapIP *nr;
	RETURN_IF(!sm_chk_t(m, SM_IP), NULL);
	n.x.k = k;
	nr = (const struct SMapIP *)st_locate(m, (const srt_tnode *)&n);
	return nr ? nr->v : NULL;
}

int64_t sm_at_si(const srt_map *m, const srt_string *k)
{
	struct SMapSI n;
	const struct SMapSI *nr;
	RETURN_IF(!sm_chk_t(m, SM_SI), 0);
	SMStrSetRef(&n.x.k, k);
	nr = (const struct SMapSI *)st_locate(m, &n.x.n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

const srt_string *sm_at_ss(const srt_map *m, const srt_string *k)
{
	struct SMapSS n;
	const struct SMapSS *nr;
	RETURN_IF(!sm_chk_t(m, SM_SS), ss_void);
	SMStrSetRef(&n.x.k, k);
	nr = (const struct SMapSS *)st_locate(m, &n.x.n);
	return nr ? SMStrGet(&nr->v) : ss_void;
}

const void *sm_at_sp(const srt_map *m, const srt_string *k)
{
	struct SMapSP n;
	const struct SMapSP *nr;
	RETURN_IF(!sm_chk_t(m, SM_SP), NULL);
	SMStrSetRef(&n.x.k, k);
	nr = (const struct SMapSP *)st_locate(m, &n.x.n);
	return nr ? nr->v : NULL;
}

/*
 * Existence check
 */

srt_bool sm_count_u(const srt_map *m, const uint32_t k)
{
	struct SMapuu n;
	RETURN_IF(!sm_chk_t(m, SM0_UU32) && !sm_chk_t(m, SM0_U32), S_FALSE);
	n.x.k = k;
	return st_locate(m, (const srt_tnode *)&n) ? S_TRUE : S_FALSE;
}

srt_bool sm_count_i(const srt_map *m, const int64_t k)
{
	const srt_tnode *n;
	struct SMapI n1;
	struct SMapi n2;
	ASSERT_RETURN_IF(!m, S_FALSE);
	if (sm_chk_Ix(m)) {
		n1.k = k;
		n = (const srt_tnode *)&n1;
	} else {
		RETURN_IF((!sm_chk_t(m, SM0_II32) && !sm_chk_t(m, SM0_I32))
				  || !sm_i32_range(k),
			  S_FALSE);
		n2.k = (int32_t)k;
		n = (const srt_tnode *)&n2;
	}
	return st_locate(m, n) ? S_TRUE : S_FALSE;
}

srt_bool sm_count_s(const srt_map *m, const srt_string *k)
{
	struct SMapS n;
	ASSERT_RETURN_IF(!sm_chk_sx(m), S_FALSE);
	SMStrSetRef(&n.k, k);
	return st_locate(m, (const srt_tnode *)&n) ? S_TRUE : S_FALSE;
}

/*
 * Insert
 */

S_INLINE srt_bool sm_insert_ii32_aux(srt_map **m, const int32_t k,
				     const int32_t v,
				     const srt_tree_rewrite rw_f)
{
	struct SMapii n;
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_II32), S_FALSE);
	n.x.k = k;
	n.v = v;
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n, rw_f);
}

srt_bool sm_insert_ii32(srt_map **m, const int32_t k, const int32_t v)
{
	return sm_insert_ii32_aux(m, k, v, NULL);
}

srt_bool sm_inc_ii32(srt_map **m, const int32_t k, const int32_t v)
{
	return sm_insert_ii32_aux(m, k, v, rw_inc_SM_II32);
}

S_INLINE srt_bool sm_insert_uu32_aux(srt_map **m, const uint32_t k,
				     const uint32_t v,
				     const srt_tree_rewrite rw_f)
{
	struct SMapuu n;
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_UU32), S_FALSE);
	n.x.k = k;
	n.v = v;
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n, rw_f);
}

srt_bool sm_insert_uu32(srt_map **m, const uint32_t k, const uint32_t v)
{
	return sm_insert_uu32_aux(m, k, v, NULL);
}

srt_bool sm_inc_uu32(srt_map **m, const uint32_t k, const uint32_t v)
{
	return sm_insert_uu32_aux(m, k, v, rw_inc_SM_UU32);
}

S_INLINE srt_bool sm_insert_ii_aux(srt_map **m, const int64_t k,
				   const int64_t v, const srt_tree_rewrite rw_f)
{
	struct SMapII n;
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_II), S_FALSE);
	n.x.k = k;
	n.v = v;
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n, rw_f);
}

srt_bool sm_insert_ii(srt_map **m, const int64_t k, const int64_t v)
{
	return sm_insert_ii_aux(m, k, v, NULL);
}

srt_bool sm_inc_ii(srt_map **m, const int64_t k, const int64_t v)
{
	return sm_insert_ii_aux(m, k, v, rw_inc_SM_II);
}

srt_bool sm_insert_is(srt_map **m, const int64_t k, const srt_string *v)
{
	srt_bool ins_ok;
	struct SMapIS n;
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_IS), S_FALSE);
	n.x.k = k;
#if 1 /* workaround */
	SMStrSet(&n.v, v);
	ins_ok = st_insert((srt_tree **)m, (const srt_tnode *)&n);
	if (!ins_ok)
		SMStrFree(&n.v);
	return ins_ok;
#else
	/* TODO: rw_add_SM_IS */
	n.v = v;
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n,
			    rw_add_SM_IS);
#endif
}

srt_bool sm_insert_ip(srt_map **m, const int64_t k, const void *v)
{
	struct SMapIP n;
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_IP), S_FALSE);
	n.x.k = k;
	n.v = v;
	return st_insert((srt_tree **)m, (const srt_tnode *)&n);
}

S_INLINE srt_bool sm_insert_si_aux(srt_map **m, const srt_string *k,
				   const int64_t v, const srt_tree_rewrite rw_f)
{
	srt_bool r;
	struct SMapSI n;
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_SI), S_FALSE);
	SMStrSetRef(&n.x.k, k);
	n.v = v;
	r = st_insert_rw((srt_tree **)m, (const srt_tnode *)&n, rw_f);
	return r;
}

srt_bool sm_insert_si(srt_map **m, const srt_string *k, const int64_t v)
{
	return sm_insert_si_aux(m, k, v, rw_add_SM_SI);
}

srt_bool sm_inc_si(srt_map **m, const srt_string *k, const int64_t v)
{
	return sm_insert_si_aux(m, k, v, rw_inc_SM_SI);
}

srt_bool sm_insert_ss(srt_map **m, const srt_string *k, const srt_string *v)
{
	struct SMapSS n;
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_SS), S_FALSE);
	SMStrSetRef(&n.x.k, k);
	SMStrSetRef(&n.v, v);
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n,
			    rw_add_SM_SS);
}

srt_bool sm_insert_sp(srt_map **m, const srt_string *k, const void *v)
{
	struct SMapSP n;
	ASSERT_RETURN_IF(!m || !sm_chk_t(*m, SM0_SP), S_FALSE);
	SMStrSetRef(&n.x.k, k);
	n.v = v;
	return st_insert_rw((srt_tree **)m, (const srt_tnode *)&n,
			    rw_add_SM_SP);
}

/*
 * Delete
 */

srt_bool sm_delete_i(srt_map *m, const int64_t k)
{
	struct SMapI n_i64;
	struct SMapi n_i32;
	struct SMapu n_u32;
	const srt_tnode *n;
	srt_tree_callback callback = NULL;
	switch (m->d.sub_type) {
	case SM0_I32:
	case SM0_II32:
		RETURN_IF(!sm_i32_range(k), S_FALSE);
		n_i32.k = (int32_t)k;
		n = (const srt_tnode *)&n_i32;
		break;
	case SM0_U32:
	case SM0_UU32:
		RETURN_IF(k > UINT32_MAX, S_FALSE);
		n_u32.k = (uint32_t)k;
		n = (const srt_tnode *)&n_u32;
		break;
	case SM0_IS:
	case SM0_I:
	case SM0_II:
	case SM0_IP:
		if (m->d.sub_type == SM0_IS)
			callback = aux_is_delete;
		n_i64.k = k;
		n = (const srt_tnode *)&n_i64;
		break;
	default:
		return S_FALSE;
	}
	return st_delete(m, n, callback);
}

srt_bool sm_delete_s(srt_map *m, const srt_string *k)
{
	srt_tree_callback callback = NULL;
	struct SMapS sx;
	SMStrSetRef(&sx.k, k);
	switch (m->d.sub_type) {
	case SM0_SS:
		callback = aux_ss_delete;
		break;
	case SM0_S:
	case SM0_SP:
	case SM0_SI:
		callback = aux_sx_delete;
		break;
	default:
		return S_FALSE;
	}
	return st_delete(m, (const srt_tnode *)&sx, callback);
}

/*
 * Enumeration / export data
 */

ssize_t sm_sort_to_vectors(const srt_map *m, srt_vector **kv, srt_vector **vv)
{
	ssize_t r;
	struct SV2X v2x;
	st_traverse traverse_f = NULL;
	enum eSV_Type kt, vt;
	RETURN_IF(!m || !kv || !vv, 0);
	v2x.kv = *kv;
	v2x.vv = *vv;
	switch (m->d.sub_type) {
	case SM_II32:
		kt = vt = SV_I32;
		break;
	case SM_UU32:
		kt = vt = SV_U32;
		break;
	case SM_II:
	case SM_IS:
	case SM_IP:
		kt = SV_I64;
		vt = sm_chk_t(m, SM_II) ? SV_I64 : SV_GEN;
		break;
	case SM_SI:
	case SM_SS:
	case SM_SP:
		kt = SV_GEN;
		vt = sm_chk_t(m, SM_SI) ? SV_I64 : SV_GEN;
		break;
	default:
		return 0; /* BEHAVIOR: invalid type */
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
	case SM_II32:
		traverse_f = aux_ii32_sort;
		break;
	case SM_UU32:
		traverse_f = aux_uu32_sort;
		break;
	case SM_II:
		traverse_f = aux_ii_sort;
		break;
	case SM_IS:
	case SM_IP:
		traverse_f = aux_is_ip_sort;
		break;
	case SM_SI:
		traverse_f = aux_si_sort;
		break;
	case SM_SS:
	case SM_SP:
		traverse_f = aux_sp_ss_sort;
		break;
	}
	r = st_traverse_inorder((const srt_tree *)m, traverse_f, (void *)&v2x);
	*kv = v2x.kv;
	*vv = v2x.vv;
	return r;
}

#ifdef S_ENABLE_SM_STRING_OPTIMIZATION

void SMStrUpdate_unsafe(union SMStr *sstr, const srt_string *s)
{
	if (sstr->t == SMStr_Indirect)
		ss_cpy(&sstr->i.s, s);
	else {
		const size_t ss = ss_size(s);
		if (ss <= SMStrMaxSize) {
			srt_string *s_out = (srt_string *)sstr->d.s_raw;
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

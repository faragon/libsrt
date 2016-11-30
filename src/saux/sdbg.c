/*
 * sdbg.c
 *
 * Debug helpers (data formatting, etc.).
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */ 

#include "sdbg.h"

const char *sv_type_to_label(const enum eSV_Type t)
{
	switch (t) {
		#define CXSV(t) case t: return #t
		CXSV(SV_I8); CXSV(SV_U8); CXSV(SV_I16); CXSV(SV_U16);
		CXSV(SV_I32); CXSV(SV_U32); CXSV(SV_I64); CXSV(SV_U64);
		CXSV(SV_GEN);
		#undef CXSV
	} 
	return "?";
}

void sv_log_obj(ss_t **log, const sv_t *v)
{
	if (!log)
		return;
	const size_t elems = sd_size((const sd_t *)v);
	enum eSV_Type t = v ? (enum eSV_Type)v->d.sub_type : SV_GEN;
	const size_t elem_size = v ? v->d.elem_size : 0;
	ss_cat_printf(log, 512, "sv_t: t: %s, elem size: " FMT_ZU ", sz: "
		      FMT_ZU ", { ", sv_type_to_label(t), elem_size, elems);
	size_t i = 0;
#ifdef _MSC_VER /* supress alloca() warning */
#pragma warning(disable: 6255)
#endif
	ss_t *aux = ss_alloca(elem_size * 2);
	const char *buf = (const char *)sv_get_buffer_r(v);
	for (; i < elems; i++) {
		ss_cpy_cn(&aux, buf + i * elem_size , elem_size);
		ss_cat_enc_hex(log, aux);
		if (i + 1 < elems)
			ss_cat_cn(log, ", ", 2);
	}
	ss_cat_c(log, " }\n");
}

struct st_log_context_data
{
	ss_t **log;
	ss_cat_stn tf;
};

static int aux_st_log_traverse(struct STraverseParams *tp)
{
	struct st_log_context_data *d =
				(struct st_log_context_data *)tp->context;
	if (tp->c == ST_NIL) {
		ss_cat_printf(d->log, 128, "\nLevel: %u\n",
			      (unsigned)tp->level);
	}
	else {
		const stn_t *cn = get_node_r(tp->t, tp->c);
		d->tf(d->log, cn, tp->c);
		ss_cat_c(d->log, " ");
	}
	return 0;
}

void st_log_obj(ss_t **log, const st_t *t, ss_cat_stn f)
{
	if (!log)
		return;
	struct st_log_context_data context = { log, f };
	ss_cpy_c(log, "");
	ssize_t levels = st_traverse_levelorder(t, aux_st_log_traverse,
						&context);
	if (levels == 0)
		ss_cat_c(log, "empty tree");
	else
		ss_cat_printf(log, 128, "\nlevels: %i, nodes: %u\n",
			      (int)levels, (unsigned)st_size(t));
	fprintf(stdout, "%s", ss_to_c(*log));
}

static void ndx2s(char *out, const size_t out_max, const stndx_t id)
{
	if (id == ST_NIL)
		strcpy(out, "nil");
	else
		snprintf(out, out_max, "%u", (unsigned)id);
}

static int aux_sm_log_traverse(struct STraverseParams *tp)
{
	ss_t **log = (ss_t **)tp->context;
	if (tp->c == ST_NIL) {
		ss_cat_printf(log, 128, "\nLevel: %u\n", (unsigned)tp->level);
		return 0;
	}
	char k[4096] = "", v[4096] = "";
	const stn_t *cn = get_node_r(tp->t, tp->c);
	switch (tp->t->d.sub_type) {
	case SM_II32:
		sprintf(k, "%i", ((const struct SMapii *)cn)->x.k);
		sprintf(v, "%i", ((const struct SMapii *)cn)->v);
		break;
	case SM_UU32:
		sprintf(k, "%u", ((const struct SMapuu *)cn)->x.k);
		sprintf(v, "%u", ((const struct SMapuu *)cn)->v);
		break;
	case SM_II:
	case SM_IS:
	case SM_IP:
		sprintf(k, FMT_I, ((const struct SMapI *)cn)->k);
		break;
	case SM_SI:
		sprintf(k, "%s",
			ss_to_c(SMStrGet(&((const struct SMapSI *)cn)->x.k)));
		break;
	case SM_SS:
	case SM_SP:
		sprintf(k, "%s",
			ss_to_c(SMStrGet(&((const struct SMapS *)cn)->k)));
		break;
	}
	switch (tp->t->d.sub_type) {
	case SM_II:
		sprintf(v, FMT_I, ((const struct SMapII *)cn)->v);
		break;
	case SM_SI:
		sprintf(v, FMT_I, ((const struct SMapSI *)cn)->v);
		break;
	case SM_IS:
		sprintf(k, "%s",
			ss_to_c(SMStrGet(&((const struct SMapIS *)cn)->v)));
		break;
	case SM_IP:
		sprintf(k, "%p",
			(const void *)((const struct SMapIP *)cn)->v);
		break;
	case SM_SS:
		sprintf(k, "%s",
			ss_to_c(SMStrGet(&((const struct SMapSS *)cn)->v)));
		break;
	case SM_SP:
		sprintf(k, "%p",
			(const void *)((const struct SMapSP *)cn)->v);
		break;
	}
	char id[128], l[128], r[128];
	ndx2s(id, sizeof(id), tp->c);
	ndx2s(l, sizeof(l), cn->x.l);
	ndx2s(r, sizeof(r), cn->r);
	ss_cat_printf(log, 128, "[%s: (%s, %s) -> (%s, %s; r:%u)] ",
		id, k, v, l, r, cn->x.is_red);
	return 0;
}

void sm_log_obj(ss_t **log, const sm_t *m)
{
	if (!log)
		return;
	ss_cpy_c(log, "");
	ssize_t levels = st_traverse_levelorder(
				(const st_t *)m,
				(st_traverse)aux_sm_log_traverse, log);
	if (levels == 0)
		ss_cat_c(log, "empty map");
	else
		ss_cat_printf(log, 128, "\nlevels: %i, nodes: %u\n",
			      (int)levels, (unsigned)st_size(m));
	fprintf(stdout, "%s", ss_to_c(*log));
}

void s_hex_dump(ss_t **log, const char *label, const char *buf,
		const size_t buf_size)
{
	if (!log)
		return;
	ss_t *aux = ss_dup_cn(buf, buf_size);
	if (label)
		ss_cat_c(log, label);
	ss_cat_enc_hex(log, aux);
	ss_free(&aux);
}


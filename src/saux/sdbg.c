/*
 * sdbg.c
 *
 * Debug helpers (data formatting, etc.).
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "sdbg.h"

const char *sv_type_to_label(const enum eSV_Type t)
{
	switch (t) {
#define CXSV(t)                                                                \
	case t:                                                                \
		return #t
		CXSV(SV_I8);
		CXSV(SV_U8);
		CXSV(SV_I16);
		CXSV(SV_U16);
		CXSV(SV_I32);
		CXSV(SV_U32);
		CXSV(SV_I64);
		CXSV(SV_U64);
		CXSV(SV_F);
		CXSV(SV_D);
		CXSV(SV_GEN);
	default:
		break;
	}
	return "?";
}

void sv_log_obj(srt_string **log, const srt_vector *v)
{
	size_t elems;
	enum eSV_Type t;
	size_t elem_size, i;
	srt_string *aux;
	const char *buf;
	if (!log)
		return;
	elems = sd_size((const srt_data *)v);
	t = v ? (enum eSV_Type)v->d.sub_type : SV_GEN;
	elem_size = v ? v->d.elem_size : 0;
	ss_cat_printf(log, 512,
		      "srt_vector: t: %s, elem size: " FMT_ZU ", sz: " FMT_ZU
		      ", { ",
		      sv_type_to_label(t), elem_size, elems);
	i = 0;
	aux = ss_alloca(elem_size * 2);
	buf = (const char *)sv_get_buffer_r(v);
	for (; i < elems; i++) {
		ss_cpy_cn(&aux, buf + i * elem_size, elem_size);
		ss_cat_enc_hex(log, aux);
		if (i + 1 < elems)
			ss_cat_cn(log, ", ", 2);
	}
	ss_cat_c(log, " }\n");
}

struct st_log_context_data {
	srt_string **log;
	ss_cat_stn tf;
};

static int aux_st_log_traverse(struct STraverseParams *tp)
{
	struct st_log_context_data *d =
		(struct st_log_context_data *)tp->context;
	if (tp->c == ST_NIL) {
		ss_cat_printf(d->log, 128, "\nLevel: %u\n",
			      (unsigned)tp->level);
	} else {
		const srt_tnode *cn = get_node_r(tp->t, tp->c);
		d->tf(d->log, cn, tp->c);
		ss_cat_c(d->log, " ");
	}
	return 0;
}

void st_log_obj(srt_string **log, const srt_tree *t, ss_cat_stn f)
{
	ssize_t levels;
	struct st_log_context_data context = {log, f};
	if (!log)
		return;
	ss_cpy_c(log, "");
	levels = st_traverse_levelorder(t, aux_st_log_traverse, &context);
	if (levels == 0)
		ss_cat_c(log, "empty tree");
	else
		ss_cat_printf(log, 128, "\nlevels: %i, nodes: %u\n",
			      (int)levels, (unsigned)st_size(t));
	fprintf(stdout, "%s", ss_to_c(*log));
}

static void ndx2s(char *out, size_t out_max, srt_tndx id)
{
	if (id == ST_NIL)
		strcpy(out, "nil");
	else
		snprintf(out, out_max, "%u", (unsigned)id);
}

static int aux_sm_log_traverse(struct STraverseParams *tp)
{
	char id[128], l[128], r[128];
	char k[4096] = "", v[4096] = "";
	srt_string **log = (srt_string **)tp->context;
	const srt_tnode *cn;
	if (tp->c == ST_NIL) {
		ss_cat_printf(log, 128, "\nLevel: %u\n", (unsigned)tp->level);
		return 0;
	}
	cn = get_node_r(tp->t, tp->c);
	switch (tp->t->d.sub_type) {
	case SM_II32:
		sprintf(k, FMT_I32, ((const struct SMapii *)cn)->x.k);
		sprintf(v, FMT_I32, ((const struct SMapii *)cn)->v);
		break;
	case SM_UU32:
		sprintf(k, FMT_U32, ((const struct SMapuu *)cn)->x.k);
		sprintf(v, FMT_U32, ((const struct SMapuu *)cn)->v);
		break;
	case SM_II:
	case SM_IS:
	case SM_IP:
		sprintf(k, FMT_I, ((const struct SMapI *)cn)->k);
		break;
	case SM_SI:
		sprintf(k, "%s",
			ss_to_c(sso_get((const srt_stringo *)&(
						(const struct SMapSI *)cn)
						->x.k)));
		break;
	case SM_SS:
	case SM_SP:
		sprintf(k, "%s",
			ss_to_c(sso_get(
				(const srt_stringo *)&((const struct SMapS *)cn)
					->k)));
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
			ss_to_c(sso_get((const srt_stringo *)&(
						(const struct SMapIS *)cn)
						->v)));
		break;
	case SM_IP:
		sprintf(k, "%p", (const void *)((const struct SMapIP *)cn)->v);
		break;
	case SM_SS:
		sprintf(k, "%s",
			ss_to_c(sso_get(&((const struct SMapSS *)cn)->s)));
		break;
	case SM_SP:
		sprintf(k, "%p", (const void *)((const struct SMapSP *)cn)->v);
		break;
	}
	ndx2s(id, sizeof(id), tp->c);
	ndx2s(l, sizeof(l), cn->x.l);
	ndx2s(r, sizeof(r), cn->r);
	ss_cat_printf(log, 128, "[%s: (%s, %s) -> (%s, %s; r:%u)] ", id, k, v,
		      l, r, cn->x.is_red);
	return 0;
}

void sm_log_obj(srt_string **log, const srt_map *m)
{
	ssize_t levels;
	if (!log)
		return;
	ss_cpy_c(log, "");
	levels = st_traverse_levelorder((const srt_tree *)m,
					(st_traverse)aux_sm_log_traverse, log);
	if (levels == 0)
		ss_cat_c(log, "empty map");
	else
		ss_cat_printf(log, 128, "\nlevels: %i, nodes: %u\n",
			      (int)levels, (unsigned)st_size(m));
	fprintf(stdout, "%s", ss_to_c(*log));
}

void shm_log_obj(srt_string **log, const srt_hmap *h)
{
	size_t es, i;
	const struct SHMBucket *b;
	const struct SHMapii *e;
	if (!log)
		return;
	ss_cpy_c(log, "");
	switch (h->d.sub_type) {
	case SHM0_II32:
	case SHM0_UU32:
		b = shm_get_buckets_r(h);
		e = (const struct SHMapii *)shm_get_buffer_r(h);
		ss_cat_printf(log, 128,
			      "hbits: %u, size: " FMT_ZU ", max_size: " FMT_ZU
			      "\n",
			      h->hbits, shm_size(h), shm_max_size(h));
		for (i = 0; i < (size_t)h->hmask + 1; i++) {
			ss_cat_printf(log, 128,
				      "b[" FMT_ZU
				      "] h: %08x "
				      "l: %u cnt: %u\n",
				      i, b[i].hash, b[i].loc, b[i].cnt);
		}
		es = shm_size(h);
		for (i = 0; i < es; i++)
			ss_cat_printf(log, 128, "e[" FMT_ZU "] kv: %u, %u\n", i,
				      e[i].x.k, e[i].v);
		break;
	default:
		ss_cpy_c(log, "[not implemented]");
		break;
	}
}

void s_hex_dump(srt_string **log, const char *label, const char *buf,
		size_t buf_size)
{
	srt_string *aux;
	if (!log)
		return;
	aux = ss_dup_cn(buf, buf_size);
	if (label)
		ss_cat_c(log, label);
	ss_cat_enc_hex(log, aux);
	ss_free(&aux);
}

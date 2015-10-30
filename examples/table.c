/*
 * table.c
 *
 * Table import/export between CSV/XML/JSON/URL formats *work in progress*
 *
 * CSV: http://tools.ietf.org/html/rfc4180
 *
 * WARNING: incomplete (work in progress)
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "../src/libsrt.h"

#define TABLE_BUF_SIZE 4096

#define DBG_TABLE

enum EncMode
{
	SENC_start_row,
	SENC_write_field,
	SENC_finish_row,
};

struct TableCodec;

typedef size_t (*f_dec2x)(const struct TableCodec *tcd, ss_t *line, ss_t **out, ss_t **tmp, ss_t **tmp2);
typedef void (*f_enc)(enum EncMode em, size_t nfields, ss_t **field_l, ss_t **field_r, ss_t **out);

struct TableCodec
{
	f_dec2x dec2x;
	f_enc enc;
};

static size_t csv2x(const struct TableCodec *tcd, ss_t *line, ss_t **out, ss_t **tmp, ss_t **tmp2);
static size_t xml2x(const struct TableCodec *tcd, ss_t *line, ss_t **out, ss_t **tmp, ss_t **tmp2);
static size_t json2x(const struct TableCodec *tcd, ss_t *line, ss_t **out, ss_t **tmp, ss_t **tmp2);
static size_t url2x(const struct TableCodec *tcd, ss_t *line, ss_t **out, ss_t **tmp, ss_t **tmp2);
static void enc_csv(enum EncMode em, size_t nfields, ss_t **field_l, ss_t **field_r, ss_t **out);
static void enc_xml(enum EncMode em, size_t nfields, ss_t **field_l, ss_t **field_r, ss_t **out);
static void enc_json(enum EncMode em, size_t nfields, ss_t **field_l, ss_t **field_r, ss_t **out);
static void enc_url(enum EncMode em, size_t nfields, ss_t **field_l, ss_t **field_r, ss_t **out);
static ssize_t process_row(const struct TableCodec *tcd, ss_t *line, ss_t **out, ss_t **tmp, ss_t **tmp2, sbool_t force_flush);

static int syntax_error(const char **argv, const int exit_code)
{
	const char *v0 = argv[0];
        fprintf(stderr, "[INCOMPLETE, work in progress] Table import/export (libsrt example). Returns: number of "
                "processed rows\nError [%i] Syntax: %s -c{c|x|j|u}{c|x|j|u}\n"
		"(where: c = csv, x = xml, j = json, u = url)\n"
		"Examples:\n"
		"%s -cc <in.csv >out.csv\n%s -cx <in.csv >out.xml\n"
		"%s -cj <in.csv >out.json\n%s -cu <in.csv >out.url\n",
		exit_code, v0, v0, v0, v0, v0);
	return exit_code;
}

int main(int argc, const char **argv)
{
	if (argc < 2 || strlen(argv[1]) != 3 || argv[1][0] != '-')
		return syntax_error(argv, 1);
	int exit_code = 0;
	size_t num_rows = 0;
	struct TableCodec tcd = { NULL, NULL };
	tcd.dec2x = argv[1][1] == 'c' ? csv2x : argv[1][1] == 'x' ? xml2x :
		    argv[1][1] == 'j' ? json2x : argv[1][1] == 'u' ? url2x : NULL;
	tcd.enc = argv[1][2] == 'c' ? enc_csv : argv[1][2] == 'x' ? enc_xml :
		  argv[1][2] == 'j' ? enc_json : argv[1][2] == 'u' ? enc_url : NULL;
	if (!tcd.dec2x || !tcd.enc)
		return syntax_error(argv, 2);
	int c = 0;
	ssize_t l, off, off2;
	char buf[TABLE_BUF_SIZE];
	sbool_t new_line = S_FALSE;
	ss_t *line = NULL, *out = NULL, *tmp = NULL, *tmp2 = NULL;
	ssize_t j;
	for (;;) {
		l = read(0, buf, TABLE_BUF_SIZE);
		if (l <= 0)
			break;
		for (off = 0; off < l;) {
			for (off2 = off; off2 < l; off2++) {
				if ((unsigned char)buf[off2] < 32) {
					if (buf[off2] == '\n')
						new_line = S_TRUE;
					break;
				}
			}
			ss_cat_cn(&line, buf + off, off2 - off);
			if (off2 == l)
				break;
			if (new_line) {
				if (process_row(&tcd, line, &out, &tmp, &tmp2, S_FALSE) < 0) {
					exit_code = 3;
					goto main_abort;
				}
				ss_set_size(line, 0);
			}
			off = off2 + 1;
		}
	}
	if (process_row(&tcd, line, &out, &tmp, &tmp2, S_TRUE))
		exit_code = 3;
main_abort:
	ss_free(&line, &tmp, &tmp2, &out);
	return exit_code;
}

static void enc_csv(enum EncMode em, size_t nfields, ss_t **field_l, ss_t **field_r, ss_t **out)
{
	switch (em) {
	case SENC_start_row:
		break;
	case SENC_write_field:
#ifdef DBG_TABLE
		fprintf(stderr, "enc_csv write_field: '%s', '%s'\n", ss_to_c(*field_l), ss_to_c(*field_r));
#endif
		if (nfields > 2) /* Not the first two pairs */
			ss_cat_cn(out, ",\"", 2);
		else
			ss_cat_cn(out, "\"", 1);
		ss_cat_enc_esc_dquote(out, ss_trim(field_l));
		if ((nfields % 2) == 0) { /* pair */
			ss_cat_cn(out, "\",\"", 3);
			ss_cat_enc_esc_dquote(out, ss_trim(field_r));
			ss_cat_cn(out, "\"", 1);
		} else { /* single field */
			ss_cat_cn(out, "\"", 1);
		}
		break;
	case SENC_finish_row:
		ss_cat_cn(out, "\n", 1);
		break;
	default:
		break;
	}
}

static void enc_xml(enum EncMode em, size_t nfields, ss_t **field_l, ss_t **field_r, ss_t **out)
{
	switch (em) {
	case SENC_start_row:
		ss_cat_c(out, "<row type=\"array\">");
		break;
	case SENC_write_field:
#ifdef DBG_TABLE
		fprintf(stderr, "enc_xml write_field: '%s', '%s'\n", ss_to_c(*field_l), ss_to_c(*field_r));
#endif
		ss_cat_cn(out, "<", 1);
		ss_cat_enc_esc_xml(out, ss_trim(field_l));
		ss_cat_cn(out, ">", 1);
		if ((nfields % 2) == 0)
			ss_cat_enc_esc_xml(out, ss_trim(field_r));
		ss_cat_cn(out, "</", 2);
		ss_cat(out, *field_l);
		ss_cat_cn(out, ">", 1);
		break;
	case SENC_finish_row:
		ss_cat_cn(out, "</row>\n", 7);
		break;
	default:
		break;
	}
}

static void enc_json(enum EncMode em, size_t nfields, ss_t **field_l, ss_t **field_r, ss_t **out)
{
	switch (em) {
	case SENC_start_row:
		ss_cat_c(out, "[");
		break;
	case SENC_write_field:
#ifdef DBG_TABLE
		fprintf(stderr, "enc_json write_field: '%s', '%s'\n", ss_to_c(*field_l), ss_to_c(*field_r));
#endif
		if (nfields > 2) /* Not the first two pairs */
			ss_cat_cn(out, ",\"", 2);
		else
			ss_cat_cn(out, "\"", 1);
		ss_cat_enc_esc_json(out, ss_trim(field_l));
		ss_cat_cn(out, "\":\"", 3);
		if ((nfields % 2) == 0)
			ss_cat_enc_esc_json(out, ss_trim(field_r));
		ss_cat_cn(out, "\"", 1);

		break;
	case SENC_finish_row:
		ss_cat_cn(out, "]\n", 2);
		break;
	default:
		break;
	}
}

static void enc_url(enum EncMode em, size_t nfields, ss_t **field_l, ss_t **field_r, ss_t **out)
{
	switch (em) {
	case SENC_start_row:
		break;
	case SENC_write_field:
#ifdef DBG_TABLE
		fprintf(stderr, "enc_url write_field: '%s', '%s'\n", ss_to_c(*field_l), ss_to_c(*field_r));
#endif
		ss_cat_cn(out, nfields > 2 ? "&" : "?", 1);
		ss_cat_enc_esc_url(out, ss_trim(field_l));
		ss_cat_cn(out, "=", 1);
		if ((nfields % 2) == 0)
			ss_cat_enc_esc_url(out, ss_trim(field_r));
		break;
	case SENC_finish_row:
		ss_cat_cn(out, "\n", 1);
		break;
	default:
		break;
	}
}

static size_t csv2x(const struct TableCodec *tcd, ss_t *line, ss_t **out, ss_t **tmp, ss_t **tmp2)
{
	size_t off = 0, nfield = 0, qo, co, qo2;
	ss_t *tq = ss_alloca(1), *tc = ss_alloca(1);
	ss_cpy_cn(&tq, "\"", 1);
	ss_cpy_cn(&tc, ",", 1);
	const char *l = ss_to_c(line);
	size_t ls = ss_size(line);
	size_t out_off0 = ss_size(*out);
	ss_set_size(*tmp, 0);
	ss_set_size(*tmp2, 0);
	ss_t ***field = &tmp;
	for (; off < ls;) {
		qo = ss_find(line, off, tq);
		co = ss_find(line, off, tc);
		if (co < qo) { /* No double quote */
			ss_cat_cn(*field, l + off, co - off);
			if ((nfield++ % 2) != 0) {
				tcd->enc(SENC_write_field, nfield, tmp, tmp2, out);
				ss_set_size(*tmp, 0);
				ss_set_size(*tmp2, 0);
				field = &tmp;
			} else {
				field = &tmp2;
			}
			off = s_size_t_add(co, 1, S_NPOS);
		} else if (qo < co) { /* Double quote start */
			ss_cat_cn(*field, l + off, qo - off);
			off = s_size_t_add(qo, 1, S_NPOS);
			for (; off < ls;) {
				qo2 = ss_find(line, off, tq);
				if (qo2 == off) { /* Two "" */
					ss_cat_cn(*field, l + off, 1);
					off = s_size_t_add(qo2, 1, S_NPOS);
					break;
				} else if (qo2 == S_NPOS) { /* BEHAVIOR: underflow */
					ss_cat_cn(*field, l + off, ls - off);
					off = ls;
					break;
				} else {
					/* 1) "...""..." vs 2) "..." */
					size_t inc = qo2 + 2 < ls &&
						     l[qo2 + 1] == '\"' ? 1 : 0;
					ss_cat_cn(*field, l + off, qo2 + inc - off);
					off = s_size_t_add(qo2, 1 + inc, S_NPOS);
					if (inc == 0) /* case (2) */
						break;
				}
			}
		} else {
			ss_cat_cn(*field, l + off, ls - off);
			/*off = ls;*/
			break;
		}
	}
	if (ss_size(**field) > 0)
		tcd->enc(SENC_write_field, ++nfield, tmp, tmp2, out);
	return nfield;
}

static size_t xml2x(const struct TableCodec *tcd, ss_t *line, ss_t **out, ss_t **tmp, ss_t **tmp2)
{
	return 0;
}

static size_t json2x(const struct TableCodec *tcd, ss_t *line, ss_t **out, ss_t **tmp, ss_t **tmp2)
{
	return 0;
}

static size_t url2x(const struct TableCodec *tcd, ss_t *line, ss_t **out, ss_t **tmp, ss_t **tmp2)
{
	return 0;
}

static ssize_t process_row(const struct TableCodec *tcd, ss_t *line, ss_t **out, ss_t **tmp, ss_t **tmp2, sbool_t force_flush)
{
#ifdef DBG_TABLE
	fprintf(stderr, "line: '%s'\n", ss_to_c(line));
#endif
	size_t written = 0, out_size_rollback = ss_size(*out);
	tcd->enc(SENC_start_row, 0, NULL, NULL, out);
	if (tcd->dec2x(tcd, line, out, tmp, tmp2)) {
		tcd->enc(SENC_finish_row, 0, NULL, NULL, out);
	} else {
		ss_set_size(*out, out_size_rollback);
	}
	size_t sso = ss_size(*out);
	if (sso > TABLE_BUF_SIZE || (sso > 0 && force_flush)) {
		written = write(1, ss_to_c(*out), ss_size(*out));
		ss_set_size(*out, 0);
	}
	return written;
}


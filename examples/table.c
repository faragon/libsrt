/*
 * table.c
 *
 * Table import/export between CSV/XML/JSON/URL formats *work in progress*
 *
 * CSV: http://tools.ietf.org/html/rfc4180
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "../src/libsrt.h"

#define TABLE_BUF_SIZE 4096

#define DBG_TABLE

enum EncMode
{
	SENC_none,
	SENC_csv,
	SENC_xml,
	SENC_json,
	SENC_url
};

static ssize_t process_row(ss_t *line, ss_t **out, ss_t **tmp, enum EncMode im, enum EncMode om, sbool_t force_flush);

static int syntax_error(const char **argv, const int exit_code)
{
	const char *v0 = argv[0];
        fprintf(stderr, "Table import/export (libsrt example). Returns: number of "
                "processed rows\nError [%i] Syntax: %s {-oc|-ox|-oj|-ou}\n"
		"Examples:\n"
		"%s -oc <in.csv >out.csv\n%s -ox <in.csv >out.xml\n"
		"%s -oj <in.csv >out.json\n%s -ou <in.csv >out.url\n",
		exit_code, v0, v0, v0, v0, v0);
	return exit_code;
}

int main(int argc, const char **argv)
{
	if (argc < 2)
		return syntax_error(argv, 1);
	int exit_code = 0;
	size_t num_rows = 0;
	enum EncMode im, om;
	im = om = SENC_none;
#if 1
	im = SENC_csv;
#endif
	if (!strncmp(argv[1], "-oc", 3))
		om = SENC_csv;
	else if (!strncmp(argv[1], "-ox", 3))
		om = SENC_xml;
	else if (!strncmp(argv[1], "-oj", 3))
		om = SENC_json;
	else if (!strncmp(argv[1], "-ou", 3))
		om = SENC_url;
	if (om == SENC_none)
		return syntax_error(argv, 2);
	int c = 0;
	ssize_t l, off, off2;
	char buf[TABLE_BUF_SIZE];
	sbool_t new_line = S_FALSE;
	ss_t *line = NULL, *out = NULL, *tmp = NULL;
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
				if (process_row(line, &out, &tmp, im, om, S_FALSE) < 0) {
					exit_code = 3;
					goto main_abort;
				}
				ss_set_size(line, 0);
			}
			off = off2 + 1;
		}
	}
	if (process_row(line, &out, &tmp, im, om, S_TRUE))
		exit_code = 3;
main_abort:
	ss_free(&line, &tmp, &out);
	return exit_code;
}

static void start_row(ss_t **out, enum EncMode om)
{
	switch (om) {
	case SENC_csv:	break;
	case SENC_xml: 	ss_cat_c(out, "<row type=\"array\">"); break;
	case SENC_json:	ss_cat_c(out, "["); break;
	case SENC_url:	break;
	default:	break;
	}
}

static void write_field(size_t nfield, ss_t **field, ss_t **out, enum EncMode om)
{
#ifdef DBG_TABLE
	fprintf(stderr, "write_field: '%s'\n", ss_to_c(*field));
#endif
	switch (om) {
	case SENC_csv:
		if (nfield > 0)
			ss_cat_cn(out, ",", 1);
		ss_cat_cn(out, "\"", 1);
		ss_cat_enc_esc_dquote(out, ss_trim(field));
		ss_cat_cn(out, "\"", 1);
		break;
	case SENC_xml: 	break;
	case SENC_json:	break;
	case SENC_url:	break;
	default:	break;
	}
	ss_set_size(*field, 0);
}

static void finish_row(ss_t **out, enum EncMode om)
{
	switch (om) {
	case SENC_csv:	break;
	case SENC_xml: 	ss_cat_c(out, "</row>"); break;
	case SENC_json:	ss_cat_c(out, "]"); break;
	case SENC_url:	break;
	default:	break;
	}
}

static sbool_t csv2x(ss_t *line, ss_t **out, ss_t **tmp, enum EncMode om)
{
	sbool_t res_ok = S_TRUE;
	size_t off = 0, nfield = 0, qo, co, qo2;
	ss_t *tq = ss_alloca(1), *tc = ss_alloca(1);
	ss_cpy_cn(&tq, "\"", 1);
	ss_cpy_cn(&tc, ",", 1);
	const char *l = ss_to_c(line);
	size_t ls = ss_size(line);
	size_t out_off0 = ss_size(*out);
	ss_set_size(*tmp, 0);
	for (; off < ls;) {
		qo = ss_find(line, off, tq);
		co = ss_find(line, off, tc);
		if (co < qo) { /* No double quote */
			ss_cat_cn(tmp, l + off, co - off);
			write_field(nfield++, tmp, out, om);
			off = s_size_t_add(co, 1, S_NPOS);
		} else if (qo < co) { /* Double quote start */
			ss_cat_cn(tmp, l + off, qo - off);
			off = s_size_t_add(qo, 1, S_NPOS);
			for (; off < ls;) {
				qo2 = ss_find(line, off, tq);
				if (qo2 == off) { /* Two "" */
					ss_cat_cn(tmp, l + off, 1);
					off = s_size_t_add(qo2, 1, S_NPOS);
					break;
				} else if (qo2 == S_NPOS) { /* BEHAVIOR: underflow */
					ss_cat_cn(tmp, l + off, ls - off);
					off = ls;
					break;
				} else {
					/* 1) "...""..." vs 2) "..." */
					size_t inc = qo2 + 2 < ls &&
						     l[qo2 + 1] == '\"' ? 1 : 0;
					ss_cat_cn(tmp, l + off, qo2 + inc - off);
					off = s_size_t_add(qo2, 1 + inc, S_NPOS);
					if (inc == 0) /* case (2) */
						break;
				}
			}
		} else {
			ss_cat_cn(tmp, l + off, ls - off);
			/*off = ls;*/
			break;
		}
	}
	if (ss_size(*tmp) > 0)
		write_field(nfield++, tmp, out, om);
	ss_cat_cn(out, "\n", 1);
	return res_ok;
}

static sbool_t xml2x(ss_t *line, ss_t **out, ss_t **tmp, enum EncMode om)
{
	return S_FALSE;
}

static sbool_t json2x(ss_t *line, ss_t **out, ss_t **tmp, enum EncMode om)
{
	return S_FALSE;
}

static sbool_t url2x(ss_t *line, ss_t **out, ss_t **tmp, enum EncMode om)
{
	return S_FALSE;
}

static ssize_t process_row(ss_t *line, ss_t **out, ss_t **tmp, enum EncMode im, enum EncMode om, sbool_t force_flush)
{
	ssize_t written = 0;
#if 1 /* TODO: work in progress, this is just a temporary echo test */
	fprintf(stderr, "line: '%s'\n", ss_to_c(line));
	start_row(out, om);
	switch (im) {
	case SENC_csv: csv2x(line, out, tmp, om); break;
	case SENC_xml: xml2x(line, out, tmp, om); break;
	case SENC_json: json2x(line, out, tmp, om); break;
	case SENC_url: url2x(line, out, tmp, om); break;
	default:
		break;
	}
	finish_row(out, om);
#else
	ss_cat(out, line);
#endif
	size_t sso = ss_size(*out);
	if (sso > TABLE_BUF_SIZE || (sso > 0 && force_flush)) {
		written = write(1, ss_to_c(*out), ss_size(*out));
		ss_set_size(*out, 0);
	}
	return written;
}


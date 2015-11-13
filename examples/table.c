/*
 * table.c
 *
 * Table import/export between CSV/HTML/JSON formats
 *
 * CSV: http://tools.ietf.org/html/rfc4180
 *
 * WARNING: incomplete (work in progress)
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "../src/libsrt.h"

#define DBG_TABLE 0

#if DBG_TABLE
	#define DBG_ENC_FIELD(type, field, nrow, nfield)		\
		fprintf(stderr, "enc %s: '%s' (%i, %i)\n",		\
			type, ss_to_c((ss_t *)field), nrow, nfield);
#else
	#define DBG_ENC_FIELD(type, field, nrow, nfield)
#endif

enum EncStep
{
	SENC_begin, SENC_field, SENC_end
};

typedef void (*f_enc)(enum EncStep em, const size_t nrow, const size_t nfield,
		      const ss_t *field, ss_t **out);
typedef ssize_t (*f_dec)(int in_fd, int out_fd, f_enc out_enc_f);

static void enc_csv(enum EncStep em, const size_t nrow, const size_t nfield,
		    const ss_t *field, ss_t **out);
static void enc_html(enum EncStep em, const size_t nrow, const size_t nfield,
		     const ss_t *field, ss_t **out);
static void enc_json(enum EncStep em, const size_t nrow, const size_t nfield,
		     const ss_t *field, ss_t **out);
static ssize_t csv2x(int in_fd, int out_fd, f_enc out_enc_f);
static ssize_t html2x(int in_fd, int out_fd, f_enc out_enc_f);
static ssize_t json2x(int in_fd, int out_fd, f_enc out_enc_f);
static int exit_msg(const char **argv, const char *msg, const int code);

int main(int argc, const char **argv)
{
	RETURN_IF(argc < 2 || strlen(argv[1]) != 3 || argv[1][0] != '-',
		  exit_msg(argv, "syntax error", 1));
	f_enc ef = argv[1][2] == 'c' ? enc_csv : argv[1][2] == 'h' ? enc_html :
		   argv[1][2] == 'j' ? enc_json : NULL;
	f_dec df = argv[1][1] == 'c' ? csv2x : argv[1][1] == 'h' ? html2x :
		   argv[1][1] == 'j' ? json2x : NULL;
	RETURN_IF(!ef || !df, exit_msg(argv, "syntax error", 2));
	RETURN_IF(df(0, 1, ef) < 0, exit_msg(argv, "input error", 3));
	return 0;
}

static int exit_msg(const char **argv, const char *msg, const int code)
{
	const char *v0 = argv[0];
        fprintf(stderr, "Table import/export (libsrt example). Returns: number"
		" of processed rows\nError [%i]: %s\nSyntax: %s -c{c|h|j}{c|h|"
		"j}\n(where: c = csv, h = html, j = json)\nExamples:\n%s -cc <"
		"in.csv >out.csv\n%s -ch <in.csv >out.html\n%s -cj <in.csv >ou"
		"t.json\nWORK IN PROGRESS: only -cc, -ch, -cj are available\n",
		code, msg, v0, v0, v0, v0);
	return code;
}

static void enc_csv(enum EncStep em, const size_t nrow, const size_t nfield,
		    const ss_t *field, ss_t **out)
{
	switch (em) {
	case SENC_field:
		DBG_ENC_FIELD("csv", field, nrow, nfield);
		if (nfield == 0 && nrow > 0)
			ss_cat_cn(out, "\n", 1);
		ss_cat_c(out, nfield > 0 ? ",\"" : "\"");
		ss_cat_enc_esc_dquote(out, field);
		ss_cat_cn(out, "\"", 1);
		break;
	case SENC_begin:
	case SENC_end:
	default:
		break;
	}
}

static void enc_html(enum EncStep em, const size_t nrow, const size_t nfield,
		     const ss_t *field, ss_t **out)
{
	switch (em) {
	case SENC_begin:
		ss_cat_cn(out, "<table>", 7);
		break;
	case SENC_field:
		DBG_ENC_FIELD("html", field, nrow, nfield);
		if (nfield == 0) {
			if (nrow > 0)	/* close previous row */
				ss_cat_cn(out, "</tr>", 5);
			ss_cat_cn(out, "<tr>", 4);
		}
		ss_cat_cn(out, "<td>", 4);
		ss_cat_enc_esc_xml(out, field);
		ss_cat_cn(out, "</td>", 5);
		break;
	case SENC_end:
		if (nrow > 0 || field > 0) /* close previous row */
			ss_cat_cn(out, "</tr>", 5);
		ss_cat_cn(out, "</table>", 8);
		break;
	default:
		break;
	}
}

static void enc_json(enum EncStep em, const size_t nrow, const size_t nfield,
		     const ss_t *field, ss_t **out)
{
	switch (em) {
	case SENC_begin:
		ss_cat_cn(out, "{", 1);
		break;
	case SENC_field:
		DBG_ENC_FIELD("json", field, nrow, nfield);
		if (nfield == 0) {
			if (nrow > 0) /* close previous row */
				ss_cat_cn(out, "],", 2);
			ss_cat_cn(out, "[\"", 2);
		} else {
			ss_cat_cn(out, ",\"", 2);
		}
		ss_cat_enc_esc_json(out, field);
		ss_cat_cn(out, "\"", 1);
		break;
	case SENC_end:
		if (nrow > 0 || field > 0) /* close previous row */
			ss_cat_cn(out, "]", 1);
		ss_cat_cn(out, "}", 1);
		break;
	default:
		break;
	}
}

static ssize_t csv2x(int in_fd, int out_fd, f_enc out_enc_f)
{
	RETURN_IF(in_fd < 0 || out_fd < 0 || !out_enc_f, -1);
	size_t l, nl, qo, co, off, nfield = 0, nrow = 0;
	const size_t rb_sz = 8192, wb_sz = 32768;
	sbool_t quote_opened = S_FALSE, quote_pending = S_FALSE;
	ss_t *rb = NULL, *wb = NULL, *field = NULL;
	out_enc_f(SENC_begin, nrow, nfield, field, &wb);
	for (; ss_read(&rb, 0, rb_sz) > 0;) {
		l = ss_size(rb);
		for (off = 0; off < l;) { /* process buffer */
			if (quote_pending) {
				quote_pending = S_FALSE;
				if (ss_at(rb, off) == '"') { /* double " */
					off++;
					continue;
				}
				quote_opened = S_TRUE;
			}
			qo = ss_findc(rb, off, '"');
			if (quote_opened) {
				if (qo == S_NPOS) {
					ss_cat_substr(&field, rb, off, S_NPOS);
					break; /* buffer empty */
				}
				ss_cat_substr(&field, rb, off, qo - off);
				off = qo + 1;
				quote_opened = S_FALSE;
				continue;
			}
			co = ss_findc(rb, off, ',');
			nl = ss_findc(rb, off, '\n');
			sbool_t coqo = co < qo, nlqo = nl < qo, nlco = nl < co,
				conl = co < nl;
			if (nlco && nlqo || conl && coqo) {
				size_t o2 = nlco && nlqo ? nl : co;
				ss_cat_substr(&field, rb, off, o2 - off);
				out_enc_f(SENC_field, nrow, nfield, field, &wb);
				nfield++;
				ss_set_size(field, 0);
				if (o2 == nl) {
					nfield = 0;
					nrow++;
				}
				if (ss_size(wb) > wb_sz &&
				    ss_write(out_fd, wb, 0, ss_size(wb)) < 0) {
					ss_set_size(wb, 0);
					nrow = -2; /* write error */
					goto csv2x_fatal_error;
				}
				off = o2 + 1;
				continue;
			}
			sbool_t qonl = qo < nl, qoco = qo < co;
			if (qonl && qoco) { /* qo nl (co) / qo co (nl) */
				ss_cat_substr(&field, rb, off, qo - off);
				off = qo + 1;
				quote_pending = S_TRUE;
				continue;
			}
			ss_cat_substr(&field, rb, off, S_NPOS);
			break;	/* buffer empty */
		}
	}
	if (ss_size(field) > 0) {
		out_enc_f(SENC_field, nrow, nfield, field, &wb);
		nrow++;
		nfield++;
	}
	out_enc_f(SENC_end, nrow, nfield, field, &wb);
	if (ss_size(wb) > 0 && ss_write(out_fd, wb, 0, ss_size(wb)) < 0)
		nrow = -2; /* write error */
csv2x_fatal_error:
	ss_free(&rb, &wb, &field);
	return nrow;
}

static ssize_t html2x(int in_fd, int out_fd, f_enc out_enc_f)
{
	return -1;
}

static ssize_t json2x(int in_fd, int out_fd, f_enc out_enc_f)
{
	return -1;
}


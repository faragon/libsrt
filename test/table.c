/*
 * table.c
 *
 * Table import/export between CSV/HTML/JSON formats example using libsrt
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 *
 * CSV (partial): http://tools.ietf.org/html/rfc4180
 *
 * Observations:
 * - Not optimized
 * - Not tested in depth
 */

#include "../src/libsrt.h"

#define DBG_TABLE 0

#if DBG_TABLE
#define DBG_ENC_FIELD(type, field, nrow, nfield)                               \
	fprintf(stderr, "enc %s: '%s' (%i, %i)\n", type, ss_to_c(field),       \
		(int)nrow, (int)nfield);
#else
#define DBG_ENC_FIELD(type, field, nrow, nfield)
#endif

#define RBUF_SIZE 8192
#define WBUF_SIZE (3 * RBUF_SIZE)

enum EncStep { SENC_begin, SENC_field, SENC_end };

typedef void (*f_enc)(enum EncStep em, size_t nrow, size_t nfield,
		      const srt_string *field, srt_string **out);
typedef ssize_t (*f_dec)(FILE *in_fd, FILE *out_fd, f_enc out_enc_f);

static void enc_csv(enum EncStep em, size_t nrow, size_t nfield,
		    const srt_string *field, srt_string **out);
static void enc_html(enum EncStep em, size_t nrow, size_t nfield,
		     const srt_string *field, srt_string **out);
static void enc_json(enum EncStep em, size_t nrow, size_t nfield,
		     const srt_string *field, srt_string **out);
static ssize_t csv2x(FILE *in_fd, FILE *out_fd, f_enc out_enc_f);
static ssize_t html2x(FILE *in_fd, FILE *out_fd, f_enc out_enc_f);
static ssize_t json2x(FILE *in_fd, FILE *out_fd, f_enc out_enc_f);
static int exit_msg(const char **argv, const char *msg, int code);

int main(int argc, const char **argv)
{
	f_enc ef;
	f_dec df;
	RETURN_IF(argc < 2 || strlen(argv[1]) != 3 || argv[1][0] != '-',
		  exit_msg(argv, "syntax error", 1));
	ef = argv[1][2] == 'c'
		     ? enc_csv
		     : argv[1][2] == 'h' ? enc_html
					 : argv[1][2] == 'j' ? enc_json : NULL;
	df = argv[1][1] == 'c'
		     ? csv2x
		     : argv[1][1] == 'h' ? html2x
					 : argv[1][1] == 'j' ? json2x : NULL;
	RETURN_IF(!ef || !df, exit_msg(argv, "syntax error", 1));
	RETURN_IF(df(stdin, stdout, ef) < 0, exit_msg(argv, "input error", 2));
	return 0;
}

static int exit_msg(const char **argv, const char *msg, int code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Table import/export (libsrt example). Returns: 0: OK,"
		" 1 syntax error, 2: input error\n\nError [%i]: %s\nSyntax: %s "
		"-{c|h|j}{c|h|j}\n(where: c = csv, h = html, j = json)\n\n"
		"Examples:\n"
		"%s -cc <in.csv >out.csv\n%s -ch <in.csv >out.html\n%s -cj "
		"<in.csv >out.json\n"
		"%s -hc <in.html >out.csv\n%s -hh <in.html >out.html\n%s -hj "
		"<in.html >out.json\n"
		"%s -jc <in.json >out.csv\n%s -jh <in.json >out.html\n%s -jj "
		"<in.json >out.json\n"
		"\n",
		code, msg, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0);
	return code;
}

static void enc_csv(enum EncStep em, size_t nrow, size_t nfield,
		    const srt_string *field, srt_string **out)
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
		break;
	}
}

#define HTML_TBL "<table>"
#define HTML_TBL_S 7
#define HTML_TBLX "</table>"
#define HTML_TBLX_S 8
#define HTML_TR "<tr>"
#define HTML_TR_S 4
#define HTML_TRX "</tr>"
#define HTML_TRX_S 5
#define HTML_TD "<td>"
#define HTML_TD_S 4
#define HTML_TDX "</td>"
#define HTML_TDX_S 5

static void enc_html(enum EncStep em, size_t nrow, size_t nfield,
		     const srt_string *field, srt_string **out)
{
	switch (em) {
	case SENC_begin:
		ss_cat_cn(out, HTML_TBL, HTML_TBL_S);
		break;
	case SENC_field:
		DBG_ENC_FIELD("html", field, nrow, nfield);
		if (nfield == 0) {
			if (nrow > 0) /* close previous row */
				ss_cat_cn(out, HTML_TRX, HTML_TRX_S);
			ss_cat_cn(out, HTML_TR, HTML_TR_S);
		}
		ss_cat_cn(out, HTML_TD, HTML_TD_S);
		ss_cat_enc_esc_xml(out, field);
		ss_cat_cn(out, HTML_TDX, HTML_TDX_S);
		break;
	case SENC_end:
		if (nrow > 0 || nfield > 0) /* close previous row */
			ss_cat_cn(out, HTML_TRX, HTML_TRX_S);
		ss_cat_cn(out, HTML_TBLX, HTML_TBLX_S);
		break;
	}
}

#define JS_OPEN "{"
#define JS_OPEN_S 1
#if 0 /* not used */
#define JS_OPENX "}"
#define JS_OPENX_S 1
#endif
#define JS_ARR "["
#define JS_ARR_S 1
#define JS_ARRX "]"
#define JS_ARRX_S 1
#define JS_COM ","
#define JS_COM_S 1
#define JS_ESC "\\"
#define JS_ESC_S 1
#define JS_QT "\""
#define JS_QT_S 1

static void enc_json(enum EncStep em, size_t nrow, size_t nfield,
		     const srt_string *field, srt_string **out)
{
	switch (em) {
	case SENC_begin:
		ss_cat_cn(out, "{[", 2);
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
		if (nrow > 0 || nfield > 0) /* close previous row */
			ss_cat_cn(out, "]", 1);
		ss_cat_cn(out, "]}", 2);
		break;
	}
}

static ssize_t csv2x(FILE *in_fd, FILE *out_fd, f_enc out_enc_f)
{
	size_t l, nl, qo, co, o, nfield = 0;
	ssize_t nrow = 0;
	srt_bool quote_opened = S_FALSE, quote_pending = S_FALSE,
		 fatal_error = S_FALSE;
	srt_bool coqo, conl, qonl, qoco, nlqo, nlco;
	srt_string *rb = NULL, *wb = NULL, *field = NULL;
	RETURN_IF(in_fd || out_fd || !out_enc_f, -1);
	out_enc_f(SENC_begin, (size_t)nrow, nfield, field, &wb);
	for (; ss_read(&rb, stdin, RBUF_SIZE) > 0;) {
		if (ss_size(wb) > WBUF_SIZE
		    && ss_write(out_fd, wb, 0, ss_size(wb)) < 0) {
			ss_set_size(wb, 0);
			nrow = -2; /* write error */
			fatal_error = S_TRUE;
			break;
		}
		l = ss_size(rb);
		for (o = 0; o < l;) { /* process buffer */
			if (quote_pending) {
				quote_pending = S_FALSE;
				if (ss_at(rb, o) == '"') { /* double " */
					o++;
					continue;
				}
				quote_opened = S_TRUE;
			}
			qo = ss_findc(rb, o, '"');
			if (quote_opened) {
				if (qo == S_NPOS) {
					ss_cat_substr(&field, rb, o, S_NPOS);
					break; /* buffer empty */
				}
				ss_cat_substr(&field, rb, o, qo - o);
				o = qo + 1;
				quote_opened = S_FALSE;
				continue;
			}
			co = ss_findc(rb, o, ',');
			nl = ss_findc(rb, o, '\n');
			coqo = co < qo;
			nlqo = nl < qo;
			nlco = nl < co;
			conl = co < nl;
			if ((nlco && nlqo) || (conl && coqo)) {
				size_t p = nlco && nlqo ? nl : co;
				ss_cat_substr(&field, rb, o, p - o);
				out_enc_f(SENC_field, (size_t)nrow, nfield,
					  field, &wb);
				nfield++;
				ss_set_size(field, 0);
				if (p == nl) {
					nfield = 0;
					nrow++;
				}
				o = p + 1;
				continue;
			}
			qonl = qo < nl;
			qoco = qo < co;
			if (qonl && qoco) { /* qo nl (co) / qo co (nl) */
				ss_cat_substr(&field, rb, o, qo - o);
				o = qo + 1;
				quote_pending = S_TRUE;
				continue;
			}
			ss_cat_substr(&field, rb, o, S_NPOS);
			break; /* buffer empty */
		}
	}
	if (!fatal_error) {
		if (ss_size(field) > 0) {
			out_enc_f(SENC_field, (size_t)nrow, nfield, field, &wb);
			nrow++;
			nfield++;
		}
		out_enc_f(SENC_end, (size_t)nrow, nfield, field, &wb);
		if (ss_size(wb) > 0 && ss_write(out_fd, wb, 0, ss_size(wb)) < 0)
			nrow = -2; /* write error */
	}
#ifdef S_USE_VA_ARGS
	ss_free(&rb, &wb, &field);
#else
	ss_free(&rb);
	ss_free(&wb);
	ss_free(&field);
#endif
	return nrow;
}

static srt_bool sflush(srt_string *wb, FILE *out_fd, size_t buf_size)
{
	size_t wbss = ss_size(wb);
	return wbss > buf_size && ss_write(out_fd, wb, 0, wbss) < 0
		       ? S_FALSE
		       :       /* write error */
		       S_TRUE; /* write OK */
}

static ssize_t html2x(FILE *in_fd, FILE *out_fd, f_enc out_enc_f)
{
	enum eHTFSM { HTSearchTable, HTSearchRow, HTSearchField, HTFillField };
	enum eHTFSM st = HTSearchTable;
	size_t o = 0, p, q, z, nfield = 0, nrow = 0, ss, ss0;
	srt_bool fatal_error = S_FALSE;
	srt_string *rb = NULL, *wb = NULL, *field = NULL;
	RETURN_IF(in_fd || out_fd || !out_enc_f, -1);
	out_enc_f(SENC_begin, nrow, nfield, field, &wb);
	for (;;) {
		ss_erase(&rb, 0, o);
		ss0 = ss_size(rb);
		ss = ss_size(ss_cat_read(&rb, stdin, RBUF_SIZE));
		if (ss == ss0)
			break; /* EOF */
		z = (ss < WBUF_SIZE - 10 ? ss : ss - 10);
		if (!sflush(wb, out_fd, WBUF_SIZE)) {
			fatal_error = S_TRUE;
			break; /* write error */
		}
		for (o = 0; o < ss;) { /* process buffer */
			switch (st) {
			case HTFillField:
				p = ss_findr_cn(rb, o, z, HTML_TDX, HTML_TDX_S);
				if (p == S_NPOS) {
					ss_cat_substr(&field, rb, o, S_NPOS);
					o = ss; /* buffer empty */
					break;
				}
				ss_cat_substr(&field, rb, o, p - o);
				out_enc_f(SENC_field, nrow, nfield, field, &wb);
				ss_set_size(field, 0);
				st = HTSearchField;
				o = p + HTML_TDX_S;
				nfield++;
				break;
			case HTSearchField:
				p = ss_findr_cn(rb, o, z, HTML_TD, HTML_TD_S);
				q = ss_findr_cn(rb, o, z, HTML_TR, HTML_TR_S);
				if (p == S_NPOS && q == S_NPOS) {
					o = ss;
					break; /* buffer empty */
				}
				if (q < p && (nfield || nrow)) {
					nrow++;
					nfield = 0;
				}
				st = HTFillField;
				o = p + HTML_TD_S;
				break;
			case HTSearchRow:
				p = ss_findr_cn(rb, o, z, HTML_TR, HTML_TR_S);
				if (p == S_NPOS) {
					o = ss;
					break; /* buffer empty */
				}
				if (nfield || nrow) {
					nrow++;
					nfield = 0;
				}
				st = HTSearchField;
				o = p + HTML_TR_S;
				break;
			case HTSearchTable:
				p = ss_findr_cn(rb, o, z, HTML_TBL, HTML_TBL_S);
				if (p == S_NPOS)
					break; /* buffer empty */
				st = HTSearchRow;
				o = p + HTML_TBL_S;
				break;
			}
		}
	}
	if (!fatal_error) {
		if (ss_size(field) > 0) {
			out_enc_f(SENC_field, nrow, nfield, field, &wb);
			nrow++;
			nfield++;
		}
		out_enc_f(SENC_end, nrow, nfield, field, &wb);
		if (!sflush(wb, out_fd, 0))
			fatal_error = S_TRUE;
	}
#ifdef S_USE_VA_ARGS
	ss_free(&rb, &wb, &field);
#else
	ss_free(&rb);
	ss_free(&wb);
	ss_free(&field);
#endif
	return fatal_error ? -2 : (ssize_t)nrow;
}

static ssize_t json2x(FILE *in_fd, FILE *out_fd, f_enc out_enc_f)
{
	enum eJSFSM {
		JSSearchOpen,
		JSSearchRow,
		JSSearchField,
		JSFillField,
		JSFillFieldQ
	};
	enum eJSFSM st = JSSearchOpen;
	size_t o = 0, p, q, r, s, nfield = 0, nrow = 0;
	srt_bool fatal_error = S_FALSE, done = S_FALSE, q_wins, r_wins;
	srt_string *rb = NULL, *wb = NULL, *field = NULL;
	size_t ss, ss0, z;
	size_t ox, next_o;
	RETURN_IF(!in_fd || !out_fd || !out_enc_f, -1);
	out_enc_f(SENC_begin, nrow, nfield, field, &wb);
	while (!done) {
		ss_erase(&rb, 0, o);
		ss0 = ss_size(rb);
		ss = ss_size(ss_cat_read(&rb, stdin, RBUF_SIZE));
		if (ss == ss0)
			break; /* EOF */
		z = (ss < WBUF_SIZE - 10 ? ss : ss - 10);
		if (!sflush(wb, out_fd, WBUF_SIZE)) {
			fatal_error = S_TRUE;
			break; /* write error */
		}
		for (o = 0; !done && o < ss;) { /* process buffer */
			switch (st) {
			case JSFillFieldQ:
				p = ss_findr_cn(rb, o, z, JS_QT, JS_QT_S);
				q = ss_findr_cn(rb, o, z, JS_COM, JS_COM_S);
				r = ss_findr_cn(rb, o, z, JS_ARRX, JS_ARRX_S);
				s = ss_findr_cn(rb, o, z, JS_ESC, JS_ESC_S);
				if (p == S_NPOS && q == S_NPOS && r == S_NPOS
				    && s == S_NPOS) {
					ss_cat_substr(&field, rb, o, S_NPOS);
					o = ss;
					break; /* buffer empty */
				}
				if (p < s) {
					ss_cat_substr(&field, rb, o, p - o);
					ss_dec_esc_json(&field, field);
					out_enc_f(SENC_field, nrow, nfield,
						  field, &wb);
					ss_set_size(field, 0);
					nfield++;
					if (p < q && q < r) {
						o = q + JS_COM_S;
						st = JSSearchField;
						break;
					} else if (p < r && r != S_NPOS) {
						o = r + JS_ARRX_S;
					} else
						o = p + JS_QT_S;
					st = JSSearchRow;
					break;
				}
				if (s != S_NPOS) {
					o = s + JS_ESC_S + 1;
					break;
				}
				/* this should not happen; to do: validate */
				o = ss;
				break;
			case JSFillField:
				q = ss_findr_cn(rb, o, z, JS_COM, JS_COM_S);
				r = ss_findr_cn(rb, o, z, JS_ARRX, JS_ARRX_S);
				s = ss_findr_cn(rb, o, z, JS_ESC, JS_ESC_S);
				if (q == S_NPOS && r == S_NPOS && s == S_NPOS) {
					ss_cat_substr(&field, rb, o, S_NPOS);
					o = ss; /* buffer empty */
					break;
				}
				q_wins = q < r && q < s;
				r_wins = r < q && r < s;
				if (q_wins || r_wins) {
					if (q_wins) {
						ox = q;
						st = JSSearchField;
						next_o = q + JS_COM_S;
					} else {
						ox = r;
						st = JSSearchRow;
						next_o = r + JS_ARRX_S;
					}
					ss_cat_substr(&field, rb, o, ox - o);
					ss_dec_esc_json(&field, field);
					out_enc_f(SENC_field, nrow, nfield,
						  field, &wb);
					ss_set_size(field, 0);
					nfield++;
					o = next_o;
					break;
				}
				if (s != S_NPOS) {
					o = s + JS_ESC_S + 1;
					break;
				}
				/* this should not happen; to do: validate */
				o = ss;
				break;
			case JSSearchField:
				p = ss_findr_cn(rb, o, z, JS_QT, JS_QT_S);
				q = ss_findr_cn(rb, o, z, JS_COM, JS_COM_S);
				r = ss_findr_cn(rb, o, z, JS_ARRX, JS_ARRX_S);
				if (p == S_NPOS && q == S_NPOS && r == S_NPOS) {
					break;
				}
				if (p < q && p < r) {
					o = p + JS_QT_S;
					st = JSFillFieldQ;
				} else {
					st = JSFillField;
					/* No oset changes */
				}
				break;
			case JSSearchRow:
				p = ss_findr_cn(rb, o, z, JS_ARR, JS_ARR_S);
				q = ss_findr_cn(rb, o, z, JS_ARRX, JS_ARRX_S);
				if (q < p) {
					/*s = ss;*/
					done = S_TRUE;
					break;
				}
				if (p == S_NPOS) {
					/*s = ss;*/ /* buffer empty */
					break;
				}
				if (nfield || nrow) {
					nrow++;
					nfield = 0;
				}
				st = JSSearchField;
				o = p + JS_ARR_S;
				break;
			case JSSearchOpen:
				p = ss_findr_cn(rb, o, z, JS_OPEN, JS_OPEN_S);
				if (p == S_NPOS) {
					/*s = ss;*/ /* buffer empty */
					break;
				}
				o = p + JS_OPEN_S;
				p = ss_findr_cn(rb, o, z, JS_ARR, JS_ARR_S);
				if (p == S_NPOS) {
					/*s = ss;*/ /* buffer empty */
					break;
				}
				st = JSSearchRow;
				o = p + JS_ARR_S;
				break;
			}
		}
	}
	if (!fatal_error) {
		if (ss_size(field) > 0) {
			ss_dec_esc_json(&field, field);
			out_enc_f(SENC_field, nrow, nfield, field, &wb);
			nrow++;
			nfield++;
		}
		out_enc_f(SENC_end, nrow, nfield, field, &wb);
		if (!sflush(wb, out_fd, 0))
			fatal_error = S_TRUE;
	}
#ifdef S_USE_VA_ARGS
	ss_free(&rb, &wb, &field);
#else
	ss_free(&rb);
	ss_free(&wb);
	ss_free(&field);
#endif
	return fatal_error ? -2 : (ssize_t)nrow;
}

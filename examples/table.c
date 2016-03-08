/*
 * table.c
 *
 * Table import/export between CSV/HTML/JSON formats
 *
 * CSV (partial): http://tools.ietf.org/html/rfc4180
 *
 * Observations:
 * - Not optimized
 * - Not tested
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "../src/libsrt.h"

#define DBG_TABLE 0

#if DBG_TABLE
	#define DBG_ENC_FIELD(type, field, nrow, nfield)		\
		fprintf(stderr, "enc %s: '%s' (%i, %i)\n",		\
			type, ss_to_c((ss_t *)field), (int)nrow, (int)nfield);
#else
	#define DBG_ENC_FIELD(type, field, nrow, nfield)
#endif

#define BUF_SIZE 8192

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
	RETURN_IF(!ef || !df, exit_msg(argv, "syntax error", 1));
	RETURN_IF(df(0, 1, ef) < 0, exit_msg(argv, "input error", 2));
	return 0;
}

static int exit_msg(const char **argv, const char *msg, const int code)
{
	const char *v0 = argv[0];
        fprintf(stderr, "Table import/export (libsrt example). Returns: 0: OK,"
		" 1 syntax error, 2: input error\nError [%i]: %s\nSyntax: %s "
		"-{c|h|j}{c|h|j}\n(where: c = csv, h = html, j = json)\n"
		"Examples:\n"
		"%s -cc <in.csv >out.csv\n%s -ch <in.csv >out.html\n%s -cj "
		"<in.csv >out.json\n"
		"%s -hc <in.html >out.csv\n%s -hh <in.html >out.html\n%s -hj "
		"<in.html >out.json\n"
		"%s -jc <in.json >out.csv\n%s -jh <in.json >out.html\n%s -jj "
		"<in.json >out.json\n""\n",
		code, msg, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0);
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

#define HTML_TABLE	"<table>"
#define HTML_TABLE_S	7
#define HTML_TABLEX	"</table>"
#define HTML_TABLEX_S	8
#define HTML_TR		"<tr>"
#define HTML_TR_S	4
#define HTML_TRX	"</tr>"
#define HTML_TRX_S	5
#define HTML_TD		"<td>"
#define HTML_TD_S	4
#define HTML_TDX	"</td>"
#define HTML_TDX_S	5

static void enc_html(enum EncStep em, const size_t nrow, const size_t nfield,
		     const ss_t *field, ss_t **out)
{
	switch (em) {
	case SENC_begin:
		ss_cat_cn(out, HTML_TABLE, HTML_TABLE_S);
		break;
	case SENC_field:
		DBG_ENC_FIELD("html", field, nrow, nfield);
		if (nfield == 0) {
			if (nrow > 0)	/* close previous row */
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
		ss_cat_cn(out, HTML_TABLEX, HTML_TABLEX_S);
		break;
	default:
		break;
	}
}

#define JSON_OPEN	"{"
#define JSON_OPEN_S	1
#define JSON_OPENX	"}"
#define JSON_OPENX_S	1
#define JSON_ARRAY	"["
#define JSON_ARRAY_S	1
#define JSON_ARRAYX	"]"
#define JSON_ARRAYX_S	1
#define JSON_COMMA	","
#define JSON_COMMA_S	1
#define JSON_ESC	"\\"
#define JSON_ESC_S	1
#define JSON_QUOTE	"\""
#define JSON_QUOTE_S	1

static void enc_json(enum EncStep em, const size_t nrow, const size_t nfield,
		     const ss_t *field, ss_t **out)
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
	default:
		break;
	}
}

static ssize_t csv2x(int in_fd, int out_fd, f_enc out_enc_f)
{
	RETURN_IF(in_fd < 0 || out_fd < 0 || !out_enc_f, -1);
	size_t l, nl, qo, co, off, nfield = 0, nrow = 0;
	const size_t rb_sz = 8192, wb_sz = 32768;
	sbool_t quote_opened = S_FALSE, quote_pending = S_FALSE,
		fatal_error = S_FALSE;
	ss_t *rb = NULL, *wb = NULL, *field = NULL;
	out_enc_f(SENC_begin, nrow, nfield, field, &wb);
	for (; ss_read(&rb, 0, rb_sz) > 0;) {
		if (ss_size(wb) > wb_sz &&
		    ss_write(out_fd, wb, 0, ss_size(wb)) < 0) {
			ss_set_size(wb, 0);
			nrow = -2; /* write error */
			fatal_error = S_TRUE;
			break;
		}
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
			if ((nlco && nlqo) || (conl && coqo)) {
				size_t o2 = nlco && nlqo ? nl : co;
				ss_cat_substr(&field, rb, off, o2 - off);
				out_enc_f(SENC_field, nrow, nfield, field, &wb);
				nfield++;
				ss_set_size(field, 0);
				if (o2 == nl) {
					nfield = 0;
					nrow++;
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
	if (!fatal_error) {
		if (ss_size(field) > 0) {
			out_enc_f(SENC_field, nrow, nfield, field, &wb);
			nrow++;
			nfield++;
		}
		out_enc_f(SENC_end, nrow, nfield, field, &wb);
		if (ss_size(wb) > 0 && ss_write(out_fd, wb, 0, ss_size(wb)) < 0)
			nrow = -2; /* write error */
	}
	ss_free(&rb, &wb, &field);
	return nrow;
}

size_t search_tag(ss_t **s, size_t off, int in_fd, const char *t,
		  const size_t ts)
{
	const size_t bs = S_MAX(BUF_SIZE, ts);
	for (;;) {
		off = ss_find_cn(*s, off, t, ts);
		RETURN_IF(off != S_NPOS, off);
		const size_t ss = ss_size(*s);
		ss_cat_read(s, in_fd, bs);
		const size_t ss2 = ss_size(*s);
		RETURN_IF(ss2 == ss, S_NPOS);   /* Not found at EOF */
		off = ss - ts - 1;
	}
}

static ssize_t html2x(int in_fd, int out_fd, f_enc out_enc_f)
{
	enum eHTFSM { HTSearchTable, HTSearchRow, HTSearchField, HTFillField };
	enum eHTFSM st = HTSearchTable;
	RETURN_IF(in_fd < 0 || out_fd < 0 || !out_enc_f, -1);
	size_t off, o2, o3, tr, td, nfield = 0, nrow = 0;
	sbool_t fatal_error = S_FALSE;
	const size_t rb_sz = 8192, wb_sz = 32768;
	ss_t *rb = NULL, *wb = NULL, *field = NULL;
	out_enc_f(SENC_begin, nrow, nfield, field, &wb);
	for (; ss_read(&rb, 0, rb_sz) > 0;) {
		if (ss_size(wb) > wb_sz &&
		    ss_write(out_fd, wb, 0, ss_size(wb)) < 0) {
			ss_set_size(wb, 0);
			nrow = -2; /* write error */
			fatal_error = S_TRUE;
			break;
		}
		for (off = 0; off < ss_size(rb);) { /* process buffer */
			if (st == HTFillField) {
				o2 = search_tag(&rb, off, in_fd, HTML_TDX,
								 HTML_TDX_S);
				if (o2 == S_NPOS) {
					ss_cat_substr(&field, rb, off, S_NPOS);
					break; /* buffer empty */
				}
				ss_cat_substr(&field, rb, off, o2 - off);
				out_enc_f(SENC_field, nrow, nfield, field, &wb);
				ss_set_size(field, 0);
				nfield++;
				st = HTSearchField;
				off = o2 + HTML_TDX_S;
				continue;
			}
			if (st == HTSearchField) {
				o2 = search_tag(&rb, off, in_fd, HTML_TD,
								 HTML_TD_S);
				o3 = search_tag(&rb, off, in_fd, HTML_TR,
								 HTML_TR_S);
				if (o2 == S_NPOS && o3 == S_NPOS)
					break; /* buffer empty */
				if (o3 < o2 && (nfield || nrow)) {
					nrow++;
					nfield = 0;
				}
				st = HTFillField;
				off = o2 + HTML_TD_S;
				continue;
			}
			if (st == HTSearchRow) {
				o2 = search_tag(&rb, off, in_fd, HTML_TR,
								 HTML_TR_S);
				if (o2 == S_NPOS)
					break; /* buffer empty */
				if (nfield || nrow){
					nrow++;
					nfield = 0;
				}
				st = HTSearchField;
				off = o2 + HTML_TR_S;
				continue;
			}
			if (st == HTSearchTable) {
				o2 = search_tag(&rb, off, in_fd, HTML_TABLE,
								 HTML_TABLE_S);
				if (o2 == S_NPOS)
					break; /* buffer empty */
				st = HTSearchRow;
				off = o2 + HTML_TABLE_S;
				continue;
			}
			break;
		}
	}
	if (!fatal_error) {
		if (ss_size(field) > 0) {
			out_enc_f(SENC_field, nrow, nfield, field, &wb);
			nrow++;
			nfield++;
		}
		out_enc_f(SENC_end, nrow, nfield, field, &wb);
		if (ss_size(wb) > 0 && ss_write(out_fd, wb, 0, ss_size(wb)) < 0)
			nrow = -2; /* write error */
	}
	ss_free(&rb, &wb, &field);
	return nrow;
}

static ssize_t json2x(int in_fd, int out_fd, f_enc out_enc_f)
{
	enum eJSFSM { JSSearchOpen, JSSearchRow, JSSearchField, JSFillField, JSFillFieldQ };
	enum eJSFSM st = JSSearchOpen;
	RETURN_IF(in_fd < 0 || out_fd < 0 || !out_enc_f, -1);
	size_t off, o2, o3, o4, o5, tr, td, nfield = 0, nrow = 0;
	const size_t rb_sz = 8192, wb_sz = 32768;
	sbool_t fatal_error = S_FALSE;
	ss_t *rb = NULL, *wb = NULL, *field = NULL;
	out_enc_f(SENC_begin, nrow, nfield, field, &wb);
	for (; ss_read(&rb, 0, rb_sz) > 0;) {
		if (ss_size(wb) > wb_sz &&
		    ss_write(out_fd, wb, 0, ss_size(wb)) < 0) {
			ss_set_size(wb, 0);
			nrow = -2; /* write error */
			fatal_error = S_TRUE;
			break;
		}
		for (off = 0; off < ss_size(rb);) { /* process buffer */
			if (st == JSFillFieldQ) {
				o2 = search_tag(&rb, off, in_fd, JSON_QUOTE,
								 JSON_QUOTE_S);
				o3 = search_tag(&rb, off, in_fd, JSON_COMMA,
								 JSON_COMMA_S);
				o4 = search_tag(&rb, off, in_fd, JSON_ARRAYX,
								 JSON_ARRAYX_S);
				o5 = search_tag(&rb, off, in_fd, JSON_ESC,
								 JSON_ESC_S);
				if (o2 == S_NPOS && o3 == S_NPOS &&
				    o4 == S_NPOS && o5 == S_NPOS) {
					ss_cat_substr(&field, rb, off, S_NPOS);
					break; /* buffer empty */
				}
				if (o2 < o5) {
					ss_cat_substr(&field, rb, off, o2 - off);
					ss_dec_esc_json(&field, field);
					out_enc_f(SENC_field, nrow, nfield, field, &wb);
					ss_set_size(field, 0);
					nfield++;
					if (o2 < o3 && o3 < o4) {
						off = o3 + JSON_COMMA_S;
						st = JSSearchField;
						continue;
					} else if (o2 < o4 && o4 != S_NPOS) {
						off = o4 + JSON_ARRAYX_S;
					} else
						off = o2 + JSON_QUOTE_S;
					st = JSSearchRow;
					continue;
				}
				if (o5 != S_NPOS) {
					search_tag(&rb, o5 + JSON_ESC_S, in_fd,
						   JSON_ESC, JSON_ESC_S);
					ss_cat_substr(&field, rb, off, o5 - off + 1);
					off = o5 + JSON_ESC_S;
					continue;
				}
				break;
			}
			if (st == JSFillField) {
				o3 = search_tag(&rb, off, in_fd, JSON_COMMA,
								 JSON_COMMA_S);
				o4 = search_tag(&rb, off, in_fd, JSON_ARRAYX,
								 JSON_ARRAYX_S);
				o5 = search_tag(&rb, off, in_fd, JSON_ESC,
								 JSON_ESC_S);
				if (o3 == S_NPOS && o4 == S_NPOS && o5 == S_NPOS) {
					ss_cat_substr(&field, rb, off, S_NPOS);
					break; /* buffer empty */
				}
				sbool_t o3_wins = o3 < o4 && o3 < o5,
					o4_wins = o4 < o3 && o4 < o5;
				if (o3_wins || o4_wins) {
					size_t ox, next_off;
					if (o3_wins) {
						ox = o3;
						st = JSSearchField;
						next_off = o3 + JSON_COMMA_S;
					} else {
						ox = o4;
						st = JSSearchRow;
						next_off = o4 + JSON_ARRAYX_S;
					}
					ss_cat_substr(&field, rb, off, ox - off);
					ss_dec_esc_json(&field, field);
					out_enc_f(SENC_field, nrow, nfield, field, &wb);
					ss_set_size(field, 0);
					nfield++;
					off = next_off;
					continue;
				}
				if (o5 != S_NPOS) {
					search_tag(&rb, o5 + JSON_ESC_S, in_fd,
						   JSON_ESC, JSON_ESC_S);
					ss_cat_substr(&field, rb, off, o5 - off + 1);
					off = o5 + JSON_ESC_S;
					continue;
				}
				break;
			}
			if (st == JSSearchField) {
				o2 = search_tag(&rb, off, in_fd, JSON_QUOTE,
								 JSON_QUOTE_S);
				o3 = search_tag(&rb, off, in_fd, JSON_COMMA,
								 JSON_COMMA_S);
				o4 = search_tag(&rb, off, in_fd, JSON_ARRAYX,
								 JSON_ARRAYX_S);
				if (o2 == S_NPOS && o3 == S_NPOS && o4 == S_NPOS)
					break; /* buffer empty */
				if (o2 < o3 && o2 < o4) {
					off = o2 + JSON_QUOTE_S;
					st = JSFillFieldQ;
				} else {
					st = JSFillField;
					/* No offset changes */
				}
				continue;
			}
			if (st == JSSearchRow) {
				o2 = search_tag(&rb, off, in_fd, JSON_ARRAY,
								 JSON_ARRAY_S);
				if (o2 == S_NPOS)
					break; /* buffer empty */
				if (nfield || nrow){
					nrow++;
					nfield = 0;
				}
				st = JSSearchField;
				off = o2 + JSON_ARRAY_S;
				continue;
			}
			if (st == JSSearchOpen) {
				o2 = search_tag(&rb, off, in_fd, JSON_OPEN,
								 JSON_OPEN_S);
				if (o2 == S_NPOS)
					break; /* EOF */
				off = o2 + JSON_OPEN_S;
				o2 = search_tag(&rb, off, in_fd, JSON_ARRAY,
								 JSON_ARRAY_S);
				if (o2 == S_NPOS)
					break; /* EOF */
				st = JSSearchRow;
				off = o2 + JSON_ARRAY_S;
				continue;
			}
			break;
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
		if (ss_size(wb) > 0 && ss_write(out_fd, wb, 0, ss_size(wb)) < 0)
			nrow = -2; /* write error */
	}
	ss_free(&rb, &wb, &field);
	return nrow;
}


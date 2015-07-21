/*
 * senc.c
 *
 * Buffer encoding/decoding
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "senc.h"
#include "sbitio.h"
#include <stdlib.h>

#define SLZW_ENABLE_RLE		1
#define SLZW_USE_STOP_CODE	0
#define SLZW_DEBUG		0

/*
 * Constants
 */

static const unsigned char b64e[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b',
	'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
	'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
	};
static const unsigned char b64d[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0,
	0, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 0, 0, 0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 32, 33,
	34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
	51, 0, 0, 0, 0, 0
	};
static const unsigned char n2h_l[16] = {
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 97, 98, 99, 100, 101, 102
	};
static const unsigned char n2h_u[16] = {
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 65, 66, 67, 68, 69, 70
	};
static const unsigned char h2n[64] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

#define S_BS	0x08
#define S_TAB	0x09
#define S_NL	0x0a
#define S_FEED	0x0c
#define S_CR	0x0d
#define S_QUOT	0x22
#define S_AMP	0x26
#define S_APOS	0x27
#define S_LT	0x3c
#define S_GT	0x3e
#define S_BSL	0x5c
#define S_QUOT_SZ	6
#define S_AMP_SZ	5
#define S_APOS_SZ	6
#define S_LT_SZ		4
#define S_GT_SZ		4

#define SLZW_MAX_TREE_BITS	12
#define SLZW_CODE_LIMIT		(1 << SLZW_MAX_TREE_BITS)
#define SLZW_MAX_CODE		(SLZW_CODE_LIMIT - 1)
#define SLZW_ROOT_NODE_BITS	8
#define SLZW_OP_START		(1 << SLZW_ROOT_NODE_BITS)
#define SLZW_RESET		SLZW_OP_START
#define SLZW_STOP		(SLZW_RESET + 1)
#if SLZW_ENABLE_RLE
	#define SLZW_RLE1	(SLZW_STOP + 1)
	#define SLZW_RLE3       (SLZW_RLE1 + 1)
	#define SLZW_RLE4       (SLZW_RLE3 + 1)
	#define SLZW_RLE_CSIZE	16
	#define SLZW_RLE_BITSD2 9
	#define SLZW_OP_END	SLZW_RLE4
#else
	#define SLZW_OP_END	SLZW_STOP
#endif
#define SLZW_FIRST		(SLZW_OP_END + 1)
#define SLZW_LUT_CHILD_ELEMS	256
#define SLZW_MAX_LUTS		(SLZW_CODE_LIMIT / 8)
#define SLZW_ESC_RATIO		(1 << (17 - SLZW_MAX_TREE_BITS))

#define SRLE_OP_MASK_SHORT	0xe0
#define SRLE_RUN_MASK_SHORT	0x1f
#define SRLE_OP_ST_SHORT	0x80
#define SRLE_OP_RLE4_SHORT	0x40
#define SRLE_OP_RLE3_SHORT	0x20
#define SRLE_OP_ST		0x08
#define SRLE_OP_RLE4		0x04
#define SRLE_OP_RLE3		0x02
#define SRLE_OP_RLE1		0x01

/*
 * Macros
 */

#define EB64C1(a)	(a >> 2)
#define EB64C2(a, b)	((a & 3) << 4 | b >> 4)
#define EB64C3(b, c)	((b & 0xf) << 2 | c >> 6)
#define EB64C4(c)	(c & 0x3f)
#define DB64C1(a, b)	(a << 2 | b >> 4)
#define DB64C2(b, c)	(b << 4 | c >> 2)
#define DB64C3(c, d)	(c << 6 | d)

#define SLZW_ENC_RESET(node_lutref, lut_stack_in_use,	\
		       node_stack_in_use, next_code, curr_code_len) {	\
		memset(node_lutref, 0, 256 * sizeof(node_lutref[0]));	\
		lut_stack_in_use = 1;					\
		node_stack_in_use = 256;				\
		curr_code_len = SLZW_ROOT_NODE_BITS + 1;		\
		next_code = SLZW_FIRST;					\
	}

#define SLZW_DEC_RESET(curr_code_len, last_code, next_inc_code, parents) { \
		int j;							   \
		curr_code_len = SLZW_ROOT_NODE_BITS + 1;		   \
		last_code = SLZW_CODE_LIMIT;				   \
		next_inc_code = SLZW_FIRST;				   \
		for (j = 0; j < 256; j += 4) 				   \
			parents[j] = parents[j + 1] = parents[j + 2] =	   \
				     parents[j + 3] = SLZW_CODE_LIMIT;	   \
	}

/*
 * Internal functions
 */

static int hex2nibble(const int h)
{
	return h2n[(h - 48) & 0x3f];
}

static size_t senc_hex_aux(const unsigned char *s, const size_t ss,
			   unsigned char *o, const unsigned char *t)
{
	RETURN_IF(!s, ss * 2);
	ASSERT_RETURN_IF(!o, 0);
	const size_t out_size = ss * 2;
	size_t i = ss, j = out_size;
	size_t tail = ss - (ss % 2);
	#define ENCHEX_LOOP(ox, ix) {		\
		const int next = s[ix - 1];	\
		o[ox - 2] = t[next >> 4];	\
		o[ox - 1] = t[next & 0x0f];	\
		}
	if (tail)
		for (; i > 0; i--, j -= 2)
			ENCHEX_LOOP(j, i);
	for (; i > 0; i -= 2, j -= 4) {
		ENCHEX_LOOP(j - 2, i - 1);
		ENCHEX_LOOP(j, i);
	}
	#undef ENCHEX_LOOP
	return out_size;
}

static void slzw_setseq256s8(unsigned *p)
{
	unsigned j, acc = S_HTON_U32(0x00010203);
	for (j = 0; j < 256 / 4; j++) {
		p[j] = acc;
		acc += 0x04040404;
	}
}

static sbool_t slzw_short_codes(size_t normal_count, size_t esc_count)
{
	return (SLZW_ESC_RATIO &&
		esc_count <= normal_count / SLZW_ESC_RATIO) ?
		S_TRUE : S_FALSE;
}

static size_t slzw_bitio_read(const unsigned char *b, size_t *i, size_t *acc,
			      size_t *accbuf, size_t cbits,
			      size_t *normal_count, size_t *esc_count)
{
	size_t c;
	if (slzw_short_codes(*normal_count, *esc_count)) {
		c = sbitio_read(b, i, acc, accbuf, 8);
		if (c == 255) {
			c = sbitio_read(b, i, acc, accbuf, cbits);
			(*esc_count)++;
		} else {
			(*normal_count)++;
		}
	} else {
		c = sbitio_read(b, i, acc, accbuf, cbits);
		(*normal_count)++;
	}
	return c;
}

static void slzw_bitio_write(unsigned char *b, size_t *i, size_t *acc,
			     size_t c, size_t cbits, size_t *normal_count,
			     size_t *esc_count)
{
	if (slzw_short_codes(*normal_count, *esc_count)) {
		if (c < 255) {
	                sbitio_write(b, i, acc, c, 8);
			(*normal_count)++;
#if SLZW_DEBUG
			fprintf(stderr, "%u (8 (%u))\n",
				(unsigned)c, (unsigned)cbits);
#endif
		} else {
	                sbitio_write(b, i, acc, 255, 8);
			sbitio_write(b, i, acc, c, cbits);
			(*esc_count)++;
#if SLZW_DEBUG
			fprintf(stderr, "%u (prefix + %u)\n",
				(unsigned)c, (unsigned)cbits);
#endif
		}
	} else {
		sbitio_write(b, i, acc, c, cbits);
		(*normal_count)++;
#if SLZW_DEBUG
		fprintf(stderr, "%u (%u)\n", (unsigned)c, (unsigned)cbits);
#endif
	}
}

static size_t srle_run(const unsigned char *s, size_t i, size_t ss,
		       size_t min_run, size_t max_run, size_t *run_elem_size,
		       sbool_t lzw_mode)
{
	RETURN_IF(i + min_run >= ss, 0);
	const suint32_t *p0 = (const suint32_t *)(s + i),
			*p3 = (const suint32_t *)(s + i + 3);
	suint32_t p00 = S_LD_U32(p0), p01 = S_LD_U32(p0 + 1),
		  p30 = S_LD_U32(p3 + 0);
	size_t run_length = 0, eq4 = p00 == p01,
	       eq3 = !eq4 && p00 == p30;
	for (; eq4 || eq3;) {
		size_t j, ss2;
		if (eq4) {
			ss2 = S_MIN(ss, max_run) - sizeof(wide_cmp_t);
			wide_cmp_t u0 = S_LD_UW(s + i),
				   v0 = S_LD_UW(s + i + 1);
			j = i + sizeof(suint32_t) * 2;
			if (u0 == v0) {
				j &= S_ALIGNMASK;
				*run_elem_size = 1;
			} else {
				*run_elem_size = 4;
			}
			for (; j < ss2 ; j += sizeof(wide_cmp_t))
				if (u0 != S_LD_UW(s + j))
					break;
		} else {
			ss2 = S_MIN(ss, max_run) - sizeof(suint32_t);
			suint32_t p31 = S_LD_U32(p3 + 1),
				  p32 = S_LD_U32(p3 + 2),
				  p02 = S_LD_U32(p0 + 2);
			if (p01 != p31 || p02 != p32)
				break;
			j = i + 3 * 4;
			*run_elem_size = 3;
			for (; j < ss2 ; j += 3)
				if (p00 != S_LD_U32(s + j))
					break;
		}
		if (j - i >= min_run)
			run_length = (j - i - (lzw_mode ? *run_elem_size : 0)) /
				     *run_elem_size;
		break;
	}
	return run_length;
}

/*
 * Base64 encoding/decoding
 */

size_t senc_b64(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s, (ss / 3 + (ss % 3 ? 1 : 0)) * 4);
	ASSERT_RETURN_IF(!o, 0);
	const size_t ssod4 = (ss / 3) * 4;
	const size_t ssd3 = ss - (ss % 3);
	const size_t tail = ss - ssd3;
	size_t i = ssd3, j = ssod4 + (tail ? 4 : 0);
	const size_t out_size = j;
	unsigned si0, si1, si2;
	switch (tail) {
	case 2: si0 = s[ssd3], si1 = s[ssd3 + 1];
		o[j - 4] = b64e[EB64C1(si0)];
		o[j - 3] = b64e[EB64C2(si0, si1)];
		o[j - 2] = b64e[EB64C3(si1, 0)];
		o[j - 1] = '=';
		j -= 4;
		break;
	case 1: si0 = s[ssd3];
		o[j - 4] = b64e[EB64C1(si0)];
		o[j - 3] = b64e[EB64C2(si0, 0)];
		o[j - 2] = '=';
		o[j - 1] = '=';
		j -= 4;
	}
	for (; i > 0; i -= 3, j -= 4) {
		si0 = s[i - 3], si1 = s[i - 2], si2 = s[i - 1];
		o[j - 4] = b64e[EB64C1(si0)];
		o[j - 3] = b64e[EB64C2(si0, si1)];
		o[j - 2] = b64e[EB64C3(si1, si2)];
		o[j - 1] = b64e[EB64C4(si2)];
	}
	return out_size;
}

size_t sdec_b64(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s, (ss / 4) * 3);
	ASSERT_RETURN_IF(!o, 0);
	size_t i = 0, j = 0;
	const size_t ssd4 = ss - (ss % 4);
	const size_t tail = s[ss - 2] == '=' || s[ss - 1] == '=' ? 4 : 0;
	for (; i  < ssd4 - tail; i += 4, j += 3) {
		const int a = b64d[s[i]], b = b64d[s[i + 1]],
			  c = b64d[s[i + 2]], d = b64d[s[i + 3]];
		o[j] = DB64C1(a, b);
		o[j + 1] = DB64C2(b, c);
		o[j + 2] = DB64C3(c, d);
	}
	if (tail) {
		const int a = b64d[s[i] & 0x7f], b = b64d[s[i + 1] & 0x7f];
		o[j++] = DB64C1(a, b);
		if (s[i + 2] != '=') {
			const int c = b64d[s[i + 2] & 0x7f];
			o[j++] = DB64C2(b, c);
		}
	}
	return j;
}

/*
 * Hexadecimal encoding/decoding
 */

size_t senc_hex(const unsigned char *s, const size_t ss, unsigned char *o)
{
	return senc_hex_aux(s, ss, o, n2h_l);
}

size_t senc_HEX(const unsigned char *s, const size_t ss, unsigned char *o)
{
	return senc_hex_aux(s, ss, o, n2h_u);
}

size_t sdec_hex(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s, ss / 2);
	ASSERT_RETURN_IF(!o, 0);
	const size_t ssd2 = ss - (ss % 2),
		     ssd4 = ss - (ss % 4);
	ASSERT_RETURN_IF(!ssd2, 0);
	size_t i = 0, j = 0;
	#define SDEC_HEX_L(n, m)	\
		o[j + n] = (hex2nibble(s[i + m]) << 4) | \
			    hex2nibble(s[i + m + 1]);
	for (; i < ssd4; i += 4, j += 2) {
		SDEC_HEX_L(0, 0);
		SDEC_HEX_L(1, 2);
	}
	for (; i < ssd2; i += 2, j += 1)
		SDEC_HEX_L(0, 0);
	#undef SDEC_HEX_L
	return j;
}

size_t senc_esc_xml_req_size(const unsigned char *s, const size_t ss)
{
	size_t i = 0, sso = ss;
	for (; i < ss; i++)
		switch (s[i]) {
		case S_QUOT: sso += S_QUOT_SZ - 1; break;
		case S_AMP: sso += S_AMP_SZ - 1; break;
		case S_APOS: sso += S_APOS_SZ - 1; break;
		case S_LT: sso += S_LT_SZ - 1; break;
		case S_GT: sso += S_GT_SZ - 1; break;
		default:
			break;
		}
	return sso;
}

size_t senc_esc_json_req_size(const unsigned char *s, const size_t ss)
{
	size_t i = 0, sso = ss;
	for (; i < ss; i++)
		switch (s[i]) {
		case S_BS: case S_TAB: case S_NL: case S_FEED: case S_CR:
		case S_QUOT: case S_BSL:
			sso++;
			break;
		default:
			break;
		}
	return sso;
}

size_t senc_esc_xml(const unsigned char *s, const size_t ss, unsigned char *o,
		    const size_t known_sso)
{
	RETURN_IF(!s || !ss || !o, 0);
	size_t csz, sso = known_sso ? known_sso : senc_esc_xml_req_size(s, ss);
	ssize_t i = ss - 1, j = sso;
	for (; i >= 0; i--) {
		switch (s[i]) {
		case S_QUOT: j -= 6; s_memcpy6(o + j, "&quot;"); break;
		case S_AMP: j -= 5; s_memcpy5(o + j, "&amp;"); break;
		case S_APOS: j -= 6; s_memcpy6(o + j, "&apos;"); break;
		case S_LT: j -= 4; s_memcpy4(o + j, "&lt;"); break;
		case S_GT: j -= 4; s_memcpy4(o + j, "&gt;"); break;
		default:
			o[--j] = s[i];
		}
	}
	return sso;
}

size_t sdec_esc_xml(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s || !ss || !o, 0);
	size_t i = 0, j = 0;
	for (; i < ss; j++) {
		if (s[i] == '&') {
			switch (s[i + 1]) {
			case 'q':
				if (i + 5 <= ss &&
				    s[i + 2] == 'u' && s[i + 3] == 'o' &&
				    s[i + 4] == 't' && s[i + 5] == ';') {
					o[j] = '"';
					i += 6;
					continue;
				}
				break;
			case 'a':
				if (i + 4 <= ss &&
				    s[i + 2] == 'm' && s[i + 3] == 'p' &&
				    s[i + 4] == ';') {
					o[j] = '&';
					i += 5;
					continue;
				}
				if (i + 5 <= ss &&
				    s[i + 2] == 'p' && s[i + 3] == 'o' &&
				    s[i + 4] == 's' && s[i + 5] == ';') {
					o[j] = '\'';
					i += 6;
					continue;
				}
				break;
			case 'l':
				if (i + 3 <= ss &&
				    s[i + 2] == 't' && s[i + 3] == ';') {
					o[j] = '<';
					i += 4;
					continue;
				}
				break;
			case 'g':
				if (i + 3 <= ss &&
				    s[i + 2] == 't' && s[i + 3] == ';') {
					o[j] = '>';
					i += 4;
					continue;
				}
				break;
#if 0	/* BEHAVIOR: not implemented (on purpose) */
			case '#':
				break;
#endif
			default:
				break;
			}
		}
		o[j] = s[i++];
	}
	return j;
}

/* BEHAVIOR: slash ('/') is not escaped (intentional) */
size_t senc_esc_json(const unsigned char *s, const size_t ss, unsigned char *o,
		     const size_t known_sso)
{
	RETURN_IF(!s || !ss || !o, 0);
	size_t sso = known_sso ? known_sso : senc_esc_json_req_size(s, ss);
	ssize_t i = ss - 1, j = sso;
	for (; i >= 0; i--) {
		switch (s[i]) {
		case S_BS: j -= 2; s_memcpy2(o + j, "\\b"); break;
		case S_TAB: j -= 2; s_memcpy2(o + j, "\\t"); break;
		case S_NL: j -= 2; s_memcpy2(o + j, "\\n"); break;
		case S_FEED: j -= 2; s_memcpy2(o + j, "\\f"); break;
		case S_CR: j -= 2; s_memcpy2(o + j, "\\r"); break;
		case S_QUOT: j -= 2; s_memcpy2(o + j, "\\\""); break;
		case S_BSL: j -= 2; s_memcpy2(o + j, "\\\\"); break;
		default:
			o[--j] = s[i];
		}
	}
	return sso;
}

size_t sdec_esc_json(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s || !ss || !o, 0);
	size_t i = 0, j = 0;
	for (; i < ss; j++) {
		if (s[i] == '\\' && i + 1 <= ss) {
			switch (s[i + 1]) {
			case 'b': o[j] = 8; i += 2; continue;
			case 't': o[j] = 9; i += 2; continue;
			case 'n': o[j] = 10; i += 2; continue;
			case 'f': o[j] = 12; i += 2; continue;
			case 'r': o[j] = 13; i += 2; continue;
			case '"':
			case '\\':
			case '/': o[j] = s[i + 1]; i += 2; continue;
#if 0	/* BEHAVIOR: not implemented (on purpose) */
			case 'u': break;
#endif
			default: break;
			}
		}
		o[j] = s[i++];
	}
	return j;
}

/*
 * LZW encoding/decoding
 */

typedef short slzw_ndx_t;

size_t senc_lzw(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s || !o || !ss, 0);
	/*
	 * Node structure (separated elements and not a "struct", in order to
	 * save space because of data alignment)
	 *
	 * node_codes[i]: LZW output code for the node
	 * node_lutref[i]: 0: empty, < 0: -next_node, > 0: 256-child LUT ref.
	 * node_child[i]: if node_lutref[0] < 0: next node byte (one-child node)
	 */
        slzw_ndx_t node_codes[SLZW_CODE_LIMIT], node_lutref[SLZW_CODE_LIMIT],
		   lut_stack[SLZW_MAX_LUTS][SLZW_LUT_CHILD_ELEMS];
        unsigned char node_child[SLZW_CODE_LIMIT];
	/*
	 * Stack allocation control
	 */
	slzw_ndx_t node_stack_in_use, lut_stack_in_use;
	/*
	 * Output encoding control
	 */
	size_t normal_count = 0, esc_count = 0;
	size_t oi = 0, next_code, curr_code_len = SLZW_ROOT_NODE_BITS + 1, acc;
	sbitio_write_init(&acc);
	/*
	 * Initialize data structures
	 */
	size_t j;
	for (j = 0; j < 256; j += 4) {
		node_codes[j] = j;
		node_codes[j + 1] = j + 1;
		node_codes[j + 2] = j + 2;
		node_codes[j + 3] = j + 3;
	}
	SLZW_ENC_RESET(node_lutref, lut_stack_in_use,
		       node_stack_in_use, next_code, curr_code_len);
	/*
	 * Encoding loop
	 */
	size_t i = 0;
	slzw_ndx_t curr_node = 0;
	for (; i < ss;) {
#if SLZW_ENABLE_RLE
		/*
		 * Attempt RLE at current offset
		 */
		size_t run_length, run_elem_size, max_run, rle_mode;
		max_run = i + S_NBIT(SLZW_RLE_BITSD2 * 2);
		run_length = srle_run(s, i, ss, SLZW_RLE_CSIZE, max_run,
				      &run_elem_size, S_TRUE);
		rle_mode = !run_length ? 0 : run_elem_size == 1 ? SLZW_RLE1 :
			   run_elem_size == 3 ? SLZW_RLE3 :
			   run_elem_size == 4 ? SLZW_RLE4 : 0;
		if (rle_mode) {
			size_t ch = run_length >> SLZW_RLE_BITSD2,
			       cl = run_length & S_NBITMASK(SLZW_RLE_BITSD2);
			slzw_bitio_write(o, &oi, &acc, rle_mode, curr_code_len,
					 &normal_count, &esc_count);
			sbitio_write(o, &oi, &acc, cl, SLZW_RLE_BITSD2);
			sbitio_write(o, &oi, &acc, ch, SLZW_RLE_BITSD2);
			sbitio_write(o, &oi, &acc, s[i], 8);
			if (run_elem_size >= 3) {
				sbitio_write(o, &oi, &acc, s[i + 1], 8);
				sbitio_write(o, &oi, &acc, s[i + 2], 8);
				if (run_elem_size >= 4)
					sbitio_write(o, &oi, &acc, s[i + 3], 8);
			}
			i += run_length * run_elem_size;
			if (i == ss)
				break;
		}
#endif
		/*
		 * Locate pattern
		 */
		unsigned in_byte = s[i++];
		curr_node = in_byte;
		slzw_ndx_t r;
		for (; i < ss; i++) {
			in_byte = s[i];
			const slzw_ndx_t nlut = node_lutref[curr_node];
			if (nlut < 0 && in_byte == node_child[curr_node]) {
				curr_node = -nlut;
				continue;
			}
			if (nlut <= 0 || !(r = lut_stack[nlut][in_byte]))
				break;
			curr_node = r;
		}
		if (i == ss)
			break;
		/*
		 * Add new code to the tree
		 */
		const slzw_ndx_t new_node = node_stack_in_use++;
		if (node_lutref[curr_node] <= 0) {
			if (node_lutref[curr_node] == 0) { /* empty */
				node_lutref[curr_node] = -new_node;
				node_child[curr_node] = in_byte;
			} else { /* single node: convert it to LUT */
				slzw_ndx_t alt_n = -node_lutref[curr_node];
				slzw_ndx_t new_lut = lut_stack_in_use++;
				node_lutref[curr_node] = new_lut;
				memset(&lut_stack[new_lut], 0,
				       sizeof(lut_stack[0]));
#ifdef _MSC_VER
#pragma warning(suppress: 6001)
#endif
				lut_stack[new_lut][node_child[curr_node]] =
									alt_n;
			}
		}
		if (node_lutref[curr_node] > 0)
			lut_stack[node_lutref[curr_node]][in_byte] = new_node;
		/*
		 * Write pattern code to the output stream
		 */
		node_codes[new_node] = next_code;
		node_lutref[new_node] = 0;
		slzw_bitio_write(o, &oi, &acc, node_codes[curr_node],
			         curr_code_len, &normal_count, &esc_count);
		if (next_code == (size_t)(1 << curr_code_len))

			curr_code_len++;
		/*
		 * Reset tree if tree code limit is reached or if running
		 * out of LUTs
		 */
		if (++next_code == SLZW_CODE_LIMIT ||
		    lut_stack_in_use == SLZW_MAX_LUTS) {
			slzw_bitio_write(o, &oi, &acc, SLZW_RESET,
					 curr_code_len, &normal_count,
					 &esc_count);
		        SLZW_ENC_RESET(node_lutref, lut_stack_in_use,
				       node_stack_in_use, next_code,
				       curr_code_len);
		}
	}
	/*
	 * Write last code, the "end of information" mark, and fill bits with 0
	 */
	slzw_bitio_write(o, &oi, &acc, node_codes[curr_node], curr_code_len,
			 &normal_count, &esc_count);
#if SLZW_USE_STOP_CODE
	slzw_bitio_write(o, &oi, &acc, SLZW_STOP, curr_code_len, &normal_count,
			 &esc_count);
#endif
	sbitio_write_close(o, &oi, &acc);
	return oi;
}

size_t sdec_lzw(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s || !o || !ss, 0);
	size_t i, acc, accbuf, oi, normal_count = 0, esc_count = 0, last_code,
	       curr_code_len = SLZW_ROOT_NODE_BITS + 1, next_inc_code;
	slzw_ndx_t parents[SLZW_CODE_LIMIT];
	unsigned char xbyte[SLZW_CODE_LIMIT], pattern[SLZW_CODE_LIMIT],
		      lastwc = 0;
	/*
	 * Init read buffer
	 */
	oi = 0;
	sbitio_read_init(&acc, &accbuf);
	/*
	 * Initialize root node
	 */
	slzw_setseq256s8((unsigned *)xbyte);
	SLZW_DEC_RESET(curr_code_len, last_code, next_inc_code, parents);
	/*
	 * Code expand loop
	 */
	size_t new_code;
	for (i = 0; i < ss;) {
		new_code = slzw_bitio_read(s, &i, &acc, &accbuf,
					   curr_code_len, &normal_count,
					   &esc_count);
		if (new_code < SLZW_OP_START || new_code > SLZW_OP_END) {
			if (last_code == SLZW_CODE_LIMIT) {
				o[oi++] = lastwc = xbyte[new_code];
				last_code = new_code;
				continue;
			}
			size_t code, pattern_off = SLZW_MAX_CODE;
			if (new_code == next_inc_code) {
				pattern[pattern_off--] = lastwc;
				code = last_code;
			} else {
				code = new_code;
			}
			for (; code >= SLZW_FIRST;) {
				pattern[pattern_off--] = xbyte[code];
				code = parents[code];
			}
			pattern[pattern_off--] = lastwc = xbyte[next_inc_code] =
								xbyte[code];
			parents[next_inc_code] = last_code;
			if (next_inc_code < SLZW_CODE_LIMIT)
				next_inc_code++;
			if (next_inc_code == (size_t)(1 << curr_code_len) &&
			    next_inc_code < SLZW_CODE_LIMIT) {
				curr_code_len++;
			}
			last_code = new_code;
			/*
			 * Write LZW pattern
			 */
			size_t write_size = SLZW_CODE_LIMIT - 1 - pattern_off;
			memcpy(o + oi, pattern + pattern_off + 1, write_size);
			oi += write_size;
			continue;
		}
#if SLZW_ENABLE_RLE
		/*
		 * Write RLE pattern
		 */
		if (new_code >= SLZW_RLE1 && new_code <= SLZW_RLE4) {
			union s_u32 rle;
			size_t count, cl, ch;
			cl = sbitio_read(s, &i, &acc, &accbuf, SLZW_RLE_BITSD2);
			ch = sbitio_read(s, &i, &acc, &accbuf, SLZW_RLE_BITSD2);
			count = ch << SLZW_RLE_BITSD2 | cl;
			rle.b[0] = sbitio_read(s, &i, &acc, &accbuf, 8);
			if (new_code == SLZW_RLE1) {
				memset(o + oi, rle.b[0], count);
			} else {
				rle.b[1] = sbitio_read(s, &i, &acc, &accbuf, 8);
				rle.b[2] = sbitio_read(s, &i, &acc, &accbuf, 8);
				if (new_code == SLZW_RLE4) {
					count *= 4;
					rle.b[3] = sbitio_read(s, &i, &acc,
							       &accbuf, 8);
					s_memset32(o + oi, rle.a32, count);
				} else {
					count *= 3;
					s_memset24(o + oi, rle.b, count);
				}
			}
			oi += count;
			continue;
		}
#endif
		if (new_code == SLZW_RESET) {
			SLZW_DEC_RESET(curr_code_len, last_code, next_inc_code,
				       parents);
			continue;
		}
#if SLZW_USE_STOP_CODE
		if (new_code == SLZW_STOP)
			break;
#endif
	}
	return oi;
}

static size_t senc_rle_flush(const unsigned char *s, const size_t i,
			     const size_t i_done, unsigned char *o, size_t oi)
{
	size_t d = i - i_done;
	if (d > 0) {
#if SLZW_DEBUG
		fprintf(stderr, "[%u] %s(%u)\n", (unsigned)oi,
			(d <= SRLE_RUN_MASK_SHORT ? "St" : "ST"), (unsigned)d);
#endif
		if (d <= SRLE_RUN_MASK_SHORT) {
			o[oi++] = SRLE_OP_ST_SHORT | d;
		} else {
			o[oi++] = SRLE_OP_ST;
			S_ST_U32(o + oi, S_HTON_U32((suint32_t)d));
			oi += 4;
		}
		memcpy(o + oi, s + i_done, d);
		oi += d;
	}
	return oi;
}

size_t senc_rle(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s || !o || !ss, 0);
	size_t i = 0, oi = 0, i_done = 0, run_length, run_elem_size;
	for (; i < ss;) {
		run_length = srle_run(s, i, ss, 10, (suint32_t)-1,
				      &run_elem_size, S_FALSE);
		if (!run_length) {
			i++;
			continue;
		}
		oi = senc_rle_flush(s, i, i_done, o, oi);
		if (run_length <= SRLE_RUN_MASK_SHORT) {
#if SLZW_DEBUG
			fprintf(stderr, "[%u] %s(%u)\n", (unsigned)oi,
				(run_elem_size == 4 ? "r4" : "r3"),
				(unsigned)run_length);
#endif
			if (run_elem_size == 1) {
				run_length /= 4;
				run_elem_size = 4;
			}
			if (run_elem_size == 4)
				o[oi++] = SRLE_OP_RLE4_SHORT | run_length;
			else
				o[oi++] = SRLE_OP_RLE3_SHORT | run_length;
		} else {
#if SLZW_DEBUG
			fprintf(stderr, "[%u] %s(%u)\n", (unsigned)oi,
				(run_elem_size == 4 ? "R4" :
				 run_elem_size == 3 ? "R3" : "R1"),
				(unsigned)run_length);
#endif
			if (run_elem_size == 1)
				o[oi++] = SRLE_OP_RLE1;
			else if (run_elem_size == 4)
				o[oi++] = SRLE_OP_RLE4;
			else
				o[oi++] = SRLE_OP_RLE3;
			S_ST_U32(o + oi, S_HTON_U32((suint32_t)run_length));
			oi += 4;
		}
		o[oi++] = s[i];
		if (run_elem_size >= 3) {
			o[oi++] = s[i + 1];
			o[oi++] = s[i + 2];
			if (run_elem_size == 4)
				o[oi++] = s[i + 3];
		}
		i += run_length * run_elem_size;
		i_done = i;
	}
	oi = senc_rle_flush(s, i, i_done, o, oi);
	return oi;
}

size_t sdec_rle(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s || !o || !ss, 0);
	size_t i = 0, oi = 0, op, ops, op_sz, cnt;
	for (; i < ss;) {
		op = s[i++];
		ops = op & SRLE_OP_MASK_SHORT;
		switch (ops) {
		case SRLE_OP_ST_SHORT:
		case SRLE_OP_RLE4_SHORT:
		case SRLE_OP_RLE3_SHORT:
			cnt = op & SRLE_RUN_MASK_SHORT;
			op_sz = ops == SRLE_OP_ST_SHORT ? 0 :
				ops == SRLE_OP_RLE4_SHORT ? 4 : 3;
#if SLZW_DEBUG
			fprintf(stderr, "[%u:%u] %s(%u)\n", (unsigned)(i - 1),
				(unsigned)oi,
				(!op_sz? "st" : op_sz == 4 ? "r4" : "r3"),
				(unsigned)cnt);
#endif
			break;
		default:
			cnt = S_LD_U32(s + i);
			cnt = S_NTOH_U32(cnt);
			op_sz = op == SRLE_OP_ST ? 0 : op == SRLE_OP_RLE1 ? 1 :
			op == SRLE_OP_RLE4 ? 4 : 3;
			i += 4;
#if SLZW_DEBUG
			fprintf(stderr, "[%u:%u] OP%u(%u)\n",
				(unsigned)(i - 5), (unsigned)oi,
				(unsigned)op_sz, (unsigned)cnt);
#endif
		}
		if (op_sz == 0) {
#if SLZW_DEBUG
			fprintf(stderr, "[%u] ST(%u)\n", (unsigned)(i),
				(unsigned)cnt);
#endif
			memcpy(o + oi, s + i, cnt);
			i += cnt;
		} else {
#if SLZW_DEBUG
			fprintf(stderr, "[%u] RLE%u(%u)\n", (unsigned)(i),
				(unsigned)op_sz, (unsigned)cnt);
#endif
			if (op_sz == 1) {
				memset(o + oi, s[i++], cnt);
			} else  {
				if (op_sz == 4) {
					cnt *= 4;
					size_t v = S_LD_U32(s + i);
					s_memset32(o + oi, v, cnt);
					i += 4;
				} else {
					cnt *= 3;
					s_memset24(o + oi, s + i, cnt);
					i += 3;
				}
			}
		}
		oi += cnt;
	}
	return oi;
}


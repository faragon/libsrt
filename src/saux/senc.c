/*
 * senc.c
 *
 * Buffer encoding/decoding
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "senc.h"
#include "sbitio.h"
#include <stdlib.h>

#ifndef SLZW_ENABLE_RLE
#define SLZW_ENABLE_RLE		1
#endif
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

#define SLZW_MAX_TREE_BITS	10
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
#define SLZW_ESC_RATIO		(1 << (17 - SLZW_MAX_TREE_BITS))
#define SLZW_LUT_CHILD_ELEMS	256
#define SLZW_MAX_LUTS		15
#define SLZW_MAX_NGROUPS	150
/*
 * SLZW_EPG: elements per group on a node; don't change it. And if requiring
 * to increase it, add elements to the "Duff's device" switch SLZW_SWITCH_DD()
 */
#define SLZW_EPG		7

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
#define DB64C1(a, b)	((unsigned char)(a << 2 | b >> 4))
#define DB64C2(b, c)	((unsigned char)(b << 4 | c >> 2))
#define DB64C3(c, d)	((unsigned char)(c << 6 | d))

#define SLZW_ENC_RESET(node_lutref, lut_stack_in_use,			\
		       node_stack_in_use, next_code, curr_code_len, 	\
		       groups_in_use) {					\
		memset(node_lutref, 0, 257 * sizeof(node_lutref[0]));	\
		lut_stack_in_use = 1;					\
		node_stack_in_use = 256;				\
		curr_code_len = SLZW_ROOT_NODE_BITS + 1;		\
		next_code = SLZW_FIRST;					\
		groups_in_use = 0;					\
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

S_INLINE unsigned char hex2nibble(const int h)
{
	return h2n[(h - 48) & 0x3f];
}

static size_t senc_hex_aux(const unsigned char *s, const size_t ss,
			   unsigned char *o, const unsigned char *t)
{
	RETURN_IF(!o, ss * 2);
	RETURN_IF(!s, 0);
	const size_t out_size = ss * 2;
	size_t i = ss, j = out_size;
	#define ENCHEX_LOOP(ox, ix) {		\
		const int next = s[ix - 1];	\
		o[ox - 2] = t[next >> 4];	\
		o[ox - 1] = t[next & 0x0f];	\
		}
	if (ss % 2) {
		ENCHEX_LOOP(j, i);
		i--;
		j -= 2;
	}
	for (; i > 0; i -= 2, j -= 4) {
		ENCHEX_LOOP(j, i);
		ENCHEX_LOOP(j - 2, i - 1);
	}
	#undef ENCHEX_LOOP
	return out_size;
}

static void slzw_setseq256s8(uint32_t *p)
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

static size_t slzw_bio_read(sbio_t *bio, size_t cbits, size_t *normal_count,
			      size_t *esc_count)
{
	size_t c;
	if (slzw_short_codes(*normal_count, *esc_count)) {
		c = sbio_read(bio, 8);
		if (c == 255) {
			c = sbio_read(bio, cbits);
			(*esc_count)++;
		} else {
			(*normal_count)++;
		}
	} else {
		c = sbio_read(bio, cbits);
		(*normal_count)++;
	}
	return c;
}

static void slzw_bio_write(sbio_t *bio, size_t c, size_t cbits,
			     size_t *normal_count, size_t *esc_count)
{
	if (slzw_short_codes(*normal_count, *esc_count)) {
		if (c < 255) {
	                sbio_write(bio, c, 8);
			(*normal_count)++;
#if SLZW_DEBUG
			fprintf(stderr, "%u (8 (%u))\n",
				(unsigned)c, (unsigned)cbits);
#endif
		} else {
	                sbio_write(bio, 255, 8);
			sbio_write(bio, c, cbits);
			(*esc_count)++;
#if SLZW_DEBUG
			fprintf(stderr, "%u (prefix + %u)\n",
				(unsigned)c, (unsigned)cbits);
#endif
		}
	} else {
		sbio_write(bio, c, cbits);
		(*normal_count)++;
#if SLZW_DEBUG
		fprintf(stderr, "%u (%u)\n", (unsigned)c, (unsigned)cbits);
#endif
	}
}

#if S_BPWORD <= 4
	#define S_RLE_LOAD(a) S_LD_U32(a)
	typedef uint32_t rle_load_t;
#else
	#define S_RLE_LOAD(a) S_LD_U64(a)
	typedef uint64_t rle_load_t;
#endif

static size_t srle_run(const unsigned char *s, size_t i, size_t ss,
		       size_t min_run, size_t max_run, size_t *run_elem_size,
		       sbool_t lzw_mode)
{
	RETURN_IF(i + min_run >= ss, 0);
	const size_t es = sizeof(uint32_t);
	const uint8_t *p0 = s + i,
		      *p3 = s + (i + 3);
	uint32_t p00 = S_LD_U32(p0), p01 = S_LD_U32(p0 + es),
		  p30 = S_LD_U32(p3 + 0);
	size_t run_length = 0, eq4 = p00 == p01,
	       eq3 = !eq4 && p00 == p30;
	for (; eq4 || eq3;) {
		size_t j, ss2;
		if (eq4) {
			ss2 = S_MIN(ss, max_run) - sizeof(rle_load_t);
			rle_load_t u0 = S_RLE_LOAD(s + i),
				   v0 = S_RLE_LOAD(s + i + 1);
			j = i + sizeof(uint32_t) * 2;
			if (u0 == v0) {
				j &= (size_t)S_ALIGNMASK;
				*run_elem_size = 1;
			} else {
				*run_elem_size = 4;
			}
			for (; j < ss2 ; j += sizeof(rle_load_t))
				if (u0 != S_RLE_LOAD(s + j))
					break;
		} else {
			ss2 = S_MIN(ss, max_run) - sizeof(uint32_t);
			uint32_t p31 = S_LD_U32(p3 + es),
				  p32 = S_LD_U32(p3 + 2 * es),
				  p02 = S_LD_U32(p0 + 2 * es);
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
	RETURN_IF(!o, (ss / 3 + (ss % 3 ? 1 : 0)) * 4);
	RETURN_IF(!s, 0);
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
	RETURN_IF(!o, (ss / 4) * 3);
	RETURN_IF(!s, 0);
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
	RETURN_IF(!o, ss / 2);
	const size_t ssd2 = ss - (ss % 2),
		     ssd4 = ss - (ss % 4);
	ASSERT_RETURN_IF(!ssd2, 0);
	size_t i = 0, j = 0;
	#define SDEC_HEX_L(n, m)	\
		o[j + n] = (unsigned char)(hex2nibble(s[i + m]) << 4) | \
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

S_INLINE size_t senc_esc_xml_req_size(const unsigned char *s, const size_t ss)
{
	size_t i = 0, sso = ss;
	for (; i < ss; i++)
		switch (s[i]) {
		case '"': case '\'': sso += 5; continue;
		case '&': sso += 4; continue;
		case '<': case '>': sso += 3; continue;
		default: continue;
		}
	return sso;
}

size_t senc_esc_xml(const unsigned char *s, const size_t ss, unsigned char *o,
		    const size_t known_sso)
{
	RETURN_IF(!s, 0);
	size_t sso = known_sso ? known_sso : senc_esc_xml_req_size(s, ss);
	RETURN_IF(!o, sso);
	RETURN_IF(!ss, 0);
	size_t i = ss - 1, j = sso;
	for (; i != (size_t)-1; i--) {
		switch (s[i]) {
		case '"': j -= 6; s_memcpy6(o + j, "&quot;"); continue;
		case '&': j -= 5; s_memcpy5(o + j, "&amp;"); continue;
		case '\'': j -= 6; s_memcpy6(o + j, "&apos;"); continue;
		case '<': j -= 4; s_memcpy4(o + j, "&lt;"); continue;
		case '>': j -= 4; s_memcpy4(o + j, "&gt;"); continue;
		default: o[--j] = s[i]; continue;
		}
	}
	return sso;
}

size_t sdec_esc_xml(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!o, ss);
	RETURN_IF(!s || !ss, 0);
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

S_INLINE size_t senc_esc_json_req_size(const unsigned char *s, const size_t ss)
{
	size_t i = 0, sso = ss;
	for (; i < ss; i++)
		switch (s[i]) {
		case '\b': case '\t': case '\n': case '\f': case '\r':
		case '"': case '\\': sso++; continue;
		default: continue;
		}
	return sso;
}

/* BEHAVIOR: slash ('/') is not escaped (intentional) */
size_t senc_esc_json(const unsigned char *s, const size_t ss, unsigned char *o,
		     const size_t known_sso)
{
	RETURN_IF(!s, 0);
	size_t sso = known_sso ? known_sso : senc_esc_json_req_size(s, ss);
	RETURN_IF(!o, sso);
	RETURN_IF(!ss, 0);
	size_t i = ss - 1, j = sso;
	for (; i != (size_t)-1; i--) {
		switch (s[i]) {
		case '\b': j -= 2; s_memcpy2(o + j, "\\b"); continue;
		case '\t': j -= 2; s_memcpy2(o + j, "\\t"); continue;
		case '\n': j -= 2; s_memcpy2(o + j, "\\n"); continue;
		case '\f': j -= 2; s_memcpy2(o + j, "\\f"); continue;
		case '\r': j -= 2; s_memcpy2(o + j, "\\r"); continue;
		case '"': j -= 2; s_memcpy2(o + j, "\\\""); continue;
		case '\\': j -= 2; s_memcpy2(o + j, "\\\\"); continue;
		default: o[--j] = s[i]; continue;
		}
	}
	return sso;
}

size_t sdec_esc_json(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!o, ss);
	RETURN_IF(!s || !ss, 0);
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

S_INLINE size_t senc_esc_url_req_size(const unsigned char *s, const size_t ss)
{
	size_t i = 0, sso = ss;
	for (; i < ss; i++) {
		if ((s[i] >= 'A' && s[i] <= 'Z') ||
		    (s[i] >= 'a' && s[i] <= 'z') ||
		    (s[i] >= '0' && s[i] <= '9'))
			continue;
		switch (s[i]) {
		case '-': case '_': case '.': case '~':
			continue;
		default:
			sso += 2;
			continue;
		}
	}
	return sso;
}

size_t senc_esc_url(const unsigned char *s, const size_t ss, unsigned char *o,
		    const size_t known_sso)
{
	RETURN_IF(!s, 0);
	size_t sso = known_sso ? known_sso : senc_esc_url_req_size(s, ss);
	RETURN_IF(!o, sso);
	RETURN_IF(!ss, 0);
	size_t i = ss - 1, j = sso;
	for (; i != (size_t)-1; i--) {
		if ((s[i] >= 'A' && s[i] <= 'Z') ||
		    (s[i] >= 'a' && s[i] <= 'z') ||
		    (s[i] >= '0' && s[i] <= '9')) {
			o[--j] = s[i];
			continue;
		}
		switch (s[i]) {
		case '-': case '_': case '.': case '~':
			o[--j] = s[i];
			continue;
		default:
			j -= 3;
			o[j + 2] = n2h_u[s[i] & 0x0f];
			o[j + 1] = n2h_u[s[i] >> 4];
			o[j] = '%';
			continue;
		}
	}
	return sso;
}

size_t sdec_esc_url(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!o, ss);
	RETURN_IF(!s || !ss, 0);
	size_t i = 0, j = 0;
	for (; i < ss; j++) {
		if (s[i] == '%' && i + 3 <= ss) {
			o[j] = (unsigned char)(hex2nibble(s[i + 1]) << 4) |
				hex2nibble(s[i + 2]);
			i += 3;
			continue;
		}
		o[j] = s[i++];
	}
	return j;
}

S_INLINE size_t senc_esc_byte_req_size(const unsigned char *s,
				       unsigned char tgt, const size_t ss)
{
	size_t i = 0, sso = ss;
	for (; i < ss; i++)
		if (s[i] == tgt)
			sso++;
	return sso;
}

static size_t senc_esc_byte(const unsigned char *s, const size_t ss,
			    unsigned char tgt, unsigned char *o,
			    const size_t known_sso)
{
	RETURN_IF(!s, 0);
	size_t sso = known_sso ? known_sso : senc_esc_byte_req_size(s, tgt, ss);
	RETURN_IF(!o, sso);
	RETURN_IF(!ss, 0);
	size_t i = ss - 1, j = sso;
	for (; i != (size_t)-1; i--) {
		if (s[i] == tgt)
			o[--j] = s[i];
		o[--j] = s[i];
	}
	return sso;
}

static size_t sdec_esc_byte(const unsigned char *s, const size_t ss,
			    unsigned char tgt, unsigned char *o)
{
	RETURN_IF(!o, ss);
	RETURN_IF(!s || !ss, 0);
	size_t i = 0, j = 0, ssm1 = ss - 1;
	for (; i < ssm1; j++) {
		if (s[i] == tgt && s[i + 1] == tgt)
			i++;
		o[j] = s[i++];
	}
	if (i < ss)
		o[j++] = s[i];
	return j;
}

size_t senc_esc_dquote(const unsigned char *s, const size_t ss,
		       unsigned char *o, const size_t known_sso)
{
	return senc_esc_byte(s, ss, '\"', o, known_sso);
}

size_t sdec_esc_dquote(const unsigned char *s, const size_t ss,
		       unsigned char *o)
{
	return sdec_esc_byte(s, ss, '\"', o);
}

size_t senc_esc_squote(const unsigned char *s, const size_t ss,
		       unsigned char *o, const size_t known_sso)
{
	return senc_esc_byte(s, ss, '\'', o, known_sso);
}

size_t sdec_esc_squote(const unsigned char *s, const size_t ss,
		       unsigned char *o)
{
	return sdec_esc_byte(s, ss, '\'', o);
}

/*
 * Header for LZW and RLE encoding
 */

static size_t build_header(const size_t ss, unsigned char *o)
{
	size_t header_bytes = ss < 128 ? 1 : 4;
	if (header_bytes == 1)
		o[0] = (unsigned char)(ss << 1); /* bit 0 kept 0 */
	else
		S_ST_LE_U32(o, (uint32_t)((ss << 1) | 1));
	return header_bytes;
}

static const unsigned char *dec_header(const unsigned char *s, const size_t ss,
				       size_t *header_size, size_t *expected_ss)
{
	if (!ss) {
		*header_size = 0;
		*expected_ss = 0;
	} else if ((s[0] & 1) == 0) { /* 1 byte header */
		*header_size = 1;
		*expected_ss = s[0] >> 1;
		s++;
	} else { /* 4 byte header */
		*header_size = 4;
		*expected_ss = S_LD_LE_U32(s) >> 1;
		s += 4;
	}
	return s;
}

/*
 * LZW encoding/decoding
 */

typedef short slzw_ndx_t;

size_t senc_lzw(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(ss >= 0x80000000, 0); /* currently limited to 2^31-1 input */
	RETURN_IF(!o && ss > 0, ss + (ss / 10) + 32); /* max out size */
	RETURN_IF(!s || !o || !ss, 0);
	size_t i, j;
	/*
	 * Node structure (separated elements and not a "struct", in order to
	 * save space because of data alignment)
	 *
	 * node_codes[i]: LZW output code for the node
	 * node_lutref[i]: 0: empty, < 0: -next_node, > 0: 256-child LUT ref.
	 * node_child[i]: if node_lutref < 0: next node byte (one-child node)
	 */
        slzw_ndx_t node_codes[SLZW_CODE_LIMIT],
		   node_lutref[SLZW_CODE_LIMIT],
		   lut_stack[SLZW_MAX_LUTS][SLZW_LUT_CHILD_ELEMS];
        unsigned char node_child[SLZW_CODE_LIMIT];
	struct NodeGroup {
		slzw_ndx_t refs[SLZW_EPG];
		unsigned char childs[SLZW_EPG];
		unsigned char nrefs;
	} g[SLZW_MAX_NGROUPS];
	/*
	 * Stack allocation control
	 */
	slzw_ndx_t node_stack_in_use, lut_stack_in_use, groups_in_use;
	/*
	 * Output encoding control
	 */
	size_t normal_count = 0, esc_count = 0;
	size_t next_code, curr_code_len = SLZW_ROOT_NODE_BITS + 1;
	size_t header_bytes = build_header(ss, o);
	sbio_t bio;
	sbio_write_init(&bio, o + header_bytes);
	/*
	 * Initialize data structures
	 */
	for (j = 0; j < 256; j += 4) {
		node_codes[j] = (slzw_ndx_t)j;
		node_codes[j + 1] = (slzw_ndx_t)j + 1;
		node_codes[j + 2] = (slzw_ndx_t)j + 2;
		node_codes[j + 3] = (slzw_ndx_t)j + 3;
	}
	SLZW_ENC_RESET(node_lutref, lut_stack_in_use,
		       node_stack_in_use, next_code, curr_code_len,
		       groups_in_use);
	/*
	 * Encoding loop
	 */
	slzw_ndx_t curr_node = 0;
	for (i = 0; i < ss;) {
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
			slzw_bio_write(&bio, rle_mode, curr_code_len,
					 &normal_count, &esc_count);
			sbio_write(&bio, cl, SLZW_RLE_BITSD2);
			sbio_write(&bio, ch, SLZW_RLE_BITSD2);
			sbio_write(&bio, s[i], 8);
			if (run_elem_size >= 3) {
				sbio_write(&bio, s[i + 1], 8);
				sbio_write(&bio, s[i + 2], 8);
				if (run_elem_size >= 4)
					sbio_write(&bio, s[i + 3], 8);
			}
			i += run_length * run_elem_size;
			if (i == ss)
				break;
		}
#endif
		/*
		 * Locate pattern
		 */
		unsigned char in_byte = s[i++];
		curr_node = in_byte;
		slzw_ndx_t r;
		for (; i < ss; i++) {
			in_byte = s[i];
			slzw_ndx_t nlut = node_lutref[curr_node];
			if (nlut < 0) {
				/*
				 * -1 .. (-SLZW_CODE_LIMIT + 1): 1-elem node
				 */
				if (nlut > -SLZW_CODE_LIMIT) {
					if (in_byte == node_child[curr_node]) {
						curr_node = -nlut;
						continue;
					}
				} else {
					/*
					 * -SLZW_CODE_LIMIT..
					 *  -SLZW_CODE_LIMIT -
					 *   SLZW_MAX_NGROUPS: SLZW_EPG el./node
					 */
					int ng = -(nlut + SLZW_CODE_LIMIT);
					struct NodeGroup *gx = &g[ng];
					#define SLZW_SWITCH_DD(n)	       \
						if (in_byte == gx->childs[n]) {\
							curr_node =	       \
								gx->refs[n];   \
							continue;	       \
						}
					switch (g[ng].nrefs) {
					case 7: SLZW_SWITCH_DD(6)
					case 6: SLZW_SWITCH_DD(5)
					case 5: SLZW_SWITCH_DD(4)
					case 4: SLZW_SWITCH_DD(3)
					case 3: SLZW_SWITCH_DD(2)
					case 2: SLZW_SWITCH_DD(1)
						SLZW_SWITCH_DD(0)
					}
					#undef SLZW_SWITCH_DD
				}
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
			if (node_lutref[curr_node] == 0) { /* empty -> 1st */
				node_lutref[curr_node] = -new_node;
				node_child[curr_node] = in_byte;
			} else {
				if (node_lutref[curr_node] > -SLZW_CODE_LIMIT) {
					/* Case of node with 1 element, growing
					 * to N-element node.
					 */
#ifdef _MSC_VER
#pragma warning(disable: 6001)
#endif
					g[groups_in_use].refs[0] =
							-node_lutref[curr_node];
					g[groups_in_use].childs[0] =
							node_child[curr_node];
					g[groups_in_use].refs[1] = new_node;
					g[groups_in_use].childs[1] = in_byte;
					g[groups_in_use].nrefs = 2;
					node_lutref[curr_node] =
					       -SLZW_CODE_LIMIT - groups_in_use;
					groups_in_use++; /* alloc new group */
				} else {
#ifdef _MSC_VER
#pragma warning(disable: 6001)
#endif
					int ng = -(node_lutref[curr_node] +
						   SLZW_CODE_LIMIT);
					unsigned char nrefs = g[ng].nrefs;
					if (nrefs < SLZW_EPG) { /* space left */
						g[ng].refs[nrefs] = new_node;
						g[ng].childs[nrefs] = in_byte;
						g[ng].nrefs = nrefs + 1;
					} else { /* > SLZW_EPG: build LUT */
						slzw_ndx_t new_lut = lut_stack_in_use++;
						node_lutref[curr_node] = new_lut;
						memset(&lut_stack[new_lut], 0,
						       sizeof(lut_stack[0]));
#ifdef _MSC_VER
#pragma warning(disable: 6001)
#endif
						for (j = 0; j < SLZW_EPG; j++)
							lut_stack[new_lut]
							     [g[ng].childs[j]] =
							          g[ng].refs[j];
					}
				}
			}
		}
		if (node_lutref[curr_node] > 0)
			lut_stack[node_lutref[curr_node]][in_byte] = new_node;
		/*
		 * Write pattern code to the output stream
		 */
		node_codes[new_node] = (slzw_ndx_t)next_code;
		node_lutref[new_node] = 0;
		slzw_bio_write(&bio, (size_t)node_codes[curr_node],
			         curr_code_len, &normal_count, &esc_count);
		if (next_code == (size_t)(1 << curr_code_len))

			curr_code_len++;
		/*
		 * Reset tree if tree code limit is reached or if running
		 * out of LUTs
		 */
		if (++next_code == SLZW_CODE_LIMIT ||
		    lut_stack_in_use == SLZW_MAX_LUTS ||
		    groups_in_use == SLZW_MAX_NGROUPS) {
			slzw_bio_write(&bio, SLZW_RESET,
					 curr_code_len, &normal_count,
					 &esc_count);
		        SLZW_ENC_RESET(node_lutref, lut_stack_in_use,
				       node_stack_in_use, next_code,
				       curr_code_len, groups_in_use);
		}
	}
	/*
	 * Write last code, the "end of information" mark, and fill bits with 0
	 */
	slzw_bio_write(&bio, (size_t)node_codes[curr_node], curr_code_len,
		       &normal_count, &esc_count);
#if SLZW_USE_STOP_CODE
	slzw_bio_write(&bio, SLZW_STOP, curr_code_len, &normal_count,
		       &esc_count);
#endif
	return sbio_write_close(&bio) + header_bytes;
}

size_t sdec_lzw(const unsigned char *s, const size_t ss0, unsigned char *o)
{
	RETURN_IF(!s || !ss0, 0);
	size_t expected_ss, header_size;
	s = dec_header(s, ss0, &header_size, &expected_ss);
	RETURN_IF(!o, expected_ss); /* max out size */
	RETURN_IF(ss0 <= header_size, 0);
	size_t ss = ss0 - header_size;
	sbio_t bio;
	size_t oi = 0, normal_count = 0, esc_count = 0, last_code,
	       curr_code_len = SLZW_ROOT_NODE_BITS + 1, next_inc_code;
	slzw_ndx_t parents[SLZW_CODE_LIMIT];
	unsigned char pattern[SLZW_CODE_LIMIT], lastwc = 0;
	union {	uint8_t g8[SLZW_CODE_LIMIT + 1];
		uint32_t g32[(SLZW_CODE_LIMIT + 7) / 4]; } xbyte;
	/*
	 * Init read buffer
	 */
	sbio_read_init(&bio, s);
	/*
	 * Initialize root node
	 */
	slzw_setseq256s8(xbyte.g32);
	SLZW_DEC_RESET(curr_code_len, last_code, next_inc_code, parents);
	/*
	 * Code expand loop
	 */
	size_t new_code;
	for (; sbio_off(&bio) < ss;) {
		new_code = slzw_bio_read(&bio, curr_code_len, &normal_count,
					 &esc_count);
		if (new_code < SLZW_OP_START || new_code > SLZW_OP_END) {
			if (last_code == SLZW_CODE_LIMIT) {
				o[oi++] = lastwc = xbyte.g8[new_code];
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
				pattern[pattern_off--] = xbyte.g8[code];
				code = (size_t)parents[code];
			}
			pattern[pattern_off--] = lastwc = xbyte.g8[next_inc_code] =
								xbyte.g8[code];
			parents[next_inc_code] = (slzw_ndx_t)last_code;
			if (next_inc_code < SLZW_MAX_CODE)
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
			cl = sbio_read(&bio, SLZW_RLE_BITSD2);
			ch = sbio_read(&bio, SLZW_RLE_BITSD2);
			count = ch << SLZW_RLE_BITSD2 | cl;
			rle.b[0] = (unsigned char)sbio_read(&bio, 8);
			if (new_code == SLZW_RLE1) {
				memset(o + oi, rle.b[0], count);
			} else {
				rle.b[1] = (unsigned char)sbio_read(&bio, 8);
				rle.b[2] = (unsigned char)sbio_read(&bio, 8);
				if (new_code == SLZW_RLE4) {
					count *= 4;
					rle.b[3] = (unsigned char)sbio_read(
						      &bio, 8);
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
			o[oi++] = (unsigned char)(SRLE_OP_ST_SHORT | d);
		} else {
			o[oi++] = SRLE_OP_ST;
			S_ST_U32(o + oi, S_HTON_U32((uint32_t)d));
			oi += 4;
		}
		memcpy(o + oi, s + i_done, d);
		oi += d;
	}
	return oi;
}

size_t senc_rle(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(ss >= 0x80000000, 0); /* currently limited to 2^31-1 input */
	RETURN_IF(!o && ss > 0, ss + (ss / 10) + 128); /* max out size */
	RETURN_IF(!s || !o || !ss, 0);
	size_t header_bytes = build_header(ss, o);
	o += header_bytes;
	size_t i = 0, oi = 0, i_done = 0, run_length, run_elem_size;
	for (; i < ss;) {
		run_length = srle_run(s, i, ss, 10, (uint32_t)-1,
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
				o[oi++] = (unsigned char)(SRLE_OP_RLE4_SHORT |
							  run_length);
			else
				o[oi++] = (unsigned char)(SRLE_OP_RLE3_SHORT |
							  run_length);
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
			S_ST_U32(o + oi, S_HTON_U32((uint32_t)run_length));
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
	return oi + header_bytes;
}

size_t sdec_rle(const unsigned char *s, const size_t ss0, unsigned char *o)
{
	RETURN_IF(!s || !ss0, 0);
	size_t expected_ss, header_size;
	s = dec_header(s, ss0, &header_size, &expected_ss);
	RETURN_IF(!o, expected_ss); /* max out size */
	RETURN_IF(ss0 <= header_size, 0);
	size_t ss = ss0 - header_size;
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
					uint32_t v = S_LD_U32(s + i);
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


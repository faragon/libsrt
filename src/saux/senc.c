/*
 * senc.c
 *
 * Buffer encoding/decoding
 *
 * Copyright (c) 2015-2018 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "senc.h"
#include "shash.h"
#include <stdlib.h>

#define SDEBUG_LZ	0
#if !defined(SDEBUG_LZ_STATS) && SDEBUG_LZ
	#undef SDEBUG_LZ
	#define SDEBUG_LZ 0
#endif

#ifdef S_MINIMAL
	/* For minimal configuration, using stack-only hash table of 2^10
	 * elements (e.g. for 32-bit, 2^11 * 4 = 8192 bytes of stack memory)
	 */
	#define LZ_MAX_HASH_BITS_STACK 11
	#define LZ_MAX_HASH_BITS   LZ_MAX_HASH_BITS_STACK
#else
	#ifndef S_LZ_DONT_ALLOW_HEAP_USAGE
		#define S_LZ_ALLOW_HEAP_USAGE
	#endif
	/* For normal configuration allowing heap, use e.g. 64KiB
	 * of stack for 32-bit, and 128KiB for 64-bit mode
	 */
	#define LZ_MAX_HASH_BITS_STACK 14
	#ifdef S_LZ_ALLOW_HEAP_USAGE
		/* Allow up to 2^26 hash table elements (64MiB * sizeof(size_t)
		 * bytes heap memory)
		 */
		#define LZ_MAX_HASH_BITS       26
	#else
		#define LZ_MAX_HASH_BITS       LZ_MAX_HASH_BITS_STACK
	#endif
#endif

/*
 * Constants
 */

static const uint8_t b64e[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b',
	'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
	'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
	};
static const uint8_t b64d[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0,
	0, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 0, 0, 0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 32, 33,
	34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
	51, 0, 0, 0, 0, 0
	};
static const uint8_t n2h_l[16] = {
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 97, 98, 99, 100, 101, 102
	};
static const uint8_t n2h_u[16] = {
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 65, 66, 67, 68, 69, 70
	};
static const uint8_t h2n[64] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

/*
 * Macros
 */

#define EB64C1(a)	(a >> 2)
#define EB64C2(a, b)	((a & 3) << 4 | b >> 4)
#define EB64C3(b, c)	((b & 0xf) << 2 | c >> 6)
#define EB64C4(c)	(c & 0x3f)
#define DB64C1(a, b)	((uint8_t)(a << 2 | b >> 4))
#define DB64C2(b, c)	((uint8_t)(b << 4 | c >> 2))
#define DB64C3(c, d)	((uint8_t)(c << 6 | d))

/*
 * Internal functions
 */

S_INLINE uint8_t hex2nibble(const int h)
{
	return h2n[(h - 48) & 0x3f];
}

static size_t senc_hex_aux(const uint8_t *s, const size_t ss,
			   uint8_t *o, const uint8_t *t)
{
	size_t out_size, i, j;
	RETURN_IF(!o, ss * 2);
	RETURN_IF(!s, 0);
	out_size = ss * 2;
	i = ss;
	j = out_size;
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

/*
 * Base64 encoding/decoding
 */

size_t senc_b64(const uint8_t *s, const size_t ss, uint8_t *o)
{
	unsigned si0, si1, si2;
	size_t ssod4, ssd3, tail, i, j, out_size;
	RETURN_IF(!o, (ss / 3 + (ss % 3 ? 1 : 0)) * 4);
	RETURN_IF(!s, 0);
	ssod4 = (ss / 3) * 4;
	ssd3 = ss - (ss % 3);
	tail = ss - ssd3;
	i = ssd3;
	j = ssod4 + (tail ? 4 : 0);
	out_size = j;
	switch (tail) {
	case 2: si0 = s[ssd3];
		si1 = s[ssd3 + 1];
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
		si0 = s[i - 3];
		si1 = s[i - 2];
		si2 = s[i - 1];
		o[j - 4] = b64e[EB64C1(si0)];
		o[j - 3] = b64e[EB64C2(si0, si1)];
		o[j - 2] = b64e[EB64C3(si1, si2)];
		o[j - 1] = b64e[EB64C4(si2)];
	}
	return out_size;
}

size_t sdec_b64(const uint8_t *s, const size_t ss, uint8_t *o)
{
	size_t i, j, ssd4, tail;
	RETURN_IF(!o, (ss / 4) * 3);
	RETURN_IF(!s, 0);
	i = 0;
	j = 0;
	ssd4 = ss - (ss % 4);
	tail = s[ss - 2] == '=' || s[ss - 1] == '=' ? 4 : 0;
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

size_t senc_hex(const uint8_t *s, const size_t ss, uint8_t *o)
{
	return senc_hex_aux(s, ss, o, n2h_l);
}

size_t senc_HEX(const uint8_t *s, const size_t ss, uint8_t *o)
{
	return senc_hex_aux(s, ss, o, n2h_u);
}

size_t sdec_hex(const uint8_t *s, const size_t ss, uint8_t *o)
{
	size_t ssd2, ssd4, i, j;
	RETURN_IF(!o, ss / 2);
	ssd2 = ss - (ss % 2);
	ssd4 = ss - (ss % 4);
	ASSERT_RETURN_IF(!ssd2, 0);
	i = 0;
	j = 0;
	#define SDEC_HEX_L(n, m)	\
		o[j + n] = (uint8_t)(hex2nibble(s[i + m]) << 4) | \
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

S_INLINE size_t senc_esc_xml_req_size(const uint8_t *s, const size_t ss)
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

size_t senc_esc_xml(const uint8_t *s, const size_t ss, uint8_t *o,
		    const size_t known_sso)
{
	size_t sso, i, j;
	RETURN_IF(!s, 0);
	sso = known_sso ? known_sso : senc_esc_xml_req_size(s, ss);
	RETURN_IF(!o, sso);
	RETURN_IF(!ss, 0);
	i = ss - 1;
	j = sso;
	for (; i != (size_t)-1; i--) {
		switch (s[i]) {
		case '"': j -= 6; memcpy(o + j, "&quot;", 6); continue;
		case '&': j -= 5; memcpy(o + j, "&amp;", 5); continue;
		case '\'': j -= 6; memcpy(o + j, "&apos;", 6); continue;
		case '<': j -= 4; memcpy(o + j, "&lt;", 4); continue;
		case '>': j -= 4; memcpy(o + j, "&gt;", 4); continue;
		default: o[--j] = s[i]; continue;
		}
	}
	return sso;
}

size_t sdec_esc_xml(const uint8_t *s, const size_t ss, uint8_t *o)
{
	size_t i, j;
	RETURN_IF(!o, ss);
	RETURN_IF(!s || !ss, 0);
	for (i = j = 0; i < ss; j++) {
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

S_INLINE size_t senc_esc_json_req_size(const uint8_t *s, const size_t ss)
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
size_t senc_esc_json(const uint8_t *s, const size_t ss, uint8_t *o,
		     const size_t known_sso)
{
	size_t i, j, sso;
	RETURN_IF(!s, 0);
	sso = known_sso ? known_sso : senc_esc_json_req_size(s, ss);
	RETURN_IF(!o, sso);
	RETURN_IF(!ss, 0);
	i = ss - 1;
	j = sso;
	for (; i != (size_t)-1; i--) {
		switch (s[i]) {
		case '\b': j -= 2; memcpy(o + j, "\\b", 2); continue;
		case '\t': j -= 2; memcpy(o + j, "\\t", 2); continue;
		case '\n': j -= 2; memcpy(o + j, "\\n", 2); continue;
		case '\f': j -= 2; memcpy(o + j, "\\f", 2); continue;
		case '\r': j -= 2; memcpy(o + j, "\\r", 2); continue;
		case '"': j -= 2; memcpy(o + j, "\\\"", 2); continue;
		case '\\': j -= 2; memcpy(o + j, "\\\\", 2); continue;
		default: o[--j] = s[i]; continue;
		}
	}
	return sso;
}

size_t sdec_esc_json(const uint8_t *s, const size_t ss, uint8_t *o)
{
	size_t i, j;
	RETURN_IF(!o, ss);
	RETURN_IF(!s || !ss, 0);
	for (i = j = 0; i < ss; j++) {
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

S_INLINE size_t senc_esc_url_req_size(const uint8_t *s, const size_t ss)
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

size_t senc_esc_url(const uint8_t *s, const size_t ss, uint8_t *o,
		    const size_t known_sso)
{
	size_t i, j, sso;
	RETURN_IF(!s, 0);
	sso = known_sso ? known_sso : senc_esc_url_req_size(s, ss);
	RETURN_IF(!o, sso);
	RETURN_IF(!ss, 0);
	i = ss - 1;
	j = sso;
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

size_t sdec_esc_url(const uint8_t *s, const size_t ss, uint8_t *o)
{
	size_t i, j;
	RETURN_IF(!o, ss);
	RETURN_IF(!s || !ss, 0);
	for (i = j = 0; i < ss; j++) {
		if (s[i] == '%' && i + 3 <= ss) {
			o[j] = (uint8_t)(hex2nibble(s[i + 1]) << 4) |
				hex2nibble(s[i + 2]);
			i += 3;
			continue;
		}
		o[j] = s[i++];
	}
	return j;
}

S_INLINE size_t senc_esc_byte_req_size(const uint8_t *s,
				       uint8_t tgt, const size_t ss)
{
	size_t i = 0, sso = ss;
	for (; i < ss; i++)
		if (s[i] == tgt)
			sso++;
	return sso;
}

static size_t senc_esc_byte(const uint8_t *s, const size_t ss,
			    uint8_t tgt, uint8_t *o,
			    const size_t known_sso)
{
	size_t i, j, sso;
	RETURN_IF(!s, 0);
	sso = known_sso ? known_sso : senc_esc_byte_req_size(s, tgt, ss);
	RETURN_IF(!o, sso);
	RETURN_IF(!ss, 0);
	i = ss - 1;
	j = sso;
	for (; i != (size_t)-1; i--) {
		if (s[i] == tgt)
			o[--j] = s[i];
		o[--j] = s[i];
	}
	return sso;
}

static size_t sdec_esc_byte(const uint8_t *s, const size_t ss,
			    uint8_t tgt, uint8_t *o)
{
	size_t i, j, ssm1;
	RETURN_IF(!o, ss);
	RETURN_IF(!s || !ss, 0);
	i = 0;
	j = 0;
	ssm1 = ss - 1;
	for (; i < ssm1; j++) {
		if (s[i] == tgt && s[i + 1] == tgt)
			i++;
		o[j] = s[i++];
	}
	if (i < ss)
		o[j++] = s[i];
	return j;
}

size_t senc_esc_dquote(const uint8_t *s, const size_t ss,
		       uint8_t *o, const size_t known_sso)
{
	return senc_esc_byte(s, ss, '\"', o, known_sso);
}

size_t sdec_esc_dquote(const uint8_t *s, const size_t ss,
		       uint8_t *o)
{
	return sdec_esc_byte(s, ss, '\"', o);
}

size_t senc_esc_squote(const uint8_t *s, const size_t ss,
		       uint8_t *o, const size_t known_sso)
{
	return senc_esc_byte(s, ss, '\'', o, known_sso);
}

size_t sdec_esc_squote(const uint8_t *s, const size_t ss,
		       uint8_t *o)
{
	return sdec_esc_byte(s, ss, '\'', o);
}

#if SDEBUG_LZ_STATS
	size_t lz_st_lit[9] = { 0 }, lz_st_lit_bytes = 0;
	size_t lz_st_ref[9] = { 0 }, lz_st_ref_bytes = 0;
#if SDEBUG_LZ
#define SZLOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define SZLOG(...)
#endif
	#define DBG_INC_LZLIT(ndx, cnt) {	\
		lz_st_lit[ndx]++;		\
		lz_st_lit_bytes += cnt;		\
		SZLOG("LIT%i:%06i\n", (int)(ndx + 1), (int)(cnt)); }
	#define DBG_INC_LZREF(ndx, cnt, dist, len) {		\
		lz_st_ref[ndx]++;				\
		lz_st_ref_bytes += cnt;				\
		SZLOG("REF%i:%06i.%08i\n", (int)(ndx + 1),	\
		      (int)(len + 4), (int)(dist + 1)); }
#else
	#define DBG_INC_LZLIT(ndx, cnt)
	#define DBG_INC_LZREF(ndx, cnt, dist, len)
#endif

/*
 * LZ opcodes
 *   x.00 R16   2
 * x.0001 R24   4
 * x.0101 R24S3 4
 * x.1001 R16S  4
 * x.1101 R16S2 4
 * x.0010 R32   4
 * x.0110 R24S  4
 * x.1010 R24S2 4
 * x.1110 L16   4
 * x.0011 R40   4
 * x.0111 R64   4
 * x.1011 L8    4
 * x.1111 L32   4
 */

#define LZOPR_HDR_16_BITS	2
#define LZOPR_HDR_16S_BITS	4
#define LZOPR_HDR_16S2_BITS	4
#define LZOPR_HDR_24_BITS	4
#define LZOPR_HDR_32_BITS	4
#define LZOPR_HDR_24S_BITS	4
#define LZOPR_HDR_24S2_BITS	4
#define LZOPR_HDR_24S3_BITS	4
#define LZOPR_HDR_40_BITS	4
#define LZOPR_HDR_64_BITS	4
#define LZOPL_HDR_8_BITS 	4
#define LZOPL_HDR_16_BITS	4
#define LZOPL_HDR_32_BITS	4

#define LZOP_MASK2		0x03
#define LZOP_MASK4		0x0f

/* Short-range references: */
#define LZOPR_16_ID		0x00
#define LZOPR_16S_ID		0x09
#define LZOPR_16S2_ID		0x0d
/* Mid-range references: */
#define LZOPR_24_ID		0x01
#define LZOPR_32_ID		0x02
#define LZOPR_40_ID		0x03
#define LZOPR_24S_ID		0x06
#define LZOPR_24S2_ID		0x0a
#define LZOPR_24S3_ID		0x05
/* Long-range ireferences: */
#define LZOPR_64_ID		0x07
/* Literal data: */
#define LZOPL_8_ID		0x0b
#define LZOPL_16_ID		0x0e
#define LZOPL_32_ID		0x0f

/* Max run: 2^28 bytes (256MB) */
#define LZOPL_8_BITS		(8 - LZOPL_HDR_8_BITS)
#define LZOPL_16_BITS		(16 - LZOPL_HDR_16_BITS)
#define LZOPL_32_BITS		(32 - LZOPL_HDR_32_BITS)
#define LZOPL_8_RANGE		(1 << LZOPL_8_BITS)
#define LZOPL_16_RANGE		(1 << LZOPL_16_BITS)
#define LZOPL_32_RANGE		(1 << LZOPL_32_BITS)

/* 16-bit chunk: 14 bits */
#define LZOPR_D16_BITS		13
#define LZOPR_L16_BITS		(16 - LZOPR_HDR_16_BITS - LZOPR_D16_BITS)

/* 16-bit chunk "S": 12 bits */
#define LZOPR_D16S_BITS		8
#define LZOPR_L16S_BITS		(16 - LZOPR_HDR_16S_BITS - LZOPR_D16S_BITS)

/* 16-bit chunk "S2": 12 bits */
#define LZOPR_D16S2_BITS	10
#define LZOPR_L16S2_BITS	(16 - LZOPR_HDR_16S2_BITS - LZOPR_D16S2_BITS)

/* 24-bit chunk: 20 */
#define LZOPR_D24_BITS		20
#define LZOPR_L24_BITS		(24 - LZOPR_HDR_24_BITS - LZOPR_D24_BITS)

/* 24-bit chunk "S": 20 bits */
#define LZOPR_D24S_BITS		14
#define LZOPR_L24S_BITS		(24 - LZOPR_HDR_24S_BITS - LZOPR_D24S_BITS)

/* 24-bit chunk "S2": 20 bits */
#define LZOPR_D24S2_BITS	16
#define LZOPR_L24S2_BITS	(24 - LZOPR_HDR_24S2_BITS - LZOPR_D24S2_BITS)

/* 24-bit chunk "S3": 20 bits */
#define LZOPR_D24S3_BITS	18
#define LZOPR_L24S3_BITS	(24 - LZOPR_HDR_24S3_BITS - LZOPR_D24S3_BITS)

/* 32-bit chunk: 30 bits */
#define LZOPR_D32_BITS		22
#define LZOPR_L32_BITS		(32 - LZOPR_HDR_32_BITS - LZOPR_D32_BITS)
/* 40-bit chunk: 36 bits */
#define LZOPR_D40_BITS		26
#define LZOPR_L40_BITS		(40 - LZOPR_HDR_40_BITS - LZOPR_D40_BITS)
/* 64-bit chunk: 60 bits */
#define LZOPR_D64_BITS		36
#define LZOPR_L64_BITS		(64 - LZOPR_HDR_64_BITS - LZOPR_D64_BITS)
/* Range: */
#define LZOPR_D16_RANGE		(1 << LZOPR_D16_BITS)
#define LZOPR_L16_RANGE		(1 << LZOPR_L16_BITS)
#define LZOPR_D16S_RANGE	(1 << LZOPR_D16S_BITS)
#define LZOPR_L16S_RANGE	(1 << LZOPR_L16S_BITS)
#define LZOPR_D16S2_RANGE	(1 << LZOPR_D16S2_BITS)
#define LZOPR_L16S2_RANGE	(1 << LZOPR_L16S2_BITS)
#define LZOPR_D24_RANGE		(1 << LZOPR_D24_BITS)
#define LZOPR_L24_RANGE		(1 << LZOPR_L24_BITS)
#define LZOPR_D24S_RANGE	(1 << LZOPR_D24S_BITS)
#define LZOPR_L24S_RANGE	(1 << LZOPR_L24S_BITS)
#define LZOPR_D24S2_RANGE	(1 << LZOPR_D24S2_BITS)
#define LZOPR_L24S2_RANGE	(1 << LZOPR_L24S2_BITS)
#define LZOPR_D24S3_RANGE	(1 << LZOPR_D24S3_BITS)
#define LZOPR_L24S3_RANGE	(1 << LZOPR_L24S3_BITS)
#define LZOPR_D32_RANGE		(1 << LZOPR_D32_BITS)
#define LZOPR_L32_RANGE		(1 << LZOPR_L32_BITS)
#define LZOPR_D40_RANGE		(1 << LZOPR_D40_BITS)
#define LZOPR_L40_RANGE		(1 << LZOPR_L40_BITS)
#if SIZE_MAX <= 0xffffffff
#define LZOPR_D64_RANGE		SIZE_MAX
#else
#define LZOPR_D64_RANGE		(1LL << LZOPR_D64_BITS)
#endif
#define LZOPR_L64_RANGE		(1 << LZOPR_L64_BITS)

S_INLINE void senc_lz_store_lit(uint8_t **o, const uint8_t *in, size_t size)
{
	const size_t sm1 = size - 1;
	if (sm1 < LZOPL_8_RANGE) {
		*((*o)++) = (uint8_t)((sm1 << LZOPL_HDR_8_BITS) |
				      LZOPL_8_ID);
		DBG_INC_LZLIT(0, 1 + size);
		memcpy(*o, in, size);
		(*o) += size;
	} else if (sm1 < LZOPL_16_RANGE) {
		S_ST_LE_U16(*o, (uint16_t)((sm1 << LZOPL_HDR_16_BITS) |
					   LZOPL_16_ID));
		DBG_INC_LZLIT(1, 2 + size);
		(*o) += 2;
		memcpy(*o, in, size);
		(*o) += size;
	} else {
		do
		{
			size_t s = size;
			if (s >= LZOPL_32_RANGE)
				s = LZOPL_32_RANGE;
			S_ST_LE_U32(*o,
				    (uint32_t)((s - 1) << LZOPL_HDR_32_BITS) |
				    LZOPL_32_ID);
			(*o) += 4;
			DBG_INC_LZLIT(3, 4 + s);
			memcpy(*o, in, s);
			(*o) += s;
			in += s;
			size -= s;
		} while (size > 0);
	}
}

S_INLINE srt_bool senc_lz_store_ref(uint8_t **o, const uint8_t *slit,
				   size_t nlit, size_t dist0, size_t *len0)
{
	uint64_t v64;
	size_t dist = dist0 - 1, len = *len0 - 4, leni, v = dist;
	if (dist < LZOPR_D24_RANGE) {
		if (dist < LZOPR_D16S_RANGE)
			goto senc_lz_store_ref_l16s;
		if (dist < LZOPR_D16S2_RANGE)
			goto senc_lz_store_ref_l16s2;
		else if (dist < LZOPR_D16_RANGE)
			goto senc_lz_store_ref_l16;
		else if (dist < LZOPR_D24S_RANGE)
			goto senc_lz_store_ref_l24s;
		else if (dist < LZOPR_D24S2_RANGE)
			goto senc_lz_store_ref_l24s2;
		else if (dist < LZOPR_D24S3_RANGE)
			goto senc_lz_store_ref_l24s3;
		else
			goto senc_lz_store_ref_l24;
	} else {
		if (dist < LZOPR_D32_RANGE) {
			if (*len0 <= 4)
				return S_FALSE;
			goto senc_lz_store_ref_l32;
		} else if (dist < LZOPR_D40_RANGE) {
			if (*len0 <= 5)
				return S_FALSE;
			goto senc_lz_store_ref_l40;
		} else
			goto senc_lz_store_ref_l64;
	}
senc_lz_store_ref_l16s:
	if (len < LZOPR_L16S_RANGE) {
		if (nlit > 0)
			senc_lz_store_lit(o, slit, nlit);
		v |= (len << LZOPR_D16S_BITS);
		S_ST_LE_U16(*o, (uint16_t)((v << LZOPR_HDR_16S_BITS) |
					   LZOPR_16S_ID));
		(*o) += 2;
		DBG_INC_LZREF(1, 2, dist, len);
		return S_TRUE;
	}
senc_lz_store_ref_l16s2:
	if (len < LZOPR_L16S2_RANGE) {
		if (nlit > 0)
			senc_lz_store_lit(o, slit, nlit);
		v |= (len << LZOPR_D16S2_BITS);
		S_ST_LE_U16(*o, (uint16_t)((v << LZOPR_HDR_16S2_BITS) |
					   LZOPR_16S2_ID));
		(*o) += 2;
		DBG_INC_LZREF(1, 2, dist, len);
		return S_TRUE;
	}
senc_lz_store_ref_l16:
	if (len < LZOPR_L16_RANGE) {
		if (nlit > 0)
			senc_lz_store_lit(o, slit, nlit);
		v |= (len << LZOPR_D16_BITS);
		S_ST_LE_U16(*o, (uint16_t)((v << LZOPR_HDR_16_BITS) |
					   LZOPR_16_ID));
		(*o) += 2;
		DBG_INC_LZREF(1, 2, dist, len);
		return S_TRUE;
	}
senc_lz_store_ref_l24s:
	if (len < LZOPR_L24S_RANGE) {
		if (nlit > 0)
			senc_lz_store_lit(o, slit, nlit);
		v |= (len << LZOPR_D24S_BITS);
		S_ST_LE_U32(*o, (uint32_t)(v << LZOPR_HDR_24S_BITS) |
				LZOPR_24S_ID);
		(*o) += 3;
		DBG_INC_LZREF(2, 3, dist, len);
		return S_TRUE;
	}
senc_lz_store_ref_l24s2:
	if (len < LZOPR_L24S2_RANGE) {
		if (nlit > 0)
			senc_lz_store_lit(o, slit, nlit);
		v |= (len << LZOPR_D24S2_BITS);
		S_ST_LE_U32(*o, (uint32_t)(v << LZOPR_HDR_24S2_BITS) |
				LZOPR_24S2_ID);
		(*o) += 3;
		DBG_INC_LZREF(2, 3, dist, len);
		return S_TRUE;
	}
senc_lz_store_ref_l24s3:
	if (len < LZOPR_L24S3_RANGE) {
		if (nlit > 0)
			senc_lz_store_lit(o, slit, nlit);
		v |= (len << LZOPR_D24S3_BITS);
		S_ST_LE_U32(*o, (uint32_t)(v << LZOPR_HDR_24S3_BITS) |
				LZOPR_24S3_ID);
		(*o) += 3;
		DBG_INC_LZREF(2, 3, dist, len);
		return S_TRUE;
	}
senc_lz_store_ref_l24:
	if (len < LZOPR_L24_RANGE) {
		if (nlit > 0)
			senc_lz_store_lit(o, slit, nlit);
		v |= (len << LZOPR_D24_BITS);
		S_ST_LE_U32(*o, (uint32_t)(v << LZOPR_HDR_24_BITS) |
				LZOPR_24_ID);
		(*o) += 3;
		DBG_INC_LZREF(2, 3, dist, len);
		return S_TRUE;
	}
senc_lz_store_ref_l32:
	if (len < LZOPR_L32_RANGE) {
		if (nlit > 0)
			senc_lz_store_lit(o, slit, nlit);
		v |= (len << LZOPR_D32_BITS);
		S_ST_LE_U32(*o, (uint32_t)(v << LZOPR_HDR_32_BITS) |
				LZOPR_32_ID);
		(*o) += 4;
		DBG_INC_LZREF(3, 4, dist, len);
		return S_TRUE;
	}
senc_lz_store_ref_l40:
	if (len < LZOPR_L40_RANGE) {
		if (nlit > 0)
			senc_lz_store_lit(o, slit, nlit);
		v64 = ((uint64_t)(v | ((uint64_t)len << LZOPR_D40_BITS))
			<< LZOPR_HDR_40_BITS) | LZOPR_40_ID;
		*(*o) = (uint8_t)v64;
		S_ST_LE_U32(*o + 1, (uint32_t)(v64 >> 8));
		(*o) += 5;
		DBG_INC_LZREF(4, 5, dist, len);
		return S_TRUE;
	}
senc_lz_store_ref_l64:
	if (*len0 <= 8) /* Not worth encoding too small references */
		return S_FALSE;
	if (dist0 > LZOPR_D64_RANGE) /* Out of range */
		return S_FALSE;
	if (nlit > 0)
		senc_lz_store_lit(o, slit, nlit);
	leni = *len0;
	*len0 = 0;
	do
	{
		if (leni >= (LZOPR_L64_RANGE + 4)) {
			len = LZOPR_L64_RANGE - 1;
			*len0 += (len + 4);
			leni -= (len + 4);
		} else {
			if (leni < 4)
				break;
			*len0 += leni;
			len = leni - 4;
			leni = 0;
		}
		v64 = ((uint64_t)(v | ((uint64_t)len << LZOPR_D64_BITS))
				 << LZOPR_HDR_64_BITS) | LZOPR_64_ID;
		S_ST_LE_U64(*o, v64);
		(*o) += 8;
		DBG_INC_LZREF(7, 8, dist, len);
		dist += (len + 4);
	} while (leni);
	return S_TRUE;
}

S_INLINE size_t senc_lz_match(const uint8_t *a, const uint8_t *b,
			      size_t max_size)
{
	size_t off = 0;
	const size_t szc = UINTPTR_MAX <= 0xffffffff ? 4 : 8;
	size_t msc = (max_size / szc) * szc;
	for (; off < msc && !memcmp(a + off, b + off, szc); off += szc);
	for (; off < max_size && a[off] == b[off]; off++);
	return off;
}

S_INLINE uint32_t senc_lz_hash(uint32_t a, size_t hash_size)
{
	return ((a >> (32 - hash_size)) + (a >> (32 / 3)) + a) &
	       S_NBITMASK(hash_size);
}

static size_t senc_lz_aux(const uint8_t *s, const size_t ss, uint8_t *o0,
			  const size_t hash_max_bits)
{
	uint8_t *o;
	size_t *refs, *refsx;
	size_t i, len, dist, w32, plit, h, last, sm4,
	       hash_size0, hash_size, hash_elems;
	/*
	 * Max out bytes = (input size) * 1.125 + 32
	 * (0 in case of edge case size_t overflow)
	 */
	RETURN_IF(!o0 && ss > 0, s_size_t_add(ss, (ss / 8) + 32, 0));
	RETURN_IF(!s || !o0 || !ss, 0);
	/*
	 * Header: unpacked length (compressed u64)
	 */
	o = o0;
	s_st_pk_u64(&o, ss);
	/*
	 * Case of small input: store uncompresed if smaller than 8 bytes
	 */
	if (ss < 8) {
		senc_lz_store_lit(&o, s, ss);
		return (size_t)(o - o0);
	}
	/*
	 * Hash size is kept proportional to the input buffer size, in order to
	 * ensure the hash initialization time don't hurt the case of small
	 * inputs.
	 */
	hash_size0 = slog2((uint64_t)ss);
	if (hash_size0 >= 2)
		hash_size0 -= 2;
	hash_size = S_RANGE(hash_size0, 3, hash_max_bits);
	hash_elems = (size_t)1 << hash_size;
	/*
	 * Hash table allocation and initialization
	 */
	refsx = hash_size > LZ_MAX_HASH_BITS_STACK ?
		(size_t *)s_malloc(sizeof(size_t) * hash_elems) : NULL;
	refs = refsx ? refsx : (size_t *)s_alloca(sizeof(size_t) *
						  hash_elems);
	RETURN_IF(!refs, 0);
	memset(refs, 0, hash_elems * sizeof(refs[0]));
	/*
	 * Compression loop
	 */
	plit = 0;
	sm4 = ss - 4;
	for (i = 4; i <= sm4;) {
		/*
		 * Load 32-bit chunk and locate it into the hash table
		 */
		w32 = S_LD_U32(s + i);
		h = senc_lz_hash((uint32_t)w32, hash_size);
		last = refs[h];
		refs[h] = i;
		if (w32 != S_LD_U32(s + last)) { /* Not found? */
			i++;
			continue;
		}
		/*
		 * Once the match is found, scan for more consecutive matches:
		 */
		len = senc_lz_match(s + i + 4, s + last + 4, ss - i - 4) + 4;
		dist = i - last;
		/*
		 * Write the reference (and pending literals, if any)
		 */
		if (!senc_lz_store_ref(&o, s + plit, i - plit, dist, &len)) {
			i++;
			continue;
		}
		/*
		 * Reset the offset for literals
		 */
		i += len;
		plit = i;
	}
	if (ss - plit > 0)
		senc_lz_store_lit(&o, s + plit, ss - plit);
	if (refsx)
		s_free(refsx);
	return (size_t)(o - o0);
}

size_t senc_lz(const uint8_t *s, const size_t ss, uint8_t *o0)
{
	return senc_lz_aux(s, ss, o0, LZ_MAX_HASH_BITS_STACK);
}

size_t senc_lzh(const uint8_t *s, const size_t ss, uint8_t *o0)
{
	return senc_lz_aux(s, ss, o0, LZ_MAX_HASH_BITS);
}

S_INLINE void s_reccpy1(uint8_t *o, const size_t dist, size_t n)
{
	size_t j = 0;
	for (; j < n; j++)
		o[j] = o[j - dist];
}

S_INLINE void s_reccpy(uint8_t *o, const size_t dist, size_t n)
{
	size_t i, n2, chunk;
	const uint8_t *s = o - dist;
	if (dist > n) {
		/* non-overlapped */
		if (n <= 16) {
			if (n == 1)
				*o = o[0 - dist];
			else
				memcpy(o, s, 16);
		} else {
			memcpy(o, s, n);
		}
		return;
	}
	/* overlapped copy: run length */
	switch (dist) {
	case 1: memset(o, *s, n);
		return;
	case 2: s_reccpy1(o, dist, 4);
		s_memset32(o + 4, s, (n / 4));
		return;
	case 3:
	case 6:
		if (dist == 6 && memcmp(s, s + 3, 3))
			break;
		s_memset24(o, s, (n / 3) + 1);
		return;
	case 4:	s_memset32(o, s, (n / 4) + 1);
		return;
	case 8:	s_memset64(o, s, (n / 8) + 1);
		return;
	}
	/* overlapped copy: generic */
	memcpy(o, s, dist);
	chunk = dist * 2;
	i = dist;
	if (i + chunk < n) {
		n2 = n - dist;
		for (; i < n2; i += chunk)
			memcpy(o + i, s, chunk);
	}
	if (i < n)
		memcpy(o + i, s, n - i);
}

S_INLINE void sdec_lz_load_ref(uint8_t **o, size_t dist, size_t len)
{
	dist++;
	len += 4;
	s_reccpy(*o, dist, len);
	(*o) += len;
}

S_INLINE void sdec_lz_load_lit(const uint8_t **s, uint8_t **o, size_t cnt)
{
	cnt++;
	memcpy(*o, *s, cnt);
	(*s) += cnt;
	(*o) += cnt;
}

/* BEHAVIOR: safety for avoiding decompression buffer overflow */
#define SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, sz)	\
	if (S_UNLIKELY(o + sz > o_top)) { 			\
		s = s_top;					\
		continue;					\
	}

size_t sdec_lz(const uint8_t *s0, const size_t ss, uint8_t *o0)
{
	uint8_t *o;
	uint64_t mix64;
	const uint8_t *s, *s_top, *o_top;
	size_t cnt, dist, len, mix, op, expected_ss;
	RETURN_IF(!s0 || ss < 4, 0); /* too small input (min hdr + opcode) */
	s = s0;
	expected_ss = (size_t)s_ld_pk_u64(&s, ss);
	RETURN_IF(ss <= (size_t)(s - s0), 0); /* invalid: incomplete header */
	RETURN_IF(!o0, expected_ss + 16); /* max out size */
	s_top = s0 + ss;
	o = o0;
	o_top = o + expected_ss;
	while (s < s_top) {
		if ((*s & LZOP_MASK2) == LZOPR_16_ID) {
			mix = S_LD_LE_U16(s) >> LZOPR_HDR_16_BITS;
			dist = mix & S_NBITMASK(LZOPR_D16_BITS);
			len = mix >> LZOPR_D16_BITS;
			s += 2;
#if SDEBUG_LZ
			fprintf(stderr, "R2:%06i.%08i\n",
				4 + (int)len, 1 + (int)dist);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
			sdec_lz_load_ref(&o, dist, len);
			continue;
		}
		op = *s & LZOP_MASK4;
		switch (op) {
		case LZOPR_16S2_ID:
			mix = S_LD_LE_U16(s) >> LZOPR_HDR_16S2_BITS;
			dist = mix & S_NBITMASK(LZOPR_D16S2_BITS);
			len = mix >> LZOPR_D16S2_BITS;
			s += 2;
#if SDEBUG_LZ
			fprintf(stderr, "R2:%06i.%08i [S2]\n",
				4 + (int)len, 1 + (int)dist);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
			sdec_lz_load_ref(&o, dist, len);
			continue;
		case LZOPR_16S_ID:
			mix = S_LD_LE_U16(s) >> LZOPR_HDR_16S_BITS;
			dist = mix & S_NBITMASK(LZOPR_D16S_BITS);
			len = mix >> LZOPR_D16S_BITS;
			s += 2;
#if SDEBUG_LZ
			fprintf(stderr, "R2:%06i.%08i [S1]\n",
				4 + (int)len, 1 + (int)dist);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
			sdec_lz_load_ref(&o, dist, len);
			continue;
		case LZOPR_24S3_ID:
			mix = (*s | (S_LD_LE_U16(s + 1) << 8))
				>> LZOPR_HDR_24S3_BITS;
			dist = (mix & S_NBITMASK(LZOPR_D24S3_BITS));
			len = mix >> LZOPR_D24S3_BITS;
			s += 3;
#if SDEBUG_LZ
			fprintf(stderr, "R3:%06i.%08i [S2]\n",
				4 + (int)len, 1 + (int)dist);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
			sdec_lz_load_ref(&o, dist, len);
			continue;
		case LZOPR_24S2_ID:
			mix = (*s | (S_LD_LE_U16(s + 1) << 8))
				>> LZOPR_HDR_24S2_BITS;
			dist = (mix & S_NBITMASK(LZOPR_D24S2_BITS));
			len = mix >> LZOPR_D24S2_BITS;
			s += 3;
#if SDEBUG_LZ
			fprintf(stderr, "R3:%06i.%08i [S3]\n",
				4 + (int)len, 1 + (int)dist);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
			sdec_lz_load_ref(&o, dist, len);
			continue;
		case LZOPR_24S_ID:
			mix = (*s | (S_LD_LE_U16(s + 1) << 8))
				>> LZOPR_HDR_24S_BITS;
			dist = (mix & S_NBITMASK(LZOPR_D24S_BITS));
			len = mix >> LZOPR_D24S_BITS;
			s += 3;
#if SDEBUG_LZ
			fprintf(stderr, "R3:%06i.%08i [S1]\n",
				4 + (int)len, 1 + (int)dist);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
			sdec_lz_load_ref(&o, dist, len);
			continue;
		case LZOPR_24_ID:
			mix = (*s | (S_LD_LE_U16(s + 1) << 8))
				>> LZOPR_HDR_24_BITS;
			dist = (mix & S_NBITMASK(LZOPR_D24_BITS));
			len = mix >> LZOPR_D24_BITS;
			s += 3;
#if SDEBUG_LZ
			fprintf(stderr, "R3:%06i.%08i\n",
				4 + (int)len, 1 + (int)dist);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
			sdec_lz_load_ref(&o, dist, len);
			continue;
		case LZOPR_32_ID:
			mix = S_LD_LE_U32(s) >> LZOPR_HDR_32_BITS;
			dist = (mix & S_NBITMASK(LZOPR_D32_BITS));
			len = mix >> LZOPR_D32_BITS;
			s += 4;
#if SDEBUG_LZ
			fprintf(stderr, "R4:%06i.%08i\n",
				4 + (int)len, 1 + (int)dist);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
			sdec_lz_load_ref(&o, dist, len);
			continue;
		case LZOPL_8_ID:
			cnt = *s >> LZOPL_HDR_8_BITS;
			s++;
#if SDEBUG_LZ
			fprintf(stderr, "L8:%06i\n", 1 + (int)cnt);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, cnt);
			sdec_lz_load_lit(&s, &o, cnt);
			continue;
		}
		switch (op) {
		case LZOPR_40_ID:
			mix64 = (*s | (((uint64_t)S_LD_LE_U32(s + 1)) << 8))
				>> LZOPR_HDR_40_BITS;
			dist = (uint32_t)mix64 & S_NBITMASK(LZOPR_D40_BITS);
			len = (uint32_t)(mix64 >> LZOPR_D40_BITS);
			s += 5;
#if SDEBUG_LZ
			fprintf(stderr, "R5:%06i.%08i\n",
				4 + (int)len, 1 + (int)dist);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
			sdec_lz_load_ref(&o, dist, len);
			continue;
		case LZOPR_64_ID:
			mix64 = S_LD_LE_U64(s) >> LZOPR_HDR_64_BITS;
			dist = (uint32_t)(mix64 & S_NBITMASK64(LZOPR_D64_BITS));
			len = (uint32_t)(mix64 >> LZOPR_D64_BITS);
			s += 8;
#if SDEBUG_LZ
			fprintf(stderr, "R8:%06i.%08i\n",
				4 + (int)len, 1 + (int)dist);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
			sdec_lz_load_ref(&o, dist, len);
			continue;
		case LZOPL_16_ID:
			cnt = S_LD_LE_U16(s) >> LZOPL_HDR_16_BITS;
			s += 2;
#if SDEBUG_LZ
			fprintf(stderr, "L16:%06i\n", 1 + (int)cnt);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, cnt);
			sdec_lz_load_lit(&s, &o, cnt);
			continue;
		case LZOPL_32_ID:
			cnt = S_LD_LE_U32(s) >> LZOPL_HDR_32_BITS;
			s += 4;
#if SDEBUG_LZ
			fprintf(stderr, "L32:%06i\n", 1 + (int)cnt);
#endif
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, cnt);
			sdec_lz_load_lit(&s, &o, cnt);
			continue;
		default:
#if SDEBUG_LZ
			fprintf(stderr, "UNKNOWN OPCODE (%02X): aborting!\n",
				(unsigned)op);
#endif
			s = s_top;
			continue;
		}
	}
	return o - o0;
}


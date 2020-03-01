/*
 * senc.c
 *
 * Buffer encoding/decoding
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "senc.h"
#include "shash.h"
#include <stdlib.h>

#ifndef SDEBUG_LZ
#define SDEBUG_LZ 0
#endif

#ifdef S_MINIMAL
/* For minimal configuration, using stack-only LUT of 2^10
 * elements (e.g. for 32-bit, 2^11 * 4 = 8192 bytes of stack memory)
 */
#define S_LZ_MAX_HASH_BITS_STACK 11
#define S_LZ_MAX_HASH_BITS S_LZ_MAX_HASH_BITS_STACK
#else
#ifndef S_LZ_DONT_ALLOW_HEAP_USAGE
#define S_LZ_ALLOW_HEAP_USAGE
#endif
/*
 * For normal configuration allowing heap, use e.g. 64KiB
 * of stack for 32-bit, and 128KiB for 64-bit mode
 */
#define S_LZ_MAX_HASH_BITS_STACK 14
#ifdef S_LZ_ALLOW_HEAP_USAGE
/*
 * Allow up to 2^26 LUT elements (64MiB * sizeof(size_t) bytes of heap memory)
 */
#define S_LZ_MAX_HASH_BITS 26
#else
#define S_LZ_MAX_HASH_BITS S_LZ_MAX_HASH_BITS_STACK
#endif
#endif

#if SDEBUG_LZ
#define SZLOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define SZLOG(...)
#endif
#define DBG_LZLIT(w, l) SZLOG("LIT%i:%06i\n", (int)(w), (int)(l))

#define DBG_LZREF(w, d, l, s)                                                  \
	SZLOG("REF%i:%06i.%06i %s\n", (int)(w), (int)(d), (int)(l), s)

/* LZ77 custom opcodes */

#define LZOP_REFVX 0x00 /* variable-size ref, 2-bit for length */
#define LZOP_REFVV 0x01 /* var-size ref, var-size length */
#define LZOP_LITV 0x03  /* var-size literals */
#define LZOP_REFVX_NBITS 1
#define LZOP_REFVV_NBITS 2
#define LZOP_LITV_NBITS 2
#define LZOP_REFVX_MASK S_NBITMASK(LZOP_REFVX_NBITS)
#define LZOP_REFVV_MASK S_NBITMASK(LZOP_REFVV_NBITS)
#define LZOP_LITV_MASK S_NBITMASK(LZOP_LITV_NBITS)
#define LZOP_REFVX_LBITS 2
#define LZOP_REFVX_LMASK S_NBITMASK64(LZOP_REFVX_LBITS)
#define LZOP_REFVX_LSHIFT LZOP_REFVX_NBITS
#define LZOP_REFVX_LRANGE (LZOP_REFVX_LMASK + 1)
#define LZOP_REFVX_DBITS (64 - LZOP_REFVX_LBITS - LZOP_REFVX_NBITS)
#define LZOP_REFVX_DMASK S_NBITMASK64(LZOP_REFVX_DBITS)
#define LZOP_REFVX_DSHIFT (LZOP_REFVX_LBITS + LZOP_REFVX_NBITS)
#define LZOP_REFVX_DRANGE (LZOP_REFVX_DMASK + 1)

/*
 * Constants
 */

static const uint8_t b64e[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};
static const uint8_t b64d[128] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  62, 0,  0,  0,  63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  0,  0,  0,
	0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  0,
	0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0,  0,  0,  0,  0};
static const uint8_t n2h_l[16] = {48, 49, 50, 51, 52, 53,  54,  55,
				  56, 57, 97, 98, 99, 100, 101, 102};
static const uint8_t n2h_u[16] = {48, 49, 50, 51, 52, 53, 54, 55,
				  56, 57, 65, 66, 67, 68, 69, 70};
static const uint8_t h2n[64] = {
	0, 1,  2,  3,  4,  5,  6,  7, 8, 9, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*
 * Macros
 */

#define EB64C1(a) (a >> 2)
#define EB64C2(a, b) ((a & 3) << 4 | b >> 4)
#define EB64C3(b, c) ((b & 0xf) << 2 | c >> 6)
#define EB64C4(c) (c & 0x3f)
#define DB64C1(a, b) ((uint8_t)(a << 2 | b >> 4))
#define DB64C2(b, c) ((uint8_t)(b << 4 | c >> 2))
#define DB64C3(c, d) ((uint8_t)(c << 6 | d))

/*
 * Internal functions
 */

S_INLINE uint8_t hex2nibble(int h)
{
	return h2n[(h - 48) & 0x3f];
}

static size_t senc_hex_aux(const uint8_t *s, size_t ss, uint8_t *o,
			   const uint8_t *t)
{
	size_t out_size, i, j;
	RETURN_IF(!o, ss * 2);
	RETURN_IF(!s, 0);
	out_size = ss * 2;
	i = ss;
	j = out_size;
#define ENCHEX_LOOP(ox, ix)                                                    \
	{                                                                      \
		int next = s[ix - 1];                                          \
		o[ox - 2] = t[next >> 4];                                      \
		o[ox - 1] = t[next & 0x0f];                                    \
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
	return out_size;
}

/*
 * Base64 encoding/decoding
 */

size_t senc_b64(const uint8_t *s, size_t ss, uint8_t *o)
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
	case 2:
		si0 = s[ssd3];
		si1 = s[ssd3 + 1];
		o[j - 4] = b64e[EB64C1(si0)];
		o[j - 3] = b64e[EB64C2(si0, si1)];
		o[j - 2] = b64e[EB64C3(si1, 0)];
		o[j - 1] = '=';
		j -= 4;
		break;
	case 1:
		si0 = s[ssd3];
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

size_t sdec_b64(const uint8_t *s, size_t ss, uint8_t *o)
{
	size_t i, j, ssd4, tail;
	RETURN_IF(!o, (ss / 4) * 3);
	RETURN_IF(!s, 0);
	i = 0;
	j = 0;
	ssd4 = ss - (ss % 4);
	tail = s[ss - 2] == '=' || s[ss - 1] == '=' ? 4 : 0;
	for (; i < ssd4 - tail; i += 4, j += 3) {
		int a = b64d[s[i]], b = b64d[s[i + 1]], c = b64d[s[i + 2]],
		    d = b64d[s[i + 3]];
		o[j] = DB64C1(a, b);
		o[j + 1] = DB64C2(b, c);
		o[j + 2] = DB64C3(c, d);
	}
	if (tail) {
		int a = b64d[s[i] & 0x7f], b = b64d[s[i + 1] & 0x7f];
		o[j++] = DB64C1(a, b);
		if (s[i + 2] != '=') {
			int c = b64d[s[i + 2] & 0x7f];
			o[j++] = DB64C2(b, c);
		}
	}
	return j;
}

/*
 * Hexadecimal encoding/decoding
 */

size_t senc_hex(const uint8_t *s, size_t ss, uint8_t *o)
{
	return senc_hex_aux(s, ss, o, n2h_l);
}

size_t senc_HEX(const uint8_t *s, size_t ss, uint8_t *o)
{
	return senc_hex_aux(s, ss, o, n2h_u);
}

size_t sdec_hex(const uint8_t *s, size_t ss, uint8_t *o)
{
	size_t ssd2, ssd4, i, j;
	RETURN_IF(!o, ss / 2);
	ssd2 = ss - (ss % 2);
	ssd4 = ss - (ss % 4);
	ASSERT_RETURN_IF(!ssd2, 0);
	i = 0;
	j = 0;
#define SDEC_HEX_L(n, m)                                                       \
	o[j + n] = (uint8_t)(hex2nibble(s[i + m]) << 4)                        \
		   | hex2nibble(s[i + m + 1]);
	for (; i < ssd4; i += 4, j += 2) {
		SDEC_HEX_L(0, 0);
		SDEC_HEX_L(1, 2);
	}
	for (; i < ssd2; i += 2, j += 1)
		SDEC_HEX_L(0, 0);
	return j;
}

S_INLINE size_t senc_esc_xml_req_size(const uint8_t *s, size_t ss)
{
	size_t i = 0, sso = ss;
	for (; i < ss; i++)
		switch (s[i]) {
		case '"':
		case '\'':
			sso += 5;
			continue;
		case '&':
			sso += 4;
			continue;
		case '<':
		case '>':
			sso += 3;
			continue;
		default:
			continue;
		}
	return sso;
}

size_t senc_esc_xml(const uint8_t *s, size_t ss, uint8_t *o, size_t known_sso)
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
		case '"':
			j -= 6;
			memcpy(o + j, "&quot;", 6);
			continue;
		case '&':
			j -= 5;
			memcpy(o + j, "&amp;", 5);
			continue;
		case '\'':
			j -= 6;
			memcpy(o + j, "&apos;", 6);
			continue;
		case '<':
			j -= 4;
			memcpy(o + j, "&lt;", 4);
			continue;
		case '>':
			j -= 4;
			memcpy(o + j, "&gt;", 4);
			continue;
		default:
			o[--j] = s[i];
			continue;
		}
	}
	return sso;
}

size_t sdec_esc_xml(const uint8_t *s, size_t ss, uint8_t *o)
{
	size_t i, j;
	RETURN_IF(!o, ss);
	RETURN_IF(!s || !ss, 0);
	for (i = j = 0; i < ss; j++) {
		if (s[i] == '&') {
			switch (s[i + 1]) {
			case 'q':
				if (i + 5 <= ss && s[i + 2] == 'u'
				    && s[i + 3] == 'o' && s[i + 4] == 't'
				    && s[i + 5] == ';') {
					o[j] = '"';
					i += 6;
					continue;
				}
				break;
			case 'a':
				if (i + 4 <= ss && s[i + 2] == 'm'
				    && s[i + 3] == 'p' && s[i + 4] == ';') {
					o[j] = '&';
					i += 5;
					continue;
				}
				if (i + 5 <= ss && s[i + 2] == 'p'
				    && s[i + 3] == 'o' && s[i + 4] == 's'
				    && s[i + 5] == ';') {
					o[j] = '\'';
					i += 6;
					continue;
				}
				break;
			case 'l':
				if (i + 3 <= ss && s[i + 2] == 't'
				    && s[i + 3] == ';') {
					o[j] = '<';
					i += 4;
					continue;
				}
				break;
			case 'g':
				if (i + 3 <= ss && s[i + 2] == 't'
				    && s[i + 3] == ';') {
					o[j] = '>';
					i += 4;
					continue;
				}
				break;
#if 0 /* BEHAVIOR: not implemented (on purpose) */
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

S_INLINE size_t senc_esc_json_req_size(const uint8_t *s, size_t ss)
{
	size_t i = 0, sso = ss;
	for (; i < ss; i++)
		switch (s[i]) {
		case '\b':
		case '\t':
		case '\n':
		case '\f':
		case '\r':
		case '"':
		case '\\':
			sso++;
			continue;
		default:
			continue;
		}
	return sso;
}

/* BEHAVIOR: slash ('/') is not escaped (intentional) */
size_t senc_esc_json(const uint8_t *s, size_t ss, uint8_t *o, size_t known_sso)
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
		case '\b':
			j -= 2;
			memcpy(o + j, "\\b", 2);
			continue;
		case '\t':
			j -= 2;
			memcpy(o + j, "\\t", 2);
			continue;
		case '\n':
			j -= 2;
			memcpy(o + j, "\\n", 2);
			continue;
		case '\f':
			j -= 2;
			memcpy(o + j, "\\f", 2);
			continue;
		case '\r':
			j -= 2;
			memcpy(o + j, "\\r", 2);
			continue;
		case '"':
			j -= 2;
			memcpy(o + j, "\\\"", 2);
			continue;
		case '\\':
			j -= 2;
			memcpy(o + j, "\\\\", 2);
			continue;
		default:
			o[--j] = s[i];
			continue;
		}
	}
	return sso;
}

size_t sdec_esc_json(const uint8_t *s, size_t ss, uint8_t *o)
{
	size_t i, j;
	RETURN_IF(!o, ss);
	RETURN_IF(!s || !ss, 0);
	for (i = j = 0; i < ss; j++) {
		if (s[i] == '\\' && i + 1 <= ss) {
			switch (s[i + 1]) {
			case 'b':
				o[j] = 8;
				i += 2;
				continue;
			case 't':
				o[j] = 9;
				i += 2;
				continue;
			case 'n':
				o[j] = 10;
				i += 2;
				continue;
			case 'f':
				o[j] = 12;
				i += 2;
				continue;
			case 'r':
				o[j] = 13;
				i += 2;
				continue;
			case '"':
			case '\\':
			case '/':
				o[j] = s[i + 1];
				i += 2;
				continue;
#if 0 /* BEHAVIOR: not implemented (on purpose) */
			case 'u': break;
#endif
			default:
				break;
			}
		}
		o[j] = s[i++];
	}
	return j;
}

S_INLINE size_t senc_esc_url_req_size(const uint8_t *s, size_t ss)
{
	size_t i = 0, sso = ss;
	for (; i < ss; i++) {
		if ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z')
		    || (s[i] >= '0' && s[i] <= '9'))
			continue;
		switch (s[i]) {
		case '-':
		case '_':
		case '.':
		case '~':
			continue;
		default:
			sso += 2;
			continue;
		}
	}
	return sso;
}

size_t senc_esc_url(const uint8_t *s, size_t ss, uint8_t *o, size_t known_sso)
{
	size_t i, j, sso;
	RETURN_IF(!s, 0);
	sso = known_sso ? known_sso : senc_esc_url_req_size(s, ss);
	RETURN_IF(!o, sso);
	RETURN_IF(!ss, 0);
	i = ss - 1;
	j = sso;
	for (; i != (size_t)-1; i--) {
		if ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z')
		    || (s[i] >= '0' && s[i] <= '9')) {
			o[--j] = s[i];
			continue;
		}
		switch (s[i]) {
		case '-':
		case '_':
		case '.':
		case '~':
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

size_t sdec_esc_url(const uint8_t *s, size_t ss, uint8_t *o)
{
	size_t i, j;
	RETURN_IF(!o, ss);
	RETURN_IF(!s || !ss, 0);
	for (i = j = 0; i < ss; j++) {
		if (s[i] == '%' && i + 3 <= ss) {
			o[j] = (uint8_t)(hex2nibble(s[i + 1]) << 4)
			       | hex2nibble(s[i + 2]);
			i += 3;
			continue;
		}
		o[j] = s[i++];
	}
	return j;
}

S_INLINE size_t senc_esc_byte_req_size(const uint8_t *s, uint8_t tgt, size_t ss)
{
	size_t i = 0, sso = ss;
	for (; i < ss; i++)
		if (s[i] == tgt)
			sso++;
	return sso;
}

static size_t senc_esc_byte(const uint8_t *s, size_t ss, uint8_t tgt,
			    uint8_t *o, size_t known_sso)
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

static size_t sdec_esc_byte(const uint8_t *s, size_t ss, uint8_t tgt,
			    uint8_t *o)
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

size_t senc_esc_dquote(const uint8_t *s, size_t ss, uint8_t *o,
		       size_t known_sso)
{
	return senc_esc_byte(s, ss, '\"', o, known_sso);
}

size_t sdec_esc_dquote(const uint8_t *s, size_t ss, uint8_t *o)
{
	return sdec_esc_byte(s, ss, '\"', o);
}

size_t senc_esc_squote(const uint8_t *s, size_t ss, uint8_t *o,
		       size_t known_sso)
{
	return senc_esc_byte(s, ss, '\'', o, known_sso);
}

size_t sdec_esc_squote(const uint8_t *s, size_t ss, uint8_t *o)
{
	return sdec_esc_byte(s, ss, '\'', o);
}

S_INLINE void senc_lz_store_lit(uint8_t **o, const uint8_t *in, size_t size)
{
#if SDEBUG_LZ
	uint8_t *o0 = *o;
#endif
	uint64_t op64 = ((uint64_t)(size - 1) << LZOP_LITV_NBITS) | LZOP_LITV;
	s_st_pk_u64(o, op64);

	DBG_LZLIT(*o - o0, size);

	memcpy(*o, in, size);
	(*o) += size;
}

S_INLINE void senc_lz_store_ref(uint8_t **o, size_t dist, size_t len)
{
#if SDEBUG_LZ
	uint8_t *o0 = *o;
#endif
	uint64_t v64;
	size_t dm1 = dist - 1, lm4 = len - 4;
	if ((uint64_t)dm1 < LZOP_REFVX_DRANGE && lm4 < LZOP_REFVX_LRANGE) {
		v64 = ((uint64_t)dm1 << LZOP_REFVX_DSHIFT)
		      | ((uint64_t)lm4 << LZOP_REFVX_LSHIFT) | LZOP_REFVX;
		s_st_pk_u64(o, v64);
		DBG_LZREF(*o - o0, dist, len, "[REFVX]");
	} else {
		v64 = ((uint64_t)lm4 << LZOP_REFVV_NBITS) | LZOP_REFVV;
		s_st_pk_u64(o, v64);
		s_st_pk_u64(o, dm1);
		DBG_LZREF(*o - o0, dist, len, "[REFVV]");
	}
}

S_INLINE size_t senc_lz_match(const uint8_t *a, const uint8_t *b,
			      size_t max_size)
{
	size_t off = 0;
#if UINTPTR_MAX <= 0xffffffff
	size_t szc = 4;
#else
	size_t szc = 8;
#endif
	size_t msc = (max_size / szc) * szc;
	for (; off < msc && !memcmp(a + off, b + off, szc); off += szc)
		;
	for (; off < max_size && a[off] == b[off]; off++)
		;
	return off;
}

S_INLINE size_t senc_lz_hash(size_t a)
{
	return (a >> 24) + (a >> 20) + (a >> 13) + a;
}

static size_t senc_lz_aux(const uint8_t *s, size_t ss, uint8_t *o0,
			  size_t hash_max_bits)
{
	uint8_t *o;
	const uint8_t *src, *tgt;
	size_t dist, h, hash_elems, hash_size, hash_size0, i, last, len, plit,
		*refs, *refsx, sm4, w32, xl, hash_mask;
	/*
	 * Max out bytes = (input size) * 1.125
	 * (0 in case of edge case size_t overflow)
	 */
	RETURN_IF(!o0 && ss > 0, s_size_t_add(ss, ((ss / 8) * 10) + 32, 0));
	RETURN_IF(!s || !o0 || !ss, 0);
	/*
	 * Header: unpacked length (compressed u64)
	 */
	o = o0;
	s_st_pk_u64(&o, ss);
	/*
	 * Case of small input: store uncompresed if smaller than 5 bytes
	 */
	if (ss < 5) {
		senc_lz_store_lit(&o, s, ss);
		return (size_t)(o - o0);
	}
	/*
	 * Hash size is kept proportional to the input buffer size, in order to
	 * ensure the hash initialization time don't hurt the case of small
	 * inputs.
	 */
	hash_size0 = slog2((uint64_t)ss);
	hash_size0 = S_MAX(10, hash_size0) - 2;
	hash_size = S_RANGE(hash_size0, 3, hash_max_bits);
	hash_elems = (size_t)1 << hash_size;
	hash_mask = hash_elems - 1;
	/*
	 * LUT allocation and initialization
	 */
	/* If using more than 4 LUTs, avoid stack allocation */
	if (hash_size > S_LZ_MAX_HASH_BITS_STACK) {
		refsx = (size_t *)s_malloc(sizeof(*refs) * hash_elems);
		RETURN_IF(!refsx, 0); /* BEHAVIOR: out of memory */
	} else {
		refsx = NULL;
	}
	refs = refsx ? refsx : (size_t *)s_alloca(sizeof(*refs) * hash_elems);
	RETURN_IF(!refs, 0);
	memset(refs, 0, sizeof(*refs) * hash_elems);
	/*
	 * Compression loop
	 */
	plit = 0;
	sm4 = ss - 4;
	i = 0;
	w32 = S_LD_U32(s + i);
	h = senc_lz_hash(w32) & hash_mask;
	refs[h] = i;
	for (i = 1; i <= sm4;) {
		/*
		 * Load 32-bit chunk and locate it into the LUT
		 */
		w32 = S_LD_U32(s + i);
		h = senc_lz_hash(w32) & hash_mask;
		last = refs[h];
		refs[h] = i;
		/*
		 * Locate matches in the LUT[s]
		 */
		if (w32 != S_LD_U32(s + last)) {
			i++;
			continue;
		}
		/*
		 * Match found: find match length
		 */
		src = s + i + 4;
		tgt = s + 4;
		xl = ss - i - 4;
		len = senc_lz_match(src, tgt + last, xl) + 4;
		dist = i - last;
		/*
		 * Avoid storing distant short references
		 */
		if (dist > 500000 && len == 4) {
			i++;
			continue;
		}
		/*
		 * Flush literals
		 */
		if (i - plit > 0)
			senc_lz_store_lit(&o, s + plit, i - plit);
		/*
		 * Write the reference
		 */
		senc_lz_store_ref(&o, dist, len);
		i += len;
		plit = i;
	}
	if (ss - plit > 0)
		senc_lz_store_lit(&o, s + plit, ss - plit);
	if (refsx)
		s_free(refsx);
	return (size_t)(o - o0);
}

size_t senc_lz(const uint8_t *s, size_t ss, uint8_t *o0)
{
	return senc_lz_aux(s, ss, o0, S_LZ_MAX_HASH_BITS_STACK);
}

size_t senc_lzh(const uint8_t *s, size_t ss, uint8_t *o0)
{
	return senc_lz_aux(s, ss, o0, S_LZ_MAX_HASH_BITS);
}

S_INLINE void s_reccpy1(uint8_t *o, size_t dist, size_t n)
{
	size_t j = 0;
	for (; j < n; j++)
		o[j] = o[j - dist];
}

S_INLINE void s_reccpy(uint8_t *o, size_t dist, size_t n)
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
	case 1:
		memset(o, *s, n);
		return;
	case 2:
		s_reccpy1(o, dist, 4);
		s_memset32(o + 4, s, (n / 4));
		return;
	case 3:
	case 6:
		if (dist == 6 && memcmp(s, s + 3, 3))
			break;
		s_memset24(o, s, (n / 3) + 1);
		return;
	case 4:
		s_memset32(o, s, (n / 4) + 1);
		return;
	case 8:
		s_memset64(o, s, (n / 8) + 1);
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
	s_reccpy(*o, dist, len);
	(*o) += len;
}

S_INLINE void sdec_lz_load_lit(const uint8_t **s, uint8_t **o, size_t cnt)
{
	memcpy(*o, *s, cnt);
	(*s) += cnt;
	(*o) += cnt;
}

/* BEHAVIOR: safety for avoiding decompression buffer overflow */
#define SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, sz)                   \
	if (S_UNLIKELY(o + sz > o_top)) {                                      \
		s = s_top;                                                     \
		continue;                                                      \
	}

size_t sdec_lz(const uint8_t *s0, size_t ss, uint8_t *o0)
{
#if SDEBUG_LZ
	const uint8_t *s_bk;
#endif
	uint64_t op64;
	uint8_t *o, op8;
	const uint8_t *s, *s_top, *o_top;
	size_t dist, len, expected_ss;
	RETURN_IF(!s0 || ss < 3, 0); /* too small input (min hdr + opcode) */
	s = s0;
	expected_ss = (size_t)s_ld_pk_u64(&s, ss);
	RETURN_IF(ss <= (size_t)(s - s0), 0); /* invalid: incomplete header */
	RETURN_IF(!o0, expected_ss + 16);     /* max out size */
	s_top = s0 + ss;
	RETURN_IF(s_top < s0, 0); /* BEHAVIOR: error on overflow */
	o = o0;
	o_top = o + expected_ss;
	while (s < s_top) {
#if SDEBUG_LZ
		s_bk = s;
#endif
		op64 = s_ld_pk_u64(&s, (size_t)(s_top - s));
		op8 = op64 & 0xff;
		if ((op8 & LZOP_REFVX_MASK) == LZOP_REFVX) {
			len = (size_t)(
				((op64 >> LZOP_REFVX_LSHIFT) & LZOP_REFVX_LMASK)
				+ 4);
			dist = (size_t)(
				((op64 >> LZOP_REFVX_DSHIFT) & LZOP_REFVX_DMASK)
				+ 1);
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
			sdec_lz_load_ref(&o, dist, len);
			DBG_LZREF(s - s_bk, dist, len, "[REFVX]");
			continue;
		}
		if ((op8 & LZOP_LITV_MASK) == LZOP_LITV) {
			len = (size_t)((op64 >> LZOP_LITV_NBITS) + 1);
			DBG_LZLIT(s - s_bk, len);
			SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
			sdec_lz_load_lit(&s, &o, len);
			continue;
		}
		/* LZOP_REFVV */
		len = (size_t)((op64 >> LZOP_REFVV_NBITS) + 4);
		dist = (size_t)(s_ld_pk_u64(&s, (size_t)(s_top - s)) + 1);
		SDEC_LZ_ILOOP_OVERFLOW_CHECK(s, s_top, o, o_top, len);
		sdec_lz_load_ref(&o, dist, len);
		DBG_LZREF(s - s_bk, dist, len, "[REFVV]");
	}
	return (size_t)(o - o0);
}

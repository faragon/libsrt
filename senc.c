/*
 * senc.c
 *
 * Buffer encoding/decoding
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "senc.h"
#include <stdlib.h>

/*
 * Constants
 */

static const char b64e[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b',
	'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
	'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
	};
static const char b64d[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0,
	0, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 0, 0, 0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 32, 33,
	34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
	51, 0, 0, 0, 0, 0
	};
static const char n2h_l[16] = {
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 97, 98, 99, 100, 101, 102
	};
static const char n2h_u[16] = {
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 65, 66, 67, 68, 69, 70
	};
static const char h2n[64] = {
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
#define DB64C1(a, b)	(a << 2 | b >> 4)
#define DB64C2(b, c)	(b << 4 | c >> 2)
#define DB64C3(c, d)	(c << 6 | d)

/*
 * Internal functions
 */

static int hex2nibble(const int h)
{
	return h2n[(h - 48) & 0x3f];
}

static size_t senc_hex_aux(const unsigned char *s, const size_t ss, unsigned char *o, const char *t)
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

#ifdef STANDALONE_TEST

static int syntax_error(const char **argv, const int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Error [%i] Syntax: %s [-cb|-db|-ch|-cH|-dh]\nExamples:\n"
		"%s -cb <in >out.b64\n%s -db <in.b64 >out\n"
		"%s -ch <in >out.hex\n%s -cH <in >out.HEX\n"
		"%s -dh <in.hex >out\n%s -dh <in.HEX >out\n",
		exit_code, v0, v0, v0, v0, v0, v0, v0);
	return exit_code;
}

int main(int argc, const char **argv)
{
	int i = 0;
	size_t lo = 0;
	char buf[128 * 1024];
	char bufo[384 * 1024];
	int f_in = 0, f_out = 1;
#if 0
	f_in = posix_open("..\\test", O_RDONLY | O_BINARY);
	f_out = posix_open("..\\test2.b64", O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0622);
#endif
	ssize_t l = posix_read(f_in, buf, sizeof(buf));
	if (l < 0 || argc < 2)
		return syntax_error(argv, 1);
	const int mode = !strcmp(argv[1], "-eb") ? 1 : !strcmp(argv[1], "-db") ? 2 :
			 !strcmp(argv[1], "-eh") ? 3 : !strcmp(argv[1], "-eH") ? 4 :
			 !strcmp(argv[1], "-dh") ? 5 : 0;
	if (!mode)
		return syntax_error(argv, 2);
	for (; i < 100000; i++)
	{
		switch (mode) {
		case 1: lo = senc_b64(buf, (size_t)l, bufo); break;
		case 2: lo = sdec_b64(buf, (size_t)l, bufo); break;
		case 3: lo = senc_hex(buf, (size_t)l, bufo); break;
		case 4: lo = senc_HEX(buf, (size_t)l, bufo); break;
		case 5: lo = sdec_hex(buf, (size_t)l, bufo); break;
		}
	}
	posix_write(f_out, bufo, lo);
	fprintf(stderr, "in: %u * %i bytes, out: %u * %i bytes\n", (unsigned)l, i, (unsigned)lo, i);
	return 0;
}
#endif


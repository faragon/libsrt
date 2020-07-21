/*
 * schar.c
 *
 * Unicode processing helper functions.
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "schar.h"

/*
 * Togglable options
 *
 * S_ENABLE_UTF8_CHAR_COUNT_HEURISTIC_OPTIMIZATION:
 *  ~370% faster for 1 byte UTF-8 characters
 *   ~10% slower for 2 byte UTF-8 characters
 *   ~15% slower for 3 byte UTF-8 characters
 *   ~23% slower for 4 byte UTF-8 characters
 *   ~15% slower for 5 and 6 byte UTF-8 characters (estimation)
 * This optimization increases Unicode counting string speed.
 * In most cases the code will perform better with this enabled, as most
 * frequently used separators and arithmetics characters already use one
 * byte, even if you're using e.g. Asian languages.
 */

#define S_ENABLE_UTF8_CHAR_COUNT_HEURISTIC_OPTIMIZATION

#ifdef S_MINIMAL
#undef S_ENABLE_UTF8_CHAR_COUNT_HEURISTIC_OPTIMIZATION
#endif

/*
 * Macros
 */

#define SSU8_SZ1(c) (((c)&SSU8_M1) == SSU8_S1)
#define SSU8_SZ2(c) (((c)&SSU8_M2) == SSU8_S2)
#define SSU8_SZ3(c) (((c)&SSU8_M3) == SSU8_S3)
#define SSU8_SZ4(c) (((c)&SSU8_M4) == SSU8_S4)
#define SSU8_SZ5(c) (((c)&SSU8_M5) == SSU8_S5)
#define SSU8_SZ6(c) (((c)&SSU8_M6) == SSU8_S6)

/* sc_tolower/sc_toupper helpers */

/* return if equal */
#define RE(c, v, r)                                                            \
	if (c == v)                                                            \
	return (r)
/* return if in range */
#define RR(c, l, u, v)                                                         \
	if (c >= l && c <= u)                                                  \
	return (v)
/* return if in range, value for odd and default */
#define RRO(c, l, u, v)                                                        \
	if (c >= l && c <= u) {                                                \
		if (c % 2)                                                     \
			return v;                                              \
		else                                                           \
			return c;                                              \
	}
/* return if in range, value for even and default */
#define RRE(c, l, u, v)                                                        \
	if (c >= l && c <= u) {                                                \
		if (!(c % 2))                                                  \
			return v;                                              \
		else                                                           \
			return c;                                              \
	}

/*
 * Functions
 */

/* clang-format off */
size_t sc_utf8_char_size(const char *s, size_t off, size_t max_off,
			 size_t *enc_errors)
{
	int c;
	size_t char_size;
	if (!s || off >= max_off)
		return 0;
	c = s[off];
	char_size = SSU8_SZ1(c)? 1 : SSU8_SZ2(c)? 2 : SSU8_SZ3(c)? 3 :
		    SSU8_SZ4(c)? 4 : SSU8_SZ5(c)? 5 : SSU8_SZ6(c)? 6 : 1;
	RETURN_IF((off + char_size) <= max_off, char_size);
	/* BEHAVIOR:
	 * On encoding errors, increase the error count, and return 1 as size
	 */
	if (enc_errors)
		*enc_errors = s_size_t_add(*enc_errors, 1, S_NPOS);
	return 1;
}
/* clang-format on */

size_t sc_utf8_count_chars(const char *s, size_t s_size, size_t *enc_errors)
{
#ifdef S_ENABLE_UTF8_CHAR_COUNT_HEURISTIC_OPTIMIZATION
	union s_u32 m1;
	size_t size_cutted;
#endif
	size_t i, unicode_sz;
	if (!s || !s_size)
		return 0;
	i = unicode_sz = 0;
#ifdef S_ENABLE_UTF8_CHAR_COUNT_HEURISTIC_OPTIMIZATION
	size_cutted = s_size >= 6 ? s_size - 6 : 0;
	m1.b[0] = m1.b[1] = m1.b[2] = m1.b[3] = SSU8_M1;
	for (; i < size_cutted;) {
		if ((S_LD_U32(s + i) & m1.a32) == 0) {
			i += 4;
			unicode_sz += 4;
			continue;
		}
		i += sc_utf8_char_size(s, i, s_size, enc_errors);
		unicode_sz++;
	}
#endif
	for (; i < s_size;
	     i += sc_utf8_char_size(s, i, s_size, enc_errors), unicode_sz++)
		;
	return unicode_sz;
}

/* clang-format off */
size_t sc_wc_to_utf8_size(int32_t c)
{
	return c <= 0x7f ? 1 : c <= 0x7ff? 2 : c <= 0xffff? 3 :
		c <= 0x1fffff? 4 : c <= 0x3ffffff? 5 : 6;
}

/* clang-format on */
#define SC_WC2UTF8_x(s, off, len, c, n)                                        \
	s[off + n - 1] = (char)(SSU8_SX | ((c >> (6 * (len - n))) & SSUB_MX));
#define SC_WC2UTF8_2(s, off, len, c) SC_WC2UTF8_x(s, off, len, c, 2)
#define SC_WC2UTF8_3(s, off, len, c)                                           \
	SC_WC2UTF8_2(s, off, len, c)                                           \
	SC_WC2UTF8_x(s, off, len, c, 3)
#define SC_WC2UTF8_4(s, off, len, c)                                           \
	SC_WC2UTF8_3(s, off, len, c)                                           \
	SC_WC2UTF8_x(s, off, len, c, 4)
#define SC_WC2UTF8_5(s, off, len, c)                                           \
	SC_WC2UTF8_3(s, off, len, c)                                           \
	SC_WC2UTF8_x(s, off, len, c, 5)
#define SC_WC2UTF8_6(s, off, len, c)                                           \
	SC_WC2UTF8_3(s, off, len, c)                                           \
	SC_WC2UTF8_x(s, off, len, c, 6)

size_t sc_wc_to_utf8(int32_t c, char *s, size_t off, size_t max_off)
{
	size_t len = sc_wc_to_utf8_size(c);
	if (s && off < max_off) {
		switch (len) {
		case 1:
			s[off] = (char)(SSU8_S1 | (c & 0x7f));
			break;
		case 2:
			s[off] = (char)(SSU8_S2 | (c >> 6));
			SC_WC2UTF8_2(s, off, len, c);
			break;
		case 3:
			s[off] = (char)(SSU8_S3 | (c >> 12));
			SC_WC2UTF8_3(s, off, len, c);
			break;
		case 4:
			s[off] = (char)(SSU8_S4 | (c >> 18));
			SC_WC2UTF8_4(s, off, len, c);
			break;
		case 5:
			s[off] = (char)(SSU8_S5 | (c >> 24));
			SC_WC2UTF8_5(s, off, len, c);
			break;
		case 6:
			s[off] = (char)(SSU8_S6 | (c >> 30));
			SC_WC2UTF8_6(s, off, len, c);
			break;
		}
	}
	return len;
}

#define SC_U82WC_x(o, s, off, size, n)                                         \
	o |= (s[off + n - 1] & SSUB_MX) << (6 * (size - n))
#define SC_U82WC_2(o, s, off, size) SC_U82WC_x(o, s, off, size, 2)
#define SC_U82WC_3(o, s, off, size)                                            \
	SC_U82WC_2(o, s, off, size);                                           \
	SC_U82WC_x(o, s, off, size, 3)
#define SC_U82WC_4(o, s, off, size)                                            \
	SC_U82WC_3(o, s, off, size);                                           \
	SC_U82WC_x(o, s, off, size, 4)
#define SC_U82WC_5(o, s, off, size)                                            \
	SC_U82WC_4(o, s, off, size);                                           \
	SC_U82WC_x(o, s, off, size, 5)
#define SC_U82WC_6(o, s, off, size)                                            \
	SC_U82WC_5(o, s, off, size);                                           \
	SC_U82WC_x(o, s, off, size, 6)
#define SC_U82WC_RETURN(out, outp, size)                                       \
	if (outp)                                                              \
		*(outp) = out;                                                 \
	return size

/* BEHAVIOR: always return a character. For broken UTF-8, return the first
   byte as character. */
size_t sc_utf8_to_wc(const char *s, size_t off, size_t max_off,
		     int32_t *unicode_out, size_t *encoding_errors)
{
	int32_t c;
	if (s && off < max_off && unicode_out) {
		c = s[off];
		if (SSU8_SZ1(c)) {
			SC_U82WC_RETURN(c, unicode_out, 1);
		} else if (SSU8_SZ2(c)) {
			if ((off + 2) <= max_off) {
				c &= 0x1f;
				c <<= (6 * (2 - 1));
				SC_U82WC_2(c, s, off, 2);
				SC_U82WC_RETURN(c, unicode_out, 2);
			}
		} else if (SSU8_SZ3(c)) {
			if ((off + 3) <= max_off) {
				c &= 0x0f;
				c <<= (6 * (3 - 1));
				SC_U82WC_3(c, s, off, 3);
				SC_U82WC_RETURN(c, unicode_out, 3);
			}
		} else if (SSU8_SZ4(c)) {
			if ((off + 4) <= max_off) {
				c &= 0x07;
				c <<= (6 * (4 - 1));
				SC_U82WC_4(c, s, off, 4);
				SC_U82WC_RETURN(c, unicode_out, 4);
			}
		} else if (SSU8_SZ5(c)) {
			if ((off + 5) <= max_off) {
				c &= 0x03;
				c <<= (6 * (5 - 1));
				SC_U82WC_5(c, s, off, 5);
				SC_U82WC_RETURN(c, unicode_out, 5);
			}
		} else if (SSU8_SZ6(c)) {
			if ((off + 6) <= max_off) {
				c &= 0x01;
				c <<= (6 * (6 - 1));
				SC_U82WC_6(c, s, off, 6);
				SC_U82WC_RETURN(c, unicode_out, 6);
			}
		}
	}
	if (encoding_errors)
		*encoding_errors = 1;
	SC_U82WC_RETURN(0, unicode_out, 1);
}

size_t sc_unicode_count_to_utf8_size(const char *s, size_t off, size_t max_off,
				     size_t unicode_count,
				     size_t *actual_unicode_count)
{
	size_t i, unicode_size;
	if (!s || off >= max_off)
		return 0;
	i = off;
	unicode_size = 0;
	for (; i < max_off && unicode_size < unicode_count;
	     i += sc_utf8_char_size(s, i, max_off, NULL), unicode_size++)
		;
	if (actual_unicode_count)
		*actual_unicode_count = unicode_size;
	return i - off;
}

ssize_t sc_utf8_calc_case_extra_size(const char *s, size_t off, size_t s_size,
				     int32_t (*ssc_toX)(int32_t))
{
	int uchr = 0;
	size_t i = off, char_size;
	ssize_t caseXsize = 0;
	for (; i < s_size;) {
		char_size = sc_utf8_to_wc(s, i, s_size, &uchr, NULL);
		i += char_size;
		caseXsize += ((ssize_t)sc_wc_to_utf8_size(ssc_toX(uchr))
			      - (ssize_t)char_size);
	}
	return caseXsize;
}

	/*
	 * Minimal build removes Unicode tolower/toupper support
	 */

#ifdef S_MINIMAL

int32_t sc_tolower(int32_t c)
{
	return tolower(c);
}

int32_t sc_toupper(int32_t c)
{
	return toupper(c);
}

int32_t sc_tolower_tr(int32_t c)
{
	return sc_tolower(c);
}

int32_t sc_toupper_tr(int32_t c)
{
	return sc_toupper(c);
}

/* just do nothing, so the slower fallback is executed */
#define sc_parallel_toX(s, off, max, o, callback) off

#else

/*
 * sc_tolower/sc_toupper are as fast as glibc towlower/towupper, and require
 * no "setlocale" nor Unicode hash tables. The only locale-specific issue comes
 * from Turkish, for that case, sc_tolower_tr/sc_toupper_tr are provided.
 */

/* clang-format off */

int32_t sc_tolower(int32_t c)
{
	int j = c < 0x18e ? 0 : c < 0x1a9 ? 1 : c < 0x1e0 ? 2 :
		c < 0x386 ? 3 : c < 0x460 ? 4 : c < 0x1fb8 ? 5 :
		c < 0x212b ? 6 : c < 0x10428 ? 7 : 8;
	switch (j) {
	case 0: /* < 0x18e */
		if (c < 0x100) {
			RR(c, 0x41, 0x5a, c + 0x20);
			RE(c, 0xd7, c);
			RR(c, 0xc0, 0xde, c + 0x20);
			RE(c, 0xdf, c);
			RE(c, 0xf7, c);
			return c;
		} else if (c < 0x150) {
			RR(c, 0x100, 0x12e, c + !(c % 2));
			RE(c, 0x130, 0x69);
			RR(c, 0x132, 0x136, c + !(c % 2));
			RR(c, 0x139, 0x148, c + (c % 2));
			RR(c, 0x14a, 0x14e, c + !(c % 2));
			return c;
		} else if (c < 0x186) {
			RR(c, 0x150, 0x176, c + !(c % 2));
			RE(c, 0x178, 0xff);
			RR(c, 0x179, 0x17d, c + (c % 2));
			RR(c, 0x182, 0x184, c + !(c % 2));
			RE(c, 0x181, 0x253);
			return c;
		}
		RE(c, 0x186, 0x254);
		RE(c, 0x187, 0x188);
		RE(c, 0x189, 0x256);
		RE(c, 0x18a, 0x257);
		RE(c, 0x18b, 0x18c);
		return c;
	case 1: /* < 0x1a9 */
		if (c < 0x194) {
			RE(c, 0x18e, 0x1dd);
			RE(c, 0x18f, 0x259);
			RE(c, 0x190, 0x25b);
			RE(c, 0x191, 0x192);
			RE(c, 0x193, 0x260);
			return c;
		} else if (c < 0x19d) {
			RE(c, 0x194, 0x263);
			RE(c, 0x196, 0x269);
			RE(c, 0x197, 0x268);
			RE(c, 0x198, 0x199);
			RE(c, 0x19c, 0x26f);
			return c;
		}
		RE(c, 0x19d, 0x272);
		RE(c, 0x19f, 0x275);
		RR(c, 0x1a0, 0x1a4, c + !(c % 2));
		RE(c, 0x1a6, 0x280);
		RE(c, 0x1a7, 0x1a8);
		return c;
	case 2: /* < 0x1e0 */
		if (c < 0x1b2) {
			RE(c, 0x1a9, 0x283);
			RE(c, 0x1ac, 0x1ad);
			RE(c, 0x1ae, 0x288);
			RE(c, 0x1af, 0x1b0);
			RE(c, 0x1b1, 0x28a);
			return c;
		} else if (c < 0x1c4) {
			RE(c, 0x1b2, 0x28b);
			RR(c, 0x1b3, 0x1b5, c + (c % 2));
			RE(c, 0x1b7, 0x292);
			RE(c, 0x1b8, 0x1b9);
			RE(c, 0x1bc, 0x1bd);
			return c;
		}
		if (c < 0x1cb) {
			RE(c, 0x1c4, 0x1c6);
			RE(c, 0x1c5, 0x1c6);
			RE(c, 0x1c7, 0x1c9);
			RE(c, 0x1c8, 0x1c9);
			RE(c, 0x1ca, 0x1cc);
			return c;
		}
		RE(c, 0x1cb, 0x1cc);
		RE(c, 0x1cd, 0x1ce);
		RE(c, 0x1cf, 0x1d0);
		RR(c, 0x1d1, 0x1db, c + (c % 2));
		RE(c, 0x1de, 0x1df);
		return c;
	case 3: /* < 0x386 */
		if (c < 0x1f8) {
			RR(c, 0x1e0, 0x1ee, c + !(c % 2));
			RE(c, 0x1f1, 0x1f3);
			RR(c, 0x1f2, 0x1f4, c + !(c % 2));
			RE(c, 0x1f6, 0x195);
			RE(c, 0x1f7, 0x1bf);
			return c;
		} else if (c < 0x23b) {
			RE(c, 0x1f8, 0x1f9);
			RR(c, 0x1fa, 0x21e, c + !(c % 2));
			RE(c, 0x220, 0x19e);
			RR(c, 0x222, 0x232, c + !(c % 2));
			RE(c, 0x23a, 0x2c65);
			return c;
		} else if (c < 0x244) {
			RE(c, 0x23b, 0x23c);
			RE(c, 0x23d, 0x19a);
			RE(c, 0x23e, 0x2c66);
			RE(c, 0x241, 0x242);
			RE(c, 0x243, 0x180);
			return c;
		}
		RE(c, 0x244, 0x289);
		RE(c, 0x245, 0x28c);
		RR(c, 0x246, 0x24e, c + !(c % 2));
		RR(c, 0x370, 0x372, c + !(c % 2));
		RE(c, 0x376, 0x377);
		RE(c, 0x37f, 0x3f3);
		return c;
	case 4: /* < 0x460 */
		if (c < 0x38e) {
			RE(c, 0x386, 0x3ac);
			RE(c, 0x388, 0x3ad);
			RE(c, 0x389, 0x3ae);
			RE(c, 0x38a, 0x3af);
			RE(c, 0x38c, 0x3cc);
			return c;
		} else if (c < 0x3e0) {
			RR(c, 0x391, 0x3a1, c + 0x20);
			RR(c, 0x3a3, 0x3ab, c + 0x20);
			RR(c, 0x3d8, 0x3de, c + !(c % 2));
			RE(c, 0x38e, 0x3cd);
			RE(c, 0x38f, 0x3ce);
			RE(c, 0x3cf, 0x3d7);
			return c;
		} else if (c < 0x3fd) {
			RR(c, 0x3e0, 0x3ee, c + !(c % 2));
			RE(c, 0x3f4, 0x3b8);
			RE(c, 0x3f7, 0x3f8);
			RE(c, 0x3f9, 0x3f2);
			RE(c, 0x3fa, 0x3fb);
			return c;
		}
		RE(c, 0x3fd, 0x37b);
		RE(c, 0x3fe, 0x37c);
		RE(c, 0x3ff, 0x37d);
		RR(c, 0x400, 0x40f, c + 0x50);
		RR(c, 0x410, 0x42f, c + 0x20);
		return c;
	case 5: /* < 0x1fb8 */
		if (c < 0x531) {
			RRE(c, 0x460, 0x480, c + 1);
			RRE(c, 0x48a, 0x4be, c + 1);
			RE(c, 0x4c0, 0x4cf);
			RR(c, 0x4c1, 0x4cd, c + (c % 2));
			RR(c, 0x4d0, 0x522, c + !(c % 2));
			RRE(c, 0x524, 0x52e, c + 1);
			return c;
		} else if (c < 0x1f08) {
			if (c < 0x13f0) {
				RR(c, 0x531, 0x556, c + 0x30);
				RR(c, 0x10a0, 0x10c5, 0x2d00 + c - 0x10a0);
				RE(c, 0x10c7, 0x2d27);
				RE(c, 0x10cd, 0x2d2d);
				RR(c, 0x13a0, 0x13ef, c + 0x97d0);
			} else {
				RR(c, 0x13f0, 0x13f5, c + 8);
				RR(c, 0x1e00, 0x1e94, c + !(c % 2));
				RR(c, 0x1ea0, 0x1efe, c + !(c % 2));
				RE(c, 0x1e9e, 0xdf);
			}
			return c;
		} else if (c < 0x1f59) {
			RR(c, 0x1f08, 0x1f0f, c - 8);
			RR(c, 0x1f18, 0x1f1d, c - 8);
			RR(c, 0x1f28, 0x1f2f, c - 8);
			RR(c, 0x1f38, 0x1f3f, c - 8);
			RR(c, 0x1f48, 0x1f4d, c - 8);
			return c;
		}
		RRO(c, 0x1f59, 0x1f5f, c - 8);
		RR(c, 0x1f68, 0x1f6f, c - 8);
		RR(c, 0x1f88, 0x1f8f, c - 8);
		RR(c, 0x1f98, 0x1f9f, c - 8);
		RR(c, 0x1fa8, 0x1faf, c - 8);
		return c;
	case 6: /* < 0x212b */
		if (c < 0x1fcc) {
			RR(c, 0x1fb8, 0x1fb9, c - 8);
			RR(c, 0x1fba, 0x1fbb, c - 0x4a);
			RE(c, 0x1fbc, 0x1fb3);
			RR(c, 0x1fc8, 0x1fca, c - 0x56);
			RE(c, 0x1fcb, 0x1f75);
			return c;
		} else if (c < 0x1fe8) {
			RE(c, 0x1fcc, 0x1fc3);
			RE(c, 0x1fd8, 0x1fd0);
			RE(c, 0x1fd9, 0x1fd1);
			RE(c, 0x1fda, 0x1f76);
			RE(c, 0x1fdb, 0x1f77);
			return c;
		} else if (c < 0x1ff8) {
			RE(c, 0x1fe8, 0x1fe0);
			RE(c, 0x1fe9, 0x1fe1);
			RE(c, 0x1fea, 0x1f7a);
			RE(c, 0x1feb, 0x1f7b);
			RE(c, 0x1fec, 0x1fe5);
			return c;
		}
		RR(c, 0x1ff8, 0x1ff9, c - 0x80);
		RR(c, 0x1ffa, 0x1ffb, c - 0x7e);
		RE(c, 0x1ffc, 0x1ff3);
		RE(c, 0x2126, 0x3c9);
		RE(c, 0x212a, 0x6b);
		return c;
	case 7: /* < 0x10428 */
		if (c < 0xa640) {
			if (c < 0x2c00) {
				RE(c, 0x212b, 0xe5);
				RE(c, 0x2132, 0x214e);
				RR(c, 0x2160, 0x216f, c + 0x10);
				RE(c, 0x2183, 0x2184);
				RR(c, 0x24b6, 0x24cf, c + 0x1a);
				return c;
			} else if (c < 0x2c67) {
				RR(c, 0x2c00, 0x2c2e, c + 0x30);
				RE(c, 0x2c60, 0x2c61);
				RE(c, 0x2c62, 0x26b);
				RE(c, 0x2c63, 0x1d7d);
				RE(c, 0x2c64, 0x27d);
				return c;
			} else if (c < 0x2c6f) {
				RE(c, 0x2c67, 0x2c68);
				RE(c, 0x2c69, 0x2c6a);
				RE(c, 0x2c6b, 0x2c6c);
				RE(c, 0x2c6d, 0x251);
				RE(c, 0x2c6e, 0x271);
				return c;
			}
			RE(c, 0x2c6f, 0x250);
			RE(c, 0x2c70, 0x252);
			RE(c, 0x2c72, 0x2c73);
			RE(c, 0x2c75, 0x2c76);
			RE(c, 0x2c7e, 0x23f);
			RE(c, 0x2c7f, 0x240);
			RR(c, 0x2c80, 0x2ce2, c + !(c % 2));
			RE(c, 0x2ceb, 0x2cec);
			RE(c, 0x2ced, 0x2cee);
			RE(c, 0x2cf2, 0x2cf3);
		} else {
			if (c < 0xa7aa) {
				if (c < 0xa780) {
					RRE(c, 0xa640, 0xa66c, c + 1);
					RRE(c, 0xa680, 0xa69a, c + 1);
					RRE(c, 0xa722, 0xa72e, c + 1);
					RRE(c, 0xa732, 0xa76e, c + 1);
					RRO(c, 0xa779, 0xa77b, c + 1);
					RE(c, 0xa77d, 0x1d79);
					RE(c, 0xa77e, 0xa77f);
				} else {
					RRE(c, 0xa780, 0xa786, c + 1);
					RE(c, 0xa78b, 0xa78c);
					RE(c, 0xa78d, 0x265);
					RRE(c, 0xa790, 0xa792, c + 1);
					RRE(c, 0xa796, 0xa7a8, c + 1);
				}
			} else {
				if (c < 0xa7b4) {
					RE(c, 0xa7aa, 0x266);
					RE(c, 0xa7ab, 0x25c);
					RE(c, 0xa7ac, 0x261);
					RE(c, 0xa7ad, 0x26c);
					RE(c, 0xa7ae, 0x26a);
					RE(c, 0xa7b0, 0x29e);
					RE(c, 0xa7b1, 0x287);
					RE(c, 0xa7b2, 0x29d);
					RE(c, 0xa7b3, 0xab53);
				} else {
					RRE(c, 0xa7b4, 0xa7b6, c + 1);
					RR(c, 0xff21, 0xff3a, c + 0x20);
					RR(c, 0x10400, 0x10427, c + 0x28);
				}
			}
		}
		return c;
	case 8: /* >= 0x10428 */
		RR(c, 0x104b0, 0x104d3, c + 0x28);
		RR(c, 0x10c80, 0x10cb2, c + 0x40);
		RR(c, 0x118a0, 0x118bf, c + 0x20);
		RR(c, 0x1e900, 0x1e921, c + 0x22);
		return c;
	}
	return c;
}

int32_t sc_toupper(int32_t c)
{
	int j = c < 0x199 ? 0 : c < 0x1dd ? 1 : c < 0x260 ? 2 :
		c < 0x377 ? 3 : c < 0x3e1 ? 4 : c < 0x1ea1 ? 5 :
		c < 0x1fb0 ? 6 : c < 0x10450 ? 7 : 8;
	switch (j) {
	case 0: /* < 0x199 */
		if (c < 0x101) {
			RR(c, 0x61, 0x7a, c - 0x20);
			RE(c, 0xb5, 0x39c);
			RE(c, 0xf7, c);
			RR(c, 0xe0, 0xfe, c - 0x20);
			RE(c, 0xff, 0x178);
			return c;
		} else if (c < 0x14f) {
			RR(c, 0x101, 0x12f, c - (c % 2));
			RE(c, 0x131, 0x49);
			RR(c, 0x133, 0x137, c - (c % 2));
			RR(c, 0x139, 0x148, c - !(c % 2));
			RR(c, 0x14a, 0x14e, c - (c % 2));
			return c;
		} else if (c < 0x183) {
			RE(c, 0x14f, 0x14e);
			RR(c, 0x151, 0x177, c - (c % 2));
			RR(c, 0x179, 0x17e, c - !(c % 2));
			RE(c, 0x17f, 0x53);
			RE(c, 0x180, 0x243);
			return c;
		}
		RR(c, 0x183, 0x185, c - (c % 2));
		RE(c, 0x188, 0x187);
		RE(c, 0x18c, 0x18b);
		RE(c, 0x192, 0x191);
		RE(c, 0x195, 0x1f6);
		return c;
	case 1: /* < 0x1dd */
		if (c < 0x1ad) {
			RE(c, 0x199, 0x198);
			RE(c, 0x19a, 0x23d);
			RE(c, 0x19e, 0x220);
			RR(c, 0x1a1, 0x1a5, c - (c % 2));
			RE(c, 0x1a8, 0x1a7);
			return c;
		} else if (c < 0x1bf) {
			RE(c, 0x1ad, 0x1ac);
			RE(c, 0x1b0, 0x1af);
			RR(c, 0x1b4, 0x1b6, c - !(c % 2));
			RE(c, 0x1b9, 0x1b8);
			RE(c, 0x1bd, 0x1bc);
			return c;
		} else if (c < 0x1cb) {
			RE(c, 0x1bf, 0x1f7);
			RE(c, 0x1c5, 0x1c4);
			RE(c, 0x1c6, 0x1c4);
			RE(c, 0x1c8, 0x1c7);
			RE(c, 0x1c9, 0x1c7);
			return c;
		}
		RE(c, 0x1cb, 0x1ca);
		RE(c, 0x1cc, 0x1ca);
		RE(c, 0x1ce, 0x1cd);
		RE(c, 0x1d0, 0x1cf);
		RR(c, 0x1d2, 0x1dc, c - !(c % 2));
		return c;
	case 2: /* < 0x260 */
		if (c < 0x1f5) {
			RE(c, 0x1dd, 0x18e);
			RE(c, 0x1df, 0x1de);
			RR(c, 0x1e1, 0x1ef, c - (c % 2));
			RE(c, 0x1f2, 0x1f1);
			RE(c, 0x1f3, 0x1f1);
			return c;
		} else if (c < 0x242) {
			RE(c, 0x1f5, 0x1f4);
			RE(c, 0x1f9, 0x1f8);
			RR(c, 0x1fb, 0x21f, c - (c % 2));
			RR(c, 0x223, 0x233, c - (c % 2));
			RE(c, 0x23c, 0x23b);
			RR(c, 0x23f, 0x240, c + 0x2a3f);
			return c;
		} else if (c < 0x254) {
			RE(c, 0x242, 0x241);
			RR(c, 0x247, 0x24f, c - (c % 2));
			RE(c, 0x250, 0x2c6f);
			RE(c, 0x251, 0x2c6d);
			RE(c, 0x252, 0x2c70);
			RE(c, 0x253, 0x181);
			return c;
		}
		RE(c, 0x254, 0x186);
		RE(c, 0x256, 0x189);
		RE(c, 0x257, 0x18a);
		RE(c, 0x259, 0x18f);
		RE(c, 0x25b, 0x190);
		RE(c, 0x25c, 0xa7ab);
		return c;
	case 3: /* < 0x377 */
		if (c < 0x26f) {
			RE(c, 0x260, 0x193);
			RE(c, 0x261, 0xa7ac);
			RE(c, 0x263, 0x194);
			RE(c, 0x265, 0xa78d);
			RE(c, 0x266, 0xa7aa);
			RE(c, 0x268, 0x197);
			RE(c, 0x269, 0x196);
			RE(c, 0x26a, 0xa7ae);
			RE(c, 0x26b, 0x2c62);
			RE(c, 0x26c, 0xa7ad);
			return c;
		} else if (c < 0x280) {
			RE(c, 0x26f, 0x19c);
			RE(c, 0x271, 0x2c6e);
			RE(c, 0x272, 0x19d);
			RE(c, 0x275, 0x19f);
			RE(c, 0x27d, 0x2c64);
			return c;
		} else if (c < 0x28b) {
			RE(c, 0x280, 0x1a6);
			RE(c, 0x283, 0x1a9);
			RE(c, 0x287, 0xa7b1);
			RE(c, 0x288, 0x1ae);
			RE(c, 0x289, 0x244);
			RE(c, 0x28a, 0x1b1);
			return c;
		}
		RE(c, 0x29d, 0xa7b2);
		RE(c, 0x29e, 0xa7b0);
		RE(c, 0x28b, 0x1b2);
		RE(c, 0x28c, 0x245);
		RE(c, 0x292, 0x1b7);
		RE(c, 0x345, 0x399);
		RR(c, 0x371, 0x373, c - (c % 2));
		return c;
	case 4: /* < 0x3e1 */
		if (c < 0x3ad) {
			RE(c, 0x377, 0x376);
			RE(c, 0x37b, 0x3fd);
			RE(c, 0x37c, 0x3fe);
			RE(c, 0x37d, 0x3ff);
			RE(c, 0x3ac, 0x386);
			return c;
		} else if (c < 0x3c3) {
			RE(c, 0x3ad, 0x388);
			RE(c, 0x3ae, 0x389);
			RE(c, 0x3af, 0x38a);
			RR(c, 0x3b1, 0x3c1, c - 0x20);
			RE(c, 0x3c2, 0x3a3);
			return c;
		} else if (c < 0x3d0) {
			RR(c, 0x3c3, 0x3cb, c - 0x20);
			RE(c, 0x3c9, 0x2126);
			RE(c, 0x3cc, 0x38c);
			RE(c, 0x3cd, 0x38e);
			RE(c, 0x3ce, 0x38f);
			return c;
		}
		RE(c, 0x3d0, 0x392);
		RE(c, 0x3d1, 0x398);
		RE(c, 0x3d7, 0x3cf);
		RE(c, 0x3d5, 0x3a6);
		RE(c, 0x3d6, 0x3a0);
		RR(c, 0x3d9, 0x3df, c - (c % 2));
		return c;
	case 5: /* < 0x1ea1 */
		if (c < 0x3f8) {
			RR(c, 0x3e1, 0x3ef, c - (c % 2));
			RE(c, 0x3f0, 0x39a);
			RE(c, 0x3f1, 0x3a1);
			RE(c, 0x3f2, 0x3f9);
			RE(c, 0x3f3, 0x37f);
			RE(c, 0x3f5, 0x395);
			return c;
		} else if (c < 0x48b) {
			RE(c, 0x3f8, 0x3f7);
			RE(c, 0x3fb, 0x3fa);
			RR(c, 0x430, 0x44f, c - 0x20);
			RR(c, 0x450, 0x45f, c - 0x50);
			RR(c, 0x461, 0x481, c - (c % 2));
			return c;
		} else if (c < 0x523) {
			RR(c, 0x48b, 0x48f, c - (c % 2));
			RR(c, 0x491, 0x4bf, c - (c % 2));
			RR(c, 0x4c2, 0x4ce, c - !(c % 2));
			RE(c, 0x4cf, 0x4c0);
			RR(c, 0x4d0, 0x522, c - (c % 2));
			return c;
		}
		RE(c, 0x523, 0x522);
		RR(c, 0x561, 0x586, c - 0x30);
		RRO(c, 0x525, 0x52f, c - 1);
		RR(c, 0x13f8, 0x13fd, c - 8);
		RE(c, 0x1c80, 0x412);
		RE(c, 0x1c81, 0x414);
		RE(c, 0x1c82, 0x41e);
		RE(c, 0x1c83, 0x421);
		RE(c, 0x1c84, 0x422);
		RE(c, 0x1c85, 0x422);
		RE(c, 0x1c86, 0x42a);
		RE(c, 0x1c87, 0x462);
		RE(c, 0x1c88, 0xa64a);
		RE(c, 0x1d79, 0xa77d);
		RE(c, 0x1d7d, 0x2c63);
		RR(c, 0x1e01, 0x1e95, c - (c % 2));
		RE(c, 0x1e9b, 0x1e60);
		return c;
	case 6: /* < 0x1fb0 */
		if (c < 0x1f40) {
			RR(c, 0x1ea1, 0x1eff, c - (c % 2));
			RR(c, 0x1f00, 0x1f07, c + 8);
			RR(c, 0x1f10, 0x1f15, c + 8);
			RR(c, 0x1f20, 0x1f27, c + 8);
			RR(c, 0x1f30, 0x1f37, c + 8);
			return c;
		} else if (c < 0x1f75) {
			RR(c, 0x1f40, 0x1f45, c + 8);
			RRO(c, 0x1f51, 0x1f57, c + 8);
			RR(c, 0x1f60, 0x1f67, c + 8);
			RR(c, 0x1f70, 0x1f71, c + 0x4a);
			RR(c, 0x1f72, 0x1f74, c + 0x56);
			return c;
		} else if (c < 0x1f7b) {
			RE(c, 0x1f75, 0x1fcb);
			RE(c, 0x1f76, 0x1fda);
			RE(c, 0x1f77, 0x1fdb);
			RR(c, 0x1f78, 0x1f79, c + 0x80);
			return 0x1fea; /* 0x1f7a -> 0x1fea */
		}
		RE(c, 0x1f7b, 0x1feb);
		RR(c, 0x1f7c, 0x1f7d, c + 0x7e);
		RR(c, 0x1f80, 0x1f87, c + 8);
		RR(c, 0x1f90, 0x1f97, c + 8);
		RR(c, 0x1fa0, 0x1fa7, c + 8);
		return c;
	case 7: /* < 0x10450 */
		if (c < 0x2d27) {
			if (c < 0x1fd1) {
				RR(c, 0x1fb0, 0x1fb1, c + 8);
				RE(c, 0x1fb3, 0x1fbc);
				RE(c, 0x1fbe, 0x399);
				RE(c, 0x1fc3, 0x1fcc);
				RE(c, 0x1fd0, 0x1fd8);
				return c;
			} else if (c < 0x214e) {
				RE(c, 0x1fd1, 0x1fd9);
				RE(c, 0x1fe0, 0x1fe8);
				RE(c, 0x1fe1, 0x1fe9);
				RE(c, 0x1fe5, 0x1fec);
				RE(c, 0x1ff3, 0x1ffc);
				return c;
			} else if (c < 0x2c61) {
				RE(c, 0x214e, 0x2132);
				RR(c, 0x2170, 0x217f, c - 0x10);
				RE(c, 0x2184, 0x2183);
				RR(c, 0x24d0, 0x24e9, c - 0x1a);
				RR(c, 0x2c30, 0x2c5e, c - 0x30);
				return c;
			} else if (c < 0x2c6c) {
				RE(c, 0x2c61, 0x2c60);
				RE(c, 0x2c65, 0x23a);
				RE(c, 0x2c66, 0x23e);
				RE(c, 0x2c68, 0x2c67);
				RE(c, 0x2c6a, 0x2c69);
				return c;
			} else if (c < 0x2d00) {
				RRE(c, 0x2cec, 0x2cee, c - 1);
				RE(c, 0x2cf3, 0x2cf2);
				RE(c, 0x2c6c, 0x2c6b);
				RE(c, 0x2c73, 0x2c72);
				RE(c, 0x2c76, 0x2c75);
				RR(c, 0x2c81, 0x2ce3, c - (c % 2));
				return c;
			}
			RR(c, 0x2d00, 0x2d25, 0x10a0 + c - 0x2d00);
			return c;
		} else {
			if (c < 0xa791) {
				if (c < 0xa723) {
					RE(c, 0x2d27, 0x10c7);
					RE(c, 0x2d2d, 0x10cd);
					RRO(c, 0xa641, 0xa66d, c - 1);
					RRO(c, 0xa681, 0xa69b, c - 1);
				} else {
					RRO(c, 0xa723, 0xa72f, c - 1);
					RRO(c, 0xa733, 0xa76f, c - 1);
					RRE(c, 0xa77a, 0xa77c, c - 1);
					RRO(c, 0xa77f, 0xa787, c - 1);
					RE(c, 0xa78c, 0xa78b);
				}
			} else {
				if (c < 0xab70) {
					RRO(c, 0xa791, 0xa793, c - 1);
					RRO(c, 0xa797, 0xa7a9, c - 1);
					RRO(c, 0xa7b5, 0xa7b7, c - 1);
					RE(c, 0xab53, 0xa7b3);
				} else {
					RR(c, 0xab70, 0xabbf, c - 0x97d0);
					RR(c, 0xff41, 0xff5a, c - 0x20);
					RR(c, 0x10428, 0x1044f, c - 0x28);
				}
			}
		}
		return c;
	case 8: /* >= 0x10450 */
		RR(c, 0x104d8, 0x104fb, c - 0x28);
		RR(c, 0x10cc0, 0x10cf2, c - 0x40);
		RR(c, 0x118c0, 0x118df, c - 0x20);
		RR(c, 0x1e922, 0x1e943, c - 0x22);
		return c;
	}
	return c;
}

/* clang-format on */

int32_t sc_tolower_tr(int32_t c)
{
	RE(c, 0x49, 0x131); /* 'I' to dotless 'i' */
	RE(c, 0x130, 0x69); /* 'I' with dot to 'i' */
	return sc_tolower(c);
}

int32_t sc_toupper_tr(int32_t c)
{
	RE(c, 0x131, 0x49);
	RE(c, 0x69, 0x130);
	return sc_toupper(c);
}

/*
 * 7-bit parallel case conversions (using the Paul Hsieh technique)
 */
size_t sc_parallel_toX(const char *s, size_t off, size_t max, char *o,
		       int32_t (*ssc_toX)(int32_t))
{
	uint32_t a, b;
	union s_u32 m1;
	int op_mod = ssc_toX == sc_tolower ? 1 : 0;
	uint32_t msk1 = 0x7f7f7f7f, msk2 = 0x1a1a1a1a, msk3 = 0x20202020;
	uint32_t msk4 = op_mod ? 0x25252525 : 0x05050505;
	size_t szm4 = max & (size_t)(~3);
	m1.b[0] = m1.b[1] = m1.b[2] = m1.b[3] = SSU8_SX;
	for (; off < szm4; off += 4) {
		a = S_LD_U32(s + off);
		if ((a & m1.a32) == 0) {
			b = (msk1 & a) + msk4;
			b = (msk1 & b) + msk2;
			b = ((b & ~a) >> 2) & msk3;
			S_ST_U32(o, op_mod ? a + b : a - b);
			o += 4;
		} else { /* Not 7-bit ASCII */
			break;
		}
	}
	return off;
}

#endif /* S_MINIMAL */

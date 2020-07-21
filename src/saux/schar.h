#ifndef SCHAR_H
#define SCHAR_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * schar.h
 *
 * Unicode processing helper functions.
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 *
 * Designed to be used by libraries or wrapped by some abstraction (e.g.
 * srt_string/libsrt), not as general-purpose direct usage.
 *
 * Features:
 *
 * - Unicode <-> UTF-8 character conversion.
 * - Compute Unicode required size for UTF-8 representation.
 * - Count Unicode characters into UTF-8 stream.
 * - Lowercase/uppercase conversions.
 * - Not relies on C library nor OS Unicode support ("locale").
 * - Small memory footprint (not using hash tables).
 * - Fast
 *
 * Observations:
 * - Turkish (tr_CY/tr_TR) is an especial case that breaks the possibility
 *   of generic case conversion, because of the 'i' <-> 'I' collision with
 *   other case conversion for other languages using the latin alphabet, as
 *   in that locale there are two lowercase 'i', with and without dot, and
 *   also two uppercase 'I' with, and without dot, so the 'i' <-> 'I' don't
 *   applies. For covering that case, equivalent to having the tr_* locale
 *   set, sc_tolower_tr()/sc_toupper_tr() are provided.
 */

#include "scommon.h"

enum SSUTF8 {
	SSU8_SX = 0x80,
	SSU8_S1 = 0x00,
	SSU8_S2 = 0xc0,
	SSU8_S3 = 0xe0,
	SSU8_S4 = 0xf0,
	SSU8_S5 = 0xf8,
	SSU8_S6 = 0xfc,
	SSUB_MX = 0x3f,
	SSU8_M1 = 0x80,
	SSU8_M2 = 0xe0,
	SSU8_M3 = 0xf0,
	SSU8_M4 = 0xf8,
	SSU8_M5 = 0xfc,
	SSU8_M6 = 0xfe,
	SSU8_MAX_SIZE = 6
};

enum SSUTF16 {
	SSU16_HS0 = 0xd800, /* high surrogate range */
	SSU16_HSN = 0xdbff,
	SSU16_LS0 = 0xdc00, /* low surrogate range */
	SSU16_LSN = 0xdfff,
	SSU16_SM = 0xfc00,
	SSU16_SMI = 0x03ff
};

#define SSU8_VALID_START(c) (((c)&0xc0) != SSU8_SX)
#define SSU16_SIMPLE(c)                                                        \
	(((unsigned short)(c)) < SSU16_HS0 || ((unsigned short)(c)) > SSU16_LSN)
#define SSU16_VALID_HS(c) (((c)&SSU16_SM) == SSU16_HS0)
#define SSU16_VALID_LS(c) (((c)&SSU16_SM) == SSU16_LS0)
#define SSU16_TO_U32(hs, ls) ((((hs)&SSU16_SMI) << 10) | ((ls)&SSU16_SMI))

size_t sc_utf8_char_size(const char *s, size_t off, size_t max_off,
			 size_t *enc_errors);
size_t sc_utf8_count_chars(const char *s, size_t s_size, size_t *enc_errors);
size_t sc_wc_to_utf8_size(int32_t c);
size_t sc_wc_to_utf8(int32_t c, char *s, size_t off, size_t max_off);
size_t sc_utf8_to_wc(const char *s, size_t off, size_t max_off,
		     int32_t *unicode_out, size_t *encoding_errors);
size_t sc_unicode_count_to_utf8_size(const char *, size_t off, size_t max_off,
				     size_t unicode_count,
				     size_t *actual_unicode_count);
ssize_t sc_utf8_calc_case_extra_size(const char *s, size_t off, size_t s_size,
				     int32_t (*ssc_toX)(int32_t));
int32_t sc_tolower(int32_t c);
int32_t sc_toupper(int32_t c);
int32_t sc_tolower_tr(int32_t c);
int32_t sc_toupper_tr(int32_t c);
size_t sc_parallel_toX(const char *s, size_t off, size_t max, char *o,
		       int32_t (*ssc_toX)(int32_t));

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* SCHAR_H */

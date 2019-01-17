/*
 * utf8_examples.h
 *
 * UTF8 constants used by libsrt tests and examples
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#ifndef UTF8_EXAMPLES_H
#define UTF8_EXAMPLES_H

#define U8_C_N_TILDE_D1 "\xc3\x91"
#define U8_S_N_TILDE_F1 "\xc3\xb1"
#define U8_S_I_DOTLESS_131 "\xc4\xb1" /* Turkish small I without dot */
#define U8_C_I_DOTTED_130 "\xc4\xb0"  /* Turkish capital I with dot */
#define U8_C_G_BREVE_11E "\xc4\x9e"
#define U8_S_G_BREVE_11F "\xc4\x9f"
#define U8_C_S_CEDILLA_15E "\xc5\x9e"
#define U8_S_S_CEDILLA_15F "\xc5\x9f"
#define U8_CENT_00A2 "\xc2\xa2"
#define U8_EURO_20AC "\xe2\x82\xac"
#define U8_HAN_24B62 "\xf0\xa4\xad\xa2"
#define U8_HAN_611B "\xe6\x84\x9b"

#define U8_MIX1                                                                \
	U8_C_N_TILDE_D1 U8_S_N_TILDE_F1 U8_S_I_DOTLESS_131 U8_C_I_DOTTED_130   \
		U8_C_G_BREVE_11E U8_S_G_BREVE_11F U8_C_S_CEDILLA_15E           \
			U8_S_S_CEDILLA_15F U8_CENT_00A2 U8_EURO_20AC           \
				U8_HAN_24B62
#define U8_MIX_28_bytes U8_MIX1 U8_HAN_611B

#define U8_MANY_UNDERSCORES                                                    \
	"____________________________________________________________________" \
	"____________________________________________________________________" \
	"____________________________________________________________________" \
	"____________________________________________________________________"

#endif /* UTF8_EXAMPLES_H */

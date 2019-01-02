#ifndef SENC_H
#define SENC_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * senc.h
 *
 * Buffer encoding/decoding
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 *
 * Features (base64 and hex encoding/decoding):
 *
 * - Aliasing safe (input and output buffer can be the same).
 * - RFC 3548/4648 base 16 (hexadecimal) and 64 encoding/decoding.
 * - Fast (~1 GB/s on i5-3330 @3GHz -using one core- and gcc 4.8.2 -O2)
 *
 * Features (JSON and XML escape/unescape):
 *
 * - Aliasing safe.
 * - JSON escape subset of RFC 4627
 * - XML escape subset of XML 1.0 W3C 26 Nov 2008 (4.6 Predefined Entities)
 * - Fast decoding (~1 GB/s on i5-3330 @3GHz -using one core-)
 * - "Fast" decoding (200-400 MB/s on "; there is room for optimization)
 *
 * Features (custom LZ77 implementation):
 *
 * - Encoding time complexity: O(n)
 * - Decoding time complexity: O(n)
 *
 * Observations:
 * - Tables take 288 bytes (could be reduced to 248 bytes -tweaking access
 * to b64d[]-, but it would require to increase the number of operations in
 * order to shift -and secure- access to that area, being code increase more
 * than those 40 bytes).
 */

#include "scommon.h"

#define SDEBUG_LZ_STATS 0

typedef size_t (*srt_enc_f)(const uint8_t *s, size_t ss, uint8_t *o);
typedef size_t (*srt_enc_f2)(const uint8_t *s, size_t ss, uint8_t *o, size_t known_sso);

size_t senc_b64(const uint8_t *s, size_t ss, uint8_t *o);
size_t sdec_b64(const uint8_t *s, size_t ss, uint8_t *o);
size_t senc_hex(const uint8_t *s, size_t ss, uint8_t *o);
size_t senc_HEX(const uint8_t *s, size_t ss, uint8_t *o);
size_t sdec_hex(const uint8_t *s, size_t ss, uint8_t *o);
size_t senc_esc_xml(const uint8_t *s, size_t ss, uint8_t *o, size_t known_sso);
size_t sdec_esc_xml(const uint8_t *s, size_t ss, uint8_t *o);
size_t senc_esc_json(const uint8_t *s, size_t ss, uint8_t *o, size_t known_sso);
size_t sdec_esc_json(const uint8_t *s, size_t ss, uint8_t *o);
size_t senc_esc_url(const uint8_t *s, size_t ss, uint8_t *o, size_t known_sso);
size_t sdec_esc_url(const uint8_t *s, size_t ss, uint8_t *o);
size_t senc_esc_dquote(const uint8_t *s, size_t ss, uint8_t *o, size_t known_sso);
size_t sdec_esc_dquote(const uint8_t *s, size_t ss, uint8_t *o);
size_t senc_esc_squote(const uint8_t *s, size_t ss, uint8_t *o, size_t known_sso);
size_t sdec_esc_squote(const uint8_t *s, size_t ss, uint8_t *o);
size_t senc_lz(const uint8_t *s, size_t ss, uint8_t *o);
size_t senc_lzh(const uint8_t *s, size_t ss, uint8_t *o);
size_t sdec_lz(const uint8_t *s, size_t ss, uint8_t *o);

#define senc_b16 senc_HEX
#define sdec_b16 sdec_hex

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* SENC_H */

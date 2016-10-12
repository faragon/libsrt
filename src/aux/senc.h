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
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
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
 * Features (custom LZW implementation):
 *
 * - Encoding time is related to input data entropy. The more random
 *   the data is, LZW encoding requires more populated nodes. When "plane"
 *   areas are found, RLE opcodes are mixed into the LZW stream.
 * - Decoding time is related to the mix of normal LZW codes or RLE codes
 *    (the more RLE codes in the stream, the fastest).
 * - Encoding speed:
 *      Random data:
 *       75 MB/s (i5-3330 @3GHz)
 *       2 MB/s (ARM11 @700MHz)
 *      First 50 MB of a "cat" from /lib (Ubuntu 15.04 x86-64):
 *       200 MB/s (i5-3330 @3GHz)
 *       13 MB/s (ARM11 @700MHz)
 *      Data with lots of repeated bytes:
 *       4 GB/s (i5-3330 @3GHz)
 *       100 MB/s (ARM11 @700MHz)
 * - Decoding speed:
 *      Random data:
 *       100 MB/s (i5-3330 @3GHz)
 *       2 MB/s (ARM11 @700MHz)
 *      First 50 MB of a "cat" from /lib (Ubuntu 15.04 x86-64):
 *       275 MB/s (i5-3330 @3GHz)
 *       34 MB/s (ARM11 @700MHz)
 *      Data with lots of repeated bytes:
 *       6 GB/s (i5-3330 @3GHz)
 *       300 MB/s (ARM11 @700MHz)
 *
 * Observations:
 * - Tables take 288 bytes (could be reduced to 248 bytes -tweaking access
 * to b64d[]-, but it would require to increase the number of operations in
 * order to shift -and secure- access to that area, being code increase more
 * than those 40 bytes).
 */

#include "scommon.h"

typedef size_t (*senc_f_t)(const unsigned char *s, const size_t ss, unsigned char *o);
typedef size_t (*senc_f2_t)(const unsigned char *s, const size_t ss, unsigned char *o, const size_t known_sso);

size_t senc_b64(const unsigned char *s, const size_t ss, unsigned char *o);
size_t sdec_b64(const unsigned char *s, const size_t ss, unsigned char *o);
size_t senc_hex(const unsigned char *s, const size_t ss, unsigned char *o);
size_t senc_HEX(const unsigned char *s, const size_t ss, unsigned char *o);
size_t sdec_hex(const unsigned char *s, const size_t ss, unsigned char *o);
size_t senc_esc_xml(const unsigned char *s, const size_t ss, unsigned char *o, const size_t known_sso);
size_t sdec_esc_xml(const unsigned char *s, const size_t ss, unsigned char *o);
size_t senc_esc_json(const unsigned char *s, const size_t ss, unsigned char *o, const size_t known_sso);
size_t sdec_esc_json(const unsigned char *s, const size_t ss, unsigned char *o);
size_t senc_esc_url(const unsigned char *s, const size_t ss, unsigned char *o, const size_t known_sso);
size_t sdec_esc_url(const unsigned char *s, const size_t ss, unsigned char *o);
size_t senc_esc_dquote(const unsigned char *s, const size_t ss, unsigned char *o, const size_t known_sso);
size_t sdec_esc_dquote(const unsigned char *s, const size_t ss, unsigned char *o);
size_t senc_esc_squote(const unsigned char *s, const size_t ss, unsigned char *o, const size_t known_sso);
size_t sdec_esc_squote(const unsigned char *s, const size_t ss, unsigned char *o);
size_t senc_lzw(const unsigned char *s, const size_t ss, unsigned char *o);
size_t sdec_lzw(const unsigned char *s, const size_t ss, unsigned char *o);
size_t senc_rle(const unsigned char *s, const size_t ss, unsigned char *o);
size_t sdec_rle(const unsigned char *s, const size_t ss, unsigned char *o);

#define senc_b16 senc_HEX
#define sdec_b16 sdec_hex

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif	/* SENC_H */


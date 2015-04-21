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
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 *
 * Features:
 *
 * - Aliasing safe (input and output buffer can be the same).
 * - RFC 3548/4648 base 16 (hexadecimal) and 64 encoding/decoding.
 * - Fast (~1 GB/s on i5-3330 @3GHz -using one core- and gcc 4.8.2 -O2)
 *
 * Observations:
 * - Tables take 288 bytes (could be reduced to 248 bytes -tweaking access
 * to b64d[]-, but it would require to increase the number of operations in
 * order to shift -and secure- access to that area, being code increase more
 * than those 40 bytes).
 */

#include "scommon.h"

typedef size_t(*senc_f_t)(const unsigned char *s, const size_t ss, unsigned char *o);

size_t senc_b64(const unsigned char *s, const size_t ss, unsigned char *o);
size_t sdec_b64(const unsigned char *s, const size_t ss, unsigned char *o);
size_t senc_hex(const unsigned char *s, const size_t ss, unsigned char *o);
size_t senc_HEX(const unsigned char *s, const size_t ss, unsigned char *o);
size_t sdec_hex(const unsigned char *s, const size_t ss, unsigned char *o);

#define senc_b16 senc_HEX
#define sdec_b16 sdec_hex

#ifdef __cplusplus
};      /* extern "C" { */
#endif
#endif	/* SENC_H */


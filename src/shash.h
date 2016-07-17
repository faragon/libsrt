#ifndef SHASH_H
#define SHASH_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * shash.h
 *
 * Buffer hashing
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 *
 * Features:
 *
 * - Currently only a simple 32-bit checksum is implemented (used in the
 *   routing hash in sdmap.c)
 */

#include "scommon.h"


/* #notAPI: |Simple hash: 32 bit hash from xor-ing 32-bit chunks ("fair" enough for hash routing, but not for generic hash tables)|buffer;buffer size (in bytes)|32-bit hash|O(n)|1;2| */
unsigned sh_csum32(const void *buf, const size_t buf_size);

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif	/* SHASH_H */


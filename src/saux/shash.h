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
 * - Three CRC-32 implementations:
 *     + Minimal, without hash tables: 1 bit/loop (100MB/s on i5@3GHz)
 *       (active if doing 'make MINIMAL=1' or 'make ADD_CFLAGS="-DS_MINIMAL"')
 *     + 1024 byte hash table: 1 byte/loop (400MB/s on i5@3GHz)
 *       (if not minimal build, doing 'make ADD_CFLAGS="-DS_DIS_CRC32_FULL_OPT")
 *     + 16384 byte hash table: 16 bytes/loop (2500MB/s on i5@3GHz)
 *       (if not minimal build, this is the enabled by default)
 */

#include "scommon.h"

/* #notAPI: |CRC-32 (0xedb88320 polynomial)|CRC accumulator;buffer;buffer size (in bytes)|32-bit hash|O(n)|1;2| */
uint32_t sh_crc32(uint32_t crc, const void *buf, size_t buf_size);

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif	/* SHASH_H */


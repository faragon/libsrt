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
 * - Six CRC-32 modes (make ADD_CFLAGS="-DS_CRC32_SLC=0/1/4/8/16"):
 *     + Minimal, without hash tables: 1 bit/loop (100MB/s on i5@3GHz)
 *     + 1024 byte hash table: 1 byte/loop (400MB/s on i5@3GHz)
 *     + 4096 byte hash table: 4 bytes/loop (1000MB/s on i5@3GHz)
 *     + 8192 byte hash table: 8 bytes/loop (2000MB/s on i5@3GHz)
 *     + 12288 byte hash table: 12 bytes/loop (2500MB/s on i5@3GHz)
 *     + 16384 byte hash table: 16 bytes/loop (2700MB/s on i5@3GHz)
 */

#include "scommon.h"

/* #notAPI: |CRC-32 (0xedb88320 polynomial)|CRC accumulator (for offset 0 must be 0);buffer;buffer size (in bytes)|32-bit hash|O(n)|1;2| */
uint32_t sh_crc32(uint32_t crc, const void *buf, size_t buf_size);
/* #notAPI: |Adler32 checksum|Adler32 accumulator (for offset 0 must be 1);buffer;buffer size (in bytes)|32-bit hash|O(n)|1;2| */
uint32_t sh_adler32(uint32_t adler, const void *buf, size_t buf_size);

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif	/* SHASH_H */


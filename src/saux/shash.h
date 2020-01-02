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
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
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

/* Linux hashing algorithm for 32 and 64-bit values */
#define S_GR32 0x61C88647
#define S_GR64 ((uint64_t)0x61C8864680B583EBULL)

#define S_CRC32_INIT 0
#define S_ADLER32_INIT 1
#define S_FNV1_INIT ((uint32_t)0x811c9dc5)
#define S_MH3_32_INIT 42

/* #notAPI: |CRC-32 (0xedb88320 polynomial)|CRC accumulator (for offset 0 must be 0);buffer;buffer size (in bytes)|32-bit hash|O(n)|1;2| */
uint32_t sh_crc32(uint32_t crc, const void *buf, size_t buf_size);
/* #notAPI: |Adler32 checksum|Adler32 accumulator (for offset 0 must be 1);buffer;buffer size (in bytes)|32-bit hash|O(n)|1;2| */
uint32_t sh_adler32(uint32_t adler, const void *buf, size_t buf_size);
/* #notAPI: |FNV-1 hash|FNV accumulator (for offset 0 must be S_FNV_INIT);buffer;buffer size (in bytes)|32-bit hash|O(n)|1;2| */
uint32_t sh_fnv1(uint32_t fnv, const void *buf, size_t buf_size);
/* #notAPI: |FNV-1A hash|FNV-1A accumulator (for offset 0 must be S_FNV1A_INIT);buffer;buffer size (in bytes)|32-bit hash|O(n)|1;2| */
uint32_t sh_fnv1a(uint32_t fnv, const void *buf, size_t buf_size);
/* #notAPI: |MurmurHash3-32 hash|MH3 accumulator (for offset 0 must be S_MM3_32_INIT);buffer;buffer size (in bytes)|32-bit hash|O(n)|1;2| */
uint32_t sh_mh3_32(uint32_t acc, const void *buf, size_t buf_size);

S_INLINE uint32_t sh_hash32(uint32_t v)
{
        return (v * S_GR32);
}

S_INLINE uint32_t sh_hash64(uint64_t v)
{
        return (uint32_t)(v * S_GR64);
}

// Floating point hashing: if matches the size of integers, use the
// integer hash. Otherwise, use FNV-1A hashing.

S_INLINE uint32_t sh_hash_f(float v)
{
	uint32_t v32;
	if (sizeof(v) == sizeof(v32)) {
		memcpy(&v32, &v, sizeof(v));
		return sh_hash32(v32);
	}
	return sh_fnv1a(S_FNV1_INIT, &v, sizeof(v));
}

S_INLINE uint32_t sh_hash_d(double v)
{
	return sh_fnv1a(S_FNV1_INIT, &v, sizeof(v));
}

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* SHASH_H */

#ifndef SSORT_H
#define SSORT_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * ssort.h
 *
 * Sorting algorithms
 *
 * Features:
 * - Fast, in-place, 8-bit integer sort
 *   - Algorithm: counting sort
 *   - Space complexity: O(1)
 *   - Time complexity: O(n)
 * - Fast, in-place, 16/32/64-bit integer sort
 *   - Algorithm: MSD binary radix sort (with optimizations)
 *   - Space complexity: O(1)
 *   - Time complexity: O(n log n). The algorithm is pseudo O(n), as that
 *     case only happens when having duplicated elements (e.g. if you
 *     sort more than 2^16 16-bit elements, it would start being really O(n),
 *     for that specific case).
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "scommon.h"

typedef void (*ssort_f)(void *, size_t);

void ssort_i8(int8_t *b, size_t elems);
void ssort_u8(uint8_t *b, size_t elems);
void ssort_i16(int16_t *b, size_t elems);
void ssort_u16(uint16_t *b, size_t elems);
void ssort_i32(int32_t *b, size_t elems);
void ssort_u32(uint32_t *b, size_t elems);
void ssort_i64(int64_t *b, size_t elems);
void ssort_u64(uint64_t *b, size_t elems);

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* SSORT_H */

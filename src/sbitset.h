#ifndef SBITSET_H
#define SBITSET_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * sbitset.h
 *
 * #SHORTDOC bit set (bit array)
 *
 * #DOC Functions allowing bit random access storage and bit counting.
 * #DOC Bit counting is optimized so, instead of per-call 'poppulation count',
 * #DOC it takes O(1) for the computation, as a record of bit set/clear is
 * #DOC kept.
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 *
 * Features:
 * - Fast: over 300 million bit set per second on i5-3330 @3GHz
 * - Very fast: O(1) bit "population count" (vs e.g. g++ O(n) std::bitset)
 * - Safe: bound checked
 * - Supports both stack and heap allocation
 * - Supports dynamic-size bit set (when using heap allocation)
 */

#include "svector.h"

/*
 * Structures
 */

typedef srt_vector srt_bitset; /* Opaque structure (accessors are provided) */
			       /* (bitset is implemented using a vector)    */

/*
 * Allocation
 */

#define SB_BITS2BYTES(n) (1 + n / 8)
#define sb_alloc(n) \
	sb_alloc_aux((srt_bitset *)sv_alloc(1, SB_BITS2BYTES(n), NULL))
#define sb_alloca(n) \
	sb_alloc_aux((srt_bitset *)sv_alloca(1, SB_BITS2BYTES(n), NULL))
#define sb_dup(b) sv_dup(b)
#define sb_free sv_free

srt_bitset *sb_alloc_aux(srt_bitset *b);

/*
#API: |Allocate bitset (stack)|space preallocated to store n elements|bitset|O(n)|1;2|
srt_bitset *sb_alloca(size_t initial_num_elems_reserve)

#API: |Allocate bitset (heap)|space preallocated to store n elements|bitset|O(n)|1;2|
srt_bitset *sb_alloc(size_t initial_num_elems_reserve)

#API: |Free one or more bitsets (heap)|bitset; more bitsets (optional)|bitset|O(1)|1;2|
srt_bitset *sb_free(srt_bitset **b, ...)

#API: |Duplicate bitset|bitset|output bitset|O(n)|1;2|
srt_bitset *sb_dup(const srt_bitset *src)
*/

/*
 * Accessors
 */

/* #API: |Reset bitset|bitset|-|O(1)|1;2| */
void sb_clear(srt_bitset *b);

/* #API: |Number of bits set to 1|bitset|Map number of elements|O(1)|1;2| */
S_INLINE size_t sb_popcount(const srt_bitset *b)
{
	return b ? b->vx.cnt : 0;
}

/*
 * Operations
 */

int sb_test_(const srt_bitset *b, size_t nth);

/* #API: |Access to nth bit|bitset; bit offset|1 or 0|O(1)|1;2| */
S_INLINE int sb_test(const srt_bitset *b, size_t nth)
{
	RETURN_IF(!b, 0);
	return sb_test_(b, nth);
}

void sb_set_(srt_bitset *b, size_t nth);

/* #API: |Set nth bit to 1|bitset; bit offset||O(1)|1;2| */
S_INLINE void sb_set(srt_bitset **b, size_t nth)
{
	if (b && *b)
		sb_set_(*b, nth);
}

void sb_reset_(srt_bitset *b, size_t nth, size_t pos);

/* #API: |Set nth bit to 0|bitset; bit offset||O(1)|1;2| */
S_INLINE void sb_reset(srt_bitset **b, size_t nth)
{
	if (b && *b) {
		size_t pos = nth / 8;
		if (pos < sv_size(*b)) {
			sb_reset_(*b, nth, pos);
		}
	}
}

/* #API: |Preallocated space left (number of 1 bit elements)|bitset|allocated space left (unit: bits)|O(1)|1;2| */
S_INLINE size_t sb_capacity(const srt_bitset *b)
{
	return 8 * sv_capacity(b);
}

/* #API: |Ensure space for N 1-bit elements|bitset;absolute element reserve (unit: bits)|reserved elements|O(1)|1;2| */
S_INLINE size_t sb_reserve(srt_bitset **b, size_t max_elems)
{
	return sv_reserve(b, 1 + max_elems / 8) * 8;
}

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* #ifndef SBITSET_H */

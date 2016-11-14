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
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 *
 * Features:
 * - Fast: over 300 million bit set per second on i5-3330 @3GHz
 * - Very fast: O(1) bit "population count" (vs e.g. g++ O(n) std::bitset)
 * - Safe: bound checked
 * - Supports both stack and heap allocation
 * - Supports dynamic-size bit set (when using heap allocation)
 * - Lazy initialization (and allocation, if using heap): no "memset" to 0
 *   is done until required, and is applied to the affected area. Also bit
 *   set to 0 on non set areas requires doing nothing, as unitialized area
 *   can be both not initialized or even not allocated (when using heap memory)
 */ 

#include "svector.h"

/*
 * Structures
 */

typedef sv_t sb_t;	/* "Hidden" structure (accessors are provided) */
			/* (bitset is implemented using a vector)  */

/*
 * Allocation
 */

#define SB_BITS2BYTES(n)	(1 + n / 8)
#define sb_alloc(n)		sv_alloc(1, SB_BITS2BYTES(n), NULL)
#define sb_alloca(n)		sv_alloca(1, SB_BITS2BYTES(n), NULL)
#define sb_dup(b)		sv_dup(b)
#define sb_free			sv_free

/*
#API: |Allocate bitset (stack)|space preallocated to store n elements|bitset|O(1)|1;2|
sb_t *sb_alloca(const size_t initial_num_elems_reserve)

#API: |Allocate bitset (heap)|space preallocated to store n elements|bitset|O(1)|1;2|
sb_t *sb_alloc(const size_t initial_num_elems_reserve)

#API: |Free one or more bitsets (heap)|bitset; more bitsets (optional)|bitset|O(1)|1;2|
sb_t *sb_free(sb_t **b, ...)

#API: |Duplicate bitset|bitset|output bitset|O(n)|1;2|
sb_t *sb_dup(const sb_t *src)

*/

/*
 * Accessors
 */

/* #API: |Reset bitset|bitset|-|O(1)|1;2| */
S_INLINE void sb_clear(sb_t *b)
{
	if (b) {
		sv_set_size(b, 0);
		b->vx.cnt = 0;
	}
}

/* #API: |Number of bits set to 1|bitset|Map number of elements|O(1)|1;2| */

S_INLINE size_t sb_popcount(const sb_t *b)
{
	return b ? b->vx.cnt : 0;
}

/*
 * Operations
 */

/* #API: |Access to nth bit|bitset; bit offset|1 or 0|O(1)|1;2| */

S_INLINE int sb_test(const sb_t *b, const size_t nth)
{
	S_ASSERT(b);
	RETURN_IF(!b, 0);
	const size_t pos = nth / 8, mask = (size_t)1 << (nth % 8);
	RETURN_IF(pos >= sv_size(b), 0);
	const unsigned char *buf = (const unsigned char *)sv_get_buffer_r(b);
	return (buf[pos] & mask) ? 1 : 0;
}

/* #API: |Set nth bit to 1|bitset; bit offset||O(n) -O(1) amortized-|1;2| */

S_INLINE void sb_set(sb_t **b, const size_t nth)
{
	S_ASSERT(b);
	if (b) {
		unsigned char *buf;
		const size_t pos = nth / 8, mask = (size_t)1 << (nth % 8),
			     pinc = pos + 1;
		if (!(*b)) { /* BEHAVIOR: if NULL, assume heap allocation */
			*b = sb_alloc(nth);
			if (!(*b)) {
				S_ERROR("not enough memory");
				return;
			}
		}
		if (pinc > sv_size(*b)) {
			if (sv_reserve(b, pinc) < pinc || !*b) {
				S_ERROR("not enough memory");
				return;
			}
			size_t ss = sv_size(*b);
			buf = (unsigned char *)sv_get_buffer(*b);
			memset(buf + ss, 0, pinc - ss);
			sv_set_size(*b, pinc);
		} else {
			buf = (unsigned char *)sv_get_buffer(*b);
		}
		if ((buf[pos] & mask) == 0) {
			buf[pos] |= mask;
			(*b)->vx.cnt++;
		}
	}
}

/* #API: |Set nth bit to 0|bitset; bit offset||O(1)|1;2| */

S_INLINE void sb_reset(sb_t **b, const size_t nth)
{
	S_ASSERT(!b);
	if (b && *b) {
		const size_t pos = nth / 8;
		if (pos < sv_size(*b)) {
			unsigned char *buf = (unsigned char *)sv_get_buffer(*b);
			size_t mask = (size_t)1 << (nth % 8);
			if ((buf[pos] & mask) != 0) {
				buf[pos] &= ~mask;
				(*b)->vx.cnt--;
			}
		}
		/* else: implicitly considered as set to 0 */
	}
	/* else: NULL bitset is implicitly considered as set to 0 */
}

/* #API: |Force evaluation of first N bits -equivalent to set to 0 all not previously referenced bits-|bitset; bit offset|-|O(n)|1;2| */

S_INLINE void sb_eval(sb_t **b, const size_t nth)
{
	if (b) {
		int prev = sb_test(*b, nth);
		sb_set(b, nth);
		if (!prev)
			sb_reset(b, nth);
	}
}

/* #API: |Preallocated space left (number of 1 bit elements)|bitset|allocated space left (unit: bits)|O(1)|1;2| */
S_INLINE size_t sb_capacity(const sb_t *b)
{
	return 8 * sv_capacity(b);
}

/* #API: |Ensure space for N 1-bit elements|bitset;absolute element reserve (unit: bits)|reserved elements|O(1)|1;2| */
S_INLINE size_t sb_reserve(sb_t **b, const size_t max_elems)
{
	return sv_reserve(b, 1 + max_elems / 8) * 8;
}

/* #API: |Free unused space|bitset|same bitset (optional usage)|O(1)|1;2| */
S_INLINE sb_t *sb_shrink(sb_t **b)
{
	RETURN_IF(!b || !*b, NULL); /* BEHAVIOR */ /* TODO: null bitset */
	if (!sb_popcount(*b))
		sv_set_size(*b, 0);
	return sv_shrink(b);
}

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* #ifndef SBITSET_H */


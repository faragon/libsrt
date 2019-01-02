#ifndef SSEARCH_H
#define SSEARCH_H
#ifdef __cplusplus
extern "C" {
#endif

#include "scommon.h"

/*
 * ssearch.h
 *
 * Real-time string search using the Rabin-Karp algorithm.
 *
 * Features:
 * - Real-time search (O(n) time complexity).
 * - Search in raw data.
 * - Over 1 GB/s sustained speed on 3-way 3GHz OooE CPU (single thread).
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 *
 * O(n) string search using a rolling hash. Two hashes are used, one more
 * complex ((f(t-1) + 1) * 2 + f(t)), to ensure O(n) time, and othersimple
 * (sum of bytes), so the algorithm can start in the "fast" mode, and switch
 * to the O(n) compliant when collisons for a given period are above an
 * specific threshold.
 *
 * Being n the elements of the string and m the elements of the pattern to
 * be found:
 * - Worst time (proof pending):
 *   m * k1 operations for computing pattern checksum +
 *   n * k2 operations for computing/comparing current checksum
 * - Other observations:
 *   - No input pre-processing required.
 *   - Suitable for real-time: predictable worst time (ss_find_csum_slow())
 *   - Although is O(n), for "good cases" is ~2-10x slower than best O(m*n).
 *     In corner cases, however, this algorithm outperforms all O(n*m)
 *     algorithms.
 *   - Cached and uncached speed is similar on OooE CPUs (more CPU than
 *     RAM limited), i.e. the RAM bus is not saturated (multi-core CPUs).
 *     Over 1 GB/s sustained search speed is achieved on modern 3GHz OooE
 *     single thread.
 *   - The FCSUM_SLOW ensures O(n), FCSUM_FAST is about twice as fast on
 *     worst-case scenarios, but does not ensure O(n). The algorithm starts
 *     with the fast version, and switches to the slow case on the first false
 *     CSUM mismatch.
 *   - Could be easily expanded for finding more than one pattern at once
 *     (like any other Rabin-Karp algorithm implementation).
 *
 * ss_find_csum_fast: O(n), because it switches to ss_find_csum_slow() when
 * collisions get over specific threshold per period.
 * ss_find_csum_slow: O(n) (half the speed of ss_find_csum_fast in good cases,
 * and just a bit faster in worst cases -as the "fast" algorithm switch
 * requires recomputing again the hash of the target pattern-).
 *
 * References:
 *   - Rabin-Karp search algorithm (search using a rolling hash)
 *   - Raphael Javaux's fast_strstr algorithm (simple hash case: sum of bytes)
 *
 * Other functions, implemented as examples:
 * ss_find_bf: O(n*m), the slowest (brute force).
 * ss_find_bmh: O(n*m), good average (Boyer-Moore-Horspool).
 * ss_find_libc: relies on libc strstr(), so don't support raw data.
 *
 * For "good cases", ss_find_csum_fast is 2-10x slower than e.g. strstr()
 * found in glibc ("Two-Way String-Matching" algorithm, O(n*m)). However, for
 * worst-case scenarios, is faster (O(n) < O(n*m)). I.e. ss_find_csum_* is
 * suitable for real-time requirements, while strstr ("Two-Way") it is not.
 * In comparison to e.g. glibc memmem(), which performs similar to
 * ss_find_bf(), the ss_find_csum_fast() is much faster.
 */

/*
 * Togglable options
 *
 * S_ENABLE_FIND_CSUM_FIRST_CHAR_LOCATION_OPTIMIZATION: skips data not
 * matching with first target byte. Disable in case you want to avoid
 * linking memchr().
 *
 * S_ENABLE_FIND_CSUM_INNER_LOOP_UNROLLING: loop unrolling for both the
 * initialization and search phases. Disable for reducing code size.
 *
 * S_ENABLE_FIND_CSUM_FAST_TO_SLOW_ALGORITHM_SWITCH: this enables the
 * check for switching between the fast and the "slow" (O(n) compliant)
 * algorithm. Don't disable this unless you already know beforehand
 * about the data being processing and willing to squeze for performance.
 * Warning: it is a bad idea to disable this without a good reason,
 * as this ensures real-time requirements.
 *
 * S_ENABLE_FIND_OTHER_EXAMPLES: additional implementations (brute force,
 * Boyer-Moore-Horspool, strstr wrapper). Disabling this could save some
 * space on devices with little memory (e.g. microcontrollers).
 *
 * Instruction cache observation: code is tiny, even with unrolling.
 */

#define S_ENABLE_FIND_CSUM_FIRST_CHAR_LOCATION_OPTIMIZATION
#define S_ENABLE_FIND_CSUM_INNER_LOOP_UNROLLING
#define S_ENABLE_FIND_CSUM_FAST_TO_SLOW_ALGORITHM_SWITCH

#ifdef S_MINIMAL
#undef S_ENABLE_FIND_CSUM_FIRST_CHAR_LOCATION_OPTIMIZATION
#undef S_ENABLE_FIND_CSUM_INNER_LOOP_UNROLLING
#endif

size_t ss_find_csum_slow(const char *s0, size_t off, size_t ss, const char *t, size_t ts);
size_t ss_find_csum_fast(const char *s0, size_t off, size_t ss, const char *t, size_t ts);

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* SSEARCH_H */

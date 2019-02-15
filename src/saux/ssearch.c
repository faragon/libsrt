/*
 * ssearch.c
 *
 * Real-time string search using the Rabin-Karp algorithm.
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "ssearch.h"
#include "scommon.h"

#ifndef S_DISABLE_SEARCH_GUARANTEE
#define S_ENABLE_FIND_CSUM_FAST_TO_SLOW_ALGORITHM_SWITCH
#endif

/*
 * ss_find_csum_* helpers
 */
#define FCSUM_FAST(p, q, i) ((unsigned char)q[i])
#define FCSUM_SLOW(p, q, i)                                                    \
	((2 * (1 + (unsigned char)p[i - 1])) + (unsigned char)q[i])
#define S_FPREPARECSUM(CSUMF, i)                                               \
	((target += CSUMF(t, t, i)), current += CSUMF(s, s, i))
#define S_FWINDOW_MOVE(CSUMF, s, i) (CSUMF(s, s, i) - CSUMF(s, s, 1 + i - ts))
#define S_FSEARCH(CSUMF)                                                       \
	(((current += S_FWINDOW_MOVE(CSUMF, s, 0)), s++, current) == target)
#define S_FCSUM_RETURN_CHECK                                                   \
	if (!memcmp(s - ts, t, ts))                                            \
	return (size_t)((size_t)(s - s0) - ts)
#define S_FCSUM_RETURN_CHECK_W_ALGORITHM_SWITCH                                \
	if (!csum_collision_off) {                                             \
		csum_collision_off = (size_t)((size_t)(s - s0) - off);         \
		csum_collision_count = 1;                                      \
	} else {                                                               \
		if ((size_t)(s - s0) - csum_collision_off > ts * 10) {         \
			csum_collision_off = (size_t)(s - s0); /* reset */     \
			csum_collision_count = 1;                              \
		} else {                                                       \
			if (++csum_collision_count > (2 + ts / 2)) {           \
				return ss_find_csum_slow(s0, (size_t)(s - s0), \
							 ss, t, ts);           \
			}                                                      \
		}                                                              \
	}
#ifdef S_ENABLE_FIND_CSUM_FAST_TO_SLOW_ALGORITHM_SWITCH
#define S_FIND_CSUM_ALG_SWITCH_SETUP                                           \
	size_t csum_collision_off = 0, csum_collision_count = 0
#define S_FIND_CSUM_ALG_SWITCH S_FCSUM_RETURN_CHECK_W_ALGORITHM_SWITCH
#else
#define S_FIND_CSUM_ALG_SWITCH_SETUP
#define S_FIND_CSUM_ALG_SWITCH
#endif

/*
 * ss_find_csum_* templates for pipeline fill phase
 */
#ifdef S_ENABLE_FIND_CSUM_FIRST_CHAR_LOCATION_OPTIMIZATION
#define S_FIND_SET_START                                                       \
	size_t i;                                                              \
	const char *stm1;                                                      \
	unsigned int target, current;                                          \
	const char *s = (const char *)memchr(s0 + off, *t, ss - off);
#else
#define S_FIND_SET_START                                                       \
	size_t i;                                                              \
	const char *stm1;                                                      \
	unsigned int target, current;                                          \
	const char *s = (const char *)(s0 + off);
#endif
#define S_FIND_CSUM_PIPELINE1(alg)                                             \
	S_FIND_SET_START;                                                      \
	if (!s || (ss - (size_t)(s - s0)) < ts)                                \
		return S_NPOS;                                                 \
	i = 1;                                                                 \
	stm1 = s0 + ss - 1;                                                    \
	target = 0, current = 0;
#ifdef S_ENABLE_FIND_CSUM_INNER_LOOP_UNROLLING
#define S_FIND_CSUM_PIPELINE2(alg)                                             \
	for (; (i + 4) < ts; i += 4) {                                         \
		S_FPREPARECSUM(alg, i);                                        \
		S_FPREPARECSUM(alg, i + 1);                                    \
		S_FPREPARECSUM(alg, i + 2);                                    \
		S_FPREPARECSUM(alg, i + 3);                                    \
	}
#else
#define S_FIND_CSUM_PIPELINE2(alg)
#endif
#define S_FIND_CSUM_PIPELINE3(alg)                                             \
	for (; i < ts; i++) {                                                  \
		S_FPREPARECSUM(alg, i);                                        \
	}                                                                      \
	if (current == target && !memcmp(s, t, ts))                            \
		return (size_t)(s - s0); /* found: prefix */                   \
	s += ts;

/*
 * ss_find_csum_* templates for search phase
 */
#ifdef S_ENABLE_FIND_CSUM_INNER_LOOP_UNROLLING
#define S_FIND_CSUM_SEARCH1(alg, extra_check)                                  \
	for (; (s + 16) <= stm1;) {                                            \
		if ((S_FSEARCH(alg) || S_FSEARCH(alg) || S_FSEARCH(alg)        \
		     || S_FSEARCH(alg) || S_FSEARCH(alg) || S_FSEARCH(alg)     \
		     || S_FSEARCH(alg) || S_FSEARCH(alg) || S_FSEARCH(alg)     \
		     || S_FSEARCH(alg) || S_FSEARCH(alg) || S_FSEARCH(alg)     \
		     || S_FSEARCH(alg) || S_FSEARCH(alg) || S_FSEARCH(alg)     \
		     || S_FSEARCH(alg))) {                                     \
			S_FCSUM_RETURN_CHECK;                                  \
			extra_check;                                           \
		}                                                              \
	}
#else
#define S_FIND_CSUM_SEARCH1(alg, extra_check)
#endif
#define S_FIND_CSUM_SEARCH2(alg, extra_check)                                  \
	for (; s <= stm1;) {                                                   \
		if (S_FSEARCH(alg)) {                                          \
			S_FCSUM_RETURN_CHECK;                                  \
			extra_check;                                           \
		}                                                              \
	}

size_t ss_find_csum_slow(const char *s0, size_t off, size_t ss, const char *t,
			 size_t ts)
{
	S_FIND_CSUM_PIPELINE1(FCSUM_SLOW);
	S_FIND_CSUM_PIPELINE2(FCSUM_SLOW);
	S_FIND_CSUM_PIPELINE3(FCSUM_SLOW);
	S_FIND_CSUM_SEARCH1(FCSUM_SLOW, {});
	S_FIND_CSUM_SEARCH2(FCSUM_SLOW, {});
	return S_NPOS;
}

size_t ss_find_csum_fast(const char *s0, size_t off, size_t ss, const char *t,
			 size_t ts)
{
	S_FIND_CSUM_ALG_SWITCH_SETUP;
	S_FIND_CSUM_PIPELINE1(FCSUM_FAST);
	S_FIND_CSUM_PIPELINE2(FCSUM_FAST);
	S_FIND_CSUM_PIPELINE3(FCSUM_FAST);
	S_FIND_CSUM_SEARCH1(FCSUM_FAST, S_FIND_CSUM_ALG_SWITCH);
	S_FIND_CSUM_SEARCH2(FCSUM_FAST, S_FIND_CSUM_ALG_SWITCH);
	return S_NPOS;
}

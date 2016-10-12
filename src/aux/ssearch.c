/*
 * ssearch.c
 *
 * Real-time string search using the Rabin-Karp algorithm.
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "ssearch.h"
#include "scommon.h"

/*
 * ss_find_csum_* helpers
 */
#define FCSUM_FAST(p, q, i)	\
	((unsigned char)q[i])
#define FCSUM_SLOW(p, q, i)	\
	((2 * (1 + (unsigned char)p[i - 1])) + (unsigned char)q[i])
#define S_FPREPARECSUM(CSUMF, i) \
	((target += CSUMF(t, t, i)), current += CSUMF(s, s, i))
#define S_FWINDOW_MOVE(CSUMF, s, i)			\
	(CSUMF(s, s, i) - CSUMF(s, s, 1 + i - ts))
#define S_FSEARCH(CSUMF)				\
	(((current += S_FWINDOW_MOVE(CSUMF, s, 0)),	\
	 s++, current) == target)
#define S_FCSUM_RETURN_CHECK			\
	if (!memcmp(s - ts, t, ts))		\
		return (size_t)((size_t)(s - s0) - ts)
#define S_FCSUM_RETURN_CHECK_W_ALGORITHM_SWITCH				   \
	if (!csum_collision_off) {					   \
		csum_collision_off = (size_t)((size_t)(s - s0) - off);	   \
		csum_collision_count = 1;				   \
	} else {							   \
		if ((size_t)(s - s0) - csum_collision_off > ts * 10) {	   \
			csum_collision_off = (size_t)(s - s0); /* reset */ \
			csum_collision_count = 1;			   \
		} else {						   \
			if (++csum_collision_count > (2 + ts/2)) {	   \
				return ss_find_csum_slow(s0,		   \
					(size_t)(s - s0), ss, t, ts);	   \
			}						   \
		}							   \
	}
#ifdef S_ENABLE_FIND_CSUM_FAST_TO_SLOW_ALGORITHM_SWITCH
#define S_FIND_CSUM_ALG_SWITCH_SETUP	\
	size_t csum_collision_off = 0, csum_collision_count = 0;
#define S_FIND_CSUM_ALG_SWITCH S_FCSUM_RETURN_CHECK_W_ALGORITHM_SWITCH
#else
#define S_FIND_CSUM_ALG_SWITCH_SETUP
#define S_FIND_CSUM_ALG_SWITCH
#endif

/*
 * ss_find_csum_* templates for pipeline fill phase
 */
#ifdef S_ENABLE_FIND_CSUM_FIRST_CHAR_LOCATION_OPTIMIZATION
#define S_FIND_SET_START	\
	const char *s = (const char *)memchr(s0 + off, *t, ss - off);
#else
#define S_FIND_SET_START \
	const char *s = (const char *)(s0 + off);
#endif
#define S_FIND_CSUM_PIPELINE1(alg)		\
	S_FIND_SET_START;			\
	if (!s || (ss - (size_t)(s - s0)) < ts)	\
		return S_NPOS;			\
	size_t i = 1;				\
	const char *stm1 = s0 + ss - 1;		\
	unsigned int target = 0, current = 0;
#ifdef S_ENABLE_FIND_CSUM_INNER_LOOP_UNROLLING
#define S_FIND_CSUM_PIPELINE2(alg)		\
	for (; (i + 4) < ts; i += 4) {		\
		S_FPREPARECSUM(alg, i);		\
		S_FPREPARECSUM(alg, i + 1);	\
		S_FPREPARECSUM(alg, i + 2);	\
		S_FPREPARECSUM(alg, i + 3);	\
	}
#else
#define S_FIND_CSUM_PIPELINE2(alg)
#endif
#define S_FIND_CSUM_PIPELINE3(alg)				\
	for (; i < ts; i++) {					\
		S_FPREPARECSUM(alg, i);				\
	}							\
	if (current == target && !memcmp(s, t, ts))		\
		return (size_t)(s - s0); /* found: prefix */	\
	s += ts;

/*
 * ss_find_csum_* templates for search phase
 */
#ifdef S_ENABLE_FIND_CSUM_INNER_LOOP_UNROLLING
#define S_FIND_CSUM_SEARCH1(alg, extra_check)			\
	for (; (s + 16) <= stm1;)	{			\
		if ((S_FSEARCH(alg) || S_FSEARCH(alg) ||	\
		     S_FSEARCH(alg) || S_FSEARCH(alg) ||	\
		     S_FSEARCH(alg) || S_FSEARCH(alg) ||	\
		     S_FSEARCH(alg) || S_FSEARCH(alg) ||	\
		     S_FSEARCH(alg) || S_FSEARCH(alg) ||	\
		     S_FSEARCH(alg) || S_FSEARCH(alg) ||	\
		     S_FSEARCH(alg) || S_FSEARCH(alg) ||	\
		     S_FSEARCH(alg) || S_FSEARCH(alg))) {	\
			S_FCSUM_RETURN_CHECK;			\
			extra_check;				\
		}						\
	}
#else
#define S_FIND_CSUM_SEARCH1(alg, extra_check)
#endif
#define S_FIND_CSUM_SEARCH2(alg, extra_check)	\
	for (; s <= stm1;) {			\
		if (S_FSEARCH(alg)) {		\
			S_FCSUM_RETURN_CHECK;	\
			extra_check;		\
		}				\
	}

size_t ss_find_csum_slow(const char *s0, const size_t off, const size_t ss,
			 const char *t, const size_t ts)
{
	S_FIND_CSUM_PIPELINE1(FCSUM_SLOW);
	S_FIND_CSUM_PIPELINE2(FCSUM_SLOW);
	S_FIND_CSUM_PIPELINE3(FCSUM_SLOW);
	S_FIND_CSUM_SEARCH1(FCSUM_SLOW, {});
	S_FIND_CSUM_SEARCH2(FCSUM_SLOW, {});
	return S_NPOS;
}

size_t ss_find_csum_fast(const char *s0, const size_t off, const size_t ss,
			 const char *t, const size_t ts)
{
	S_FIND_CSUM_PIPELINE1(FCSUM_FAST);
	S_FIND_CSUM_PIPELINE2(FCSUM_FAST);
	S_FIND_CSUM_PIPELINE3(FCSUM_FAST);
	S_FIND_CSUM_ALG_SWITCH_SETUP;
	S_FIND_CSUM_SEARCH1(FCSUM_FAST, S_FIND_CSUM_ALG_SWITCH);
	S_FIND_CSUM_SEARCH2(FCSUM_FAST, S_FIND_CSUM_ALG_SWITCH);
	return S_NPOS;
}

#undef S_ENABLE_FIND_CSUM_FIRST_CHAR_LOCATION_OPTIMIZATION
#undef S_ENABLE_FIND_CSUM_INNER_LOOP_UNROLLING
#undef S_ENABLE_FIND_CSUM_FAST_TO_SLOW_ALGORITHM_SWITCH
#undef FCSUM_FAST
#undef FCSUM_SLOW
#undef S_FPREPARECSUM
#undef S_FWINDOW_MOVE
#undef S_FSEARCH
#undef S_FCSUM_RETURN_CHECK
#undef S_FCSUM_RETURN_CHECK_W_ALGORITHM_SWITCH
#undef S_FIND_CSUM_ALG_SWITCH_SETUP
#undef S_FIND_CSUM_ALG_SWITCH
#undef S_FIND_CSUM_ALG_SWITCH_SETUP
#undef S_FIND_CSUM_ALG_SWITCH
#undef S_FIND_CSUM_PIPELINE1
#undef S_FIND_CSUM_PIPELINE2
#undef S_FIND_CSUM_PIPELINE2
#undef S_FIND_CSUM_PIPELINE3
#undef S_FIND_CSUM_SEARCH1
#undef S_FIND_CSUM_SEARCH1
#undef S_FIND_CSUM_SEARCH2

#ifdef S_ENABLE_OTHER_EXAMPLES
/*
 * Brute force search: O(n*m)
 */
size_t ss_find_bf(const char *s0, const size_t off, const size_t ss,
		  const char *t, const size_t ts)
{
	RETURN_IF(!ss, S_NPOS);
	const char t0 = t[0],
		  *s = s0 + off, *s_top = s0 + ss - ts,
		  *t_top = t + ts,
		  *sa, *ta;
	for (; s <= s_top;) {
		if (!((*s++ == t0) || (s_top >= (s + 24) &&
		    ((*s++==t0)||(*s++==t0)||(*s++==t0)||(*s++==t0)||(*s++==t0)||
		     (*s++==t0)||(*s++==t0)||(*s++==t0)||(*s++==t0)||(*s++==t0)||
		     (*s++==t0)||(*s++==t0)||(*s++==t0)||(*s++==t0)||(*s++==t0)||
		     (*s++==t0)||(*s++==t0)||(*s++==t0)||(*s++==t0)||(*s++==t0)||
		     (*s++==t0)||(*s++==t0)||(*s++==t0)||(*s++==t0))))) {
			s = (const char *)memchr(s, t0, (size_t)(s_top - s) + 1);
			if (!s)
				return S_NPOS;
			s++;
		}
		sa = s;
		ta = t + 1;
		for (; (ta + 8) <= t_top;)
			if (*sa++ != *ta++ || *sa++ != *ta++ ||
			    *sa++ != *ta++ || *sa++ != *ta++ ||
			    *sa++ != *ta++ || *sa++ != *ta++ ||
			    *sa++ != *ta++ || *sa++ != *ta++ )
				goto ss_find_next;
		for (; (ta + 3) <= t_top;)
			if (*sa++ != *ta++ || *sa++ != *ta++ ||
			    *sa++ != *ta++)
				goto ss_find_next;
		for (; ta < t_top && *sa == *ta; sa++, ta++);
		if (ta == t_top)
			return (size_t)(s - s0 - 1);
ss_find_next:;
	}
	return S_NPOS;
}

/*
 * BMH search: O(m*n)
 */
size_t ss_find_bmh(const char *s0, const size_t off, const size_t ss0,
		   const char *t0, const size_t ts)
{
	const unsigned char *s = (const unsigned char *)(s0 + off),
					*t = (const unsigned char *)t0;
	size_t ss = ss0 - off, i = 0, bad_chars[256], last = ts - 1;
	for (i = 0; i < 256; bad_chars[i++] = ts);
	for (i = 0; i < last; i++)
		bad_chars[t[i]] = last - i;
	for (; ss >= ts; ss -= bad_chars[s[last]], s += bad_chars[s[last]])
		for (i = last; s[i] == t[i]; i--)
			if (!i)
				return (size_t)((const char *)s - s0);
	return S_NPOS;
}

/*
 * Using libc. Supports string-only (no raw data search, just for benchmarks)
 */
size_t ss_find_libc(const char *s, const size_t off, const size_t ss,
						const char *t, const size_t ts)
{
	RETURN_IF(!ss || !ts, S_NPOS);
	const char *l = strstr(s + off, t);
	return l ? (size_t)(l - s) : S_NPOS;
}

#endif	/* S_ENABLE_EXAMPLES */


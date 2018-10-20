#ifndef SSTRINGO_H
#define SSTRINGO_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * sstringo.h
 *
 * In-place string optimizations.
 *
 * Copyright (c) 2015-2018, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "../sstring.h"

#ifndef S_DISABLE_SM_STRING_OPTIMIZATION
#define S_ENABLE_SM_STRING_OPTIMIZATION
#endif

/*
 * Structures and types
 */

#ifdef S_ENABLE_SM_STRING_OPTIMIZATION

/*
 * Up to 18-byte string when using direct storage
 */
#define OptStrRawSize 24
#define OptStrAllocSize (OptStrRawSize - sizeof(uint8_t))
#define OptStrMaxSize (OptStrAllocSize - sizeof(struct SDataSmall) - 1)

/*
 * Empty + non-empty: Empty + 50-byte string when using direct storage
 * 2 x non-empty: up to 45 bytes to be distributed between two strings
 */
#define OptStrRawSize2 48
#define OptStrAllocSize2_DD (OptStrRawSize2 - sizeof(uint8_t))
#define OptStrAllocSize2_DI (OptStrAllocSize2_DD - sizeof(srt_string *))
#define OptStr_MaxSize_DD	\
	(OptStrAllocSize2_DD - 2 * (sizeof(struct SDataSmall) + 1))
#define OptStr_MaxSize_DI	\
	(OptStrAllocSize2_DI - sizeof(struct SDataSmall) - 1)

#define OptStr_2 0x08
#define OptStr_Ix 0x10
#define OptStr_Null 0x20
#define OptStr_D 1 /* OptStrRaw */
#define OptStr_I 2 /* OptStrI */
#define OptStr_DD (OptStr_2 | 3) /* OptStrRaw2 */
#define OptStr_DI (OptStr_2 | OptStr_Ix | 4) /* OptStrDI */
#define OptStr_ID (OptStr_2 | OptStr_Ix | 5) /* OptStrDI */
#define OptStr_II (OptStr_2 | 6) /* OptStrII */

struct OptStrRaw {
	uint8_t t;
	uint8_t s_raw[OptStrAllocSize];
};

struct OptStrI {
	uint8_t t;
	srt_string *s;
};

struct OptStrRaw2 {
	uint8_t t;
	uint8_t s_raw[OptStrAllocSize2_DD];
};

struct OptStrDI {
	uint8_t t;
	uint8_t s_raw[OptStrAllocSize2_DI];
	srt_string *si;
};

struct OptStrII {
	uint8_t t;
	srt_string *s1;
	srt_string *s2;
};

union OptStr1 {
	uint8_t t;
	struct OptStrRaw d;
	struct OptStrI i;
};

union OptStr2 {
	uint8_t t;
	struct OptStrRaw2 d;
	struct OptStrDI di;
	struct OptStrII ii;
};

union OptStr {
	uint8_t t;
	union OptStr1 k;
	union OptStr2 kv;
};

typedef union OptStr1 srt_stringo1; /* one-string */

#else

struct OptStr1 {
	srt_string *s;
};

struct OptStr2 {
	srt_string *s1, *s2;
};

union OptStr {
	struct OptStr1 k;
	struct OptStr2 kv;
};

typedef struct OptStr1 srt_stringo1; /* one-string */

#endif

typedef union OptStr srt_stringo; /* one or two strings sharing space */

#ifdef S_ENABLE_SM_STRING_OPTIMIZATION

S_INLINE const srt_string *sso_get(const srt_stringo *s)
{
	RETURN_IF(!s || (s->k.t & OptStr_Null) != 0, ss_void);
	RETURN_IF(s->t == OptStr_D, (const srt_string *)s->k.d.s_raw);
	RETURN_IF(s->t == OptStr_DD, (const srt_string *)s->kv.d.s_raw);
	RETURN_IF(s->t == OptStr_DI, (const srt_string *)s->kv.di.s_raw);
	RETURN_IF(s->t == OptStr_I, s->k.i.s);
	RETURN_IF(s->t == OptStr_ID, (const srt_string *)s->kv.di.si);
	return s->kv.ii.s1; /* OptStr_II */
}

S_INLINE const uint8_t *sso_dd_get_s2_raw(const srt_stringo *s)
{
	const srt_string *s1 = (const srt_string *)s->kv.d.s_raw;
	return &s->kv.d.s_raw[0] + ss_size(s1) + sizeof(struct SDataSmall) + 1;
}

S_INLINE const srt_string *sso_dd_get_s2(const srt_stringo *s)
{
	RETURN_IF(!s || (s->t & OptStr_2) == 0, ss_void);
	return (const srt_string *)sso_dd_get_s2_raw(s);
}

S_INLINE const srt_string *sso_get_s2(const srt_stringo *s)
{
	RETURN_IF(!s || (s->t & OptStr_2) == 0, ss_void);
	RETURN_IF(s->t == OptStr_ID, (const srt_string *)s->kv.di.s_raw);
	RETURN_IF(s->t == OptStr_DI, (const srt_string *)s->kv.di.si);
	RETURN_IF(s->t == OptStr_II, s->kv.ii.s2);
	return sso_dd_get_s2(s); /* OptStr_DD */
}

S_INLINE void sso_set0(srt_stringo *so, const srt_string *s, srt_string *s0)
{
	size_t ss;
	if (!so)
		return;
	if (!s)
		s = ss_void;
	ss = ss_size(s);
	if (ss <= OptStrMaxSize) {
		srt_string *s_out = (srt_string *)so->k.d.s_raw;
		ss_alloc_into_ext_buf(s_out, OptStrMaxSize);
		so->t = OptStr_D;
		ss_cpy(&s_out, s);
	} else {
		so->t = OptStr_I;
		if (s0) {
			so->k.i.s = s0;
			s0 = NULL;
			ss_cpy(&so->k.i.s, s);
		} else
			so->k.i.s = ss_dup(s);
	}
	ss_free(&s0);
}

S_INLINE void sso_set(srt_stringo *so, const srt_string *s)
{
	sso_set0(so, s, NULL);
}

S_INLINE void sso_set20(srt_stringo *so, const srt_string *s1,
		       const srt_string *s2, srt_string *sa, srt_string *sb)
{
	size_t s1s, s2s;
	srt_string *so1, *so2;
	if (!so)
		return;
	if (!s1)
		s1 = ss_void;
	if (!s2)
		s2 = ss_void;
	s1s = ss_size(s1);
	s2s = ss_size(s2);
	if (s1s + s2s <= OptStr_MaxSize_DD) {
		so1 = (srt_string *)so->kv.di.s_raw;
		ss_alloc_into_ext_buf(so1, OptStr_MaxSize_DI);
		ss_cpy(&so1, s1);
		so->t = OptStr_DD;
		so2 = (srt_string *)sso_get_s2(so);
		ss_alloc_into_ext_buf(so2, OptStr_MaxSize_DI - s1s);
		ss_cpy(&so2, s2);
	} else if (s1s <= OptStr_MaxSize_DI) {
		so1 = (srt_string *)so->kv.di.s_raw;
		ss_alloc_into_ext_buf(so1, OptStr_MaxSize_DI);
		ss_cpy(&so1, s1);
		if (sa || sb) {
			if (sa) {
				so->kv.di.si = sa;
				sa = NULL;
			} else {
				so->kv.di.si = sb;
				sb = NULL;
			}
			ss_cpy(&so->kv.di.si, s2);
		} else
			so->kv.di.si = ss_dup(s2);
		so->t = OptStr_DI;
	} else if (s2s <= OptStr_MaxSize_DI) {
		if (sa || sb) {
			if (sa) {
				so->kv.di.si = sa;
				sa = NULL;
			} else {
				so->kv.di.si = sb;
				sb = NULL;
			}
			ss_cpy(&so->kv.di.si, s1);
		} else
			so->kv.di.si = ss_dup(s1);
		so2 = (srt_string *)so->kv.di.s_raw;
		ss_alloc_into_ext_buf(so2, OptStr_MaxSize_DI);
		ss_cpy(&so2, s2);
		so->t = OptStr_ID;
	} else {
		if (sa) {
			so->kv.ii.s1 = sa;
			sa = NULL;
			ss_cpy(&so->kv.ii.s1, s1);
		} else
			so->kv.ii.s1 = ss_dup(s1);
		if (sb) {
			so->kv.ii.s2 = sb;
			sb = NULL;
			ss_cpy(&so->kv.ii.s2, s2);
		} else
			so->kv.ii.s2 = ss_dup(s2);
		so->t = OptStr_II;
	}
	ss_free(&sa);
	ss_free(&sb);
}

S_INLINE void sso_set2(srt_stringo *so, const srt_string *s1,
		       const srt_string *s2)
{
	sso_set20(so, s1, s2, NULL, NULL);
}

S_INLINE void sso_update2(srt_stringo *so, const srt_string *s,
			  const srt_string *s2)
{
	srt_string *s0, *s20;
	if (so) {
		if ((so->t & OptStr_2) != 0) {
			if ((so->t & OptStr_Ix) != 0) {
				s0 = so->kv.di.si;
				s20 = NULL;
			} else
				if (so->t == OptStr_II) {
					s0 = so->kv.ii.s1;
					s20 = so->kv.ii.s2;
				} else
					s0 = s20 = NULL;
			sso_set20(so, s, s2, s0, s20);
		} else {
			s0 = so->t == OptStr_I ? so->k.i.s : NULL;
			sso_set0(so, s, s0);
		}
	}
}

S_INLINE void sso_update(srt_stringo *so, const srt_string *s)
{
	sso_update2(so, s, NULL);
}

S_INLINE void sso_setref(srt_stringo *so, const srt_string *s)
{
	if (so) {
		so->t = OptStr_I;
		so->k.i.s = (srt_string *)s; /* CONSTNESS */
	}
}


S_INLINE void sso_setref2(srt_stringo *so, const srt_string *s1,
			  const srt_string *s2)
{
	if (so) {
		so->t = OptStr_II;
		so->kv.ii.s1 = (srt_string *)s1; /* CONSTNESS */
		so->kv.ii.s2 = (srt_string *)s2; /* CONSTNESS */
	}
}

S_INLINE void sso_free(srt_stringo *so)
{
	if (!so)
		return;
	if (so->k.t == OptStr_I)
		ss_free(&so->k.i.s);
	else if (so->kv.t == OptStr_II) {
		ss_free(&so->kv.ii.s1);
		ss_free(&so->kv.ii.s2);
	} else if (so->kv.t == OptStr_DI)
		ss_free(&so->kv.di.si);
	so->k.t |= OptStr_Null;
}

S_INLINE void sso_free2(srt_stringo *so)
{
	sso_free(so);
}

#else

S_INLINE const srt_string *sso_get(const srt_stringo *s)
{
	return s ? s->k.s : ss_void;
}

S_INLINE const srt_string *sso_get_s2(const srt_stringo *s)
{
	return s ? s->kv.s2 : ss_void;
}

S_INLINE void sso_set(srt_stringo *so, const srt_string *s)
{
	if (so)
		so->k.s = ss_dup(s);
}

S_INLINE void sso_set2(srt_stringo *so, const srt_string *s1,
		       const srt_string *s2)
{
	if (so) {
		so->kv.s1 = ss_dup(s1);
		so->kv.s2 = ss_dup(s2);
	}
}

S_INLINE void sso_update(srt_stringo *so, const srt_string *s)
{
	if (so)
		ss_cpy(&so->k.s, s);
}

S_INLINE void sso_update2(srt_stringo *so, const srt_string *s1,
			  const srt_string *s2)
{
	if (so) {
		ss_cpy(&so->kv.s1, s1);
		ss_cpy(&so->kv.s2, s2);
	}
}

S_INLINE void sso_setref(srt_stringo *so, const srt_string *s)
{
	if (so)
		so->k.s = (srt_string *)s; /* CONSTNESS */
}

S_INLINE void sso_setref2(srt_stringo *so, const srt_string *s1,
			  const srt_string *s2)
{
	if (so) {
		so->kv.s1 = (srt_string *)s1; /* CONSTNESS */
		so->kv.s2 = (srt_string *)s2; /* CONSTNESS */
	}
}

S_INLINE void sso_free(srt_stringo *so)
{
	if (so)
		ss_free(&so->k.s);
}

S_INLINE void sso_free2(srt_stringo *so)
{
	if (so) {
		ss_free(&so->kv.s1);
		ss_free(&so->kv.s2);
	}
}

#endif /* #ifdef S_ENABLE_SM_STRING_OPTIMIZATION */

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* #ifndef SSTRINGO_H */


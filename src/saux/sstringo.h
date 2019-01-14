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
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved. Released under
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

const srt_string *sso1_get(const srt_stringo1 *s);
const srt_string *sso_get(const srt_stringo *s);
const srt_string *sso_dd_get_s2(const srt_stringo *s);
const srt_string *sso_get_s2(const srt_stringo *s);
void sso1_set(srt_stringo1 *so, const srt_string *s);
void sso_set(srt_stringo *so, const srt_string *s1, const srt_string *s2);
void sso_update(srt_stringo *so, const srt_string *s, const srt_string *s2);
void sso1_update(srt_stringo1 *so, const srt_string *s);
void sso1_setref(srt_stringo1 *so, const srt_string *s);
void sso_setref(srt_stringo *so, const srt_string *s1, const srt_string *s2);
void sso1_free(srt_stringo1 *so);
void sso_free(srt_stringo *so);
void sso_dupa(srt_stringo *s);
void sso_dupa1(srt_stringo1 *s);

#else

S_INLINE const srt_string *sso1_get(const srt_stringo1 *s)
{
	return s ? s->s : ss_void;
}

S_INLINE const srt_string *sso_get(const srt_stringo *s)
{
	return s ? s->k.s : ss_void;
}

S_INLINE const srt_string *sso_get_s2(const srt_stringo *s)
{
	return s ? s->kv.s2 : ss_void;
}

S_INLINE void sso1_set(srt_stringo1 *so, const srt_string *s)
{
	if (so)
		so->s = ss_dup(s);
}

S_INLINE void sso_set(srt_stringo *so, const srt_string *s1,
		      const srt_string *s2)
{
	if (so) {
		so->kv.s1 = ss_dup(s1);
		so->kv.s2 = ss_dup(s2);
	}
}

S_INLINE void sso1_update(srt_stringo1 *so, const srt_string *s)
{
	if (so)
		ss_cpy(&so->s, s);
}

S_INLINE void sso_update(srt_stringo *so, const srt_string *s1,
			 const srt_string *s2)
{
	if (so) {
		ss_cpy(&so->kv.s1, s1);
		ss_cpy(&so->kv.s2, s2);
	}
}

S_INLINE void sso1_setref(srt_stringo1 *so, const srt_string *s)
{
	if (so)
		so->s = (srt_string *)s; /* CONSTNESS */
}

S_INLINE void sso_setref(srt_stringo *so, const srt_string *s1,
			 const srt_string *s2)
{
	if (so) {
		so->kv.s1 = (srt_string *)s1; /* CONSTNESS */
		so->kv.s2 = (srt_string *)s2; /* CONSTNESS */
	}
}

S_INLINE void sso1_free(srt_stringo1 *so)
{
	if (so)
		ss_free(&so->s);
}

S_INLINE void sso_free(srt_stringo *so)
{
	if (so) {
		ss_free(&so->kv.s1);
		ss_free(&so->kv.s2);
	}
}

S_INLINE void sso_dupa1(srt_stringo1 *s)
{
	s->s = ss_dup(s->s);
}

S_INLINE void sso_dupa(srt_stringo *s)
{
	s->kv.s1 = ss_dup(s->kv.s1);
	s->kv.s2 = ss_dup(s->kv.s2);
}

#endif /* #ifdef S_ENABLE_SM_STRING_OPTIMIZATION */

S_INLINE srt_bool sso1_eq(const srt_string *s, const srt_stringo1 *sso1)
{
	return !ss_cmp(s, sso1_get(sso1));
}

S_INLINE srt_bool sso_eq(const srt_string *s, const srt_stringo *sso)
{
	return !ss_cmp(s, sso_get(sso));
}

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* #ifndef SSTRINGO_H */

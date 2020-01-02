#ifndef SCOMMON_H
#define SCOMMON_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * scommon.h
 *
 * Common stuff
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "sconfig.h"
#include "scopyright.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#include <BaseTsd.h>
#include <crtdefs.h>
#include <malloc.h>
/* MS VC don't support UTF-8 in sprintf, not even using _setmbcp(65001)
 * and "multi-byte character set" compile mode.
 */
#define S_NOT_UTF8_SPRINTF
#define s_alloca(size) ((size) > 0 ? _malloca(size) : NULL)
#else
#include <sys/types.h>
#define s_alloca(size) ((size) > 0 ? alloca(size) : NULL)
#endif

#ifdef S_MINIMAL /* microcontroller-related */
#define S_NOT_UTF8_SPRINTF
#endif

#if !defined(BSD4_3) && !defined(BSD4_4) && !defined(__FreeBSD__)              \
	&& !defined(_MSC_VER) && !defined(__MINGW32__)
#include <alloca.h>
#endif

#ifdef __MINGW32__
#include <malloc.h>
#define snprintf(str, size, ...) sprintf(str, __VA_ARGS__)
#endif

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#define S_WCHAR32

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L                   \
	|| __cplusplus >= 19971L || defined(_MSC_VER) && _MSC_VER >= 1800      \
	|| (defined(__APPLE_CC__) && __APPLE_CC__ >= 5658)                     \
	|| defined(__GNUC__) && (__GNUC__ > 3 || __GNUC__ == 2 && __GNUC_MINOR__ > 7)
#ifndef S_C90
#define S_C99_SUPPORT
#endif
#else
#if defined(__GNUC__) && (__GNUC__ < 2 || __GNUC__ == 2 && __GNUC_MINOR__ <= 8)
#define S_OLD_C_PREPROCESSOR
#endif
#endif

/*
 * C99 requires to define __STDC_LIMIT_MACROS before stdint.h if in C++ mode
 * (WG14/N1256 Committee Draft 20070907 ISO/IEC 9899:TC3, 7.18.2, page 257)
 */
#ifdef S_C99_SUPPORT
#ifdef __cplusplus
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>
#endif

#ifndef WCHAR_MAX
#define WCHAR_MAX ((wchar_t)-1)
#endif
#ifndef UINTPTR_MAX
#if defined(__LP64__) || defined(__64BIT__) || defined(__arch64__) || \
    defined(_LP64) || defined(__ia64__) || defined(__ia64) || \
    defined(__sparc_v9__) || defined(__sparcv9) || defined(_WIN64) || \
    defined(__x86_64__) || defined(__WORDSIZE) && __WORDSIZE == 64 || \
    defined(__ppc64__)
#define UINTPTR_MAX 0xffffffffffffffffU
#else
#define UINTPTR_MAX 0xffffffff
#endif
#endif
#ifndef UINT32_MAX
#define UINT32_MAX ((uint32_t)-1)
#endif
#ifndef INT32_MAX
#define INT32_MAX ((int32_t)0x7fffffff)
#endif
#ifndef INT32_MIN
#define INT32_MIN ((int32_t)0x80000000)
#endif
#ifndef INT64_MAX
#define INT64_MAX ((int64_t)0x7fffffffffffffffLL)
#endif
#ifndef INT64_MIN
#define INT64_MIN ((int64_t)0x8000000000000000LL)
#endif

#ifndef S_OLD_C_PREPROCESSOR
#if WCHAR_MAX > 0 && WCHAR_MAX <= 0xffff
#undef S_WCHAR32
#endif
#endif

/*
 * Context
 */

#if defined(__GNUC__) && __GNUC__ >= 4 || defined(__clang__)                   \
	|| defined(__INTEL_COMPILER)
#define S_EXPECT(expr, val) __builtin_expect(expr, val)
#else
#define S_EXPECT(expr, val) (expr)
#endif

#define S_LIKELY(expr) S_EXPECT((expr) != 0, 1)
#define S_UNLIKELY(expr) S_EXPECT((expr) != 0, 0)

#if defined(S_C99_SUPPORT) || defined(__TINYC__)
#define S_MODERN_COMPILER
#ifndef S_NO_VARGS
#define S_USE_VA_ARGS
#endif
#endif

#ifdef S_C99_SUPPORT
#ifdef _MSC_VER
#define S_INLINE __inline static
#else
#ifdef S_C99_SUPPORT
#define S_INLINE inline static
#else
#define S_INLINE static
#endif
#endif

#if defined(__clang_major__) && __clang_major__ <= 3                           \
	&& defined(__clang_minor__) && __clang_minor__ <= 4
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#elif defined(S_MODERN_COMPILER)
#define S_INLINE __inline__ static
#else
#define S_INLINE static
#endif
#if !defined(S_C99_SUPPORT) && !defined(_MSC_VER) && !defined(__cplusplus)
int snprintf(char *str, size_t size, const char *format, ...);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif
#if defined(S_MODERN_COMPILER) && !defined(_MSC_VER)
#define S_POSIX_LOCALE_SUPPORT
#endif

/*
 * Macros
 */

#define S_MIN(a, b) ((a) < (b) ? (a) : (b))
#define S_MAX(a, b) ((a) > (b) ? (a) : (b))
#define S_MIN3(a, b, c) S_MIN(S_MIN(a, b), (c))
#define S_RANGE(v, l, r) ((v) <= (l) ? (l) : (v) >= (r) ? (r) : (v))

/*
 * Bit range: 0..(n-1)
 * Bit mask range: 1..n
 */
#define S_NBIT(n) (1 << (n))
#define S_NBIT64(n) ((uint64_t)1 << (n))
#define S_NBITMASK(n) (S_NBIT(n) - 1)
#define S_NBITMASK64(n) (S_NBIT64(n) - 1)
#if UINTPTR_MAX <= 0xffffffff
#define S_NBITMASK32(n) (((S_NBIT(n - 1) - 1) << 1) | 1)
#else
#define S_NBITMASK32(n) ((uint32_t)S_NBITMASK(n))
#endif
#define RETURN_IF(a, v)                                                        \
	if (a)                                                                 \
		return (v);                                                    \
	else {                                                                 \
	}
#define ASSERT_RETURN_IF(a, v)                                                 \
	{                                                                      \
		S_ASSERT(!(a));                                                \
		RETURN_IF(a, v);                                               \
	}
#define S_UNUSED(v) (void)(v)

/*
 * Types
 */

#ifdef _MSC_VER
#if UINTPTR_MAX <= 0xffffffff
#define FMT_ZI "%i"
#define FMT_ZU "%u"
#define FMT_I "%I64i"
#define FMTSSZ_T int
#define FMTSZ_T unsigned int
#else
#define FMT_ZI "%I64i"
#define FMT_ZU "%I64u"
#define FMT_I FMT_ZI
#define FMTSSZ_T __int64
#define FMTSZ_T unsigned __int64
#endif
#else
#ifdef S_C99_SUPPORT
#define FMT_ZI "%zi"
#define FMT_ZU "%zu"
#define FMT_I "%" PRId64
#define FMT_U "%" PRIu64
#define FMT_I32 "%" PRId32
#define FMT_U32 "%" PRIu32
#else
#define FMT_ZI "%li"
#define FMT_ZU "%lu"
#define FMT_I "%li"
#define FMT_U "%lu"
#endif
#define FMTSZ_T ssize_t
#define FMTSSZ_T size_t
#endif

#ifndef FMT_I32
#define FMT_I32 "%i"
#define FMT_U32 "%u"
#endif

#if defined(_MSC_VER)
typedef SSIZE_T ssize_t;
#endif

#if !defined(S_C99_SUPPORT)
#if !defined(__GNUC__)
typedef char int8_t;
typedef short int16_t;
typedef long int32_t;
#endif
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#ifdef _MSC_VER
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
typedef long intptr_t;
typedef unsigned long uintptr_t;
#if !defined(__GNUC__)
typedef long long int64_t;
#endif
typedef unsigned long long uint64_t;
#endif
#ifndef SIZE_MAX
#define SIZE_MAX (size_t)-1
#endif
#ifndef UINTPTR_MAX
#define UINTPTR_MAX (uintptr_t)-1
#endif
#endif

typedef unsigned char srt_bool;

union s_u32 {
	uint32_t a32;
	unsigned char b[4];
};

union s_u64 {
	uint64_t a64;
	unsigned char b[8];
};

/*
 * Constants
 */

#define S_SIZET_MAX ((size_t)-1)
#define S_NPOS S_SIZET_MAX
#define S_NULL_C "(null)"
#define S_NULL_WC L"(null)"
#define S_TRUE 1
#define S_FALSE 0

/*
 * Variable argument helpers
 */

#define S_INVALID_PTR_VARG_TAIL ((const void *)-1)

S_INLINE srt_bool s_varg_tail_ptr_tag(const void *p)
{
	return p == S_INVALID_PTR_VARG_TAIL ? S_TRUE : S_FALSE;
}

	/*
	 * Artificial memory allocation limits (tests/debug)
	 */

#if defined(S_MAX_MALLOC_SIZE) && !defined(S_DEBUG_ALLOC)

S_INLINE void *s_malloc(size_t size)
{
	return size <= S_MAX_MALLOC_SIZE ? _s_malloc(size) : NULL;
}

S_INLINE void *s_calloc(size_t nmemb, size_t size)
{
	return size <= S_MAX_MALLOC_SIZE ? _s_calloc(nmemb, size) : NULL;
}

S_INLINE void *s_realloc(void *ptr, size_t size)
{
	return size <= S_MAX_MALLOC_SIZE ? _s_realloc(ptr, size) : NULL;
}

#elif defined(S_DEBUG_ALLOC)

#ifndef S_MAX_MALLOC_SIZE
#define S_MAX_MALLOC_SIZE ((size_t)-1)

#endif

S_INLINE void *s_alloc_check(void *p, size_t size, char *from_label)
{
	fprintf(stderr, "[libsrt %s @%s] alloc size: " FMT_ZU "\n",
		(p ? "OK" : "ERROR"), from_label, size);
	return p;
}

S_INLINE void *s_malloc(size_t size)
{
	void *p = size <= S_MAX_MALLOC_SIZE ? _s_malloc(size) : NULL;
	return s_alloc_check(p, size, "malloc");
}

S_INLINE void *s_calloc(size_t nmemb, size_t size)
{
	void *p = size <= S_MAX_MALLOC_SIZE ? _s_calloc(nmemb, size) : NULL;
	return s_alloc_check(p, nmemb * size, "calloc");
}

S_INLINE void *s_realloc(void *ptr, size_t size)
{
	void *p = size <= S_MAX_MALLOC_SIZE ? _s_realloc(ptr, size) : NULL;
	return s_alloc_check(p, size, "realloc");
}

S_INLINE void s_free(void *ptr)
{
	_s_free(ptr);
}

#else

#define s_malloc _s_malloc
#define s_calloc _s_calloc
#define s_realloc _s_realloc
#define s_free _s_free

#endif

/*
 * Little-endian detection is used for optimizing the data compression
 * routines and other internal code involving little-endian data
 * serialization.
 */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__       \
	|| defined(__i386__) || defined(__x86_64__) || defined(__ARMEL__)      \
	|| defined(__i960__) || defined(__TIC80__)                             \
	|| (defined(MIPSEL) && MIPSEL > 0)                                     \
	|| (defined(__MIPSEL__) && __MIPSEL__ > 0) || defined(__AVR__)         \
	|| defined(__MSP430__)                                                 \
	|| defined(__sparc__) && defined(__LITTLE_ENDIAN_DATA__)               \
	|| defined(__PPC__) && (defined(_LITTLE_ENDIAN) && _LITTLE_ENDIAN)     \
	|| defined(__IEEE_LITTLE_ENDIAN) || defined(_M_AMD64)                  \
	|| defined(_M_IX86)
#ifdef S_DISABLE_LE_OPTIMIZATIONS
#define S_ALLOW_LE_OPTIMIZATIONS 0
#else
#define S_ALLOW_LE_OPTIMIZATIONS 1
#endif
#endif

#ifndef S_ALLOW_LE_OPTIMIZATIONS
#define S_ALLOW_LE_OPTIMIZATIONS 0
#endif

#define RS_LD_X(a, T)                                                          \
	T r;                                                                   \
	memcpy(&r, a, sizeof(r));                                              \
	return r

S_INLINE uint16_t S_LD_U16(const void *a)
{
	RS_LD_X(a, uint16_t);
}

S_INLINE uint32_t S_LD_U32(const void *a)
{
	RS_LD_X(a, uint32_t);
}

S_INLINE uint64_t S_LD_U64(const void *a)
{
	RS_LD_X(a, uint64_t);
}

S_INLINE size_t S_LD_SZT(const void *a)
{
	RS_LD_X(a, size_t);
}

S_INLINE float S_LD_F(const void *a)
{
	RS_LD_X(a, float);
}

S_INLINE double S_LD_D(const void *a)
{
	RS_LD_X(a, double);
}

S_INLINE void S_ST_U16(void *a, uint16_t v)
{
	memcpy(a, &v, sizeof(v));
}

S_INLINE void S_ST_U32(void *a, uint32_t v)
{
	memcpy(a, &v, sizeof(v));
}

S_INLINE void S_ST_U64(void *a, uint64_t v)
{
	memcpy(a, &v, sizeof(v));
}

S_INLINE void S_ST_SZT(void *a, size_t v)
{
	memcpy(a, &v, sizeof(v));
}

S_INLINE uint16_t S_LD_LE_U16(const void *a)
{
#if S_ALLOW_LE_OPTIMIZATIONS
	return S_LD_U16(a);
#else
	const uint8_t *p = (const uint8_t *)a;
	return (uint16_t)(p[1] << 8 | p[0]);
#endif
}

S_INLINE void S_ST_LE_U16(void *a, uint16_t v)
{
#if S_ALLOW_LE_OPTIMIZATIONS
	S_ST_U16(a, v);
#else
	uint8_t *p = (uint8_t *)a;
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
#endif
}

S_INLINE uint32_t S_LD_LE_U32(const void *a)
{
#if S_ALLOW_LE_OPTIMIZATIONS
	return S_LD_U32(a);
#else
	const uint8_t *p = (const uint8_t *)a;
	return (uint32_t)(p[3] << 24 | p[2] << 16 | p[1] << 8 | p[0]);
#endif
}

S_INLINE void S_ST_LE_U32(void *a, uint32_t v)
{
#if S_ALLOW_LE_OPTIMIZATIONS
	S_ST_U32(a, v);
#else
	uint8_t *p = (uint8_t *)a;
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
#endif
}

S_INLINE uint64_t S_LD_LE_U64(const void *a)
{
#if S_ALLOW_LE_OPTIMIZATIONS
	return S_LD_U64(a);
#else
	const uint8_t *p = (const uint8_t *)a;
	return S_LD_LE_U32(p) | ((uint64_t)S_LD_LE_U32(p + 4) << 32);
#endif
}

S_INLINE void S_ST_LE_U64(void *a, uint64_t v)
{
#if S_ALLOW_LE_OPTIMIZATIONS
	S_ST_U64(a, v);
#else
	uint8_t *p = (uint8_t *)a;
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
	p[4] = (uint8_t)(v >> 32);
	p[5] = (uint8_t)(v >> 40);
	p[6] = (uint8_t)(v >> 48);
	p[7] = (uint8_t)(v >> 56);
#endif
}

/*
 * Packed 64-bit integer: load/store a variable-size 64 bit integer
 * that can take from 1 to 9 bytes, depending on the value.
 */
#define S_PK_U64_MAX_BYTES 9
void s_st_pk_u64(uint8_t **buf, uint64_t v);
uint64_t s_ld_pk_u64(const uint8_t **buf, size_t bs);
size_t s_pk_u64_size(const uint8_t *buf);

/*
 * Debug and alloc counter (used by the test)
 */
#ifdef _DEBUG
#define S_DEBUG
#endif
#ifdef S_DEBUG
#define S_ASSERT(a)                                                            \
	if (!(a))                                                              \
	fprintf(stderr, "S_ASSERT[file: %s (%i)]: '%s'\n", __FILE__, __LINE__, \
		#a)
#define S_ERROR(msg) fprintf(stderr, "%s (%i): %s\n", __FILE__, __LINE__, msg)
#define S_PROFILE_ALLOC_CALL dbg_cnt_alloc_calls++
extern size_t dbg_cnt_alloc_calls; /* alloc or realloc calls */
#else
#define S_ASSERT(a)
#define S_ERROR(msg)
#define S_PROFILE_ALLOC_CALL
#endif

/*
 * Standard-compliance workarounds
 */

#ifdef _MSC_VER
#define snprintf sprintf_s
#define S_FOPEN_BINARY_RW_TRUNC "wb+"
#else
#define S_FOPEN_BINARY_RW_TRUNC "w+"
#endif

/*
 * Helper functions intended to be inlined
 */

S_INLINE srt_bool s_size_t_overflow(size_t off, size_t inc)
{
	return inc > (S_SIZET_MAX - off) ? S_TRUE : S_FALSE;
}

S_INLINE size_t s_size_t_pct(size_t q, size_t pct)
{
	return q > 10000 ? (q / 100) * pct : (q * pct) / 100;
}

S_INLINE size_t s_size_t_inc_pct(size_t q, size_t pct, size_t val_if_saturated)
{
	size_t inc = s_size_t_pct(q, pct);
	return s_size_t_overflow(q, inc) ? val_if_saturated : q + inc;
}

S_INLINE size_t s_size_t_add(size_t a, size_t b, size_t val_if_saturated)
{
	return s_size_t_overflow(a, b) ? val_if_saturated : a + b;
}

S_INLINE size_t s_size_t_sub(size_t a, size_t b)
{
	return a > b ? a - b : 0;
}

unsigned slog2_32(uint32_t i);
unsigned slog2_64(uint64_t i);
unsigned slog2_ceil_32(uint32_t i);
unsigned slog2_ceil(uint64_t i);
uint64_t snextpow2(uint64_t i); /* round to next power of 2 */

S_INLINE unsigned slog2(uint64_t i)
{
	return i > 0xffffffff ? slog2_64(i) : slog2_32((uint32_t)i);
}

S_INLINE void s_move_elems(void *t, size_t t_off, const void *s, size_t s_off,
			   size_t n, size_t e_size)
{
	if (t && s)
		memmove((char *)t + t_off * e_size,
			(const char *)s + s_off * e_size, n * e_size);
}

S_INLINE void s_copy_elems(void *t, size_t t_off, const void *s, size_t s_off,
			   size_t n, size_t e_size)
{
	if (t && s)
		memcpy((char *)t + t_off * e_size,
		       (const char *)s + s_off * e_size, n * e_size);
}

/*
 * Custom "memset" functions
 */

void s_memset64(void *o, const void *s, size_t n);
void s_memset32(void *o, const void *s, size_t n);
void s_memset24(void *o, const void *s, size_t n);
void s_memset16(void *o, const void *s, size_t n);

/*
 * Least/most significant bit
 */

#define BUILD_S_LSB(FN, T)                                                     \
	S_INLINE T FN(T v)                                                     \
	{                                                                      \
		return v & (~v + 1);                                           \
	}

#define BUILD_S_MSB(FN, T)                                                     \
	S_INLINE T FN(T v)                                                     \
	{                                                                      \
		size_t i;                                                      \
		for (i = 1; i < sizeof(T) * 8 / 2; i <<= 1)                    \
			v |= (v >> i);                                         \
		return v & ~(v >> 1);                                          \
	}

BUILD_S_LSB(s_lsb8, uint8_t)
BUILD_S_LSB(s_lsb16, uint16_t)
BUILD_S_LSB(s_lsb32, uint32_t)
BUILD_S_LSB(s_lsb64, uint64_t)
BUILD_S_MSB(s_msb8, uint8_t)
BUILD_S_MSB(s_msb16, uint16_t)
BUILD_S_MSB(s_msb32, uint32_t)
BUILD_S_MSB(s_msb64, uint64_t)

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* SCOMMON_H */

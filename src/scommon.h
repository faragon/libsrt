#ifndef SCOMMON_H
#define SCOMMON_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * scommon.h
 *
 * Common definitions.
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
	#include <crtdefs.h>
	#include <BaseTsd.h>
	#include <malloc.h>
	#include <io.h>
	/* MS VC don't support UTF-8 in sprintf, not even using _setmbcp(65001)
	 * and "multi-byte character set" compile mode.
	 */
	#define S_NOT_UTF8_SPRINTF
#else
	#include <sys/types.h>
	#include <alloca.h>
	#include <unistd.h>
#endif
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>
#include <wctype.h>
#include <wchar.h>

/*
 * C99 requires to define __STDC_LIMIT_MACROS before stdint.h if in C++ mode
 * (WG14/N1256 Committee Draft 20070907 ISO/IEC 9899:TC3, 7.18.2, page 257)
 */
#ifdef __cplusplus
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

/*
 * Context
 */

#if __STDC_VERSION__ >= 199901L || __cplusplus >= 19971L || \
	defined(__GNUC__) || defined (__TINYC__) || _MSC_VER >= 1800 || \
	defined(__DMC__)
	#define S_MODERN_COMPILER
	#define S_USE_VA_ARGS
#endif

#if __STDC_VERSION__ < 199901L && !defined(_MSC_VER)
	int snprintf(char *str, size_t size, const char *format, ...);
	int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif

#if defined(S_MODERN_COMPILER) && !defined(_MSC_VER)
	#define S_POSIX_LOCALE_SUPPORT
#endif

#ifndef offsetof
	#define offsetof(s, m) ((size_t)(&((s *)0)->m))
#endif

#if defined(__CYGWIN__) && !defined(UINTPTR_MAX)
	#define UINTPTR_MAX 0xffffffff
#endif

#if UINTPTR_MAX == 0xffff
	#define S_BPWORD 2 /* 16-bit CPU */
#elif UINTPTR_MAX == 0xffffffff
	#define S_BPWORD 4 /* 32 */
#elif UINTPTR_MAX == 0xffffffffffffffff
	#define S_BPWORD 8 /* 64 */
#else /* assume 128-bit CPU */
	#define S_BPWORD 16
#endif

#define S_UALIGNMASK (S_BPWORD - 1)
#define S_ALIGNMASK (~S_UALIGNMASK)

/*
 * Variable argument helpers
 */

#ifdef S_USE_VA_ARGS /* purposes: 1) count parameters, 2) parameter syntax check */
	/* void *, char *, wchar_t * */
	#define S_NARGS_W(...) (sizeof((void * []){__VA_ARGS__})/sizeof(void *))
	#define S_NARGS_R(...) (sizeof((const void * []){__VA_ARGS__})/sizeof(const void *))
	#define S_NARGS_CW(...) (sizeof((char * []){__VA_ARGS__})/sizeof(char *))
	#define S_NARGS_CR(...) (sizeof((const char * []){__VA_ARGS__})/sizeof(const char *))
	#define S_NARGS_WW(...) (sizeof((wchar_t * []){__VA_ARGS__})/sizeof(wchar_t *))
	#define S_NARGS_WR(...) (sizeof((const wchar_t * []){__VA_ARGS__})/sizeof(const wchar_t *))
	/* ss_t * */
	#define S_NARGS_SW(...) (sizeof((ss_t * []){__VA_ARGS__})/sizeof(ss_t *))
	#define S_NARGS_SR(...) (sizeof((const ss_t * []){__VA_ARGS__})/sizeof(const ss_t *))
	#define S_NARGS_SPW(...) (sizeof((ss_t ** []){__VA_ARGS__})/sizeof(ss_t **))
	#define S_NARGS_SPR(...) (sizeof((const ss_t ** []){__VA_ARGS__})/sizeof(const ss_t **))
	/* sv_t * */
	#define S_NARGS_SVW(...) (sizeof((sv_t * []){__VA_ARGS__})/sizeof(sv_t *))
	#define S_NARGS_SVR(...) (sizeof((const sv_t * []){__VA_ARGS__})/sizeof(const sv_t *))
	#define S_NARGS_SVPW(...) (sizeof((sv_t ** []){__VA_ARGS__})/sizeof(sv_t **))
	#define S_NARGS_SVPR(...) (sizeof((const sv_t ** []){__VA_ARGS__})/sizeof(const sv_t **))
	/* st_t * */
	#define S_NARGS_STW(...) (sizeof((st_t * []){__VA_ARGS__})/sizeof(st_t *))
	#define S_NARGS_STR(...) (sizeof((const st_t * []){__VA_ARGS__})/sizeof(const st_t *))
	#define S_NARGS_STPW(...) (sizeof((st_t ** []){__VA_ARGS__})/sizeof(st_t **))
	#define S_NARGS_STPR(...) (sizeof((const st_t ** []){__VA_ARGS__})/sizeof(const st_t **))
	/* sdm_t */
	#define S_NARGS_SDMW(...) (sizeof((sdm_t * []){__VA_ARGS__})/sizeof(sdm_t *))
	#define S_NARGS_SDMR(...) (sizeof((const sdm_t * []){__VA_ARGS__})/sizeof(const sdm_t *))
	#define S_NARGS_SDMPW(...) (sizeof((sdm_t ** []){__VA_ARGS__})/sizeof(sdm_t **))
	#define S_NARGS_SDMPR(...) (sizeof((const sdm_t ** []){__VA_ARGS__})/sizeof(const sdm_t **))

#endif

/*
 * Constants
 */

#define S_SIZET_MAX	((size_t)-1)
#define S_NPOS		S_SIZET_MAX
#define S_NULL_C	"(null)"
#define S_NULL_WC	L"(null)"
#define S_TRUE		(1)
#define S_FALSE		(0)

#ifdef _MSC_VER
	#if S_BPWORD == 8
		#define FMT_ZI "%I64i"
		#define FMT_ZU "%I64u"
		#define FMT_I FMT_ZI
		#define FMTSSZ_T __int64
		#define FMTSZ_T unsigned __int64
	#else
		#define FMT_ZI "%i"
		#define FMT_ZU "%u"
		#define FMT_I "%I64i"
		#define FMTSSZ_T int
		#define FMTSZ_T unsigned int
	#endif
#else
	#define FMT_ZI "%zi"
	#define FMT_ZU "%zu"
	#if S_BPWORD >= 8
		#define FMT_I FMT_ZI
	#else
		#define FMT_I "%lli"
	#endif
	#define FMTSZ_T ssize_t
	#define FMTSSZ_T size_t
#endif

/*
 * Macros
 */

#define S_MIN(a, b) ((a) < (b) ? (a) : (b))
#define S_MAX(a, b) ((a) > (b) ? (a) : (b))
#define S_MIN3(a, b, c) S_MIN(S_MIN(a, b), (c))
#define S_ROL32(a, c) ((a) << (c) | (a) >> (32 - (c)))
#define S_ROR32(a, c) ((a) >> (c) | (a) << (32 - (c)))
#define S_NBIT(n) (1 << n)
#define S_NBITMASK(n) (S_NBIT(n) - 1)
#if defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 1
#define S_BSWAP32(a) __builtin_bswap32(a)
#else
#define S_BSWAP32(a) ((a) << 24 | (a) >> 24 | ((a) & 0xff00) << 8 | ((a) & 0xff0000) >> 8)
#endif
#define RETURN_IF(a, v) if (a) return (v)
#define ASSERT_RETURN_IF(a, v) S_ASSERT(!(a)); if (a) return (v)

/*
 * Types
 */

#if defined(_MSC_VER)
	typedef SSIZE_T ssize_t;
	typedef __int64 sint_t;
	typedef unsigned __int64 suint_t;
#elif	LONG_MAX == INT_MAX /* 32 bit or 64 bit (LLP64) mode */
	#ifdef S_MODERN_COMPILER
		typedef long long sint_t;
		typedef unsigned long long suint_t;
	#else /* no 64 bit container support: */
		typedef long sint_t;
		typedef unsigned long suint_t;
	#endif
#else /* 64 bit mode (LP64) */
	#ifdef S_MODERN_COMPILER
		typedef ssize_t sint_t;
		typedef size_t suint_t;
	#else
		typedef long sint_t;
		typedef unsigned long suint_t;
	#endif
#endif
#if S_BPWORD >= 4
	typedef int sint32_t;
	typedef unsigned int suint32_t;
#else
	typedef long sint32_t;
	typedef unsigned long suint32_t;
#endif
typedef unsigned char sbool_t;
typedef sint_t sint64_t;
typedef suint_t suint64_t;

#define SINT_MIN (8LL << ((sizeof(sint_t) * 8) - 4))
#define SINT_MAX (~SINT_MIN)
#define SINT32_MAX ((sint32_t)(0x7fffffff))
#define SINT32_MIN ((sint32_t)(0x80000000))
#define SUINT32_MAX 0xffffffff
#define SUINT32_MIN 0

/*
 * Low-level stuff
 * - Aligned memory access CPU support detection
 * - Endianess
 * - Unaligned load/store
 */

#if defined(__i386__) || defined(__x86_64__) || __ARM_ARCH >= 6 ||	\
    ((defined(_ARM_ARCH_6) || defined(__ARM_ARCH_6__)) &&		\
     !defined(__ARM_ARCH_6M__)) ||					\
    defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6Z__) ||		\
    defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6K__) ||		\
    defined(__ARM_ARCH_6T2__) || defined(_ARM_ARCH_7) ||		\
    defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) ||		\
    defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) ||		\
    defined(_ARM_ARCH_8) || defined(__ARM_ARCH_8__) ||			\
    defined(__ARM_FEATURE_UNALIGNED)
	#define S_UNALIGNED_MEMORY_ACCESS
#endif

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || \
    defined(__i386__) || defined(__x86_64__) || defined(__ARMEL__) ||	    \
    defined(__i960__) || defined(__TIC80__) || defined(__MIPSEL__) ||	    \
    defined(__AVR__) || defined(__MSP430__) ||				    \
    defined(__sparc__) && defined(__LITTLE_ENDIAN_DATA__) ||		    \
    defined(__PPC__) && (defined(_LITTLE_ENDIAN) && _LITTLE_ENDIAN) ||	    \
    defined(__IEEE_LITTLE_ENDIAN)
	#define S_IS_LITTLE_ENDIAN 1
#endif

#ifndef S_IS_LITTLE_ENDIAN
	#define S_IS_LITTLE_ENDIAN 0
#endif

#if S_IS_LITTLE_ENDIAN
	#define S_NTOH_U32(a) S_BSWAP32((unsigned)(a))
#else
	#define S_NTOH_U32(a) (a)
#endif
#define S_HTON_U32(a) S_NTOH_U32(a)

#define S_LD_X(a, T) *(T *)(a)
#define S_ST_X(a, T, v) S_LD_X(a, T) = v
#ifdef S_UNALIGNED_MEMORY_ACCESS
	#define S_LD_U32(a) S_LD_X(a, unsigned)
	#define S_LD_SZT(a) S_LD_X(a, size_t)
	#define S_ST_U32(a, v) S_ST_X(a, unsigned, v)
	#define S_ST_SZT(a, v) S_ST_X(a, size_t, v)
#else /* Aligned access supported only for 32 and >= 64 bit CPUs */
	#if S_IS_LITTLE_ENDIAN
		#define S_UALD_U32(a) (*(unsigned char *)(a) |	\
			*((unsigned char *)(a) + 1) << 8 |	\
			*((unsigned char *)(a) + 2) << 16 |	\
			*((unsigned char *)(a) + 3) << 24)
		#define S_UALD_U64(a)	\
			((S_UALD_U32(a + 4) << 32) | S_UALD_U32(a))
	#else
		#define S_UALD_U32(a) (*(unsigned char *)(a) << 24 |	\
			*((unsigned char *)(a) + 1) << 16 |		\
			*((unsigned char *)(a) + 2) << 8 |		\
			*((unsigned char *)(a) + 3))
		#define S_UALD_U64(a)	\
			(((suint64_t)S_UALD_U32(a) << 32) | S_UALD_U32(a + 4))
	#endif
	#define S_LD_U32(a)		 			\
		(((uintptr_t)(a) & S_UALIGNMASK) ?		\
			S_UALD_U32(a) : S_LD_X(a, unsigned))
	#define S_LD_U64(a)		 			\
		(((uintptr_t)(a) & S_UALIGNMASK) ?		\
			S_UALD_U64(a) : S_LD_X(a, suint_t))
	#define S_LD_SZT(a) \
		(sizeof(size_t) == 4 ? S_LD_U32(a) : S_LD_U64(a))
	#define S_UAST_X(a, T, v) { T w = (T)v; memcpy((a), &w, sizeof(w)); }
	#define S_ST_SZT(a, v) S_UAST_X(a, size_t, v)
	#define S_ST_U32(a, v) S_UAST_X(a, suint32_t, v)
#endif

#if defined(MSVC) && defined(_M_X86)
	#include <emmintrin.h>
	#define S_PREFETCH_R(address) 					\
		if (address)						\
			_mm_prefetch((char *)(address), _MM_HINT_T0)
	#define S_PREFETCH_W(address) S_PREFETCH_R(address)
#endif
#if defined(__GNUC__)
	#define S_PREFETCH_GNU(address, rw)		\
		if (address)				\
			__builtin_prefetch(address, rw)
	#define S_PREFETCH_R(address) S_PREFETCH_GNU(address, 0)
	#define S_PREFETCH_W(address) S_PREFETCH_GNU(address, 1)
#endif
#ifndef S_PREFETCH_R
	#define S_PREFETCH_R(address)
#endif
#ifndef S_PREFETCH_W
	#define S_PREFETCH_W(address)
#endif

/*
 * Debug and alloc counter (used by the test)
 */
#ifdef _DEBUG
	#define S_DEBUG
#endif
#ifdef S_DEBUG
	#define S_ASSERT(a)						    \
		!(a) &&							    \
		fprintf(stderr, "S_ASSERT[function: %s, line: %i]: '%s'\n", \
		__FUNCTION__, __LINE__, #a)
	#define S_ERROR(msg) \
		fprintf(stderr, "%s: %s\n", __FUNCTION__, msg)
	#define S_PROFILE_ALLOC_CALL dbg_cnt_alloc_calls++
	extern size_t dbg_cnt_alloc_calls;      /* alloc or realloc calls */
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
#define posix_open _open
#define posix_read _read
#define posix_write _write
#define posix_close _close
#else
#define posix_open open
#define posix_read read
#define posix_write write
#define posix_close close
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif

/*
 * Helper functions intended to be inlined
 */

static void s_move_elems(void *t, const size_t t_off, const void *s, const size_t s_off, const size_t n, const size_t e_size)
{
        memmove((char *)t + t_off * e_size, (const char *)s + s_off * e_size, n * e_size);
}

static void s_copy_elems(void *t, const size_t t_off, const void *s, const size_t s_off, const size_t n, const size_t e_size)
{
	if (t && s && n)
	        memcpy((char *)t + t_off * e_size, (const char *)s + s_off * e_size, n * e_size);
}

static size_t s_load_size_t(const void *aligned_base_address, const size_t offset)
{
	return S_LD_SZT(((char *)aligned_base_address) + offset);
}

static sbool_t s_size_t_overflow(const size_t off, const size_t inc)
{
	return inc > (S_SIZET_MAX - off) ? S_TRUE : S_FALSE;
}

/*
 * Integer log2(N) approximation
 */

static unsigned slog2(suint_t i)
{
	unsigned o = 0, test;
	#define SLOG2STEP(mask, bits)				\
			test = !!(i & mask);			\
			o |= test * bits;			\
			i = test * (i >> bits) | !test * i;
#if INTPTR_MAX >= INT64_MAX
	SLOG2STEP(0xffffffff00000000, 32);
#endif
#if INTPTR_MAX >= INT32_MAX
	SLOG2STEP(0xffff0000, 16);
#endif
	SLOG2STEP(0xff00, 8);
	SLOG2STEP(0xf0, 4);
	SLOG2STEP(0x0c, 2);
	SLOG2STEP(0x02, 1);
	#undef SLOG2STEP
	return o;
}

/*
 * Custom "memset" functions
 */

static void s_memset32(unsigned char *o, unsigned data, size_t n)
{
	size_t k = 0, n4 = n / 4;
	unsigned *o32;
#if defined(S_UNALIGNED_MEMORY_ACCESS) || S_BPWORD == 4
	size_t ua_head = (intptr_t)o & 3;
	if (ua_head && n4) {
		S_ST_U32(o, data);
		S_ST_U32(o + n - 4, data);
		o32 = (unsigned *)(o + 4 - ua_head);
	#if S_IS_LITTLE_ENDIAN
		data = S_ROL32(data, ua_head * 8);
	#else
		data = S_ROR32(data, ua_head * 8);
	#endif
		k = 0;
		n4--;
		for (; k < n4; k++)
			o32[k] = data;
	} else
#endif
	{
		o32 = (unsigned *)o;
		for (; k < n4; k++)
			S_ST_U32(o32 + k, data);
	}
}

static void s_memset24(unsigned char *o, const unsigned char *data, size_t n)
{
	size_t k = 0;
	if (n >= 15) {
		size_t ua_head = (intptr_t)o & 3;
		if (ua_head) {
			memcpy(o + k, data, 3);
			k = 4 - ua_head;
		}
		unsigned *o32 = (unsigned *)(o + k);
		union { unsigned a32; char b[4]; } d[3];
		size_t i, j, copy_size = n - k, c12 = (copy_size / 12);
		for (i = 0; i < 3; i ++)
			for (j = 0; j < 4; j++)
				d[i].b[j] = data[(j + k + i * 4) % 3];
		for (i = 0; i < c12; i++) {
			*o32++ = d[0].a32;
			*o32++ = d[1].a32;
			*o32++ = d[2].a32;
		}
		k = c12 * 12;
	}
	for (; k < n; k += 3)
		memcpy(o + k, data, 3);
}

#ifdef __cplusplus
};      /* extern "C" { */
#endif
#endif	/* SCOMMON_H */


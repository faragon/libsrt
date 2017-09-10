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
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "sconfig.h"

#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
	#define _CRT_NONSTDC_NO_DEPRECATE
	#include <crtdefs.h>
	#include <BaseTsd.h>
	#include <malloc.h>
	/* MS VC don't support UTF-8 in sprintf, not even using _setmbcp(65001)
	 * and "multi-byte character set" compile mode.
	 */
	#define S_NOT_UTF8_SPRINTF
#else
	#include <sys/types.h>
#endif

#ifdef S_MINIMAL /* microcontroller-related */
	#define S_NOT_UTF8_SPRINTF
#endif

#if !defined(BSD4_3) && !defined(BSD4_4) && !defined(__FreeBSD__) && \
    !defined(_MSC_VER) && !defined(__MINGW32__)
	#include <alloca.h>
#endif

#ifdef __MINGW32__
	#include <malloc.h>
	#define snprintf(str, size, ...) sprintf(str, __VA_ARGS__)
#endif

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
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

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L || \
    __cplusplus >= 19971L || defined(_MSC_VER) && _MSC_VER >= 1800 || \
    (defined(__APPLE_CC__) && __APPLE_CC__ >= 5658) || \
	defined(__GNUC__)
	#define S_C99_SUPPORT
#endif

#if defined(S_C99_SUPPORT) || defined(__GNUC__) || defined (__TINYC__)
	#define S_MODERN_COMPILER
	#define S_USE_VA_ARGS
#endif

#ifdef S_C99_SUPPORT
	#ifdef _MSC_VER
		#define S_INLINE __inline static
	#else
		#define S_INLINE inline static
	#endif
	#define __STDC_FORMAT_MACROS /* req for clang 3.4 */
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
 * Macros
 */

#define S_MIN(a, b) ((a) < (b) ? (a) : (b))
#define S_MAX(a, b) ((a) > (b) ? (a) : (b))
#define S_MIN3(a, b, c) S_MIN(S_MIN(a, b), (c))
#define S_ROL32(a, c) ((a) << (c) | (a) >> (32 - (c)))
#define S_ROR32(a, c) ((a) >> (c) | (a) << (32 - (c)))
#define S_NBIT(n) (1 << n)
#define S_NBITMASK(n) (S_NBIT(n) - 1)
#if defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 3
#define S_BSWAP32(a) __builtin_bswap32(a) /* Added in GCC 4.3 */
#else
#define S_BSWAP32(a) ((a) << 24 | (a) >> 24 | ((a) & 0xff00) << 8 | ((a) & 0xff0000) >> 8)
#endif
#define RETURN_IF(a, v) if (a) return (v); else {}
#define ASSERT_RETURN_IF(a, v) { S_ASSERT(!(a)); RETURN_IF(a, v); }

/*
 * Types
 */

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
	#ifdef S_C99_SUPPORT
		#define FMT_ZI "%zi"
		#define FMT_ZU "%zu"
		#define FMT_I "%" PRId64
	#else
		#define FMT_ZI "%lli"
		#define FMT_ZU "%llu"
		#define FMT_I "%lli"
	#endif
	#define FMTSZ_T ssize_t
	#define FMTSSZ_T size_t
#endif

#if defined(_MSC_VER)
	typedef SSIZE_T ssize_t;
#endif

#if !defined(S_C99_SUPPORT) && !defined(__GNUC__)
	typedef char int8_t;
	typedef short int16_t;
	typedef long int32_t;
	typedef unsigned char uint8_t;
	typedef unsigned short uint16_t;
	typedef unsigned long uint32_t;
	#ifdef _MSC_VER
		typedef __int64 int64_t;
		typedef unsigned __int64 uint64_t;
	#else
		typedef long long int64_t;
		typedef unsigned long long uint64_t;
	#endif
#endif

typedef unsigned char sbool_t;

union s_u32 {
	uint32_t a32;
	unsigned char b[4];
};

/* Integer compare functions */

#define BUILD_SINT_CMPF(fn, T)				\
        S_INLINE int fn(T a, T b) {              	\
                return a > b ? 1 : a < b ? -1 : 0;	\
        }

BUILD_SINT_CMPF(scmp_int32, int32_t)
BUILD_SINT_CMPF(scmp_uint32, uint32_t)
BUILD_SINT_CMPF(scmp_int64, int64_t)
BUILD_SINT_CMPF(scmp_uint64, uint64_t)
BUILD_SINT_CMPF(scmp_ptr, const void *)

/*
 * Constants
 */

#define S_SIZET_MAX	((size_t)-1)
#define S_NPOS		S_SIZET_MAX
#define S_NULL_C	"(null)"
#define S_NULL_WC	L"(null)"
#define S_TRUE		(1)
#define S_FALSE		(0)
#define S_CRC32_INIT     0
#define S_ADLER32_INIT   1
#define SINT32_MAX	((int32_t)0x7fffffff)
#define SINT32_MIN	((int32_t)0x80000000)
#define SUINT32_MAX	((uint32_t)-1)
#define SUINT32_MIN	0
#define SINT64_MAX	((int64_t)0x7fffffffffffffff)
#define SINT64_MIN	((int64_t)0x8000000000000000)
#define SUINT64_MAX	((uint64_t)-1)
#define SUINT64_MIN	0

/*
 * Variable argument helpers
 */

#define S_INVALID_PTR_VARG_TAIL	((const void *)-1)

S_INLINE sbool_t s_varg_tail_ptr_tag(const void *p)
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
 * Low-level stuff
 * - Aligned memory access CPU support detection
 * - Endianess
 * - Unaligned load/store
 */

#if (defined(__i386__) || defined(__x86_64__) ||			\
    (defined(__ARM_ARCH) && __ARM_ARCH >= 6) ||				\
    defined(_ARM_ARCH_6) || defined(__ARM_ARCH_6__) ||			\
    defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6Z__) ||		\
    defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6K__) ||		\
    defined(__ARM_ARCH_6T2__) || defined(_ARM_ARCH_7) ||		\
    defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) ||		\
    defined(__ARM_ARCH_7R__) ||						\
    defined(_ARM_ARCH_8) || defined(__ARM_ARCH_8__) ||			\
    (defined(__ARM_NEON) && __ARM_NEON > 0) ||				\
    defined(__ARM_FEATURE_UNALIGNED) ||					\
    defined(_MIPS_ARCH_OCTEON) && _MIPS_ARCH_OCTEON ||			\
    defined(__OCTEON__) && __OCTEON__ ||				\
    defined(__ppc__) || defined(__POWERPC__) ||				\
    defined(_M_AMD64) || defined(_M_IX86)) &&				\
    (!defined(__ARM_ARCH_6M__) && !defined(__ARM_ARCH_7M__))
	#define S_UNALIGNED_MEMORY_ACCESS
#endif

#if defined(S_FORCE_DISABLE_UNALIGNED) && defined(S_UNALIGNED_MEMORY_ACCESS)
	#undef S_UNALIGNED_MEMORY_ACCESS
#endif

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || \
    defined(__i386__) || defined(__x86_64__) || defined(__ARMEL__) ||	    \
    defined(__i960__) || defined(__TIC80__) || defined(__MIPSEL__) ||	    \
    defined(__AVR__) || defined(__MSP430__) ||				    \
    defined(__sparc__) && defined(__LITTLE_ENDIAN_DATA__) ||		    \
    defined(__PPC__) && (defined(_LITTLE_ENDIAN) && _LITTLE_ENDIAN) ||	    \
    defined(__IEEE_LITTLE_ENDIAN) || defined(_M_AMD64) || defined(_M_IX86)
	#define S_IS_LITTLE_ENDIAN 1
#endif

#ifndef S_IS_LITTLE_ENDIAN
	#define S_IS_LITTLE_ENDIAN 0
#endif

/*
 * NTOH: "network (big endian) to host"
 * LTOH: "little endian to host"
 */
#if S_IS_LITTLE_ENDIAN
	#define S_NTOH_U32(a) S_BSWAP32((uint32_t)(a))
	#define S_LTOH_U32(a) (a)
	#define S_U32_BYTE0(a) ((a) & 0xff)
	#define S_U32_BYTE1(a) ((a >> 8) & 0xff)
	#define S_U32_BYTE2(a) ((a >> 16) & 0xff)
	#define S_U32_BYTE3(a) ((a >> 24) & 0xff)
#else
	#define S_NTOH_U32(a) (a)
	#define S_LTOH_U32(a) S_BSWAP32((uint32_t)(a))
	#define S_U32_BYTE0(a) ((a >> 24) & 0xff)
	#define S_U32_BYTE1(a) ((a >> 16) & 0xff)
	#define S_U32_BYTE2(a) ((a >> 8) & 0xff)
	#define S_U32_BYTE3(a) ((a) & 0xff)
#endif
#define S_HTON_U32(a) S_NTOH_U32(a)
#define S_LD_X(a, T) *(T *)(a)
#define S_ST_X(a, T, v) S_LD_X(a, T) = v

S_INLINE uint32_t s_ld_u32(const void *a)
{
	return S_LD_X(a, const uint32_t);
}

S_INLINE uint64_t s_ld_u64(const void *a)
{
	return S_LD_X(a, const uint64_t);
}

S_INLINE size_t s_ld_szt(const void *a)
{
	return S_LD_X(a, const size_t);
}

S_INLINE void s_st_u32(void *a, uint32_t v)
{
	S_ST_X(a, uint32_t, v);
}

S_INLINE void s_st_u64(void *a, uint64_t v)
{
	S_ST_X(a, uint64_t, v);
}

S_INLINE void s_st_szt(void *a, size_t v)
{
	S_ST_X(a, size_t, v);
}

S_INLINE uint32_t *s_mar_u32(void *a)
{
	return (uint32_t *)a;
}

S_INLINE unsigned short s_ld_le_u16(const void *a)
{
	const unsigned char *p = (const unsigned char *)a;
	return (unsigned short)(p[1] << 8 | p[0]);
}

S_INLINE void s_st_le_u16(void *a, unsigned short v)
{
	unsigned char *p = (unsigned char *)a;
	p[0] = (unsigned char)v;
	p[1] = (unsigned char)(v >> 8);
}

S_INLINE unsigned int s_ld_le_u32(const void *a)
{
	const unsigned char *p = (const unsigned char *)a;
	return (unsigned int)(p[3] << 24 | p[2] << 16 | p[1] << 8 | p[0]);
}

S_INLINE void s_st_le_u32(void *a, unsigned int v)
{
	unsigned char *p = (unsigned char *)a;
	p[0] = (unsigned char)v;
	p[1] = (unsigned char)(v >> 8);
	p[2] = (unsigned char)(v >> 16);
	p[3] = (unsigned char)(v >> 24);
}

#define S_LD_LE_U16(a) s_ld_le_u16(a)
#define S_ST_LE_U16(a, v) s_st_le_u16(a, v)
#define S_LD_LE_U32(a) s_ld_le_u32(a)
#define S_ST_LE_U32(a, v) s_st_le_u32(a, v)

#ifdef S_UNALIGNED_MEMORY_ACCESS
	#define S_LD_U32(a) s_ld_u32((const void *)(a))
	#define S_LD_U64(a) s_ld_u64((const void *)(a))
	#define S_LD_SZT(a) s_ld_szt((const void *)(a))
	#define S_ST_U32(a, v) s_st_u32((void *)(a), v)
	#define S_ST_U64(a, v) s_st_u64((void *)(a), v)
	#define S_ST_SZT(a, v) s_st_szt((void *)(a), v)
#else /* Aligned access supported only for 32 and >= 64 bit CPUs */
	#if S_IS_LITTLE_ENDIAN
		#define S_LD_U32(a)					\
			((uint32_t)*(unsigned char *)(a) |		\
			 (uint32_t)*((unsigned char *)(a) + 1) << 8 |	\
			 (uint32_t)*((unsigned char *)(a) + 2) << 16 |	\
			 (uint32_t)*((unsigned char *)(a) + 3) << 24)
		#define S_LD_U64(a)					\
			((uint64_t)S_LD_U32(a) |			\
			 (uint64_t)(S_LD_U32((unsigned char *)(a) +	\
						4)) << 32)
	#else
		#define S_LD_U32(a)					\
			((uint32_t)*(unsigned char *)(a) << 24 |	\
			 (uint32_t)*((unsigned char *)(a) + 1) << 16 |	\
			 (uint32_t)*((unsigned char *)(a) + 2) << 8 |	\
			 (uint32_t)*((unsigned char *)(a) + 3))
		#define S_LD_U64(a)				\
			(((uint64_t)S_LD_U32(a)) << 32 |	\
			 S_LD_U32((unsigned char *)(a) + 4))
	#endif
	S_INLINE size_t S_LD_SZT(const void *a)
	{
		size_t tmp;
		memcpy(&tmp, a, sizeof(tmp));
		return tmp;
	}
	#define S_UAST_X(a, T, v) { T w = (T)v; memcpy((a), &w, sizeof(w)); }
	#define S_ST_U32(a, v) S_UAST_X(a, uint32_t, v)
	#define S_ST_U64(a, v) S_UAST_X(a, uint64_t, v)
	#define S_ST_SZT(a, v) S_UAST_X(a, size_t, v)
#endif

#if defined(MSVC) && defined(_M_X86)
	#include <emmintrin.h>
	#define S_PREFETCH_R(address) 					\
		if (address)						\
			_mm_prefetch((char *)(address), _MM_HINT_T0)
	#define S_PREFETCH_W(address) S_PREFETCH_R(address)
#endif

#if (defined(_M_X86) || defined(__x86_64__) || defined(__ARM_PCS_VFP) || \
     defined(__PPC__) || defined(__sparc__)) && !defined(__SOFTFP__)
	#define S_HW_FPU
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
		if (!(a))						    \
			fprintf(stderr, "S_ASSERT[file: %s (%i)]: '%s'\n",  \
			__FILE__, __LINE__, #a)
	#define S_ERROR(msg)						    \
		fprintf(stderr, "%s (%i): %s\n", __FILE__, __LINE__, msg)
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
#define S_FOPEN_BINARY_RW_TRUNC "wb+"
#else
#define S_FOPEN_BINARY_RW_TRUNC "w+"
#endif

/*
 * Helper functions intended to be inlined
 */

S_INLINE void s_move_elems(void *t, const size_t t_off, const void *s, const size_t s_off, const size_t n, const size_t e_size)
{
        memmove((char *)t + t_off * e_size, (const char *)s + s_off * e_size, n * e_size);
}

S_INLINE void s_copy_elems(void *t, const size_t t_off, const void *s, const size_t s_off, const size_t n, const size_t e_size)
{
	if (t && s && n)
	        memcpy((char *)t + t_off * e_size, (const char *)s + s_off * e_size, n * e_size);
}

S_INLINE size_t s_load_size_t(const void *aligned_base_address, const size_t offset)
{
	return S_LD_SZT(((const char *)aligned_base_address) + offset);
}

S_INLINE sbool_t s_size_t_overflow(const size_t off, const size_t inc)
{
	return inc > (S_SIZET_MAX - off) ? S_TRUE : S_FALSE;
}

S_INLINE size_t s_size_t_pct(const size_t q, const size_t pct)
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

S_INLINE size_t s_size_t_mul(size_t a, size_t b, size_t val_if_saturated)
{
	RETURN_IF(b == 0, 0);
	return a > SIZE_MAX / b ? val_if_saturated : a * b;
}

unsigned slog2(uint64_t i);

/*
 * Custom "memset" functions
 */

void s_memset32(void *o, uint32_t data, size_t n);
void s_memset24(unsigned char *o, const unsigned char *data, size_t n);


S_INLINE void s_memcpy2(void *o, const void *i)
{
#ifdef S_UNALIGNED_MEMORY_ACCESS
	S_ST_X(o, unsigned short, S_LD_X(i, const unsigned short));
#else
	((char *)o)[0] = ((const char *)i)[0];
	((char *)o)[1] = ((const char *)i)[1];
#endif
}

S_INLINE void s_memcpy4(void *o, const void *i)
{
	S_ST_U32(o, S_LD_U32(i));
}

S_INLINE void s_memcpy5(void *o, const void *i)
{
	s_memcpy4(o, i);
	((char *)o)[4] = ((const char *)i)[4];
}

S_INLINE void s_memcpy6(void *o, const void *i)
{
	s_memcpy4(o, i);
	s_memcpy2((char *)o + 4, (const char *)i + 4);
}

/*
 * Least/most significant bit
 */

#define BUILD_S_LSB(FN, T)		\
	S_INLINE T FN(T v)		\
	{				\
		return v & (~v + 1);	\
	}

#define BUILD_S_MSB(FN, T, NBITS)			\
	S_INLINE T FN(T v)				\
	{						\
		int i;					\
		for (i = 1; i < NBITS / 2; i <<= 1)	\
			v |= (v >> i);			\
		return v & ~(v >> 1);			\
	}

BUILD_S_LSB(s_lsb8, uint8_t)
BUILD_S_LSB(s_lsb16, uint16_t)
BUILD_S_LSB(s_lsb32, uint32_t)
BUILD_S_LSB(s_lsb64, uint64_t)
BUILD_S_MSB(s_msb8, uint8_t, 8)
BUILD_S_MSB(s_msb16, uint16_t, 16)
BUILD_S_MSB(s_msb32, uint32_t, 32)
BUILD_S_MSB(s_msb64, uint64_t, 64)

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif	/* SCOMMON_H */


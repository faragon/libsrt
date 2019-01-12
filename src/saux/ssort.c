/*
 * ssort.c
 *
 * Sorting algorithms
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "ssort.h"

/*
 * Templates
 */

#define BUILD_COUNT_SORT_x8(FN, T, CNT_T, OFF)                                 \
	S_INLINE void FN(T *b, size_t elems)                                   \
	{                                                                      \
		size_t i, j;                                                   \
		CNT_T cnt[256];                                                \
		memset(cnt, 0, sizeof(cnt));                                   \
		for (i = 0; i < elems; i++)                                    \
			cnt[b[i] + (OFF)]++;                                   \
		for (i = j = 0; j < 256 && i < elems; j++)                     \
			if (cnt[j]) {                                          \
				memset(b + i, (int)(j - (OFF)), cnt[j]);       \
				i += cnt[j];                                   \
			}                                                      \
	}

#define BUILD_SWAP(FN, T)                                                      \
	S_INLINE void FN(T *b, size_t i, size_t j)                             \
	{                                                                      \
		T tmp = b[i];                                                  \
		b[i] = b[j];                                                   \
		b[j] = tmp;                                                    \
	}

#define BUILD_SORT2(FN, T, SWAPF)                                              \
	S_INLINE void FN(T *b)                                                 \
	{                                                                      \
		if (b[0] > b[1])                                               \
			SWAPF(b, 0, 1);                                        \
	}

#define BUILD_SORT3(FN, T, SWAPF, SORT2F)                                      \
	S_INLINE void FN(T *b)                                                 \
	{                                                                      \
		if (b[0] > b[2])                                               \
			SWAPF(b, 0, 2);                                        \
		SORT2F(b);                                                     \
	}

#define BUILD_SORT4(FN, T, SWAPF, SORT2F)                                      \
	S_INLINE void FN(T *b)                                                 \
	{                                                                      \
		SORT2F(b);                                                     \
		SORT2F(b + 2);                                                 \
		if (b[2] < b[0])                                               \
			SWAPF(b, 0, 2);                                        \
		SORT2F(b + 1);                                                 \
		if (b[2] < b[1])                                               \
			SWAPF(b, 1, 2);                                        \
		SORT2F(b + 2);                                                 \
	}

#define BUILD_MSD_RADIX_SORT(FN, T, TC, MSBF, SWPF, S2F, S3F, S4F, OFF)        \
	static void FN##_aux(TC acc, TC msd_bit, T *b, size_t elems)           \
	{                                                                      \
		TC aux;                                                        \
		size_t i = 0, j = elems - 1;                                   \
		for (; i < elems; i++) {                                       \
			aux = (TC)b[i] + (OFF);                                \
			if ((aux & msd_bit) != 0) {                            \
				for (; (((TC)b[j] + (OFF)) & msd_bit) != 0     \
				       && i < j;                               \
				     j--)                                      \
					;                                      \
				if (j <= i)                                    \
					break;                                 \
				SWPF(b, i, j);                                 \
				j--;                                           \
			}                                                      \
		}                                                              \
		acc &= ~msd_bit;                                               \
		if (acc) {                                                     \
			size_t elems0 = i, elems1 = elems - elems0;            \
			if (elems0 > 4 || elems1 > 4)                          \
				msd_bit = MSBF(acc);                           \
			if (elems0 > 1) {                                      \
				if (elems0 > 4)                                \
					FN##_aux(acc, msd_bit, b, elems0);     \
				else if (elems0 == 4)                          \
					S4F(b);                                \
				else if (elems0 == 3)                          \
					S3F(b);                                \
				else                                           \
					S2F(b);                                \
			}                                                      \
			if (elems1 > 1) {                                      \
				if (elems1 > 4)                                \
					FN##_aux(acc, msd_bit, b + i, elems1); \
				else if (elems1 == 4)                          \
					S4F(b + i);                            \
				else if (elems1 == 3)                          \
					S3F(b + i);                            \
				else                                           \
					S2F(b + i);                            \
			}                                                      \
		}                                                              \
	}                                                                      \
	static void FN(T *b, size_t elems)                                     \
	{                                                                      \
		size_t i;                                                      \
		TC acc = 0;                                                    \
		for (i = 1; i < elems; i++)                                    \
			acc |= (((TC)b[i - 1] + (OFF)) ^ ((TC)b[i] + (OFF)));  \
		if (acc)                                                       \
			FN##_aux(acc, MSBF(acc), b, elems);                    \
	}

#ifndef S_MINIMAL

/* clang-format off */

BUILD_COUNT_SORT_x8(s_count_sort_i8, int8_t, size_t, 1<<7)
BUILD_COUNT_SORT_x8(s_count_sort_u8, uint8_t, size_t, 0)
BUILD_COUNT_SORT_x8(s_count_sort_i8_small, int8_t, uint8_t, 1<<7)
BUILD_COUNT_SORT_x8(s_count_sort_u8_small, uint8_t, uint8_t, 0)
BUILD_SWAP(s_swap_i16, int16_t)
BUILD_SORT2(s_sort2_i16, int16_t, s_swap_i16)
BUILD_SORT3(s_sort3_i16, int16_t, s_swap_i16, s_sort2_i16)
BUILD_SORT4(s_sort4_i16, int16_t, s_swap_i16, s_sort2_i16)
BUILD_SWAP(s_swap_u16, uint16_t)
BUILD_SORT2(s_sort2_u16, uint16_t, s_swap_u16)
BUILD_SORT3(s_sort3_u16, uint16_t, s_swap_u16, s_sort2_u16)
BUILD_SORT4(s_sort4_u16, uint16_t, s_swap_u16, s_sort2_u16)
BUILD_SWAP(s_swap_i32, int32_t)
BUILD_SORT2(s_sort2_i32, int32_t, s_swap_i32)
BUILD_SORT3(s_sort3_i32, int32_t, s_swap_i32, s_sort2_i32)
BUILD_SORT4(s_sort4_i32, int32_t, s_swap_i32, s_sort2_i32)
BUILD_SWAP(s_swap_u32, uint32_t)
BUILD_SORT2(s_sort2_u32, uint32_t, s_swap_u32)
BUILD_SORT3(s_sort3_u32, uint32_t, s_swap_u32, s_sort2_u32)
BUILD_SORT4(s_sort4_u32, uint32_t, s_swap_u32, s_sort2_u32)
BUILD_SWAP(s_swap_i64, int64_t)
BUILD_SORT2(s_sort2_i64, int64_t, s_swap_i64)
BUILD_SORT3(s_sort3_i64, int64_t, s_swap_i64, s_sort2_i64)
BUILD_SORT4(s_sort4_i64, int64_t, s_swap_i64, s_sort2_i64)
BUILD_SWAP(s_swap_u64, uint64_t)
BUILD_SORT2(s_sort2_u64, uint64_t, s_swap_u64)
BUILD_SORT3(s_sort3_u64, uint64_t, s_swap_u64, s_sort2_u64)
BUILD_SORT4(s_sort4_u64, uint64_t, s_swap_u64, s_sort2_u64)
BUILD_MSD_RADIX_SORT(s_msd_radix_sort_i16, int16_t, uint16_t, s_msb16,
		     s_swap_i16, s_sort2_i16, s_sort3_i16, s_sort4_i16, 1<<15)
BUILD_MSD_RADIX_SORT(s_msd_radix_sort_u16, uint16_t, uint16_t, s_msb16,
		     s_swap_u16, s_sort2_u16, s_sort3_u16, s_sort4_u16, 0)
BUILD_MSD_RADIX_SORT(s_msd_radix_sort_i32, int32_t, uint32_t, s_msb32,
		     s_swap_i32, s_sort2_i32, s_sort3_i32, s_sort4_i32, 1UL<<31)
BUILD_MSD_RADIX_SORT(s_msd_radix_sort_u32, uint32_t, uint32_t, s_msb32,
		     s_swap_u32, s_sort2_u32, s_sort3_u32, s_sort4_u32, 0)
BUILD_MSD_RADIX_SORT(s_msd_radix_sort_i64, int64_t, uint64_t, s_msb64,
		     s_swap_i64, s_sort2_i64, s_sort3_i64, s_sort4_i64,
		     (uint64_t)1<<63)
BUILD_MSD_RADIX_SORT(s_msd_radix_sort_u64, uint64_t, uint64_t, s_msb64,
		     s_swap_u64, s_sort2_u64, s_sort3_u64, s_sort4_u64, 0)

/*
 * Sort functions
 */

#define SSORT_CHECK(b, elems)                                                  \
	if (!b || elems <= 1)                                                  \
	return

void ssort_i8(int8_t *b, size_t elems)
{
	SSORT_CHECK(b, elems);
	if (elems < 256)
		s_count_sort_i8_small(b, elems);
	else
		s_count_sort_i8(b, elems);
}

/* clang-format on */

void ssort_u8(uint8_t *b, size_t elems)
{
	SSORT_CHECK(b, elems);
	if (elems < 256)
		s_count_sort_u8_small(b, elems);
	else
		s_count_sort_u8(b, elems);
}

void ssort_i16(int16_t *b, size_t elems)
{
	SSORT_CHECK(b, elems);
	s_msd_radix_sort_i16(b, elems);
}

void ssort_u16(uint16_t *b, size_t elems)
{
	SSORT_CHECK(b, elems);
	s_msd_radix_sort_u16(b, elems);
}

void ssort_i32(int32_t *b, size_t elems)
{
	SSORT_CHECK(b, elems);
	s_msd_radix_sort_i32(b, elems);
}

void ssort_u32(uint32_t *b, size_t elems)
{
	SSORT_CHECK(b, elems);
	s_msd_radix_sort_u32(b, elems);
}

void ssort_i64(int64_t *b, size_t elems)
{
	SSORT_CHECK(b, elems);
	s_msd_radix_sort_i64(b, elems);
}

void ssort_u64(uint64_t *b, size_t elems)
{
	SSORT_CHECK(b, elems);
	s_msd_radix_sort_u64(b, elems);
}

#endif /* #ifndef S_MINIMAL */

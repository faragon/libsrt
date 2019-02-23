/*
 * scommon.c
 *
 * Common stuff
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "scommon.h"

#define D8_LE_MASK S_NBITMASK(8 - D8_LE_SHIFT)
#define D16_LE_MASK S_NBITMASK(16 - D16_LE_SHIFT)
#define D24_LE_MASK S_NBITMASK(24 - D24_LE_SHIFT)
#define D32_LE_MASK S_NBITMASK(32 - D32_LE_SHIFT)
#define D40_LE_MASK S_NBITMASK64(40 - D40_LE_SHIFT)
#define D48_LE_MASK S_NBITMASK64(48 - D48_LE_SHIFT)
#define D56_LE_MASK S_NBITMASK64(56 - D56_LE_SHIFT)

enum SPK64IDMASK {
	D8_LE_ID = 1,
	D16_LE_ID = 2,
	D24_LE_ID = 4,
	D32_LE_ID = 8,
	D40_LE_ID = 16,
	D48_LE_ID = 32,
	D56_LE_ID = 64,
	D72_LE_ID = 128
};
enum SPK64SIZE {
	D8_LE_SZ = 1,
	D16_LE_SZ = 2,
	D24_LE_SZ = 3,
	D32_LE_SZ = 4,
	D40_LE_SZ = 5,
	D48_LE_SZ = 6,
	D56_LE_SZ = 7,
	D72_LE_SZ = 9
};
enum SPK64SHIFT {
	D8_LE_SHIFT = 1,
	D16_LE_SHIFT,
	D24_LE_SHIFT,
	D32_LE_SHIFT,
	D40_LE_SHIFT,
	D48_LE_SHIFT,
	D56_LE_SHIFT
};

void s_st_pk_u64(uint8_t **buf0, uint64_t v)
{
	uint8_t *buf;
	if (!buf0 || !*buf0)
		return;
	buf = *buf0;
	/* 7-bit (8-bit container) */
	if (v <= D8_LE_MASK) {
		buf[0] = D8_LE_ID | (uint8_t)(v << D8_LE_SHIFT);
		(*buf0) += D8_LE_SZ;
		return;
	}
	/* 14-bit (16-bit container) */
	if (v <= D16_LE_MASK) {
		S_ST_LE_U16(buf, D16_LE_ID | (uint16_t)(v << D16_LE_SHIFT));
		(*buf0) += D16_LE_SZ;
		return;
	}
	/* 21-bit (24-bit container) */
	if (v <= D24_LE_MASK) {
		S_ST_LE_U32(buf, D24_LE_ID | (uint32_t)(v << D24_LE_SHIFT));
		(*buf0) += D24_LE_SZ;
		return;
	}
	/* 28-bit (32-bit container) */
	if (v <= D32_LE_MASK) {
		S_ST_LE_U32(buf, D32_LE_ID | ((uint32_t)v << D32_LE_SHIFT));
		(*buf0) += D32_LE_SZ;
		return;
	}
	/* 35-bit (40-bit container) */
	if (v <= D40_LE_MASK) {
		S_ST_LE_U64(buf, D40_LE_ID | (v << D40_LE_SHIFT));
		(*buf0) += D40_LE_SZ;
		return;
	}
	/* 42-bit (48-bit container) */
	if (v <= D48_LE_MASK) {
		S_ST_LE_U64(buf, D48_LE_ID | (v << D48_LE_SHIFT));
		(*buf0) += D48_LE_SZ;
		return;
	}
	/* 49-bit (56-bit container) */
	if (v <= D56_LE_MASK) {
		S_ST_LE_U64(buf, D56_LE_ID | (v << D56_LE_SHIFT));
		(*buf0) += D56_LE_SZ;
		return;
	}
	/* 64-bit (72-bit container) */
	buf[0] = D72_LE_ID;
	S_ST_LE_U64(buf + 1, v);
	(*buf0) += D72_LE_SZ;
}

uint64_t s_ld_pk_u64(const uint8_t **buf0, size_t bs)
{
	size_t hdr_bytes;
	const uint8_t *buf;
	RETURN_IF(!buf0 || !bs, 0);
	buf = *buf0;
	RETURN_IF(!buf, 0);
	hdr_bytes = s_pk_u64_size(buf);
	(*buf0) += hdr_bytes;
	RETURN_IF(hdr_bytes > bs, 0);
	switch (hdr_bytes) {
	case D8_LE_SZ:
		return buf[0] >> D8_LE_SHIFT;
	case D16_LE_SZ:
		return S_LD_LE_U16(buf) >> D16_LE_SHIFT;
	case D24_LE_SZ:
		return D24_LE_MASK & (S_LD_LE_U32(buf) >> D24_LE_SHIFT);
	case D32_LE_SZ:
		return S_LD_LE_U32(buf) >> D32_LE_SHIFT;
	case D40_LE_SZ:
		return D40_LE_MASK & (S_LD_LE_U64(buf) >> D40_LE_SHIFT);
	case D48_LE_SZ:
		return D48_LE_MASK & (S_LD_LE_U64(buf) >> D48_LE_SHIFT);
	case D56_LE_SZ:
		return D56_LE_MASK & (S_LD_LE_U64(buf) >> D56_LE_SHIFT);
	case D72_LE_SZ:
		return S_LD_LE_U64(buf + 1);
	}
	return 0;
}

/* clang-format off */
size_t s_pk_u64_size(const uint8_t *buf)
{
	int h = *buf;
	return h & D8_LE_ID ? D8_LE_SZ : h & D16_LE_ID ? D16_LE_SZ :
	       h & D24_LE_ID ? D24_LE_SZ : h & D32_LE_ID ? D32_LE_SZ :
	       h & D40_LE_ID ? D40_LE_SZ : h & D48_LE_ID ? D48_LE_SZ :
	       h & D56_LE_ID ? D56_LE_SZ : h & D72_LE_ID ? D72_LE_SZ : 0;
}
/* clang-format on */

/*
 * Integer log2(N) approximation
 */
#define SLOG2STEP(mask, bits)                                                  \
	test = !!(i & mask);                                                   \
	o |= test * bits;                                                      \
	i = test * (i >> bits) | !test * i;

unsigned slog2_32(uint32_t i)
{
	unsigned o = 0, test;
	SLOG2STEP(0xffff0000, 16);
	SLOG2STEP(0xff00, 8);
	SLOG2STEP(0xf0, 4);
	SLOG2STEP(0x0c, 2);
	SLOG2STEP(0x02, 1);
	return o;
}

unsigned slog2_64(uint64_t i)
{
	unsigned o = 0, test;
	SLOG2STEP(0xffffffff00000000LL, 32);
	SLOG2STEP(0xffff0000, 16);
	SLOG2STEP(0xff00, 8);
	SLOG2STEP(0xf0, 4);
	SLOG2STEP(0x0c, 2);
	SLOG2STEP(0x02, 1);
	return o;
}

#define SLOG2_CEIL(lf, i)                                                      \
	unsigned a, b;                                                         \
	RETURN_IF(!i, 0);                                                      \
	a = lf(i - 1);                                                         \
	b = lf(i);                                                             \
	return a == b ? b + 1 : b

/* log2, rounding-up */
unsigned slog2_ceil_32(uint32_t i)
{
	SLOG2_CEIL(slog2_32, i);
}

unsigned slog2_ceil(uint64_t i)
{
	SLOG2_CEIL(slog2_64, i);
}

uint64_t snextpow2(uint64_t i)
{
#if 1
	i--;
	i |= i >> 1;
	i |= i >> 2;
	i |= i >> 4;
	i |= i >> 8;
	i |= i >> 16;
	i |= i >> 32;
	return i + 1;
#else
	return i <= 2 ? i : (uint64_t)1 << slog2_ceil_64(i);
#endif
}

/*
 * Custom "memset" functions
 */

void s_memset64(void *o0, const void *s0, size_t n8)
{
	uint8_t *o;
	const uint8_t *s;
	size_t i, head, tail;
	uint64_t data, d[2], *o64;
	if (!o0 || !s0 || !n8)
		return;
	head = (intptr_t)o0 & 7;
	tail = 8 - head;
	o = (uint8_t *)o0;
	s = (const uint8_t *)s0;
	if (head) {
		n8--;
		memcpy(o, s, tail);
		o += tail;
		memcpy(&d, s0, sizeof(data));
		d[1] = d[0];
		memcpy(&data, (uint8_t *)&d + tail, sizeof(data));
	} else {
		memcpy(&data, s, sizeof(data));
	}
	o64 = (uint64_t *)o;
	for (i = 0; i < n8; i++, o64++)
		memcpy(o64, &data, sizeof(data));
	if (head)
		memcpy(o64, s + tail, head);
}

void s_memset32(void *o, const void *s, size_t n4)
{
	size_t n8;
	uint64_t s64;
	if (!o || !s || !n4)
		return;
	n8 = n4 / 2;
	memcpy(&s64, s, 4);
	memcpy((uint8_t *)&s64 + 4, s, 4);
	s_memset64(o, &s64, n8);
	if (n4 % 2)
		memcpy((uint8_t *)o + n8 * 8, s, 4);
}

void s_memset24(void *o0, const void *s0, size_t n3)
{
	size_t n = n3 * 3;
	uint8_t *o = (uint8_t *)o0;
	const uint8_t *s = (const uint8_t *)s0;
	size_t k = 0, i, j, copy_size, c12;
	uint32_t *o32;
	union s_u32 d[3];
	if (n >= 15) {
		size_t ua_head = (intptr_t)o & 3;
		if (ua_head) {
			memcpy(o + k, s, 3);
			k = 4 - ua_head;
		}
		o32 = (uint32_t *)(o + k);
		copy_size = n - k;
		c12 = (copy_size / 12);
		for (i = 0; i < 3; i++)
			for (j = 0; j < 4; j++)
				d[i].b[j] = s[(j + k + i * 4) % 3];
		for (i = 0; i < c12; i++) {
			*o32++ = d[0].a32;
			*o32++ = d[1].a32;
			*o32++ = d[2].a32;
		}
		k = c12 * 12;
	}
	for (; k < n; k += 3)
		memcpy(o + k, s, 3);
}

void s_memset16(void *o, const void *s, size_t n2)
{
	size_t n4;
	uint32_t s32;
	if (!o || !s || !n2)
		return;
	memcpy(&s32, s, 2);
	memcpy((uint8_t *)&s32 + 2, s, 2);
	n4 = n2 / 2;
	s_memset32(o, &s32, n4);
	if (n2 % 2)
		memcpy((uint8_t *)o + n4 * 4, s, 2);
}

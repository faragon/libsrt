/*
 * scommon.c
 *
 * Common stuff
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "scommon.h"

/*
 * Integer log2(N) approximation
 */

unsigned slog2(uint64_t i)
{
	unsigned o = 0, test;
	#define SLOG2STEP(mask, bits)				\
			test = !!(i & mask);			\
			o |= test * bits;			\
			i = test * (i >> bits) | !test * i;
	SLOG2STEP(0xffffffff00000000LL, 32);
	SLOG2STEP(0xffff0000, 16);
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

void s_memset32(void *o, uint32_t data, size_t n)
{
	size_t k = 0, n4 = n / 4;
	uint32_t *o32;
#if defined(S_UNALIGNED_MEMORY_ACCESS) || S_BPWORD == 4
	size_t ua_head = (intptr_t)o & 3;
	if (ua_head && n4) {
		S_ST_U32(o, data);
		S_ST_U32((char *)o + n - 4, data);
		o32 = s_mar_u32((char *)o + 4 - ua_head);
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
		o32 = (uint32_t *)o;
		for (; k < n4; k++)
			S_ST_U32(o32 + k, data);
	}
}

void s_memset24(unsigned char *o, const unsigned char *data, size_t n)
{
	size_t k = 0;
	if (n >= 15) {
		size_t ua_head = (intptr_t)o & 3;
		if (ua_head) {
			memcpy(o + k, data, 3);
			k = 4 - ua_head;
		}
		uint32_t *o32 = s_mar_u32(o + k);
		union s_u32 d[3];
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


/*
 * sbitset.c
 *
 * Bit set handling
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "sbitset.h"

srt_bitset *sb_alloc_aux(srt_bitset *b)
{
	sb_clear(b);
	return b;
}

void sb_clear(srt_bitset *b)
{
	size_t nb;
	uint8_t *buf;
	srt_vector *v = (srt_vector *)b;
	if (v) {
		nb = sv_max_size(v);
		buf = (uint8_t *)sv_get_buffer(v);
		memset(buf, 0, nb);
		sv_set_size(v, nb);
		v->vx.cnt = 0;
	}
}

int sb_test_(const srt_bitset *b, size_t nth)
{
	size_t pos, mask;
	const uint8_t *buf;
	pos = nth / 8;
	mask = (size_t)1 << (nth % 8);
	buf = (const uint8_t *)sv_get_buffer_r(b);
	return (buf[pos] & mask) ? 1 : 0;
}

void sb_set_(srt_bitset *b, size_t nth)
{
	uint8_t *buf;
	size_t pos = nth / 8, mask = (size_t)1 << (nth % 8), pinc = pos + 1;
	if (pinc <= sv_size(b)) { /* BEHAVIOR: ignore out of range set */
		buf = (uint8_t *)sv_get_buffer(b);
		if ((buf[pos] & mask) == 0) {
			buf[pos] |= mask;
			b->vx.cnt++;
		}
	}
}

void sb_reset_(srt_bitset *b, size_t nth, size_t pos)
{
	uint8_t *buf = (uint8_t *)sv_get_buffer((srt_vector *)b);
	size_t mask = (size_t)1 << (nth % 8);
	if ((buf[pos] & mask) != 0) {
		buf[pos] &= ~mask;
		b->vx.cnt--;
	}
}

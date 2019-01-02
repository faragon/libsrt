/*
 * shset.c
 *
 * Hash set handling
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "shset.h"

#define SHS_ITP_X(t, hs, f, begin, end)                                        \
	size_t cnt = 0, ms;                                                    \
	const uint8_t *d0, *db, *de;                                           \
	RETURN_IF(!hs || t != hs->d.sub_type, 0);                              \
	ms = shs_size(hs);                                                     \
	RETURN_IF(begin > ms || begin >= end, 0);                              \
	if (end > ms)                                                          \
		end = ms;                                                      \
	RETURN_IF(!f, end - begin);                                            \
	d0 = shm_get_buffer_r(hs);                                             \
	db = d0 + hs->d.elem_size * begin;                                     \
	de = d0 + hs->d.elem_size * end;                                       \
	for (; db < de; db += hs->d.elem_size, cnt++)

size_t shs_itp_i32(const srt_hset *hs, size_t begin, size_t end,
		   srt_hset_it_i32 f, void *context)
{
	SHS_ITP_X(SHS_I32, hs, f, begin, end)
	{
		if (!f(((const struct SHMapi *)db)->k, context))
			break;
	}
	return cnt;
}

size_t shs_itp_u32(const srt_hset *hs, size_t begin, size_t end,
		   srt_hset_it_u32 f, void *context)
{
	SHS_ITP_X(SHS_U32, hs, f, begin, end)
	{
		if (!f(((const struct SHMapu *)db)->k, context))
			break;
	}
	return cnt;
}

size_t shs_itp_i(const srt_hset *hs, size_t begin, size_t end, srt_hset_it_i f,
		 void *context)
{
	SHS_ITP_X(SHS_I, hs, f, begin, end)
	{
		if (!f(((const struct SHMapI *)db)->k, context))
			break;
	}
	return cnt;
}

size_t shs_itp_s(const srt_hset *hs, size_t begin, size_t end, srt_hset_it_s f,
		 void *context)
{
	SHS_ITP_X(SHS_S, hs, f, begin, end)
	{
		if (!f(sso1_get(&((const struct SHMapS *)db)->k), context))
			break;
	}
	return cnt;
}

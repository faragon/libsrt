/*
 * shset.c
 *
 * Hash set handling
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "shset.h"

#define SHS_ITP_CNUM(TS) f(((const TS *)db)->k, context)
#define SHS_ITP_CS f(sso1_get(&((const struct SHMapS *)db)->k), context)
#define BUILD_SHS_ITP(FN, CBFT, ID, TS, COND)                                  \
	size_t FN(const srt_hset *hs, size_t begin, size_t end, CBFT f,        \
		  void *context)                                               \
	{                                                                      \
		size_t cnt = 0, ms;                                            \
		const uint8_t *d0, *db, *de;                                   \
		RETURN_IF(!hs || ID != hs->d.sub_type, 0);                     \
		ms = shs_size(hs);                                             \
		RETURN_IF(begin > ms || begin >= end, 0);                      \
		if (end > ms)                                                  \
			end = ms;                                              \
		RETURN_IF(!f, end - begin);                                    \
		d0 = shm_get_buffer_r(hs);                                     \
		db = d0 + hs->d.elem_size * begin;                             \
		de = d0 + hs->d.elem_size * end;                               \
		for (; db < de; db += hs->d.elem_size, cnt++) {                \
			if (!COND)                                             \
				break;                                         \
		}                                                              \
		return cnt;                                                    \
	}

BUILD_SHS_ITP(shs_itp_i32, srt_hset_it_i32, SHS_I32, struct SHMapi,
	      SHS_ITP_CNUM(struct SHMapi))
BUILD_SHS_ITP(shs_itp_u32, srt_hset_it_u32, SHS_U32, struct SHMapu,
	      SHS_ITP_CNUM(struct SHMapu))
BUILD_SHS_ITP(shs_itp_i, srt_hset_it_i, SHS_I, struct SHMapI,
	      SHS_ITP_CNUM(struct SHMapI))
BUILD_SHS_ITP(shs_itp_f, srt_hset_it_f, SHS_F, struct SHMapF,
	      SHS_ITP_CNUM(struct SHMapF))
BUILD_SHS_ITP(shs_itp_d, srt_hset_it_d, SHS_D, struct SHMapD,
	      SHS_ITP_CNUM(struct SHMapD))
BUILD_SHS_ITP(shs_itp_s, srt_hset_it_s, SHS_S, struct SHMapS, SHS_ITP_CS)

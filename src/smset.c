/*
 * smset.c
 *
 * Set handling.
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "smset.h"
#include "saux/scommon.h"

SM_ENUM_INORDER_XX(sms_itr_i32, srt_set_it_i32, SM0_I32, int32_t,
		   cmp_ni_i((const struct SMapi *)cn, kmin),
		   cmp_ni_i((const struct SMapi *)cn, kmax),
		   f(((const struct SMapi *)cn)->k, context))

SM_ENUM_INORDER_XX(sms_itr_u32, srt_set_it_u32, SM0_U32, uint32_t,
		   cmp_nu_u((const struct SMapu *)cn, kmin),
		   cmp_nu_u((const struct SMapu *)cn, kmax),
		   f(((const struct SMapu *)cn)->k, context))

SM_ENUM_INORDER_XX(sms_itr_i, srt_set_it_i, SM0_I, int64_t,
		   cmp_nI_I((const struct SMapI *)cn, kmin),
		   cmp_nI_I((const struct SMapI *)cn, kmax),
		   f(((const struct SMapI *)cn)->k, context))

SM_ENUM_INORDER_XX(
	sms_itr_s, srt_set_it_s, SM0_S, const srt_string *,
	cmp_ns_s((const struct SMapS *)cn, kmin),
	cmp_ns_s((const struct SMapS *)cn, kmax),
	f(sso_get((const srt_stringo *)&((const struct SMapS *)cn)->k),
	  context))

SM_ENUM_INORDER_XX(sms_itr_f, srt_set_it_f, SM0_F, float,
		   cmp_nF_F((const struct SMapF *)cn, kmin),
		   cmp_nF_F((const struct SMapF *)cn, kmax),
		   f(((const struct SMapF *)cn)->k, context))

SM_ENUM_INORDER_XX(sms_itr_d, srt_set_it_d, SM0_D, double,
		   cmp_nD_D((const struct SMapD *)cn, kmin),
		   cmp_nD_D((const struct SMapD *)cn, kmax),
		   f(((const struct SMapD *)cn)->k, context))

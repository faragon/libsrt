/*
 * smset.c
 *
 * Set handling.
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */ 

#include "smset.h"
#include "saux/scommon.h"

#ifdef _MSC_VER /* supress alloca() warning */
#pragma warning(disable: 6255)
#endif

SM_ENUM_INORDER_XX(sms_itr_i32, sms_it_i32_t, SM0_I32, int32_t,
		   cmp_ni_i((const struct SMapi *)cn, kmin),
		   cmp_ni_i((const struct SMapi *)cn, kmax),
		   f(((const struct SMapi *)cn)->k, context))

SM_ENUM_INORDER_XX(sms_itr_u32, sms_it_u32_t, SM0_U32, uint32_t,
		   cmp_nu_u((const struct SMapu *)cn, kmin),
		   cmp_nu_u((const struct SMapu *)cn, kmax),
		   f(((const struct SMapu *)cn)->k, context))

SM_ENUM_INORDER_XX(sms_itr_i, sms_it_i_t, SM0_I, int64_t,
		   cmp_nI_I((const struct SMapI *)cn, kmin),
		   cmp_nI_I((const struct SMapI *)cn, kmax),
		   f(((const struct SMapI *)cn)->k, context))

SM_ENUM_INORDER_XX(sms_itr_s, sms_it_s_t, SM0_S, const ss_t *,
		   cmp_ns_s((const struct SMapS *)cn, kmin),
		   cmp_ns_s((const struct SMapS *)cn, kmax),
		   f(SMStrGet(&((const struct SMapS *)cn)->k), context))


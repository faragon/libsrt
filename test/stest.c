/*
 * stest.c
 *
 * libsrt API tests
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "../src/libsrt.h"
#include "../src/saux/schar.h"
#include "../src/saux/sdbg.h"
#include "utf8_examples.h"
#include <locale.h>

#if !defined(_MSC_VER) && !defined(__CYGWIN__) && !defined(S_MINIMAL)
#define GOOD_LOCALE_SUPPORT
#endif

/*
 * Unit testing helpers
 */

#define STEST_START int ss_errors = 0, atmp = 0, r
#define STEST_END ss_errors
#define STEST_ASSERT_BASE(a, pre_op)                                           \
	{                                                                      \
		pre_op;                                                        \
		r = (int)(a);                                                  \
		atmp += r;                                                     \
		if (r) {                                                       \
			fprintf(stderr, "%s: %08X <--------\n", #a, r);        \
			ss_errors++;                                           \
		}                                                              \
	}
#ifdef S_DEBUG
#define STEST_ASSERT(a) STEST_ASSERT_BASE(a, fprintf(stderr, "%s...\n", #a))
#else
#define STEST_ASSERT(a) STEST_ASSERT_BASE(a, ;)
#endif

#define STEST_FILE "test.dat"
#define S_MAX_I64 9223372036854775807LL
#define S_MIN_I64 (0 - S_MAX_I64 - 1)

/* Integer compare functions */

#define BUILD_SINT_CMPF(fn, T)                                                 \
	S_INLINE int fn(T a, T b)                                              \
	{                                                                      \
		return a > b ? 1 : a < b ? -1 : 0;                             \
	}

BUILD_SINT_CMPF(scmp_int32, int32_t)
BUILD_SINT_CMPF(scmp_uint32, uint32_t)
BUILD_SINT_CMPF(scmp_i, int64_t)
BUILD_SINT_CMPF(scmp_f, float)
BUILD_SINT_CMPF(scmp_d, double)
BUILD_SINT_CMPF(scmp_ptr, const void *)

/*
 * Test common data structures
 */

#define TEST_AA(s, t) (s.a == t.a && s.b == t.b)
#define TEST_NUM(s, t) (s == t)

struct AA {
	int a, b;
};
static struct AA a1 = {1, 2}, a2 = {3, 4};
static struct AA av1[3] = {{1, 2}, {3, 4}, {5, 6}};

static int8_t i8v[10] = {1, 2, 3, 4, 5, -5, -4, -3, -2, -1};
static uint8_t u8v[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static int16_t i16v[10] = {1, 2, 3, 4, 5, -5, -4, -3, -2, -1};
static uint16_t u16v[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static int32_t i32v[10] = {1, 2, 3, 4, 5, -5, -4, -3, -2, -1};
static uint32_t u32v[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static int64_t i64v[10] = {1, 2, 3, 4, 5, -5, -4, -3, -2, -1};
static uint64_t u64v[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static float fv[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static double dv[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static void *spv[10] = {(void *)0, (void *)1, (void *)2, (void *)3, (void *)4,
			(void *)5, (void *)6, (void *)7, (void *)8, (void *)9};
static int64_t sik[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static int32_t i32k[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static uint32_t u32k[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static float sfk[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static double sdk[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

#define NO_CMPF , NULL
#define X_CMPF
#define AA_CMPF , AA_cmp

static int AA_cmp(const void *a, const void *b)
{
	int64_t r = ((const struct AA *)a)->a - ((const struct AA *)b)->a;
	return r < 0 ? -1 : r > 0 ? 1 : 0;
}

/*
 * Tests generated from templates
 */

/*
 * Following test template covers two cases: non-aliased (sa to sb) and
 * aliased (sa to sa)
 */
#define MK_TEST_SS_DUP_CODEC(suffix)                                           \
	static int test_ss_dup_##suffix(const srt_string *a,                   \
					const srt_string *b)                   \
	{                                                                      \
		srt_string *sa = ss_dup(a), *sb = ss_dup_##suffix(sa),         \
			   *sc = ss_dup_##suffix(a);                           \
		int res = (!sa || !sb) ? 1                                     \
				       : (ss_##suffix(&sa, sa) ? 0 : 2)        \
						 | (!ss_cmp(sa, b) ? 0 : 4)    \
						 | (!ss_cmp(sb, b) ? 0 : 8)    \
						 | (!ss_cmp(sc, b) ? 0 : 16);  \
		ss_free(&sa);                                                  \
		ss_free(&sb);                                                  \
		ss_free(&sc);                                                  \
		return res;                                                    \
	}

#define MK_TEST_SS_CPY_CODEC(suffix)                                           \
	static int test_ss_cpy_##suffix(const srt_string *a,                   \
					const srt_string *b)                   \
	{                                                                      \
		int res;                                                       \
		srt_string *sa = ss_dup(a), *sb = ss_dup_c("garbage i!&/()="), \
			   *sc = ss_dup(sb);                                   \
		ss_cpy_##suffix(&sb, sa);                                      \
		ss_cpy_##suffix(&sc, a);                                       \
		res = (!sa || !sb) ? 1                                         \
				   : (ss_##suffix(&sa, sa) ? 0 : 2)            \
					     | (!ss_cmp(sa, b) ? 0 : 4)        \
					     | (!ss_cmp(sb, b) ? 0 : 8)        \
					     | (!ss_cmp(sc, b) ? 0 : 16);      \
		ss_free(&sa);                                                  \
		ss_free(&sb);                                                  \
		ss_free(&sc);                                                  \
		return res;                                                    \
	}

#define MK_TEST_SS_CAT_CODEC(suffix)                                           \
	static int test_ss_cat_##suffix(const srt_string *a,                   \
					const srt_string *b,                   \
					const srt_string *expected)            \
	{                                                                      \
		int res;                                                       \
		srt_string *sa = ss_dup(a), *sb = ss_dup(b), *sc = ss_dup(sa); \
		ss_cat_##suffix(&sa, sb);                                      \
		ss_cat_##suffix(&sc, b);                                       \
		res = (!sa || !sb)                                             \
			      ? 1                                              \
			      : (!ss_cmp(sa, expected) ? 0 : 2)                \
					| (!ss_cmp(sc, expected) ? 0 : 4);     \
		ss_free(&sa);                                                  \
		ss_free(&sb);                                                  \
		ss_free(&sc);                                                  \
		return res;                                                    \
	}

#define MK_TEST_SS_DUP_CPY_CAT_CODEC(codec)                                    \
	MK_TEST_SS_DUP_CODEC(codec)                                            \
	MK_TEST_SS_CPY_CODEC(codec)                                            \
	MK_TEST_SS_CAT_CODEC(codec)

MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_b64)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_hex)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_HEX)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_lz)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_lzh)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_xml)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_json)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_url)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_dquote)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_squote)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_b64)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_hex)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_lz)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_xml)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_json)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_url)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_dquote)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_squote)

#undef MK_TEST_SS_DUP_CPY_CAT

/*
 * Tests
 */

static int test_sb(size_t nelems)
{
	int res;
	srt_bitset *da, *db;
	srt_bitset *a = sb_alloca(nelems), *b = sb_alloc(nelems);
#define TEST_SB(bs, ne)                                                        \
	sb_set(&bs, 0);                                                        \
	sb_set(&bs, ne - 1);
	TEST_SB(a, nelems);
	TEST_SB(b, nelems);
#undef TEST_SB
	da = sb_dup(a);
	db = sb_dup(b);
	res = sb_popcount(a) != sb_popcount(b)
			      || sb_popcount(da) != sb_popcount(db)
			      || sb_popcount(a) != sb_popcount(da)
			      || sb_popcount(a) != 2
		      ? 1
		      : !sb_test(a, 0) || !sb_test(b, 0) || !sb_test(da, 0)
					|| !sb_test(db, 0)
				? 2
				: 0;
	/* clear 'a', 'b', 'da' all at once */
	sb_clear(a);
	sb_clear(b);
	sb_clear(da);
	/* clear 'db' one manually */
	sb_reset(&db, 0);
	sb_reset(&db, nelems - 1);
	res |= sb_popcount(a) || sb_popcount(b) || sb_popcount(da)
			       || sb_popcount(db)
		       ? 4
		       : sb_test(da, 0) || sb_test(db, 0) ? 8 : 0;
	res |= sb_capacity(da) < nelems || sb_capacity(db) < nelems ? 16 : 0;
#ifdef S_USE_VA_ARGS
	sb_free(&b, &da, &db);
#else
	sb_free(&b);
	sb_free(&da);
	sb_free(&db);
#endif
	return res;
}

static int test_ss_alloc(size_t max_size)
{
	srt_string *a = ss_alloc(max_size);
	int res = !a ? 1
		     : (ss_len(a) != 0) * 2 | (ss_capacity(a) != max_size) * 4;
	ss_free(&a);
	return res;
}

static int test_ss_alloca(size_t max_size)
{
	srt_string *a = ss_alloca(max_size);
	int res = !a ? 1
		     : (ss_len(a) != 0) * 2 | (ss_capacity(a) != max_size) * 4;
	ss_free(&a);
	return res;
}

static int test_ss_shrink()
{
	srt_string *a = ss_dup_printf(1024, "hello");
	int res = !a ? 1
		     : (ss_shrink(&a) ? 0 : 2)
				  | (ss_capacity(a) == ss_len(a) ? 0 : 4);
	ss_free(&a);
	return res;
}

static int test_ss_grow(size_t init_size, size_t extra_size)
{
	srt_string *a = ss_alloc(init_size);
	int res = a ? 0 : 1;
	res |= res ? 0 : (ss_grow(&a, extra_size) > 0 ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_reserve(size_t max_size)
{
	srt_string *a = ss_alloc(max_size / 2);
	int res = a ? 0 : 1;
	res |= res ? 0 : (ss_reserve(&a, max_size) >= max_size ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_resize_x()
{
	const char *i0 = U8_C_N_TILDE_D1 "1234567890";
	const char *i1 = U8_C_N_TILDE_D1 "1234567890__";
	const char *i2 = U8_C_N_TILDE_D1 "12";
	const char *i3 = U8_C_N_TILDE_D1 "123";
	srt_string *a = ss_dup_c(i0), *b = ss_dup(a);
	int res = a && b ? 0 : 1;
	res |= res ? 0
		   : ((ss_resize(&a, 14, '_') && !strcmp(ss_to_c(a), i1)) ? 0
									  : 2)
			       | ((ss_resize_u(&b, 13, '_')
				   && !strcmp(ss_to_c(b), i1))
					  ? 0
					  : 4)
			       | ((ss_resize(&a, 4, '_')
				   && !strcmp(ss_to_c(a), i2))
					  ? 0
					  : 8)
			       | ((ss_resize_u(&b, 4, '_')
				   && !strcmp(ss_to_c(b), i3))
					  ? 0
					  : 16);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b);
#else
	ss_free(&a);
	ss_free(&b);
#endif
	return res;
}

/*
 * String operation tests are repeated 4 times, checking all Unicode size
 * cached input combinations:
 * - non cached input/output, non-cached src
 * - non cached input/output, cached src
 * - cached input/output, non-cached src
 * - cached input/output, cached src
 */
#define TEST_SS_OPCHK_VARS                                                     \
	int res = 0;                                                           \
	int i = 0;                                                             \
	srt_string *sa = NULL, *sb = NULL, *se
#define TEST_SS_OPCHK(f, a, b, e)                                              \
	se = ss_dup_c(e);                                                      \
	for (i = 0; i < 4 && !res; i++) {                                      \
		ss_free(&sa);                                                  \
		ss_free(&sb);                                                  \
		sa = ss_dup_c(a);                                              \
		sb = ss_dup_c(b);                                              \
		if ((i & 1) != 0)                                              \
			(void)ss_len_u(sa);                                    \
		if ((i & 2) != 0)                                              \
			(void)ss_len_u(sb);                                    \
		f;                                                             \
		res = (!sa || !sb)                                             \
			      ? 1                                              \
			      : (!strcmp(ss_to_c(sa), e) ? 0 : (i << 4 | 2))   \
					| (!ss_cmp(sa, se) ? 0 : (i << 4 | 4)) \
					| (ss_len_u(sa) == ss_len_u(se)        \
						   ? 0                         \
						   : (i << 4 | 8));            \
	}                                                                      \
	ss_free(&sa);                                                          \
	ss_free(&sb);                                                          \
	ss_free(&se);                                                          \
	return res

static int test_ss_trim(const char *in, const char *expected)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_trim(&sa), in, "", expected);
}

static int test_ss_ltrim(const char *in, const char *expected)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_ltrim(&sa), in, "", expected);
}

static int test_ss_rtrim(const char *in, const char *expected)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_rtrim(&sa), in, "", expected);
}

static int test_ss_erase(const char *in, size_t byte_off, size_t num_bytes,
			 const char *expected)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_erase(&sa, byte_off, num_bytes), in, "", expected);
}

static int test_ss_erase_u(const char *in, size_t char_off, size_t num_chars,
			   const char *expected)
{
	srt_string *a = ss_dup_c(in);
	int res = a ? 0 : 1;
	size_t in_wlen = ss_len_u(a),
	       expected_len = char_off >= in_wlen
				      ? in_wlen
				      : (in_wlen - char_off) > num_chars
						? in_wlen - num_chars
						: char_off;
	res |= res ? 0
		   : (ss_erase_u(&a, char_off, num_chars) ? 0 : 2)
			       | (!strcmp(ss_to_c(a), expected) ? 0 : 4)
			       | (expected_len == ss_len_u(a) ? 0 : 8);
	ss_free(&a);
	return res;
}

static int test_ss_free(size_t max_size)
{
	srt_string *a = ss_alloc(max_size);
	int res = a ? 0 : 1;
	ss_free(&a);
	res |= (!a ? 0 : 2);
	return res;
}

static int test_ss_len(const char *a, size_t expected_size)
{
	srt_string *sa = ss_dup_c(a);
	const srt_string *sb = ss_crefa(a), *sc = ss_refa_buf(a, strlen(a));
	int res = !sa ? 1
		      : (ss_len(sa) == expected_size ? 0 : 1)
				  | (ss_len(sb) == expected_size ? 0 : 2)
				  | (ss_len(sc) == expected_size ? 0 : 4);
	ss_free(&sa);
	return res;
}

static int test_ss_len_u(const char *a, size_t expected_size)
{
	srt_string *sa = ss_dup_c(a);
	const srt_string *sb = ss_crefa(a), *sc = ss_refa_buf(a, strlen(a));
	int res = !sa ? 1
		      : (ss_len_u(sa) == expected_size ? 0 : 1)
				  | (ss_len_u(sb) == expected_size ? 0 : 2)
				  | (ss_len_u(sc) == expected_size ? 0 : 4);
	ss_free(&sa);
	return res;
}

/* clang-format off */
static int test_ss_capacity()
{
	size_t test_max_size = 100;
	srt_string *a = ss_alloc(test_max_size);
	const srt_string *b = ss_crefa("123");
	int res = !a ? 1 :
		  !ss_empty(a) ? 2 :
		  ss_capacity_left(a) != test_max_size ? 3 :
		  !ss_cpy_c(&a, "a") ? 4 :
		  ss_empty(a) ? 5 :
		  ss_get_buffer(a) != ss_get_buffer_r(a) ? 6 :
		  !ss_get_buffer_r(a) ? 7 :
		  ss_get_buffer_size(a) != 1 ? 8 :
		  *(const char *)ss_get_buffer_r(a) != 'a' ? 9 :
		  ss_capacity_left(a) != test_max_size - 1 ? 10 :
		  ss_capacity(a) != test_max_size ? 11 :
		  ss_capacity(b) != 3 ? 12 :
		  ss_capacity_left(b) != 0 ? 13 : 0;
	ss_free(&a);
	return res;
}
/* clang-format on */

static int test_ss_len_left()
{
	int res;
	srt_string *a = ss_alloc(10);
	ss_cpy_c(&a, "a");
	res = !a ? 1 : (ss_capacity(a) - ss_len(a) == 9 ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_max()
{
	srt_string *a = ss_alloca(10), *b = ss_alloc(10);
	const srt_string *c = ss_crefa("hello");
	int res = (a && b) ? 0 : 1;
	res |= res ? 0 : (ss_max(a) == 10 ? 0 : 1);
	res |= res ? 0 : (ss_max(b) == SS_RANGE ? 0 : 2);
	res |= res ? 0 : (ss_max(c) == 5 ? 0 : 4);
	ss_free(&b);
	return res;
}

static int test_ss_dup()
{
	srt_string *b = ss_dup_c("hello"), *a = ss_dup(b);
	const srt_string *c = ss_crefa("hello");
	int res = !a ? 1
		     : (!strcmp("hello", ss_to_c(a)) ? 0 : 2)
				  | (!strcmp("hello", ss_to_c(c)) ? 0 : 4);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b);
#else
	ss_free(&a);
	ss_free(&b);
#endif
	return res;
}

static int test_ss_dup_substr()
{
	const srt_string *crefa_hello = ss_crefa("hello");
	srt_string *a = ss_dup_c("hello"), *b = ss_dup_substr(a, 0, 5),
		   *c = ss_dup_substr(crefa_hello, 0, 5);
	int res = (!a || !b)
			  ? 1
			  : (ss_len(a) == ss_len(b) ? 0 : 2)
				    | (!strcmp("hello", ss_to_c(a)) ? 0 : 4)
				    | (ss_len(a) == ss_len(c) ? 0 : 8)
				    | (!strcmp("hello", ss_to_c(c)) ? 0 : 16);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
#endif
	return res;
}

static int test_ss_dup_substr_u()
{
	const srt_string *crefa_hello = ss_crefa("hello");
	srt_string *b = ss_dup_c("hello"), *a = ss_dup_substr(b, 0, 5),
		   *c = ss_dup_substr(crefa_hello, 0, 5);
	int res = !a ? 1
		     : (ss_len(a) == ss_len(b) ? 0 : 2)
				  | (!strcmp("hello", ss_to_c(a)) ? 0 : 4)
				  | (ss_len(a) == ss_len(c) ? 0 : 8)
				  | (!strcmp("hello", ss_to_c(c)) ? 0 : 16);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
#endif
	return res;
}

static int test_ss_dup_cn()
{
	srt_string *a = ss_dup_cn("hello123", 5);
	int res = !a ? 1 : (!strcmp("hello", ss_to_c(a)) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_dup_c()
{
	srt_string *a = ss_dup_c("hello");
	const srt_string *b = ss_crefa("hello");
	int res = !a ? 1
		     : (!strcmp("hello", ss_to_c(a)) ? 0 : 2)
				  | (!strcmp("hello", ss_to_c(b)) ? 0 : 4);
	ss_free(&a);
	return res;
}

static int test_ss_dup_wn()
{
	srt_string *a = ss_dup_wn(L"hello123", 5);
	int res = !a ? 1 : (!strcmp("hello", ss_to_c(a)) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_dup_w()
{
	srt_string *a = ss_dup_w(L"hello");
	int res = !a ? 1 : (!strcmp("hello", ss_to_c(a)) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_dup_int(int64_t num, const char *expected)
{
	srt_string *a = ss_dup_int(num);
	int res = !a ? 1 : (!strcmp(ss_to_c(a), expected) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_dup_tolower(const srt_string *a, const srt_string *b)
{
	srt_string *sa = ss_dup(a), *sb = ss_dup_tolower(sa),
		   *sc = ss_dup_tolower(a);
	int res = (!sa || !sb || !sc)
			  ? 1
			  : (ss_tolower(&sa) ? 0 : 2) | (!ss_cmp(sa, b) ? 0 : 4)
				    | (!ss_cmp(sb, b) ? 0 : 8)
				    | (!ss_cmp(sc, b) ? 0 : 16)
				    | (!ss_cmp(sb, b) ? 0 : 32)
				    | (!ss_cmp(sc, b) ? 0 : 64);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb, &sc);
#else
	ss_free(&sa);
	ss_free(&sb);
	ss_free(&sc);
#endif
	return res;
}

static int test_ss_dup_toupper(const srt_string *a, const srt_string *b)
{
	srt_string *sa = ss_dup(a), *sb = ss_dup_toupper(sa);
	int res = (!sa || !sb)
			  ? 1
			  : (ss_toupper(&sa) ? 0 : 2) | (!ss_cmp(sa, b) ? 0 : 4)
				    | (!ss_cmp(sb, b) ? 0 : 8)
				    | (!ss_cmp(sb, b) ? 0 : 16);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_dup_erase(const char *in, size_t off, size_t size,
			     const char *expected)
{
	srt_string *a = ss_dup_c(in), *b = ss_dup_erase(a, off, size);
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), expected) ? 0 : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b);
#else
	ss_free(&a);
	ss_free(&b);
#endif
	return res;
}

static int test_ss_dup_erase_u()
{
	srt_string *a = ss_dup_c("hel" U8_S_N_TILDE_F1 "lo"),
		   *b = ss_dup_erase_u(a, 2, 3);
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), "heo") ? 0 : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b);
#else
	ss_free(&a);
	ss_free(&b);
#endif
	return res;
}

static int test_ss_dup_replace(const char *in, const char *r, const char *s,
			       const char *expected)
{
	srt_string *a = ss_dup_c(in), *b = ss_dup_c(r), *c = ss_dup_c(s),
		   *d = ss_dup_replace(a, 0, b, c),
		   *e = ss_dup_replace(a, 0, a, a);
	int res = (!a || !b) ? 1
			     : (!strcmp(ss_to_c(d), expected) ? 0 : 2)
				       | (!strcmp(ss_to_c(e), in)
						  ? 0
						  : 4); /* aliasing */
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c, &d, &e);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
	ss_free(&d);
	ss_free(&e);
#endif
	return res;
}

static int test_ss_dup_resize()
{
	srt_string *a = ss_dup_c("hello"), *b = ss_dup_resize(a, 10, 'z'),
		   *c = ss_dup_resize(a, 2, 'z');
	int res = (!a || !b) ? 1
			     : (!strcmp(ss_to_c(b), "hellozzzzz") ? 0 : 2)
				       | (!strcmp(ss_to_c(c), "he") ? 0 : 4);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
#endif
	return res;
}

static int test_ss_dup_resize_u()
{
	srt_string *a = ss_dup_c(U8_S_N_TILDE_F1 "hello"),
		   *b = ss_dup_resize_u(a, 11, 'z'),
		   *c = ss_dup_resize_u(a, 3, 'z');
	int res = (!a || !b)
			  ? 1
			  : (!strcmp(ss_to_c(b), U8_S_N_TILDE_F1 "hellozzzzz")
				     ? 0
				     : 2)
				    | (!strcmp(ss_to_c(c), U8_S_N_TILDE_F1 "he")
					       ? 0
					       : 4);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
#endif
	return res;
}

static int test_ss_dup_trim(const char *a, const char *expected)
{
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_trim(sa);
	const srt_string *crefa_a = ss_crefa(a);
	srt_string *sc = ss_dup_trim(crefa_a);
	int res = (!sa || !sb) ? 1
			       : (!strcmp(ss_to_c(sb), expected) ? 0 : 2)
					 | (!ss_cmp(sb, sc) ? 0 : 4);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb, &sc);
#else
	ss_free(&sa);
	ss_free(&sb);
	ss_free(&sc);
#endif
	return res;
}

static int test_ss_dup_ltrim(const char *a, const char *expected)
{
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_ltrim(sa);
	const srt_string *crefa_a = ss_crefa(a);
	srt_string *sc = ss_dup_ltrim(crefa_a);
	int res = (!sa || !sb) ? 1
			       : (!strcmp(ss_to_c(sb), expected) ? 0 : 2)
					 | (!ss_cmp(sb, sc) ? 0 : 4);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb, &sc);
#else
	ss_free(&sa);
	ss_free(&sb);
	ss_free(&sc);
#endif
	return res;
}

static int test_ss_dup_rtrim(const char *a, const char *expected)
{
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_rtrim(sa);
	const srt_string *crefa_a = ss_crefa(a);
	srt_string *sc = ss_dup_rtrim(crefa_a);
	int res = (!sa || !sb) ? 1
			       : (!strcmp(ss_to_c(sb), expected) ? 0 : 2)
					 | (!ss_cmp(sb, sc) ? 0 : 4);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb, &sc);
#else
	ss_free(&sa);
	ss_free(&sb);
	ss_free(&sc);
#endif
	return res;
}

static int test_ss_dup_printf()
{
	int res;
	char btmp[512];
	srt_string *a = ss_dup_printf(512, "abc%i%s%08X", 1, "hello", -1);
	sprintf(btmp, "abc%i%s%08X", 1, "hello", -1);
	res = !strcmp(ss_to_c(a), btmp) ? 0 : 1;
	ss_free(&a);
	return res;
}

static int test_ss_dup_printf_va(const char *expected, size_t max_size,
				 const char *fmt, ...)
{
	int res;
	srt_string *a;
	va_list ap;
	va_start(ap, fmt);
	a = ss_dup_printf_va(max_size, fmt, ap);
	va_end(ap);
	res = !strcmp(ss_to_c(a), expected) ? 0 : 1;
	ss_free(&a);
	return res;
}

static int test_ss_dup_char(int32_t in, const char *expected)
{
	srt_string *a = ss_dup_char(in);
	int res = !a ? 1 : !strcmp(ss_to_c(a), expected) ? 0 : 2;
	ss_free(&a);
	return res;
}

static int test_ss_dup_read(const char *pattern)
{
	int res = 1;
	size_t pattern_size = strlen(pattern);
	srt_string *s = NULL;
	FILE *f = fopen(STEST_FILE, S_FOPEN_BINARY_RW_TRUNC);
	if (f) {
		size_t write_size = fwrite(pattern, 1, pattern_size, f);
		res = !write_size || ferror(f) || write_size != pattern_size
			      ? 2
			      : fseek(f, 0, SEEK_SET) != 0
					? 4
					: (s = ss_dup_read(f, pattern_size))
							  == NULL
						  ? 8
						  : strcmp(ss_to_c(s), pattern)
							    ? 16
							    : 0;
		fclose(f);
		if (remove(STEST_FILE) != 0)
			res |= 32;
	}
	ss_free(&s);
	return res;
}

static int test_ss_cpy(const char *in)
{
	srt_string *a = ss_dup_c(in);
	int res = !a ? 1
		     : (!strcmp(ss_to_c(a), in) ? 0 : 2)
				  | (ss_len(a) == strlen(in) ? 0 : 4);
	srt_string *b = NULL;
	ss_cpy(&b, a);
	res = res ? 0
		  : (!strcmp(ss_to_c(b), in) ? 0 : 8)
			      | (ss_len(b) == strlen(in) ? 0 : 16);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b);
#else
	ss_free(&a);
	ss_free(&b);
#endif
	return res;
}

static int test_ss_cpy_substr()
{
	srt_string *b = NULL, *a = ss_dup_c("how are you"),
		   *c = ss_dup_c("how");
	int res = (!a || !c) ? 1 : 0;
	res |= res ? 0 : (ss_cpy_substr(&b, a, 0, 3) ? 0 : 2);
	res |= res ? 0 : (!ss_cmp(b, c) ? 0 : 4);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
#endif
	return res;
}

static int test_ss_cpy_substr_u()
{
	srt_string *b = NULL, *a = ss_dup_c("how are you"),
		   *c = ss_dup_c("how");
	int res = (!a || !c) ? 1 : 0;
	res |= res ? 0 : (ss_cpy_substr_u(&b, a, 0, 3) ? 0 : 2);
	res |= res ? 0 : (!ss_cmp(b, c) ? 0 : 4);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
#endif
	return res;
}

static int test_ss_cpy_cn()
{
	char b[3] = {0, 1, 2};
	srt_string *a = NULL;
	int res = !ss_cpy_cn(&a, b, 3)
			  ? 1
			  : (ss_to_c(a)[0] == b[0] ? 0 : 2)
				    | (ss_to_c(a)[1] == b[1] ? 0 : 4)
				    | (ss_to_c(a)[2] == b[2] ? 0 : 8)
				    | (ss_len(a) == 3 ? 0 : 16);
	ss_free(&a);
	return res;
}

static int test_ss_cpy_c(const char *in)
{
#ifdef S_USE_VA_ARGS
	srt_string *a = NULL;
	int res = !ss_cpy_c(&a, in)
			  ? 1
			  : (!strcmp(ss_to_c(a), in) ? 0 : 2)
				    | (ss_len(a) == strlen(in) ? 0 : 4);
	char tmp[512];
	sprintf(tmp, "%s%s", in, in);
	res |= res ? 0
		   : ss_cpy_c(&a, in, in) && !strcmp(ss_to_c(a), tmp) ? 0 : 8;
	ss_free(&a);
	return res;
#else
	(void)in;
	return 0;
#endif
}

static int test_ss_cpy_w(const wchar_t *in, const char *expected_utf8)
{
#ifdef S_USE_VA_ARGS
	srt_string *a = NULL;
	int res = !ss_cpy_w(&a, in)
			  ? 1
			  : (!strcmp(ss_to_c(a), expected_utf8) ? 0 : 2)
				    | (ss_len(a) == strlen(expected_utf8) ? 0
									  : 4);
	char tmp[512];
	sprintf(tmp, "%ls%ls", in, in);
	res |= res ? 0
		   : ss_cpy_w(&a, in, in) && !strcmp(ss_to_c(a), tmp) ? 0 : 8;
	ss_free(&a);
	return res;
#else
	(void)in;
	(void)expected_utf8;
	return 0;
#endif
}

static int test_ss_cpy_wn()
{
	int res = 0;
#ifndef S_WCHAR32
	const uint16_t t16[] = {0xd1,
				0xf1,
				0x131,
				0x130,
				0x11e,
				0x11f,
				0x15e,
				0x15f,
				0xa2,
				0x20ac,
				0xd800 | (uint16_t)(0x24b62 >> 10),
				0xdc00 | (uint16_t)(0x24b62 & 0x3ff),
				0};
	const wchar_t *t = (wchar_t *)t16;
#else
	const uint32_t t32[] = {0xd1,  0xf1,  0x131, 0x130,  0x11e,   0x11f,
				0x15e, 0x15f, 0xa2,  0x20ac, 0x24b62, 0};
	const wchar_t *t = (const wchar_t *)t32;
#endif
#define TU8A3 U8_C_N_TILDE_D1 U8_S_N_TILDE_F1 U8_S_I_DOTLESS_131
#define TU8B3 U8_C_I_DOTTED_130 U8_C_G_BREVE_11E U8_S_G_BREVE_11F
#define TU8C3 U8_C_S_CEDILLA_15E U8_S_S_CEDILLA_15F U8_CENT_00A2
#define TU8D2 U8_EURO_20AC U8_HAN_24B62
#define TU8ALL11 TU8A3 TU8B3 TU8C3 TU8D2
	srt_string *a = ss_dup_c("hello"), *b_a3 = ss_dup_c(TU8A3),
		   *b_b3 = ss_dup_c(TU8B3), *b_c3 = ss_dup_c(TU8C3),
		   *b_d2 = ss_dup_c(TU8D2), *b_all11 = ss_dup_c(TU8ALL11);
	ss_cpy_wn(&a, t, 3);
	res |= !ss_cmp(a, b_a3) ? 0 : 1;
	ss_cpy_wn(&a, t + 3, 3);
	res |= !ss_cmp(a, b_b3) ? 0 : 2;
	ss_cpy_wn(&a, t + 6, 3);
	res |= !ss_cmp(a, b_c3) ? 0 : 4;
	if (sizeof(wchar_t) == 2)
		ss_cpy_wn(&a, t + 9, 3);
	else
		ss_cpy_wn(&a, t + 9, 2);
	res |= !ss_cmp(a, b_d2) ? 0 : 8;
	if (sizeof(wchar_t) == 2)
		ss_cpy_wn(&a, t, 12);
	else
		ss_cpy_wn(&a, t, 11);
	res |= !ss_cmp(a, b_all11) ? 0 : 16;
	res |= !strcmp(TU8ALL11, ss_to_c(b_all11)) ? 0 : 32;
	res |= ss_len_u(b_a3) == 3 && ss_len_u(b_b3) == 3 && ss_len_u(b_c3) == 3
			       && ss_len_u(b_d2) == 2 && ss_len_u(b_all11) == 11
		       ? 0
		       : 64;
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b_a3, &b_b3, &b_c3, &b_d2, &b_all11);
#else
	ss_free(&a);
	ss_free(&b_a3);
	ss_free(&b_b3);
	ss_free(&b_c3);
	ss_free(&b_d2);
	ss_free(&b_all11);
#endif
	return res;
}

static int test_ss_cpy_int(int64_t num, const char *expected)
{
	int res;
	srt_string *a = NULL;
	ss_cpy_int(&a, num);
	res = !a ? 1 : (!strcmp(ss_to_c(a), expected) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_cpy_tolower(const srt_string *a, const srt_string *b)
{
	int res;
	srt_string *sa = ss_dup(a), *sb = ss_dup_c("garbage i!&/()=");
	ss_cpy_tolower(&sb, sa);
	res = (!sa || !sb)
		      ? 1
		      : (ss_tolower(&sa) ? 0 : 2) | (!ss_cmp(sa, b) ? 0 : 4)
				| (!ss_cmp(sb, b) ? 0 : 8);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_cpy_toupper(const srt_string *a, const srt_string *b)
{
	int res;
	srt_string *sa = ss_dup(a), *sb = ss_dup_c("garbage i!&/()=");
	ss_cpy_toupper(&sb, sa);
	res = (!sa || !sb)
		      ? 1
		      : (ss_toupper(&sa) ? 0 : 2) | (!ss_cmp(sa, b) ? 0 : 4)
				| (!ss_cmp(sb, b) ? 0 : 8);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_cpy_erase(const char *in, size_t off, size_t size,
			     const char *expected)
{
	int res;
	srt_string *a = ss_dup_c(in), *b = ss_dup_c("garbage i!&/()=");
	ss_cpy_erase(&b, a, off, size);
	res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), expected) ? 0 : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b);
#else
	ss_free(&a);
	ss_free(&b);
#endif
	return res;
}

static int test_ss_cpy_erase_u()
{
	int res;
	srt_string *a = ss_dup_c("hel" U8_S_N_TILDE_F1 "lo"),
		   *b = ss_dup_c("garbage i!&/()=");
	ss_cpy_erase_u(&b, a, 2, 3);
	res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), "heo") ? 0 : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b);
#else
	ss_free(&a);
	ss_free(&b);
#endif
	return res;
}

static int test_ss_cpy_replace(const char *in, const char *r, const char *s,
			       const char *expected)
{
	int res;
	srt_string *a = ss_dup_c(in), *b = ss_dup_c(r), *c = ss_dup_c(s),
		   *d = ss_dup_c("garbage i!&/()=");
	ss_cpy_replace(&d, a, 0, b, c);
	res = (!a || !b) ? 1 : (!strcmp(ss_to_c(d), expected) ? 0 : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c, &d);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
	ss_free(&d);
#endif
	return res;
}

static int test_ss_cpy_resize()
{
	int res;
	srt_string *a = ss_dup_c("hello"), *b = ss_dup_c("garbage i!&/()="),
		   *c = ss_dup_c("garbage i!&/()=");
	ss_cpy_resize(&b, a, 10, 'z');
	ss_cpy_resize(&c, a, 2, 'z');
	res = (!a || !b) ? 1
			 : (!strcmp(ss_to_c(b), "hellozzzzz") ? 0 : 2)
				   | (!strcmp(ss_to_c(c), "he") ? 0 : 4);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
#endif
	return res;
}

static int test_ss_cpy_resize_u()
{
	int res;
	srt_string *a = ss_dup_c(U8_S_N_TILDE_F1 "hello"),
		   *b = ss_dup_c("garbage i!&/()="),
		   *c = ss_dup_c("garbage i!&/()=");
	ss_cpy_resize_u(&b, a, 11, 'z');
	ss_cpy_resize_u(&c, a, 3, 'z');
	res = (!a || !b)
		      ? 1
		      : (!strcmp(ss_to_c(b), U8_S_N_TILDE_F1 "hellozzzzz") ? 0
									   : 2)
				| (!strcmp(ss_to_c(c), U8_S_N_TILDE_F1 "he")
					   ? 0
					   : 4);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
#endif
	return res;
}

static int test_ss_cpy_trim(const char *a, const char *expected)
{
	int res;
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_c("garbage i!&/()=");
	ss_cpy_trim(&sb, sa);
	res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sb), expected) ? 0 : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_cpy_ltrim(const char *a, const char *expected)
{
	int res;
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_c("garbage i!&/()=");
	ss_cpy_ltrim(&sb, sa);
	res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sb), expected) ? 0 : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_cpy_rtrim(const char *a, const char *expected)
{
	int res;
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_c("garbage i!&/()=");
	ss_cpy_rtrim(&sb, sa);
	res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sb), expected) ? 0 : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_cpy_printf()
{
	int res;
	char btmp[512];
	srt_string *a = NULL;
	ss_cpy_printf(&a, 512, "abc%i%s%08X", 1, "hello", -1);
	sprintf(btmp, "abc%i%s%08X", 1, "hello", -1);
	res = !a ? 1 : !strcmp(ss_to_c(a), btmp) ? 0 : 2;
	ss_free(&a);
	return res;
}

static int test_ss_cpy_printf_va(const char *expected, size_t max_size,
				 const char *fmt, ...)
{
	int res;
	srt_string *a = NULL;
	va_list ap;
	va_start(ap, fmt);
	ss_cpy_printf_va(&a, max_size, fmt, ap);
	va_end(ap);
	res = !a ? 1 : !strcmp(ss_to_c(a), expected) ? 0 : 2;
	ss_free(&a);
	return res;
}

static int test_ss_cpy_char(int32_t in, const char *expected)
{
	int res;
	srt_string *a = ss_dup_c("zz");
	ss_cpy_char(&a, in);
	res = !a ? 1 : !strcmp(ss_to_c(a), expected) ? 0 : 2;
	ss_free(&a);
	return res;
}

/* clang-format off */
static int test_ss_cpy_read()
{
	int res = 1;
	size_t max_buf = 512;
	srt_string *ah = NULL;
	srt_string *as = ss_alloca(max_buf);
	size_t write_size;
	const char *pattern = "hello world";
	size_t pattern_size = strlen(pattern);
	FILE *f = fopen(STEST_FILE, S_FOPEN_BINARY_RW_TRUNC);
	ss_cpy(&as, ah);
	if (f) {
		write_size = fwrite(pattern, 1, pattern_size, f);
		res = !write_size || ferror(f) ||
			write_size != pattern_size ? 2 :
			fseek(f, 0, SEEK_SET) != 0 ? 4 :
			!ss_cpy_read(&ah, f, max_buf) ? 8 :
			fseek(f, 0, SEEK_SET) != 0 ? 16 :
			!ss_cpy_read(&as, f, max_buf) ? 32 :
			strcmp(ss_to_c(ah), pattern) ? 64 :
			strcmp(ss_to_c(as), pattern) ? 128 : 0;
		fclose(f);
		if (remove(STEST_FILE) != 0)
			res |= 256;
	}
	ss_free(&ah);
	return res;
}
/* clang-format on */

static int test_ss_cat(const char *a, const char *b)
{
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	srt_string *sc = ss_dup_c("c"), *sd = ss_dup_c("d"),
		   *se = ss_dup_c("e"), *sf = ss_dup_c("f");
	int res = (sa && sb) ? 0 : 1;
	char btmp[8192];
#ifdef S_DEBUG
	size_t alloc_calls_before, alloc_calls_after;
#endif
	sprintf(btmp, "%s%s%s", a, b, b);
	res |= res ? 0
#ifdef S_USE_VA_ARGS
		   : (ss_cat(&sa, sb, sb) ? 0 : 2)
#else
		   : (ss_cat(&sa, sb) && ss_cat(&sa, sb) ? 0 : 2)
#endif
			       | (!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	/* Aliasing test: */
	res |= res ? 0
		   : (sprintf(btmp, "%s%s%s%s", ss_to_c(sa), ss_to_c(sa),
			      ss_to_c(sa), ss_to_c(sa))
#ifdef S_USE_VA_ARGS
				      && ss_cat(&sa, sa, sa, sa)
#else
				      && ss_cat(&sa, sa) && ss_cat(&sa, sa)
#endif
			      ? 0
			      : 8);
	res |= res ? 0 : (!strcmp(ss_to_c(sa), btmp) ? 0 : 16);
#if defined(S_USE_VA_ARGS) && defined(S_DEBUG)
	alloc_calls_before = dbg_cnt_alloc_calls;
#endif
	res |= res ? 0
		   : (sc && sd && se && sf
#ifdef S_USE_VA_ARGS
		      && ss_cat(&sc, sd, se, sf)
#else
		      && ss_cat(&sc, sd) && ss_cat(&sc, se) && ss_cat(&sc, sf)
#endif
			      )
			       ? 0
			       : 32;
#if defined(S_USE_VA_ARGS) && defined(S_DEBUG)
	/* Check that multiple append uses just one -or zero- allocation: */
	alloc_calls_after = dbg_cnt_alloc_calls;
	res |= res ? 0
		   : (alloc_calls_after - alloc_calls_before) <= 1 ? 0 : 128;
#endif
	/* Aliasing check */
	res |= res ? 0
#ifdef S_USE_VA_ARGS
		   : ss_cat(&sf, se, sf, se, sf, se)
#else
		   : ss_cat(&sf, se) && ss_cat(&sf, sf) && ss_cat_c(&sf, "fe")
#endif
				       && !strcmp(ss_to_c(sf), "fefefe")
			       ? 0
			       : 64;
	res |= res ? 0 : (!strcmp(ss_to_c(sc), "cdef")) ? 0 : 256;
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb, &sc, &sd, &se, &sf);
#else
	ss_free(&sa);
	ss_free(&sb);
	ss_free(&sc);
	ss_free(&sd);
	ss_free(&se);
	ss_free(&sf);
#endif
	return res;
}

static int test_ss_cat_substr()
{
	srt_string *a = ss_dup_c("how are you"), *b = ss_dup_c(" "),
		   *c = ss_dup_c("are you"), *d = ss_dup_c("are ");
	int res = (!a || !b) ? 1 : 0;
	res |= res ? 0 : (ss_cat_substr(&d, a, 8, 3) ? 0 : 4);
	res |= res ? 0 : (!ss_cmp(c, d) ? 0 : 8);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c, &d);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
	ss_free(&d);
#endif
	return res;
}

static int test_ss_cat_substr_u()
{
	/* like the above, but replacing the 'e' with a Chinese character */
	srt_string *a = ss_dup_c("how ar" U8_HAN_24B62 " you"),
		   *b = ss_dup_c(" "), *c = ss_dup_c("ar" U8_HAN_24B62 " you"),
		   *d = ss_dup_c("ar" U8_HAN_24B62 " ");
	int res = (!a || !b) ? 1 : 0;
	res |= res ? 0 : (ss_cat_substr_u(&d, a, 8, 3) ? 0 : 4);
	res |= res ? 0 : (!ss_cmp(c, d) ? 0 : 8);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c, &d);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
	ss_free(&d);
#endif
	return res;
}

static int test_ss_cat_cn()
{
	const char *a = "12345", *b = "67890";
	srt_string *sa = ss_dup_c(a);
	int res = sa ? 0 : 1;
	char btmp[8192];
	sprintf(btmp, "%s%s", a, b);
	res |= res ? 0
		   : (ss_cat_cn(&sa, b, 5) ? 0 : 2)
			       | (!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	ss_free(&sa);
	return res;
}

static int test_ss_cat_c(const char *a, const char *b)
{
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_c("b");
	int res = sa ? 0 : 1;
	char btmp[8192];
	sprintf(btmp, "%s%s%s", a, b, b);
	res |= res ? 0
#ifdef S_USE_VA_ARGS
		   : (ss_cat_c(&sa, b, b) ? 0 : 2)
#else
		   : (ss_cat_c(&sa, b) && ss_cat_c(&sa, b) ? 0 : 2)
#endif
			       | (!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	/* Concatenate multiple literal inputs */
	res |= res ? 0
#ifdef S_USE_VA_ARGS
		   : ss_cat_c(&sb, "c", "b", "c", "b", "c")
#else
		   : ss_cat_c(&sb, "c") && ss_cat_c(&sb, "b")
				       && ss_cat_c(&sb, "c")
				       && ss_cat_c(&sb, "b")
				       && ss_cat_c(&sb, "c")
#endif
				       && !strcmp(ss_to_c(sb), "bcbcbc")
			       ? 0
			       : 8;
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_cat_wn()
{
	const wchar_t *a = L"12345", *b = L"67890";
	srt_string *sa = ss_dup_w(a);
	int res = sa ? 0 : 1;
	char btmp[8192];
	sprintf(btmp, "%ls%ls", a, b);
	res |= res ? 0
		   : (ss_cat_wn(&sa, b, 5) ? 0 : 2)
			       | (!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	ss_free(&sa);
	return res;
}

static int test_ss_cat_w(const wchar_t *a, const wchar_t *b)
{
	srt_string *sa = ss_dup_w(a), *sb = ss_dup_c("b");
	int res = sa ? 0 : 1;
	char btmp[8192];
	sprintf(btmp, "%ls%ls%ls%ls", a, b, b, b);
	res |= res ? 0
#ifdef S_USE_VA_ARGS
		   : (ss_cat_w(&sa, b, b, b) ? 0 : 2)
#else
		   : (ss_cat_w(&sa, b) && ss_cat_w(&sa, b) && ss_cat_w(&sa, b)
			      ? 0
			      : 2)
#endif
			       | (!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	/* Concatenate multiple literal inputs */
	res |= res ? 0
#ifdef S_USE_VA_ARGS
		   : ss_cat_w(&sb, L"c", L"b", L"c", L"b", L"c")
#else
		   : ss_cat_w(&sb, L"c") && ss_cat_w(&sb, L"b")
				       && ss_cat_w(&sb, L"c")
				       && ss_cat_w(&sb, L"b")
				       && ss_cat_w(&sb, L"c")
#endif
				       && !strcmp(ss_to_c(sb), "bcbcbc")
			       ? 0
			       : 8;
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_cat_int(const char *in, int64_t num, const char *expected)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_cat_int(&sa, num), in, "", expected);
}

static int test_ss_cat_tolower(const srt_string *a, const srt_string *b,
			       const srt_string *expected)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_cat_tolower(&sa, sb), ss_to_c(a), ss_to_c(b),
		      ss_to_c(expected));
}

static int test_ss_cat_toupper(const srt_string *a, const srt_string *b,
			       const srt_string *expected)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_cat_toupper(&sa, sb), ss_to_c(a), ss_to_c(b),
		      ss_to_c(expected));
}

static int test_ss_cat_erase(const char *prefix, const char *in, size_t off,
			     size_t size, const char *expected)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_cat_erase(&sa, sb, off, size), prefix, in, expected);
}

static int test_ss_cat_erase_u()
{
	TEST_SS_OPCHK_VARS;
	const char *a = "hel" U8_S_N_TILDE_F1 "lo", *b = "x";
	TEST_SS_OPCHK(ss_cat_erase_u(&sa, sb, 2, 3), b, a, "xheo");
}

static int test_ss_cat_replace(const char *prefix, const char *in,
			       const char *r, const char *s,
			       const char *expected)
{
	TEST_SS_OPCHK_VARS;
	const srt_string *sr = ss_crefa(r), *ss = ss_crefa(s);
	TEST_SS_OPCHK(ss_cat_replace(&sa, sb, 0, sr, ss), prefix, in, expected);
}

static int test_ss_cat_resize()
{
	int res;
	srt_string *a = ss_dup_c("hello"), *b = ss_dup_c("x"),
		   *c = ss_dup_c("x");
	ss_cat_resize(&b, a, 10, 'z');
	ss_cat_resize(&c, a, 2, 'z');
	res = (!a || !b) ? 1
			 : (!strcmp(ss_to_c(b), "xhellozzzzz") ? 0 : 2)
				   | (!strcmp(ss_to_c(c), "xhe") ? 0 : 4);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
#endif
	return res;
}

static int test_ss_cat_resize_u()
{
	int res;
	srt_string *a = ss_dup_c(U8_S_N_TILDE_F1 "hello"), *b = ss_dup_c("x"),
		   *c = ss_dup_c("x");
	ss_cat_resize_u(&b, a, 11, 'z');
	ss_cat_resize_u(&c, a, 3, 'z');
	res = (!a || !b)
		      ? 1
		      : (!strcmp(ss_to_c(b), "x" U8_S_N_TILDE_F1 "hellozzzzz")
				 ? 0
				 : 2)
				| (!strcmp(ss_to_c(c), "x" U8_S_N_TILDE_F1 "he")
					   ? 0
					   : 4);
#ifdef S_USE_VA_ARGS
	ss_free(&a, &b, &c);
#else
	ss_free(&a);
	ss_free(&b);
	ss_free(&c);
#endif
	return res;
}

static int test_ss_cat_trim(const char *a, const char *b, const char *expected)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_cat_trim(&sa, sb), a, b, expected);
}

static int test_ss_cat_ltrim(const char *a, const char *b, const char *expected)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_cat_ltrim(&sa, sb), a, b, expected);
}

static int test_ss_cat_rtrim(const char *a, const char *b, const char *expected)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_cat_rtrim(&sa, sb), a, b, expected);
}

static int test_ss_cat_printf()
{
	char expected[512];
	const char *b0 = "hello";
	TEST_SS_OPCHK_VARS;
	sprintf(expected, "abc%i%s%08X", 1, "hello", -1);
	TEST_SS_OPCHK(ss_cat_printf(&sa, 512, "%i%s%08X", 1, b0, -1), "abc", b0,
		      expected);
}

static int test_ss_cat_printf_va()
{
	return test_ss_cat_printf();
}

static int test_ss_cat_char()
{
	char expected[512];
	const char *a = "12345", b0 = '6';
	TEST_SS_OPCHK_VARS;
	sprintf(expected, "%s%c", a, b0);
	TEST_SS_OPCHK(ss_cat_char(&sa, b0), a, "", expected);
}

/* clang-format off */
static int test_ss_cat_read()
{
	int res = 1;
	size_t max_buf = 512;
	srt_string *ah = ss_dup_c("hello "), *as = ss_alloca(max_buf);
	const char *pattern = "hello world", *suffix = "world";
	size_t suffix_size = strlen(suffix);
	FILE *f = fopen(STEST_FILE, S_FOPEN_BINARY_RW_TRUNC);
	ss_cpy(&as, ah);
	if (f) {
		size_t write_size = fwrite(suffix, 1, suffix_size, f);
		res = !write_size || ferror(f) ||
			write_size != suffix_size ? 2 :
			fseek(f, 0, SEEK_SET) != 0 ? 4 :
			!ss_cat_read(&ah, f, max_buf) ? 8 :
			fseek(f, 0, SEEK_SET) != 0 ? 16 :
			!ss_cat_read(&as, f, max_buf) ? 32 :
			strcmp(ss_to_c(ah), pattern) ? 64 :
			strcmp(ss_to_c(as), pattern) ? 128 : 0;
		fclose(f);
		if (remove(STEST_FILE) != 0)
			res |= 256;
	}
	ss_free(&ah);
	return res;
}
/* clang-format on */

static int test_ss_tolower(const char *a, const char *b)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_tolower(&sa), a, "", b);
}

static int test_ss_toupper(const char *a, const char *b)
{
	TEST_SS_OPCHK_VARS;
	TEST_SS_OPCHK(ss_toupper(&sa), a, "", b);
}

static int test_ss_clear(const char *in)
{
	srt_string *sa = ss_dup_c(in);
	int res = !sa ? 1
		      : (ss_len(sa) == strlen(in) ? 0 : 2)
				  | (ss_capacity(sa) == strlen(in) ? 0 : 4);
	ss_clear(sa);
	res |= res ? 0 : (ss_len(sa) == 0 ? 0 : 16);
	ss_free(&sa);
	return res;
}

static int test_ss_check()
{
	srt_string *sa = NULL;
	int res = ss_check(&sa) ? 0 : 1;
	ss_free(&sa);
	return res;
}

static int test_ss_replace(const char *s, size_t off, const char *s1,
			   const char *s2, const char *expected)
{
	TEST_SS_OPCHK_VARS;
	const srt_string *sc = ss_crefa(s2);
	TEST_SS_OPCHK(ss_replace(&sa, off, sb, sc), s, s1, expected);
}

static int test_ss_to_c(const char *in)
{
	srt_string *a = ss_dup_c(in);
	int res = !a ? 1 : (!strcmp(in, ss_to_c(a)) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_to_w(const char *in)
{
	srt_string *a = ss_dup_c(in);
	/* wchar_t == 2 -> UTF16, it could require twice the length
	 * wchar_t > 2 -> UTF32
	 */
	size_t ssa = ss_len(a) * (sizeof(wchar_t) == 2 ? 2 : 1);
	wchar_t *out =
		a ? (wchar_t *)s_malloc(sizeof(wchar_t) * (ssa + 1)) : NULL;
	size_t out_size = 0;
	int res = !a ? 1 : (ss_to_w(a, out, ssa + 1, &out_size) ? 0 : 2);
	res |= (((ssa > 0 && out_size > 0) || ssa == out_size) ? 0 : 4);
	if (!res) {
		char b1[4096], b2[4096];
		int sb1i = sprintf(b1, "%s", ss_to_c(a));
		int sb2i = sprintf(b2, "%ls", out);
		size_t sb1 = sb1i >= 0 ? (size_t)sb1i : 0,
		       sb2 = sb2i >= 0 ? (size_t)sb2i : 0;
		res |= (sb1 == sb2 ? 0 : 8) | (!memcmp(b1, b2, sb1) ? 0 : 16);
	}
	free(out);
	ss_free(&a);
	return res;
}

static int test_ss_find(const char *a, const char *b, size_t expected_loc)
{
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	int res =
		(!sa || !sb) ? 1 : (ss_find(sa, 0, sb) == expected_loc ? 0 : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_find_misc()
{
	int res = 0;
	/*                    01234 56 7 8901234  5             6 */
	const char *sample = "abc \t \n\r 123zyx" U8_HAN_24B62 "s";
	const srt_string *a = ss_crefa(sample),
			 *han = ss_crefa(U8_HAN_24B62 "s"),
			 *yx = ss_crefa("yx");
	res |= ss_findb(a, 0) == 3 ? 0 : 1;
	res |= ss_findrb(a, 0, S_NPOS) == 3 ? 0 : 2;
	res |= ss_findc(a, 0, 'z') == 12 ? 0 : 4;
	res |= ss_findrc(a, 0, S_NPOS, 'z') == 12 ? 0 : 8;
	res |= ss_findu(a, 0, 0x24B62) == 15 ? 0 : 16;
	res |= ss_findru(a, 0, S_NPOS, 0x24B62) == 15 ? 0 : 32;
	res |= ss_findcx(a, 0, '0', '9') == 9 ? 0 : 64;
	res |= ss_findrcx(a, 0, S_NPOS, '0', '9') == 9 ? 0 : 128;
	res |= ss_findnb(a, 3) == 9 ? 0 : 256;
	res |= ss_findrnb(a, 3, S_NPOS) == 9 ? 0 : 512;
	res |= ss_find(a, 0, han) == 15 ? 0 : 1024;
	res |= ss_findr(a, 0, S_NPOS, yx) == 13 ? 0 : 2048;
	res |= ss_find_cn(a, 0, "yx", 2) == 13 ? 0 : 4096;
	res |= ss_findr_cn(a, 0, S_NPOS, "yx", 2) == 13 ? 0 : 8192;
	return res;
}

static int test_ss_split()
{
	const char *howareyou = "how are you";
	srt_string *a = ss_dup_c(howareyou), *sep1 = ss_dup_c(" ");
	const srt_string *c = ss_crefa(howareyou), *sep2 = ss_crefa(" "),
			 *how = ss_crefa("how"), *are = ss_crefa("are"),
			 *you = ss_crefa("you");
#define TSS_SPLIT_MAX_SUBS 16
	srt_string_ref subs[TSS_SPLIT_MAX_SUBS];
	size_t elems1 = ss_split(a, sep1, subs, TSS_SPLIT_MAX_SUBS), elems2;
	int res = 0;
	if (!a || !sep1) {
		res |= 1;
	} else {
		if (elems1 != 3 || ss_cmp(ss_ref(&subs[0]), how)
		    || ss_cmp(ss_ref(&subs[1]), are)
		    || ss_cmp(ss_ref(&subs[2]), you)) {
			res |= 2;
		} else {
			elems2 = ss_split(c, sep2, subs, TSS_SPLIT_MAX_SUBS);
			if (elems2 != 3 || ss_cmp(ss_ref(&subs[0]), how)
			    || ss_cmp(ss_ref(&subs[1]), are)
			    || ss_cmp(ss_ref(&subs[2]), you)) {
				res |= 4;
			}
		}
	}
#ifdef S_USE_VA_ARGS
	ss_free(&a, &sep1);
#else
	ss_free(&a);
	ss_free(&sep1);
#endif
	return res;
}

static int validate_cmp(int res1, int res2)
{
	return (res1 == 0 && res2 == 0) || (res1 < 0 && res2 < 0)
			       || (res1 > 0 && res2 > 0)
		       ? 1
		       : 0;
}

static int test_ss_cmp(const char *a, const char *b, int expected_cmp)
{
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	int res =
		(!sa || !sb)
			? 1
			: (validate_cmp(ss_cmp(sa, sb), expected_cmp) ? 0 : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_cmpi(const char *a, const char *b, int expected_cmp)
{
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	int res =
		(!sa || !sb)
			? 1
			: (validate_cmp(ss_cmpi(sa, sb), expected_cmp) ? 0 : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_ncmp(const char *a, size_t a_off, const char *b, size_t n,
			int expected_cmp)
{
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	int res =
		(!sa || !sb)
			? 1
			: (validate_cmp(ss_ncmp(sa, a_off, sb, n), expected_cmp)
				   ? 0
				   : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_ncmpi(const char *a, size_t a_off, const char *b, size_t n,
			 int expected_cmp)
{
	srt_string *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	int res = (!sa || !sb) ? 1
			       : (validate_cmp(ss_ncmpi(sa, a_off, sb, n),
					       expected_cmp)
					  ? 0
					  : 2);
#ifdef S_USE_VA_ARGS
	ss_free(&sa, &sb);
#else
	ss_free(&sa);
	ss_free(&sb);
#endif
	return res;
}

static int test_ss_printf()
{
	int res;
	srt_string *a = NULL;
	char btmp[512];
	sprintf(btmp, "%i%s%08X", 1, "hello", -1);
	ss_printf(&a, 512, "%i%s%08X", 1, "hello", -1);
	res = !strcmp(ss_to_c(a), btmp) ? 0 : 1;
	ss_free(&a);
	return res;
}

static int test_ss_getchar()
{
	srt_string *a = ss_dup_c("12");
	size_t off = 0;
	int res = !a ? 1
		     : (ss_getchar(a, &off) == '1' ? 0 : 2)
				  | (ss_getchar(a, &off) == '2' ? 0 : 4)
				  | (ss_getchar(a, &off) == EOF ? 0 : 8);
	ss_free(&a);
	return res;
}

static int test_ss_putchar()
{
	srt_string *a = NULL;
	int res = ss_putchar(&a, '1') && ss_putchar(&a, '2') ? 0 : 1;
	res |= res ? 0 : (!strcmp(ss_to_c(a), "12") ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_popchar()
{
	srt_string *a = ss_dup_c("12" U8_C_N_TILDE_D1 "a");
	int res = !a ? 1
		     : (ss_popchar(&a) == 'a' ? 0 : 2)
				  | (ss_popchar(&a) == 0xd1 ? 0 : 4)
				  | (ss_popchar(&a) == '2' ? 0 : 8)
				  | (ss_popchar(&a) == '1' ? 0 : 16)
				  | (ss_popchar(&a) == EOF ? 0 : 32);
	ss_free(&a);
	return res;
}

/* clang-format off */
static int test_ss_read_write()
{
	int res = 1; /* Error: can not open file */
	FILE *f;
	const char *a;
	size_t la;
	srt_string *sa, *sb;
	remove(STEST_FILE);
	f = fopen(STEST_FILE, S_FOPEN_BINARY_RW_TRUNC);
	if (f) {
		a = "once upon a time";
		la = strlen(a);
		sa = ss_dup_c(a);
		sb = NULL;
		res =	ss_size(sa) != la ? 2 :
			ss_write(f, sa, 0, S_NPOS) != (ssize_t)la ? 3 :
			fseek(f, 0, SEEK_SET) != 0 ? 4 :
			ss_read(&sb, f, 4000) != (ssize_t)la ? 5 :
			strcmp(ss_to_c(sa), ss_to_c(sb)) != 0 ? 6 : 0;
#ifdef S_USE_VA_ARGS
		ss_free(&sa, &sb);
#else
		ss_free(&sa);
		ss_free(&sb);
#endif
		fclose(f);
		if (remove(STEST_FILE) != 0)
			res |= 32;
	}
	return res;
}
/* clang-format on */

static int test_ss_csum32()
{
	const char *a = "hola";
	uint32_t a_crc32 = 0x6fa0f988;
	srt_string *sa = ss_dup_c(a);
	int res = ss_crc32(sa) != a_crc32 ? 1 : 0;
	ss_free(&sa);
	return res;
}

static int test_sc_utf8_to_wc(const char *utf8_char, int32_t unicode32_expected)
{
	int32_t uc_out = 0;
	size_t char_size = sc_utf8_to_wc(utf8_char, 0, 6, &uc_out, NULL);
	return !char_size ? 1 : (uc_out == unicode32_expected ? 0 : 2);
}

static int test_sc_wc_to_utf8(int32_t unicode32, const char *utf8_char_expected)
{
	char utf8[6];
	size_t char_size = sc_wc_to_utf8(unicode32, utf8, 0, 6);
	return !char_size
		       ? 1
		       : (!memcmp(utf8, utf8_char_expected, char_size) ? 0 : 2);
}

static int test_ss_null()
{
	/*
	 * This tests code is not crashing (more calls to be added)
	 */
	ss_clear(NULL);
	ss_to_c(NULL);
	ss_reserve(NULL, 0);
	ss_cpy(NULL, NULL);
	ss_cpy_c(NULL, NULL);
	ss_cat(NULL, NULL);
	ss_cat_c(NULL, NULL);
	ss_printf(NULL, 0, "");
	return 0;
}

static int test_ss_misc()
{
	srt_string *a = ss_alloc(10);
	int res = a ? 0 : 1;
	size_t gs;
	char xutf8[5][3] = {{(char)SSU8_S2, 32, '2'},
			    {(char)SSU8_S3, 32, '3'},
			    {(char)SSU8_S4, 32, '4'},
			    {(char)SSU8_S5, 32, '5'},
			    {(char)SSU8_S6, 32, '6'}};
	const srt_string *srefs[5] = {
		ss_refa_buf(xutf8[0], 3), ss_refa_buf(xutf8[1], 3),
		ss_refa_buf(xutf8[2], 3), ss_refa_buf(xutf8[3], 3),
		ss_refa_buf(xutf8[4], 3)};
	res |= (!ss_alloc_errors(a) ? 0 : 2);
	/*
	 * 32-bit mode: ask for growing 4 GB
	 * 64-bit mode: Ask for growing 1 PB (10^15 bytes), and check
	 * memory allocation fails:
	 */
#if UINTPTR_MAX == 0xffffffff
	gs = ss_grow(&a, (size_t)0xffffff00);
#elif UINTPTR_MAX > 0xffffffff
	gs = ss_grow(&a, (int64_t)1000 * 1000 * 1000 * 1000 * 1000);
#else
	gs = 0;
#endif
	res |= (gs == 0 && ss_alloc_errors(a) ? 0 : 4);
	/*
	 * Clear errors, and check are not set afterwards
	 */
	ss_clear_errors(a);
	res |= (!ss_alloc_errors(a) ? 0 : 8);
	ss_free(&a);
	/*
	 * UTF-8 encoding error check, on strings starting by
	 * Unicode characters of size 2/3/4/5/6 that instead of
	 * that size, have the first character broken
	 */
	res |= (ss_len_u(srefs[0]) == 2 && !ss_encoding_errors(srefs[0]) ? 0
									 : 16);
	res |= (ss_len_u(srefs[1]) == 1 && !ss_encoding_errors(srefs[1]) ? 0
									 : 32);
	res |= (ss_len_u(srefs[2]) == 3 && ss_encoding_errors(srefs[2]) ? 0
									: 64);
	res |= (ss_len_u(srefs[3]) == 3 && ss_encoding_errors(srefs[3]) ? 0
									: 128);
	res |= (ss_len_u(srefs[4]) == 3 && ss_encoding_errors(srefs[4]) ? 0
									: 256);
	ss_clear_errors((srt_string *)srefs[2]);
	ss_clear_errors((srt_string *)srefs[3]);
	ss_clear_errors((srt_string *)srefs[4]);
	res |= (!ss_encoding_errors(srefs[2]) && !ss_encoding_errors(srefs[3])
				&& !ss_encoding_errors(srefs[4])
			? 0
			: 512);
	return res;
}

#define TEST_SV_ALLOC(sv_alloc_x, sv_alloc_x_t, free_op)                       \
	srt_vector *a = sv_alloc_x(sizeof(struct AA), 10, NULL),               \
		   *b = sv_alloc_x_t(SV_I8, 10);                               \
	int res = (!a || !b)                                                   \
			  ? 1                                                  \
			  : (sv_len(a) == 0 ? 0 : 2) | (sv_empty(a) ? 0 : 4)   \
				    | (sv_empty(b) ? 0 : 8)                    \
				    | (sv_push(&a, &a1) ? 0 : 16)              \
				    | (sv_push_i8(&b, 1) ? 0 : 32)             \
				    | (!sv_empty(a) ? 0 : 64)                  \
				    | (!sv_empty(b) ? 0 : 128)                 \
				    | (sv_len(a) == 1 ? 0 : 256)               \
				    | (sv_len(b) == 1 ? 0 : 512)               \
				    | (!sv_push(&a, NULL) ? 0 : 1024)          \
				    | (sv_len(a) == 1 ? 0 : 2048);             \
	free_op;                                                               \
	return res;

static int test_sv_alloc()
{
#ifdef S_USE_VA_ARGS
	TEST_SV_ALLOC(sv_alloc, sv_alloc_t, sv_free(&a, &b));
#else
	TEST_SV_ALLOC(sv_alloc, sv_alloc_t, sv_free(&a); sv_free(&b));
#endif
}

static int test_sv_alloca()
{
	TEST_SV_ALLOC(sv_alloca, sv_alloca_t, {});
}

#define TEST_SV_GROW_VARS(v) srt_vector *v

#define TEST_SV_GROW(v, pushval, ntest, sv_alloc_f, data_id, CMPF,             \
		     initial_reserve, sv_push_x)                               \
	v = sv_alloc_f(data_id, initial_reserve CMPF);                         \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : (sv_push_x(&v, pushval) && sv_len(v) == 1                  \
		     && sv_push_x(&v, pushval) && sv_len(v) == 2               \
		     && sv_capacity(v) >= 2)                                   \
			       ? 0                                             \
			       : 2 << (ntest * 2);                             \
	sv_free(&v);

static int test_sv_grow()
{
	TEST_SV_GROW_VARS(a);
	TEST_SV_GROW_VARS(b);
	TEST_SV_GROW_VARS(c);
	TEST_SV_GROW_VARS(d);
	TEST_SV_GROW_VARS(e);
	TEST_SV_GROW_VARS(f);
	TEST_SV_GROW_VARS(g);
	TEST_SV_GROW_VARS(h);
	TEST_SV_GROW_VARS(i);
	TEST_SV_GROW_VARS(j);
	TEST_SV_GROW_VARS(k);
	int res = 0;
	TEST_SV_GROW(a, &a1, 0, sv_alloc, sizeof(struct AA), NO_CMPF, 1,
		     sv_push);
	TEST_SV_GROW(b, 123, 1, sv_alloc_t, SV_U8, X_CMPF, 1, sv_push_u8);
	TEST_SV_GROW(c, 123, 2, sv_alloc_t, SV_I8, X_CMPF, 1, sv_push_i8);
	TEST_SV_GROW(d, 123, 3, sv_alloc_t, SV_U16, X_CMPF, 1, sv_push_u16);
	TEST_SV_GROW(e, 123, 4, sv_alloc_t, SV_I16, X_CMPF, 1, sv_push_i16);
	TEST_SV_GROW(f, 123, 5, sv_alloc_t, SV_U32, X_CMPF, 1, sv_push_u32);
	TEST_SV_GROW(g, 123, 6, sv_alloc_t, SV_I32, X_CMPF, 1, sv_push_i32);
	TEST_SV_GROW(h, 123, 7, sv_alloc_t, SV_U64, X_CMPF, 1, sv_push_u64);
	TEST_SV_GROW(i, 123, 8, sv_alloc_t, SV_I64, X_CMPF, 1, sv_push_i64);
	TEST_SV_GROW(j, 123, 9, sv_alloc_t, SV_F, X_CMPF, 1, sv_push_f);
	TEST_SV_GROW(k, 123, 10, sv_alloc_t, SV_D, X_CMPF, 1, sv_push_d);
	return res;
}

#define TEST_SV_RESERVE_VARS(v) srt_vector *v

#define TEST_SV_RESERVE(v, ntest, sv_alloc_f, data_id, CMPF, reserve)          \
	v = sv_alloc_f(data_id, 1 CMPF);                                       \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : (sv_reserve(&v, reserve) && sv_len(v) == 0                 \
		     && sv_capacity(v) == reserve)                             \
			       ? 0                                             \
			       : 2 << (ntest * 2);                             \
	sv_free(&v);

static int test_sv_reserve()
{
	TEST_SV_RESERVE_VARS(a);
	TEST_SV_RESERVE_VARS(b);
	TEST_SV_RESERVE_VARS(c);
	TEST_SV_RESERVE_VARS(d);
	TEST_SV_RESERVE_VARS(e);
	TEST_SV_RESERVE_VARS(f);
	TEST_SV_RESERVE_VARS(g);
	TEST_SV_RESERVE_VARS(h);
	TEST_SV_RESERVE_VARS(i);
	TEST_SV_RESERVE_VARS(j);
	TEST_SV_RESERVE_VARS(k);
	int res = 0;
	TEST_SV_RESERVE(a, 0, sv_alloc, sizeof(struct AA), NO_CMPF, 1);
	TEST_SV_RESERVE(b, 1, sv_alloc_t, SV_U8, X_CMPF, 1);
	TEST_SV_RESERVE(c, 2, sv_alloc_t, SV_I8, X_CMPF, 1);
	TEST_SV_RESERVE(d, 3, sv_alloc_t, SV_U16, X_CMPF, 1);
	TEST_SV_RESERVE(e, 4, sv_alloc_t, SV_I16, X_CMPF, 1);
	TEST_SV_RESERVE(f, 5, sv_alloc_t, SV_U32, X_CMPF, 1);
	TEST_SV_RESERVE(g, 6, sv_alloc_t, SV_I32, X_CMPF, 1);
	TEST_SV_RESERVE(h, 7, sv_alloc_t, SV_U64, X_CMPF, 1);
	TEST_SV_RESERVE(i, 8, sv_alloc_t, SV_I64, X_CMPF, 1);
	TEST_SV_RESERVE(j, 9, sv_alloc_t, SV_F, X_CMPF, 1);
	TEST_SV_RESERVE(k, 10, sv_alloc_t, SV_D, X_CMPF, 1);
	return res;
}

#define TEST_SV_SHRINK_TO_FIT_VARS(v) srt_vector *v

#define TEST_SV_SHRINK_TO_FIT(v, ntest, alloc, push, type, CMPF, pushval, r)   \
	v = alloc(type, r CMPF);                                               \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : (push(&v, pushval) && sv_shrink(&v)                        \
		     && sv_capacity(v) == sv_len(v))                           \
			       ? 0                                             \
			       : 2 << (ntest * 2);                             \
	sv_free(&v)

static int test_sv_shrink()
{
	TEST_SV_SHRINK_TO_FIT_VARS(a);
	TEST_SV_SHRINK_TO_FIT_VARS(b);
	TEST_SV_SHRINK_TO_FIT_VARS(c);
	TEST_SV_SHRINK_TO_FIT_VARS(d);
	TEST_SV_SHRINK_TO_FIT_VARS(e);
	TEST_SV_SHRINK_TO_FIT_VARS(f);
	TEST_SV_SHRINK_TO_FIT_VARS(g);
	TEST_SV_SHRINK_TO_FIT_VARS(h);
	TEST_SV_SHRINK_TO_FIT_VARS(i);
	TEST_SV_SHRINK_TO_FIT_VARS(j);
	TEST_SV_SHRINK_TO_FIT_VARS(k);
	int res = 0;
	TEST_SV_SHRINK_TO_FIT(a, 0, sv_alloc, sv_push, sizeof(struct AA),
			      NO_CMPF, &a1, 100);
	TEST_SV_SHRINK_TO_FIT(b, 1, sv_alloc_t, sv_push_i8, SV_I8, X_CMPF, 123,
			      100);
	TEST_SV_SHRINK_TO_FIT(c, 2, sv_alloc_t, sv_push_u8, SV_U8, X_CMPF, 123,
			      100);
	TEST_SV_SHRINK_TO_FIT(d, 3, sv_alloc_t, sv_push_i16, SV_I16, X_CMPF,
			      123, 100);
	TEST_SV_SHRINK_TO_FIT(e, 4, sv_alloc_t, sv_push_u16, SV_U16, X_CMPF,
			      123, 100);
	TEST_SV_SHRINK_TO_FIT(f, 5, sv_alloc_t, sv_push_i32, SV_I32, X_CMPF,
			      123, 100);
	TEST_SV_SHRINK_TO_FIT(g, 6, sv_alloc_t, sv_push_u32, SV_U32, X_CMPF,
			      123, 100);
	TEST_SV_SHRINK_TO_FIT(h, 7, sv_alloc_t, sv_push_i64, SV_I64, X_CMPF,
			      123, 100);
	TEST_SV_SHRINK_TO_FIT(i, 8, sv_alloc_t, sv_push_u64, SV_U64, X_CMPF,
			      123, 100);
	TEST_SV_SHRINK_TO_FIT(j, 9, sv_alloc_t, sv_push_f, SV_F, X_CMPF, 123,
			      100);
	TEST_SV_SHRINK_TO_FIT(k, 10, sv_alloc_t, sv_push_d, SV_D, X_CMPF, 123,
			      100);
	return res;
}

#define TEST_SV_LEN_VARS(v) srt_vector *v

#define TEST_SV_LEN(v, ntest, alloc, push, type, CMPF, pushval)                \
	v = alloc(type, 1 CMPF);                                               \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : (push(&v, pushval) && push(&v, pushval)                    \
		     && push(&v, pushval) && push(&v, pushval)                 \
		     && sv_len(v) == 4)                                        \
			       ? 0                                             \
			       : 2 << (ntest * 2);                             \
	sv_free(&v)

static int test_sv_len()
{
	TEST_SV_LEN_VARS(a);
	TEST_SV_LEN_VARS(b);
	TEST_SV_LEN_VARS(c);
	TEST_SV_LEN_VARS(d);
	TEST_SV_LEN_VARS(e);
	TEST_SV_LEN_VARS(f);
	TEST_SV_LEN_VARS(g);
	TEST_SV_LEN_VARS(h);
	TEST_SV_LEN_VARS(i);
	TEST_SV_LEN_VARS(j);
	TEST_SV_LEN_VARS(k);
	int res = 0;
	TEST_SV_LEN(a, 0, sv_alloc, sv_push, sizeof(struct AA), NO_CMPF, &a1);
	TEST_SV_LEN(b, 1, sv_alloc_t, sv_push_i8, SV_I8, X_CMPF, 123);
	TEST_SV_LEN(c, 2, sv_alloc_t, sv_push_u8, SV_U8, X_CMPF, 123);
	TEST_SV_LEN(d, 3, sv_alloc_t, sv_push_i16, SV_I16, X_CMPF, 123);
	TEST_SV_LEN(e, 4, sv_alloc_t, sv_push_u16, SV_U16, X_CMPF, 123);
	TEST_SV_LEN(f, 5, sv_alloc_t, sv_push_i32, SV_I32, X_CMPF, 123);
	TEST_SV_LEN(g, 6, sv_alloc_t, sv_push_u32, SV_U32, X_CMPF, 123);
	TEST_SV_LEN(h, 7, sv_alloc_t, sv_push_i64, SV_I64, X_CMPF, 123);
	TEST_SV_LEN(i, 8, sv_alloc_t, sv_push_u64, SV_U64, X_CMPF, 123);
	TEST_SV_LEN(j, 9, sv_alloc_t, sv_push_f, SV_F, X_CMPF, 123);
	TEST_SV_LEN(k, 10, sv_alloc_t, sv_push_d, SV_D, X_CMPF, 123);
	return res;
}

#ifdef SD_ENABLE_HEURISTIC_GROWTH
#define SDCMP >=
#else
#define SDCMP ==
#endif

#define TEST_SV_CAPACITY_VARS(v) srt_vector *v

#define TEST_SV_CAPACITY(v, ntest, alloc, push, type, CMPF, pushval)           \
	v = alloc(type, 2 CMPF);                                               \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : (sv_capacity(v) == 2 && sv_reserve(&v, 100)                \
		     && sv_capacity(v) SDCMP 100 && sv_shrink(&v)              \
		     && sv_capacity(v) == 0)                                   \
			       ? 0                                             \
			       : 2 << (ntest * 2);                             \
	sv_free(&v)

static int test_sv_capacity()
{
	TEST_SV_CAPACITY_VARS(a);
	TEST_SV_CAPACITY_VARS(b);
	TEST_SV_CAPACITY_VARS(c);
	TEST_SV_CAPACITY_VARS(d);
	TEST_SV_CAPACITY_VARS(e);
	TEST_SV_CAPACITY_VARS(f);
	TEST_SV_CAPACITY_VARS(g);
	TEST_SV_CAPACITY_VARS(h);
	TEST_SV_CAPACITY_VARS(i);
	TEST_SV_CAPACITY_VARS(j);
	TEST_SV_CAPACITY_VARS(k);
	int res = 0;
	TEST_SV_CAPACITY(a, 0, sv_alloc, sv_push, sizeof(struct AA), NO_CMPF,
			 &a1);
	TEST_SV_CAPACITY(b, 1, sv_alloc_t, sv_push_i8, SV_I8, X_CMPF, 123);
	TEST_SV_CAPACITY(c, 2, sv_alloc_t, sv_push_u8, SV_U8, X_CMPF, 123);
	TEST_SV_CAPACITY(d, 3, sv_alloc_t, sv_push_i16, SV_I16, X_CMPF, 123);
	TEST_SV_CAPACITY(e, 4, sv_alloc_t, sv_push_u16, SV_U16, X_CMPF, 123);
	TEST_SV_CAPACITY(f, 5, sv_alloc_t, sv_push_i32, SV_I32, X_CMPF, 123);
	TEST_SV_CAPACITY(g, 6, sv_alloc_t, sv_push_u32, SV_U32, X_CMPF, 123);
	TEST_SV_CAPACITY(h, 7, sv_alloc_t, sv_push_i64, SV_I64, X_CMPF, 123);
	TEST_SV_CAPACITY(i, 8, sv_alloc_t, sv_push_u64, SV_U64, X_CMPF, 123);
	TEST_SV_CAPACITY(j, 9, sv_alloc_t, sv_push_f, SV_F, X_CMPF, 123);
	TEST_SV_CAPACITY(k, 10, sv_alloc_t, sv_push_d, SV_D, X_CMPF, 123);
	return res;
}

static int test_sv_capacity_left()
{
	size_t init_alloc = 10;
	srt_vector *a = sv_alloc_t(SV_I8, init_alloc);
	int res = !a ? 1
		     : (sv_capacity(a) - sv_len(a) == init_alloc ? 0 : 2)
				  | (sv_capacity_left(a) == init_alloc ? 0 : 4)
				  | (sv_push_i8(&a, 1) ? 0 : 8)
				  | (sv_capacity_left(a) == (init_alloc - 1)
					     ? 0
					     : 16);
	sv_free(&a);
	return res;
}

static int test_sv_get_buffer()
{
	srt_vector *a = sv_alloc_t(SV_I8, 0), *b = sv_alloca_t(SV_I8, 4),
		   *c = sv_alloc_t(SV_U32, 0), *d = sv_alloca_t(SV_U32, 1);
	size_t sa, sb, sc, sd;
	int res = !a || !b || !c || !d
			  ? 1
			  : (sv_push_i8(&a, 1) ? 0 : 2)
				    | (sv_push_i8(&a, 2) ? 0 : 4)
				    | (sv_push_i8(&a, 2) ? 0 : 8)
				    | (sv_push_i8(&a, 1) ? 0 : 0x10)
				    | (sv_push_i8(&b, 1) ? 0 : 0x20)
				    | (sv_push_i8(&b, 2) ? 0 : 0x40)
				    | (sv_push_i8(&b, 2) ? 0 : 0x80)
				    | (sv_push_i8(&b, 1) ? 0 : 0x100)
				    | (sv_push_u32(&c, 0x01020201) ? 0 : 0x200)
				    | (sv_push_u32(&d, 0x01020201) ? 0 : 0x400);
	if (!res) {
		unsigned ai = S_LD_U32(sv_get_buffer(a)),
			 bi = S_LD_U32(sv_get_buffer(b)),
			 air = S_LD_U32(sv_get_buffer_r(a)),
			 bir = S_LD_U32(sv_get_buffer_r(b)),
			 cir = S_LD_U32(sv_get_buffer(c)),
			 dir = S_LD_U32(sv_get_buffer(d));
		if (ai != bi || air != bir || ai != air || ai != cir
		    || ai != dir)
			res |= 0x800;
		if (ai != 0x01020201)
			res |= 0x1000;
		sa = sv_get_buffer_size(a);
		sb = sv_get_buffer_size(b);
		sc = sv_get_buffer_size(c);
		sd = sv_get_buffer_size(d);
		if (sa != sb || sa != sc || sa != sd)
			res |= 0x2000;
	}
#ifdef S_USE_VA_ARGS
	sv_free(&a, &c);
#else
	sv_free(&a);
	sv_free(&c);
#endif
	return res;
}

static int test_sv_elem_size()
{
	int res = (sv_elem_size(SV_I8) == 1 ? 0 : 2)
		  | (sv_elem_size(SV_U8) == 1 ? 0 : 4)
		  | (sv_elem_size(SV_I16) == 2 ? 0 : 8)
		  | (sv_elem_size(SV_U16) == 2 ? 0 : 16)
		  | (sv_elem_size(SV_I32) == 4 ? 0 : 32)
		  | (sv_elem_size(SV_U32) == 4 ? 0 : 64)
		  | (sv_elem_size(SV_I64) == 8 ? 0 : 128)
		  | (sv_elem_size(SV_U64) == 8 ? 0 : 256)
		  | (sv_elem_size(SV_F) == sizeof(float) ? 0 : 512)
		  | (sv_elem_size(SV_D) == sizeof(double) ? 0 : 1024);
	return res;
}

#define TEST_SV_DUP_VARS(v) srt_vector *v, *v##2

#define TEST_SV_DUP(v, ntest, alloc, push, check, check2, type, CMPF, pushval) \
	v = alloc(type, 0 CMPF);                                               \
	push(&v, pushval);                                                     \
	v##2 = sv_dup(v);                                                      \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : ((check) && (check2)) ? 0 : 2 << (ntest * 2);              \
	if (res)                                                               \
		fprintf(stderr, "%i %i %i\n", (int)(v != 0), (int)(check),     \
			(int)(check2));                                        \
	sv_free(&v);                                                           \
	sv_free(&v##2)

static int test_sv_dup()
{
	TEST_SV_DUP_VARS(z);
	TEST_SV_DUP_VARS(b);
	TEST_SV_DUP_VARS(c);
	TEST_SV_DUP_VARS(d);
	TEST_SV_DUP_VARS(e);
	TEST_SV_DUP_VARS(f);
	TEST_SV_DUP_VARS(g);
	TEST_SV_DUP_VARS(h);
	TEST_SV_DUP_VARS(i);
	TEST_SV_DUP_VARS(j);
	TEST_SV_DUP_VARS(k);
	int res = 0;
	int val = 123;
	TEST_SV_DUP(z, 0, sv_alloc, sv_push,
		    ((const struct AA *)sv_at(z, 0))->a == a1.a,
		    ((const struct AA *)sv_pop(z))->a
			    == ((const struct AA *)sv_pop(z2))->a,
		    sizeof(struct AA), NO_CMPF, &a1);
#define SVAT(v, at) at(v, 0) == val
#define SVUAT(v, at) at(v, 0) == (unsigned)val
#define CHKP(v, pop) pop(v) == pop(v##2)
	TEST_SV_DUP(b, 1, sv_alloc_t, sv_push_i8, SVAT(b, sv_at_i8),
		    CHKP(b, sv_pop_i8), SV_I8, X_CMPF, val);
	TEST_SV_DUP(c, 2, sv_alloc_t, sv_push_u8, SVUAT(c, sv_at_u8),
		    CHKP(c, sv_pop_u8), SV_U8, X_CMPF, val);
	TEST_SV_DUP(d, 3, sv_alloc_t, sv_push_i16, SVAT(d, sv_at_i16),
		    CHKP(d, sv_pop_i16), SV_I16, X_CMPF, val);
	TEST_SV_DUP(e, 4, sv_alloc_t, sv_push_u16, SVUAT(e, sv_at_u16),
		    CHKP(e, sv_pop_u16), SV_U16, X_CMPF, val);
	TEST_SV_DUP(f, 5, sv_alloc_t, sv_push_i32, SVAT(f, sv_at_i32),
		    CHKP(f, sv_pop_i32), SV_I32, X_CMPF, val);
	TEST_SV_DUP(g, 6, sv_alloc_t, sv_push_u32, SVUAT(g, sv_at_u32),
		    CHKP(g, sv_pop_u32), SV_U32, X_CMPF, val);
	TEST_SV_DUP(h, 7, sv_alloc_t, sv_push_i64, SVAT(h, sv_at_i64),
		    CHKP(h, sv_pop_i64), SV_I64, X_CMPF, val);
	TEST_SV_DUP(i, 8, sv_alloc_t, sv_push_u64, SVUAT(i, sv_at_u64),
		    CHKP(i, sv_pop_u64), SV_U64, X_CMPF, val);
	TEST_SV_DUP(j, 9, sv_alloc_t, sv_push_f, SVAT(j, sv_at_f),
		    CHKP(j, sv_pop_f), SV_F, X_CMPF, val);
	TEST_SV_DUP(k, 10, sv_alloc_t, sv_push_d, SVAT(k, sv_at_d),
		    CHKP(k, sv_pop_d), SV_D, X_CMPF, val);
#undef SVAT
#undef SVUAT
#undef CHKP
	return res;
}

#define TEST_SV_DUP_ERASE_VARS(v) srt_vector *v, *v##2

#define TEST_SV_DUP_ERASE(v, ntest, alloc, push, check, type, CMPF, a, b)      \
	v = alloc(type, 0 CMPF);                                               \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	v##2 = sv_dup_erase(v, 1, 5);                                          \
	res |= !v ? 1 << (ntest * 2) : (check) ? 0 : 2 << (ntest * 2);         \
	sv_free(&v);                                                           \
	sv_free(&v##2)

static int test_sv_dup_erase()
{
	TEST_SV_DUP_ERASE_VARS(z);
	TEST_SV_DUP_ERASE_VARS(b);
	TEST_SV_DUP_ERASE_VARS(c);
	TEST_SV_DUP_ERASE_VARS(d);
	TEST_SV_DUP_ERASE_VARS(e);
	TEST_SV_DUP_ERASE_VARS(f);
	TEST_SV_DUP_ERASE_VARS(g);
	TEST_SV_DUP_ERASE_VARS(h);
	TEST_SV_DUP_ERASE_VARS(i);
	TEST_SV_DUP_ERASE_VARS(j);
	TEST_SV_DUP_ERASE_VARS(k);
	int res = 0;
	int r = 12, s = 34;
	TEST_SV_DUP_ERASE(z, 0, sv_alloc, sv_push,
			  ((const struct AA *)sv_at(z2, 0))->a
				  == ((const struct AA *)sv_at(z2, 1))->a,
			  sizeof(struct AA), NO_CMPF, &a1, &a2);
#define SVAT(v, at) at(v##2, 0) == at(v##2, 1)
	TEST_SV_DUP_ERASE(b, 1, sv_alloc_t, sv_push_i8, SVAT(b, sv_at_i8),
			  SV_I8, X_CMPF, r, s);
	TEST_SV_DUP_ERASE(c, 2, sv_alloc_t, sv_push_u8, SVAT(c, sv_at_u8),
			  SV_U8, X_CMPF, r, s);
	TEST_SV_DUP_ERASE(d, 3, sv_alloc_t, sv_push_i16, SVAT(d, sv_at_i16),
			  SV_I16, X_CMPF, r, s);
	TEST_SV_DUP_ERASE(e, 4, sv_alloc_t, sv_push_u16, SVAT(e, sv_at_u16),
			  SV_U16, X_CMPF, r, s);
	TEST_SV_DUP_ERASE(f, 5, sv_alloc_t, sv_push_i32, SVAT(f, sv_at_i32),
			  SV_I32, X_CMPF, r, s);
	TEST_SV_DUP_ERASE(g, 6, sv_alloc_t, sv_push_u32, SVAT(g, sv_at_u32),
			  SV_U32, X_CMPF, r, s);
	TEST_SV_DUP_ERASE(h, 7, sv_alloc_t, sv_push_i64, SVAT(h, sv_at_i64),
			  SV_I64, X_CMPF, r, s);
	TEST_SV_DUP_ERASE(i, 8, sv_alloc_t, sv_push_u64, SVAT(i, sv_at_u64),
			  SV_U64, X_CMPF, r, s);
	TEST_SV_DUP_ERASE(j, 9, sv_alloc_t, sv_push_f, SVAT(j, sv_at_f), SV_F,
			  X_CMPF, r, s);
	TEST_SV_DUP_ERASE(k, 10, sv_alloc_t, sv_push_d, SVAT(k, sv_at_d), SV_D,
			  X_CMPF, r, s);
#undef SVAT
	return res;
}

#define TEST_SV_DUP_RESIZE_VARS(v) srt_vector *v, *v##2

#define TEST_SV_DUP_RESIZE(v, ntest, alloc, push, type, CMPF, a, b)            \
	v = alloc(type, 0 CMPF);                                               \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	v##2 = sv_dup_resize(v, 2);                                            \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : (sv_size(v##2) == 2 ? 0 : 2 << (ntest * 2))                \
			       | (!sv_ncmp(v, 0, v##2, 0, sv_size(v##2))       \
					  ? 0                                  \
					  : 4 << (ntest * 2))                  \
			       | (sv_ncmp(v, 0, v##2, 0, sv_size(v)) > 0       \
					  ? 0                                  \
					  : 8 << (ntest * 2))                  \
			       | (sv_ncmp(v##2, 0, v, 0, sv_size(v)) < 0       \
					  ? 0                                  \
					  : 16 << (ntest * 2));                \
	sv_free(&v);                                                           \
	sv_free(&v##2);

static int test_sv_dup_resize()
{
	TEST_SV_DUP_RESIZE_VARS(z);
	TEST_SV_DUP_RESIZE_VARS(b);
	TEST_SV_DUP_RESIZE_VARS(c);
	TEST_SV_DUP_RESIZE_VARS(d);
	TEST_SV_DUP_RESIZE_VARS(e);
	TEST_SV_DUP_RESIZE_VARS(f);
	TEST_SV_DUP_RESIZE_VARS(g);
	TEST_SV_DUP_RESIZE_VARS(h);
	TEST_SV_DUP_RESIZE_VARS(i);
	TEST_SV_DUP_RESIZE_VARS(j);
	TEST_SV_DUP_RESIZE_VARS(k);
	int res = 0;
	int r = 12, s = 34;
	TEST_SV_DUP_RESIZE(z, 0, sv_alloc, sv_push, sizeof(struct AA), NO_CMPF,
			   &a1, &a2);
	TEST_SV_DUP_RESIZE(b, 1, sv_alloc_t, sv_push_i8, SV_I8, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(c, 2, sv_alloc_t, sv_push_u8, SV_U8, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(d, 3, sv_alloc_t, sv_push_i16, SV_I16, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(e, 4, sv_alloc_t, sv_push_u16, SV_U16, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(f, 5, sv_alloc_t, sv_push_i32, SV_I32, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(g, 6, sv_alloc_t, sv_push_u32, SV_U32, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(h, 7, sv_alloc_t, sv_push_i64, SV_I64, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(i, 8, sv_alloc_t, sv_push_u64, SV_U64, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(j, 9, sv_alloc_t, sv_push_f, SV_F, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(k, 10, sv_alloc_t, sv_push_d, SV_D, X_CMPF, r, s);
	return res;
}

#define TEST_SV_CPY_VARS(v) srt_vector *v, *v##2

#define TEST_SV_CPY(v, nt, alloc, push, type, CMPF, a, b)                      \
	v = alloc(type, 0 CMPF);                                               \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	v##2 = NULL;                                                           \
	sv_cpy(&v##2, v);                                                      \
	res |= !v ? 1 << (nt * 2)                                              \
		  : (sv_size(v) == sv_size(v##2) ? 0 : 2 << (nt * 2))          \
			       | (!sv_ncmp(v, 0, v##2, 0, sv_size(v))          \
					  ? 0                                  \
					  : 4 << (nt * 2));                    \
	sv_free(&v);                                                           \
	sv_free(&v##2)

static int test_sv_cpy()
{
	TEST_SV_CPY_VARS(z);
	TEST_SV_CPY_VARS(b);
	TEST_SV_CPY_VARS(c);
	TEST_SV_CPY_VARS(d);
	TEST_SV_CPY_VARS(e);
	TEST_SV_CPY_VARS(f);
	TEST_SV_CPY_VARS(g);
	TEST_SV_CPY_VARS(h);
	TEST_SV_CPY_VARS(i);
	TEST_SV_CPY_VARS(j);
	TEST_SV_CPY_VARS(k);
	int res = 0;
	int r = 12, s = 34;
	srt_vector *v1 = sv_alloc_t(SV_I8, 0), *v2 = sv_alloc_t(SV_I64, 0),
		   *v1a3 = sv_alloca_t(SV_I8, 3),
		   *v1a24 = sv_alloca_t(SV_I8, 24);
	TEST_SV_CPY(z, 0, sv_alloc, sv_push, sizeof(struct AA), NO_CMPF, &a1,
		    &a2);
	TEST_SV_CPY(b, 1, sv_alloc_t, sv_push_i8, SV_I8, X_CMPF, r, s);
	TEST_SV_CPY(c, 2, sv_alloc_t, sv_push_u8, SV_U8, X_CMPF, r, s);
	TEST_SV_CPY(d, 3, sv_alloc_t, sv_push_i16, SV_I16, X_CMPF, r, s);
	TEST_SV_CPY(e, 4, sv_alloc_t, sv_push_u16, SV_U16, X_CMPF, r, s);
	TEST_SV_CPY(f, 5, sv_alloc_t, sv_push_i32, SV_I32, X_CMPF, r, s);
	TEST_SV_CPY(g, 6, sv_alloc_t, sv_push_u32, SV_U32, X_CMPF, r, s);
	TEST_SV_CPY(h, 7, sv_alloc_t, sv_push_i64, SV_I64, X_CMPF, r, s);
	TEST_SV_CPY(i, 8, sv_alloc_t, sv_push_u64, SV_U64, X_CMPF, r, s);
	TEST_SV_CPY(j, 9, sv_alloc_t, sv_push_f, SV_F, X_CMPF, r, s);
	TEST_SV_CPY(k, 10, sv_alloc_t, sv_push_d, SV_D, X_CMPF, r, s);
	sv_push_i8(&v1, 1);
	sv_push_i8(&v1, 2);
	sv_push_i8(&v1, 3);
	sv_push_i64(&v2, 1);
	sv_push_i64(&v2, 2);
	sv_push_i64(&v2, 3);
	sv_cpy(&v1, v2);
	sv_cpy(&v1a3, v2);
	sv_cpy(&v1a24, v2);
	res |= (sv_size(v1) == 3 ? 0 : 1 << 25);
	res |= (sv_size(v1a3) == 0 ? 0 : 1 << 26);
	res |= (sv_size(v1a24) == 3 ? 0 : 1 << 27);
	res |= (sv_at_i64(v1, 0) == 1 && sv_at_i64(v1, 1) == 2
				&& sv_at_i64(v1, 2) == 3
			? 0
			: 1 << 28);
	res |= (sv_at_i64(v1a24, 0) == 1 && sv_at_i64(v1a24, 1) == 2
				&& sv_at_i64(v1a24, 2) == 3
			? 0
			: 1 << 29);
#ifdef S_USE_VA_ARGS
	sv_free(&v1, &v2);
#else
	sv_free(&v1);
	sv_free(&v2);
#endif
	return res;
}

#define TEST_SV_CPY_ERASE_VARS(v) srt_vector *v, *v##2

#define TEST_SV_CPY_ERASE(v, ntest, alloc, push, check, type, CMPF, a, b)      \
	v = alloc(type, 0 CMPF);                                               \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	v##2 = NULL;                                                           \
	sv_cpy_erase(&v##2, v, 1, 5);                                          \
	res |= !v ? 1 << (ntest * 2) : (check) ? 0 : 2 << (ntest * 2);         \
	sv_free(&v);                                                           \
	sv_free(&v##2)

static int test_sv_cpy_erase()
{
	TEST_SV_CPY_ERASE_VARS(z);
	TEST_SV_CPY_ERASE_VARS(b);
	TEST_SV_CPY_ERASE_VARS(c);
	TEST_SV_CPY_ERASE_VARS(d);
	TEST_SV_CPY_ERASE_VARS(e);
	TEST_SV_CPY_ERASE_VARS(f);
	TEST_SV_CPY_ERASE_VARS(g);
	TEST_SV_CPY_ERASE_VARS(h);
	TEST_SV_CPY_ERASE_VARS(i);
	TEST_SV_CPY_ERASE_VARS(j);
	TEST_SV_CPY_ERASE_VARS(k);
	int res = 0;
	int r = 12, s = 34;
	TEST_SV_CPY_ERASE(z, 0, sv_alloc, sv_push,
			  ((const struct AA *)sv_at(z2, 0))->a
				  == ((const struct AA *)sv_at(z2, 1))->a,
			  sizeof(struct AA), NO_CMPF, &a1, &a2);
#define SVAT(v, at) at(v##2, 0) == at(v##2, 1)
	TEST_SV_CPY_ERASE(b, 1, sv_alloc_t, sv_push_i8, SVAT(b, sv_at_i8),
			  SV_I8, X_CMPF, r, s);
	TEST_SV_CPY_ERASE(c, 2, sv_alloc_t, sv_push_u8, SVAT(c, sv_at_u8),
			  SV_U8, X_CMPF, r, s);
	TEST_SV_CPY_ERASE(d, 3, sv_alloc_t, sv_push_i16, SVAT(d, sv_at_i16),
			  SV_I16, X_CMPF, r, s);
	TEST_SV_CPY_ERASE(e, 4, sv_alloc_t, sv_push_u16, SVAT(e, sv_at_u16),
			  SV_U16, X_CMPF, r, s);
	TEST_SV_CPY_ERASE(f, 5, sv_alloc_t, sv_push_i32, SVAT(f, sv_at_i32),
			  SV_I32, X_CMPF, r, s);
	TEST_SV_CPY_ERASE(g, 6, sv_alloc_t, sv_push_u32, SVAT(g, sv_at_u32),
			  SV_U32, X_CMPF, r, s);
	TEST_SV_CPY_ERASE(h, 7, sv_alloc_t, sv_push_i64, SVAT(h, sv_at_i64),
			  SV_I64, X_CMPF, r, s);
	TEST_SV_CPY_ERASE(i, 8, sv_alloc_t, sv_push_u64, SVAT(i, sv_at_u64),
			  SV_U64, X_CMPF, r, s);
	TEST_SV_CPY_ERASE(j, 9, sv_alloc_t, sv_push_f, SVAT(j, sv_at_f), SV_F,
			  X_CMPF, r, s);
	TEST_SV_CPY_ERASE(k, 10, sv_alloc_t, sv_push_d, SVAT(k, sv_at_d), SV_D,
			  X_CMPF, r, s);
#undef SVAT
	return res;
}

#define TEST_SV_CPY_RESIZE_VARS(v) srt_vector *v, *v##2
#define TEST_SV_CPY_RESIZE(v, ntest, alloc, push, type, CMPF, a, b)            \
	v = alloc(type, 0 CMPF);                                               \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	v##2 = NULL;                                                           \
	sv_cpy_resize(&v##2, v, 2);                                            \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : (sv_size(v##2) == 2 ? 0 : 2 << (ntest * 2))                \
			       | (!sv_ncmp(v, 0, v##2, 0, sv_size(v##2))       \
					  ? 0                                  \
					  : 4 << (ntest * 2))                  \
			       | (sv_ncmp(v, 0, v##2, 0, sv_size(v)) > 0       \
					  ? 0                                  \
					  : 8 << (ntest * 2))                  \
			       | (sv_ncmp(v##2, 0, v, 0, sv_size(v)) < 0       \
					  ? 0                                  \
					  : 16 << (ntest * 2));                \
	sv_free(&v);                                                           \
	sv_free(&v##2)

static int test_sv_cpy_resize()
{
	TEST_SV_CPY_RESIZE_VARS(z);
	TEST_SV_CPY_RESIZE_VARS(b);
	TEST_SV_CPY_RESIZE_VARS(c);
	TEST_SV_CPY_RESIZE_VARS(d);
	TEST_SV_CPY_RESIZE_VARS(e);
	TEST_SV_CPY_RESIZE_VARS(f);
	TEST_SV_CPY_RESIZE_VARS(g);
	TEST_SV_CPY_RESIZE_VARS(h);
	TEST_SV_CPY_RESIZE_VARS(i);
	TEST_SV_CPY_RESIZE_VARS(j);
	TEST_SV_CPY_RESIZE_VARS(k);
	int res = 0;
	int r = 12, s = 34;
	TEST_SV_CPY_RESIZE(z, 0, sv_alloc, sv_push, sizeof(struct AA), NO_CMPF,
			   &a1, &a2);
	TEST_SV_CPY_RESIZE(b, 1, sv_alloc_t, sv_push_i8, SV_I8, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(c, 2, sv_alloc_t, sv_push_u8, SV_U8, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(d, 3, sv_alloc_t, sv_push_i16, SV_I16, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(e, 4, sv_alloc_t, sv_push_u16, SV_U16, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(f, 5, sv_alloc_t, sv_push_i32, SV_I32, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(g, 6, sv_alloc_t, sv_push_u32, SV_U32, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(h, 7, sv_alloc_t, sv_push_i64, SV_I64, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(i, 8, sv_alloc_t, sv_push_u64, SV_U64, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(j, 9, sv_alloc_t, sv_push_f, SV_F, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(k, 10, sv_alloc_t, sv_push_d, SV_D, X_CMPF, r, s);
	return res;
}

#define TEST_SV_CAT_VARS(v) srt_vector *v, *v##2

#define TEST_SV_CAT(v, ntest, alloc, push, check, check2, type, CMPF, pushval) \
	v = alloc(type, 0 CMPF);                                               \
	push(&v, pushval);                                                     \
	v##2 = sv_dup(v);                                                      \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : (sv_cat(&v, v##2) && sv_cat(&v, v) && sv_len(v) == 4       \
		     && (check) && (check2))                                   \
			       ? 0                                             \
			       : 2 << (ntest * 2);                             \
	sv_free(&v);                                                           \
	sv_free(&v##2)

static int test_sv_cat()
{
	TEST_SV_CAT_VARS(z);
	TEST_SV_CAT_VARS(b);
	TEST_SV_CAT_VARS(c);
	TEST_SV_CAT_VARS(d);
	TEST_SV_CAT_VARS(e);
	TEST_SV_CAT_VARS(f);
	TEST_SV_CAT_VARS(g);
	TEST_SV_CAT_VARS(h);
	TEST_SV_CAT_VARS(i);
	TEST_SV_CAT_VARS(j);
	TEST_SV_CAT_VARS(k);
	int res = 0;
	int w = 123;
	TEST_SV_CAT(z, 0, sv_alloc, sv_push,
		    ((const struct AA *)sv_at(z, 3))->a == a1.a,
		    ((const struct AA *)sv_pop(z))->a
			    == ((const struct AA *)sv_pop(z2))->a,
		    sizeof(struct AA), NO_CMPF, &a1);
#define SVIAT(v, at) at(v, 3) == w
#define SVUAT(v, at) at(v, 3) == (unsigned)w
#define CHKP(v, pop) pop(v) == pop(v##2)
	TEST_SV_CAT(b, 1, sv_alloc_t, sv_push_i8, SVIAT(b, sv_at_i8),
		    CHKP(b, sv_pop_i8), SV_I8, X_CMPF, w);
	TEST_SV_CAT(c, 2, sv_alloc_t, sv_push_u8, SVUAT(c, sv_at_u8),
		    CHKP(c, sv_pop_u8), SV_U8, X_CMPF, w);
	TEST_SV_CAT(d, 3, sv_alloc_t, sv_push_i16, SVIAT(d, sv_at_i16),
		    CHKP(d, sv_pop_i16), SV_I16, X_CMPF, w);
	TEST_SV_CAT(e, 4, sv_alloc_t, sv_push_u16, SVUAT(e, sv_at_u16),
		    CHKP(e, sv_pop_u16), SV_U16, X_CMPF, w);
	TEST_SV_CAT(f, 5, sv_alloc_t, sv_push_i32, SVIAT(f, sv_at_i32),
		    CHKP(f, sv_pop_i32), SV_I32, X_CMPF, w);
	TEST_SV_CAT(g, 6, sv_alloc_t, sv_push_u32, SVUAT(g, sv_at_u32),
		    CHKP(g, sv_pop_u32), SV_U32, X_CMPF, w);
	TEST_SV_CAT(h, 7, sv_alloc_t, sv_push_i64, SVIAT(h, sv_at_i64),
		    CHKP(h, sv_pop_i64), SV_I64, X_CMPF, w);
	TEST_SV_CAT(i, 8, sv_alloc_t, sv_push_u64, SVUAT(i, sv_at_u64),
		    CHKP(i, sv_pop_u64), SV_U64, X_CMPF, w);
	TEST_SV_CAT(j, 9, sv_alloc_t, sv_push_f, SVUAT(j, sv_at_f),
		    CHKP(j, sv_pop_f), SV_F, X_CMPF, w);
	TEST_SV_CAT(k, 10, sv_alloc_t, sv_push_d, SVUAT(k, sv_at_d),
		    CHKP(k, sv_pop_d), SV_D, X_CMPF, w);
#undef SVIAT
#undef SVUAT
#undef CHKP
	return res;
}

#define TEST_SV_CAT_ERASE_VARS(v) srt_vector *v, *v##2

#define TEST_SV_CAT_ERASE(v, ntest, alloc, push, check, type, CMPF, a, b)      \
	v = alloc(type, 0 CMPF);                                               \
	v##2 = alloc(type, 0 CMPF);                                            \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v##2, b);                                                        \
	sv_cat_erase(&v##2, v, 1, 5);                                          \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : ((check) ? 0 : 2 << (ntest * 2))                           \
			       | (sv_size(v##2) == 6 ? 0 : 4 << (ntest * 2));  \
	sv_free(&v);                                                           \
	sv_free(&v##2)

static int test_sv_cat_erase()
{
	TEST_SV_CAT_ERASE_VARS(z);
	TEST_SV_CAT_ERASE_VARS(b);
	TEST_SV_CAT_ERASE_VARS(c);
	TEST_SV_CAT_ERASE_VARS(d);
	TEST_SV_CAT_ERASE_VARS(e);
	TEST_SV_CAT_ERASE_VARS(f);
	TEST_SV_CAT_ERASE_VARS(g);
	TEST_SV_CAT_ERASE_VARS(h);
	TEST_SV_CAT_ERASE_VARS(i);
	TEST_SV_CAT_ERASE_VARS(j);
	TEST_SV_CAT_ERASE_VARS(k);
	int res = 0;
	int r = 12, s = 34;
	TEST_SV_CAT_ERASE(z, 0, sv_alloc, sv_push,
			  ((const struct AA *)sv_at(z2, 0))->a
				  == ((const struct AA *)sv_at(z2, 2))->a,
			  sizeof(struct AA), NO_CMPF, &a1, &a2);
#define SVAT(v, at) at(v##2, 0) == at(v##2, 2)
	TEST_SV_CAT_ERASE(b, 1, sv_alloc_t, sv_push_i8, SVAT(b, sv_at_i8),
			  SV_I8, X_CMPF, r, s);
	TEST_SV_CAT_ERASE(c, 2, sv_alloc_t, sv_push_u8, SVAT(c, sv_at_u8),
			  SV_U8, X_CMPF, r, s);
	TEST_SV_CAT_ERASE(d, 3, sv_alloc_t, sv_push_i16, SVAT(d, sv_at_i16),
			  SV_I16, X_CMPF, r, s);
	TEST_SV_CAT_ERASE(e, 4, sv_alloc_t, sv_push_u16, SVAT(e, sv_at_u16),
			  SV_U16, X_CMPF, r, s);
	TEST_SV_CAT_ERASE(f, 5, sv_alloc_t, sv_push_i32, SVAT(f, sv_at_i32),
			  SV_I32, X_CMPF, r, s);
	TEST_SV_CAT_ERASE(g, 6, sv_alloc_t, sv_push_u32, SVAT(g, sv_at_u32),
			  SV_U32, X_CMPF, r, s);
	TEST_SV_CAT_ERASE(h, 7, sv_alloc_t, sv_push_i64, SVAT(h, sv_at_i64),
			  SV_I64, X_CMPF, r, s);
	TEST_SV_CAT_ERASE(i, 8, sv_alloc_t, sv_push_u64, SVAT(i, sv_at_u64),
			  SV_U64, X_CMPF, r, s);
	TEST_SV_CAT_ERASE(j, 9, sv_alloc_t, sv_push_f, SVAT(j, sv_at_f), SV_F,
			  X_CMPF, r, s);
	TEST_SV_CAT_ERASE(k, 10, sv_alloc_t, sv_push_d, SVAT(k, sv_at_d), SV_D,
			  X_CMPF, r, s);
#undef SVAT
	return res;
}

#define TEST_SV_CAT_RESIZE_VARS(v) srt_vector *v, *v##2
#define TEST_SV_CAT_RESIZE(v, ntest, alloc, push, type, CMPF, a, b)            \
	v = alloc(type, 0 CMPF);                                               \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	v##2 = NULL;                                                           \
	sv_cat_resize(&v##2, v, 2);                                            \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : (sv_size(v##2) == 2 ? 0 : 2 << (ntest * 2))                \
			       | (!sv_ncmp(v, 0, v##2, 0, sv_size(v##2))       \
					  ? 0                                  \
					  : 4 << (ntest * 2))                  \
			       | (sv_ncmp(v, 0, v##2, 0, sv_size(v)) > 0       \
					  ? 0                                  \
					  : 8 << (ntest * 2))                  \
			       | (sv_ncmp(v##2, 0, v, 0, sv_size(v)) < 0       \
					  ? 0                                  \
					  : 16 << (ntest * 2));                \
	sv_free(&v);                                                           \
	sv_free(&v##2)

static int test_sv_cat_resize()
{
	TEST_SV_CAT_RESIZE_VARS(z);
	TEST_SV_CAT_RESIZE_VARS(b);
	TEST_SV_CAT_RESIZE_VARS(c);
	TEST_SV_CAT_RESIZE_VARS(d);
	TEST_SV_CAT_RESIZE_VARS(e);
	TEST_SV_CAT_RESIZE_VARS(f);
	TEST_SV_CAT_RESIZE_VARS(g);
	TEST_SV_CAT_RESIZE_VARS(h);
	TEST_SV_CAT_RESIZE_VARS(i);
	TEST_SV_CAT_RESIZE_VARS(j);
	TEST_SV_CAT_RESIZE_VARS(k);
	int res = 0;
	int r = 12, s = 34;
	TEST_SV_CAT_RESIZE(z, 0, sv_alloc, sv_push, sizeof(struct AA), NO_CMPF,
			   &a1, &a2);
	TEST_SV_CAT_RESIZE(b, 1, sv_alloc_t, sv_push_i8, SV_I8, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(c, 2, sv_alloc_t, sv_push_u8, SV_U8, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(d, 3, sv_alloc_t, sv_push_i16, SV_I16, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(e, 4, sv_alloc_t, sv_push_u16, SV_U16, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(f, 5, sv_alloc_t, sv_push_i32, SV_I32, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(g, 6, sv_alloc_t, sv_push_u32, SV_U32, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(h, 7, sv_alloc_t, sv_push_i64, SV_I64, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(i, 8, sv_alloc_t, sv_push_u64, SV_U64, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(j, 9, sv_alloc_t, sv_push_f, SV_F, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(k, 10, sv_alloc_t, sv_push_d, SV_D, X_CMPF, r, s);
	return res;
}

#define TEST_SV_ERASE_VARS(v) srt_vector *v

#define TEST_SV_ERASE(v, ntest, alloc, push, check, type, CMPF, a, b)          \
	v = alloc(type, 0 CMPF);                                               \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	sv_erase(&v, 1, 5);                                                    \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : ((check) ? 0 : 2 << (ntest * 2))                           \
			       | (sv_size(v) == 5 ? 0 : 4 << (ntest * 2));     \
	sv_free(&v);

static int test_sv_erase()
{
	TEST_SV_ERASE_VARS(z);
	TEST_SV_ERASE_VARS(b);
	TEST_SV_ERASE_VARS(c);
	TEST_SV_ERASE_VARS(d);
	TEST_SV_ERASE_VARS(e);
	TEST_SV_ERASE_VARS(f);
	TEST_SV_ERASE_VARS(g);
	TEST_SV_ERASE_VARS(h);
	TEST_SV_ERASE_VARS(i);
	TEST_SV_ERASE_VARS(j);
	TEST_SV_ERASE_VARS(k);
	int res = 0;
	int r = 12, s = 34;
	TEST_SV_ERASE(z, 0, sv_alloc, sv_push,
		      ((const struct AA *)sv_at(z, 0))->a
			      == ((const struct AA *)sv_at(z, 1))->a,
		      sizeof(struct AA), NO_CMPF, &a1, &a2);
#define SVAT(v, at) at(v, 0) == at(v, 1)
	TEST_SV_ERASE(b, 1, sv_alloc_t, sv_push_i8, SVAT(b, sv_at_i8), SV_I8,
		      X_CMPF, r, s);
	TEST_SV_ERASE(c, 2, sv_alloc_t, sv_push_u8, SVAT(c, sv_at_u8), SV_U8,
		      X_CMPF, r, s);
	TEST_SV_ERASE(d, 3, sv_alloc_t, sv_push_i16, SVAT(d, sv_at_i16), SV_I16,
		      X_CMPF, r, s);
	TEST_SV_ERASE(e, 4, sv_alloc_t, sv_push_u16, SVAT(e, sv_at_u16), SV_U16,
		      X_CMPF, r, s);
	TEST_SV_ERASE(f, 5, sv_alloc_t, sv_push_i32, SVAT(f, sv_at_i32), SV_I32,
		      X_CMPF, r, s);
	TEST_SV_ERASE(g, 6, sv_alloc_t, sv_push_u32, SVAT(g, sv_at_u32), SV_U32,
		      X_CMPF, r, s);
	TEST_SV_ERASE(h, 7, sv_alloc_t, sv_push_i64, SVAT(h, sv_at_i64), SV_I64,
		      X_CMPF, r, s);
	TEST_SV_ERASE(i, 8, sv_alloc_t, sv_push_u64, SVAT(i, sv_at_u64), SV_U64,
		      X_CMPF, r, s);
	TEST_SV_ERASE(j, 9, sv_alloc_t, sv_push_f, SVAT(j, sv_at_f), SV_F,
		      X_CMPF, r, s);
	TEST_SV_ERASE(k, 10, sv_alloc_t, sv_push_d, SVAT(k, sv_at_d), SV_D,
		      X_CMPF, r, s);
#undef SVAT
	return res;
}

#define TEST_SV_RESIZE_VARS(v) srt_vector *v

#define TEST_SV_RESIZE(v, ntest, alloc, push, check, type, CMPF, a, b)         \
	v = alloc(type, 0 CMPF);                                               \
	push(&v, b);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	sv_resize(&v, 5);                                                      \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : ((check) ? 0 : 2 << (ntest * 2))                           \
			       | (sv_size(v) == 5 ? 0 : 4 << (ntest * 2));     \
	sv_clear(v);                                                           \
	res |= (!sv_size(v) ? 0 : 4 << (ntest * 2));                           \
	sv_free(&v);

static int test_sv_resize()
{
	TEST_SV_RESIZE_VARS(z);
	TEST_SV_RESIZE_VARS(b);
	TEST_SV_RESIZE_VARS(c);
	TEST_SV_RESIZE_VARS(d);
	TEST_SV_RESIZE_VARS(e);
	TEST_SV_RESIZE_VARS(f);
	TEST_SV_RESIZE_VARS(g);
	TEST_SV_RESIZE_VARS(h);
	TEST_SV_RESIZE_VARS(i);
	TEST_SV_RESIZE_VARS(j);
	TEST_SV_RESIZE_VARS(k);
	int res = 0;
	int r = 12, s = 34;
	TEST_SV_RESIZE(z, 0, sv_alloc, sv_push,
		       ((const struct AA *)sv_at(z, 0))->a
			       == ((const struct AA *)sv_at(z, 1))->a,
		       sizeof(struct AA), NO_CMPF, &a1, &a2);
#define SVAT(v, at) at(v, 0) == at(v, 1)
	TEST_SV_RESIZE(b, 1, sv_alloc_t, sv_push_i8, SVAT(b, sv_at_i8), SV_I8,
		       X_CMPF, r, s);
	TEST_SV_RESIZE(c, 2, sv_alloc_t, sv_push_u8, SVAT(c, sv_at_u8), SV_U8,
		       X_CMPF, r, s);
	TEST_SV_RESIZE(d, 3, sv_alloc_t, sv_push_i16, SVAT(d, sv_at_i16),
		       SV_I16, X_CMPF, r, s);
	TEST_SV_RESIZE(e, 4, sv_alloc_t, sv_push_u16, SVAT(e, sv_at_u16),
		       SV_U16, X_CMPF, r, s);
	TEST_SV_RESIZE(f, 5, sv_alloc_t, sv_push_i32, SVAT(f, sv_at_i32),
		       SV_I32, X_CMPF, r, s);
	TEST_SV_RESIZE(g, 6, sv_alloc_t, sv_push_u32, SVAT(g, sv_at_u32),
		       SV_U32, X_CMPF, r, s);
	TEST_SV_RESIZE(h, 7, sv_alloc_t, sv_push_i64, SVAT(h, sv_at_i64),
		       SV_I64, X_CMPF, r, s);
	TEST_SV_RESIZE(i, 8, sv_alloc_t, sv_push_u64, SVAT(i, sv_at_u64),
		       SV_U64, X_CMPF, r, s);
	TEST_SV_RESIZE(j, 9, sv_alloc_t, sv_push_f, SVAT(j, sv_at_f), SV_F,
		       X_CMPF, r, s);
	TEST_SV_RESIZE(k, 10, sv_alloc_t, sv_push_d, SVAT(k, sv_at_d), SV_D,
		       X_CMPF, r, s);
#undef SVAT
	return res;
}

#define TEST_SV_SORT_VARS(v)                                                   \
	srt_vector *v;                                                         \
	int v##i, v##r

#define TEST_SV_SORT(v, ntest, alloc, push, type, CMPF, a, b, c)               \
	v = alloc(type, 0 CMPF);                                               \
	push(&v, c);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, c);                                                           \
	push(&v, c);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	sv_sort(v);                                                            \
	v##r = 0;                                                              \
	for (v##i = 1; v##i < 9 && v##r <= 0; v##i++)                          \
		v##r = sv_cmp(v, (size_t)(v##i - 1), (size_t)v##i);            \
	res |= !v ? 1 << (ntest * 2) : v##r <= 0 ? 0 : 2 << (ntest * 2);       \
	sv_free(&v);

#define BUILD_CMPF(FN, T)                                                      \
	static int FN(const void *a, const void *b)                            \
	{                                                                      \
		return *(const T *)(a) < *(const T *)(b)                       \
			       ? -1                                            \
			       : *(const T *)(a) == *(const T *)(b) ? 0 : 1;   \
	}

BUILD_CMPF(cmp8i, int8_t)
BUILD_CMPF(cmp8u, uint8_t)
BUILD_CMPF(cmp16i, int16_t)
BUILD_CMPF(cmp16u, uint16_t)
BUILD_CMPF(cmp32i, int32_t)
BUILD_CMPF(cmp32u, uint32_t)
BUILD_CMPF(cmp64i, int64_t)
BUILD_CMPF(cmp64u, uint64_t)
BUILD_CMPF(cmpf, float)
BUILD_CMPF(cmpd, double)

static int test_sv_sort()
{
	TEST_SV_SORT_VARS(z);
	TEST_SV_SORT_VARS(b);
	TEST_SV_SORT_VARS(c);
	TEST_SV_SORT_VARS(d);
	TEST_SV_SORT_VARS(e);
	TEST_SV_SORT_VARS(f);
	TEST_SV_SORT_VARS(g);
	TEST_SV_SORT_VARS(h);
	TEST_SV_SORT_VARS(j);
	TEST_SV_SORT_VARS(k);
	TEST_SV_SORT_VARS(l);
	int res = 0;
	int r = 12, s = 34, t = -1;
	unsigned tu = 11;
	srt_vector *v8i = sv_alloc_t(SV_I8, 0), *v8u = sv_alloc_t(SV_U8, 0),
		   *v16i = sv_alloc_t(SV_I16, 0), *v16u = sv_alloc_t(SV_U16, 0),
		   *v32i = sv_alloc_t(SV_I32, 0), *v32u = sv_alloc_t(SV_U32, 0),
		   *v64i = sv_alloc_t(SV_I64, 0), *v64u = sv_alloc_t(SV_U64, 0),
		   *vf = sv_alloc_t(SV_F, 0), *vd = sv_alloc_t(SV_D, 0);
	int i, nelems = 1000;
	int8_t *r8i = (int8_t *)s_malloc(nelems * sizeof(int8_t));
	uint8_t *r8u = (uint8_t *)s_malloc(nelems * sizeof(uint8_t));
	int16_t *r16i = (int16_t *)s_malloc(nelems * sizeof(int16_t));
	uint16_t *r16u = (uint16_t *)s_malloc(nelems * sizeof(uint16_t));
	int32_t *r32i = (int32_t *)s_malloc(nelems * sizeof(int32_t));
	uint32_t *r32u = (uint32_t *)s_malloc(nelems * sizeof(uint32_t));
	int64_t *r64i = (int64_t *)s_malloc(nelems * sizeof(int64_t));
	uint64_t *r64u = (uint64_t *)s_malloc(nelems * sizeof(uint64_t));
	float *rf = (float *)s_malloc(nelems * sizeof(float));
	double *rd = (double *)s_malloc(nelems * sizeof(double));
	/*
	 * Generic sort tests
	 */
	TEST_SV_SORT(z, 0, sv_alloc, sv_push, sizeof(struct AA), AA_CMPF, &a1,
		     &a2, &a1);
	TEST_SV_SORT(b, 1, sv_alloc_t, sv_push_i8, SV_I8, X_CMPF, r, s, t);
	TEST_SV_SORT(c, 2, sv_alloc_t, sv_push_u8, SV_U8, X_CMPF, r, s, tu);
	TEST_SV_SORT(d, 3, sv_alloc_t, sv_push_i16, SV_I16, X_CMPF, r, s, t);
	TEST_SV_SORT(e, 4, sv_alloc_t, sv_push_u16, SV_U16, X_CMPF, r, s, tu);
	TEST_SV_SORT(f, 5, sv_alloc_t, sv_push_i32, SV_I32, X_CMPF, r, s, t);
	TEST_SV_SORT(g, 6, sv_alloc_t, sv_push_u32, SV_U32, X_CMPF, r, s, tu);
	TEST_SV_SORT(h, 7, sv_alloc_t, sv_push_i64, SV_I64, X_CMPF, r, s, t);
	TEST_SV_SORT(j, 8, sv_alloc_t, sv_push_u64, SV_U64, X_CMPF, r, s, tu);
	TEST_SV_SORT(k, 9, sv_alloc_t, sv_push_f, SV_F, X_CMPF, r, s, tu);
	TEST_SV_SORT(l, 10, sv_alloc_t, sv_push_d, SV_D, X_CMPF, r, s, tu);
	/*
	 * Integer-specific sort tests
	 */
	if (!r8i || !r8u || !r16i || !r16u || !r32i || !r32u || !r64i || !r64u
	    || !rf || !rd)
		res = -1;
	else {
		for (i = 0; i < nelems; i += 2) {
			sv_push_i8(&v8i, i & 0xff);
			sv_push_u8(&v8u, i & 0xff);
			sv_push_i16(&v16i, i);
			sv_push_u16(&v16u, i);
			sv_push_i32(&v32i, i);
			sv_push_u32(&v32u, i);
			sv_push_i64(&v64i, i);
			sv_push_u64(&v64u, i);
			sv_push_f(&vf, (float)i);
			sv_push_d(&vd, (double)i);
			r8i[i] = i & 0xff;
			r8u[i] = i & 0xff;
			r16i[i] = i;
			r16u[i] = i;
			r32i[i] = i;
			r32u[i] = i;
			r64i[i] = i;
			r64u[i] = i;
			rf[i] = (float)i;
			rd[i] = (double)i;
		}
		for (i = 1; i < nelems; i += 2) {
			sv_push_i8(&v8i, i & 0xff);
			sv_push_u8(&v8u, i & 0xff);
			sv_push_i16(&v16i, i);
			sv_push_u16(&v16u, i);
			sv_push_i32(&v32i, i);
			sv_push_u32(&v32u, i);
			sv_push_i64(&v64i, i);
			sv_push_u64(&v64u, i);
			sv_push_f(&vf, (float)i);
			sv_push_d(&vd, (double)i);
			r8i[i] = i & 0xff;
			r8u[i] = i & 0xff;
			r16i[i] = i;
			r16u[i] = i;
			r32i[i] = i;
			r32u[i] = i;
			r64i[i] = i;
			r64u[i] = i;
			rf[i] = (float)i;
			rd[i] = (double)i;
		}
		sv_sort(v8i);
		sv_sort(v8u);
		sv_sort(v16i);
		sv_sort(v16u);
		sv_sort(v32i);
		sv_sort(v32u);
		sv_sort(v64i);
		sv_sort(v64u);
		sv_sort(vf);
		sv_sort(vd);
		qsort(r8i, nelems, 1, cmp8i);
		qsort(r8u, nelems, 1, cmp8u);
		qsort(r16i, nelems, 2, cmp16i);
		qsort(r16u, nelems, 2, cmp16u);
		qsort(r32i, nelems, 4, cmp32i);
		qsort(r32u, nelems, 4, cmp32u);
		qsort(r64i, nelems, 8, cmp64i);
		qsort(r64u, nelems, 8, cmp64u);
		qsort(rf, nelems, sizeof(float), cmpf);
		qsort(rd, nelems, sizeof(double), cmpd);
		for (i = 0; i < nelems; i++) {
			res |= sv_at_i8(v8i, i) == r8i[i]
					       && sv_at_u8(v8u, i) == r8u[i]
					       && (int)sv_at_i16(v16i, i) == i
					       && (int)sv_at_u16(v16u, i) == i
					       && (int)sv_at_i32(v32i, i) == i
					       && (int)sv_at_u32(v32u, i) == i
					       && (int)sv_at_i64(v64i, i) == i
					       && (int)sv_at_u64(v64u, i) == i
					       && (int)sv_at_f(vf, i) == i
					       && (int)sv_at_d(vd, i) == i
				       ? 0
				       : 1 << 30;
		}
	}
#ifdef S_USE_VA_ARGS
	sv_free(&v8i, &v8u, &v16i, &v16u, &v32i, &v32u, &v64i, &v64u, &vf, &vd);
#else
	sv_free(&v8i);
	sv_free(&v8u);
	sv_free(&v16i);
	sv_free(&v16u);
	sv_free(&v32i);
	sv_free(&v32u);
	sv_free(&v64i);
	sv_free(&v64u);
	sv_free(&vf);
	sv_free(&vd);
#endif
	s_free(r8i);
	s_free(r8u);
	s_free(r16i);
	s_free(r16u);
	s_free(r32i);
	s_free(r32u);
	s_free(r64i);
	s_free(r64u);
	s_free(rf);
	s_free(rd);
	return res;
}

#define TEST_SV_FIND_VARS(v) srt_vector *v

#define TEST_SV_FIND(v, ntest, alloc, push, check, type, CMPF, a, b)           \
	v = alloc(type, 0 CMPF);                                               \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, b);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	push(&v, a);                                                           \
	res |= !v ? 1 << (ntest * 2)                                           \
		  : ((check) ? 0 : 2 << (ntest * 2))                           \
			       | (sv_size(v) == 10 ? 0 : 4 << (ntest * 2));    \
	sv_free(&v);

static int test_sv_find()
{
	TEST_SV_FIND_VARS(z);
	TEST_SV_FIND_VARS(b);
	TEST_SV_FIND_VARS(c);
	TEST_SV_FIND_VARS(d);
	TEST_SV_FIND_VARS(e);
	TEST_SV_FIND_VARS(f);
	TEST_SV_FIND_VARS(g);
	TEST_SV_FIND_VARS(h);
	TEST_SV_FIND_VARS(i);
	TEST_SV_FIND_VARS(j);
	TEST_SV_FIND_VARS(k);
	int res = 0;
	int r = 12, s = 34;
	TEST_SV_FIND(z, 0, sv_alloc, sv_push, sv_find(z, 0, &a2) == 6,
		     sizeof(struct AA), NO_CMPF, &a1, &a2);
#define SVF(v, f) f(v, 0, s) == 6
	TEST_SV_FIND(b, 1, sv_alloc_t, sv_push_i8, SVF(b, sv_find_i8), SV_I8,
		     X_CMPF, r, s);
	TEST_SV_FIND(c, 2, sv_alloc_t, sv_push_u8, SVF(c, sv_find_u8), SV_U8,
		     X_CMPF, r, s);
	TEST_SV_FIND(d, 3, sv_alloc_t, sv_push_i16, SVF(d, sv_find_i16), SV_I16,
		     X_CMPF, r, s);
	TEST_SV_FIND(e, 4, sv_alloc_t, sv_push_u16, SVF(e, sv_find_u16), SV_U16,
		     X_CMPF, r, s);
	TEST_SV_FIND(f, 5, sv_alloc_t, sv_push_i32, SVF(f, sv_find_i32), SV_I32,
		     X_CMPF, r, s);
	TEST_SV_FIND(g, 6, sv_alloc_t, sv_push_u32, SVF(g, sv_find_u32), SV_U32,
		     X_CMPF, r, s);
	TEST_SV_FIND(h, 7, sv_alloc_t, sv_push_i64, SVF(h, sv_find_i64), SV_I64,
		     X_CMPF, r, s);
	TEST_SV_FIND(i, 8, sv_alloc_t, sv_push_u64, SVF(i, sv_find_u64), SV_U64,
		     X_CMPF, r, s);
	TEST_SV_FIND(j, 9, sv_alloc_t, sv_push_f, SVF(j, sv_find_f), SV_F,
		     X_CMPF, r, s);
	TEST_SV_FIND(k, 10, sv_alloc_t, sv_push_d, SVF(k, sv_find_d), SV_D,
		     X_CMPF, r, s);
#undef SVF
	return res;
}

static int test_sv_push_pop_set()
{
	size_t as = 10;
	srt_vector *a = sv_alloc(sizeof(struct AA), as, NULL),
		   *b = sv_alloca(sizeof(struct AA), as, NULL);
	const struct AA *t = NULL;
	int res = (!a || !b) ? 1 : 0;
	if (!res) {
#ifdef S_USE_VA_ARGS
		res |= (sv_push(&a, &a2, &a2, &a1) ? 0 : 4);
#else
		res |= (sv_push(&a, &a2) && sv_push(&a, &a2) && sv_push(&a, &a1)
				? 0
				: 4);
#endif
		res |= (sv_len(a) == 3 ? 0 : 8);
		res |= (((t = (const struct AA *)sv_pop(a)) && t->a == a1.a
			 && t->b == a1.b)
				? 0
				: 16);
		res |= (sv_len(a) == 2 ? 0 : 32);
		res |= (sv_push(&b, &a2) ? 0 : 64);
		res |= (sv_len(b) == 1 ? 0 : 128);
		res |= (((t = (struct AA *)sv_pop(b)) && t->a == a2.a
			 && t->b == a2.b)
				? 0
				: 256);
		res |= (sv_len(b) == 0 ? 0 : 512);
		sv_set(&a, 0, &a2);
		sv_set(&a, 1000, &a1);
		sv_set(&b, 0, &a2);
		sv_set(&b, as - 1, &a1);
		t = (const struct AA *)sv_at(a, 0);
		res |= t->a == a2.a ? 0 : 1024;
		t = (const struct AA *)sv_at(a, 1000);
		res |= t->a == a1.a ? 0 : 2048;
		t = (const struct AA *)sv_at(b, 0);
		res |= t->a == a2.a ? 0 : 4096;
		t = (const struct AA *)sv_at(b, as - 1);
		res |= t->a == a1.a ? 0 : 8192;
	}
	sv_free(&a);
	return res;
}

#define BUILD_TEST_SV_PUSH_POP_SET_X(FN, T, TID, PUSH, POP, SET, AT)           \
	static int FN()                                                        \
	{                                                                      \
		size_t as = 10;                                                \
		int i = 0, res = 0;                                            \
		srt_vector *a = sv_alloc_t(TID, as);                           \
		srt_vector *b = sv_alloca_t(TID, as);                          \
		do {                                                           \
			if (!a || !b) {                                        \
				res |= 1 << (i * 4);                           \
				break;                                         \
			}                                                      \
			if (!PUSH(&a, (T)i) || !PUSH(&b, (T)i)) {              \
				res |= 2 << (i * 4);                           \
				break;                                         \
			}                                                      \
			if (POP(a) != (T)i || POP(b) != (T)i) {                \
				res |= 3 << (i * 4);                           \
				break;                                         \
			}                                                      \
			if (sv_len(a) != 0 || sv_len(b) != 0) {                \
				res |= 4 << (i * 4);                           \
				break;                                         \
			}                                                      \
			SET(&a, 0, (T)-2);                                     \
			SET(&a, 1000, (T)-1);                                  \
			SET(&b, 0, (T)-2);                                     \
			SET(&b, as - 1, (T)-1);                                \
			res |= AT(a, 0) == (T)-2 ? 0 : -1;                     \
			res |= AT(a, 1000) == (T)-1 ? 0 : -1;                  \
			res |= AT(b, 0) == (T)-2 ? 0 : -1;                     \
			res |= AT(b, as - 1) == (T)-1 ? 0 : -1;                \
		} while (0);                                                   \
		sv_free(&a);                                                   \
		return res;                                                    \
	}

BUILD_TEST_SV_PUSH_POP_SET_X(test_sv_push_pop_set_u8, uint8_t, SV_U8,
			     sv_push_u8, sv_pop_u8, sv_set_u8, sv_at_u8)
BUILD_TEST_SV_PUSH_POP_SET_X(test_sv_push_pop_set_i8, int8_t, SV_I8, sv_push_i8,
			     sv_pop_i8, sv_set_i8, sv_at_i8)
BUILD_TEST_SV_PUSH_POP_SET_X(test_sv_push_pop_set_u16, uint16_t, SV_U16,
			     sv_push_u16, sv_pop_u16, sv_set_u16, sv_at_u16)
BUILD_TEST_SV_PUSH_POP_SET_X(test_sv_push_pop_set_i16, int16_t, SV_I16,
			     sv_push_i16, sv_pop_i16, sv_set_i16, sv_at_i16)
BUILD_TEST_SV_PUSH_POP_SET_X(test_sv_push_pop_set_u32, uint32_t, SV_U32,
			     sv_push_u32, sv_pop_u32, sv_set_u32, sv_at_u32)
BUILD_TEST_SV_PUSH_POP_SET_X(test_sv_push_pop_set_i32, int32_t, SV_I32,
			     sv_push_i32, sv_pop_i32, sv_set_i32, sv_at_i32)
BUILD_TEST_SV_PUSH_POP_SET_X(test_sv_push_pop_set_u64, uint64_t, SV_U64,
			     sv_push_u64, sv_pop_u64, sv_set_u64, sv_at_u64)
BUILD_TEST_SV_PUSH_POP_SET_X(test_sv_push_pop_set_i64, int64_t, SV_I64,
			     sv_push_i64, sv_pop_i64, sv_set_i64, sv_at_i64)
BUILD_TEST_SV_PUSH_POP_SET_X(test_sv_push_pop_set_f, float, SV_F, sv_push_f,
			     sv_pop_f, sv_set_f, sv_at_f)
BUILD_TEST_SV_PUSH_POP_SET_X(test_sv_push_pop_set_d, float, SV_D, sv_push_d,
			     sv_pop_d, sv_set_d, sv_at_d)

#define TEST_SV_PUSH_RAW_VARS(v, T)                                            \
	srt_vector *v;                                                         \
	const T *v##b;                                                         \
	int v##i, v##r

#define TEST_SV_PUSH_RAW(v, ntest, alloc, type, CMPF, T, TEST, vbuf, ve)       \
	v = alloc(type, 0 CMPF);                                               \
	sv_push_raw(&v, vbuf, ve);                                             \
	v##b = (const T *)sv_get_buffer_r(v);                                  \
	v##r = 0;                                                              \
	for (v##i = 0; v##i < ve && v##r == 0; v##i++)                         \
		v##r = TEST(vbuf[v##i], v##b[v##i]) ? 0 : 1;                   \
	res |= !v ? 1 << (ntest * 2) : v##r <= 0 ? 0 : 2 << (ntest * 2);       \
	sv_free(&v);

static int test_sv_push_raw()
{
	TEST_SV_PUSH_RAW_VARS(z, struct AA);
	TEST_SV_PUSH_RAW_VARS(b, int8_t);
	TEST_SV_PUSH_RAW_VARS(c, uint8_t);
	TEST_SV_PUSH_RAW_VARS(d, int16_t);
	TEST_SV_PUSH_RAW_VARS(e, uint16_t);
	TEST_SV_PUSH_RAW_VARS(f, int32_t);
	TEST_SV_PUSH_RAW_VARS(g, uint32_t);
	TEST_SV_PUSH_RAW_VARS(h, int64_t);
	TEST_SV_PUSH_RAW_VARS(i, uint64_t);
	TEST_SV_PUSH_RAW_VARS(j, float);
	TEST_SV_PUSH_RAW_VARS(k, double);
	int res = 0;
	TEST_SV_PUSH_RAW(z, 0, sv_alloc, sizeof(struct AA), NO_CMPF, struct AA,
			 TEST_AA, av1, 3);
	TEST_SV_PUSH_RAW(b, 1, sv_alloc_t, SV_I8, X_CMPF, int8_t, TEST_NUM, i8v,
			 3);
	TEST_SV_PUSH_RAW(c, 2, sv_alloc_t, SV_U8, X_CMPF, uint8_t, TEST_NUM,
			 u8v, 3);
	TEST_SV_PUSH_RAW(d, 3, sv_alloc_t, SV_I16, X_CMPF, int16_t, TEST_NUM,
			 i16v, 3);
	TEST_SV_PUSH_RAW(e, 4, sv_alloc_t, SV_U16, X_CMPF, uint16_t, TEST_NUM,
			 u16v, 3);
	TEST_SV_PUSH_RAW(f, 5, sv_alloc_t, SV_I32, X_CMPF, int32_t, TEST_NUM,
			 i32v, 3);
	TEST_SV_PUSH_RAW(g, 6, sv_alloc_t, SV_U32, X_CMPF, uint32_t, TEST_NUM,
			 u32v, 3);
	TEST_SV_PUSH_RAW(h, 7, sv_alloc_t, SV_I64, X_CMPF, int64_t, TEST_NUM,
			 i64v, 3);
	TEST_SV_PUSH_RAW(i, 8, sv_alloc_t, SV_U64, X_CMPF, uint64_t, TEST_NUM,
			 u64v, 3);
	TEST_SV_PUSH_RAW(j, 9, sv_alloc_t, SV_F, X_CMPF, float, TEST_NUM, fv,
			 3);
	TEST_SV_PUSH_RAW(k, 10, sv_alloc_t, SV_D, X_CMPF, double, TEST_NUM, dv,
			 3);
	return res;
}

struct MyNode1 {
	struct S_Node n;
	int k;
	int v;
};

static int cmp1(const struct MyNode1 *a, const struct MyNode1 *b)
{
	return a->k - b->k;
}

#ifdef S_EXTRA_TREE_TEST_DEBUG
static void ndx2s(char *out, size_t out_max, const srt_tndx id)
{
	if (id == ST_NIL)
		strcpy(out, "nil");
	else
		snprintf(out, out_max, "%u", (unsigned)id);
}

static srt_string *ss_cat_stn_MyNode1(srt_string **s, const srt_tnode *n,
				      const srt_tndx id)
{
	ASSERT_RETURN_IF(!s, NULL);
	struct MyNode1 *node = (struct MyNode1 *)n;
	char l[128], r[128];
	ndx2s(l, sizeof(l), n->x.l);
	ndx2s(r, sizeof(r), n->r);
	ss_cat_printf(s, 128, "[%u: (%i, %c) -> (%s, %s; r:%u)]", (unsigned)id,
		      node->k, node->v, l, r, n->x.is_red);
	return *s;
}
#endif

static int test_st_alloc()
{
	srt_tree *t = st_alloc((srt_cmp)cmp1, sizeof(struct MyNode1), 1000);
	struct MyNode1 n = {EMPTY_STN, 0, 0};
	int res = !t ? 1
		     : st_len(t) != 0 ? 2
				      : (!st_insert(&t, (const srt_tnode *)&n)
					 || st_len(t) != 1)
						? 4
						: 0;
	st_free(&t);
	return res;
}

#define ST_ENV_TEST_AUX                                                        \
	srt_tree *t = st_alloc((srt_cmp)cmp1, sizeof(struct MyNode1), 1000);   \
	srt_string *log = NULL;                                                \
	struct MyNode1 n0 = {EMPTY_STN, 0, 0};                                 \
	srt_tnode *n = (srt_tnode *)&n0;                                       \
	srt_bool r = S_FALSE;                                                  \
	if (!t)                                                                \
		return 1;

#define ST_ENV_TEST_AUX_LEAVE                                                  \
	st_free(&t);                                                           \
	ss_free(&log);                                                         \
	if (!r)                                                                \
		res |= 0x40000000;
/* #define S_EXTRA_TREE_TEST_DEBUG */
#ifdef S_EXTRA_TREE_TEST_DEBUG
#define STLOGMYT(t) st_log_obj(&log, t, ss_cat_stn_MyNode1)
#else
#define STLOGMYT(t)
#endif
#define ST_A_INS(key, val)                                                     \
	{                                                                      \
		n0.k = key;                                                    \
		n0.v = val;                                                    \
		r = st_insert(&t, n);                                          \
		STLOGMYT(t);                                                   \
	}
#define ST_A_DEL(key)                                                          \
	{                                                                      \
		n0.k = key;                                                    \
		r = st_delete(t, n, NULL);                                     \
		STLOGMYT(t);                                                   \
	}
#define ST_CHK_BRK(t, len, err)                                                \
	if (st_len(t) != (size_t)(len)) {                                      \
		res = err;                                                     \
		break;                                                         \
	}

static int test_st_insert_del()
{
	int res = 0;
	const struct MyNode1 *nr;
	int i = 0;
	int tree_elems = 90;
	int cbase = ' ';
	ST_ENV_TEST_AUX;
	for (;;) {
		/* Case 1: add in order, delete in order */
		for (i = 0; i < (int)tree_elems; i++)
			ST_A_INS(i, cbase + i);
		ST_CHK_BRK(t, tree_elems, 1);
		for (i = 0; i < ((int)tree_elems - 1); i++)
			ST_A_DEL(i);
		ST_CHK_BRK(t, 1, 2);
		n0.k = tree_elems - 1;
		nr = (const struct MyNode1 *)st_locate(t, &n0.n);
		if (!nr || nr->v != (cbase + tree_elems - 1)
		    || st_traverse_levelorder(t, NULL, NULL) != 1) {
			res = 3;
			break;
		}
		ST_A_DEL(tree_elems - 1);
		ST_CHK_BRK(t, 0, 4);
		/* Case 2: add in reverse order, delete in order */
		for (i = tree_elems - 1; i >= 0; i--)
			ST_A_INS(i, cbase + i);
		ST_CHK_BRK(t, tree_elems, 4);
		for (i = 0; i < ((int)tree_elems - 1); i++)
			ST_A_DEL(i);
		ST_CHK_BRK(t, 1, 5);
		n0.k = tree_elems - 1;
		nr = (const struct MyNode1 *)st_locate(t, &n0.n);
		if (!nr || nr->v != (cbase + tree_elems - 1)
		    || st_traverse_levelorder(t, NULL, NULL) != 1) {
			res = 6;
			break;
		}
		ST_A_DEL(tree_elems - 1);
		ST_CHK_BRK(t, 0, tree_elems);
		/* Case 3: add in order, delete in reverse order */
		for (i = 0; i < (int)tree_elems; i++) /* ST_A_INS(2, 'c'); */
			ST_A_INS(i, cbase + i);       /* double rotation   */
		ST_CHK_BRK(t, tree_elems, 8);
		for (i = (tree_elems - 1); i > 0; i--)
			ST_A_DEL(i);
		ST_CHK_BRK(t, 1, 9);
		n0.k = 0;
		nr = (const struct MyNode1 *)st_locate(t, &n0.n);
		if (!nr || nr->v != cbase
		    || st_traverse_levelorder(t, NULL, NULL) != 1) {
			res = 10;
			break;
		}
		ST_A_DEL(0);
		ST_CHK_BRK(t, 0, 11);
		break;
	}
	ST_ENV_TEST_AUX_LEAVE;
	return res;
}

static int test_traverse(struct STraverseParams *tp)
{
	srt_string **log = (srt_string **)tp->context;
	const struct MyNode1 *node =
		(const struct MyNode1 *)get_node_r(tp->t, tp->c);
	if (node)
		ss_cat_printf(log, 512, "%c", node->v);
	return 0;
}

static int test_st_traverse()
{
	int res = 0;
	ssize_t levels = 0;
	const char *out;
	ST_ENV_TEST_AUX;
	for (;;) {
		ST_A_INS(6, 'g');
		ST_A_INS(7, 'h');
		ST_A_INS(8, 'i');
		ST_A_INS(3, 'd');
		ST_A_INS(4, 'e');
		ST_A_INS(5, 'f');
		ST_A_INS(0, 'a');
		ST_A_INS(1, 'b');
		ST_A_INS(2, 'c');
		ss_cpy_c(&log, "");
		levels = st_traverse_preorder(t, test_traverse, (void *)&log);
		out = ss_to_c(log);
		if (levels < 1 || strcmp(out, "ebadchgfi") != 0) {
			res |= 1;
			break;
		}
		ss_cpy_c(&log, "");
		levels = st_traverse_levelorder(t, test_traverse, (void *)&log);
		out = ss_to_c(log);
		if (levels < 1 || strcmp(out, "ebhadgicf") != 0) {
			res |= 2;
			break;
		}
		ss_cpy_c(&log, "");
		levels = st_traverse_inorder(t, test_traverse, (void *)&log);
		out = ss_to_c(log);
		if (levels < 1 || strcmp(out, "abcdefghi") != 0) {
			res |= 4;
			break;
		}
		ss_cpy_c(&log, "");
		levels = st_traverse_postorder(t, test_traverse, (void *)&log);
		out = ss_to_c(log);
		if (levels < 1 || strcmp(out, "acdbfgihe") != 0) {
			res |= 8;
			break;
		}
		break;
	}
	ST_ENV_TEST_AUX_LEAVE;
	return res;
}

#define TEST_SM_ALLOC_DONOTHING(a)
#define TEST_SM_ALLOC_X(fn, sm_alloc_X, type, insert, at, sm_free_X)           \
	static int fn()                                                        \
	{                                                                      \
		size_t n = 1000;                                               \
		srt_map *m = sm_alloc_X(type, n);                              \
		int res = 0;                                                   \
		for (;;) {                                                     \
			if (!m) {                                              \
				res = 1;                                       \
				break;                                         \
			}                                                      \
			if (!sm_empty(m) || sm_capacity(m) != n                \
			    || sm_capacity_left(m) != n) {                     \
				res = 2;                                       \
				break;                                         \
			}                                                      \
			insert(&m, 1, 1001);                                   \
			if (sm_size(m) != 1) {                                 \
				res = 3;                                       \
				break;                                         \
			}                                                      \
			insert(&m, 2, 1002);                                   \
			insert(&m, 3, 1003);                                   \
			if (sm_empty(m) || sm_capacity(m) != n                 \
			    || sm_capacity_left(m) != n - 3) {                 \
				res = 4;                                       \
				break;                                         \
			}                                                      \
			if (at(m, 1) != 1001) {                                \
				res = 5;                                       \
				break;                                         \
			}                                                      \
			if (at(m, 2) != 1002) {                                \
				res = 6;                                       \
				break;                                         \
			}                                                      \
			if (at(m, 3) != 1003) {                                \
				res = 7;                                       \
				break;                                         \
			}                                                      \
			break;                                                 \
		}                                                              \
		if (!res) {                                                    \
			sm_clear(m);                                           \
			res = !sm_size(m) ? 0 : 8;                             \
		}                                                              \
		sm_free_X(&m);                                                 \
		return res;                                                    \
	}

TEST_SM_ALLOC_X(test_sm_alloc_ii32, sm_alloc, SM_II32, sm_insert_ii32,
		sm_at_ii32, sm_free)
TEST_SM_ALLOC_X(test_sm_alloca_ii32, sm_alloca, SM_II32, sm_insert_ii32,
		sm_at_ii32, TEST_SM_ALLOC_DONOTHING)
TEST_SM_ALLOC_X(test_sm_alloc_uu32, sm_alloc, SM_UU32, sm_insert_uu32,
		sm_at_uu32, sm_free)
TEST_SM_ALLOC_X(test_sm_alloca_uu32, sm_alloca, SM_UU32, sm_insert_uu32,
		sm_at_uu32, TEST_SM_ALLOC_DONOTHING)
TEST_SM_ALLOC_X(test_sm_alloc_ii, sm_alloc, SM_II, sm_insert_ii, sm_at_ii,
		sm_free)
TEST_SM_ALLOC_X(test_sm_alloca_ii, sm_alloca, SM_II, sm_insert_ii, sm_at_ii,
		TEST_SM_ALLOC_DONOTHING)

TEST_SM_ALLOC_X(test_sm_alloc_ff, sm_alloc, SM_FF, sm_insert_ff, sm_at_ff,
		sm_free)
TEST_SM_ALLOC_X(test_sm_alloca_ff, sm_alloca, SM_FF, sm_insert_ff, sm_at_ff,
		TEST_SM_ALLOC_DONOTHING)
TEST_SM_ALLOC_X(test_sm_alloc_dd, sm_alloc, SM_DD, sm_insert_dd, sm_at_dd,
		sm_free)
TEST_SM_ALLOC_X(test_sm_alloca_dd, sm_alloca, SM_DD, sm_insert_dd, sm_at_dd,
		TEST_SM_ALLOC_DONOTHING)

#define TEST_SM_SHRINK_TO_FIT_VARS(m, atype, r)                                \
	srt_map *m = sm_alloc(atype, r), *m##2 = sm_alloca(atype, r)

#define TEST_SM_SHRINK_TO_FIT(m, ntest, atype, r)                              \
	res |= !m ? 1 << ntest                                                 \
		  : !m##2 ? 2 << ntest                                         \
			  : (sm_max_size(m) == r ? 0 : 3 << ntest)             \
				       | (sm_max_size(m##2) == r ? 0           \
								 : 4 << ntest) \
				       | (sm_shrink(&m) && sm_max_size(m) == 0 \
						  ? 0                          \
						  : 5 << ntest)                \
				       | (sm_shrink(&m##2)                     \
							  && sm_max_size(m##2) \
								     == r      \
						  ? 0                          \
						  : 6 << ntest);               \
	sm_free(&m)

static int test_sm_shrink()
{
	TEST_SM_SHRINK_TO_FIT_VARS(aa, SM_II32, 10);
	TEST_SM_SHRINK_TO_FIT_VARS(bb, SM_UU32, 10);
	TEST_SM_SHRINK_TO_FIT_VARS(cc, SM_II, 10);
	TEST_SM_SHRINK_TO_FIT_VARS(dd, SM_IS, 10);
	TEST_SM_SHRINK_TO_FIT_VARS(ee, SM_IP, 10);
	TEST_SM_SHRINK_TO_FIT_VARS(ff, SM_SI, 10);
	TEST_SM_SHRINK_TO_FIT_VARS(gg, SM_SS, 10);
	TEST_SM_SHRINK_TO_FIT_VARS(hh, SM_SP, 10);
	TEST_SM_SHRINK_TO_FIT_VARS(ii, SM_FF, 10);
	TEST_SM_SHRINK_TO_FIT_VARS(jj, SM_DD, 10);
	TEST_SM_SHRINK_TO_FIT_VARS(kk, SM_DS, 10);
	TEST_SM_SHRINK_TO_FIT_VARS(ll, SM_DP, 10);
	TEST_SM_SHRINK_TO_FIT_VARS(mm, SM_SD, 10);
	int res = 0;
	TEST_SM_SHRINK_TO_FIT(aa, 0, SM_II32, 10);
	TEST_SM_SHRINK_TO_FIT(bb, 1, SM_UU32, 10);
	TEST_SM_SHRINK_TO_FIT(cc, 2, SM_II, 10);
	TEST_SM_SHRINK_TO_FIT(dd, 3, SM_IS, 10);
	TEST_SM_SHRINK_TO_FIT(ee, 4, SM_IP, 10);
	TEST_SM_SHRINK_TO_FIT(ff, 5, SM_SI, 10);
	TEST_SM_SHRINK_TO_FIT(gg, 6, SM_SS, 10);
	TEST_SM_SHRINK_TO_FIT(hh, 7, SM_SP, 10);
	TEST_SM_SHRINK_TO_FIT(ii, 8, SM_FF, 10);
	TEST_SM_SHRINK_TO_FIT(jj, 9, SM_DD, 10);
	TEST_SM_SHRINK_TO_FIT(kk, 10, SM_DS, 10);
	TEST_SM_SHRINK_TO_FIT(ll, 11, SM_DP, 10);
	TEST_SM_SHRINK_TO_FIT(mm, 12, SM_SD, 10);
	return res;
}

#define TEST_SM_DUP_VARS(m, atype, r)                                          \
	srt_map *m = sm_alloc(atype, r), *m##2 = sm_alloca(atype, r),          \
		*m##b = NULL, *m##2b = NULL

#define TEST_SM_DUP(m, ntest, atype, insf, kv, vv, atf, cmpf, r)               \
	res |= !m || !m##2 ? 1 << (ntest * 2) : 0;                             \
	if (!res) {                                                            \
		int j = 0;                                                     \
		for (; j < r; j++) {                                           \
			srt_bool b1 = insf(&m, kv[j], vv[j]);                  \
			srt_bool b2 = insf(&m##2, kv[j], vv[j]);               \
			if (!b1 || !b2) {                                      \
				res |= 2 << (ntest * 2);                       \
				break;                                         \
			}                                                      \
		}                                                              \
	}                                                                      \
	if (!res) {                                                            \
		m##b = sm_dup(m);                                              \
		m##2b = sm_dup(m##2);                                          \
		res = (!m##b || !m##2b)                                        \
			      ? 4 << (ntest * 2)                               \
			      : (sm_size(m) != r || sm_size(m##2) != r         \
				 || sm_size(m##b) != r || sm_size(m##2b) != r) \
					? 2 << (ntest * 2)                     \
					: 0;                                   \
	}                                                                      \
	if (!res) {                                                            \
		int j = 0;                                                     \
		for (; j < r; j++) {                                           \
			if (cmpf(atf(m, kv[j]), atf(m##2, kv[j]))              \
			    || cmpf(atf(m, kv[j]), atf(m##b, kv[j]))           \
			    || cmpf(atf(m##b, kv[j]), atf(m##2b, kv[j]))) {    \
				res |= 3 << (ntest * 2);                       \
				break;                                         \
			}                                                      \
		}                                                              \
	}                                                                      \
	sm_free(&m);                                                           \
	sm_free(&m##2);                                                        \
	sm_free(&m##b);                                                        \
	sm_free(&m##2b)

/*
 * Covers: sm_dup, sm_insert_ii32, sm_insert_uu32, sm_insert_ii, sm_insert_is,
 * sm_insert_ip, sm_insert_si, sm_insert_ss, sm_insert_sp, sm_at_ii32,
 * sm_at_uu32, sm_at_ii, sm_at_is, sm_at_ip, sm_at_si, sm_at_ss, sm_at_sp
 */
static int test_sm_dup()
{
	TEST_SM_DUP_VARS(aa, SM_II32, 10);
	TEST_SM_DUP_VARS(bb, SM_UU32, 10);
	TEST_SM_DUP_VARS(cc, SM_II, 10);
	TEST_SM_DUP_VARS(dd, SM_IS, 10);
	TEST_SM_DUP_VARS(ee, SM_IP, 10);
	TEST_SM_DUP_VARS(ff, SM_SI, 10);
	TEST_SM_DUP_VARS(gg, SM_SS, 10);
	TEST_SM_DUP_VARS(hh, SM_SP, 10);
	TEST_SM_DUP_VARS(ii, SM_FF, 10);
	TEST_SM_DUP_VARS(jj, SM_DD, 10);
	TEST_SM_DUP_VARS(kk, SM_DS, 10);
	TEST_SM_DUP_VARS(ll, SM_DP, 10);
	TEST_SM_DUP_VARS(mm, SM_SD, 10);
	int res = 0;
	srt_string *s = ss_dup_c("hola1"), *s2 = ss_alloca(100);
	srt_string *ssk[10], *ssv[10];
	int i = 0;
	ss_cpy_c(&s2, "hola2");
	for (; i < 10; i++) {
		ssk[i] = ss_alloca(100);
		ssv[i] = ss_alloca(100);
		ss_printf(&ssk[i], 50, "key_%i", i);
		ss_printf(&ssv[i], 50, "val_%i", i);
	}
	TEST_SM_DUP(aa, 0, SM_II32, sm_insert_ii32, i32k, i32v, sm_at_ii32,
		    scmp_int32, 10);
	TEST_SM_DUP(bb, 1, SM_UU32, sm_insert_uu32, u32k, u32v, sm_at_uu32,
		    scmp_uint32, 10);
	TEST_SM_DUP(cc, 2, SM_II, sm_insert_ii, sik, i64v, sm_at_ii, scmp_i,
		    10);
	TEST_SM_DUP(dd, 3, SM_IS, sm_insert_is, sik, ssv, sm_at_is, ss_cmp, 10);
	TEST_SM_DUP(ee, 4, SM_IP, sm_insert_ip, sik, spv, sm_at_ip, scmp_ptr,
		    10);
	TEST_SM_DUP(ff, 5, SM_SI, sm_insert_si, ssk, i64v, sm_at_si, scmp_i,
		    10);
	TEST_SM_DUP(gg, 6, SM_SS, sm_insert_ss, ssk, ssv, sm_at_ss, ss_cmp, 10);
	TEST_SM_DUP(hh, 7, SM_SP, sm_insert_sp, ssk, spv, sm_at_sp, scmp_ptr,
		    10);
	TEST_SM_DUP(ii, 8, SM_FF, sm_insert_ff, sik, fv, sm_at_ff, scmp_f, 10);
	TEST_SM_DUP(jj, 9, SM_DD, sm_insert_dd, sik, dv, sm_at_dd, scmp_d, 10);
	TEST_SM_DUP(kk, 10, SM_DS, sm_insert_ds, sdk, ssv, sm_at_ds, ss_cmp,
		    10);
	TEST_SM_DUP(ll, 11, SM_DP, sm_insert_dp, sdk, spv, sm_at_dp, scmp_ptr,
		    10);
	TEST_SM_DUP(mm, 12, SM_SD, sm_insert_sd, ssk, dv, sm_at_sd, scmp_d, 10);
	ss_free(&s);
	return res;
}

static int test_sm_cpy()
{
	int res = 0;
	srt_map *m1 = sm_alloc(SM_II32, 0), *m2 = sm_alloc(SM_SS, 0),
		*m1a3 = sm_alloca(SM_II32, 3), *m1a10 = sm_alloca(SM_II32, 10);
	const srt_string *k1 = ss_crefa("key1"), *k2 = ss_crefa("key2"),
			 *k3 = ss_crefa("key3"), *v1 = ss_crefa("val1"),
			 *v2 = ss_crefa("val2"), *v3 = ss_crefa("val3");
	sm_insert_ii32(&m1, 1, 1);
	sm_insert_ii32(&m1, 2, 2);
	sm_insert_ii32(&m1, 3, 3);
	sm_insert_ss(&m2, k1, v1);
	sm_insert_ss(&m2, k2, v2);
	sm_insert_ss(&m2, k3, v3);
	sm_cpy(&m1, m2);
	sm_cpy(&m1a3, m2);
	sm_cpy(&m1a10, m2);
	res |= (sm_size(m1) == 3 ? 0 : 1);
#ifdef S_ENABLE_SM_STRING_OPTIMIZATION
	/*
	 * TODO: adjust test to the node size of the string optimized map mode
	 */
#else
	if (sizeof(uint32_t) == sizeof(void *))
		res |= (sm_size(m1a3) == 3 ? 0 : 2);
	else
		res |= (sm_size(m1a3) < 3 ? 0 : 4);
	res |= (sm_size(m1a10) == 3 ? 0 : 8);
	res |= (!ss_cmp(sm_it_s_k(m1, 0), sm_it_s_k(m2, 0)) ? 0 : 16);
	res |= (!ss_cmp(sm_it_s_k(m1, 1), sm_it_s_k(m2, 1)) ? 0 : 32);
	res |= (!ss_cmp(sm_it_s_k(m1, 2), sm_it_s_k(m2, 2)) ? 0 : 64);
	res |= (!ss_cmp(sm_it_s_k(m1, 0), sm_it_s_k(m1a10, 0)) ? 0 : 128);
	res |= (!ss_cmp(sm_it_s_k(m1, 1), sm_it_s_k(m1a10, 1)) ? 0 : 256);
	res |= (!ss_cmp(sm_it_s_k(m1, 2), sm_it_s_k(m1a10, 2)) ? 0 : 512);
#endif
#ifdef S_USE_VA_ARGS
	sm_free(&m1, &m2, &m1a3, &m1a10);
#else
	sm_free(&m1);
	sm_free(&m2);
	sm_free(&m1a3);
	sm_free(&m1a10);
#endif
	return res;
}

#define TEST_SM_X_COUNT(ID, T, insf, cntf, v)                                  \
	int res = 0;                                                           \
	uint32_t i, tcount = 100;                                              \
	srt_map *m = sm_alloc(ID, tcount);                                     \
	for (i = 0; i < tcount; i++)                                           \
		insf(&m, (T)i, v);                                             \
	for (i = 0; i < tcount; i++)                                           \
		if (!cntf(m, (T)i)) {                                          \
			res = 1 + (int)i;                                      \
			break;                                                 \
		}                                                              \
	sm_free(&m);                                                           \
	return res;

static int test_sm_count_u32()
{
	TEST_SM_X_COUNT(SM_UU32, uint32_t, sm_insert_uu32, sm_count_u32, 1);
}

static int test_sm_count_i32()
{
	TEST_SM_X_COUNT(SM_II32, int32_t, sm_insert_ii32, sm_count_i32, 1);
}

static int test_sm_count_i64()
{
	TEST_SM_X_COUNT(SM_II, int64_t, sm_insert_ii, sm_count_i, 1);
}

static int test_sm_count_f()
{
	TEST_SM_X_COUNT(SM_FF, float, sm_insert_ff, sm_count_f, 1);
}

static int test_sm_count_d()
{
	TEST_SM_X_COUNT(SM_DD, double, sm_insert_dd, sm_count_d, 1);
}

static int test_sm_count_s()
{
	int res;
	srt_string *s = ss_dup_c("a_1"), *t = ss_dup_c("a_2"),
		   *u = ss_dup_c("a_3");
	srt_map *m = sm_alloc(SM_SI, 3);
	sm_insert_si(&m, s, 1);
	sm_insert_si(&m, t, 2);
	sm_insert_si(&m, u, 3);
	res = sm_count_s(m, s) && sm_count_s(m, t) && sm_count_s(m, u) ? 0 : 1;
#ifdef S_USE_VA_ARGS
	ss_free(&s, &t, &u);
#else
	ss_free(&s);
	ss_free(&t);
	ss_free(&u);
#endif
	sm_free(&m);
	return res;
}

static int test_sm_inc_ii32()
{
	int res;
	srt_map *m = sm_alloc(SM_II32, 0);
	sm_inc_ii32(&m, 123, -10);
	sm_inc_ii32(&m, 123, -20);
	sm_inc_ii32(&m, 123, -30);
	res = !m ? 1 : (sm_at_ii32(m, 123) == -60 ? 0 : 2);
	sm_free(&m);
	return res;
}

static int test_sm_inc_uu32()
{
	int res;
	srt_map *m = sm_alloc(SM_UU32, 0);
	sm_inc_uu32(&m, 123, 10);
	sm_inc_uu32(&m, 123, 20);
	sm_inc_uu32(&m, 123, 30);
	res = !m ? 1 : (sm_at_uu32(m, 123) == 60 ? 0 : 2);
	sm_free(&m);
	return res;
}

static int test_sm_inc_ii()
{
	int res;
	srt_map *m = sm_alloc(SM_II, 0);
	sm_inc_ii(&m, 123, -7);
	sm_inc_ii(&m, 123, S_MAX_I64);
	sm_inc_ii(&m, 123, 3);
	res = !m ? 1 : (sm_at_ii(m, 123) == S_MAX_I64 - 4 ? 0 : 2);
	sm_free(&m);
	return res;
}

static int test_sm_inc_si()
{
	int res;
	srt_map *m = sm_alloc(SM_SI, 0);
	const srt_string *k = ss_crefa("hello");
	sm_inc_si(&m, k, -7);
	sm_inc_si(&m, k, S_MAX_I64);
	sm_inc_si(&m, k, 3);
	res = !m ? 1 : (sm_at_si(m, k) == S_MAX_I64 - 4 ? 0 : 2);
	sm_free(&m);
	return res;
}

static int test_sm_inc_sd()
{
	int res;
	srt_map *m = sm_alloc(SM_SD, 0);
	const srt_string *k = ss_crefa("hello");
	sm_inc_sd(&m, k, (double)-7);
	sm_inc_sd(&m, k, (double)S_MAX_I64);
	sm_inc_sd(&m, k, (double)3);
	res = !m ? 1 : (sm_at_sd(m, k) == (double)(S_MAX_I64 - 4) ? 0 : 2);
	sm_free(&m);
	return res;
}

static int test_sm_delete_i()
{
	int res;
	srt_map *m_ii32 = sm_alloc(SM_II32, 0), *m_uu32 = sm_alloc(SM_UU32, 0),
		*m_ii = sm_alloc(SM_II, 0);
	/*
	 * Insert elements
	 */
	sm_insert_ii32(&m_ii32, -1, -1);
	sm_insert_ii32(&m_ii32, -2, -2);
	sm_insert_ii32(&m_ii32, -3, -3);
	sm_insert_uu32(&m_uu32, 1, 1);
	sm_insert_uu32(&m_uu32, 2, 2);
	sm_insert_uu32(&m_uu32, 3, 3);
	sm_insert_ii(&m_ii, S_MAX_I64, S_MAX_I64);
	sm_insert_ii(&m_ii, S_MAX_I64 - 1, S_MAX_I64 - 1);
	sm_insert_ii(&m_ii, S_MAX_I64 - 2, S_MAX_I64 - 2);
	/*
	 * Check elements were properly inserted
	 */
	res = m_ii32 && m_uu32 && m_ii ? 0 : 1;
	res |= (sm_at_ii32(m_ii32, -2) == -2 ? 0 : 2);
	res |= (sm_at_uu32(m_uu32, 2) == 2 ? 0 : 4);
	res |= (sm_at_ii(m_ii, S_MAX_I64 - 1) == S_MAX_I64 - 1 ? 0 : 8);
	/*
	 * Delete elements, checking that second deletion of same
	 * element gives error
	 */
	res |= sm_delete_i32(m_ii32, -2) ? 0 : 16;
	res |= !sm_delete_i32(m_ii32, -2) ? 0 : 32;
	res |= sm_delete_u32(m_uu32, 2) ? 0 : 64;
	res |= !sm_delete_u32(m_uu32, 2) ? 0 : 128;
	res |= sm_delete_i(m_ii, S_MAX_I64 - 1) ? 0 : 256;
	res |= !sm_delete_i(m_ii, S_MAX_I64 - 1) ? 0 : 512;
	/*
	 * Check that querying for deleted elements gives default value
	 */
	res |= (sm_at_ii32(m_ii32, -2) == 0 ? 0 : 1024);
	res |= (sm_at_uu32(m_uu32, 2) == 0 ? 0 : 2048);
	res |= (sm_at_ii(m_ii, S_MAX_I64 - 1) == 0 ? 0 : 4096);
#ifdef S_USE_VA_ARGS
	sm_free(&m_ii32, &m_uu32, &m_ii);
#else
	sm_free(&m_ii32);
	sm_free(&m_uu32);
	sm_free(&m_ii);
#endif
	return res;
}

static int test_sm_delete_s()
{
	int res;
	srt_map *m_si = sm_alloc(SM_SI, 0), *m_sp = sm_alloc(SM_SP, 0),
		*m_ss = sm_alloc(SM_SS, 0);
	/*
	 * Insert elements
	 */
	const srt_string *k1 = ss_crefa("key1"), *k2 = ss_crefa("key2"),
			 *k3 = ss_crefa("key3"), *v1 = ss_crefa("val1"),
			 *v2 = ss_crefa("val2"), *v3 = ss_crefa("val3");
	sm_insert_si(&m_si, k1, -1);
	sm_insert_si(&m_si, k2, S_MAX_I64);
	sm_insert_si(&m_si, k3, -3);
	sm_insert_sp(&m_sp, k1, (void *)-1);
	sm_insert_sp(&m_sp, k2, (void *)-2);
	sm_insert_sp(&m_sp, k3, (void *)-3);
	sm_insert_ss(&m_ss, k1, v1);
	sm_insert_ss(&m_ss, k2, v2);
	sm_insert_ss(&m_ss, k3, v3);
	/*
	 * Check elements were properly inserted
	 */
	res = m_si && m_sp && m_ss ? 0 : 1;
	res |= (sm_at_si(m_si, k2) == S_MAX_I64 ? 0 : 2);
	res |= (sm_at_sp(m_sp, k2) == (void *)-2 ? 0 : 4);
	res |= (!ss_cmp(sm_at_ss(m_ss, k2), v2) ? 0 : 8);
	/*
	 * Delete elements, checking that second deletion of same
	 * element gives error
	 */
	res |= sm_delete_s(m_si, k2) ? 0 : 16;
	res |= !sm_delete_s(m_si, k2) ? 0 : 32;
	res |= sm_delete_s(m_sp, k2) ? 0 : 64;
	res |= !sm_delete_s(m_sp, k2) ? 0 : 128;
	res |= sm_delete_s(m_ss, k2) ? 0 : 256;
	res |= !sm_delete_s(m_ss, k2) ? 0 : 512;
	/*
	 * Check that querying for deleted elements gives default value
	 */
	res |= (sm_at_si(m_si, k2) == 0 ? 0 : 1024);
	res |= (sm_at_sp(m_sp, k2) == 0 ? 0 : 2048);
	res |= (!ss_cmp(sm_at_ss(m_ss, k2), ss_void) ? 0 : 4096);
#ifdef S_USE_VA_ARGS
	sm_free(&m_si, &m_sp, &m_ss);
#else
	sm_free(&m_si);
	sm_free(&m_sp);
	sm_free(&m_ss);
#endif
	return res;
}

static srt_bool cback_i32i32(int32_t k, int32_t v, void *context)
{
	(void)k;
	(void)v;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

static srt_bool cback_ss(const srt_string *k, const srt_string *v,
			 void *context)
{
	(void)k;
	(void)v;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

static srt_bool cback_i32(int32_t k, void *context)
{
	(void)k;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

static srt_bool cback_u32(uint32_t k, void *context)
{
	(void)k;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

static srt_bool cback_i(int64_t k, void *context)
{
	(void)k;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

static srt_bool cback_f(float k, void *context)
{
	(void)k;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

static srt_bool cback_d(double k, void *context)
{
	(void)k;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

static srt_bool cback_s(const srt_string *k, void *context)
{
	(void)k;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

#define TEST_SM_IT_X_VARS(id, et)                                              \
	srt_map *m_##id = sm_alloc(et, 0), *m_a##id = sm_alloca(et, 3)

#define TEST_SM_IT_X(n, id, et, itk, itv, cmpkf, cmpvf, k1, v1, k2, v2, k3,    \
		     v3, res)                                                  \
	sm_insert_##id(&m_##id, k1, v1);                                       \
	sm_insert_##id(&m_a##id, k1, v1);                                      \
	res |= (!cmpkf(itk(m_##id, 0), k1) && !cmpvf(itv(m_##id, 0), v1)) ? 0  \
									  : n; \
	sm_insert_##id(&m_##id, k2, v2);                                       \
	sm_insert_##id(&m_##id, k3, v3);                                       \
	sm_insert_##id(&m_a##id, k2, v2);                                      \
	sm_insert_##id(&m_a##id, k3, v3);                                      \
	res |= (!cmpvf(sm_at_##id(m_##id, k1), v1)                             \
		&& !cmpvf(sm_at_##id(m_##id, k2), v2)                          \
		&& !cmpvf(sm_at_##id(m_##id, k3), v3)                          \
		&& !cmpvf(sm_at_##id(m_a##id, k1), v1)                         \
		&& !cmpvf(sm_at_##id(m_a##id, k2), v2)                         \
		&& !cmpvf(sm_at_##id(m_a##id, k3), v3)                         \
		&& !cmpkf(itk(m_##id, 0), itk(m_a##id, 0))                     \
		&& !cmpkf(itk(m_##id, 1), itk(m_a##id, 1))                     \
		&& !cmpkf(itk(m_##id, 2), itk(m_a##id, 2))                     \
		&& !cmpvf(itv(m_##id, 0), itv(m_a##id, 0))                     \
		&& !cmpvf(itv(m_##id, 1), itv(m_a##id, 1))                     \
		&& !cmpvf(itv(m_##id, 2), itv(m_a##id, 2))                     \
		&& cmpkf(itk(m_##id, 0), 0) && cmpkf(itk(m_a##id, 0), 0)       \
		&& cmpkf(itk(m_##id, 1), 0) && cmpkf(itk(m_a##id, 1), 0)       \
		&& cmpkf(itk(m_##id, 2), 0) && cmpkf(itk(m_a##id, 2), 0)       \
		&& cmpvf(itv(m_##id, 0), 0) && cmpvf(itv(m_a##id, 0), 0)       \
		&& cmpvf(itv(m_##id, 1), 0) && cmpvf(itv(m_a##id, 1), 0)       \
		&& cmpvf(itv(m_##id, 2), 0) && cmpvf(itv(m_a##id, 2), 0))      \
		       ? 0                                                     \
		       : n;                                                    \
	sm_free(&m_##id);                                                      \
	sm_free(&m_a##id);

static int test_sm_it()
{
	TEST_SM_IT_X_VARS(ii32, SM_II32);
	TEST_SM_IT_X_VARS(uu32, SM_UU32);
	TEST_SM_IT_X_VARS(ii, SM_II);
	TEST_SM_IT_X_VARS(is, SM_IS);
	TEST_SM_IT_X_VARS(ip, SM_IP);
	TEST_SM_IT_X_VARS(si, SM_SI);
	TEST_SM_IT_X_VARS(ss, SM_SS);
	TEST_SM_IT_X_VARS(sp, SM_SP);
	TEST_SM_IT_X_VARS(ff, SM_FF);
	TEST_SM_IT_X_VARS(dd, SM_DD);
	TEST_SM_IT_X_VARS(ds, SM_DS);
	TEST_SM_IT_X_VARS(dp, SM_DP);
	TEST_SM_IT_X_VARS(sd, SM_SD);
	int res = 0;
	const srt_string *k1 = ss_crefa("key1"), *k2 = ss_crefa("key2"),
			 *k3 = ss_crefa("key3"), *v1 = ss_crefa("val1"),
			 *v2 = ss_crefa("val2"), *v3 = ss_crefa("val3");
	TEST_SM_IT_X(1, ii32, SM_II32, sm_it_i32_k, sm_it_ii32_v, scmp_i,
		     scmp_i, -1, -1, -2, -2, -3, -3, res);
	TEST_SM_IT_X(2, uu32, SM_UU32, sm_it_u32_k, sm_it_uu32_v, scmp_i,
		     scmp_i, 1, 1, 2, 2, 3, 3, res);
	TEST_SM_IT_X(4, ii, SM_II, sm_it_i_k, sm_it_ii_v, scmp_i, scmp_i,
		     S_MAX_I64, S_MIN_I64, S_MIN_I64, S_MAX_I64, S_MAX_I64 - 1,
		     S_MIN_I64 + 1, res);
	TEST_SM_IT_X(8, is, SM_IS, sm_it_i_k, sm_it_is_v, scmp_i, ss_cmp, 1, v1,
		     2, v2, 3, v3, res);
	TEST_SM_IT_X(16, ip, SM_IP, sm_it_i_k, sm_it_ip_v, scmp_i, scmp_ptr, 1,
		     (void *)1, 2, (void *)2, 3, (void *)3, res);
	TEST_SM_IT_X(32, si, SM_SI, sm_it_s_k, sm_it_si_v, ss_cmp, scmp_i, v1,
		     1, v2, 2, v3, 3, res);
	TEST_SM_IT_X(64, ss, SM_SS, sm_it_s_k, sm_it_ss_v, ss_cmp, ss_cmp, k1,
		     v1, k2, v2, k3, v3, res);
	TEST_SM_IT_X(128, sp, SM_SP, sm_it_s_k, sm_it_sp_v, ss_cmp, scmp_ptr,
		     k1, (void *)1, k2, (void *)2, k3, (void *)3, res);
	TEST_SM_IT_X(1 << 9, ff, SM_FF, sm_it_f_k, sm_it_ff_v, scmp_f, scmp_f,
		     -1, -1, -2, -2, -3, -3, res);
	TEST_SM_IT_X(1 << 10, dd, SM_DD, sm_it_d_k, sm_it_dd_v, scmp_d, scmp_d,
		     -1, -1, -2, -2, -3, -3, res);
	TEST_SM_IT_X(1 << 11, ds, SM_DS, sm_it_d_k, sm_it_ds_v, scmp_d, ss_cmp,
		     1, v1, 2, v2, 3, v3, res);
	TEST_SM_IT_X(1 << 12, dp, SM_DP, sm_it_d_k, sm_it_dp_v, scmp_d,
		     scmp_ptr, 1, (void *)1, 2, (void *)2, 3, (void *)3, res);
	TEST_SM_IT_X(1 << 13, sd, SM_SD, sm_it_s_k, sm_it_sd_v, ss_cmp, scmp_d,
		     v1, 1, v2, 2, v3, 3, res);
	return res;
}

static int test_sm_itr()
{
	int res = 1;
	size_t nelems = 100;
	srt_map *m_ii32 = sm_alloc(SM_II32, nelems),
		*m_uu32 = sm_alloc(SM_UU32, nelems),
		*m_ii = sm_alloc(SM_II, nelems),
		*m_ff = sm_alloc(SM_FF, nelems),
		*m_dd = sm_alloc(SM_DD, nelems),
		*m_is = sm_alloc(SM_IS, nelems),
		*m_ip = sm_alloc(SM_IP, nelems),
		*m_si = sm_alloc(SM_SI, nelems),
		*m_ss = sm_alloc(SM_SS, nelems),
		*m_sp = sm_alloc(SM_SP, nelems),
		*m_ds = sm_alloc(SM_DS, nelems),
		*m_dp = sm_alloc(SM_DP, nelems),
		*m_sd = sm_alloc(SM_SD, nelems);
	size_t processed_ii321, processed_ii322, processed_uu32, processed_ii,
		processed_ff, processed_dd, processed_is, processed_ip,
		processed_si, processed_ds, processed_dp, processed_sd,
		processed_ss1, processed_ss2, processed_sp, cnt1, cnt2;
	int32_t lower_i32, upper_i32;
	uint32_t lower_u32, upper_u32;
	int64_t lower_i, upper_i;
	float lower_f, upper_f;
	double lower_d, upper_d;
	srt_string *lower_s, *upper_s, *ktmp, *vtmp;
	const srt_string *crefa_10 = ss_crefa("10"), *crefa_20 = ss_crefa("20");
	int i;
	if (m_ii32 && m_uu32 && m_ii && m_ff && m_dd && m_is && m_ip && m_si
	    && m_ss && m_sp && m_ds && m_dp && m_sd) {
		ktmp = ss_alloca(1000);
		vtmp = ss_alloca(1000);
		for (i = 0; i < (int)nelems; i++) {
			sm_insert_ii32(&m_ii32, -i, -i);
			sm_insert_uu32(&m_uu32, (uint32_t)i, (uint32_t)i);
			sm_insert_ii(&m_ii, -i, -i);
			sm_insert_ff(&m_ff, (float)-i, (float)-i);
			sm_insert_dd(&m_dd, (double)-i, (double)-i);
			ss_printf(&ktmp, 200, "k%04i", i);
			ss_printf(&vtmp, 200, "v%04i", i);
			sm_insert_is(&m_is, -i, vtmp);
			sm_insert_ip(&m_ip, -i, (void *)(intptr_t)i);
			sm_insert_si(&m_si, ktmp, i);
			sm_insert_ss(&m_ss, ktmp, vtmp);
			sm_insert_sp(&m_sp, ktmp, (void *)(intptr_t)i);
			sm_insert_ds(&m_ds, (double)-i, vtmp);
			sm_insert_dp(&m_dp, (double)-i, (void *)(intptr_t)i);
			sm_insert_sd(&m_sd, ktmp, (double)i);
		}
		cnt1 = cnt2 = 0;
		lower_i32 = -20;
		upper_i32 = -10;
		lower_u32 = 10;
		upper_u32 = 20;
		lower_i = -20;
		upper_i = -10;
		lower_f = (float)-20.00001;
		upper_f = (float)-9.99999;
		lower_d = -20.00000000000000001;
		upper_d = -9.999999999999999999;
		lower_s = ss_alloca(100);
		upper_s = ss_alloca(100);
		ss_cpy_c(&lower_s, "k001"); /* covering "k0010" to "k0019" */
		ss_cpy_c(&upper_s, "k002");
		processed_ii321 = sm_itr_ii32(m_ii32, lower_i32, upper_i32,
					      cback_i32i32, &cnt1);
		processed_ii322 =
			sm_itr_ii32(m_ii32, lower_i32, upper_i32, NULL, NULL);
		processed_uu32 =
			sm_itr_uu32(m_uu32, lower_u32, upper_u32, NULL, NULL);
		processed_ii = sm_itr_ii(m_ii, lower_i, upper_i, NULL, NULL);
		processed_ff = sm_itr_ff(m_ff, lower_f, upper_f, NULL, NULL);
		processed_dd = sm_itr_dd(m_dd, lower_d, upper_d, NULL, NULL);
		processed_is = sm_itr_is(m_is, lower_i, upper_i, NULL, NULL);
		processed_ip = sm_itr_ip(m_ip, lower_i, upper_i, NULL, NULL);
		processed_si = sm_itr_si(m_si, lower_s, upper_s, NULL, NULL);
		processed_ds = sm_itr_ds(m_ds, lower_d, upper_d, NULL, NULL);
		processed_dp = sm_itr_dp(m_dp, lower_d, upper_d, NULL, NULL);
		processed_sd = sm_itr_sd(m_sd, lower_s, upper_s, NULL, NULL);
		processed_ss1 =
			sm_itr_ss(m_ss, lower_s, upper_s, cback_ss, &cnt2);
		processed_ss2 = sm_itr_ss(m_ss, lower_s, upper_s, NULL, NULL);
		processed_sp = sm_itr_sp(m_sp, lower_s, upper_s, NULL, NULL);
		res = processed_ii321 == 11 ? 0 : 2;
		res |= processed_ii321 == processed_ii322 ? 0 : 4;
		res |= processed_ii322 == cnt1 ? 0 : 8;
		res |= processed_uu32 == cnt1 ? 0 : 0x10;
		res |= processed_ii == cnt1 ? 0 : 0x20;
		res |= processed_ff == cnt1 ? 0 : 0x20;
		res |= processed_dd == cnt1 ? 0 : 0x20;
		res |= processed_is == cnt1 ? 0 : 0x40;
		res |= processed_ip == cnt1 ? 0 : 0x80;
		res |= processed_si == 10 ? 0 : 0x100;
		res |= processed_ds == cnt1 ? 0 : 0x40;
		res |= processed_dp == cnt1 ? 0 : 0x80;
		res |= processed_sd == 10 ? 0 : 0x100;
		res |= processed_ss1 == 10 ? 0 : 0x200;
		res |= processed_ss1 == processed_ss2 ? 0 : 0x400;
		res |= processed_ss2 == cnt2 ? 0 : 0x800;
		res |= processed_sp == 10 ? 0 : 0x1000;
		/*
		 * Wrong type checks
		 */
		res |= sm_itr_uu32(m_ii32, 10, 20, NULL, NULL) > 0 ? 0x2000 : 0;
		res |= sm_itr_ii(m_ii32, 10, 20, NULL, NULL) > 0 ? 0x4000 : 0;
		res |= sm_itr_ff(m_ii32, 10, 20, NULL, NULL) > 0 ? 0x4000 : 0;
		res |= sm_itr_dd(m_ii32, 10, 20, NULL, NULL) > 0 ? 0x4000 : 0;
		res |= sm_itr_is(m_ii32, 10, 20, NULL, NULL) > 0 ? 0x8000 : 0;
		res |= sm_itr_ip(m_ii32, 10, 20, NULL, NULL) > 0 ? 0x10000 : 0;
		res |= sm_itr_si(m_ii32, crefa_10, crefa_20, NULL, NULL) > 0
			       ? 0x20000
			       : 0;
		res |= sm_itr_ds(m_ii32, 10, 20, NULL, NULL) > 0 ? 0x8000 : 0;
		res |= sm_itr_dp(m_ii32, 10, 20, NULL, NULL) > 0 ? 0x10000 : 0;
		res |= sm_itr_sd(m_ii32, crefa_10, crefa_20, NULL, NULL) > 0
			       ? 0x20000
			       : 0;
		res |= sm_itr_sp(m_ii32, crefa_10, crefa_20, NULL, NULL) > 0
			       ? 0x40000
			       : 0;
	}
#ifdef S_USE_VA_ARGS
	sm_free(&m_ii32, &m_uu32, &m_ii, &m_ff, &m_dd, &m_is, &m_ip, &m_si,
		&m_ds, &m_dp, &m_sd, &m_ss, &m_sp);
#else
	sm_free(&m_ii32);
	sm_free(&m_uu32);
	sm_free(&m_ii);
	sm_free(&m_ff);
	sm_free(&m_dd);
	sm_free(&m_is);
	sm_free(&m_ip);
	sm_free(&m_si);
	sm_free(&m_ds);
	sm_free(&m_dp);
	sm_free(&m_sd);
	sm_free(&m_ss);
	sm_free(&m_sp);
#endif
	return res;
}

static int test_sm_sort_to_vectors()
{
	size_t test_elems = 100;
	srt_map *m = sm_alloc(SM_II32, test_elems);
	srt_vector *kv = NULL, *vv = NULL, *kv2 = NULL, *vv2 = NULL;
	int res = m ? 0 : 1;
	ssize_t i, j;
	do {
		/*
		 * Add elements
		 */
		for (i = test_elems; i > 0 && !res; i--) {
			if (!sm_insert_ii32(&m, (int)i, (int)-i)
			    || !st_assert((srt_tree *)m)) {
				res |= 2;
				break;
			}
		}
		if (res)
			break;
		/*
		 * Check all elements after every delete (O(n^2))
		 */
		for (j = test_elems; j > 0 && !res; j--) {
			sm_sort_to_vectors(m, &kv2, &vv2);
			for (i = 0; i < j; i++) {
				int k = (int)sv_at_i32(kv2, (int32_t)i);
				int v = (int)sv_at_i32(vv2, (int32_t)i);
				if (k != (i + 1) || v != -(i + 1)) {
					res |= 4;
					break;
				}
			}
			if (!st_assert((srt_tree *)m))
				res |= 8;
			if (res)
				break;
			sv_set_size(kv2, 0);
			sv_set_size(vv2, 0);
			if (!sm_delete_i32(m, (int32_t)j)
			    || !st_assert((srt_tree *)m)) {
				res |= 16;
				break;
			}
		}
	} while (0);
	sm_free(&m);
#ifdef S_USE_VA_ARGS
	sv_free(&kv, &vv, &kv2, &vv2);
#else
	sv_free(&kv);
	sv_free(&vv);
	sv_free(&kv2);
	sv_free(&vv2);
#endif
	return res;
}

static int test_sm_double_rotation()
{
	size_t test_elems = 15;
	srt_map *m = sm_alloc(SM_II32, test_elems);
	srt_vector *kv = NULL, *vv = NULL, *kv2 = NULL, *vv2 = NULL;
	int res = m ? 0 : 1;
	ssize_t i;
	for (;;) {
		/*
		 * Add elements
		 */
		for (i = test_elems; i > 0; i--) {
			if (!sm_insert_ii32(&m, (int)i, (int)-i)
			    || !st_assert((srt_tree *)m)) {
				res = 2;
				break;
			}
		}
		if (res)
			break;
		/*
		 * Delete pair elements (it triggers double-rotation cases
		 * in the tree code of map implementation)
		 */
		for (i = test_elems; i > 0; i--) {
			if ((i % 2))
				continue;
			if (!sm_delete_i32(m, i) || !st_assert((srt_tree *)m)) {
				res = 3;
				break;
			}
		}
		if (res)
			break;
		/*
		 * Add pair elements in reverse order
		 */
		for (i = 1; i <= (ssize_t)test_elems; i++) {
			if ((i % 2))
				continue;
			if (!sm_insert_ii32(&m, (int)i, (int)-i)
			    || !st_assert((srt_tree *)m)) {
				res = 4;
				break;
			}
		}
		if (res)
			break;
		/*
		 * Delete all elements
		 */
		for (i = test_elems; i > 0; i--) {
			if (!sm_delete_i32(m, i)) {
				res = 5;
				break;
			}
			if (!st_assert((srt_tree *)m)) {
				res = 50;
				break;
			}
		}
		if (res)
			break;
		if (!st_assert((srt_tree *)m)) {
			res = 100;
			break;
		}
		break;
	}
	sm_free(&m);
#ifdef S_USE_VA_ARGS
	sv_free(&kv, &vv, &kv2, &vv2);
#else
	sv_free(&kv);
	sv_free(&vv);
	sv_free(&kv2);
	sv_free(&vv2);
#endif
	return res;
}

static int test_sms()
{
	int i, res = 0;
	const srt_string *k[] = {ss_crefa("k000"), ss_crefa("k001"),
				 ss_crefa("k002")};
	size_t cnt_i32 = 0, cnt_u32 = 0, cnt_i = 0, cnt_f = 0, cnt_d = 0,
	       cnt_s = 0, cnt_s2 = 0, processed_i32, processed_u32, processed_i,
	       processed_f, processed_d, processed_s, processed_s2;
	/*
	 * Allocation: heap, stack with 3 elements, and stack with 10 elements
	 */
	srt_set *s_i32 = sms_alloc(SMS_I32, 0), *s_u32 = sms_alloc(SMS_U32, 0),
		*s_i = sms_alloc(SMS_I, 0), *s_f = sms_alloc(SMS_F, 0),
		*s_d = sms_alloc(SMS_D, 0), *s_s = sms_alloc(SMS_S, 0),
		*s_s2 = NULL, *s_a3 = sms_alloca(SMS_I32, 3),
		*s_a10 = sms_alloca(SMS_I32, 10);
	res |= (sms_empty(s_i32) && sms_empty(s_u32) && sms_empty(s_i)
				&& sms_empty(s_f) && sms_empty(s_d)
				&& sms_empty(s_s) && sms_empty(s_s2)
				&& sms_empty(s_a3) && sms_empty(s_a10)
			? 0
			: 1 << 0);
	/*
	 * Insert elements
	 */
	for (i = 0; i < 3; i++) {
		sms_insert_i32(&s_i32, i + 10);
		sms_insert_u32(&s_u32, (uint32_t)i + 20);
		sms_insert_i(&s_i, i);
		sms_insert_f(&s_f, (float)i);
		sms_insert_d(&s_d, (double)i);
		sms_insert_s(&s_s, k[i]);
	}
	res |= (!sms_empty(s_i32) && !sms_empty(s_u32) && !sms_empty(s_i)
				&& !sms_empty(s_f) && !sms_empty(s_d)
				&& !sms_empty(s_s) && sms_empty(s_s2)
				&& sms_empty(s_a3) && sms_empty(s_a10)
			? 0
			: 1 << 1);
	res |= (sms_count_i32(s_i32, 10) && sms_count_i32(s_i32, 11)
				&& sms_count_i32(s_i32, 12)
				&& sms_count_u32(s_u32, 20)
				&& sms_count_u32(s_u32, 21)
				&& sms_count_u32(s_u32, 22)
				&& sms_count_i(s_i, 0) && sms_count_i(s_i, 1)
				&& sms_count_i(s_i, 2) && sms_count_f(s_f, 0)
				&& sms_count_f(s_f, (float)1)
				&& sms_count_f(s_f, (float)2)
				&& sms_count_d(s_d, (double)0)
				&& sms_count_d(s_d, (double)1)
				&& sms_count_d(s_d, (double)2)
				&& sms_count_s(s_s, k[0])
				&& sms_count_s(s_s, k[1])
				&& sms_count_s(s_s, k[2])
			? 0
			: 1 << 2);
	/*
	 * Enumeration
	 */
	s_s2 = sms_dup(s_s);
	processed_i32 = sms_itr_i32(s_i32, -1, 100, cback_i32, &cnt_i32);
	processed_u32 = sms_itr_u32(s_u32, 0, 100, cback_u32, &cnt_u32);
	processed_i = sms_itr_i(s_i, -1, 100, cback_i, &cnt_i);
	processed_f = sms_itr_f(s_f, -1, 100, cback_f, &cnt_f);
	processed_d = sms_itr_d(s_d, -1, 100, cback_d, &cnt_d);
	processed_s = sms_itr_s(s_s, k[0], k[2], cback_s, &cnt_s);
	processed_s2 = sms_itr_s(s_s2, k[0], k[2], cback_s, &cnt_s2);
	res |= (processed_i32 == cnt_i32 && cnt_i32 == 3 ? 0 : 1 << 3);
	res |= (processed_u32 == cnt_u32 && cnt_u32 == 3 ? 0 : 1 << 4);
	res |= (processed_i == cnt_i && cnt_i == 3 ? 0 : 1 << 5);
	res |= (processed_f == cnt_f && cnt_f == 3 ? 0 : 1 << 6);
	res |= (processed_d == cnt_d && cnt_d == 3 ? 0 : 1 << 7);
	res |= (processed_s == cnt_s && cnt_s == 3 ? 0 : 1 << 8);
	res |= (processed_s2 == cnt_s2 && cnt_s2 == 3 ? 0 : 1 << 9);
	/*
	 * sms_cpy() to stack with small stack allocation size
	 */
	sms_cpy(&s_a3, s_i32);
	res |= (sms_size(s_a3) == 3 ? 0 : 1 << 10);
	sms_cpy(&s_a3, s_u32);
	res |= (sms_size(s_a3) == 3 ? 0 : 1 << 11);
	sms_cpy(&s_a3, s_i);
	res |= (sms_size(s_a3) == 0 ? 0 : 1 << 12);
	sms_cpy(&s_a3, s_s);
	res |= (sms_size(s_a3) == 0 ? 0 : 1 << 13);
	/*
	 * sms_cpy() to stack with enough stack allocation size
	 */
	sms_cpy(&s_a10, s_i32);
	res |= (sms_size(s_a10) == 3 ? 0 : 1 << 14);
	sms_cpy(&s_a10, s_u32);
	res |= (sms_size(s_a10) == 3 ? 0 : 1 << 15);
	sms_cpy(&s_a10, s_i);
	res |= (sms_size(s_a10) == 3 ? 0 : 1 << 16);
	sms_cpy(&s_a10, s_s);
	res |= (sms_size(s_a10) == 3 ? 0 : 1 << 17);
	sms_clear(s_a10);
	res |= (sms_size(s_a10) == 0 ? 0 : 1 << 18);
	/*
	 * Reserve/grow
	 */
	sms_reserve(&s_i32, 1000);
	sms_grow(&s_u32, 1000 - sms_max_size(s_u32));
	res |= (sms_capacity(s_i32) >= 1000 && sms_capacity(s_u32) >= 1000
				&& sms_capacity_left(s_i32) >= 997
				&& sms_capacity_left(s_u32) >= 997
			? 0
			: 1 << 19);
	/*
	 * Shrink memory
	 */
	sms_shrink(&s_i32);
	sms_shrink(&s_u32);
	res |= (sms_capacity(s_i32) == 3 && sms_capacity(s_u32) == 3
				&& sms_capacity_left(s_i32) == 0
				&& sms_capacity_left(s_u32) == 0
			? 0
			: 1 << 20);
	/*
	 * Random access
	 */
	res |= (sms_it_i32(s_i32, 0) == 10 && sms_it_i32(s_i32, 1) == 11
				&& sms_it_i32(s_i32, 2) == 12
			? 0
			: 1 << 21);
	res |= (sms_it_u32(s_u32, 0) == 20 && sms_it_u32(s_u32, 1) == 21
				&& sms_it_u32(s_u32, 2) == 22
			? 0
			: 1 << 22);
	res |= (sms_it_i(s_i, 0) == 0 && sms_it_i(s_i, 1) == 1
				&& sms_it_i(s_i, 2) == 2
			? 0
			: 1 << 23);
	res |= (!ss_cmp(sms_it_s(s_s, 0), k[0])
				&& !ss_cmp(sms_it_s(s_s, 1), k[1])
				&& !ss_cmp(sms_it_s(s_s, 2), k[2])
			? 0
			: 1 << 24);
	/*
	 * Delete
	 */
	sms_delete_i32(s_i32, 10);
	sms_delete_u32(s_u32, 20);
	sms_delete_i(s_i, 0);
	sms_delete_s(s_s, k[1]);
	res |= (!sms_count_i32(s_i32, 10) && !sms_count_i32(s_u32, 20)
				&& !sms_count_i(s_i, 0)
				&& !sms_count_s(s_s, k[1])
			? 0
			: 1 << 25);
	/*
	 * Release memory
	 */
#ifdef S_USE_VA_ARGS
	sms_free(&s_i32, &s_u32, &s_i, &s_f, &s_d, &s_s, &s_s2, &s_a3, &s_a10);
#else
	sms_free(&s_i32);
	sms_free(&s_u32);
	sms_free(&s_i);
	sms_free(&s_f);
	sms_free(&s_d);
	sms_free(&s_s);
	sms_free(&s_s2);
	sms_free(&s_a3);
	sms_free(&s_a10);
#endif
	return res;
}

static int test_shs()
{
	int res = 0;
	size_t i, tcount = 3, cnt_i32 = 0, cnt_u32 = 0, cnt_i = 0, cnt_f = 0,
		  cnt_d = 0, cnt_s = 0, cnt_s2 = 0, processed_i32,
		  processed_u32, processed_i, processed_f, processed_d,
		  processed_s, processed_s2;
	const srt_string *k[] = {ss_crefa("k000"), ss_crefa("k001"),
				 ss_crefa("k002")};
	/*
	 * Allocation: heap, stack with 3 elements, and stack with 10 elements
	 */
	srt_hset *s_i32 = shs_alloc(SHS_I32, 0), *s_u32 = shs_alloc(SHS_U32, 0),
		 *s_i = shs_alloc(SHS_I, 0), *s_f = shs_alloc(SHS_F, 0),
		 *s_d = shs_alloc(SHS_D, 0), *s_s = shs_alloc(SHS_S, 0),
		 *s_s2 = NULL, *s_a3 = shs_alloca(SHS_I32, 3),
		 *s_a10 = shs_alloca(SHS_I32, 10);
	res |= (shs_empty(s_i32) && shs_empty(s_u32) && shs_empty(s_i)
				&& shs_empty(s_f) && shs_empty(s_d)
				&& shs_empty(s_s) && shs_empty(s_a3)
				&& shs_empty(s_a10)
			? 0
			: 1 << 0);
	/*
	 * Insert elements
	 */
	for (i = 0; i < tcount; i++) {
		shs_insert_i32(&s_i32, (uint32_t)i + 10);
		shs_insert_u32(&s_u32, (uint32_t)i + 20);
		shs_insert_i(&s_i, i);
		shs_insert_f(&s_f, (float)i);
		shs_insert_d(&s_d, (double)i);
		shs_insert_s(&s_s, k[i]);
	}
	res |= (!shs_empty(s_i32) && !shs_empty(s_u32) && !shs_empty(s_i)
				&& !shs_empty(s_f) && !shs_empty(s_d)
				&& !shs_empty(s_s) && shs_empty(s_a3)
				&& shs_empty(s_a10)
			? 0
			: 1 << 1);
	res |= (shs_count_i32(s_i32, 10) && shs_count_i32(s_i32, 11)
				&& shs_count_i32(s_i32, 12)
				&& shs_count_u32(s_u32, 20)
				&& shs_count_u32(s_u32, 21)
				&& shs_count_u32(s_u32, 22)
				&& shs_count_i(s_i, 0) && shs_count_i(s_i, 1)
				&& shs_count_i(s_i, 2) && shs_count_f(s_f, 0)
				&& shs_count_f(s_f, 1) && shs_count_f(s_f, 2)
				&& shs_count_d(s_d, 0) && shs_count_d(s_d, 1)
				&& shs_count_d(s_d, 2) && shs_count_s(s_s, k[0])
				&& shs_count_s(s_s, k[1])
				&& shs_count_s(s_s, k[2])
			? 0
			: 1 << 2);
	/*
	 * Enumeration
	 */
	s_s2 = shs_dup(s_s);
	processed_i32 = shs_itp_i32(s_i32, 0, S_NPOS, cback_i32, &cnt_i32);
	processed_u32 = shs_itp_u32(s_u32, 0, S_NPOS, cback_u32, &cnt_u32);
	processed_i = shs_itp_i(s_i, 0, S_NPOS, cback_i, &cnt_i);
	processed_f = shs_itp_f(s_f, 0, S_NPOS, cback_f, &cnt_f);
	processed_d = shs_itp_d(s_d, 0, S_NPOS, cback_d, &cnt_d);
	processed_s = shs_itp_s(s_s, 0, S_NPOS, cback_s, &cnt_s);
	processed_s2 = shs_itp_s(s_s2, 0, S_NPOS, cback_s, &cnt_s2);
	res |= (processed_i32 == cnt_i32 && cnt_i32 == tcount ? 0 : 1 << 3);
	res |= (processed_u32 == cnt_u32 && cnt_u32 == tcount ? 0 : 1 << 4);
	res |= (processed_i == cnt_i && cnt_i == tcount ? 0 : 1 << 5);
	res |= (processed_f == cnt_f && cnt_f == tcount ? 0 : 1 << 6);
	res |= (processed_d == cnt_d && cnt_d == tcount ? 0 : 1 << 7);
	res |= (processed_s == cnt_s && cnt_s == tcount ? 0 : 1 << 8);
	res |= (processed_s2 == cnt_s2 && cnt_s2 == tcount ? 0 : 1 << 9);
	/*
	 * shs_cpy() to stack with small stack allocation size
	 */
	shs_cpy(&s_a3, s_i32);
	res |= (shs_size(s_a3) == 3 ? 0 : 1 << 10);
	shs_cpy(&s_a3, s_u32);
	res |= (shs_size(s_a3) == 3 ? 0 : 1 << 11);
	shs_cpy(&s_a3, s_i);
	res |= (shs_size(s_a3) == 0 ? 0 : 1 << 12);
	shs_cpy(&s_a3, s_s);
	res |= (shs_size(s_a3) == 0 ? 0 : 1 << 13);
	/*
	 * shs_cpy() to stack with enough stack allocation size
	 */
	shs_cpy(&s_a10, s_i32);
	res |= (shs_size(s_a10) == 3 ? 0 : 1 << 14);
	shs_cpy(&s_a10, s_u32);
	res |= (shs_size(s_a10) == 3 ? 0 : 1 << 15);
	shs_cpy(&s_a10, s_i);
	res |= (shs_size(s_a10) == 3 ? 0 : 1 << 16);
	shs_cpy(&s_a10, s_s);
	res |= (shs_size(s_a10) == 3 ? 0 : 1 << 17);
	shs_clear(s_a10);
	res |= (shs_size(s_a10) == 0 ? 0 : 1 << 18);
	/*
	 * Reserve/grow
	 */
	shs_reserve(&s_i32, 1000);
	shs_grow(&s_u32, 1000 - shs_max_size(s_u32));
	res |= (shs_capacity(s_i32) >= 1000 && shs_capacity(s_u32) >= 1000
				&& shs_capacity_left(s_i32) >= 997
				&& shs_capacity_left(s_u32) >= 997
			? 0
			: 1 << 19);
	/*
	 * Shrink memory
	 */
	shs_shrink(&s_i32);
	shs_shrink(&s_u32);
	res |= (shs_capacity(s_i32) == 3 && shs_capacity(s_u32) == 3
				&& shs_capacity_left(s_i32) == 0
				&& shs_capacity_left(s_u32) == 0
			? 0
			: 1 << 20);
	/*
	 * Random access
	 */
	res |= (shs_it_i32(s_i32, 0) == 10 && shs_it_i32(s_i32, 1) == 11
				&& shs_it_i32(s_i32, 2) == 12
			? 0
			: 1 << 21);
	res |= (shs_it_u32(s_u32, 0) == 20 && shs_it_u32(s_u32, 1) == 21
				&& shs_it_u32(s_u32, 2) == 22
			? 0
			: 1 << 22);
	res |= (shs_it_i(s_i, 0) == 0 && shs_it_i(s_i, 1) == 1
				&& shs_it_i(s_i, 2) == 2
			? 0
			: 1 << 23);
	res |= (!ss_cmp(shs_it_s(s_s, 0), k[0])
				&& !ss_cmp(shs_it_s(s_s, 1), k[1])
				&& !ss_cmp(shs_it_s(s_s, 2), k[2])
			? 0
			: 1 << 24);
	/*
	 * Delete
	 */
	shs_delete_i32(s_i32, 10);
	shs_delete_u32(s_u32, 20);
	shs_delete_i(s_i, 0);
	shs_delete_s(s_s, k[1]);
	res |= (!shs_count_i32(s_i32, 10) && !shs_count_u32(s_u32, 20)
				&& !shs_count_i(s_i, 0)
				&& !shs_count_s(s_s, k[1])
			? 0
			: 1 << 25);
	/*
	 * Release memory
	 */
#ifdef S_USE_VA_ARGS
	shs_free(&s_i32, &s_u32, &s_i, &s_f, &s_d, &s_s, &s_s2, &s_a3, &s_a10);
#else
	shs_free(&s_i32);
	shs_free(&s_u32);
	shs_free(&s_i);
	shs_free(&s_f);
	shs_free(&s_d);
	shs_free(&s_s);
	shs_free(&s_s2);
	shs_free(&s_a3);
	shs_free(&s_a10);
#endif
	return res;
}

#define TEST_SHM_ALLOC_DONOTHING(a)
#define TEST_SHM_ALLOC_X(fn, shm_alloc_X, type, insert, at, shm_free_X)        \
	static int fn()                                                        \
	{                                                                      \
		size_t n = 1000;                                               \
		srt_hmap *m = shm_alloc_X(type, n);                            \
		int res = 0;                                                   \
		for (;;) {                                                     \
			if (!m) {                                              \
				res = 1;                                       \
				break;                                         \
			}                                                      \
			if (!shm_empty(m) || shm_capacity(m) != n              \
			    || shm_capacity_left(m) != n) {                    \
				res = 2;                                       \
				break;                                         \
			}                                                      \
			insert(&m, 1, 1001);                                   \
			if (shm_size(m) != 1) {                                \
				res = 3;                                       \
				break;                                         \
			}                                                      \
			insert(&m, 2, 1002);                                   \
			insert(&m, 3, 1003);                                   \
			if (shm_empty(m) || shm_capacity(m) != n               \
			    || shm_capacity_left(m) != n - 3) {                \
				res = 4;                                       \
				break;                                         \
			}                                                      \
			if (at(m, 1) != 1001) {                                \
				res = 5;                                       \
				break;                                         \
			}                                                      \
			if (at(m, 2) != 1002) {                                \
				res = 6;                                       \
				break;                                         \
			}                                                      \
			if (at(m, 3) != 1003) {                                \
				res = 7;                                       \
				break;                                         \
			}                                                      \
			break;                                                 \
		}                                                              \
		if (!res) {                                                    \
			shm_clear(m);                                          \
			res = !shm_size(m) ? 0 : 8;                            \
		}                                                              \
		shm_free_X(&m);                                                \
		return res;                                                    \
	}

TEST_SHM_ALLOC_X(test_shm_alloc_ii32, shm_alloc, SHM_II32, shm_insert_ii32,
		 shm_at_ii32, shm_free)
TEST_SHM_ALLOC_X(test_shm_alloca_ii32, shm_alloca, SHM_II32, shm_insert_ii32,
		 shm_at_ii32, TEST_SHM_ALLOC_DONOTHING)
TEST_SHM_ALLOC_X(test_shm_alloc_uu32, shm_alloc, SHM_UU32, shm_insert_uu32,
		 shm_at_uu32, shm_free)
TEST_SHM_ALLOC_X(test_shm_alloca_uu32, shm_alloca, SHM_UU32, shm_insert_uu32,
		 shm_at_uu32, TEST_SHM_ALLOC_DONOTHING)
TEST_SHM_ALLOC_X(test_shm_alloc_ii, shm_alloc, SHM_II, shm_insert_ii, shm_at_ii,
		 shm_free)
TEST_SHM_ALLOC_X(test_shm_alloca_ii, shm_alloca, SHM_II, shm_insert_ii,
		 shm_at_ii, TEST_SHM_ALLOC_DONOTHING)
TEST_SHM_ALLOC_X(test_shm_alloc_ff, shm_alloc, SHM_FF, shm_insert_ff, shm_at_ff,
		 shm_free)
TEST_SHM_ALLOC_X(test_shm_alloca_ff, shm_alloca, SHM_FF, shm_insert_ff,
		 shm_at_ff, TEST_SHM_ALLOC_DONOTHING)
TEST_SHM_ALLOC_X(test_shm_alloc_dd, shm_alloc, SHM_DD, shm_insert_dd, shm_at_dd,
		 shm_free)
TEST_SHM_ALLOC_X(test_shm_alloca_dd, shm_alloca, SHM_DD, shm_insert_dd,
		 shm_at_dd, TEST_SHM_ALLOC_DONOTHING)

#define TEST_SHM_SHRINK_TO_FIT_VARS(m, atype, r)                               \
	srt_hmap *m = shm_alloc(atype, r), *m##2 = shm_alloca(atype, r)

#define TEST_SHM_SHRINK_TO_FIT(m, ntest, atype, r)                             \
	res |= !m ? 1 << (ntest * 4)                                           \
		  : !m##2 ? 2 << (ntest * 4)                                   \
			  : (shm_max_size(m) == r ? 0 : 3 << (ntest * 4))      \
				       | (shm_max_size(m##2) == r              \
						  ? 0                          \
						  : 4 << (ntest * 4))          \
				       | (shm_shrink(                          \
						  &m) && shm_max_size(m) == 0  \
						  ? 0                          \
						  : 5 << (ntest * 4))          \
				       | (shm_shrink(&m##2)                    \
							  && shm_max_size(     \
								     m##2)     \
								     == r      \
						  ? 0                          \
						  : 6 << (ntest * 4));         \
	shm_free(&m)

static int test_shm_shrink()
{
	TEST_SHM_SHRINK_TO_FIT_VARS(aa, SHM_II32, 10);
	TEST_SHM_SHRINK_TO_FIT_VARS(bb, SHM_UU32, 10);
	TEST_SHM_SHRINK_TO_FIT_VARS(cc, SHM_II, 10);
	TEST_SHM_SHRINK_TO_FIT_VARS(dd, SHM_IS, 10);
	TEST_SHM_SHRINK_TO_FIT_VARS(ee, SHM_IP, 10);
	TEST_SHM_SHRINK_TO_FIT_VARS(ff, SHM_SI, 10);
	TEST_SHM_SHRINK_TO_FIT_VARS(gg, SHM_SS, 10);
	TEST_SHM_SHRINK_TO_FIT_VARS(hh, SHM_SP, 10);
	TEST_SHM_SHRINK_TO_FIT_VARS(ii, SHM_FF, 10);
	TEST_SHM_SHRINK_TO_FIT_VARS(jj, SHM_DD, 10);
	TEST_SHM_SHRINK_TO_FIT_VARS(kk, SHM_DS, 10);
	TEST_SHM_SHRINK_TO_FIT_VARS(ll, SHM_DP, 10);
	TEST_SHM_SHRINK_TO_FIT_VARS(mm, SHM_SD, 10);
	int res = 0;
	TEST_SHM_SHRINK_TO_FIT(aa, 0, SHM_II32, 10);
	TEST_SHM_SHRINK_TO_FIT(bb, 1, SHM_UU32, 10);
	TEST_SHM_SHRINK_TO_FIT(cc, 2, SHM_II, 10);
	TEST_SHM_SHRINK_TO_FIT(dd, 3, SHM_IS, 10);
	TEST_SHM_SHRINK_TO_FIT(ee, 4, SHM_IP, 10);
	TEST_SHM_SHRINK_TO_FIT(ff, 5, SHM_SI, 10);
	TEST_SHM_SHRINK_TO_FIT(gg, 6, SHM_SS, 10);
	TEST_SHM_SHRINK_TO_FIT(hh, 7, SHM_SP, 10);
	TEST_SHM_SHRINK_TO_FIT(ii, 2, SHM_FF, 10);
	TEST_SHM_SHRINK_TO_FIT(jj, 2, SHM_DD, 10);
	TEST_SHM_SHRINK_TO_FIT(kk, 2, SHM_DS, 10);
	TEST_SHM_SHRINK_TO_FIT(ll, 2, SHM_DP, 10);
	TEST_SHM_SHRINK_TO_FIT(mm, 2, SHM_SD, 10);
	return res;
}

#define TEST_SHM_DUP_VARS(m, atype, r)                                         \
	srt_hmap *m = shm_alloc(atype, r), *m##2 = shm_alloca(atype, r),       \
		 *m##b = NULL, *m##2b = NULL

#define TEST_SHM_DUP(m, ntest, atype, insf, kv, vv, atf, cmpf, r)              \
	res |= !m || !m##2 ? 1 << (ntest * 2) : 0;                             \
	if (!res) {                                                            \
		int j = 0;                                                     \
		for (; j < r; j++) {                                           \
			srt_bool b1 = insf(&m, kv[j], vv[j]);                  \
			srt_bool b2 = insf(&m##2, kv[j], vv[j]);               \
			if (!b1 || !b2) {                                      \
				res |= 2 << (ntest * 2);                       \
				break;                                         \
			}                                                      \
		}                                                              \
	}                                                                      \
	if (!res) {                                                            \
		m##b = shm_dup(m);                                             \
		m##2b = shm_dup(m##2);                                         \
		res = (!m##b || !m##2b)                                        \
			      ? 4 << (ntest * 2)                               \
			      : (shm_size(m) != r || shm_size(m##2) != r       \
				 || shm_size(m##b) != r                        \
				 || shm_size(m##2b) != r)                      \
					? 2 << (ntest * 2)                     \
					: 0;                                   \
	}                                                                      \
	if (!res) {                                                            \
		int j = 0;                                                     \
		for (; j < r; j++) {                                           \
			if (cmpf(atf(m, kv[j]), atf(m##2, kv[j]))              \
			    || cmpf(atf(m, kv[j]), atf(m##b, kv[j]))           \
			    || cmpf(atf(m##b, kv[j]), atf(m##2b, kv[j]))) {    \
				res |= 3 << (ntest * 2);                       \
				break;                                         \
			}                                                      \
		}                                                              \
	}                                                                      \
	shm_free(&m);                                                          \
	shm_free(&m##2);                                                       \
	shm_free(&m##b);                                                       \
	shm_free(&m##2b)

/*
 * Covers: shm_dup, shm_insert_ii32, shm_insert_uu32, shm_insert_ii,
 * shm_insert_is, shm_insert_ip, shm_insert_si, shm_insert_ss, shm_insert_sp,
 * shm_at_ii32, shm_at_uu32, shm_at_ii, shm_at_is, shm_at_ip, shm_at_si,
 * shm_at_ss, shm_at_sp
 */
static int test_shm_dup()
{
	TEST_SHM_DUP_VARS(aa, SHM_II32, 10);
	TEST_SHM_DUP_VARS(bb, SHM_UU32, 10);
	TEST_SHM_DUP_VARS(cc, SHM_II, 10);
	TEST_SHM_DUP_VARS(dd, SHM_IS, 10);
	TEST_SHM_DUP_VARS(ee, SHM_IP, 10);
	TEST_SHM_DUP_VARS(ff, SHM_SI, 10);
	TEST_SHM_DUP_VARS(gg, SHM_SS, 10);
	TEST_SHM_DUP_VARS(hh, SHM_SP, 10);
	TEST_SHM_DUP_VARS(ii, SHM_FF, 10);
	TEST_SHM_DUP_VARS(jj, SHM_DD, 10);
	TEST_SHM_DUP_VARS(kk, SHM_DS, 10);
	TEST_SHM_DUP_VARS(ll, SHM_DP, 10);
	TEST_SHM_DUP_VARS(mm, SHM_SD, 10);
	int i = 0, res = 0;
	srt_string *s = ss_dup_c("hola1"), *s2 = ss_alloca(100);
	srt_string *ssk[10], *ssv[10];
	ss_cpy_c(&s2, "hola2");
	for (; i < 10; i++) {
		ssk[i] = ss_alloca(100);
		ssv[i] = ss_alloca(100);
		ss_printf(&ssk[i], 50, "key_%i", i);
		ss_printf(&ssv[i], 50, "val_%i", i);
	}
	TEST_SHM_DUP(aa, 0, SHM_II32, shm_insert_ii32, i32k, i32v, shm_at_ii32,
		     scmp_int32, 10);
	TEST_SHM_DUP(bb, 1, SHM_UU32, shm_insert_uu32, u32k, u32v, shm_at_uu32,
		     scmp_uint32, 10);
	TEST_SHM_DUP(cc, 2, SHM_II, shm_insert_ii, sik, i64v, shm_at_ii, scmp_i,
		     10);
	TEST_SHM_DUP(dd, 3, SHM_IS, shm_insert_is, sik, ssv, shm_at_is, ss_cmp,
		     10);
	TEST_SHM_DUP(ee, 4, SHM_IP, shm_insert_ip, sik, spv, shm_at_ip,
		     scmp_ptr, 10);
	TEST_SHM_DUP(ff, 5, SHM_SI, shm_insert_si, ssk, i64v, shm_at_si, scmp_i,
		     10);
	TEST_SHM_DUP(gg, 6, SHM_SS, shm_insert_ss, ssk, ssv, shm_at_ss, ss_cmp,
		     10);
	TEST_SHM_DUP(hh, 7, SHM_SP, shm_insert_sp, ssk, spv, shm_at_sp,
		     scmp_ptr, 10);
	TEST_SHM_DUP(ii, 8, SHM_FF, shm_insert_ff, sfk, fv, shm_at_ff, scmp_f,
		     10);
	TEST_SHM_DUP(jj, 9, SHM_DD, shm_insert_dd, sdk, dv, shm_at_dd, scmp_d,
		     10);
	TEST_SHM_DUP(kk, 10, SHM_DS, shm_insert_ds, sdk, ssv, shm_at_ds, ss_cmp,
		     10);
	TEST_SHM_DUP(ll, 11, SHM_DP, shm_insert_dp, sdk, spv, shm_at_dp,
		     scmp_ptr, 10);
	TEST_SHM_DUP(mm, 12, SHM_SD, shm_insert_sd, ssk, dv, shm_at_sd, scmp_d,
		     10);
	ss_free(&s);
	return res;
}

static int test_shm_cpy()
{
	int res = 0;
	srt_hmap *m1 = shm_alloc(SHM_II32, 0), *m2 = shm_alloc(SHM_SS, 0),
		 *m1a3 = shm_alloca(SHM_II32, 3),
		 *m1a10 = shm_alloca(SHM_II32, 10);
	const srt_string *k1 = ss_crefa("key1"), *k2 = ss_crefa("key2"),
			 *k3 = ss_crefa("key3"), *v1 = ss_crefa("val1"),
			 *v2 = ss_crefa("val2"), *v3 = ss_crefa("val3");
	shm_insert_ii32(&m1, 1, 1);
	shm_insert_ii32(&m1, 2, 2);
	shm_insert_ii32(&m1, 3, 3);
	shm_insert_ss(&m2, k1, v1);
	shm_insert_ss(&m2, k2, v2);
	shm_insert_ss(&m2, k3, v3);
	shm_cpy(&m1, m2);
	shm_cpy(&m1a3, m2);
	shm_cpy(&m1a10, m2);
	res |= (shm_size(m1) == 3 ? 0 : 1);
#ifdef S_ENABLE_SM_STRING_OPTIMIZATION
	/*
	 * TODO: adjust test to the node size of the string optimized map mode
	 */
#else
	if (sizeof(uint32_t) == sizeof(void *))
		res |= (shm_size(m1a3) == 3 ? 0 : 2);
	else
		res |= (shm_size(m1a3) < 3 ? 0 : 4);
	res |= (shm_size(m1a10) == 3 ? 0 : 8);
	res |= (!ss_cmp(shm_it_s_k(m1, 0), shm_it_s_k(m2, 0)) ? 0 : 16);
	res |= (!ss_cmp(shm_it_s_k(m1, 1), shm_it_s_k(m2, 1)) ? 0 : 32);
	res |= (!ss_cmp(shm_it_s_k(m1, 2), shm_it_s_k(m2, 2)) ? 0 : 64);
	res |= (!ss_cmp(shm_it_s_k(m1, 0), shm_it_s_k(m1a10, 0)) ? 0 : 128);
	res |= (!ss_cmp(shm_it_s_k(m1, 1), shm_it_s_k(m1a10, 1)) ? 0 : 256);
	res |= (!ss_cmp(shm_it_s_k(m1, 2), shm_it_s_k(m1a10, 2)) ? 0 : 512);
#endif
#ifdef S_USE_VA_ARGS
	shm_free(&m1, &m2, &m1a3, &m1a10);
#else
	shm_free(&m1);
	shm_free(&m2);
	shm_free(&m1a3);
	shm_free(&m1a10);
#endif
	return res;
}

#define TEST_SHM_X_COUNT(T, insf, cntf, v)                                     \
	int res = 0;                                                           \
	uint32_t i, tcount = 100;                                              \
	srt_hmap *m = shm_alloc(T, tcount);                                    \
	for (i = 0; i < tcount; i++)                                           \
		insf(&m, (unsigned)i, v);                                      \
	for (i = 0; i < tcount; i++)                                           \
		if (!cntf(m, i)) {                                             \
			res = 1 + (int)i;                                      \
			break;                                                 \
		}                                                              \
	shm_free(&m);                                                          \
	return res;

static int test_shm_count_u()
{
	TEST_SHM_X_COUNT(SHM_UU32, shm_insert_uu32, shm_count_u32, 1);
}

static int test_shm_count_i()
{
	TEST_SHM_X_COUNT(SHM_II32, shm_insert_ii32, shm_count_i32, 1);
}

static int test_shm_count_s()
{
	int res;
	srt_string *s = ss_dup_c("a_1"), *t = ss_dup_c("a_2"),
		   *u = ss_dup_c("a_3");
	srt_hmap *m = shm_alloc(SHM_SI, 3);
	shm_insert_si(&m, s, 1);
	shm_insert_si(&m, t, 2);
	shm_insert_si(&m, u, 3);
	res = shm_count_s(m, s) && shm_count_s(m, t) && shm_count_s(m, u) ? 0
									  : 1;
#ifdef S_USE_VA_ARGS
	ss_free(&s, &t, &u);
#else
	ss_free(&s);
	ss_free(&t);
	ss_free(&u);
#endif
	shm_free(&m);
	return res;
}

static int test_shm_inc_ii32()
{
	int res;
	srt_hmap *m = shm_alloc(SHM_II32, 0);
	shm_inc_ii32(&m, 123, -10);
	shm_inc_ii32(&m, 123, -20);
	shm_inc_ii32(&m, 123, -30);
	res = !m ? 1 : (shm_at_ii32(m, 123) == -60 ? 0 : 2);
	shm_free(&m);
	return res;
}

static int test_shm_inc_uu32()
{
	int res;
	srt_hmap *m = shm_alloc(SHM_UU32, 0);
	shm_inc_uu32(&m, 123, 10);
	shm_inc_uu32(&m, 123, 20);
	shm_inc_uu32(&m, 123, 30);
	res = !m ? 1 : (shm_at_uu32(m, 123) == 60 ? 0 : 2);
	shm_free(&m);
	return res;
}

static int test_shm_inc_ii()
{
	int res;
	srt_hmap *m = shm_alloc(SHM_II, 0);
	shm_inc_ii(&m, 123, -7);
	shm_inc_ii(&m, 123, S_MAX_I64);
	shm_inc_ii(&m, 123, 3);
	res = !m ? 1 : (shm_at_ii(m, 123) == S_MAX_I64 - 4 ? 0 : 2);
	shm_free(&m);
	return res;
}

static int test_shm_inc_si()
{
	int res;
	srt_hmap *m = shm_alloc(SHM_SI, 0);
	const srt_string *k = ss_crefa("hello");
	shm_inc_si(&m, k, -7);
	shm_inc_si(&m, k, S_MAX_I64);
	shm_inc_si(&m, k, 3);
	res = !m ? 1 : (shm_at_si(m, k) == S_MAX_I64 - 4 ? 0 : 2);
	shm_free(&m);
	return res;
}

static int test_shm_delete_i()
{
	int res;
	srt_hmap *m_ii32 = shm_alloc(SHM_II32, 0),
		 *m_uu32 = shm_alloc(SHM_UU32, 0), *m_ii = shm_alloc(SHM_II, 0);
	/*
	 * Insert elements
	 */
	shm_insert_ii32(&m_ii32, -1, -1);
	shm_insert_ii32(&m_ii32, -2, -2);
	shm_insert_ii32(&m_ii32, -3, -3);
	shm_insert_uu32(&m_uu32, 1, 1);
	shm_insert_uu32(&m_uu32, 2, 2);
	shm_insert_uu32(&m_uu32, 3, 3);
	shm_insert_ii(&m_ii, S_MAX_I64, S_MAX_I64);
	shm_insert_ii(&m_ii, S_MAX_I64 - 1, S_MAX_I64 - 1);
	shm_insert_ii(&m_ii, S_MAX_I64 - 2, S_MAX_I64 - 2);
	/*
	 * Check elements were properly inserted
	 */
	res = m_ii32 && m_uu32 && m_ii ? 0 : 1;
	res |= (shm_at_ii32(m_ii32, -2) == -2 ? 0 : 2);
	res |= (shm_at_uu32(m_uu32, 2) == 2 ? 0 : 4);
	res |= (shm_at_ii(m_ii, S_MAX_I64 - 1) == S_MAX_I64 - 1 ? 0 : 8);
	/*
	 * Delete elements, checking that second deletion of same
	 * element gives error
	 */
	res |= shm_delete_i32(m_ii32, -2) ? 0 : 16;
	res |= !shm_delete_i32(m_ii32, -2) ? 0 : 32;
	res |= shm_delete_u32(m_uu32, 2) ? 0 : 64;
	res |= !shm_delete_u32(m_uu32, 2) ? 0 : 128;
	res |= shm_delete_i(m_ii, S_MAX_I64 - 1) ? 0 : 256;
	res |= !shm_delete_i(m_ii, S_MAX_I64 - 1) ? 0 : 512;
	/*
	 * Check that querying for deleted elements gives default value
	 */
	res |= (shm_at_ii32(m_ii32, -2) == 0 ? 0 : 1024);
	res |= (shm_at_uu32(m_uu32, 2) == 0 ? 0 : 2048);
	res |= (shm_at_ii(m_ii, S_MAX_I64 - 1) == 0 ? 0 : 4096);
#ifdef S_USE_VA_ARGS
	shm_free(&m_ii32, &m_uu32, &m_ii);
#else
	shm_free(&m_ii32);
	shm_free(&m_uu32);
	shm_free(&m_ii);
#endif
	return res;
}

static int test_shm_delete_s()
{
	int res;
	srt_hmap *m_si = shm_alloc(SHM_SI, 0), *m_sp = shm_alloc(SHM_SP, 0),
		 *m_ss = shm_alloc(SHM_SS, 0);
	/*
	 * Insert elements
	 */
	const srt_string *k1 = ss_crefa("key1"), *k2 = ss_crefa("key2"),
			 *k3 = ss_crefa("key3"), *v1 = ss_crefa("val1"),
			 *v2 = ss_crefa("val2"), *v3 = ss_crefa("val3");
	shm_insert_si(&m_si, k1, -1);
	shm_insert_si(&m_si, k2, S_MAX_I64);
	shm_insert_si(&m_si, k3, -3);
	shm_insert_sp(&m_sp, k1, (void *)-1);
	shm_insert_sp(&m_sp, k2, (void *)-2);
	shm_insert_sp(&m_sp, k3, (void *)-3);
	shm_insert_ss(&m_ss, k1, v1);
	shm_insert_ss(&m_ss, k2, v2);
	shm_insert_ss(&m_ss, k3, v3);
	/*
	 * Check elements were properly inserted
	 */
	res = m_si && m_sp && m_ss ? 0 : 1;
	res |= (shm_at_si(m_si, k2) == S_MAX_I64 ? 0 : 2);
	res |= (shm_at_sp(m_sp, k2) == (void *)-2 ? 0 : 4);
	res |= (!ss_cmp(shm_at_ss(m_ss, k2), v2) ? 0 : 8);
	/*
	 * Delete elements, checking that second deletion of same
	 * element gives error
	 */
	res |= shm_delete_s(m_si, k2) ? 0 : 16;
	res |= !shm_delete_s(m_si, k2) ? 0 : 32;
	res |= shm_delete_s(m_sp, k2) ? 0 : 64;
	res |= !shm_delete_s(m_sp, k2) ? 0 : 128;
	res |= shm_delete_s(m_ss, k2) ? 0 : 256;
	res |= !shm_delete_s(m_ss, k2) ? 0 : 512;
	/*
	 * Check that querying for deleted elements gives default value
	 */
	res |= (shm_at_si(m_si, k2) == 0 ? 0 : 1024);
	res |= (shm_at_sp(m_sp, k2) == 0 ? 0 : 2048);
	res |= (!ss_cmp(shm_at_ss(m_ss, k2), ss_void) ? 0 : 4096);
#ifdef S_USE_VA_ARGS
	shm_free(&m_si, &m_sp, &m_ss);
#else
	shm_free(&m_si);
	shm_free(&m_sp);
	shm_free(&m_ss);
#endif
	return res;
}

#define TEST_SHM_IT_X_VARS(id, et)                                             \
	srt_hmap *m_##id = shm_alloc(et, 0), *m_a##id = shm_alloca(et, 3)

#define TEST_SHM_IT_X(n, id, et, itk, itv, cmpkf, cmpvf, k1, v1, k2, v2, k3,   \
		      v3, res)                                                 \
	shm_insert_##id(&m_##id, k1, v1);                                      \
	shm_insert_##id(&m_a##id, k1, v1);                                     \
	res |= (!cmpkf(itk(m_##id, 0), k1) && !cmpvf(itv(m_##id, 0), v1)) ? 0  \
									  : n; \
	shm_insert_##id(&m_##id, k2, v2);                                      \
	shm_insert_##id(&m_##id, k3, v3);                                      \
	shm_insert_##id(&m_a##id, k2, v2);                                     \
	shm_insert_##id(&m_a##id, k3, v3);                                     \
	res |= (!cmpvf(shm_at_##id(m_##id, k1), v1)                            \
		&& !cmpvf(shm_at_##id(m_##id, k2), v2)                         \
		&& !cmpvf(shm_at_##id(m_##id, k3), v3)                         \
		&& !cmpvf(shm_at_##id(m_a##id, k1), v1)                        \
		&& !cmpvf(shm_at_##id(m_a##id, k2), v2)                        \
		&& !cmpvf(shm_at_##id(m_a##id, k3), v3)                        \
		&& !cmpkf(itk(m_##id, 0), itk(m_a##id, 0))                     \
		&& !cmpkf(itk(m_##id, 1), itk(m_a##id, 1))                     \
		&& !cmpkf(itk(m_##id, 2), itk(m_a##id, 2))                     \
		&& !cmpvf(itv(m_##id, 0), itv(m_a##id, 0))                     \
		&& !cmpvf(itv(m_##id, 1), itv(m_a##id, 1))                     \
		&& !cmpvf(itv(m_##id, 2), itv(m_a##id, 2))                     \
		&& cmpkf(itk(m_##id, 0), 0) && cmpkf(itk(m_a##id, 0), 0)       \
		&& cmpkf(itk(m_##id, 1), 0) && cmpkf(itk(m_a##id, 1), 0)       \
		&& cmpkf(itk(m_##id, 2), 0) && cmpkf(itk(m_a##id, 2), 0)       \
		&& cmpvf(itv(m_##id, 0), 0) && cmpvf(itv(m_a##id, 0), 0)       \
		&& cmpvf(itv(m_##id, 1), 0) && cmpvf(itv(m_a##id, 1), 0)       \
		&& cmpvf(itv(m_##id, 2), 0) && cmpvf(itv(m_a##id, 2), 0))      \
		       ? 0                                                     \
		       : n;                                                    \
	shm_free(&m_##id);                                                     \
	shm_free(&m_a##id);

static int test_shm_it()
{
	TEST_SHM_IT_X_VARS(ii32, SHM_II32);
	TEST_SHM_IT_X_VARS(uu32, SHM_UU32);
	TEST_SHM_IT_X_VARS(ii, SHM_II);
	TEST_SHM_IT_X_VARS(is, SHM_IS);
	TEST_SHM_IT_X_VARS(ip, SHM_IP);
	TEST_SHM_IT_X_VARS(si, SHM_SI);
	TEST_SHM_IT_X_VARS(ss, SHM_SS);
	TEST_SHM_IT_X_VARS(sp, SHM_SP);
	TEST_SHM_IT_X_VARS(ff, SHM_FF);
	TEST_SHM_IT_X_VARS(dd, SHM_DD);
	TEST_SHM_IT_X_VARS(ds, SHM_DS);
	TEST_SHM_IT_X_VARS(dp, SHM_DP);
	TEST_SHM_IT_X_VARS(sd, SHM_SD);
	int res = 0;
	const srt_string *k1 = ss_crefa("key1"), *k2 = ss_crefa("key2"),
			 *k3 = ss_crefa("key3"), *v1 = ss_crefa("val1"),
			 *v2 = ss_crefa("val2"), *v3 = ss_crefa("val3");
	TEST_SHM_IT_X(1, ii32, SHM_II32, shm_it_i32_k, shm_it_ii32_v, scmp_i,
		      scmp_i, -1, -1, -2, -2, -3, -3, res);
	TEST_SHM_IT_X(2, uu32, SHM_UU32, shm_it_u32_k, shm_it_uu32_v, scmp_i,
		      scmp_i, 1, 1, 2, 2, 3, 3, res);
	TEST_SHM_IT_X(4, ii, SHM_II, shm_it_i_k, shm_it_ii_v, scmp_i, scmp_i,
		      S_MAX_I64, S_MIN_I64, S_MIN_I64, S_MAX_I64, S_MAX_I64 - 1,
		      S_MIN_I64 + 1, res);
	TEST_SHM_IT_X(8, is, SHM_IS, shm_it_i_k, shm_it_is_v, scmp_i, ss_cmp, 1,
		      v1, 2, v2, 3, v3, res);
	TEST_SHM_IT_X(16, ip, SHM_IP, shm_it_i_k, shm_it_ip_v, scmp_i, scmp_ptr,
		      1, (void *)1, 2, (void *)2, 3, (void *)3, res);
	TEST_SHM_IT_X(32, si, SHM_SI, shm_it_s_k, shm_it_si_v, ss_cmp, scmp_i,
		      v1, 1, v2, 2, v3, 3, res);
	TEST_SHM_IT_X(64, ss, SHM_SS, shm_it_s_k, shm_it_ss_v, ss_cmp, ss_cmp,
		      k1, v1, k2, v2, k3, v3, res);
	TEST_SHM_IT_X(128, sp, SHM_SP, shm_it_s_k, shm_it_sp_v, ss_cmp,
		      scmp_ptr, k1, (void *)1, k2, (void *)2, k3, (void *)3,
		      res);
	TEST_SHM_IT_X(1 << 9, ff, SHM_FF, shm_it_f_k, shm_it_ff_v, scmp_f,
		      scmp_f, 1, 1, 2, 2, 3, 3, res);
	TEST_SHM_IT_X(1 << 10, dd, SHM_DD, shm_it_d_k, shm_it_dd_v, scmp_d,
		      scmp_d, 1, 1, 2, 2, 3, 3, res);
	TEST_SHM_IT_X(1 << 11, ds, SHM_DS, shm_it_d_k, shm_it_ds_v, scmp_d,
		      ss_cmp, 1, v1, 2, v2, 3, v3, res);
	TEST_SHM_IT_X(1 << 12, dp, SHM_DP, shm_it_d_k, shm_it_dp_v, scmp_d,
		      scmp_ptr, 1, (void *)1, 2, (void *)2, 3, (void *)3, res);
	TEST_SHM_IT_X(1 << 13, sd, SHM_SD, shm_it_s_k, shm_it_sd_v, ss_cmp,
		      scmp_d, v1, 1, v2, 2, v3, 3, res);
	return res;
}

static int test_shm_itp()
{
	int i, res = 1;
	size_t nelems = 100, processed_ii321, processed_ii322, processed_uu32,
	       processed_ii, processed_ff, processed_dd, processed_is,
	       processed_ip, processed_si, processed_ds, processed_dp,
	       processed_sd, processed_ss1, processed_ss2, processed_sp, cnt1,
	       cnt2;
	srt_string *ktmp = ss_alloca(1000), *vtmp = ss_alloca(1000);
	srt_hmap *hm_ii32 = shm_alloc(SHM_II32, nelems),
		 *hm_uu32 = shm_alloc(SHM_UU32, nelems),
		 *hm_ii = shm_alloc(SHM_II, nelems),
		 *hm_ff = shm_alloc(SHM_FF, nelems),
		 *hm_dd = shm_alloc(SHM_DD, nelems),
		 *hm_is = shm_alloc(SHM_IS, nelems),
		 *hm_ip = shm_alloc(SHM_IP, nelems),
		 *hm_si = shm_alloc(SHM_SI, nelems),
		 *hm_ds = shm_alloc(SHM_DS, nelems),
		 *hm_dp = shm_alloc(SHM_DP, nelems),
		 *hm_sd = shm_alloc(SHM_SD, nelems),
		 *hm_ss = shm_alloc(SHM_SS, nelems),
		 *hm_sp = shm_alloc(SHM_SP, nelems);
	if (hm_ii32 && hm_uu32 && hm_ii && hm_ff && hm_dd && hm_is && hm_ip
	    && hm_si && hm_ds && hm_dp && hm_sd && hm_ss && hm_sp) {
		for (i = 0; i < (int)nelems; i++) {
			shm_insert_ii32(&hm_ii32, -i, -i);
			shm_insert_uu32(&hm_uu32, (uint32_t)i, (uint32_t)i);
			shm_insert_ii(&hm_ii, -i, -i);
			shm_insert_ff(&hm_ff, (float)-i, (float)-i);
			shm_insert_dd(&hm_dd, (double)-i, (double)-i);
			ss_printf(&ktmp, 200, "k%04i", i);
			ss_printf(&vtmp, 200, "v%04i", i);
			shm_insert_is(&hm_is, -i, vtmp);
			shm_insert_ip(&hm_ip, -i, (void *)(intptr_t)i);
			shm_insert_si(&hm_si, ktmp, i);
			shm_insert_ds(&hm_ds, (double)-i, vtmp);
			shm_insert_dp(&hm_dp, (double)-i, (void *)(intptr_t)i);
			shm_insert_sd(&hm_sd, ktmp, (double)i);
			shm_insert_ss(&hm_ss, ktmp, vtmp);
			shm_insert_sp(&hm_sp, ktmp, (void *)(intptr_t)i);
		}
		cnt1 = cnt2 = 0;
		processed_ii321 =
			shm_itp_ii32(hm_ii32, 0, S_NPOS, cback_i32i32, &cnt1);
		processed_ii322 = shm_itp_ii32(hm_ii32, 0, S_NPOS, NULL, NULL);
		processed_uu32 = shm_itp_uu32(hm_uu32, 0, S_NPOS, NULL, NULL);
		processed_ii = shm_itp_ii(hm_ii, 0, S_NPOS, NULL, NULL);
		processed_ff = shm_itp_ff(hm_ff, 0, S_NPOS, NULL, NULL);
		processed_dd = shm_itp_dd(hm_dd, 0, S_NPOS, NULL, NULL);
		processed_is = shm_itp_is(hm_is, 0, S_NPOS, NULL, NULL);
		processed_ip = shm_itp_ip(hm_ip, 0, S_NPOS, NULL, NULL);
		processed_si = shm_itp_si(hm_si, 0, S_NPOS, NULL, NULL);
		processed_ds = shm_itp_ds(hm_ds, 0, S_NPOS, NULL, NULL);
		processed_dp = shm_itp_dp(hm_dp, 0, S_NPOS, NULL, NULL);
		processed_sd = shm_itp_sd(hm_sd, 0, S_NPOS, NULL, NULL);
		processed_ss1 = shm_itp_ss(hm_ss, 0, S_NPOS, cback_ss, &cnt2);
		processed_ss2 = shm_itp_ss(hm_ss, 0, S_NPOS, NULL, NULL);
		processed_sp = shm_itp_sp(hm_sp, 0, S_NPOS, NULL, NULL);
		res = processed_ii321 == nelems ? 0 : 1 << 0;
		res |= processed_ii321 == processed_ii322 ? 0 : 1 << 2;
		res |= processed_ii322 == cnt1 ? 0 : 1 << 3;
		res |= processed_uu32 == cnt1 ? 0 : 1 << 4;
		res |= processed_ii == cnt1 ? 0 : 1 << 5;
		res |= processed_ff == cnt1 ? 0 : 1 << 6;
		res |= processed_dd == cnt1 ? 0 : 1 << 7;
		res |= processed_is == cnt1 ? 0 : 1 << 8;
		res |= processed_ip == cnt1 ? 0 : 1 << 9;
		res |= processed_si == nelems ? 0 : 1 << 10;
		res |= processed_ds == cnt1 ? 0 : 1 << 11;
		res |= processed_dp == cnt1 ? 0 : 1 << 12;
		res |= processed_sd == nelems ? 0 : 1 << 13;
		res |= processed_ss1 == nelems ? 0 : 1 << 14;
		res |= processed_ss1 == processed_ss2 ? 0 : 1 << 15;
		res |= processed_ss2 == cnt2 ? 0 : 1 << 16;
		res |= processed_sp == nelems ? 0 : 1 << 17;
		/*
		 * Wrong type checks
		 */
		res |= shm_itp_uu32(hm_ii32, 0, S_NPOS, NULL, NULL) > 0
			       ? 1 << 18
			       : 0;
		res |= shm_itp_ii(hm_ii32, 0, S_NPOS, NULL, NULL) > 0 ? 1 << 19
								      : 0;
		res |= shm_itp_ff(hm_ii32, 0, S_NPOS, NULL, NULL) > 0 ? 1 << 20
								      : 0;
		res |= shm_itp_dd(hm_ii32, 0, S_NPOS, NULL, NULL) > 0 ? 1 << 21
								      : 0;
		res |= shm_itp_is(hm_ii32, 0, S_NPOS, NULL, NULL) > 0 ? 1 << 22
								      : 0;
		res |= shm_itp_ip(hm_ii32, 0, S_NPOS, NULL, NULL) > 0 ? 1 << 23
								      : 0;
		res |= shm_itp_si(hm_ii32, 0, S_NPOS, NULL, NULL) > 0 ? 1 << 24
								      : 0;
		res |= shm_itp_ds(hm_ii32, 0, S_NPOS, NULL, NULL) > 0 ? 1 << 25
								      : 0;
		res |= shm_itp_dp(hm_ii32, 0, S_NPOS, NULL, NULL) > 0 ? 1 << 26
								      : 0;
		res |= shm_itp_sd(hm_ii32, 0, S_NPOS, NULL, NULL) > 0 ? 1 << 27
								      : 0;
		res |= shm_itp_sp(hm_ii32, 0, S_NPOS, NULL, NULL) > 0 ? 1 << 28
								      : 0;
	}
#ifdef S_USE_VA_ARGS
	shm_free(&hm_ii32, &hm_uu32, &hm_ii, &hm_ff, &hm_dd, &hm_is, &hm_ip,
		 &hm_si, &hm_ds, &hm_dp, &hm_sd, &hm_ss, &hm_sp);
#else
	shm_free(&hm_ii32);
	shm_free(&hm_uu32);
	shm_free(&hm_ii);
	shm_free(&hm_ff);
	shm_free(&hm_dd);
	shm_free(&hm_is);
	shm_free(&hm_ip);
	shm_free(&hm_si);
	shm_free(&hm_ds);
	shm_free(&hm_dp);
	shm_free(&hm_sd);
	shm_free(&hm_ss);
	shm_free(&hm_sp);
#endif
	return res;
}

static int test_tree_vs_hash()
{
	int i, count_stack = 150, count = 500, res = 0;
	srt_set *s_i32 = sms_alloc(SMS_I32, 0), *s_u32 = sms_alloc(SMS_U32, 0),
		*s_i = sms_alloc(SMS_I, 0), *s_f = sms_alloc(SMS_F, 0),
		*s_d = sms_alloc(SMS_D, 0), *s_s = sms_alloc(SMS_S, 0),
		*sa_i32 = sms_alloca(SMS_I32, count_stack),
		*sa_u32 = sms_alloca(SMS_U32, count_stack),
		*sa_i = sms_alloca(SMS_I, count_stack),
		*sa_f = sms_alloca(SMS_F, count_stack),
		*sa_d = sms_alloca(SMS_D, count_stack),
		*sa_s = sms_alloca(SMS_S, count_stack);
	srt_hset *hs_i32 = shs_alloc(SHS_I32, 0),
		 *hs_u32 = shs_alloc(SHS_U32, 0), *hs_i = shs_alloc(SHS_I, 0),
		 *hs_f = shs_alloc(SHS_F, 0), *hs_d = shs_alloc(SHS_D, 0),
		 *hs_s = shs_alloc(SHS_S, 0),
		 *hsa_i32 = shs_alloca(SHS_I32, count_stack),
		 *hsa_u32 = shs_alloca(SHS_U32, count_stack),
		 *hsa_i = shs_alloca(SHS_I, count_stack),
		 *hsa_f = shs_alloca(SHS_F, count_stack),
		 *hsa_d = shs_alloca(SHS_D, count_stack),
		 *hsa_s = shs_alloca(SHS_S, count_stack);
	srt_map *m_ii32 = sm_alloc(SM_II32, 0), *m_uu32 = sm_alloc(SM_UU32, 0),
		*m_ii = sm_alloc(SM_II, 0), *m_ff = sm_alloc(SM_FF, 0),
		*m_dd = sm_alloc(SM_DD, 0), *m_is = sm_alloc(SM_IS, 0),
		*m_ip = sm_alloc(SM_IP, 0), *m_si = sm_alloc(SM_SI, 0),
		*m_ds = sm_alloc(SM_DS, 0), *m_dp = sm_alloc(SM_DP, 0),
		*m_sd = sm_alloc(SM_SD, 0), *m_ss = sm_alloc(SM_SS, 0),
		*m_sp = sm_alloc(SM_SP, 0),
		*ma_ii32 = sm_alloca(SM_II32, count_stack),
		*ma_uu32 = sm_alloca(SM_UU32, count_stack),
		*ma_ii = sm_alloca(SM_II, count_stack),
		*ma_ff = sm_alloca(SM_FF, count_stack),
		*ma_dd = sm_alloca(SM_DD, count_stack),
		*ma_is = sm_alloca(SM_IS, count_stack),
		*ma_ip = sm_alloca(SM_IP, count_stack),
		*ma_si = sm_alloca(SM_SI, count_stack),
		*ma_ds = sm_alloca(SM_DS, count_stack),
		*ma_dp = sm_alloca(SM_DP, count_stack),
		*ma_sd = sm_alloca(SM_SD, count_stack),
		*ma_ss = sm_alloca(SM_SS, count_stack),
		*ma_sp = sm_alloca(SM_SP, count_stack);
	srt_hmap *hm_ii32 = shm_alloc(SHM_II32, 0),
		 *hm_uu32 = shm_alloc(SHM_UU32, 0),
		 *hm_ii = shm_alloc(SHM_II, 0), *hm_ff = shm_alloc(SHM_FF, 0),
		 *hm_dd = shm_alloc(SHM_DD, 0), *hm_is = shm_alloc(SHM_IS, 0),
		 *hm_ip = shm_alloc(SHM_IP, 0), *hm_si = shm_alloc(SHM_SI, 0),
		 *hm_ds = shm_alloc(SHM_DS, 0), *hm_dp = shm_alloc(SHM_DP, 0),
		 *hm_sd = shm_alloc(SHM_SD, 0), *hm_ss = shm_alloc(SHM_SS, 0),
		 *hm_sp = shm_alloc(SHM_SP, 0),
		 *hma_ii32 = shm_alloca(SHM_II32, count_stack),
		 *hma_uu32 = shm_alloca(SHM_UU32, count_stack),
		 *hma_ii = shm_alloca(SHM_II, count_stack),
		 *hma_ff = shm_alloca(SHM_FF, count_stack),
		 *hma_dd = shm_alloca(SHM_DD, count_stack),
		 *hma_is = shm_alloca(SHM_IS, count_stack),
		 *hma_ip = shm_alloca(SHM_IP, count_stack),
		 *hma_si = shm_alloca(SHM_SI, count_stack),
		 *hma_ds = shm_alloca(SHM_DS, count_stack),
		 *hma_dp = shm_alloca(SHM_DP, count_stack),
		 *hma_sd = shm_alloca(SHM_SD, count_stack),
		 *hma_ss = shm_alloca(SHM_SS, count_stack),
		 *hma_sp = shm_alloca(SHM_SP, count_stack);
	srt_string *ktmp = ss_alloca(1000), *vtmp = ss_alloca(1000);
	/*
	 * Insert data
	 */
	for (i = 0; i < count; i++) {
		ss_printf(&ktmp, 200, "k%04i", i);
		ss_printf(&vtmp, 200, "v%04i", i);
		/* sets */
		sms_insert_i32(&s_i32, -i);
		sms_insert_u32(&s_u32, (uint32_t)i);
		sms_insert_i(&s_i, -i);
		sms_insert_f(&s_f, (float)-i);
		sms_insert_d(&s_d, (double)-i);
		sms_insert_s(&s_s, ktmp);
		/* hash sets */
		shs_insert_i32(&hs_i32, -i);
		shs_insert_u32(&hs_u32, (uint32_t)i);
		shs_insert_i(&hs_i, -i);
		shs_insert_f(&hs_f, (float)-i);
		shs_insert_d(&hs_d, (double)-i);
		shs_insert_s(&hs_s, ktmp);
		/* maps */
		sm_insert_ii32(&m_ii32, -i, -i);
		sm_insert_uu32(&m_uu32, (uint32_t)i, (uint32_t)i);
		sm_insert_ii(&m_ii, -i, -i);
		sm_insert_ff(&m_ff, (float)-i, (float)-i);
		sm_insert_dd(&m_dd, (double)-i, (double)-i);
		sm_insert_is(&m_is, -i, vtmp);
		sm_insert_ip(&m_ip, -i, (void *)(intptr_t)i);
		sm_insert_si(&m_si, ktmp, i);
		sm_insert_ds(&m_ds, (double)-i, vtmp);
		sm_insert_dp(&m_dp, (double)-i, (void *)(intptr_t)i);
		sm_insert_sd(&m_sd, ktmp, (double)i);
		sm_insert_ss(&m_ss, ktmp, vtmp);
		sm_insert_sp(&m_sp, ktmp, (void *)(intptr_t)i);
		/* hash maps */
		shm_insert_ii32(&hm_ii32, -i, -i);
		shm_insert_uu32(&hm_uu32, (uint32_t)i, (uint32_t)i);
		shm_insert_ii(&hm_ii, -i, -i);
		shm_insert_ff(&hm_ff, (float)-i, (float)-i);
		shm_insert_dd(&hm_dd, (double)-i, (double)-i);
		shm_insert_is(&hm_is, -i, vtmp);
		shm_insert_ip(&hm_ip, -i, (void *)(intptr_t)i);
		shm_insert_si(&hm_si, ktmp, i);
		shm_insert_ds(&hm_ds, (double)-i, vtmp);
		shm_insert_dp(&hm_dp, (double)-i, (void *)(intptr_t)i);
		shm_insert_sd(&hm_sd, ktmp, (double)i);
		shm_insert_ss(&hm_ss, ktmp, vtmp);
		shm_insert_sp(&hm_sp, ktmp, (void *)(intptr_t)i);
	}
	for (i = 0; i < count_stack; i++) {
		ss_printf(&ktmp, 200, "k%04i", i);
		ss_printf(&vtmp, 200, "v%04i", i);
		/* sets */
		sms_insert_i32(&sa_i32, -i);
		sms_insert_u32(&sa_u32, (uint32_t)i);
		sms_insert_i(&sa_i, -i);
		sms_insert_f(&sa_f, (float)-i);
		sms_insert_d(&sa_d, (double)-i);
		sms_insert_s(&sa_s, ktmp);
		/* hash sets */
		shs_insert_i32(&hsa_i32, -i);
		shs_insert_u32(&hsa_u32, (uint32_t)i);
		shs_insert_i(&hsa_i, -i);
		shs_insert_f(&hsa_f, (float)-i);
		shs_insert_d(&hsa_d, (double)-i);
		shs_insert_s(&hsa_s, ktmp);
		/* maps */
		sm_insert_ii32(&ma_ii32, -i, -i);
		sm_insert_uu32(&ma_uu32, (uint32_t)i, (uint32_t)i);
		sm_insert_ii(&ma_ii, -i, -i);
		sm_insert_ff(&ma_ff, (float)-i, (float)-i);
		sm_insert_dd(&ma_dd, (double)-i, (double)-i);
		sm_insert_is(&ma_is, -i, vtmp);
		sm_insert_ip(&ma_ip, -i, (void *)(intptr_t)i);
		sm_insert_si(&ma_si, ktmp, i);
		sm_insert_ds(&ma_ds, (double)-i, vtmp);
		sm_insert_dp(&ma_dp, (double)-i, (void *)(intptr_t)i);
		sm_insert_sd(&ma_sd, ktmp, (double)i);
		sm_insert_ss(&ma_ss, ktmp, vtmp);
		sm_insert_sp(&ma_sp, ktmp, (void *)(intptr_t)i);
		/* hash maps */
		shm_insert_ii32(&hma_ii32, -i, -i);
		shm_insert_uu32(&hma_uu32, (uint32_t)i, (uint32_t)i);
		shm_insert_ii(&hma_ii, -i, -i);
		shm_insert_ff(&hma_ff, (float)-i, (float)-i);
		shm_insert_dd(&hma_dd, (double)-i, (double)-i);
		shm_insert_is(&hma_is, -i, vtmp);
		shm_insert_ip(&hma_ip, -i, (void *)(intptr_t)i);
		shm_insert_si(&hma_si, ktmp, i);
		shm_insert_ds(&hma_ds, (double)-i, vtmp);
		shm_insert_dp(&hma_dp, (double)-i, (void *)(intptr_t)i);
		shm_insert_sd(&hma_sd, ktmp, (double)i);
		shm_insert_ss(&hma_ss, ktmp, vtmp);
		shm_insert_sp(&hma_sp, ktmp, (void *)(intptr_t)i);
	}
	/*
	 * Compare map vs hash map
	 */
	for (i = 0; i < count; i++) {
		ss_printf(&ktmp, 200, "k%04i", i);
		ss_printf(&vtmp, 200, "v%04i", i);
		/* sets vs hash sets */
		if (sms_count_i32(s_i32, -i) != shs_count_i32(hs_i32, -i))
			res |= 0x08000000;
		if (sms_count_u32(s_u32, i) != shs_count_u32(hs_u32, i))
			res |= 0x04000000;
		if (sms_count_i(s_i, -i) != shs_count_i(hs_i, -i))
			res |= 0x02000000;
		if (sms_count_f(s_f, (float)-i) != shs_count_f(hs_f, (float)-i))
			res |= 0x02000000;
		if (sms_count_d(s_d, (double)-i)
		    != shs_count_d(hs_d, (double)-i))
			res |= 0x02000000;
		if (sms_count_s(s_s, ktmp) != shs_count_s(hs_s, ktmp))
			res |= 0x01000000;
		/* maps vs hash maps: count check */
		if (sm_count_i32(m_ii32, -i) != shm_count_i32(hm_ii32, -i))
			res |= 0x00000001;
		if (sm_count_u32(m_uu32, i) != shm_count_u32(hm_uu32, i))
			res |= 0x00000002;
		if (sm_count_i(m_ii, -i) != shm_count_i(hm_ii, -i))
			res |= 0x00000004;
		if (sm_count_f(m_ff, (float)-i)
		    != shm_count_f(hm_ff, (float)-i))
			res |= 0x00000004;
		if (sm_count_d(m_dd, (double)-i)
		    != shm_count_d(hm_dd, (double)-i))
			res |= 0x00000004;
		if (sm_count_i(m_is, -i) != shm_count_i(hm_is, -i))
			res |= 0x00000008;
		if (sm_count_i(m_ip, -i) != shm_count_i(hm_ip, -i))
			res |= 0x00000010;
		if (sm_count_s(m_si, ktmp) != shm_count_s(hm_si, ktmp))
			res |= 0x00000020;
		if (sm_count_d(m_ds, (double)-i)
		    != shm_count_d(hm_ds, (double)-i))
			res |= 0x00000008;
		if (sm_count_d(m_dp, (double)-i)
		    != shm_count_d(hm_dp, (double)-i))
			res |= 0x00000010;
		if (sm_count_s(m_sd, ktmp) != shm_count_s(hm_sd, ktmp))
			res |= 0x00000020;
		if (sm_count_s(m_ss, ktmp) != shm_count_s(hm_ss, ktmp))
			res |= 0x00000040;
		if (sm_count_s(m_sp, ktmp) != shm_count_s(hm_sp, ktmp))
			res |= 0x00000080;
		/* maps vs hash maps: value check */
		if (sm_at_ii32(m_ii32, -i) != shm_at_ii32(hm_ii32, -i))
			res |= 0x00000100;
		if (sm_at_uu32(m_uu32, i) != shm_at_uu32(hm_uu32, i))
			res |= 0x00000200;
		if (sm_at_ii(m_ii, -i) != shm_at_ii(hm_ii, -i))
			res |= 0x00000400;
		if (sm_at_ff(m_ff, (float)-i) != shm_at_ff(hm_ff, (float)-i))
			res |= 0x00000400;
		if (sm_at_dd(m_dd, (double)-i) != shm_at_dd(hm_dd, (double)-i))
			res |= 0x00000400;
		if (ss_cmp(sm_at_is(m_is, -i), shm_at_is(hm_is, -i)))
			res |= 0x00000800;
		if (sm_at_ip(m_ip, -i) != shm_at_ip(hm_ip, -i))
			res |= 0x00001000;
		if (sm_at_si(m_si, ktmp) != shm_at_si(hm_si, ktmp))
			res |= 0x00002000;
		if (ss_cmp(sm_at_ds(m_ds, -i), shm_at_ds(hm_ds, -i)))
			res |= 0x00000800;
		if (sm_at_dp(m_dp, (double)-i) != shm_at_dp(hm_dp, (double)-i))
			res |= 0x00001000;
		if (sm_at_sd(m_sd, ktmp) != shm_at_sd(hm_sd, ktmp))
			res |= 0x00002000;
		if (ss_cmp(sm_at_ss(m_ss, ktmp), shm_at_ss(hm_ss, ktmp)))
			res |= 0x00004000;
		if (sm_at_sp(m_sp, ktmp) != shm_at_sp(hm_sp, ktmp))
			res |= 0x00008000;
	}
	for (i = 0; i < count_stack; i++) {
		ss_printf(&ktmp, 200, "k%04i", i);
		ss_printf(&vtmp, 200, "v%04i", i);
		/* sets vs hash sets */
		if (sms_count_i32(sa_i32, -i) != shs_count_i32(hsa_i32, -i))
			res |= 0x88000000;
		if (sms_count_u32(sa_u32, i) != shs_count_u32(hsa_u32, i))
			res |= 0x84000000;
		if (sms_count_i(sa_i, -i) != shs_count_i(hsa_i, -i))
			res |= 0x82000000;
		if (sms_count_f(sa_f, (float)-i)
		    != shs_count_f(hsa_f, (float)-i))
			res |= 0x82000000;
		if (sms_count_d(sa_d, (double)-i)
		    != shs_count_d(hsa_d, (double)-i))
			res |= 0x82000000;
		if (sms_count_s(sa_s, ktmp) != shs_count_s(hsa_s, ktmp))
			res |= 0x81000000;
		/* maps vs hash maps: count check */
		if (sm_count_i32(ma_ii32, -i) != shm_count_i32(hma_ii32, -i))
			res |= 0x40000001;
		if (sm_count_u32(ma_uu32, i) != shm_count_u32(hma_uu32, i))
			res |= 0x40000002;
		if (sm_count_i(ma_ii, -i) != shm_count_i(hma_ii, -i))
			res |= 0x40000004;
		if (sm_count_f(ma_ff, (float)-i)
		    != shm_count_f(hma_ff, (float)-i))
			res |= 0x40000004;
		if (sm_count_d(ma_dd, (double)-i)
		    != shm_count_d(hma_dd, (double)-i))
			res |= 0x40000004;
		if (sm_count_i(ma_is, -i) != shm_count_i(hma_is, -i))
			res |= 0x40000008;
		if (sm_count_i(ma_ip, -i) != shm_count_i(hma_ip, -i))
			res |= 0x40000010;
		if (sm_count_s(ma_si, ktmp) != shm_count_s(hma_si, ktmp))
			res |= 0x40000020;
		if (sm_count_d(ma_ds, (double)-i)
		    != shm_count_d(hma_ds, (double)-i))
			res |= 0x40000008;
		if (sm_count_d(ma_dp, (double)-i)
		    != shm_count_d(hma_dp, (double)-i))
			res |= 0x40000010;
		if (sm_count_s(ma_sd, ktmp) != shm_count_s(hma_sd, ktmp))
			res |= 0x40000020;
		if (sm_count_s(ma_ss, ktmp) != shm_count_s(hma_ss, ktmp))
			res |= 0x40000040;
		if (sm_count_s(ma_sp, ktmp) != shm_count_s(hma_sp, ktmp))
			res |= 0x40000080;
		/* maps vs hash maps: value check */
		if (sm_at_ii32(ma_ii32, -i) != shm_at_ii32(hma_ii32, -i))
			res |= 0x20000100;
		if (sm_at_uu32(ma_uu32, i) != shm_at_uu32(hma_uu32, i))
			res |= 0x20000200;
		if (sm_at_ii(ma_ii, -i) != shm_at_ii(hma_ii, -i))
			res |= 0x20000400;
		if (sm_at_ff(ma_ff, (float)-i) != shm_at_ff(hma_ff, (float)-i))
			res |= 0x20000400;
		if (sm_at_dd(ma_dd, (double)-i)
		    != shm_at_dd(hma_dd, (double)-i))
			res |= 0x20000400;
		if (ss_cmp(sm_at_is(ma_is, -i), shm_at_is(hma_is, -i)))
			res |= 0x20000800;
		if (sm_at_ip(ma_ip, -i) != shm_at_ip(hma_ip, -i))
			res |= 0x20001000;
		if (sm_at_si(ma_si, ktmp) != shm_at_si(hma_si, ktmp))
			res |= 0x20002000;
		if (ss_cmp(sm_at_ds(ma_ds, (double)-i),
			   shm_at_ds(hma_ds, (double)-i)))
			res |= 0x20000800;
		if (sm_at_dp(ma_dp, (double)-i)
		    != shm_at_dp(hma_dp, (double)-i))
			res |= 0x20001000;
		if (sm_at_sd(ma_sd, ktmp) != shm_at_sd(hma_sd, ktmp))
			res |= 0x20002000;
		if (ss_cmp(sm_at_ss(ma_ss, ktmp), shm_at_ss(hma_ss, ktmp)))
			res |= 0x20004000;
		if (sm_at_sp(ma_sp, ktmp) != shm_at_sp(hma_sp, ktmp))
			res |= 0x20008000;
	}
#ifdef S_USE_VA_ARGS
	sms_free(&s_i32, &s_u32, &s_i, &s_f, &s_d, &s_s);
	shs_free(&hs_i32, &hs_u32, &hs_i, &hs_f, &hs_d, &hs_s);
	sm_free(&m_ii32, &m_uu32, &m_ii, &m_ff, &m_dd, &m_is, &m_ip, &m_si,
		&m_sd, &m_dp, &m_ds, &m_ss, &m_sp);
	shm_free(&hm_ii32, &hm_uu32, &hm_ii, &hm_ff, &hm_dd, &hm_is, &hm_ip,
		 &hm_si, &hm_ss, &hm_ds, &hm_dp, &hm_sd, &hm_sp);
	/*
	 * Stack-allocated sets/maps only require to be freed when using strings
	 * Note: if you're sure the strings used are below the threshold for
	 * small string optimizations, it could be also avoided.
	 */
	sms_free(&sa_s);
	shs_free(&hsa_s);
	sm_free(&ma_is, &ma_si, &ma_ds, &ma_sd, &ma_ss, &ma_sp);
	shm_free(&hma_is, &hma_si, &hma_ds, &hma_sd, &hma_ss, &hma_sp);
#else
	sms_free(&s_i32);
	sms_free(&s_u32);
	sms_free(&s_i);
	sms_free(&s_f);
	sms_free(&s_d);
	sms_free(&s_s);
	shs_free(&hs_i32);
	shs_free(&hs_u32);
	shs_free(&hs_i);
	shs_free(&hs_f);
	shs_free(&hs_d);
	shs_free(&hs_s);
	sm_free(&m_ii32);
	sm_free(&m_uu32);
	sm_free(&m_ii);
	sm_free(&m_ff);
	sm_free(&m_dd);
	sm_free(&m_is);
	sm_free(&m_ip);
	sm_free(&m_si);
	sm_free(&m_ds);
	sm_free(&m_dp);
	sm_free(&m_sd);
	sm_free(&m_ss);
	sm_free(&m_sp);
	shm_free(&hm_ii32);
	shm_free(&hm_uu32);
	shm_free(&hm_ii);
	shm_free(&hm_ff);
	shm_free(&hm_dd);
	shm_free(&hm_is);
	shm_free(&hm_ip);
	shm_free(&hm_si);
	shm_free(&hm_ds);
	shm_free(&hm_dp);
	shm_free(&hm_sd);
	shm_free(&hm_ss);
	shm_free(&hm_sp);
	sms_free(&sa_s);
	shs_free(&hsa_s);
	sm_free(&ma_is);
	sm_free(&ma_ds);
	sm_free(&ma_ss);
	sm_free(&ma_sp);
	shm_free(&hma_is);
	shm_free(&hma_si);
	shm_free(&hma_ds);
	shm_free(&hma_sd);
	shm_free(&hma_ss);
	shm_free(&hma_sp);
#endif
	return res;
}

static int test_endianess()
{
	int res = 0;
	union s_u32 be, le;
	union s_u64 b, l;
	uint32_t n = 0x00010203;
	uint64_t n2 = 0x0001020304050607LL;
	le.b[0] = be.b[3] = n & 0xff;
	le.b[1] = be.b[2] = (n >> 8) & 0xff;
	le.b[2] = be.b[1] = (n >> 16) & 0xff;
	le.b[3] = be.b[0] = (n >> 24) & 0xff;
	if (S_LD_LE_U32(le.b) != n)
		res |= 0x01;
	if (S_LD_LE_U32(le.b) != n)
		res |= 0x02;
	l.b[0] = b.b[7] = n2 & 0xff;
	l.b[1] = b.b[6] = (n2 >> 8) & 0xff;
	l.b[2] = b.b[5] = (n2 >> 16) & 0xff;
	l.b[3] = b.b[4] = (n2 >> 24) & 0xff;
	l.b[4] = b.b[3] = (n2 >> 32) & 0xff;
	l.b[5] = b.b[2] = (n2 >> 40) & 0xff;
	l.b[6] = b.b[1] = (n2 >> 48) & 0xff;
	l.b[7] = b.b[0] = (n2 >> 56) & 0xff;
	if (S_LD_LE_U64(l.b) != n2)
		res |= 0x10;
	if (S_LD_LE_U64(l.b) != n2)
		res |= 0x20;
	return res;
}

static int test_alignment()
{
	int res = 0;
	char ac[sizeof(uint32_t)] = {0}, bc[sizeof(uint32_t)] = {0},
	     cc[sizeof(size_t)], dd[sizeof(uint64_t)];
	unsigned a = 0x12345678, b = 0x87654321;
	size_t c = ((size_t)-1) & a;
	uint64_t d = (uint64_t)a << 32 | b;
	S_ST_U32(&ac, a);
	S_ST_U32(&bc, b);
	S_ST_SZT(&cc, c);
	S_ST_U64(&dd, d);
	res |= S_LD_U32(ac) != a ? 1 : 0;
	res |= S_LD_U32(bc) != b ? 2 : 0;
	res |= S_LD_SZT(cc) != c ? 4 : 0;
	res |= S_LD_U64(dd) != d ? 8 : 0;
	return res;
}

static int test_lsb_msb()
{
	uint8_t tv8[9] = {0x00, 0x01, 0x0f, 0x01, 0x0f, 0x80, 0xf0, 0x80, 0xf0},
		tv8l[9] = {0x00, 0x01, 0x01, 0x01, 0x01,
			   0x80, 0x10, 0x80, 0x10},
		tv8m[9] = {0x00, 0x01, 0x08, 0x01, 0x08,
			   0x80, 0x80, 0x80, 0x80};
	uint16_t tv16[9] = {0x0000, 0x0001, 0x000f, 0x0ff1, 0x0fff,
			    0x8000, 0xf000, 0x8ff0, 0xfff0},
		 tv16l[9] = {0x0000, 0x0001, 0x0001, 0x0001, 0x0001,
			     0x8000, 0x1000, 0x0010, 0x0010},
		 tv16m[9] = {0x0000, 0x0001, 0x0008, 0x0800, 0x0800,
			     0x8000, 0x8000, 0x8000, 0x8000};
	uint32_t tv32[9] = {0x00000000, 0x00000001, 0x0000000f,
			    0x0ffffff1, 0x0fffffff, 0x80000000,
			    0xf0000000, 0x8ffffff0, 0xfffffff0},
		 tv32l[9] = {0x00000000, 0x00000001, 0x00000001,
			     0x00000001, 0x00000001, 0x80000000,
			     0x10000000, 0x00000010, 0x00000010},
		 tv32m[9] = {0x00000000, 0x00000001, 0x00000008,
			     0x08000000, 0x08000000, 0x80000000,
			     0x80000000, 0x80000000, 0x80000000};
	uint64_t tv64[9] = {0x0000000000000000LL, 0x0000000000000001LL,
			    0x000000000000000fLL, 0x0ffffffffffffff1LL,
			    0x0fffffffffffffffLL, 0x8000000000000000LL,
			    0xf000000000000000LL, 0x8ffffffffffffff0LL,
			    0xfffffffffffffff0LL},
		 tv64l[9] = {0x0000000000000000LL, 0x0000000000000001LL,
			     0x0000000000000001LL, 0x0000000000000001LL,
			     0x0000000000000001LL, 0x8000000000000000LL,
			     0x1000000000000000LL, 0x0000000000000010LL,
			     0x0000000000000010LL},
		 tv64m[9] = {0x0000000000000000LL, 0x0000000000000001LL,
			     0x0000000000000008LL, 0x0800000000000000LL,
			     0x0800000000000000LL, 0x8000000000000000LL,
			     0x8000000000000000LL, 0x8000000000000000LL,
			     0x8000000000000000LL};
	int res = 0;
	size_t i;
#define LSBMSB_TEST(tvx, tvxl, tvxm, lsb, msb, err)                            \
	for (i = 0; i < sizeof(tvx) / sizeof(tvx[0]); i++)                     \
		if (lsb(tvx[i]) != tvxl[i] || msb(tvx[i]) != tvxm[i]) {        \
			res |= err;                                            \
			break;                                                 \
		}
	LSBMSB_TEST(tv8, tv8l, tv8m, s_lsb8, s_msb8, 1 << 0)
	LSBMSB_TEST(tv16, tv16l, tv16m, s_lsb16, s_msb16, 1 << 1)
	LSBMSB_TEST(tv32, tv32l, tv32m, s_lsb32, s_msb32, 1 << 2)
	LSBMSB_TEST(tv64, tv64l, tv64m, s_lsb64, s_msb64, 1 << 3)
#undef LSBMSB_TEST
	return res;
}

static int test_pk_u64()
{
	int res = 0;
	uint64_t u64;
	const uint8_t *dpc;
	uint8_t d[S_PK_U64_MAX_BYTES], *dp;
	size_t maxvs = 65;
	intptr_t s1, s2;
	srt_vector *v = sv_alloca_t(SV_U64, maxvs);
	size_t i, testc = sv_max_size(v), vs;
	sv_push_u64(&v, 0);
	for (i = 1; i < testc; i++)
		sv_push_u64(&v, S_NBITMASK64(i));
	vs = sv_size(v);
	for (i = 0; i < vs; i++) {
		memset(d, (i & 1) * 0xff, sizeof(d));
		dp = d;
		s_st_pk_u64(&dp, sv_at_u64(v, i));
		s1 = dp - d;
		dpc = d;
		u64 = s_ld_pk_u64(&dpc, sizeof(d));
		s2 = dpc - d;
		if (u64 != sv_at_u64(v, i))
			res |= 1;
		if (s1 != s2)
			res |= 0x10;
	}
	if (maxvs != vs)
		res |= 0x100;
	return res;
}

static int test_slog2()
{
	int res = 0;
	unsigned int i, j, k, a, b;
	uint64_t j2, k2;
	for (i = 0, j = 1, k = 1; i < 32; i++, j <<= 1, k = (k << 1) | 1) {
		a = slog2_32(j);
		b = slog2_32(k);
		if (a != b || a != i) {
			res |= 1;
			break;
		}
	}
	for (i = 0, j2 = 1, k2 = 1; i < 64; i++, j2 <<= 1, k2 = (k2 << 1) | 1) {
		a = slog2_64(j2);
		b = slog2_64(k2);
		if (a != b || a != i) {
			fprintf(stderr, "a: %08x, b: %08x, i: %u\n", a, b, i);
			res |= 0x10;
			break;
		}
		a = slog2_ceil(j2 + 1);
		if (a != i + 1) {
			fprintf(stderr, "a: %08x, i: %u\n", a, i);
			res |= 0x20;
			break;
		}
	}
	a = slog2_32(0);
	b = slog2_64(0);
	if (a != b || a != 0)
		res |= 0x100;
	return res;
}

static int test_smemset()
{
	int res = 0, i;
	uint8_t p64[64];
	uint8_t v16[2] = {0x01, 0x23};
	uint8_t v24[3] = {0x01, 0x23, 0x45};
	uint32_t v32 = 0x01234567;
	uint64_t v64 = 0x0123456789abcdefLL;
	for (i = 0; i < 32; i++) {
		memset(p64, 0, sizeof(p64));
		s_memset64(&p64[i], &v64, 2);
		if (S_LD_U64(&p64[i]) != v64)
			res |= 1 << i;
		memset(p64, 0, sizeof(p64));
		s_memset32(&p64[i], &v32, 2);
		if (S_LD_U32(&p64[i]) != v32)
			res |= 1 << i;
		memset(p64, 0, sizeof(p64));
		s_memset24(&p64[i], v24, 2);
		if (!memcpy(&p64[i], v24, 3) && !memcpy(&p64[i] + 3, v24, 3))
			res |= 1 << i;
		memset(p64, 0, sizeof(p64));
		s_memset16(&p64[i], v16, 2);
		if (!memcpy(&p64[i], v16, 2) && !memcpy(&p64[i] + 2, v16, 2))
			res |= 1 << i;
	}
	return res;
}

/*
 * Test execution
 */

int main()
{
	STEST_START;
	const char *utf8[] = {"a",
			      "$",
			      U8_CENT_00A2,
			      U8_EURO_20AC,
			      U8_HAN_24B62,
			      U8_C_N_TILDE_D1,
			      U8_S_N_TILDE_F1};
	const int32_t uc[] = {'a', '$', 0xa2, 0x20ac, 0x24b62, 0xd1, 0xf1};
	unsigned i = 0;
	char btmp1[400], btmp2[400];
	srt_string *co;
	int j;
	const srt_string *ci[5] = {
		ss_crefa("hellohellohellohellohellohellohello!"),
		ss_crefa("111111111111111111111111111111111111"),
		ss_crefa("121212121212121212121212121212121212"),
		ss_crefa("123123123123123123123123123123123123"),
		ss_crefa("123412341234123412341234123412341234")};
	srt_string *stmp;
	srt_bool unicode_support = S_TRUE;
	wint_t check[] = {0xc0, 0x23a, 0x10a0, 0x1e9e};
	size_t chkl = 0;
#ifdef GOOD_LOCALE_SUPPORT
	unsigned wchar_range;
	size_t test_tolower, test_toupper;
	int32_t l0, l, u0, u;
#endif
	const srt_string *crefa_HELLO = ss_crefa("HELLO"),
			 *crefa_hello = ss_crefa("hello"),
			 *crefa_0123_DEF = ss_crefa("0123456789ABCDEF"),
			 *crefa_0123_DEF_hex =
				 ss_crefa("30313233343536373839414243444546"),
			 *crefa_0123_DEF_b64 =
				 ss_crefa("MDEyMzQ1Njc4OUFCQ0RFRg=="),
			 *crefa_01 = ss_crefa("01"),
			 *crefa_01_b64 = ss_crefa("MDE="),
			 *crefa_0xf8 = ss_crefa("\xf8"),
			 *crefa_0xf8_hex = ss_crefa("f8"),
			 *crefa_0xf8_HEX = ss_crefa("F8"),
			 *crefa_0xffff = ss_crefa("\xff\xff"),
			 *crefa_0xffff_hex = ss_crefa("ffff"),
			 *crefa_0xffff_HEX = ss_crefa("FFFF"),
			 *crefa_01z = ss_crefa("01z"),
			 *crefa_01z_hex = ss_crefa("30317a"),
			 *crefa_01z_HEX = ss_crefa("30317A"),
			 *crefa_hi_there = ss_crefa("hi\"&'<>there"),
			 *crefa_hi_there_exml =
				 ss_crefa("hi&quot;&amp;&apos;&lt;&gt;there"),
			 *crefa_json1 = ss_crefa("\b\t\f\n\r\"\x5c"),
			 *crefa_json1_esc =
				 ss_crefa("\\b\\t\\f\\n\\r\\\"\x5c\x5c"),
			 *crefa_url1 =
				 ss_crefa("0189ABCXYZ-_.~abcxyz \\/&!?<>"),
			 *crefa_url1_esc = ss_crefa(
				 "0189ABCXYZ-_.~abcxyz%20%5C%2F%26"
				 "%21%3F%3C%3E"),
			 *crefa_qhow1 = ss_crefa("\"how\" are you?"),
			 *crefa_qhow1_quoted = ss_crefa("\"\"how\"\" are you?"),
			 *crefa_qhow2 = ss_crefa("'how' are you?"),
			 *crefa_qhow2_quoted = ss_crefa("''how'' are you?"),
			 *crefa_abc = ss_crefa("abc"),
			 *crefa_ABC = ss_crefa("ABC");
#ifndef S_MINIMAL
	/* setlocale: required for this test, not for using ss_* calls */
#ifdef S_POSIX_LOCALE_SUPPORT
	setlocale(LC_ALL, "en_US.UTF-8");
#else
	setlocale(LC_ALL, "");
#endif
	for (; chkl < sizeof(check) / sizeof(check[0]); chkl++)
		if (towlower(check[chkl]) == check[chkl]) {
			unicode_support = S_FALSE;
			break;
		}
	if (!unicode_support) {
		fprintf(stderr,
			"warning: OS without full built-in Unicode "
			"support (not required by libsrt, but used for "
			"some tests -those will be skipped-)\n");
	}
#endif
	STEST_ASSERT(test_sb(2));
	STEST_ASSERT(test_sb(3));
	STEST_ASSERT(test_sb(101));
	STEST_ASSERT(test_sb(1024));
	STEST_ASSERT(test_sb(1500));
	STEST_ASSERT(test_sb(8192));
	STEST_ASSERT(test_sb(10001));
#ifndef S_MINIMAL
	STEST_ASSERT(test_sb(128 * 1024));
	STEST_ASSERT(test_sb(1000 * 1000 + 1));
#endif
	STEST_ASSERT(test_ss_alloc(0));
	STEST_ASSERT(test_ss_alloc(16));
	STEST_ASSERT(test_ss_alloca(32));
	STEST_ASSERT(test_ss_shrink());
	STEST_ASSERT(test_ss_grow(100, 1));
	STEST_ASSERT(test_ss_grow(100, 100));
	STEST_ASSERT(test_ss_grow(100, 500));
	STEST_ASSERT(test_ss_reserve(2));
	STEST_ASSERT(test_ss_reserve(200));
	STEST_ASSERT(test_ss_resize_x());
	STEST_ASSERT(test_ss_trim("  a b c d e  ", "a b c d e"));
	STEST_ASSERT(test_ss_ltrim("  a b c d e  ", "a b c d e  "));
	STEST_ASSERT(test_ss_rtrim("  a b c d e  ", "  a b c d e"));
	STEST_ASSERT(test_ss_trim("abc", "abc"));
	STEST_ASSERT(test_ss_ltrim("abc", "abc"));
	STEST_ASSERT(test_ss_rtrim("abc", "abc"));
	STEST_ASSERT(test_ss_erase("abcde", 2, 1, "abde"));
	STEST_ASSERT(test_ss_erase("abcde", 2, 3, "ab"));
	STEST_ASSERT(test_ss_erase("abcde", 2, 10000, "ab"));
	STEST_ASSERT(test_ss_erase("abcde", 0, 5, ""));
	STEST_ASSERT(test_ss_erase("abcde", 0, 10000, ""));
	STEST_ASSERT(test_ss_erase_u("abcde", 2, 1, "abde"));
	STEST_ASSERT(test_ss_erase_u("abcde", 2, 3, "ab"));
	STEST_ASSERT(test_ss_erase_u("abcde", 2, 10000, "ab"));
	STEST_ASSERT(test_ss_erase_u("abcde", 0, 5, ""));
	STEST_ASSERT(test_ss_erase_u("abcde", 0, 10000, ""));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 "a" U8_C_N_TILDE_D1, 1, 2,
				     U8_C_N_TILDE_D1));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 "a" U8_C_N_TILDE_D1, 2, 1,
				     U8_C_N_TILDE_D1 "a"));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 "a" U8_C_N_TILDE_D1, 2,
				     1000, U8_C_N_TILDE_D1 "a"));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 U8_C_N_TILDE_D1 "a", 1, 2,
				     U8_C_N_TILDE_D1));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 U8_C_N_TILDE_D1 "a", 2, 1,
				     U8_C_N_TILDE_D1 U8_C_N_TILDE_D1));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 U8_C_N_TILDE_D1 "a", 2,
				     1000, U8_C_N_TILDE_D1 U8_C_N_TILDE_D1));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 U8_C_N_TILDE_D1 "a", 0, 1,
				     U8_C_N_TILDE_D1 "a"));
	STEST_ASSERT(test_ss_free(0));
	STEST_ASSERT(test_ss_free(16));
	STEST_ASSERT(test_ss_len("hello", 5));
	STEST_ASSERT(test_ss_len(U8_HAN_24B62, 4));
	STEST_ASSERT(test_ss_len(
		U8_C_N_TILDE_D1 U8_S_N_TILDE_F1 U8_S_I_DOTLESS_131
			U8_C_I_DOTTED_130 U8_C_G_BREVE_11E U8_S_G_BREVE_11F
				U8_C_S_CEDILLA_15E U8_S_S_CEDILLA_15F
					U8_CENT_00A2 U8_EURO_20AC U8_HAN_24B62,
		25)); /* bytes */
	STEST_ASSERT(test_ss_len_u("hello" U8_C_N_TILDE_D1, 6));
	STEST_ASSERT(test_ss_len_u(U8_HAN_24B62, 1));
	STEST_ASSERT(test_ss_len_u(
		U8_C_N_TILDE_D1 U8_S_N_TILDE_F1 U8_S_I_DOTLESS_131
			U8_C_I_DOTTED_130 U8_C_G_BREVE_11E U8_S_G_BREVE_11F
				U8_C_S_CEDILLA_15E U8_S_S_CEDILLA_15F
					U8_CENT_00A2 U8_EURO_20AC U8_HAN_24B62,
		11)); /* Unicode chrs */
	STEST_ASSERT(test_ss_capacity());
	STEST_ASSERT(test_ss_len_left());
	STEST_ASSERT(test_ss_max());
	STEST_ASSERT(test_ss_dup());
	STEST_ASSERT(test_ss_dup_substr());
	STEST_ASSERT(test_ss_dup_substr_u());
	STEST_ASSERT(test_ss_dup_cn());
	STEST_ASSERT(test_ss_dup_c());
	STEST_ASSERT(test_ss_dup_wn());
	STEST_ASSERT(test_ss_dup_w());
	STEST_ASSERT(test_ss_dup_int(0, "0"));
	STEST_ASSERT(test_ss_dup_int(1, "1"));
	STEST_ASSERT(test_ss_dup_int(-1, "-1"));
	STEST_ASSERT(test_ss_dup_int(2147483647, "2147483647"));
	STEST_ASSERT(test_ss_dup_int(-2147483647, "-2147483647"));
	STEST_ASSERT(
		test_ss_dup_int(9223372036854775807LL, "9223372036854775807"));
	STEST_ASSERT(test_ss_dup_int(-9223372036854775807LL,
				     "-9223372036854775807"));
	stmp = ss_alloca(128);
#define MK_TEST_SS_DUP_CPY_CAT(encc, decc, a, b)                               \
	{                                                                      \
		STEST_ASSERT(test_ss_dup_##encc(a, b));                        \
		STEST_ASSERT(test_ss_cpy_##encc(a, b));                        \
		ss_cpy_c(&stmp, "abc");                                        \
		STEST_ASSERT(                                                  \
			test_ss_cat_##encc(crefa_abc, a, ss_cat(&stmp, b)));   \
		ss_cpy_c(&stmp, "ABC");                                        \
		STEST_ASSERT(                                                  \
			test_ss_cat_##encc(crefa_ABC, a, ss_cat(&stmp, b)));   \
		STEST_ASSERT(test_ss_dup_##decc(b, a));                        \
		STEST_ASSERT(test_ss_cpy_##decc(b, a));                        \
		ss_cpy_c(&stmp, "abc");                                        \
		STEST_ASSERT(                                                  \
			test_ss_cat_##decc(crefa_abc, b, ss_cat(&stmp, a)));   \
		ss_cpy_c(&stmp, "ABC");                                        \
		STEST_ASSERT(                                                  \
			test_ss_cat_##decc(crefa_ABC, b, ss_cat(&stmp, a)));   \
	}
	MK_TEST_SS_DUP_CPY_CAT(tolower, toupper, crefa_HELLO, crefa_hello);
	MK_TEST_SS_DUP_CPY_CAT(enc_b64, dec_b64, crefa_0123_DEF,
			       crefa_0123_DEF_b64);
	MK_TEST_SS_DUP_CPY_CAT(enc_b64, dec_b64, crefa_01, crefa_01_b64);
	MK_TEST_SS_DUP_CPY_CAT(enc_hex, dec_hex, crefa_0xf8, crefa_0xf8_hex);
	MK_TEST_SS_DUP_CPY_CAT(enc_hex, dec_hex, crefa_0xffff,
			       crefa_0xffff_hex);
	MK_TEST_SS_DUP_CPY_CAT(enc_hex, dec_hex, crefa_01z, crefa_01z_hex);
	MK_TEST_SS_DUP_CPY_CAT(enc_HEX, dec_hex, crefa_0123_DEF,
			       crefa_0123_DEF_hex);
	MK_TEST_SS_DUP_CPY_CAT(enc_HEX, dec_hex, crefa_0xf8, crefa_0xf8_HEX);
	MK_TEST_SS_DUP_CPY_CAT(enc_HEX, dec_hex, crefa_0xffff,
			       crefa_0xffff_HEX);
	MK_TEST_SS_DUP_CPY_CAT(enc_HEX, dec_hex, crefa_01z, crefa_01z_HEX);
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_xml, dec_esc_xml, crefa_hi_there,
			       crefa_hi_there_exml);
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_json, dec_esc_json, crefa_json1,
			       crefa_json1_esc);
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_url, dec_esc_url, crefa_url1,
			       crefa_url1_esc);
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_dquote, dec_esc_dquote, crefa_qhow1,
			       crefa_qhow1_quoted);
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_squote, dec_esc_squote, crefa_qhow2,
			       crefa_qhow2_quoted);
	co = ss_alloca(256);
	for (j = 0; j < 5; j++) {
		ss_enc_lz(&co, ci[j]);
		MK_TEST_SS_DUP_CPY_CAT(enc_lz, dec_lz, ci[j], co);
		ss_enc_lzh(&co, ci[j]);
		MK_TEST_SS_DUP_CPY_CAT(enc_lzh, dec_lz, ci[j], co);
	}
	STEST_ASSERT(test_ss_dup_erase("hello", 2, 2, "heo"));
	STEST_ASSERT(test_ss_dup_erase_u());
	STEST_ASSERT(test_ss_dup_replace("hello", "ll", "*LL*", "he*LL*o"));
	STEST_ASSERT(test_ss_dup_resize());
	STEST_ASSERT(test_ss_dup_resize_u());
	STEST_ASSERT(test_ss_dup_trim("  hello  ", "hello"));
	STEST_ASSERT(test_ss_dup_ltrim("  hello  ", "hello  "));
	STEST_ASSERT(test_ss_dup_rtrim("  hello  ", "  hello"));
	STEST_ASSERT(test_ss_dup_printf());
	STEST_ASSERT(test_ss_dup_printf_va("abc1helloFFFFFFFF", 512,
					   "abc%i%s%08X", 1, "hello", -1));
	STEST_ASSERT(test_ss_dup_char('a', "a"));
	STEST_ASSERT(test_ss_dup_char(0x24b62, U8_HAN_24B62));
	STEST_ASSERT(test_ss_dup_read("abc"));
	STEST_ASSERT(test_ss_dup_read("a\nb\tc\rd\te\ff"));
	STEST_ASSERT(test_ss_cpy(""));
	STEST_ASSERT(test_ss_cpy("hello"));
	STEST_ASSERT(test_ss_cpy_cn());
	STEST_ASSERT(test_ss_cpy_substr());
	STEST_ASSERT(test_ss_cpy_substr_u());
	STEST_ASSERT(test_ss_cpy_c(""));
	STEST_ASSERT(test_ss_cpy_c("hello"));
	STEST_ASSERT(test_ss_cpy_w(L"hello", "hello"));
	STEST_ASSERT(test_ss_cpy_wn());
	STEST_ASSERT(test_ss_cpy_int(0, "0"));
	STEST_ASSERT(test_ss_cpy_int(1, "1"));
	STEST_ASSERT(test_ss_cpy_int(-1, "-1"));
	STEST_ASSERT(test_ss_cpy_int(2147483647, "2147483647"));
	STEST_ASSERT(test_ss_cpy_int(-2147483647, "-2147483647"));
	STEST_ASSERT(
		test_ss_cpy_int(9223372036854775807LL, "9223372036854775807"));
	STEST_ASSERT(test_ss_cpy_int(-9223372036854775807LL,
				     "-9223372036854775807"));
	STEST_ASSERT(test_ss_cpy_erase("hello", 2, 2, "heo"));
	STEST_ASSERT(test_ss_cpy_erase_u());
	STEST_ASSERT(test_ss_cpy_replace("hello", "ll", "*LL*", "he*LL*o"));
	STEST_ASSERT(test_ss_cpy_resize());
	STEST_ASSERT(test_ss_cpy_resize_u());
	STEST_ASSERT(test_ss_cpy_trim("  hello  ", "hello"));
	STEST_ASSERT(test_ss_cpy_ltrim("  hello  ", "hello  "));
	STEST_ASSERT(test_ss_cpy_rtrim("  hello  ", "  hello"));
	STEST_ASSERT(test_ss_cpy_printf());
	STEST_ASSERT(test_ss_cpy_printf_va("abc1helloFFFFFFFF", 512,
					   "abc%i%s%08X", 1, "hello", -1));
	STEST_ASSERT(test_ss_cpy_char('a', "a"));
	STEST_ASSERT(test_ss_cpy_char(0x24b62, U8_HAN_24B62));
	STEST_ASSERT(test_ss_cat("hello", "all"));
	STEST_ASSERT(test_ss_cat_substr());
	STEST_ASSERT(test_ss_cat_substr_u());
	STEST_ASSERT(test_ss_cat_cn());
	STEST_ASSERT(test_ss_cat_c("hello", "all"));
	memset(btmp1, 48, sizeof(btmp1) - 1); /* 0's */
	memset(btmp2, 49, sizeof(btmp2) - 1); /* 1's */
	btmp1[sizeof(btmp1) - 1] = btmp2[sizeof(btmp2) - 1] = 0;
	STEST_ASSERT(test_ss_cat_c(btmp1, btmp2));
	STEST_ASSERT(test_ss_cat_wn());
	STEST_ASSERT(test_ss_cat_w(L"hello", L"all"));
	STEST_ASSERT(test_ss_cat_int("prefix", 1, "prefix1"));
	STEST_ASSERT(test_ss_cat_erase("x", "hello", 2, 2, "xheo"));
	STEST_ASSERT(test_ss_cat_erase_u());
	STEST_ASSERT(
		test_ss_cat_replace("x", "hello", "ll", "*LL*", "xhe*LL*o"));
	STEST_ASSERT(test_ss_cat_resize());
	STEST_ASSERT(test_ss_cat_resize_u());
	STEST_ASSERT(test_ss_cat_trim("aaa", " hello ", "aaahello"));
	STEST_ASSERT(test_ss_cat_ltrim("aaa", " hello ", "aaahello "));
	STEST_ASSERT(test_ss_cat_rtrim("aaa", " hello ", "aaa hello"));
	STEST_ASSERT(test_ss_cat_printf());
	STEST_ASSERT(test_ss_cat_printf_va());
	STEST_ASSERT(test_ss_cat_char());
	STEST_ASSERT(test_ss_popchar());
	STEST_ASSERT(test_ss_tolower("aBcDeFgHiJkLmNoPqRsTuVwXyZ",
				     "abcdefghijklmnopqrstuvwxyz"));
	STEST_ASSERT(test_ss_toupper("aBcDeFgHiJkLmNoPqRsTuVwXyZ",
				     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
#if !defined(S_MINIMAL)
	STEST_ASSERT(test_ss_tolower(U8_C_N_TILDE_D1, U8_S_N_TILDE_F1));
	STEST_ASSERT(test_ss_toupper(U8_S_N_TILDE_F1, U8_C_N_TILDE_D1));
	STEST_ASSERT(!ss_set_turkish_mode(1));
	STEST_ASSERT(test_ss_tolower("I", U8_S_I_DOTLESS_131));
	STEST_ASSERT(test_ss_tolower(
		"III",
		U8_S_I_DOTLESS_131 U8_S_I_DOTLESS_131 U8_S_I_DOTLESS_131));
	STEST_ASSERT(test_ss_toupper(U8_S_I_DOTLESS_131, "I"));
	STEST_ASSERT(test_ss_tolower(U8_C_I_DOTTED_130, "i"));
	STEST_ASSERT(test_ss_toupper("i", U8_C_I_DOTTED_130));
	STEST_ASSERT(test_ss_tolower(U8_C_G_BREVE_11E, U8_S_G_BREVE_11F));
	STEST_ASSERT(test_ss_toupper(U8_S_G_BREVE_11F, U8_C_G_BREVE_11E));
	STEST_ASSERT(test_ss_tolower(U8_C_S_CEDILLA_15E, U8_S_S_CEDILLA_15F));
	STEST_ASSERT(test_ss_toupper(U8_S_S_CEDILLA_15F, U8_C_S_CEDILLA_15E));
	STEST_ASSERT(!ss_set_turkish_mode(0));
	/* Turkish characters that should work also in the general case: */
	STEST_ASSERT(test_ss_tolower(U8_C_G_BREVE_11E, U8_S_G_BREVE_11F));
	STEST_ASSERT(test_ss_toupper(U8_S_G_BREVE_11F, U8_C_G_BREVE_11E));
	STEST_ASSERT(test_ss_tolower(U8_C_S_CEDILLA_15E, U8_S_S_CEDILLA_15F));
	STEST_ASSERT(test_ss_toupper(U8_S_S_CEDILLA_15F, U8_C_S_CEDILLA_15E));
#endif
	STEST_ASSERT(test_ss_clear(""));
	STEST_ASSERT(test_ss_clear("hello"));
	STEST_ASSERT(test_ss_check());
	STEST_ASSERT(test_ss_replace("where are you? where are we?", 0, "where",
				     "who", "who are you? who are we?"));
	STEST_ASSERT(test_ss_replace("who are you? who are we?", 0, "who",
				     "where", "where are you? where are we?"));
	STEST_ASSERT(test_ss_to_c(""));
	STEST_ASSERT(test_ss_to_c("hello"));
	STEST_ASSERT(test_ss_to_w(""));
	STEST_ASSERT(test_ss_to_w("hello"));
#if !defined(S_NOT_UTF8_SPRINTF)
	if (unicode_support)
		STEST_ASSERT(test_ss_to_w("hello" U8_C_N_TILDE_D1));
#endif
	STEST_ASSERT(test_ss_find("full text", "text", 5));
	STEST_ASSERT(test_ss_find("full text", "hello", S_NPOS));
	STEST_ASSERT(test_ss_find_misc());
	STEST_ASSERT(test_ss_split());
	STEST_ASSERT(test_ss_cmp("hello", "hello2", -1));
	STEST_ASSERT(test_ss_cmp("hello2", "hello", 1));
	STEST_ASSERT(test_ss_cmp("hello", "hello", 0));
	STEST_ASSERT(test_ss_cmp("hello", "Hello", 'h' - 'H'));
	STEST_ASSERT(test_ss_cmp("Hello", "hello", 'H' - 'h'));
	STEST_ASSERT(test_ss_cmpi("hello", "HELLO2", -1));
	STEST_ASSERT(test_ss_cmpi("hello2", "HELLO", 1));
	STEST_ASSERT(test_ss_cmpi("HELLO", "hello", 0));
	STEST_ASSERT(test_ss_ncmp("hello", 0, "hello2", 6, -1));
	STEST_ASSERT(test_ss_ncmp("hello2", 0, "hello", 6, 1));
	STEST_ASSERT(test_ss_ncmp("hello1", 0, "hello2", 5, 0));
	STEST_ASSERT(test_ss_ncmp("hello", 0, "Hello", 5, 'h' - 'H'));
	STEST_ASSERT(test_ss_ncmp("Hello", 0, "hello", 5, 'H' - 'h'));
	STEST_ASSERT(test_ss_ncmpi("hello", 0, "HELLO2", 6, -1));
	STEST_ASSERT(test_ss_ncmpi("hello2", 0, "HELLO", 6, 1));
	STEST_ASSERT(test_ss_ncmpi("hello1", 0, "HELLO2", 5, 0));
	STEST_ASSERT(test_ss_ncmp("xxhello", 2, "hello2", 6, -1));
	STEST_ASSERT(test_ss_ncmp("xxhello2", 2, "hello", 6, 1));
	STEST_ASSERT(test_ss_ncmp("xxhello1", 2, "hello2", 5, 0));
	STEST_ASSERT(test_ss_ncmp("xxhello", 2, "Hello", 5, 'h' - 'H'));
	STEST_ASSERT(test_ss_ncmp("xxHello", 2, "hello", 5, 'H' - 'h'));
	STEST_ASSERT(test_ss_ncmpi("xxhello", 2, "HELLO2", 6, -1));
	STEST_ASSERT(test_ss_ncmpi("xxhello2", 2, "HELLO", 6, 1));
	STEST_ASSERT(test_ss_ncmpi("xxhello1", 2, "HELLO2", 5, 0));
	STEST_ASSERT(test_ss_printf());
	STEST_ASSERT(test_ss_getchar());
	STEST_ASSERT(test_ss_putchar());
	STEST_ASSERT(test_ss_cpy_read());
	STEST_ASSERT(test_ss_cat_read());
	STEST_ASSERT(test_ss_read_write());
	STEST_ASSERT(test_ss_csum32());
	STEST_ASSERT(test_ss_null());
	STEST_ASSERT(test_ss_misc());
	i = 0;
	for (; i < sizeof(utf8) / sizeof(utf8[0]); i++) {
		STEST_ASSERT(test_sc_utf8_to_wc(utf8[i], uc[i]));
		STEST_ASSERT(test_sc_wc_to_utf8(uc[i], utf8[i]));
	}
		/*
		 * Windows require specific locale for doing case conversions
		 * properly
		 */
#ifdef GOOD_LOCALE_SUPPORT
	if (unicode_support) {
		wchar_range = sizeof(wchar_t) == 2 ? 0xd7ff : 0x3fffff;
		test_tolower = 0;
		for (i = 0; i <= wchar_range; i++) {
			l0 = (int32_t)towlower((wint_t)i);
			l = sc_tolower((int)i);
			if (l != l0) {
				fprintf(stderr,
					"Warning: sc_tolower(%x): %x "
					"[%x system reported]\n",
					i, (unsigned)l, (unsigned)l0);
				test_tolower++;
			}
		}
#if 0 /* do not break the build if system has no full Unicode support */
		STEST_ASSERT(test_tolower);
#endif
		test_toupper = 0;
		for (i = 0; i <= wchar_range; i++) {
			u0 = (int32_t)towupper((wint_t)i);
			u = sc_toupper((int)i);
			if (u != u0) {
				fprintf(stderr,
					"Warning sc_toupper(%x): %x "
					"[%x system reported]\n",
					i, (unsigned)u, (unsigned)u0);
				test_toupper++;
			}
		}
#if 0 /* do not break the build if system has no full Unicode support */
		STEST_ASSERT(test_toupper);
#endif
	}
#endif /* #ifndef _MSC_VER */
	/*
	 * Vector
	 */
	STEST_ASSERT(test_sv_alloc());
	STEST_ASSERT(test_sv_alloca());
	STEST_ASSERT(test_sv_grow());
	STEST_ASSERT(test_sv_reserve());
	STEST_ASSERT(test_sv_shrink());
	STEST_ASSERT(test_sv_len());
	STEST_ASSERT(test_sv_capacity());
	STEST_ASSERT(test_sv_capacity_left());
	STEST_ASSERT(test_sv_get_buffer());
	STEST_ASSERT(test_sv_elem_size());
	STEST_ASSERT(test_sv_dup());
	STEST_ASSERT(test_sv_dup_erase());
	STEST_ASSERT(test_sv_dup_resize());
	STEST_ASSERT(test_sv_cpy());
	STEST_ASSERT(test_sv_cpy_erase());
	STEST_ASSERT(test_sv_cpy_resize());
	STEST_ASSERT(test_sv_cat());
	STEST_ASSERT(test_sv_cat_erase());
	STEST_ASSERT(test_sv_cat_resize());
	STEST_ASSERT(test_sv_erase());
	STEST_ASSERT(test_sv_resize());
	STEST_ASSERT(test_sv_sort());
	STEST_ASSERT(test_sv_find());
	STEST_ASSERT(test_sv_push_pop_set());
	STEST_ASSERT(test_sv_push_pop_set_u8());
	STEST_ASSERT(test_sv_push_pop_set_i8());
	STEST_ASSERT(test_sv_push_pop_set_u16());
	STEST_ASSERT(test_sv_push_pop_set_i16());
	STEST_ASSERT(test_sv_push_pop_set_u32());
	STEST_ASSERT(test_sv_push_pop_set_i32());
	STEST_ASSERT(test_sv_push_pop_set_u64());
	STEST_ASSERT(test_sv_push_pop_set_i64());
	STEST_ASSERT(test_sv_push_pop_set_f());
	STEST_ASSERT(test_sv_push_pop_set_d());
	STEST_ASSERT(test_sv_push_raw());
	STEST_ASSERT(test_st_alloc());
	STEST_ASSERT(test_st_insert_del());
	STEST_ASSERT(test_st_traverse());
	/*
	 * Map
	 */
	STEST_ASSERT(test_sm_alloc_ii32());
	STEST_ASSERT(test_sm_alloca_ii32());
	STEST_ASSERT(test_sm_alloc_uu32());
	STEST_ASSERT(test_sm_alloca_uu32());
	STEST_ASSERT(test_sm_alloc_ii());
	STEST_ASSERT(test_sm_alloca_ii());
	STEST_ASSERT(test_sm_alloc_ff());
	STEST_ASSERT(test_sm_alloca_ff());
	STEST_ASSERT(test_sm_alloc_dd());
	STEST_ASSERT(test_sm_alloca_dd());
	STEST_ASSERT(test_sm_shrink());
	STEST_ASSERT(test_sm_dup());
	STEST_ASSERT(test_sm_cpy());
	STEST_ASSERT(test_sm_count_u32());
	STEST_ASSERT(test_sm_count_i32());
	STEST_ASSERT(test_sm_count_i64());
	STEST_ASSERT(test_sm_count_f());
	STEST_ASSERT(test_sm_count_d());
	STEST_ASSERT(test_sm_count_s());
	STEST_ASSERT(test_sm_inc_ii32());
	STEST_ASSERT(test_sm_inc_uu32());
	STEST_ASSERT(test_sm_inc_ii());
	STEST_ASSERT(test_sm_inc_si());
	STEST_ASSERT(test_sm_inc_sd());
	STEST_ASSERT(test_sm_delete_i());
	STEST_ASSERT(test_sm_delete_s());
	STEST_ASSERT(test_sm_it());
	STEST_ASSERT(test_sm_itr());
	STEST_ASSERT(test_sm_sort_to_vectors());
	STEST_ASSERT(test_sm_double_rotation());
	/*
	 * Set
	 */
	STEST_ASSERT(test_sms());
	/*
	 * Hash map
	 */
	STEST_ASSERT(test_shm_alloc_ii32());
	STEST_ASSERT(test_shm_alloca_ii32());
	STEST_ASSERT(test_shm_alloc_uu32());
	STEST_ASSERT(test_shm_alloca_uu32());
	STEST_ASSERT(test_shm_alloc_ii());
	STEST_ASSERT(test_shm_alloca_ii());
	STEST_ASSERT(test_shm_alloc_ff());
	STEST_ASSERT(test_shm_alloca_ff());
	STEST_ASSERT(test_shm_alloc_dd());
	STEST_ASSERT(test_shm_alloca_dd());
	STEST_ASSERT(test_shm_shrink());
	STEST_ASSERT(test_shm_dup());
	STEST_ASSERT(test_shm_cpy());
	STEST_ASSERT(test_shm_count_u());
	STEST_ASSERT(test_shm_count_i());
	STEST_ASSERT(test_shm_count_s());
	STEST_ASSERT(test_shm_inc_ii32());
	STEST_ASSERT(test_shm_inc_uu32());
	STEST_ASSERT(test_shm_inc_ii());
	STEST_ASSERT(test_shm_inc_si());
	STEST_ASSERT(test_shm_delete_i());
	STEST_ASSERT(test_shm_delete_s());
	STEST_ASSERT(test_shm_it());
	STEST_ASSERT(test_shm_itp());
	/*
	 * Hash set
	 */
	STEST_ASSERT(test_shs());
	/*
	 * Low level stuff
	 */
	STEST_ASSERT(test_endianess());
	STEST_ASSERT(test_tree_vs_hash());
	STEST_ASSERT(test_alignment());
	STEST_ASSERT(test_lsb_msb());
	STEST_ASSERT(test_pk_u64());
	STEST_ASSERT(test_slog2());
	STEST_ASSERT(test_smemset());
	/*
	 * Report
	 */
#ifdef S_DEBUG
#define S_LOGSZ(t)                                                             \
	fprintf(stderr, "\tsizeof(" #t "): %u\n", (unsigned)sizeof(t));
	fprintf(stderr, "Internal data structures:\n");
	S_LOGSZ(srt_data);
	S_LOGSZ(srt_tnode);
	S_LOGSZ(srt_tree);
	S_LOGSZ(srt_string_ref);
	S_LOGSZ(srt_stringo);
	S_LOGSZ(srt_stringo1);
#ifdef S_ENABLE_SM_STRING_OPTIMIZATION
	fprintf(stderr, "\tsrt_stringo1 max in-place length: %u\n",
		(unsigned)OptStrMaxSize);
	fprintf(stderr, "\tsrt_stringo max in-place length DD: %u\n",
		(unsigned)OptStr_MaxSize_DD);
	fprintf(stderr, "\tsrt_stringo max in-place length DI/ID: %u\n",
		(unsigned)OptStr_MaxSize_DI);
#endif
	S_LOGSZ(struct SMapi);
	S_LOGSZ(struct SMapu);
	S_LOGSZ(struct SMapI);
	S_LOGSZ(struct SMapS);
	S_LOGSZ(struct SMapii);
	S_LOGSZ(struct SMapuu);
	S_LOGSZ(struct SMapII);
	S_LOGSZ(struct SMapIS);
	S_LOGSZ(struct SMapIP);
	S_LOGSZ(struct SMapSI);
	S_LOGSZ(struct SMapSS);
	S_LOGSZ(struct SMapSP);
	S_LOGSZ(struct SHMBucket);
	S_LOGSZ(struct SHMapi);
	S_LOGSZ(struct SHMapu);
	S_LOGSZ(struct SHMapI);
	S_LOGSZ(struct SHMapS);
	S_LOGSZ(struct SHMapii);
	S_LOGSZ(struct SHMapuu);
	S_LOGSZ(struct SHMapII);
	S_LOGSZ(struct SHMapIS);
	S_LOGSZ(struct SHMapIP);
	S_LOGSZ(struct SHMapSI);
	S_LOGSZ(struct SHMapSS);
	S_LOGSZ(struct SHMapSP);
	fprintf(stderr, "Data structures exposed via API:\n");
	fprintf(stderr, "\tsizeof(srt_string): %u (small mode)\n",
		(unsigned)sizeof(struct SDataSmall));
	fprintf(stderr, "\tsizeof(srt_string): %u (full mode)\n",
		(unsigned)sizeof(srt_string));
	fprintf(stderr, "\tmax srt_string length: " FMT_ZU "\n", SS_RANGE);
	S_LOGSZ(srt_bitset);
	S_LOGSZ(srt_bool);
	S_LOGSZ(srt_hmap);
	S_LOGSZ(srt_map);
	S_LOGSZ(srt_set);
	S_LOGSZ(srt_vector);
	fprintf(stderr, "Errors: %i\n", ss_errors);
#endif
	return STEST_END;
}

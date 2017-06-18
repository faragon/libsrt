/*
 * stest.c
 *
 * libsrt API tests
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "../src/libsrt.h"
#include "../src/saux/sbitio.h"
#include "../src/saux/schar.h"
#include "../src/saux/sdbg.h"
#include "utf8_examples.h"
#include <locale.h>

/*
 * Unit testing helpers
 */

#define STEST_START	int ss_errors = 0, ss_tmp = 0
#define STEST_END	ss_errors
#define STEST_ASSERT_BASE(a, pre_op) {					\
		pre_op;							\
		const int r = (int)(a);					\
		ss_tmp += r;						\
		if (r) {						\
			fprintf(stderr, "%s: %08X <--------\n", #a, r);	\
			ss_errors++;					\
		}							\
	}
#ifdef S_DEBUG
#define STEST_ASSERT(a) \
	STEST_ASSERT_BASE(a, fprintf(stderr, "%s...\n", #a))
#else
#define STEST_ASSERT(a) \
	STEST_ASSERT_BASE(a, ;)
#endif

#define STEST_FILE	"test.dat"
#define S_MAX_I64	9223372036854775807LL
#define S_MIN_I64	(0 - S_MAX_I64 - 1)

/*
 * Test common data structures
 */

#define TEST_AA(s, t) (s.a == t.a && s.b == t.b)
#define TEST_INT(s, t) (s == t)

struct AA { int a, b; };
static struct AA a1 = { 1, 2 }, a2 = { 3, 4 };
static struct AA av1[3] = { { 1, 2 }, { 3, 4 }, { 5, 6 } };

static int8_t i8v[10] = { 1, 2, 3, 4, 5, -5, -4, -3, -2, -1 };
static uint8_t u8v[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
static int16_t i16v[10] = { 1, 2, 3, 4, 5, -5, -4, -3, -2, -1 };
static uint16_t u16v[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
static int32_t i32v[10] = { 1, 2, 3, 4, 5, -5, -4, -3, -2, -1 };
static uint32_t u32v[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
static int64_t i64v[10] = { 1, 2, 3, 4, 5, -5, -4, -3, -2, -1 };
static uint64_t u64v[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
static void *spv[10]= { 0, (void *)1, (void *)2, (void *)3, (void *)4,
		 (void *)5, (void *)6, (void *)7, (void *)8,
		 (void *)9 };
static int64_t sik[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
static int32_t i32k[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
static uint32_t u32k[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

#define NO_CMPF ,NULL
#define X_CMPF
#define AA_CMPF ,AA_cmp

static int AA_cmp(const void *a, const void *b) {
	int64_t r = ((const struct AA *)a)->a - ((const struct AA *)b)->a;
	return r < 0 ? -1 : r > 0 ? 1 : 0;
}
static int cmp_ii(int64_t a, int64_t b) {
	return a > b ? 1 : b > a ? -1 : 0;
}
static int cmp_pp(const void *a, const void *b) {
	return a > b ? 1 : b > a ? -1 : 0;
}

/*
 * Tests generated from templates
 */

/*
 * Following test template covers two cases: non-aliased (sa to sb) and
 * aliased (sa to sa)
 */
#define MK_TEST_SS_DUP_CODEC(suffix)					      \
	static int test_ss_dup_##suffix(const ss_t *a, const ss_t *b) {	      \
		ss_t *sa = ss_dup(a), *sb = ss_dup_##suffix(sa),	      \
		     *sc = ss_dup_##suffix(a);				      \
		int res = (!sa || !sb) ? 1 : (ss_##suffix(&sa, sa) ? 0 : 2) | \
			(!ss_cmp(sa, b) ? 0 : 4) |			      \
			(!ss_cmp(sb, b) ? 0 : 8) |			      \
			(!ss_cmp(sc, b) ? 0 : 16);			      \
		ss_free(&sa, &sb, &sc);					      \
		return res;						      \
	}

#define MK_TEST_SS_CPY_CODEC(suffix)					      \
	static int test_ss_cpy_##suffix(const ss_t *a, const ss_t *b) {	      \
		ss_t *sa = ss_dup(a), *sb = ss_dup_c("garbage i!&/()="),      \
		     *sc = ss_dup(sb);					      \
		ss_cpy_##suffix(&sb, sa);				      \
		ss_cpy_##suffix(&sc, a);				      \
		int res = (!sa || !sb) ? 1 : (ss_##suffix(&sa, sa) ? 0 : 2) | \
			(!ss_cmp(sa, b) ? 0 : 4) |			      \
			(!ss_cmp(sb, b) ? 0 : 8) |			      \
			(!ss_cmp(sc, b) ? 0 : 16);			      \
		ss_free(&sa, &sb, &sc);					      \
		return res;						      \
	}

#define MK_TEST_SS_CAT_CODEC(suffix)					   \
	static int test_ss_cat_##suffix(const ss_t *a, const ss_t *b,	   \
					const ss_t *expected) {		   \
		ss_t *sa = ss_dup(a), *sb = ss_dup(b),			   \
		     *sc = ss_dup(sa);					   \
		ss_cat_##suffix(&sa, sb);				   \
		ss_cat_##suffix(&sc, b);				   \
		int res = (!sa || !sb) ? 1 :				   \
				(!ss_cmp(sa, expected) ? 0 : 2) |	   \
				(!ss_cmp(sc, expected) ? 0 : 4);	   \
		ss_free(&sa, &sb, &sc);					   \
		return res;						   \
	}

#define MK_TEST_SS_DUP_CPY_CAT_CODEC(codec)	\
	MK_TEST_SS_DUP_CODEC(codec)		\
	MK_TEST_SS_CPY_CODEC(codec)		\
	MK_TEST_SS_CAT_CODEC(codec)

MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_b64)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_hex)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_HEX)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_lzw)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_rle)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_xml)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_json)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_url)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_dquote)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_squote)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_b64)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_hex)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_lzw)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_rle)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_xml)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_json)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_url)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_dquote)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_squote)

/*
 * Tests
 */

static int test_sb(size_t nelems)
{
#ifdef _MSC_VER /* supress alloca() warning */
#pragma warning(disable: 6255)
#endif
	sb_t *a = sb_alloca(nelems), *b = sb_alloc(0);
	#define TEST_SB(bs, ne) 	\
		sb_eval(&bs, ne);	\
		sb_set(&bs, 0);		\
		sb_set(&bs, ne - 1);
	TEST_SB(a, nelems);
	TEST_SB(b, nelems);
	#undef TEST_SB
	sb_t *da = sb_dup(a), *db = sb_dup(b);
	int res = sb_popcount(a) != sb_popcount(b) ||
		  sb_popcount(da) != sb_popcount(db) ||
		  sb_popcount(a) != sb_popcount(da) ||
		  sb_popcount(a) != 2 ? 1 :
		  !sb_test(a, 0) || !sb_test(b, 0) ||
		  !sb_test(da, 0) || !sb_test(db, 0) ? 2 : 0;
	/* clear 'a', 'b', 'da' all at once */
	sb_clear(a);
	sb_clear(b);
	sb_clear(da);
	/* clear 'db' one manually */
	sb_reset(&db, 0);
	sb_reset(&db, nelems - 1);
	res |= sb_popcount(a) || sb_popcount(b) || sb_popcount(da) ||
	       sb_popcount(db) ? 4 : sb_test(da, 0) || sb_test(db, 0) ? 8 : 0;
	res |= sb_capacity(da) < nelems || sb_capacity(db) < nelems ? 16 : 0;
	sb_shrink(&a);
	sb_shrink(&b);
	sb_shrink(&da);
	sb_shrink(&db);
	res |= sb_capacity(a) < nelems ? 32 : 0; /* stack is not shrinkable */
	res |= sb_capacity(b) || sb_capacity(da) || sb_capacity(db) ? 64 : 0;
	sb_free(&b, &da, &db);
	return res;
}

static int test_ss_alloc(const size_t max_size)
{
	ss_t *a = ss_alloc(max_size);
	int res = !a ? 1 : (ss_len(a) != 0) * 2 |
			   (ss_capacity(a) != max_size) * 4;
	ss_free(&a);
	return res;
}

static int test_ss_alloca(const size_t max_size)
{
	ss_t *a = ss_alloca(max_size);
	int res = !a ? 1 : (ss_len(a) != 0) * 2 |
			   (ss_capacity(a) != max_size) * 4;
	ss_free(&a);
	return res;
}

static int test_ss_shrink()
{
	ss_t *a = ss_dup_printf(1024, "hello");
	int res = !a ? 1 : (ss_shrink(&a) ? 0 : 2) |
			   (ss_capacity(a) == ss_len(a) ? 0 : 4);
	ss_free(&a);
	return res;
}

static int test_ss_grow(const size_t init_size, const size_t extra_size)
{
	ss_t *a = ss_alloc(init_size);
	int res = a ? 0 : 1;
	res |= res ? 0 : (ss_grow(&a, extra_size) > 0 ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_reserve(const size_t max_size)
{
	ss_t *a = ss_alloc(max_size / 2);
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
	ss_t *a = ss_dup_c(i0), *b = ss_dup(a);
	int res = a && b ? 0 : 1;
	res |= res ? 0 : ((ss_resize(&a, 14, '_') &&
			 !strcmp(ss_to_c(a), i1)) ? 0 : 2) |
			 ((ss_resize_u(&b, 13, '_') &&
			  !strcmp(ss_to_c(b), i1)) ? 0 : 4) |
			 ((ss_resize(&a, 4, '_') &&
			  !strcmp(ss_to_c(a), i2)) ? 0 : 8) |
			 ((ss_resize_u(&b, 4, '_') &&
			  !strcmp(ss_to_c(b), i3)) ? 0 : 16);
	ss_free(&a, &b);
	return res;
}

static int test_ss_trim(const char *in, const char *expected)
{
	ss_t *a = ss_dup_c(in);
	int res = a ? 0 : 1;
	res |= res ? 0 : (ss_trim(&a) ? 0 : 2) |
			(!strcmp(ss_to_c(a), expected) ? 0 : 4);
	ss_free(&a);
	return res;
}

static int test_ss_ltrim(const char *in, const char *expected)
{
	ss_t *a = ss_dup_c(in);
	int res = a ? 0 : 1;
	res |= res ? 0 : (ss_ltrim(&a) ? 0 : 2) |
			(!strcmp(ss_to_c(a), expected) ? 0 : 4);
	ss_free(&a);
	return res;
}

static int test_ss_rtrim(const char *in, const char *expected)
{
	ss_t *a = ss_dup_c(in);
	int res = a ? 0 : 1;
	res |= res ? 0 : (ss_rtrim(&a) ? 0 : 2) |
			(!strcmp(ss_to_c(a), expected) ? 0 : 4);
	ss_free(&a);
	return res;
}

static int test_ss_erase(const char *in, const size_t byte_off,
		const size_t num_bytes, const char *expected)
{
	ss_t *a = ss_dup_c(in);
	int res = a ? 0 : 1;
	res |= res ? 0 : (ss_erase(&a, byte_off, num_bytes) ? 0 : 2) |
			(!strcmp(ss_to_c(a), expected) ? 0 : 4);
	ss_free(&a);
	return res;
}

static int test_ss_erase_u(const char *in, const size_t char_off,
		const size_t num_chars, const char *expected)
{
	ss_t *a = ss_dup_c(in);
	int res = a ? 0 : 1;
	size_t in_wlen = ss_len_u(a),
	       expected_len = char_off >= in_wlen ? in_wlen :
			(in_wlen - char_off) > num_chars ? in_wlen - num_chars :
			char_off;
	res |= res ? 0 : (ss_erase_u(&a, char_off, num_chars) ? 0 : 2) |
			 (!strcmp(ss_to_c(a), expected) ? 0 : 4) |
			 (expected_len == ss_len_u(a) ? 0 : 8);
	ss_free(&a);
	return res;
}

static int test_ss_free(const size_t max_size)
{
	ss_t *a = ss_alloc(max_size);
	int res = a ? 0 : 1;
	ss_free(&a);
	res |= (!a ? 0 : 2);
	return res;
}

static int test_ss_len(const char *a, const size_t expected_size)
{
	ss_t *sa = ss_dup_c(a);
	const ss_t *sb = ss_crefa(a), *sc = ss_refa_buf(a, strlen(a));
	int res = !sa ? 1 : (ss_len(sa) == expected_size ? 0 : 1) |
			    (ss_len(sb) == expected_size ? 0 : 2) |
			    (ss_len(sc) == expected_size ? 0 : 4);
	ss_free(&sa);
	return res;
}

static int test_ss_len_u(const char *a, const size_t expected_size)
{
	ss_t *sa = ss_dup_c(a);
	const ss_t *sb = ss_crefa(a), *sc = ss_refa_buf(a, strlen(a));
	int res = !sa ? 1 : (ss_len_u(sa) == expected_size ? 0 : 1) |
			    (ss_len_u(sb) == expected_size ? 0 : 2) |
			    (ss_len_u(sc) == expected_size ? 0 : 4);
	ss_free(&sa);
	return res;
}

static int test_ss_capacity()
{
	const size_t test_max_size = 100;
	ss_t *a = ss_alloc(test_max_size);
	const ss_t *b = ss_crefa("123");
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

static int test_ss_len_left()
{
	ss_t *a = ss_alloc(10);
	ss_cpy_c(&a, "a");
	int res = !a ? 1 : (ss_capacity(a) - ss_len(a) == 9 ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_max()
{
	ss_t *a = ss_alloca(10), *b = ss_alloc(10);
	const ss_t *c = ss_crefa("hello");
	int res = (a && b) ? 0 : 1;
	res |= res ? 0 : (ss_max(a) == 10 ? 0 : 1);
	res |= res ? 0 : (ss_max(b) == SS_RANGE ? 0 : 2);
	res |= res ? 0 : (ss_max(c) == 5 ? 0 : 4);
	ss_free(&b);
	return res;
}

static int test_ss_dup()
{
	ss_t *b = ss_dup_c("hello"), *a = ss_dup(b);
	const ss_t *c = ss_crefa("hello");
	int res = !a ? 1 : (!strcmp("hello", ss_to_c(a)) ? 0 : 2) |
			   (!strcmp("hello", ss_to_c(c)) ? 0 : 4);
	ss_free(&a, &b);
	return res;
}

static int test_ss_dup_substr()
{
	ss_t *a = ss_dup_c("hello"), *b = ss_dup_substr(a, 0, 5),
	     *c = ss_dup_substr(ss_crefa("hello"), 0, 5);
	int res = (!a || !b) ? 1 : (ss_len(a) == ss_len(b) ? 0 : 2) |
				   (!strcmp("hello", ss_to_c(a)) ? 0 : 4) |
				   (ss_len(a) == ss_len(c) ? 0 : 8) |
				   (!strcmp("hello", ss_to_c(c)) ? 0 : 16);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_dup_substr_u()
{
	ss_t *b = ss_dup_c("hello"), *a = ss_dup_substr(b, 0, 5),
	     *c = ss_dup_substr(ss_crefa("hello"), 0, 5);
	int res = !a ? 1 : (ss_len(a) == ss_len(b) ? 0 : 2) |
			   (!strcmp("hello", ss_to_c(a)) ? 0 : 4) |
			   (ss_len(a) == ss_len(c) ? 0 : 8) |
			   (!strcmp("hello", ss_to_c(c)) ? 0 : 16);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_dup_cn()
{
	ss_t *a = ss_dup_cn("hello123", 5);
	int res = !a ? 1 : (!strcmp("hello", ss_to_c(a)) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_dup_c()
{
	ss_t *a = ss_dup_c("hello");
	const ss_t *b = ss_crefa("hello");
	int res = !a ? 1 : (!strcmp("hello", ss_to_c(a)) ? 0 : 2) |
			   (!strcmp("hello", ss_to_c(b)) ? 0 : 4);
	ss_free(&a);
	return res;
}

static int test_ss_dup_wn()
{
	ss_t *a = ss_dup_wn(L"hello123", 5);
	int res = !a ? 1 : (!strcmp("hello", ss_to_c(a)) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_dup_w()
{
	ss_t *a = ss_dup_w(L"hello");
	int res = !a ? 1 : (!strcmp("hello", ss_to_c(a)) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_dup_int(const int64_t num, const char *expected)
{
	ss_t *a = ss_dup_int(num);
	int res = !a ? 1 : (!strcmp(ss_to_c(a), expected) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_dup_tolower(const ss_t *a, const ss_t *b)
{
	ss_t *sa = ss_dup(a), *sb = ss_dup_tolower(sa), *sc = ss_dup_tolower(a);
	int res = (!sa || !sb || !sc) ? 1 :
		  (ss_tolower(&sa) ? 0 : 2) |
		  (!ss_cmp(sa, b) ? 0 : 4) |
		  (!ss_cmp(sb, b) ? 0 : 8) |
		  (!ss_cmp(sc, b) ? 0 : 16) |
		  (!ss_cmp(sb, b) ? 0 : 32) |
		  (!ss_cmp(sc, b) ? 0 : 64);
	ss_free(&sa, &sb, &sc);
	return res;
}

static int test_ss_dup_toupper(const ss_t *a, const ss_t *b)
{
	ss_t *sa = ss_dup(a), *sb = ss_dup_toupper(sa);
	int res = (!sa ||!sb) ? 1 : (ss_toupper(&sa) ? 0 : 2) |
			   (!ss_cmp(sa, b) ? 0 : 4) |
			   (!ss_cmp(sb, b) ? 0 : 8) |
			   (!ss_cmp(sb, b) ? 0 : 16);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_dup_erase(const char *in, const size_t off,
			     const size_t size, const char *expected)
{
	ss_t *a = ss_dup_c(in), *b = ss_dup_erase(a, off, size);
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), expected) ? 0 : 2);
	ss_free(&a, &b);
	return res;
}

static int test_ss_dup_erase_u()
{
	ss_t *a = ss_dup_c("hel" U8_S_N_TILDE_F1 "lo"),
	     *b = ss_dup_erase_u(a, 2, 3);
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), "heo") ? 0 : 2);
	ss_free(&a, &b);
	return res;
}

static int test_ss_dup_replace(const char *in, const char *r, const char *s,
			       const char *expected)
{
	ss_t *a = ss_dup_c(in), *b = ss_dup_c(r),
	     *c = ss_dup_c(s), *d = ss_dup_replace(a, 0, b, c),
	     *e = ss_dup_replace(a, 0, a, a);
	int res = (!a || !b) ? 1 :
			(!strcmp(ss_to_c(d), expected) ? 0 : 2) |
			(!strcmp(ss_to_c(e), in)? 0 : 4); /* aliasing */
	ss_free(&a, &b, &c, &d, &e);
	return res;
}

static int test_ss_dup_resize()
{
	ss_t *a = ss_dup_c("hello"), *b = ss_dup_resize(a, 10, 'z'),
	     *c = ss_dup_resize(a, 2, 'z');
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), "hellozzzzz") ? 0 : 2) |
				   (!strcmp(ss_to_c(c), "he") ? 0 : 4);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_dup_resize_u()
{
	ss_t *a = ss_dup_c(U8_S_N_TILDE_F1 "hello"),
	     *b = ss_dup_resize_u(a, 11, 'z'),
	     *c = ss_dup_resize_u(a, 3, 'z');
	int res = (!a || !b) ?
		1 :
		(!strcmp(ss_to_c(b), U8_S_N_TILDE_F1 "hellozzzzz") ? 0 : 2) |
		(!strcmp(ss_to_c(c), U8_S_N_TILDE_F1 "he") ? 0 : 4);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_dup_trim(const char *a, const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_trim(sa);
	const ss_t *sc = ss_dup_trim(ss_crefa(a));
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sb), expected) ? 0 : 2) |
				     (!ss_cmp(sb, sc) ? 0 : 4);
	ss_free(&sa, &sb, &sc);
	return res;
}

static int test_ss_dup_ltrim(const char *a, const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_ltrim(sa);
	const ss_t *sc = ss_dup_ltrim(ss_crefa(a));
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sb), expected) ? 0 : 2) |
				     (!ss_cmp(sb, sc) ? 0 : 4);
	ss_free(&sa, &sb, &sc);
	return res;
}

static int test_ss_dup_rtrim(const char *a, const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_rtrim(sa);
	const ss_t *sc = ss_dup_rtrim(ss_crefa(a));
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sb), expected) ? 0 : 2) |
				     (!ss_cmp(sb, sc) ? 0 : 4);
	ss_free(&sa, &sb, &sc);
	return res;
}

static int test_ss_dup_printf()
{
	char btmp[512];
	ss_t *a = ss_dup_printf(512, "abc%i%s%08X", 1, "hello", -1);
	sprintf(btmp, "abc%i%s%08X", 1, "hello", -1);
	int res = !strcmp(ss_to_c(a), btmp) ? 0 : 1;
	ss_free(&a);
	return res;
}

static int test_ss_dup_printf_va(const char *expected, const size_t max_size,
							const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	ss_t *a = ss_dup_printf_va(max_size, fmt, ap);
	va_end(ap);
	int res = !strcmp(ss_to_c(a), expected) ? 0 : 1;
	ss_free(&a);
	return res;
}

static int test_ss_dup_char(const int32_t in, const char *expected)
{
	ss_t *a = ss_dup_char(in);
	int res = !a ? 1 : !strcmp(ss_to_c(a), expected) ? 0 : 2;
	ss_free(&a);
	return res;
}

static int test_ss_dup_read(const char *pattern)
{
	int res = 1;
	const size_t pattern_size = strlen(pattern);
	ss_t *s = NULL;
	FILE *f = fopen(STEST_FILE, S_FOPEN_BINARY_RW_TRUNC);
	if (f) {
		size_t write_size = fwrite(pattern, 1, pattern_size, f);
		res = !write_size || ferror(f) ||
		      write_size != pattern_size ? 2 :
		      fseek(f, 0, SEEK_SET) != 0 ? 4 :
		      (s = ss_dup_read(f, pattern_size)) == NULL ? 8 :
		      strcmp(ss_to_c(s), pattern) ? 16 : 0;
		fclose(f);
		if (remove(STEST_FILE) != 0)
			res |= 32;
	}
	ss_free(&s);
	return res;
}

static int test_ss_cpy(const char *in)
{
	ss_t *a = ss_dup_c(in);
	int res = !a ? 1 : (!strcmp(ss_to_c(a), in) ? 0 : 2) |
			  (ss_len(a) == strlen(in) ? 0 : 4);
	ss_t *b = NULL;
	ss_cpy(&b, a);
	res = res ? 0 : (!strcmp(ss_to_c(b), in) ? 0 : 8) |
		       (ss_len(b) == strlen(in) ? 0 : 16);
	ss_free(&a, &b);
	return res;
}

static int test_ss_cpy_substr()
{
	ss_t *b = NULL, *a = ss_dup_c("how are you"), *c = ss_dup_c("how");
	int res = (!a || !c) ? 1 : 0;
	res |= res ? 0 : (ss_cpy_substr(&b, a, 0, 3) ? 0 : 2);
	res |= res ? 0 : (!ss_cmp(b, c) ? 0 : 4);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_cpy_substr_u()
{
	ss_t *b = NULL, *a = ss_dup_c("how are you"), *c = ss_dup_c("how");
	int res = (!a || !c) ? 1 : 0;
	res |= res ? 0 : (ss_cpy_substr_u(&b, a, 0, 3) ? 0 : 2);
	res |= res ? 0 : (!ss_cmp(b, c) ? 0 : 4);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_cpy_cn()
{
	char b[3] = { 0, 1, 2 };
	ss_t *a = NULL;
	int res = !ss_cpy_cn(&a, b, 3) ? 1 : (ss_to_c(a)[0] == b[0] ? 0: 2) |
					     (ss_to_c(a)[1] == b[1] ? 0: 4) |
					     (ss_to_c(a)[2] == b[2] ? 0: 8) |
					     (ss_len(a) == 3 ? 0 : 16);
	ss_free(&a);
	return res;
}

static int test_ss_cpy_c(const char *in)
{
	ss_t *a = NULL;
	int res = !ss_cpy_c(&a, in) ? 1 : (!strcmp(ss_to_c(a), in) ? 0 : 2) |
					 (ss_len(a) == strlen(in) ? 0 : 4);
	char tmp[512];
	sprintf(tmp, "%s%s", in, in);
	res |= res ? 0 :
		ss_cpy_c(&a, in, in) && !strcmp(ss_to_c(a), tmp) ? 0 : 8;
	ss_free(&a);
	return res;
}

static int test_ss_cpy_w(const wchar_t *in, const char *expected_utf8)
{
	ss_t *a = NULL;
	int res = !ss_cpy_w(&a, in) ? 1 :
				(!strcmp(ss_to_c(a), expected_utf8) ? 0 : 2) |
				(ss_len(a) == strlen(expected_utf8) ? 0 : 4);
	char tmp[512];
	sprintf(tmp, "%ls%ls", in, in);
	res |= res ? 0 : ss_cpy_w(&a, in, in) && !strcmp(ss_to_c(a), tmp) ?
									  0 : 8;
	ss_free(&a);
	return res;
}

static int test_ss_cpy_wn()
{
	int res = 0;
	const uint32_t t32[] = { 0xd1, 0xf1, 0x131, 0x130, 0x11e, 0x11f, 0x15e,
				 0x15f, 0xa2, 0x20ac, 0x24b62, 0 };
	const uint16_t t16[] = { 0xd1, 0xf1, 0x131, 0x130, 0x11e, 0x11f, 0x15e,
				 0x15f, 0xa2, 0x20ac, 0xd800 | (uint16_t)(0x24b62 >> 10),
				 0xdc00 | (uint16_t)(0x24b62 & 0x3ff), 0 };
	const wchar_t *t = sizeof(wchar_t) == 2 ? (wchar_t *)t16 :
						  (wchar_t *)t32;
	#define TU8A3 U8_C_N_TILDE_D1 U8_S_N_TILDE_F1 U8_S_I_DOTLESS_131
	#define TU8B3 U8_C_I_DOTTED_130 U8_C_G_BREVE_11E U8_S_G_BREVE_11F
	#define TU8C3 U8_C_S_CEDILLA_15E U8_S_S_CEDILLA_15F U8_CENT_00A2
	#define TU8D2 U8_EURO_20AC U8_HAN_24B62
	#define TU8ALL11 TU8A3 TU8B3 TU8C3 TU8D2
	ss_t *a = ss_dup_c("hello"), *b_a3 = ss_dup_c(TU8A3),
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
	res |= ss_len_u(b_a3) == 3 && ss_len_u(b_b3) == 3 &&
	       ss_len_u(b_c3) == 3 && ss_len_u(b_d2) == 2 &&
	       ss_len_u(b_all11) == 11 ? 0 : 64;
	ss_free(&a, &b_a3, &b_b3, &b_c3, &b_d2, &b_all11);
	return res;
}

static int test_ss_cpy_int(const int64_t num, const char *expected)
{
	ss_t *a = NULL;
	ss_cpy_int(&a, num);
	int res = !a ? 1 : (!strcmp(ss_to_c(a), expected) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_cpy_tolower(const ss_t *a, const ss_t *b)
{
	ss_t *sa = ss_dup(a), *sb = ss_dup_c("garbage i!&/()=");
	ss_cpy_tolower(&sb, sa);
	int res = (!sa ||!sb) ? 1 : (ss_tolower(&sa) ? 0 : 2) |
			   (!ss_cmp(sa, b) ? 0 : 4) |
			   (!ss_cmp(sb, b) ? 0 : 8);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cpy_toupper(const ss_t *a, const ss_t *b)
{
	ss_t *sa = ss_dup(a), *sb = ss_dup_c("garbage i!&/()=");
	ss_cpy_toupper(&sb, sa);
	int res = (!sa ||!sb) ? 1 : (ss_toupper(&sa) ? 0 : 2) |
			   (!ss_cmp(sa, b) ? 0 : 4) |
			   (!ss_cmp(sb, b) ? 0 : 8);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cpy_erase(const char *in, const size_t off,
			     const size_t size, const char *expected)
{
	ss_t *a = ss_dup_c(in), *b = ss_dup_c("garbage i!&/()=");
	ss_cpy_erase(&b, a, off, size);
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), expected) ? 0 : 2);
	ss_free(&a, &b);
	return res;
}

static int test_ss_cpy_erase_u()
{
	ss_t *a = ss_dup_c("hel" U8_S_N_TILDE_F1 "lo"),
	     *b = ss_dup_c("garbage i!&/()=");
	ss_cpy_erase_u(&b, a, 2, 3);
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), "heo") ? 0 : 2);
	ss_free(&a, &b);
	return res;
}

static int test_ss_cpy_replace(const char *in, const char *r, const char *s,
			       const char *expected)
{
	ss_t *a = ss_dup_c(in), *b = ss_dup_c(r),
	     *c = ss_dup_c(s), *d = ss_dup_c("garbage i!&/()=");
	ss_cpy_replace(&d, a, 0, b, c);
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(d), expected) ? 0 : 2);
	ss_free(&a, &b, &c, &d);
	return res;
}

static int test_ss_cpy_resize()
{
	ss_t *a = ss_dup_c("hello"), *b = ss_dup_c("garbage i!&/()="),
	     *c = ss_dup_c("garbage i!&/()=");
	ss_cpy_resize(&b, a, 10, 'z');
	ss_cpy_resize(&c, a, 2, 'z');
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), "hellozzzzz") ? 0 : 2) |
				   (!strcmp(ss_to_c(c), "he") ? 0 : 4);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_cpy_resize_u()
{
	ss_t *a = ss_dup_c(U8_S_N_TILDE_F1 "hello"),
	     *b = ss_dup_c("garbage i!&/()="),
	     *c = ss_dup_c("garbage i!&/()=");
	ss_cpy_resize_u(&b, a, 11, 'z');
	ss_cpy_resize_u(&c, a, 3, 'z');
	int res = (!a || !b) ? 1 :
			(!strcmp(ss_to_c(b),
				 U8_S_N_TILDE_F1 "hellozzzzz") ? 0 : 2) |
			(!strcmp(ss_to_c(c),
				 U8_S_N_TILDE_F1 "he") ? 0 : 4);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_cpy_trim(const char *a, const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c("garbage i!&/()=");
	ss_cpy_trim(&sb, sa);
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sb), expected) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cpy_ltrim(const char *a, const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c("garbage i!&/()=");
	ss_cpy_ltrim(&sb, sa);
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sb), expected) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cpy_rtrim(const char *a, const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c("garbage i!&/()=");
	ss_cpy_rtrim(&sb, sa);
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sb), expected) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cpy_printf()
{
	char btmp[512];
	ss_t *a = NULL;
	ss_cpy_printf(&a, 512, "abc%i%s%08X", 1, "hello", -1);
	sprintf(btmp, "abc%i%s%08X", 1, "hello", -1);
	int res = !a ? 1 : !strcmp(ss_to_c(a), btmp) ? 0 : 2;
	ss_free(&a);
	return res;
}

static int test_ss_cpy_printf_va(const char *expected, const size_t max_size,
							const char *fmt, ...)
{
	ss_t *a = NULL;
	va_list ap;
	va_start(ap, fmt);
	ss_cpy_printf_va(&a, max_size, fmt, ap);
	va_end(ap);
	int res = !a ? 1 : !strcmp(ss_to_c(a), expected) ? 0 : 2;
	ss_free(&a);
	return res;
}

static int test_ss_cpy_char(const int32_t in, const char *expected)
{
	ss_t *a = ss_dup_c("zz");
	ss_cpy_char(&a, in);
	int res = !a ? 1 : !strcmp(ss_to_c(a), expected) ? 0 : 2;
	ss_free(&a);
	return res;
}

static int test_ss_cpy_read()
{
	int res = 1;
	const size_t max_buf = 512;
	ss_t *ah = NULL;
	ss_t *as = ss_alloca(max_buf);
	ss_cpy(&as, ah);
	const char *pattern = "hello world";
	const size_t pattern_size = strlen(pattern);
	FILE *f = fopen(STEST_FILE, S_FOPEN_BINARY_RW_TRUNC);
	if (f) {
		size_t write_size = fwrite(pattern, 1, pattern_size, f);
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

static int test_ss_cat(const char *a, const char *b)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	int res = (sa && sb) ? 0 : 1;
	char btmp[8192];
	sprintf(btmp, "%s%s%s", a, b, b);
	res |= res ? 0 : (ss_cat(&sa, sb, sb) ? 0 : 2) |
			 (!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	/* Aliasing test: */
	res |= res ? 0 : (sprintf(btmp, "%s%s", ss_to_c(sa), ss_to_c(sa)) &&
			  ss_cat(&sa, sa) ? 0 : 8); 
	res |= res ? 0 : (!strcmp(ss_to_c(sa), btmp) ? 0 : 16);
	ss_t *sc = ss_dup_c("c"), *sd = ss_dup_c("d"),
		    *se = ss_dup_c("e"), *sf = ss_dup_c("f");
#ifdef S_DEBUG
	size_t alloc_calls_before = dbg_cnt_alloc_calls;
#endif
	res |= res ? 0 : (sc && sd && se && sf &&
			  ss_cat(&sc, sd, se, sf)) ? 0 : 32;
#ifdef S_DEBUG
	/* Check that multiple append uses just one -or zero- allocation: */
	size_t alloc_calls_after = dbg_cnt_alloc_calls;
	res |= res ? 0 :
		     (alloc_calls_after - alloc_calls_before) <= 1 ? 0 : 128;
#endif
	/* Aliasing check */
	res |= res ? 0 :
		     ss_cat(&sf, se, sf, se, sf, se) &&
		     !strcmp(ss_to_c(sf), "fefefe") ? 0 : 64;

	res |= res ? 0 : (!strcmp(ss_to_c(sc), "cdef")) ? 0 : 256;
	ss_free(&sa, &sb, &sc, &sd, &se, &sf);
	return res;
}

static int test_ss_cat_substr()
{
	ss_t *a = ss_dup_c("how are you"), *b = ss_dup_c(" "),
	     *c = ss_dup_c("are you"), *d = ss_dup_c("are ");
	int res = (!a || !b) ? 1 : 0;
	res |= res ? 0 : (ss_cat_substr(&d, a, 8, 3) ? 0 : 4);
	res |= res ? 0 : (!ss_cmp(c, d) ? 0 : 8);
	ss_free(&a, &b, &c, &d);
	return res;
}

static int test_ss_cat_substr_u()
{
	/* like the above, but replacing the 'e' with a Chinese character */
	ss_t *a = ss_dup_c("how ar" U8_HAN_24B62 " you"),
	     *b = ss_dup_c(" "), *c = ss_dup_c("ar" U8_HAN_24B62 " you"),
	     *d = ss_dup_c("ar" U8_HAN_24B62 " ");
	int res = (!a || !b) ? 1 : 0;
	res |= res ? 0 : (ss_cat_substr_u(&d, a, 8, 3) ? 0 : 4);
	res |= res ? 0 : (!ss_cmp(c, d) ? 0 : 8);
	ss_free(&a, &b, &c, &d);
	return res;
}

static int test_ss_cat_cn()
{
	const char *a = "12345", *b = "67890";
	ss_t *sa = ss_dup_c(a);
	int res = sa ? 0 : 1;
	char btmp[8192];
	sprintf(btmp, "%s%s", a, b);
	res |= res ? 0 : (ss_cat_cn(&sa, b, 5) ? 0 : 2) |
			(!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	ss_free(&sa);
	return res;
}

static int test_ss_cat_c(const char *a, const char *b)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c("b");
	int res = sa ? 0 : 1;
	char btmp[8192];
	sprintf(btmp, "%s%s%s", a, b, b);
	res |= res ? 0 : (ss_cat_c(&sa, b, b) ? 0 : 2) |
			(!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	/* Concatenate multiple literal inputs */
	res |= res ? 0 :
		     ss_cat_c(&sb, "c", "b", "c", "b", "c") &&
		     !strcmp(ss_to_c(sb), "bcbcbc") ? 0 : 8;
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cat_wn()
{
	const wchar_t *a = L"12345", *b = L"67890";
	ss_t *sa = ss_dup_w(a);
	int res = sa ? 0 : 1;
	char btmp[8192];
	sprintf(btmp, "%ls%ls", a, b);
	res |= res ? 0 : (ss_cat_wn(&sa, b, 5) ? 0 : 2) |
			 (!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	ss_free(&sa);
	return res;
}

static int test_ss_cat_w(const wchar_t *a, const wchar_t *b)
{
	ss_t *sa = ss_dup_w(a), *sb = ss_dup_c("b");
	int res = sa ? 0 : 1;
	char btmp[8192];
	sprintf(btmp, "%ls%ls%ls%ls", a, b, b, b);
	res |= res ? 0 : (ss_cat_w(&sa, b, b, b) ? 0 : 2) |
			 (!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	/* Concatenate multiple literal inputs */
	res |= res ? 0 :
		     ss_cat_w(&sb, L"c", L"b", L"c", L"b", L"c") &&
		     !strcmp(ss_to_c(sb), "bcbcbc") ? 0 : 8;
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cat_int(const char *in, const int64_t num,
			   const char *expected)
{
	ss_t *a = ss_dup_c(in);
	ss_cat_int(&a, num);
	int res = !a ? 1 : (!strcmp(ss_to_c(a), expected) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_cat_tolower(const ss_t *a, const ss_t*b,
			       const ss_t *expected)
{
	ss_t *sa = ss_dup(a), *sb = ss_dup(b);
	ss_cat_tolower(&sa, sb);
	int res = (!sa || !sb) ? 1 : (!ss_cmp(sa, expected) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cat_toupper(const ss_t *a, const ss_t *b,
			       const ss_t *expected)
{
	ss_t *sa = ss_dup(a), *sb = ss_dup(b);
	ss_cat_toupper(&sa, sb);
	int res = (!sa || !sb) ? 1 : (!ss_cmp(sa, expected) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cat_erase(const char *prefix, const char *in,
			     const size_t off, const size_t size,
			     const char *expected)
{
	ss_t *a = ss_dup_c(in), *b = ss_dup_c(prefix);
	ss_cat_erase(&b, a, off, size);
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), expected) ? 0 : 2);
	ss_free(&a, &b);
	return res;
}

static int test_ss_cat_erase_u()
{
	ss_t *a = ss_dup_c("hel" U8_S_N_TILDE_F1 "lo"), *b = ss_dup_c("x");
	ss_cat_erase_u(&b, a, 2, 3);
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), "xheo") ? 0 : 2);
	ss_free(&a, &b);
	return res;
}

static int test_ss_cat_replace(const char *prefix, const char *in,
			       const char *r, const char *s,
			       const char *expected)
{
	ss_t *a = ss_dup_c(in), *b = ss_dup_c(r),
	     *c = ss_dup_c(s), *d = ss_dup_c(prefix);
	ss_cat_replace(&d, a, 0, b, c);
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(d), expected) ? 0 : 2);
	ss_free(&a, &b, &c, &d);
	return res;
}

static int test_ss_cat_resize()
{
	ss_t *a = ss_dup_c("hello"), *b = ss_dup_c("x"),
	     *c = ss_dup_c("x");
	ss_cat_resize(&b, a, 10, 'z');
	ss_cat_resize(&c, a, 2, 'z');
	int res = (!a || !b) ? 1 :
		  (!strcmp(ss_to_c(b), "xhellozzzzz") ? 0 : 2) |
		  (!strcmp(ss_to_c(c), "xhe") ? 0 : 4);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_cat_resize_u()
{
	ss_t *a = ss_dup_c(U8_S_N_TILDE_F1 "hello"), *b = ss_dup_c("x"),
	     *c = ss_dup_c("x");
	ss_cat_resize_u(&b, a, 11, 'z');
	ss_cat_resize_u(&c, a, 3, 'z');
	int res = (!a || !b) ? 1 :
		    (!strcmp(ss_to_c(b), "x" U8_S_N_TILDE_F1 "hellozzzzz") ? 0 : 2) |
		    (!strcmp(ss_to_c(c), "x" U8_S_N_TILDE_F1 "he") ? 0 : 4);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_cat_trim(const char *a, const char *b, const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	ss_cat_trim(&sa, sb);
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sa), expected) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cat_ltrim(const char *a, const char *b, const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	ss_cat_ltrim(&sa, sb);
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sa), expected) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cat_rtrim(const char *a, const char *b, const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	ss_cat_rtrim(&sa, sb);
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sa), expected) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cat_printf()
{
	char btmp[512];
	ss_t *sa = ss_dup_c("abc");
	sprintf(btmp, "abc%i%s%08X", 1, "hello", -1);
	ss_cat_printf(&sa, 512, "%i%s%08X", 1, "hello", -1);
	int res = !sa? 1 : !strcmp(ss_to_c(sa), btmp) ? 0 : 2;
	ss_free(&sa);
	return res;
}

static int test_ss_cat_printf_va()
{
	return test_ss_cat_printf();
}

static int test_ss_cat_char()
{
	const wchar_t *a = L"12345", *b = L"6";
	ss_t *sa = ss_dup_w(a);
	int res = sa ? 0 : 1;
	char btmp[8192];
	sprintf(btmp, "%ls%ls", a, b);
	res |= res ? 0 : (ss_cat_char(&sa, b[0]) ? 0 : 2) |
			(!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	ss_free(&sa);
	return res;
}

static int test_ss_cat_read()
{
	int res = 1;
	const size_t max_buf = 512;
	ss_t *ah = ss_dup_c("hello "), *as = ss_alloca(max_buf);
	ss_cpy(&as, ah);
	const char *pattern = "hello world", *suffix = "world";
	const size_t suffix_size = strlen(suffix);
	FILE *f = fopen(STEST_FILE, S_FOPEN_BINARY_RW_TRUNC);
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

static int test_ss_tolower(const char *a, const char *b)
{
	ss_t *sa = ss_dup_c(a);
	int res = !sa ? 1 : (ss_tolower(&sa) ? 0 : 2) |
			   (!strcmp(ss_to_c(sa), b) ? 0 : 4);
	ss_free(&sa);
	return res;
}

static int test_ss_toupper(const char *a, const char *b)
{
	ss_t *sa = ss_dup_c(a);
	int res = !sa ? 1 : (ss_toupper(&sa) ? 0 : 2) |
			   (!strcmp(ss_to_c(sa), b) ? 0 : 4);
	ss_free(&sa);
	return res;
}

static int test_ss_clear(const char *in)
{
	ss_t *sa = ss_dup_c(in);
	int res = !sa ? 1 : (ss_len(sa) == strlen(in) ? 0 : 2) |
			   (ss_capacity(sa) == strlen(in) ? 0 : 4);
	ss_clear(sa);
	res |= res ? 0 : (ss_len(sa) == 0 ? 0 : 16);
	ss_free(&sa);
	return res;
}

static int test_ss_check()
{
	ss_t *sa = NULL;
	int res = ss_check(&sa) ? 0 : 1;
	ss_free(&sa);
	return res;
}

static int test_ss_replace(const char *s, const size_t off,
			   const char *s1, const char *s2,
			   const char *expected)
{
	ss_t *a = ss_dup_c(s), *b = ss_dup_c(s1),
		    *c = ss_dup_c(s2);
	int res = a && b && c && ss_replace(&a, off, b, c) ? 0 : 1;
	res |= res ? 0 : (!strcmp(ss_to_c(a), expected) ? 0 : 2);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_to_c(const char *in)
{
	ss_t *a = ss_dup_c(in);
	int res = !a ? 1 : (!strcmp(in, ss_to_c(a)) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_to_w(const char *in)
{
	ss_t *a = ss_dup_c(in);
	const size_t ssa = ss_len(a) * (sizeof(wchar_t) == 2 ? 2 : 1);
	wchar_t *out = a ? (wchar_t *)s_malloc(sizeof(wchar_t) * (ssa + 1)) :
			  NULL;
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

static int test_ss_find(const char *a, const char *b, const size_t expected_loc)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	int res = (!sa || !sb) ? 1 :
		  (ss_find(sa, 0, sb) == expected_loc ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_find_misc()
{
	int res = 0;
	/*                    01234 56 7 8901234  5             6 */
	const char *sample = "abc \t \n\r 123zyx" U8_HAN_24B62 "s";
	const ss_t *a = ss_crefa(sample);
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
	res |= ss_find(a, 0, ss_crefa(U8_HAN_24B62 "s")) == 15 ? 0 : 1024;
	res |= ss_findr(a, 0, S_NPOS, ss_crefa("yx")) == 13 ? 0 : 2048;
	res |= ss_find_cn(a, 0, "yx", 2) == 13 ? 0 : 4096;
	res |= ss_findr_cn(a, 0, S_NPOS, "yx", 2) == 13 ? 0 : 8192;
	return res;
}

static int test_ss_split()
{
	const char *howareyou = "how are you";
	ss_t *a = ss_dup_c(howareyou), *sep1 = ss_dup_c(" ");
	const ss_t *c = ss_crefa(howareyou), *sep2 = ss_crefa(" ");
	#define TSS_SPLIT_MAX_SUBS 16
	ss_ref_t subs[TSS_SPLIT_MAX_SUBS];
	size_t elems1 = ss_split(a, sep1, subs, TSS_SPLIT_MAX_SUBS);
	int res = (!a || !sep1) ? 1 :
			(elems1 == 3 &&
			 !ss_cmp(ss_ref(&subs[0]), ss_crefa("how")) &&
			 !ss_cmp(ss_ref(&subs[1]), ss_crefa("are")) &&
			 !ss_cmp(ss_ref(&subs[2]), ss_crefa("you")) ? 0 : 2);
	size_t elems2 = ss_split(c, sep2, subs, TSS_SPLIT_MAX_SUBS);
	res |= elems2 == 3 &&
	       !ss_cmp(ss_ref(&subs[0]), ss_crefa("how")) &&
	       !ss_cmp(ss_ref(&subs[1]), ss_crefa("are")) &&
	       !ss_cmp(ss_ref(&subs[2]), ss_crefa("you")) ? 0 : 4;
	ss_free(&a, &sep1);
	return res;
}

static int validate_cmp(int res1, int res2)
{
	return (res1 == 0 && res2 == 0) || (res1 < 0 && res2 < 0) ||
		(res1 > 0 && res2 > 0) ? 1 : 0;
}

static int test_ss_cmp(const char *a, const char *b, int expected_cmp)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	int res = (!sa || !sb) ? 1 :
			(validate_cmp(ss_cmp(sa, sb), expected_cmp) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cmpi(const char *a, const char *b, int expected_cmp)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	int res = (!sa || !sb) ? 1 :
			(validate_cmp(ss_cmpi(sa, sb), expected_cmp) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_ncmp(const char *a, const size_t a_off, const char *b,
					const size_t n, int expected_cmp)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	int res = (!sa || !sb) ? 1 :
			(validate_cmp(ss_ncmp(sa, a_off, sb, n),
					expected_cmp) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_ncmpi(const char *a, const size_t a_off, const char *b,
					const size_t n, int expected_cmp)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	int res = (!sa || !sb) ? 1 :
			(validate_cmp(ss_ncmpi(sa, a_off, sb, n),
					expected_cmp) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_printf()
{
	ss_t *a = NULL;
	char btmp[512];
	sprintf(btmp, "%i%s%08X", 1, "hello", -1);
	ss_printf(&a, 512, "%i%s%08X", 1, "hello", -1);
	int res = !strcmp(ss_to_c(a), btmp) ? 0 : 1;
	ss_free(&a);
	return res;
}

static int test_ss_getchar()
{
	ss_t *a = ss_dup_c("12");
	size_t off = 0;
	int res = !a ? 1 : (ss_getchar(a, &off) == '1' ? 0 : 2) |
			   (ss_getchar(a, &off) == '2' ? 0 : 4) |
			   (ss_getchar(a, &off) == EOF ? 0 : 8);
	ss_free(&a);
	return res;
}

static int test_ss_putchar()
{
	ss_t *a = NULL;
	int res = ss_putchar(&a, '1') && ss_putchar(&a, '2') ? 0 : 1;
	res |= res ? 0 : (!strcmp(ss_to_c(a), "12") ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_popchar()
{
	ss_t *a = ss_dup_c("12" U8_C_N_TILDE_D1 "a");
	int res = !a ? 1 : (ss_popchar(&a) == 'a' ? 0 : 2) |
			   (ss_popchar(&a) == 0xd1 ? 0 : 4) |
			   (ss_popchar(&a) == '2' ? 0 : 8) |
			   (ss_popchar(&a) == '1' ? 0 : 16) |
			   (ss_popchar(&a) == EOF ? 0 : 32);
	ss_free(&a);
	return res;
}

static int test_ss_read_write()
{
	int res = 1;	 /* Error: can not open file */
	remove(STEST_FILE);
	FILE *f = fopen(STEST_FILE, S_FOPEN_BINARY_RW_TRUNC);
	if (f) {
		const char *a = "once upon a time";
		size_t la = strlen(a);
		ss_t *sa = ss_dup_c(a), *sb = NULL;
		res =	ss_size(sa) != la ? 2 :
			ss_write(f, sa, 0, S_NPOS) != (ssize_t)la ? 3 :
			fseek(f, 0, SEEK_SET) != 0 ? 4 :
			ss_read(&sb, f, 4000) != (ssize_t)la ? 5 :
			strcmp(ss_to_c(sa), ss_to_c(sb)) != 0 ? 6 : 0;
		ss_free(&sa, &sb);
		fclose(f);
		if (remove(STEST_FILE) != 0)
			res |= 32;
	}
	return res;
}

static int test_ss_csum32()
{
	const char *a = "hola";
	uint32_t a_crc32 = 0x6fa0f988;
	ss_t *sa = ss_dup_c(a);
	int res = ss_crc32(sa) != a_crc32 ? 1 : 0;
	ss_free(&sa);
	return res;
}

static int test_sc_utf8_to_wc(const char *utf8_char,
			      const int unicode32_expected)
{
	int uc_out = 0;
	const size_t char_size = sc_utf8_to_wc(utf8_char, 0, 6,
						&uc_out, NULL);
	return !char_size ? 1 : (uc_out == unicode32_expected ? 0 : 2);
}

static int test_sc_wc_to_utf8(const int unicode32,
			      const char *utf8_char_expected)
{
	char utf8[6];
	size_t char_size = sc_wc_to_utf8(unicode32, utf8, 0, 6);
	return !char_size ? 1 :
			(!memcmp(utf8, utf8_char_expected, char_size) ? 0 : 2);
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
	ss_t *a = ss_alloc(10);
	int res = a ? 0 : 1;
	res |= (!ss_alloc_errors(a) ? 0 : 2);
	/*
	 * 32-bit mode: ask for growing 4 GB
	 * 64-bit mode: Ask for growing 1 PB (10^15 bytes), and check
	 * memory allocation fails:
	 */
#if UINTPTR_MAX == 0xffffffff
	size_t gs = ss_grow(&a, (size_t)0xffffff00);
#elif UINTPTR_MAX > 0xffffffff
	size_t gs = ss_grow(&a, (int64_t)1000 * 1000 * 1000 * 1000 * 1000);
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
	char xutf8[5][3] = {
		{ (char)SSU8_S2, 32, '2' },
		{ (char)SSU8_S3, 32, '3' },
		{ (char)SSU8_S4, 32, '4' },
		{ (char)SSU8_S5, 32, '5' },
		{ (char)SSU8_S6, 32, '6' } };
	const ss_t *srefs[5] = {
		ss_refa_buf(xutf8[0], 3),
		ss_refa_buf(xutf8[1], 3),
		ss_refa_buf(xutf8[2], 3),
		ss_refa_buf(xutf8[3], 3),
		ss_refa_buf(xutf8[4], 3) };
	res |= (ss_len_u(srefs[0]) == 2 &&
		!ss_encoding_errors(srefs[0]) ? 0 : 16);
	res |= (ss_len_u(srefs[1]) == 1 &&
		!ss_encoding_errors(srefs[1]) ? 0 : 32);
	res |= (ss_len_u(srefs[2]) == 3 &&
		ss_encoding_errors(srefs[2]) ? 0 : 64);
	res |= (ss_len_u(srefs[3]) == 3 &&
		ss_encoding_errors(srefs[3]) ? 0 : 128);
	res |= (ss_len_u(srefs[4]) == 3 &&
		ss_encoding_errors(srefs[4]) ? 0 : 256);
	ss_clear_errors((ss_t *)srefs[2]);
	ss_clear_errors((ss_t *)srefs[3]);
	ss_clear_errors((ss_t *)srefs[4]);
	res |= (!ss_encoding_errors(srefs[2]) &&
		!ss_encoding_errors(srefs[3]) &&
		!ss_encoding_errors(srefs[4]) ? 0: 512);
	return res;
}

#define TEST_SV_ALLOC(sv_alloc_x, sv_alloc_x_t, free_op)		\
	sv_t *a = sv_alloc_x(sizeof(struct AA), 10, NULL),		\
	     *b = sv_alloc_x_t(SV_I8, 10);				\
	int res = (!a || !b) ? 1 : (sv_len(a) == 0 ? 0 : 2) |		\
				   (sv_empty(a) ? 0 : 4) |		\
				   (sv_empty(b) ? 0 : 8) |		\
				   (sv_push(&a, &a1) ? 0 : 16) |	\
				   (sv_push_i(&b, 1) ? 0 : 32) |	\
				   (!sv_empty(a) ? 0 : 64) |		\
				   (!sv_empty(b) ? 0 : 128) |		\
				   (sv_len(a) == 1 ? 0 : 256) |		\
				   (sv_len(b) == 1 ? 0 : 512) |		\
				   (!sv_push(&a, NULL) ? 0 : 1024) |	\
				   (sv_len(a) == 1 ? 0 : 2048);		\
	free_op;							\
	return res;

static int test_sv_alloc()
{
	TEST_SV_ALLOC(sv_alloc, sv_alloc_t, sv_free(&a, &b));
}

static int test_sv_alloca()
{
	TEST_SV_ALLOC(sv_alloca, sv_alloca_t, {});
}

#define TEST_SV_GROW(v, pushval, ntest, sv_alloc_f, data_id, CMPF,	\
		     initial_reserve, sv_push_x)			\
	sv_t *v = sv_alloc_f(data_id, initial_reserve CMPF);		\
	res |= !v ? 1<<(ntest*3) :					\
			(sv_push_x(&v, pushval) &&			\
			sv_len(v) == 1 &&				\
			sv_push_x(&v, pushval) &&			\
			sv_len(v) == 2 &&				\
			sv_capacity(v) >= 2) ? 0 : 2<<(ntest*3);	\
	sv_free(&v);

static int test_sv_grow()
{
	int res = 0;
	TEST_SV_GROW(a, &a1, 0, sv_alloc, sizeof(struct AA), NO_CMPF, 1,
								sv_push);
	TEST_SV_GROW(b, 123, 1, sv_alloc_t, SV_U8, X_CMPF, 1, sv_push_u);
	TEST_SV_GROW(c, 123, 2, sv_alloc_t, SV_I8, X_CMPF, 1, sv_push_i);
	TEST_SV_GROW(d, 123, 3, sv_alloc_t, SV_U16, X_CMPF, 1, sv_push_u);
	TEST_SV_GROW(e, 123, 4, sv_alloc_t, SV_I16, X_CMPF, 1, sv_push_i);
	TEST_SV_GROW(f, 123, 5, sv_alloc_t, SV_U32, X_CMPF, 1, sv_push_u);
	TEST_SV_GROW(g, 123, 6, sv_alloc_t, SV_I32, X_CMPF, 1, sv_push_i);
	TEST_SV_GROW(h, 123, 7, sv_alloc_t, SV_U64, X_CMPF, 1, sv_push_u);
	TEST_SV_GROW(i, 123, 8, sv_alloc_t, SV_I64, X_CMPF, 1, sv_push_i);
	return res;
}

#define TEST_SV_RESERVE(v, ntest, sv_alloc_f, data_id, CMPF, reserve)	\
	sv_t *v = sv_alloc_f(data_id, 1 CMPF);			      	\
	res |= !v ? 1<<(ntest*3) :					\
		      (sv_reserve(&v, reserve) &&			\
		       sv_len(v) == 0 &&				\
		       sv_capacity(v) == reserve) ? 0 : 2<<(ntest*3);	\
	sv_free(&v);

static int test_sv_reserve()
{
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
	return res;
}

#define TEST_SV_SHRINK_TO_FIT(v, ntest, alloc, push, type, CMPF, pushval, r) \
	sv_t *v = alloc(type, r CMPF);					\
	res |= !v ? 1<<(ntest*3) :					\
		(push(&v, pushval) &&					\
		 sv_shrink(&v) &&					\
		 sv_capacity(v) == sv_len(v)) ? 0 : 2<<(ntest*3);	\
	sv_free(&v)

static int test_sv_shrink()
{
	int res = 0;
	TEST_SV_SHRINK_TO_FIT(a, 0, sv_alloc, sv_push,
			      sizeof(struct AA), NO_CMPF, &a1, 100);
	TEST_SV_SHRINK_TO_FIT(b, 1, sv_alloc_t, sv_push_i, SV_I8, X_CMPF, 123,
									  100);
	TEST_SV_SHRINK_TO_FIT(c, 2, sv_alloc_t, sv_push_u, SV_U8, X_CMPF, 123,
									  100);
	TEST_SV_SHRINK_TO_FIT(d, 3, sv_alloc_t, sv_push_i, SV_I16, X_CMPF, 123,
									   100);
	TEST_SV_SHRINK_TO_FIT(e, 4, sv_alloc_t, sv_push_u, SV_U16, X_CMPF, 123,
									   100);
	TEST_SV_SHRINK_TO_FIT(f, 5, sv_alloc_t, sv_push_i, SV_I32, X_CMPF, 123,
									   100);
	TEST_SV_SHRINK_TO_FIT(g, 6, sv_alloc_t, sv_push_u, SV_U32, X_CMPF, 123,
									   100);
	TEST_SV_SHRINK_TO_FIT(h, 7, sv_alloc_t, sv_push_i, SV_I64, X_CMPF, 123,
									   100);
	TEST_SV_SHRINK_TO_FIT(i, 8, sv_alloc_t, sv_push_u, SV_U64, X_CMPF, 123,
									   100);
	return res;
}

#define TEST_SV_LEN(v, ntest, alloc, push, type, CMPF, pushval)	\
	sv_t *v = alloc(type, 1 CMPF);				\
	res |= !v ? 1<<(ntest*3) :				\
		(push(&v, pushval) && push(&v, pushval) &&	\
		 push(&v, pushval) && push(&v, pushval) &&	\
		 sv_len(v) == 4) ? 0 : 2<<(ntest*3);		\
	sv_free(&v)

static int test_sv_len()
{
	int res = 0;
	TEST_SV_LEN(a, 0, sv_alloc, sv_push, sizeof(struct AA), NO_CMPF, &a1);
	TEST_SV_LEN(b, 1, sv_alloc_t, sv_push_i, SV_I8, X_CMPF, 123);
	TEST_SV_LEN(c, 2, sv_alloc_t, sv_push_u, SV_U8, X_CMPF, 123);
	TEST_SV_LEN(d, 3, sv_alloc_t, sv_push_i, SV_I16, X_CMPF, 123);
	TEST_SV_LEN(e, 4, sv_alloc_t, sv_push_u, SV_U16, X_CMPF, 123);
	TEST_SV_LEN(f, 5, sv_alloc_t, sv_push_i, SV_I32, X_CMPF, 123);
	TEST_SV_LEN(g, 6, sv_alloc_t, sv_push_u, SV_U32, X_CMPF, 123);
	TEST_SV_LEN(h, 7, sv_alloc_t, sv_push_i, SV_I64, X_CMPF, 123);
	TEST_SV_LEN(i, 8, sv_alloc_t, sv_push_u, SV_U64, X_CMPF, 123);
	return res;
}

#ifdef SD_ENABLE_HEURISTIC_GROWTH
#define SDCMP >=
#else
#define SDCMP ==
#endif

#define TEST_SV_CAPACITY(v, ntest, alloc, push, type, CMPF, pushval)	\
	sv_t *v = alloc(type, 2 CMPF);					\
	res |= !v ? 1<<(ntest*3) :					\
		(sv_capacity(v) == 2 &&	sv_reserve(&v, 100) &&		\
		 sv_capacity(v) SDCMP 100 && sv_shrink(&v) &&		\
		 sv_capacity(v) == 0) ? 0 : 2<<(ntest*3);		\
	sv_free(&v)

static int test_sv_capacity()
{
	int res = 0;
	TEST_SV_CAPACITY(a, 0, sv_alloc, sv_push, sizeof(struct AA),
			 NO_CMPF, &a1);
	TEST_SV_CAPACITY(b, 1, sv_alloc_t, sv_push_i, SV_I8, X_CMPF, 123);
	TEST_SV_CAPACITY(c, 2, sv_alloc_t, sv_push_u, SV_U8, X_CMPF, 123);
	TEST_SV_CAPACITY(d, 3, sv_alloc_t, sv_push_i, SV_I16, X_CMPF, 123);
	TEST_SV_CAPACITY(e, 4, sv_alloc_t, sv_push_u, SV_U16, X_CMPF, 123);
	TEST_SV_CAPACITY(f, 5, sv_alloc_t, sv_push_i, SV_I32, X_CMPF, 123);
	TEST_SV_CAPACITY(g, 6, sv_alloc_t, sv_push_u, SV_U32, X_CMPF, 123);
	TEST_SV_CAPACITY(h, 7, sv_alloc_t, sv_push_i, SV_I64, X_CMPF, 123);
	TEST_SV_CAPACITY(i, 8, sv_alloc_t, sv_push_u, SV_U64, X_CMPF, 123);
	return res;
}

static int test_sv_capacity_left()
{
	const size_t init_alloc = 10;
	sv_t *a = sv_alloc_t(SV_I8, init_alloc);
	int res = !a ? 1 : (sv_capacity(a) - sv_len(a) == init_alloc ? 0 : 2) |
			   (sv_capacity_left(a) == init_alloc ? 0 : 4) |
			   (sv_push_i(&a, 1) ? 0 : 8) |
			   (sv_capacity_left(a) == (init_alloc - 1) ? 0 : 16);
	sv_free(&a);
	return res;
}

static int test_sv_get_buffer()
{
	sv_t *a = sv_alloc_t(SV_I8, 0),
	     *b = sv_alloca_t(SV_I8, 4),
	     *c = sv_alloc_t(SV_U32, 0),
	     *d = sv_alloca_t(SV_U32, 1);
	int res = !a || !b || !c || !d ? 1 :
			(sv_push_i(&a, 1) ? 0 : 2) |
			(sv_push_i(&a, 2) ? 0 : 4) |
			(sv_push_i(&a, 2) ? 0 : 8) |
			(sv_push_i(&a, 1) ? 0 : 0x10) |
			(sv_push_i(&b, 1) ? 0 : 0x20) |
			(sv_push_i(&b, 2) ? 0 : 0x40) |
			(sv_push_i(&b, 2) ? 0 : 0x80) |
			(sv_push_i(&b, 1) ? 0 : 0x100) |
			(sv_push_u(&c, 0x01020201) ? 0 : 0x200) |
			(sv_push_u(&d, 0x01020201) ? 0 : 0x400);
	if (!res) {
		unsigned ai = S_LD_U32(sv_get_buffer(a)),
			 bi = S_LD_U32(sv_get_buffer(b)),
			 air = S_LD_U32(sv_get_buffer_r(a)),
			 bir = S_LD_U32(sv_get_buffer_r(b)),
			 cir = S_LD_U32(sv_get_buffer(c)),
			 dir = S_LD_U32(sv_get_buffer(d));
		if (ai != bi || air != bir || ai != air || ai != cir ||
		    ai != dir)
			res |= 0x800;
		if (ai != 0x01020201)
			res |= 0x1000;
		size_t sa = sv_get_buffer_size(a),
		       sb = sv_get_buffer_size(b),
		       sc = sv_get_buffer_size(c),
		       sd = sv_get_buffer_size(d);
		if (sa != sb || sa != sc || sa != sd)
			res |= 0x2000;
	}
	sv_free(&a, &c);
	return res;
}

static int test_sv_elem_size()
{
	int res =  (sv_elem_size(SV_I8) == 1  ? 0 : 2) |
		   (sv_elem_size(SV_U8) == 1  ? 0 : 4) |
		   (sv_elem_size(SV_I16) == 2  ? 0 : 8) |
		   (sv_elem_size(SV_U16) == 2  ? 0 : 16) |
		   (sv_elem_size(SV_I32) == 4  ? 0 : 32) |
		   (sv_elem_size(SV_U32) == 4  ? 0 : 64) |
		   (sv_elem_size(SV_I64) == 8  ? 0 : 128) |
		   (sv_elem_size(SV_U64) == 8  ? 0 : 256);
	return res;
}

#define TEST_SV_DUP(v, ntest, alloc, push, check, check2, type, CMPF,	\
		    pushval)						\
	sv_t *v = alloc(type, 0 CMPF);					\
	push(&v, pushval);						\
	sv_t *v##2 = sv_dup(v);						\
	res |= !v ? 1 << (ntest*3) :					\
		((check) && (check2)) ? 0 : 2 << (ntest*3);		\
	sv_free(&v, &v##2);

static int test_sv_dup()
{
	int res = 0;
	const int val = 123;
	TEST_SV_DUP(z, 0, sv_alloc, sv_push,
		    ((const struct AA *)sv_at(z, 0))->a == a1.a,
		    ((const struct AA *)sv_pop(z))->a ==
					((const struct AA *)sv_pop(z2))->a,
		    sizeof(struct AA), NO_CMPF, &a1);
	#define SVIAT(v) sv_at_i(v, 0) == val
	#define SVUAT(v) sv_at_u(v, 0) == (unsigned)val
	#define CHKPI(v) sv_pop_i(v) == sv_pop_i(v##2)
	#define CHKPU(v) sv_pop_u(v) == sv_pop_u(v##2)
	TEST_SV_DUP(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), CHKPI(b), SV_I8,
		    X_CMPF, val);
	TEST_SV_DUP(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), CHKPU(c), SV_U8,
		    X_CMPF, val);
	TEST_SV_DUP(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), CHKPI(d), SV_I16,
		    X_CMPF, val);
	TEST_SV_DUP(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), CHKPU(e), SV_U16,
		    X_CMPF, val);
	TEST_SV_DUP(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), CHKPI(f), SV_I32,
		    X_CMPF, val);
	TEST_SV_DUP(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), CHKPU(g), SV_U32,
		    X_CMPF, val);
	TEST_SV_DUP(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), CHKPI(h), SV_I64,
		    X_CMPF, val);
	TEST_SV_DUP(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), CHKPU(i), SV_U64,
		    X_CMPF, val);
	#undef SVIAT
	#undef SVUAT
	#undef CHKPI
	#undef CHKPU
	return res;
}

#define TEST_SV_DUP_ERASE(v, ntest, alloc, push, check, type, CMPF, a, b) \
	sv_t *v = alloc(type, 0 CMPF);					  \
	push(&v, a); push(&v, b); push(&v, b); push(&v, b); push(&v, b);  \
	push(&v, b); push(&v, a); push(&v, b); push(&v, b); push(&v, b);  \
	sv_t *v##2 = sv_dup_erase(v, 1, 5);				  \
	res |= !v ? 1<<(ntest*3) : (check) ? 0 : 2<<(ntest*3);		  \
	sv_free(&v, &v##2);

static int test_sv_dup_erase()
{
	int res = 0;
	const int r = 12, s = 34;
	TEST_SV_DUP_ERASE(z, 0, sv_alloc, sv_push,
	    ((const struct AA *)sv_at(z2, 0))->a ==
				((const struct AA *)sv_at(z2, 1))->a,
	    sizeof(struct AA), NO_CMPF, &a1, &a2);
	#define SVIAT(v) sv_at_i(v##2, 0) == sv_at_i(v##2, 1)
	#define SVUAT(v) sv_at_u(v##2, 0) == sv_at_u(v##2, 1)
	TEST_SV_DUP_ERASE(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), SV_I8, X_CMPF,
			  r, s);
	TEST_SV_DUP_ERASE(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), SV_U8, X_CMPF,
			  r, s);
	TEST_SV_DUP_ERASE(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), SV_I16, X_CMPF,
			  r, s);
	TEST_SV_DUP_ERASE(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), SV_U16, X_CMPF,
			  r, s);
	TEST_SV_DUP_ERASE(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), SV_I32, X_CMPF,
			  r, s);
	TEST_SV_DUP_ERASE(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), SV_U32, X_CMPF,
			  r, s);
	TEST_SV_DUP_ERASE(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), SV_I64, X_CMPF,
			  r, s);
	TEST_SV_DUP_ERASE(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), SV_U64, X_CMPF,
			  r, s);
	#undef SVIAT
	#undef SVUAT
	return res;
}

#define TEST_SV_DUP_RESIZE(v, ntest, alloc, push, type, CMPF, a, b)	       \
	sv_t *v = alloc(type, 0 CMPF);					       \
	push(&v, a); push(&v, b); push(&v, b); push(&v, b); push(&v, b);       \
	push(&v, b); push(&v, a); push(&v, b); push(&v, b); push(&v, b);       \
	sv_t *v##2 = sv_dup_resize(v, 2);				       \
	res |= !v ? 1<<(ntest*3) :					       \
		(sv_size(v##2) == 2 ? 0 : 2<<(ntest*3)) |		       \
		(!sv_ncmp(v, 0, v##2, 0, sv_size(v##2)) ? 0 : 4 << (ntest*3)) |\
		(sv_ncmp(v, 0, v##2, 0, sv_size(v)) > 0 ? 0 : 8 << (ntest*3)) |\
		(sv_ncmp(v##2, 0, v, 0, sv_size(v)) < 0 ? 0 : 16 << (ntest*3));\
	sv_free(&v, &v##2);

static int test_sv_dup_resize()
{
	int res = 0;
	const int r = 12, s = 34;
	TEST_SV_DUP_RESIZE(z, 0, sv_alloc, sv_push, sizeof(struct AA), NO_CMPF,
			   &a1, &a2);
	TEST_SV_DUP_RESIZE(b, 1, sv_alloc_t, sv_push_i, SV_I8, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(c, 2, sv_alloc_t, sv_push_u, SV_U8, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(d, 3, sv_alloc_t, sv_push_i, SV_I16, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(e, 4, sv_alloc_t, sv_push_u, SV_U16, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(f, 5, sv_alloc_t, sv_push_i, SV_I32, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(g, 6, sv_alloc_t, sv_push_u, SV_U32, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(h, 7, sv_alloc_t, sv_push_i, SV_I64, X_CMPF, r, s);
	TEST_SV_DUP_RESIZE(i, 8, sv_alloc_t, sv_push_u, SV_U64, X_CMPF, r, s);
	return res;
}

#define TEST_SV_CPY(v, nt, alloc, push, type, CMPF, a, b)	       	   \
	sv_t *v = alloc(type, 0 CMPF);					   \
	push(&v, a); push(&v, b); push(&v, a); push(&v, b); push(&v, a);   \
	push(&v, b); push(&v, a); push(&v, b); push(&v, a); push(&v, b);   \
	sv_t *v##2 = NULL;						   \
	sv_cpy(&v##2, v);				       		   \
	res |= !v ? 1 << (nt * 3) :					   \
		(sv_size(v) == sv_size(v##2) ? 0 : 2 << (nt * 3)) |	   \
		(!sv_ncmp(v, 0, v##2, 0, sv_size(v)) ? 0 : 4 << (nt * 3)); \
		sv_free(&v, &v##2);

static int test_sv_cpy()
{
	int res = 0;
	const int r = 12, s = 34;
	TEST_SV_CPY(z, 0, sv_alloc, sv_push, sizeof(struct AA), NO_CMPF,
		    &a1, &a2);
	TEST_SV_CPY(b, 1, sv_alloc_t, sv_push_i, SV_I8, X_CMPF, r, s);
	TEST_SV_CPY(c, 2, sv_alloc_t, sv_push_u, SV_U8, X_CMPF, r, s);
	TEST_SV_CPY(d, 3, sv_alloc_t, sv_push_i, SV_I16, X_CMPF, r, s);
	TEST_SV_CPY(e, 4, sv_alloc_t, sv_push_u, SV_U16, X_CMPF, r, s);
	TEST_SV_CPY(f, 5, sv_alloc_t, sv_push_i, SV_I32, X_CMPF, r, s);
	TEST_SV_CPY(g, 6, sv_alloc_t, sv_push_u, SV_U32, X_CMPF, r, s);
	TEST_SV_CPY(h, 7, sv_alloc_t, sv_push_i, SV_I64, X_CMPF, r, s);
	TEST_SV_CPY(i, 8, sv_alloc_t, sv_push_u, SV_U64, X_CMPF, r, s);
	sv_t *v1 = sv_alloc_t(SV_I8, 0), *v2 = sv_alloc_t(SV_I64, 0),
	     *v1a3 = sv_alloca_t(SV_I8, 3), *v1a24 = sv_alloca_t(SV_I8, 24);
	sv_push_i(&v1, 1);
	sv_push_i(&v1, 2);
	sv_push_i(&v1, 3);
	sv_push_i(&v2, 1);
	sv_push_i(&v2, 2);
	sv_push_i(&v2, 3);
	sv_cpy(&v1, v2);
	sv_cpy(&v1a3, v2);
	sv_cpy(&v1a24, v2);
	res |= (sv_size(v1) == 3 ? 0 : 1<<25);
	res |= (sv_size(v1a3) == 0 ? 0 : 1<<26);
	res |= (sv_size(v1a24) == 3 ? 0 : 1<<27);
	res |= (sv_at_i(v1, 0) == 1 && sv_at_i(v1, 1) == 2 &&
		sv_at_i(v1, 2) == 3 ? 0 : 1<<28);
	res |= (sv_at_i(v1a24, 0) == 1 && sv_at_i(v1a24, 1) == 2 &&
		sv_at_i(v1a24, 2) == 3 ? 0 : 1<<29);
	sv_free(&v1, &v2);
	return res;
}

#define TEST_SV_CPY_ERASE(v, ntest, alloc, push, check, type, CMPF, a, b)\
	sv_t *v = alloc(type, 0 CMPF);					 \
	push(&v, a); push(&v, b); push(&v, b); push(&v, b); push(&v, b); \
	push(&v, b); push(&v, a); push(&v, b); push(&v, b); push(&v, b); \
	sv_t *v##2 = NULL;						 \
	sv_cpy_erase(&v##2, v, 1, 5);					 \
	res |= !v ? 1<<(ntest*3) : (check) ? 0 : 2<<(ntest*3);		 \
	sv_free(&v, &v##2);

static int test_sv_cpy_erase()
{
	int res = 0;
	const int r = 12, s = 34;
	TEST_SV_CPY_ERASE(z, 0, sv_alloc, sv_push,
	    ((const struct AA *)sv_at(z2, 0))->a ==
				((const struct AA *)sv_at(z2, 1))->a,
	    sizeof(struct AA), NO_CMPF, &a1, &a2);
	#define SVIAT(v) sv_at_i(v##2, 0) == sv_at_i(v##2, 1)
	#define SVUAT(v) sv_at_u(v##2, 0) == sv_at_u(v##2, 1)
	TEST_SV_CPY_ERASE(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), SV_I8, X_CMPF,
			  r, s);
	TEST_SV_CPY_ERASE(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), SV_U8, X_CMPF,
			  r, s);
	TEST_SV_CPY_ERASE(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), SV_I16, X_CMPF,
			  r, s);
	TEST_SV_CPY_ERASE(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), SV_U16, X_CMPF,
			  r, s);
	TEST_SV_CPY_ERASE(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), SV_I32, X_CMPF,
			  r, s);
	TEST_SV_CPY_ERASE(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), SV_U32, X_CMPF,
			  r, s);
	TEST_SV_CPY_ERASE(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), SV_I64, X_CMPF,
			  r, s);
	TEST_SV_CPY_ERASE(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), SV_U64, X_CMPF,
			  r, s);
	#undef SVIAT
	#undef SVUAT
	return res;
}

#define TEST_SV_CPY_RESIZE(v, ntest, alloc, push, type, CMPF, a, b)	       \
	sv_t *v = alloc(type, 0 CMPF);					       \
	push(&v, a); push(&v, b); push(&v, b); push(&v, b); push(&v, b);       \
	push(&v, b); push(&v, a); push(&v, b); push(&v, b); push(&v, b);       \
	sv_t *v##2 = NULL;						       \
	sv_cpy_resize(&v##2, v, 2);					       \
	res |= !v ? 1<<(ntest*3) :					       \
		(sv_size(v##2) == 2 ? 0 : 2<<(ntest*3)) |		       \
		(!sv_ncmp(v, 0, v##2, 0, sv_size(v##2)) ? 0 : 4 << (ntest*3)) |\
		(sv_ncmp(v, 0, v##2, 0, sv_size(v)) > 0 ? 0 : 8 << (ntest*3)) |\
		(sv_ncmp(v##2, 0, v, 0, sv_size(v)) < 0 ? 0 : 16 << (ntest*3));\
	sv_free(&v, &v##2);

static int test_sv_cpy_resize()
{
	int res = 0;
	const int r = 12, s = 34;
	TEST_SV_CPY_RESIZE(z, 0, sv_alloc, sv_push, sizeof(struct AA), NO_CMPF,
			   &a1, &a2);
	TEST_SV_CPY_RESIZE(b, 1, sv_alloc_t, sv_push_i, SV_I8, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(c, 2, sv_alloc_t, sv_push_u, SV_U8, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(d, 3, sv_alloc_t, sv_push_i, SV_I16, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(e, 4, sv_alloc_t, sv_push_u, SV_U16, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(f, 5, sv_alloc_t, sv_push_i, SV_I32, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(g, 6, sv_alloc_t, sv_push_u, SV_U32, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(h, 7, sv_alloc_t, sv_push_i, SV_I64, X_CMPF, r, s);
	TEST_SV_CPY_RESIZE(i, 8, sv_alloc_t, sv_push_u, SV_U64, X_CMPF, r, s);
	return res;
}

#define TEST_SV_CAT(v, ntest, alloc, push, check, check2, type, CMPF, pushval)\
	sv_t *v = alloc(type, 0 CMPF);					      \
	push(&v, pushval);						      \
	sv_t *v##2 = sv_dup(v);						      \
	res |= !v ? 1<<(ntest*3) :					      \
		(sv_cat(&v, v##2) && sv_cat(&v, v) && sv_len(v) == 4 &&	      \
		 (check) && (check2)) ? 0 : 2<<(ntest*3);		      \
	sv_free(&v, &v##2);

static int test_sv_cat()
{
	int res = 0;
	const int w = 123;
	TEST_SV_CAT(z, 0, sv_alloc, sv_push,
		    ((const struct AA *)sv_at(z, 3))->a == a1.a,
		    ((const struct AA *)sv_pop(z))->a ==
					((const struct AA *)sv_pop(z2))->a,
		    sizeof(struct AA), NO_CMPF, &a1);
	#define SVIAT(v) sv_at_i(v, 3) == w
	#define SVUAT(v) sv_at_u(v, 3) == (unsigned)w
	#define CHKPI(v) sv_pop_i(v) == sv_pop_i(v##2)
	#define CHKPU(v) sv_pop_u(v) == sv_pop_u(v##2)
	TEST_SV_CAT(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), CHKPI(b), SV_I8,
		    X_CMPF, w);
	TEST_SV_CAT(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), CHKPU(c), SV_U8,
		    X_CMPF, w);
	TEST_SV_CAT(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), CHKPI(d), SV_I16,
		    X_CMPF, w);
	TEST_SV_CAT(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), CHKPU(e), SV_U16,
		    X_CMPF, w);
	TEST_SV_CAT(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), CHKPI(f), SV_I32,
		    X_CMPF, w);
	TEST_SV_CAT(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), CHKPU(g), SV_U32,
		    X_CMPF, w);
	TEST_SV_CAT(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), CHKPI(h), SV_I64,
		    X_CMPF, w);
	TEST_SV_CAT(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), CHKPU(i), SV_U64,
		    X_CMPF, w);
	#undef SVIAT
	#undef SVUAT
	#undef CHKPI
	#undef CHKPU
	return res;
}

#define TEST_SV_CAT_ERASE(v, ntest, alloc, push, check, type, CMPF, a, b)\
	sv_t *v = alloc(type, 0 CMPF), *v##2 = alloc(type, 0 CMPF);	 \
	push(&v, a); push(&v, a); push(&v, a); push(&v, a); push(&v, a); \
	push(&v, a); push(&v, b); push(&v, a); push(&v, a); push(&v, a); \
	push(&v##2, b);							 \
	sv_cat_erase(&v##2, v, 1, 5);					 \
	res |= !v ? 1<<(ntest*3) :					 \
		    ((check) ? 0 : 2 << (ntest * 3)) |			 \
		    (sv_size(v##2) == 6 ? 0 : 4 << (ntest * 3));	 \
	sv_free(&v, &v##2);

static int test_sv_cat_erase()
{
	int res = 0;
	const int r = 12, s = 34;
	TEST_SV_CAT_ERASE(z, 0, sv_alloc, sv_push,
	    ((const struct AA *)sv_at(z2, 0))->a ==
				((const struct AA *)sv_at(z2, 2))->a,
	    sizeof(struct AA), NO_CMPF, &a1, &a2);
	#define SVIAT(v) sv_at_i(v##2, 0) == sv_at_i(v##2, 2)
	#define SVUAT(v) sv_at_u(v##2, 0) == sv_at_u(v##2, 2)
	TEST_SV_CAT_ERASE(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), SV_I8,
			  X_CMPF, r, s);
	TEST_SV_CAT_ERASE(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), SV_U8,
			  X_CMPF, r, s);
	TEST_SV_CAT_ERASE(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), SV_I16,
			  X_CMPF, r, s);
	TEST_SV_CAT_ERASE(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), SV_U16,
			  X_CMPF, r, s);
	TEST_SV_CAT_ERASE(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), SV_I32,
			  X_CMPF, r, s);
	TEST_SV_CAT_ERASE(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), SV_U32,
			  X_CMPF, r, s);
	TEST_SV_CAT_ERASE(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), SV_I64,
			  X_CMPF, r, s);
	TEST_SV_CAT_ERASE(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), SV_U64,
			  X_CMPF, r, s);
	#undef SVIAT
	#undef SVUAT
	return res;
}

#define TEST_SV_CAT_RESIZE(v, ntest, alloc, push, type, CMPF, a, b)	       \
	sv_t *v = alloc(type, 0 CMPF);					       \
	push(&v, a); push(&v, b); push(&v, b); push(&v, b); push(&v, b);       \
	push(&v, b); push(&v, a); push(&v, b); push(&v, b); push(&v, b);       \
	sv_t *v##2 = NULL;						       \
	sv_cat_resize(&v##2, v, 2);					       \
	res |= !v ? 1<<(ntest*3) :					       \
		(sv_size(v##2) == 2 ? 0 : 2<<(ntest*3)) |		       \
		(!sv_ncmp(v, 0, v##2, 0, sv_size(v##2)) ? 0 : 4 << (ntest*3)) |\
		(sv_ncmp(v, 0, v##2, 0, sv_size(v)) > 0 ? 0 : 8 << (ntest*3)) |\
		(sv_ncmp(v##2, 0, v, 0, sv_size(v)) < 0 ? 0 : 16 << (ntest*3));\
	sv_free(&v, &v##2);

static int test_sv_cat_resize()
{
	int res = 0;
	const int r = 12, s = 34;
	TEST_SV_CAT_RESIZE(z, 0, sv_alloc, sv_push, sizeof(struct AA), NO_CMPF,
			   &a1, &a2);
	TEST_SV_CAT_RESIZE(b, 1, sv_alloc_t, sv_push_i, SV_I8, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(c, 2, sv_alloc_t, sv_push_u, SV_U8, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(d, 3, sv_alloc_t, sv_push_i, SV_I16, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(e, 4, sv_alloc_t, sv_push_u, SV_U16, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(f, 5, sv_alloc_t, sv_push_i, SV_I32, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(g, 6, sv_alloc_t, sv_push_u, SV_U32, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(h, 7, sv_alloc_t, sv_push_i, SV_I64, X_CMPF, r, s);
	TEST_SV_CAT_RESIZE(i, 8, sv_alloc_t, sv_push_u, SV_U64, X_CMPF, r, s);
	return res;
}

#define TEST_SV_ERASE(v, ntest, alloc, push, check, type, CMPF, a, b)	 \
	sv_t *v = alloc(type, 0 CMPF);					 \
	push(&v, b); push(&v, a); push(&v, a); push(&v, a); push(&v, a); \
	push(&v, a); push(&v, b); push(&v, a); push(&v, a); push(&v, a); \
	sv_erase(&v, 1, 5);						 \
	res |= !v ? 1 << (ntest * 3) :					 \
		    ((check) ? 0 : 2 << (ntest * 3)) |			 \
		    (sv_size(v) == 5 ? 0 : 4 << (ntest * 3));		 \
	sv_free(&v);

static int test_sv_erase()
{
	int res = 0;
	const int r = 12, s = 34;
	TEST_SV_ERASE(z, 0, sv_alloc, sv_push,
	    ((const struct AA *)sv_at(z, 0))->a ==
		    ((const struct AA *)sv_at(z, 1))->a,
	    sizeof(struct AA), NO_CMPF, &a1, &a2);
	#define SVIAT(v) sv_at_i(v, 0) == sv_at_i(v, 1)
	#define SVUAT(v) sv_at_u(v, 0) == sv_at_u(v, 1)
	TEST_SV_ERASE(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), SV_I8,
		      X_CMPF, r, s);
	TEST_SV_ERASE(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), SV_U8,
		      X_CMPF, r, s);
	TEST_SV_ERASE(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), SV_I16,
		      X_CMPF, r, s);
	TEST_SV_ERASE(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), SV_U16,
		      X_CMPF, r, s);
	TEST_SV_ERASE(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), SV_I32,
		      X_CMPF, r, s);
	TEST_SV_ERASE(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), SV_U32,
		      X_CMPF, r, s);
	TEST_SV_ERASE(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), SV_I64,
		      X_CMPF, r, s);
	TEST_SV_ERASE(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), SV_U64,
		      X_CMPF, r, s);
	#undef SVIAT
	#undef SVUAT
	return res;
}

#define TEST_SV_RESIZE(v, ntest, alloc, push, check, type, CMPF, a, b)	 \
	sv_t *v = alloc(type, 0 CMPF);					 \
	push(&v, b); push(&v, b); push(&v, a); push(&v, a); push(&v, a); \
	push(&v, a); push(&v, b); push(&v, a); push(&v, a); push(&v, a); \
	sv_resize(&v, 5);						 \
	res |= !v ? 1 << (ntest * 3) :					 \
		    ((check) ? 0 : 2 << (ntest * 3)) |			 \
		    (sv_size(v) == 5 ? 0 : 4 << (ntest * 3));		 \
	sv_clear(v);							 \
	res |= (!sv_size(v) ? 0 : 4 << (ntest * 3));			 \
	sv_free(&v);

static int test_sv_resize()
{
	int res = 0;
	const int r = 12, s = 34;
	TEST_SV_RESIZE(z, 0, sv_alloc, sv_push,
	    ((const struct AA *)sv_at(z, 0))->a ==
		    ((const struct AA *)sv_at(z, 1))->a,
	    sizeof(struct AA), NO_CMPF, &a1, &a2);
	#define SVIAT(v) sv_at_i(v, 0) == sv_at_i(v, 1)
	#define SVUAT(v) sv_at_u(v, 0) == sv_at_u(v, 1)
	TEST_SV_RESIZE(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), SV_I8,
		       X_CMPF, r, s);
	TEST_SV_RESIZE(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), SV_U8,
		       X_CMPF, r, s);
	TEST_SV_RESIZE(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), SV_I16,
		       X_CMPF, r, s);
	TEST_SV_RESIZE(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), SV_U16,
		       X_CMPF, r, s);
	TEST_SV_RESIZE(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), SV_I32,
		       X_CMPF, r, s);
	TEST_SV_RESIZE(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), SV_U32,
		       X_CMPF, r, s);
	TEST_SV_RESIZE(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), SV_I64,
		       X_CMPF, r, s);
	TEST_SV_RESIZE(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), SV_U64,
		       X_CMPF, r, s);
	#undef SVIAT
	#undef SVUAT
	return res;
}

#define TEST_SV_SORT(v, ntest, alloc, push, type, CMPF, a, b, c)	 \
	sv_t *v = alloc(type, 0 CMPF);					 \
	push(&v, c); push(&v, b); push(&v, a); push(&v, a); push(&v, b); \
	push(&v, c); push(&v, c); push(&v, b); push(&v, a);		 \
	sv_sort(v);							 \
	int v##i, v##r = 0;						 \
	for (v##i = 1; v##i < 9 && v##r <= 0; v##i++)			 \
		v##r = sv_cmp(v, (size_t)(v##i - 1), (size_t)v##i);	 \
	res |= !v ? 1 << (ntest * 3) :					 \
		    v##r <= 0 ? 0 : 2 << (ntest * 3);			 \
	sv_free(&v);

#define BUILD_CMPF(FN, T)				\
	int FN(const void *a, const void *b)		\
	{						\
		return *(T *)(a) < *(T *)(b) ? -1 :	\
		       *(T *)(a) == *(T *)(b) ? 0 : 1;	\
	}

BUILD_CMPF(cmp8i, int8_t)
BUILD_CMPF(cmp8u, uint8_t)
BUILD_CMPF(cmp16i, int16_t)
BUILD_CMPF(cmp16u, uint16_t)
BUILD_CMPF(cmp32i, int32_t)
BUILD_CMPF(cmp32u, uint32_t)
BUILD_CMPF(cmp64i, int64_t)
BUILD_CMPF(cmp64u, uint64_t)

static int test_sv_sort()
{
	int res = 0;
	const int r = 12, s = 34, t = -1;
	const unsigned tu = 11;
	/*
	 * Generic sort tests
	 */
	TEST_SV_SORT(z, 0, sv_alloc, sv_push,
		sizeof(struct AA), AA_CMPF, &a1, &a2, &a1);
	TEST_SV_SORT(b, 1, sv_alloc_t, sv_push_i, SV_I8, X_CMPF, r, s, t);
	TEST_SV_SORT(c, 2, sv_alloc_t, sv_push_u, SV_U8, X_CMPF, r, s, tu);
	TEST_SV_SORT(d, 3, sv_alloc_t, sv_push_i, SV_I16, X_CMPF, r, s, t);
	TEST_SV_SORT(e, 4, sv_alloc_t, sv_push_u, SV_U16, X_CMPF, r, s, tu);
	TEST_SV_SORT(f, 5, sv_alloc_t, sv_push_i, SV_I32, X_CMPF, r, s, t);
	TEST_SV_SORT(g, 6, sv_alloc_t, sv_push_u, SV_U32, X_CMPF, r, s, tu);
	TEST_SV_SORT(h, 7, sv_alloc_t, sv_push_i, SV_I64, X_CMPF, r, s, t);
	TEST_SV_SORT(j, 8, sv_alloc_t, sv_push_u, SV_U64, X_CMPF, r, s, tu);
	/*
	 * Integer-specific sort tests
	 */
	sv_t *v8i = sv_alloc_t(SV_I8, 0), *v8u = sv_alloc_t(SV_U8, 0),
	     *v16i = sv_alloc_t(SV_I16, 0), *v16u = sv_alloc_t(SV_U16, 0),
	     *v32i = sv_alloc_t(SV_I32, 0), *v32u = sv_alloc_t(SV_U32, 0),
	     *v64i = sv_alloc_t(SV_I64, 0), *v64u = sv_alloc_t(SV_U64, 0);
	int i, nelems = 1000;
	int8_t *r8i = (int8_t *)malloc(nelems * sizeof(int8_t));
	uint8_t *r8u = (uint8_t *)malloc(nelems * sizeof(uint8_t));
	int16_t *r16i = (int16_t *)malloc(nelems * sizeof(int16_t));
	uint16_t *r16u = (uint16_t *)malloc(nelems * sizeof(uint16_t));
	int32_t *r32i = (int32_t *)malloc(nelems * sizeof(int32_t));
	uint32_t *r32u = (uint32_t *)malloc(nelems * sizeof(uint32_t));
	int64_t *r64i = (int64_t *)malloc(nelems * sizeof(int64_t));
	uint64_t *r64u = (uint64_t *)malloc(nelems * sizeof(uint64_t));
	for (i = 0; i < nelems; i += 2) {
		sv_push_i(&v8i, i & 0xff); sv_push_u(&v8u, i & 0xff);
		sv_push_i(&v16i, i); sv_push_u(&v16u, i);
		sv_push_i(&v32i, i); sv_push_u(&v32u, i);
		sv_push_i(&v64i, i); sv_push_u(&v64u, i);
		r8i[i] = i & 0xff; r8u[i] = i & 0xff; r16i[i] = i;
		r16u[i] = i; r32i[i] = i; r32u[i] = i; r64i[i] = i; r64u[i] = i;
	}
	for (i = 1; i < nelems; i += 2) {
		sv_push_i(&v8i, i & 0xff); sv_push_u(&v8u, i & 0xff);
		sv_push_i(&v16i, i); sv_push_u(&v16u, i);
		sv_push_i(&v32i, i); sv_push_u(&v32u, i);
		sv_push_i(&v64i, i); sv_push_u(&v64u, i);
		r8i[i] = i & 0xff; r8u[i] = i & 0xff; r16i[i] = i;
		r16u[i] = i; r32i[i] = i; r32u[i] = i; r64i[i] = i; r64u[i] = i;
	}
	sv_sort(v8i); sv_sort(v8u); sv_sort(v16i); sv_sort(v16u);
	sv_sort(v32i); sv_sort(v32u); sv_sort(v64i); sv_sort(v64u);
	qsort(r8i, nelems, 1, cmp8i);
	qsort(r8u, nelems, 1, cmp8u);
	qsort(r16i, nelems, 2, cmp16i);
	qsort(r16u, nelems, 2, cmp16u);
	qsort(r32i, nelems, 4, cmp32i);
	qsort(r32u, nelems, 4, cmp32u);
	qsort(r64i, nelems, 8, cmp64i);
	qsort(r64u, nelems, 8, cmp64u);
	for (i = 0; i < nelems; i ++) {
		res |= sv_at_i(v8i, i) == r8i[i] &&
		       sv_at_u(v8u, i) == r8u[i] &&
		       (int)sv_at_i(v16i, i) == i &&
		       (int)sv_at_u(v16u, i) == i &&
		       (int)sv_at_i(v32i, i) == i &&
		       (int)sv_at_u(v32u, i) == i &&
		       (int)sv_at_i(v64i, i) == i &&
		       (int)sv_at_u(v64u, i) == i ? 0 : 1 << 31;
	}
	sv_free(&v8i, &v8u, &v16i, &v16u, &v32i, &v32u, &v64i, &v64u);
	free(r8i); free(r8u); free(r16i); free(r16u);
	free(r32i); free(r32u); free(r64i); free(r64u);
	return res;
}

#define TEST_SV_FIND(v, ntest, alloc, push, check, type, CMPF, a, b)	 \
	sv_t *v = alloc(type, 0 CMPF);					 \
	push(&v, a); push(&v, a); push(&v, a); push(&v, a); push(&v, a); \
	push(&v, a); push(&v, b); push(&v, a); push(&v, a); push(&v, a); \
	res |= !v ? 1 << (ntest * 3) :					 \
		    ((check) ? 0 : 2 << (ntest * 3)) |			 \
		    (sv_size(v) == 10 ? 0 : 4 << (ntest * 3));		 \
	sv_free(&v);

static int test_sv_find()
{
	int res = 0;
	const int r = 12, s = 34;
	TEST_SV_FIND(z, 0, sv_alloc, sv_push, sv_find(z, 0, &a2) == 6,
		     sizeof(struct AA), NO_CMPF, &a1, &a2);
	#define SVIAT(v) sv_find_i(v, 0, s) == 6
	#define SVUAT(v) sv_find_u(v, 0, s) == 6
	TEST_SV_FIND(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), SV_I8,
		     X_CMPF, r, s);
	TEST_SV_FIND(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), SV_U8,
		     X_CMPF, r, s);
	TEST_SV_FIND(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), SV_I16,
		     X_CMPF, r, s);
	TEST_SV_FIND(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), SV_U16,
		     X_CMPF, r, s);
	TEST_SV_FIND(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), SV_I32,
		     X_CMPF, r, s);
	TEST_SV_FIND(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), SV_U32,
		     X_CMPF, r, s);
	TEST_SV_FIND(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), SV_I64,
		     X_CMPF, r, s);
	TEST_SV_FIND(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), SV_U64,
		     X_CMPF, r, s);
	#undef SVIAT
	#undef SVUAT
	return res;
}

static int test_sv_push_pop_set()
{
	size_t as = 10;
	sv_t *a = sv_alloc(sizeof(struct AA), as, NULL),
	     *b = sv_alloca(sizeof(struct AA), as, NULL);
	const struct AA *t = NULL;
	int res = (!a || !b) ? 1 : 0;
	if (!res) {
		res |= (sv_push(&a, &a2, &a2, &a1) ? 0 : 4);
		res |= (sv_len(a) == 3 ? 0 : 8);
		res |= (((t = (const struct AA *)sv_pop(a)) &&
			t->a == a1.a && t->b == a1.b)? 0 : 16);
		res |= (sv_len(a) == 2 ? 0 : 32);
		res |= (sv_push(&b, &a2) ? 0 : 64);
		res |= (sv_len(b) == 1 ? 0 : 128);
		res |= (((t = (struct AA *)sv_pop(b)) &&
			 t->a == a2.a && t->b == a2.b)? 0 : 256);
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

static int test_sv_push_pop_set_i()
{
	enum eSV_Type t[] = { SV_I8, SV_I16, SV_I32, SV_I64 };
	int i = 0, ntests = sizeof(t)/sizeof(t[0]), res = 0;
	for (; i < ntests; i++) {
#ifdef _MSC_VER /* supress alloca() warning */
#pragma warning(disable: 6255)
#pragma warning(disable: 6263)
#endif
		size_t as = 10;
		sv_t *a = sv_alloc_t(t[i], as);
		sv_t *b = sv_alloca_t(t[i], as);
		do {
			if (!a || !b) {
				res |= 1 << (i * 4);
				break;
			}
			if (!sv_push_i(&a, i) || !sv_push_i(&b, i)) {
				res |= 2 << (i * 4);
				break;
			}
			if (sv_pop_i(a) != i || sv_pop_i(b) != i) {
				res |= 3 << (i * 4);
				break;
			}
			if (sv_len(a) != 0 || sv_len(b) != 0 ){
				res |= 4 << (i * 4);
				break;
			}
			sv_set_i(&a, 0, -2);
			sv_set_i(&a, 1000, -1);
			sv_set_i(&b, 0, -2);
			sv_set_i(&b, as - 1, -1);
			res |= sv_at_i(a, 0) == -2 ? 0 : -1;
			res |= sv_at_i(a, 1000) == -1 ? 0 : -1;
			res |= sv_at_i(b, 0) == -2 ? 0 : -1;
			res |= sv_at_i(b, as - 1) == -1 ? 0 : -1;
		} while (0);
		sv_free(&a);
	}
	return res;
}

static int test_sv_push_pop_set_u()
{
	uint64_t init[] = { (uint64_t)-1, (uint64_t)-1, (uint64_t)-1,
			    (uint64_t)-1 },
		expected[] = { 0xff, 0xffff, 0xffffffff, 0xffffffffffffffffLL };
	enum eSV_Type t[] = { SV_U8, SV_U16, SV_U32, SV_U64 };
	int i = 0, ntests = sizeof(t)/sizeof(t[0]), res = 0;
	for (; i < ntests; i++) {
		size_t as = 10;
		sv_t *a = sv_alloc_t(t[i], as);
		sv_t *b = sv_alloca_t(t[i], as);
		int64_t r;
		if (!(a && b && sv_push_u(&a, (uint64_t)-1) &&
		      sv_push_u(&b, init[i]) &&
		      (r = (int64_t)sv_pop_u(a)) && r == sv_pop_i(b) &&
		      r == (int64_t)expected[i] &&
		      sv_len(a) == 0 && sv_len(b) == 0))
			res |= 1 << i;
		sv_free(&a);
	}
	return res;
}

#define TEST_SV_PUSH_RAW(v, ntest, alloc, type, CMPF, T, TEST, vbuf, ve)\
	sv_t *v = alloc(type, 0 CMPF);					\
	sv_push_raw(&v, vbuf, ve);					\
	const T *v##b = (const T *)sv_get_buffer_r(v);			\
	int v##i, v##r = 0;						\
	for (v##i = 0; v##i < ve && v##r == 0; v##i++)			\
		v##r = TEST(vbuf[v##i], v##b[v##i]) ? 0 : 1;		\
	res |= !v ? 1 << (ntest * 3) :					\
		    v##r <= 0 ? 0 : 2 << (ntest * 3);			\
	sv_free(&v);

static int test_sv_push_raw()
{
	int res = 0;
	TEST_SV_PUSH_RAW(z, 0, sv_alloc, sizeof(struct AA), NO_CMPF, struct AA,
			 TEST_AA, av1, 3);
	TEST_SV_PUSH_RAW(b, 1, sv_alloc_t, SV_I8, X_CMPF, int8_t,
			 TEST_INT, i8v, 3);
	TEST_SV_PUSH_RAW(c, 2, sv_alloc_t, SV_U8, X_CMPF, uint8_t,
			 TEST_INT, u8v, 3);
	TEST_SV_PUSH_RAW(d, 3, sv_alloc_t, SV_I16, X_CMPF, int16_t,
			 TEST_INT, i16v, 3);
	TEST_SV_PUSH_RAW(e, 4, sv_alloc_t, SV_U16, X_CMPF, uint16_t,
			 TEST_INT, u16v, 3);
	TEST_SV_PUSH_RAW(f, 5, sv_alloc_t, SV_I32, X_CMPF, int32_t,
			 TEST_INT, i32v, 3);
	TEST_SV_PUSH_RAW(g, 6, sv_alloc_t, SV_U32, X_CMPF, uint32_t,
			 TEST_INT, u32v, 3);
	TEST_SV_PUSH_RAW(h, 7, sv_alloc_t, SV_I64, X_CMPF, int64_t,
			 TEST_INT, i64v, 3);
	TEST_SV_PUSH_RAW(i, 8, sv_alloc_t, SV_U64, X_CMPF, uint64_t,
			 TEST_INT, u64v, 3);
	return res;
}

struct MyNode1
{
	struct S_Node n;
	int k;
	int v;
};

static int cmp1(const struct MyNode1 *a, const struct MyNode1 *b)
{
	return a->k - b->k;
}

#ifdef S_EXTRA_TREE_TEST_DEBUG
static void ndx2s(char *out, const size_t out_max, const stndx_t id)
{
	if (id == ST_NIL)
		strcpy(out, "nil");
	else
		snprintf(out, out_max, "%u", (unsigned)id);
}

static ss_t *ss_cat_stn_MyNode1(ss_t **s, const stn_t *n, const stndx_t id)
{
	ASSERT_RETURN_IF(!s, NULL);
	struct MyNode1 *node = (struct MyNode1 *)n;
	char l[128], r[128];
	ndx2s(l, sizeof(l), n->x.l);
	ndx2s(r, sizeof(r), n->r);
	ss_cat_printf(s, 128, "[%u: (%i, %c) -> (%s, %s; r:%u)]",
		     (unsigned)id, node->k, node->v, l, r, n->x.is_red );
	return *s;
}
#endif

static int test_st_alloc()
{
	st_t *t = st_alloc((st_cmp_t)cmp1, sizeof(struct MyNode1), 1000);
	struct MyNode1 n = { EMPTY_STN, 0, 0 };
	int res = !t ? 1 : st_len(t) != 0 ? 2 :
			   (!st_insert(&t, (const stn_t *)&n) ||
			    st_len(t) != 1) ? 4 : 0;
	st_free(&t);
	return res;
}

#define ST_ENV_TEST_AUX						          \
	st_t *t = st_alloc((st_cmp_t)cmp1, sizeof(struct MyNode1), 1000); \
	if (!t)							          \
		return 1;					          \
	ss_t *log = NULL;					          \
	struct MyNode1 n0 = { EMPTY_STN, 0, 0 };		          \
	stn_t *n = (stn_t *)&n0;				          \
	sbool_t r = S_FALSE;
#define ST_ENV_TEST_AUX_LEAVE	\
	st_free(&t);		\
	ss_free(&log);		\
	if (!r)			\
		res |= 0x40000000;
/* #define S_EXTRA_TREE_TEST_DEBUG */
#ifdef S_EXTRA_TREE_TEST_DEBUG
	#define STLOGMYT(t) st_log_obj(&log, t, ss_cat_stn_MyNode1)
#else
	#define STLOGMYT(t)
#endif
#define ST_A_INS(key, val) {			\
			n0.k = key;		\
			n0.v = val;		\
			r = st_insert(&t, n);	\
			STLOGMYT(t);		\
		}
#define ST_A_DEL(key) {					\
			n0.k = key;			\
			r = st_delete(t, n, NULL);	\
			STLOGMYT(t);			\
		}
#define ST_CHK_BRK(t, len, err)				\
		if (st_len(t) != (size_t)(len)) {	\
			res = err;			\
			break;				\
		}

static int test_st_insert_del()
{
	ST_ENV_TEST_AUX;
	int res = 0;
	const struct MyNode1 *nr;
	int i = 0;
	const int tree_elems = 90;
	const int cbase = ' ';
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
		if (!nr || nr->v != (cbase + tree_elems - 1) ||
		    st_traverse_levelorder(t, NULL, NULL) != 1) {
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
		if (!nr || nr->v != (cbase + tree_elems - 1) ||
		    st_traverse_levelorder(t, NULL, NULL) != 1) {
			res = 6;
			break;
		}
		ST_A_DEL(tree_elems - 1);
		ST_CHK_BRK(t, 0, tree_elems);
		/* Case 3: add in order, delete in reverse order */
		for (i = 0; i < (int)tree_elems; i++) /* ST_A_INS(2, 'c'); */
			ST_A_INS(i, cbase + i);	      /* double rotation   */
		ST_CHK_BRK(t, tree_elems, 8);
		for (i = (tree_elems - 1); i > 0; i--)
			ST_A_DEL(i);
		ST_CHK_BRK(t, 1, 9);
		n0.k = 0;
		nr = (const struct MyNode1 *)st_locate(t, &n0.n);
		if (!nr || nr->v != cbase ||
		    st_traverse_levelorder(t, NULL, NULL) != 1) {
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
	ss_t **log = (ss_t **)tp->context;
	const struct MyNode1 *node = (const struct MyNode1 *)
						get_node_r(tp->t, tp->c);
	if (node)
		ss_cat_printf(log, 512, "%c", node->v);
	return 0;
}

static int test_st_traverse()
{
	ST_ENV_TEST_AUX;
	int res = 0;
	ssize_t levels = 0;
	for (;;) {
		ST_A_INS(6, 'g'); ST_A_INS(7, 'h'); ST_A_INS(8, 'i');
		ST_A_INS(3, 'd'); ST_A_INS(4, 'e'); ST_A_INS(5, 'f');
		ST_A_INS(0, 'a'); ST_A_INS(1, 'b'); ST_A_INS(2, 'c');
		ss_cpy_c(&log, "");
		levels = st_traverse_preorder(t, test_traverse, (void *)&log);
		const char *out = ss_to_c(log);
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
#define TEST_SM_ALLOC_X(fn, sm_alloc_X, type, insert, at, sm_free_X)	\
	static int fn()							\
	{								\
		size_t n = 1000;					\
		sm_t *m = sm_alloc_X(type, n);				\
		int res = 0;						\
		for (;;) {						\
			if (!m) { res = 1; break; }			\
			if (!sm_empty(m) || sm_capacity(m) != n ||	\
			    sm_capacity_left(m) != n) {			\
				res = 2;				\
				break;					\
			}						\
			insert(&m, 1, 1001);				\
			if (sm_size(m) != 1) { res = 3; break; }	\
			insert(&m, 2, 1002);				\
			insert(&m, 3, 1003);				\
			if (sm_empty(m) ||sm_capacity(m) != n ||	\
			    sm_capacity_left(m) != n - 3) {		\
				res = 4;				\
				break;					\
			}						\
			if (at(m, 1) != 1001) { res = 5; break; }	\
			if (at(m, 2) != 1002) { res = 6; break; }	\
			if (at(m, 3) != 1003) { res = 7; break; }	\
			break;						\
		}							\
		if (!res) {						\
			sm_clear(m);					\
			res = !sm_size(m) ? 0 : 8;			\
		}							\
		sm_free_X(&m);						\
		return res;						\
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
TEST_SM_ALLOC_X(test_sm_alloca_ii, sm_alloca, SM_II, sm_insert_ii,
		sm_at_ii, TEST_SM_ALLOC_DONOTHING)

#define TEST_SM_SHRINK_TO_FIT(m, ntest, atype, r)			\
	sm_t *m = sm_alloc(atype, r), *m##2 = sm_alloca(atype, r);	\
	res |= !m ? 1 << (ntest * 4) : !m##2 ? 2 << (ntest * 4) :	\
		(sm_max_size(m) == r ? 0 : 3 << (ntest * 4)) |		\
		(sm_max_size(m##2) == r ? 0 : 4 << (ntest * 4)) |	\
		(sm_shrink(&m) && sm_max_size(m) == 0 ? 0 :		\
		 5 << (ntest * 4)) |					\
		(sm_shrink(&m##2) && sm_max_size(m##2) == r ? 0 :	\
		 6 << (ntest * 4));					\
	sm_free(&m)

static int test_sm_shrink()
{
	int res = 0;
	TEST_SM_SHRINK_TO_FIT(aa, 0, SM_II32, 10);
	TEST_SM_SHRINK_TO_FIT(bb, 1, SM_UU32, 10);
	TEST_SM_SHRINK_TO_FIT(cc, 2, SM_II, 10);
	TEST_SM_SHRINK_TO_FIT(dd, 3, SM_IS, 10);
	TEST_SM_SHRINK_TO_FIT(ee, 4, SM_IP, 10);
	TEST_SM_SHRINK_TO_FIT(ff, 5, SM_SI, 10);
	TEST_SM_SHRINK_TO_FIT(gg, 6, SM_SS, 10);
	TEST_SM_SHRINK_TO_FIT(hh, 7, SM_SP, 10);
	return res;
}

#define TEST_SM_DUP(m, ntest, atype, insf, kv, vv, atf, cmpf, r)	\
	sm_t *m = sm_alloc(atype, r), *m##2 = sm_alloca(atype, r),	\
	     *m##b = NULL, *m##2b = NULL;				\
	res |= !m || !m##2 ? 1 << (ntest * 2) : 0;			\
	if (!res) {							\
		int j = 0;						\
		for (; j < r; j++) {					\
			sbool_t b1 = insf(&m, kv[j], vv[j]);		\
			sbool_t b2 = insf(&m##2, kv[j], vv[j]);		\
			if (!b1 || !b2) {				\
				res |= 2 << (ntest * 2);		\
				break;					\
			}						\
		}							\
	}								\
	if (!res) {							\
		m##b = sm_dup(m);					\
		m##2b = sm_dup(m##2);					\
		res = (!m##b || !m##2b) ? 4 << (ntest * 2) :		\
			(sm_size(m) != r || sm_size(m##2) != r ||	\
			 sm_size(m##b) != r || sm_size(m##2b) != r) ?	\
			2 << (ntest * 2) : 0;				\
	}								\
	if (!res) {							\
		int j = 0;						\
		for (; j < r; j++)	{				\
			if (cmpf(atf(m, kv[j]), atf(m##2, kv[j])) ||	\
			    cmpf(atf(m, kv[j]), atf(m##b, kv[j])) ||	\
			    cmpf(atf(m##b, kv[j]), atf(m##2b, kv[j]))) {\
				res |= 3 << (ntest * 2);		\
				break;					\
			}						\
		}							\
	}								\
	sm_free(&m, &m##2, &m##b, &m##2b)

/*
 * Covers: sm_dup, sm_insert_ii32, sm_insert_uu32, sm_insert_ii, sm_insert_is,
 * sm_insert_ip, sm_insert_si, sm_insert_ss, sm_insert_sp, sm_at_ii32,
 * sm_at_uu32, sm_at_ii, sm_at_is, sm_at_ip, sm_at_si, sm_at_ss, sm_at_sp
 */
static int test_sm_dup()
{
	int res = 0;
	ss_t *s = ss_dup_c("hola1"), *s2 = ss_alloca(100);
	ss_cpy_c(&s2, "hola2");
	ss_t *ssk[10], *ssv[10];
	int i = 0;
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
	TEST_SM_DUP(cc, 2, SM_II, sm_insert_ii, sik, i64v, sm_at_ii,
		    scmp_int64, 10);
	TEST_SM_DUP(dd, 3, SM_IS, sm_insert_is, sik, ssv, sm_at_is,
		    ss_cmp, 10);
	TEST_SM_DUP(ee, 4, SM_IP, sm_insert_ip, sik, spv, sm_at_ip,
		    scmp_ptr, 10);
	TEST_SM_DUP(ff, 5, SM_SI, sm_insert_si, ssk, i64v, sm_at_si,
		    scmp_int64, 10);
	TEST_SM_DUP(gg, 6, SM_SS, sm_insert_ss, ssk, ssv, sm_at_ss,
		    ss_cmp, 10);
	TEST_SM_DUP(hh, 7, SM_SP, sm_insert_sp, ssk, spv, sm_at_sp,
		    scmp_ptr, 10);
	ss_free(&s);
	return res;
}

static int test_sm_cpy()
{
	int res = 0;
	sm_t *m1 = sm_alloc(SM_II32, 0), *m2 = sm_alloc(SM_SS, 0),
	     *m1a3 = sm_alloca(SM_II32, 3), *m1a10 = sm_alloca(SM_II32, 10);
	sm_insert_ii32(&m1, 1, 1);
	sm_insert_ii32(&m1, 2, 2);
	sm_insert_ii32(&m1, 3, 3);
	sm_insert_ss(&m2, ss_crefa("k1"), ss_crefa("v1"));
	sm_insert_ss(&m2, ss_crefa("k2"), ss_crefa("v2"));
	sm_insert_ss(&m2, ss_crefa("k3"), ss_crefa("v3"));
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
	sm_free(&m1, &m2, &m1a3, &m1a10);
	return res;
}

#define TEST_SM_X_COUNT(T, insf, cntf, v)	\
	int res = 0;				\
	uint32_t i, tcount = 100;		\
	sm_t *m = sm_alloc(T, tcount);		\
	for (i = 0; i < tcount; i++)		\
		insf(&m, (unsigned)i, v);	\
	for (i = 0; i < tcount; i++)		\
		if (!cntf(m, i)) {		\
			res = 1 + (int)i;	\
			break;			\
		}				\
	sm_free(&m);				\
	return res;

static int test_sm_count_u()
{
	TEST_SM_X_COUNT(SM_UU32, sm_insert_uu32, sm_count_u, 1);
}

static int test_sm_count_i()
{
	TEST_SM_X_COUNT(SM_II32, sm_insert_ii32, sm_count_i, 1);
}

static int test_sm_count_s()
{
	ss_t *s = ss_dup_c("a_1"), *t = ss_dup_c("a_2"), *u = ss_dup_c("a_3");
	sm_t *m = sm_alloc(SM_SI, 3);
	sm_insert_si(&m, s, 1);
	sm_insert_si(&m, t, 2);
	sm_insert_si(&m, u, 3);
	int res = sm_count_s(m, s) && sm_count_s(m, t) &&
		  sm_count_s(m, u) ? 0 : 1;
	ss_free(&s, &t, &u);
	sm_free(&m);
	return res;
}

static int test_sm_inc_ii32()
{
	sm_t *m = sm_alloc(SM_II32, 0);
	sm_inc_ii32(&m, 123, -10);
	sm_inc_ii32(&m, 123, -20);
	sm_inc_ii32(&m, 123, -30);
	int res = !m ? 1 : (sm_at_ii32(m, 123) == -60 ? 0 : 2);
	sm_free(&m);
	return res;
}

static int test_sm_inc_uu32()
{
	sm_t *m = sm_alloc(SM_UU32, 0);
	sm_inc_uu32(&m, 123, 10);
	sm_inc_uu32(&m, 123, 20);
	sm_inc_uu32(&m, 123, 30);
	int res = !m ? 1 : (sm_at_uu32(m, 123) == 60 ? 0 : 2);
	sm_free(&m);
	return res;
}

static int test_sm_inc_ii()
{
	sm_t *m = sm_alloc(SM_II, 0);
	sm_inc_ii(&m, 123, -7);
	sm_inc_ii(&m, 123, S_MAX_I64);
	sm_inc_ii(&m, 123, 3);
	int res = !m ? 1 : (sm_at_ii(m, 123) == S_MAX_I64 - 4 ? 0 : 2);
	sm_free(&m);
	return res;
}

static int test_sm_inc_si()
{
	sm_t *m = sm_alloc(SM_SI, 0);
	const ss_t *k = ss_crefa("hello");
	sm_inc_si(&m, k, -7);
	sm_inc_si(&m, k, S_MAX_I64);
	sm_inc_si(&m, k, 3);
	int res = !m ? 1 : (sm_at_si(m, k) == S_MAX_I64 - 4 ? 0 : 2);
	sm_free(&m);
	return res;
}

static int test_sm_delete_i()
{
	sm_t *m_ii32 = sm_alloc(SM_II32, 0), *m_uu32 = sm_alloc(SM_UU32, 0),
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
	int res = m_ii32 && m_uu32 && m_ii ? 0 : 1;
	res |= (sm_at_ii32(m_ii32, -2) == -2 ? 0 : 2);
	res |= (sm_at_uu32(m_uu32, 2) == 2 ? 0 : 4);
	res |= (sm_at_ii(m_ii, S_MAX_I64 - 1) == S_MAX_I64 - 1 ? 0 : 8);
	/*
	 * Delete elements, checking that second deletion of same
	 * element gives error
	 */
	res |= sm_delete_i(m_ii32, -2) ? 0 : 16;
	res |= !sm_delete_i(m_ii32, -2) ? 0 : 32;
	res |= sm_delete_i(m_uu32, 2) ? 0 : 64;
	res |= !sm_delete_i(m_uu32, 2) ? 0 : 128;
	res |= sm_delete_i(m_ii, S_MAX_I64 - 1) ? 0 : 256;
	res |= !sm_delete_i(m_ii, S_MAX_I64 - 1) ? 0 : 512;
	/*
	 * Check that querying for deleted elements gives default value
	 */
	res |= (sm_at_ii32(m_ii32, -2) == 0 ? 0 : 1024);
	res |= (sm_at_uu32(m_uu32, 2) == 0 ? 0 : 2048);
	res |= (sm_at_ii(m_ii, S_MAX_I64 - 1) == 0 ? 0 : 4096);
	sm_free(&m_ii32, &m_uu32, &m_ii);
	return res;
}

static int test_sm_delete_s()
{
	sm_t *m_si = sm_alloc(SM_SI, 0), *m_sp = sm_alloc(SM_SP, 0),
	     *m_ss = sm_alloc(SM_SS, 0);
	/*
	 * Insert elements
	 */
	const ss_t *k1 = ss_crefa("key1"), *k2 = ss_crefa("key2"),
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
	int res = m_si && m_sp && m_ss ? 0 : 1;
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
	sm_free(&m_si, &m_sp, &m_ss);
	return res;
}

static sbool_t cback_i32i32(int32_t k, int32_t v, void *context)
{
	(void)k;
	(void)v;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

static sbool_t cback_ss(const ss_t *k, const ss_t *v, void *context)
{
	(void)k;
	(void)v;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

static sbool_t cback_i32(int32_t k, void *context)
{
	(void)k;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

static sbool_t cback_u32(uint32_t k, void *context)
{
	(void)k;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

static sbool_t cback_i(int64_t k, void *context)
{
	(void)k;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

static sbool_t cback_s(const ss_t *k, void *context)
{
	(void)k;
	if (context)
		(*((size_t *)context))++;
	return S_TRUE;
}

#define TEST_SM_IT_X(n, id, et, itk, itv, cmpkf, cmpvf, k1, v1, k2, v2, k3, v3,\
		     res)						       \
	sm_t *m_##id = sm_alloc(et, 0), *m_a##id = sm_alloca(et, 3);	       \
	sm_insert_##id(&m_##id, k1, v1); sm_insert_##id(&m_a##id, k1, v1);     \
	res |= (!cmpkf(itk(m_##id, 0), k1) &&				       \
		!cmpvf(itv(m_##id, 0), v1)) ? 0 : n;			       \
	sm_insert_##id(&m_##id, k2, v2); sm_insert_##id(&m_##id, k3, v3);      \
	sm_insert_##id(&m_a##id, k2, v2); sm_insert_##id(&m_a##id, k3, v3);    \
	res |= (!cmpvf(sm_at_##id(m_##id, k1), v1) &&			       \
		!cmpvf(sm_at_##id(m_##id, k2), v2) &&			       \
		!cmpvf(sm_at_##id(m_##id, k3), v3) &&			       \
		!cmpvf(sm_at_##id(m_a##id, k1), v1) &&			       \
		!cmpvf(sm_at_##id(m_a##id, k2), v2) &&			       \
		!cmpvf(sm_at_##id(m_a##id, k3), v3) &&			       \
		!cmpkf(itk(m_##id, 0), itk(m_a##id, 0)) &&		       \
		!cmpkf(itk(m_##id, 1), itk(m_a##id, 1)) &&		       \
		!cmpkf(itk(m_##id, 2), itk(m_a##id, 2)) &&		       \
		!cmpvf(itv(m_##id, 0), itv(m_a##id, 0)) &&		       \
		!cmpvf(itv(m_##id, 1), itv(m_a##id, 1)) &&		       \
		!cmpvf(itv(m_##id, 2), itv(m_a##id, 2)) &&		       \
		cmpkf(itk(m_##id, 0), 0) && cmpkf(itk(m_a##id, 0), 0) &&       \
		cmpkf(itk(m_##id, 1), 0) && cmpkf(itk(m_a##id, 1), 0) &&       \
		cmpkf(itk(m_##id, 2), 0) && cmpkf(itk(m_a##id, 2), 0) &&       \
		cmpvf(itv(m_##id, 0), 0) && cmpvf(itv(m_a##id, 0), 0) &&       \
		cmpvf(itv(m_##id, 1), 0) && cmpvf(itv(m_a##id, 1), 0) &&       \
		cmpvf(itv(m_##id, 2), 0) && cmpvf(itv(m_a##id, 2), 0)) ? 0 : n;\
	sm_free(&m_##id); sm_free(&m_a##id);

static int test_sm_it()
{
	int res = 0;
	TEST_SM_IT_X(1, ii32, SM_II32, sm_it_i32_k, sm_it_ii32_v, cmp_ii, cmp_ii,
		     -1, -1, -2, -2, -3, -3, res);
	TEST_SM_IT_X(2, uu32, SM_UU32, sm_it_u32_k, sm_it_uu32_v, cmp_ii, cmp_ii,
		     1, 1, 2, 2, 3, 3, res);
	TEST_SM_IT_X(4, ii, SM_II, sm_it_i_k, sm_it_ii_v, cmp_ii, cmp_ii,
		     S_MAX_I64, S_MIN_I64, S_MIN_I64, S_MAX_I64, S_MAX_I64 - 1,
		     S_MIN_I64 + 1, res);
	TEST_SM_IT_X(8, is, SM_IS, sm_it_i_k, sm_it_is_v, cmp_ii, ss_cmp, 1,
		     ss_crefa("v1"), 2, ss_crefa("v2"), 3, ss_crefa("v3"), res);
	TEST_SM_IT_X(16, ip, SM_IP, sm_it_i_k, sm_it_ip_v, cmp_ii, cmp_pp,
		     1, (void *)1, 2, (void *)2, 3, (void *)3, res);
	TEST_SM_IT_X(32, si, SM_SI, sm_it_s_k, sm_it_si_v, ss_cmp, cmp_ii,
		     ss_crefa("v1"), 1, ss_crefa("v2"), 2, ss_crefa("v3"), 3,
		     res);
	TEST_SM_IT_X(64, ss, SM_SS, sm_it_s_k, sm_it_ss_v, ss_cmp, ss_cmp,
		     ss_crefa("k1"), ss_crefa("v1"), ss_crefa("k2"),
		     ss_crefa("v2"), ss_crefa("k3"), ss_crefa("v3"), res);
	TEST_SM_IT_X(128, sp, SM_SP, sm_it_s_k, sm_it_sp_v, ss_cmp, cmp_pp,
		     ss_crefa("k1"), (void *)1, ss_crefa("k2"), (void *)2,
		     ss_crefa("k3"), (void *)3, res);
	return res;
}

static int test_sm_itr()
{
	int res = 1;
	size_t nelems = 100;
	sm_t *m_ii32 = sm_alloc(SM_II32, nelems),
	     *m_uu32 = sm_alloc(SM_UU32, nelems),
	     *m_ii = sm_alloc(SM_II, nelems),
	     *m_is = sm_alloc(SM_IS, nelems),
	     *m_ip = sm_alloc(SM_IP, nelems),
	     *m_si = sm_alloc(SM_SI, nelems),
	     *m_ss = sm_alloc(SM_SS, nelems),
	     *m_sp = sm_alloc(SM_SP, nelems);
	if (m_ii32 && m_uu32 && m_ii && m_is && m_ip && m_si && m_ss && m_sp) {
		ss_t *ktmp = ss_alloca(1000), *vtmp = ss_alloca(1000);
		int i;
		for (i = 0; i < (int)nelems; i++) {
			sm_insert_ii32(&m_ii32, -i, -i);
			sm_insert_uu32(&m_uu32, (uint32_t)i, (uint32_t)i);
			sm_insert_ii(&m_ii, -i, -i);
			ss_printf(&ktmp, 200, "k%04i", i);
			ss_printf(&vtmp, 200, "v%04i", i);
			sm_insert_is(&m_is, -i, vtmp);
			sm_insert_ip(&m_ip, -i, (char *)0 + i);
			sm_insert_si(&m_si, ktmp, i);
			sm_insert_ss(&m_ss, ktmp, vtmp);
			sm_insert_sp(&m_sp, ktmp, (char *)0 + i);
		}
		size_t cnt1 = 0, cnt2 = 0;
		int32_t lower_i32 = -20, upper_i32 = -10;
		uint32_t lower_u32 = 10, upper_u32 = 20;
		int64_t lower_i = -20, upper_i = -10;
		ss_t *lower_s = ss_alloca(100);
		ss_t *upper_s = ss_alloca(100);
		ss_cpy_c(&lower_s, "k001"); /* covering from "k0010" to "k0019" */
		ss_cpy_c(&upper_s, "k002");
		size_t processed_ii321 = sm_itr_ii32(m_ii32, lower_i32, upper_i32,
							cback_i32i32, &cnt1),
		       processed_ii322 = sm_itr_ii32(m_ii32, lower_i32, upper_i32,
							NULL, NULL),
		       processed_uu32 = sm_itr_uu32(m_uu32, lower_u32, upper_u32,
						NULL, NULL),
		       processed_ii = sm_itr_ii(m_ii, lower_i, upper_i,
						NULL, NULL),
		       processed_is = sm_itr_is(m_is, lower_i, upper_i,
						NULL, NULL),
		       processed_ip = sm_itr_ip(m_ip, lower_i, upper_i,
						NULL, NULL),
		       processed_si = sm_itr_si(m_si, lower_s, upper_s,
						NULL, NULL),
		       processed_ss1 = sm_itr_ss(m_ss, lower_s, upper_s,
						cback_ss, &cnt2),
		       processed_ss2 = sm_itr_ss(m_ss, lower_s, upper_s,
						NULL, NULL),
		       processed_sp = sm_itr_sp(m_sp, lower_s, upper_s,
							NULL, NULL);
		res = processed_ii321 == 11 ? 0 : 2;
		res |= processed_ii321 == processed_ii322 ? 0 : 4;
		res |= processed_ii322 == cnt1 ? 0 : 8;
		res |= processed_uu32 == cnt1 ? 0 : 0x10;
		res |= processed_ii == cnt1 ? 0 : 0x20;
		res |= processed_is == cnt1 ? 0 : 0x40;
		res |= processed_ip == cnt1 ? 0 : 0x80;
		res |= processed_si == 10 ? 0 : 0x100;
		res |= processed_ss1 == 10 ? 0 : 0x200;
		res |= processed_ss1 == processed_ss2 ? 0 : 0x400;
		res |= processed_ss2 == cnt2 ? 0 : 0x800;
		res |= processed_sp == 10 ? 0 : 0x1000;
		/*
		 * Wrong type checks
		 */
		res |= sm_itr_uu32(m_ii32, 10, 20, NULL, NULL) > 0 ? 0x2000 : 0;
		res |= sm_itr_ii(m_ii32, 10, 20, NULL, NULL) > 0 ? 0x4000 : 0;
		res |= sm_itr_is(m_ii32, 10, 20, NULL, NULL) > 0 ? 0x8000 : 0;
		res |= sm_itr_ip(m_ii32, 10, 20, NULL, NULL) > 0 ? 0x10000 : 0;
		res |= sm_itr_si(m_ii32, ss_crefa("10"), ss_crefa("20"),
				 NULL, NULL) > 0 ? 0x20000 : 0;
		res |= sm_itr_sp(m_ii32, ss_crefa("10"), ss_crefa("20"),
				 NULL, NULL) > 0 ? 0x40000 : 0;
	}

	/*
	 * TODO: add sm_itr_i32, sm_itr_u32, sm_itr_i, sm_itr_s
	 */

	sm_free(&m_ii32, &m_uu32, &m_ii, &m_is, &m_ip, &m_si, &m_ss, &m_sp);
        return res;
}

static int test_sm_sort_to_vectors()
{
	const size_t test_elems = 100;
	sm_t *m = sm_alloc(SM_II32, test_elems);
	sv_t *kv = NULL, *vv = NULL, *kv2 = NULL, *vv2 = NULL;
	int res = m ? 0 : 1;
	ssize_t i, j;
	do {
		/*
		 * Add elements
		 */
		for (i = test_elems; i > 0 && !res; i--) {
			if (!sm_insert_ii32(&m, (int)i, (int)-i) ||
			    !st_assert((st_t *)m)) {
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
				int k = (int)sv_at_i(kv2, (size_t)i);
				int v = (int)sv_at_i(vv2, (size_t)i);
				if (k != (i + 1) || v != -(i + 1)) {
					res |= 4;
					break;
				}
			}
			if (!st_assert((st_t *)m))
				res |= 8;
			if (res)
				break;
			sv_set_size(kv2, 0);
			sv_set_size(vv2, 0);
			if (!sm_delete_i(m, j) || !st_assert((st_t *)m)) {
				res |= 16;
				break;
			}
		}
	} while (0);
	sm_free(&m);
	sv_free(&kv, &vv, &kv2, &vv2);
	return res;
}

static int test_sm_double_rotation()
{
	const size_t test_elems = 15;
	sm_t *m = sm_alloc(SM_II32, test_elems);
	sv_t *kv = NULL, *vv = NULL, *kv2 = NULL, *vv2 = NULL;
	int res = m ? 0 : 1;
	ssize_t i;
	for (;;) {
		/*
		 * Add elements
		 */
		for (i = test_elems; i > 0; i--) {
			if (!sm_insert_ii32(&m, (int)i, (int)-i) ||
			    !st_assert((st_t *)m)) {
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
			if (!sm_delete_i(m, i) || !st_assert((st_t *)m)) {
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
			if (!sm_insert_ii32(&m, (int)i, (int)-i) ||
			    !st_assert((st_t *)m)) {
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
			if (!sm_delete_i(m, i)) {
				res = 5;
				break;
			}
			if (!st_assert((st_t *)m)) {
				res = 50;
				break;
			}
		}
		if (res)
			break;
		if (!st_assert((st_t *)m)) {
			res = 100;
			break;
		}
		break;
	}
	sm_free(&m);
	sv_free(&kv, &vv, &kv2, &vv2);
	return res;
}

static int test_sms()
{
	int i, res = 0;
	const ss_t *k[] = { ss_crefa("k000"), ss_crefa("k001"), ss_crefa("k002") };
	/*
	 * Allocation: heap, stack with 3 elements, and stack with 10 elements
	 */
	sms_t *s_i32 = sms_alloc(SMS_I32, 0),
	      *s_u32 = sms_alloc(SMS_U32, 0),
	      *s_i = sms_alloc(SMS_I, 0),
	      *s_s = sms_alloc(SMS_S, 0),
	      *s_s2 = NULL,
	      *s_a3 = sms_alloca(SMS_I32, 3),
	      *s_a10 = sms_alloca(SMS_I32, 10);
	res |= (sms_empty(s_i32) && sms_empty(s_u32) && sms_empty(s_i) &&
		sms_empty(s_s) && sms_empty(s_s2) && sms_empty(s_a3) &&
		sms_empty(s_a10) ? 0 : 1<<0);
	/*
	 * Insert elements
	 */
	for (i = 0; i < 3; i++) {
		sms_insert_i32(&s_i32, i + 10);
		sms_insert_u32(&s_u32, (uint32_t)i + 20);
		sms_insert_i(&s_i, i);
		sms_insert_s(&s_s, k[i]);
	}
	res |= (!sms_empty(s_i32) && !sms_empty(s_u32) && !sms_empty(s_i) &&
		!sms_empty(s_s) && sms_empty(s_s2) && sms_empty(s_a3) &&
		sms_empty(s_a10) ? 0 : 1<<1);
	res |= (sms_count_i(s_i32, 10) && sms_count_i(s_i32, 11) &&
		sms_count_i(s_i32, 12) && sms_count_u(s_u32, 20) &&
		sms_count_u(s_u32, 21) && sms_count_u(s_u32, 22) &&
		sms_count_i(s_i, 0) && sms_count_i(s_i, 1) &&
		sms_count_i(s_i, 2) && sms_count_s(s_s, k[0]) &&
		sms_count_s(s_s, k[1]) && sms_count_s(s_s, k[2]) ? 0 : 1<<2);
	/*
	 * Enumeration
	 */
	size_t cnt_i32 = 0, cnt_u32 = 0, cnt_i = 0, cnt_s = 0, cnt_s2 = 0;
	s_s2 = sms_dup(s_s);
	size_t processed_i32 = sms_itr_i32(s_i32, -1, 100, cback_i32, &cnt_i32),
	       processed_u32 = sms_itr_u32(s_u32, 0, 100, cback_u32, &cnt_u32),
	       processed_i = sms_itr_i(s_i, -1, 100, cback_i, &cnt_i),
	       processed_s = sms_itr_s(s_s, k[0], k[2], cback_s, &cnt_s),
	       processed_s2 = sms_itr_s(s_s2, k[0], k[2], cback_s, &cnt_s2);
	res |= (processed_i32 == cnt_i32 && cnt_i32 == 3 ? 0 : 1<<3);
	res |= (processed_u32 == cnt_u32 && cnt_u32 == 3 ? 0 : 1<<4);
	res |= (processed_i == cnt_i && cnt_i == 3 ? 0 : 1<<5);
	res |= (processed_s == cnt_s && cnt_s == 3 ? 0 : 1<<6);
	res |= (processed_s2 == cnt_s2 && cnt_s2 == 3 ? 0 : 1<<7);
	/*
	 * sms_cpy() to stack with small stack allocation size
	 */
	sms_cpy(&s_a3, s_i32);
	res |= (sms_size(s_a3) == 3 ? 0 : 1<<8);
	sms_cpy(&s_a3, s_u32);
	res |= (sms_size(s_a3) == 3 ? 0 : 1<<9);
	sms_cpy(&s_a3, s_i);
	res |= (sms_size(s_a3) == 0 ? 0 : 1<<10);
	sms_cpy(&s_a3, s_s);
	res |= (sms_size(s_a3) == 0 ? 0 : 1<<11);
	/*
	 * sms_cpy() to stack with enough stack allocation size
	 */
	sms_cpy(&s_a10, s_i32);
	res |= (sms_size(s_a10) == 3 ? 0 : 1<<12);
	sms_cpy(&s_a10, s_u32);
	res |= (sms_size(s_a10) == 3 ? 0 : 1<<13);
	sms_cpy(&s_a10, s_i);
	res |= (sms_size(s_a10) == 3 ? 0 : 1<<14);
	sms_cpy(&s_a10, s_s);
	res |= (sms_size(s_a10) == 3 ? 0 : 1<<15);
	sms_clear(s_a10);
	res |= (sms_size(s_a10) == 0 ? 0 : 1<<16);
	/*
	 * Reserve/grow
	 */
	sms_reserve(&s_i32, 1000);
	sms_grow(&s_u32, 1000 - sms_max_size(s_u32));
	res |= (sms_capacity(s_i32) >= 1000 &&
		sms_capacity(s_u32) >= 1000 &&
		sms_capacity_left(s_i32) >= 997 &&
		sms_capacity_left(s_u32) >= 997 ? 0 : 1<<17);
	/*
	 * Shrink memory
	 */
	sms_shrink(&s_i32);
	sms_shrink(&s_u32);
	res |= (sms_capacity(s_i32) == 3 &&
		sms_capacity(s_u32) == 3 &&
		sms_capacity_left(s_i32) == 0 &&
		sms_capacity_left(s_u32) == 0 ? 0 : 1<<18);
	/*
	 * Random access
	 */
	res |= (sms_it_i32(s_i32, 0) == 10 &&
		sms_it_i32(s_i32, 1) == 11 &&
		sms_it_i32(s_i32, 2) == 12 ? 0 : 1<<19);
	res |= (sms_it_u32(s_u32, 0) == 20 &&
		sms_it_u32(s_u32, 1) == 21 &&
		sms_it_u32(s_u32, 2) == 22 ? 0 : 1<<20);
	res |= (sms_it_i(s_i, 0) == 0 &&
		sms_it_i(s_i, 1) == 1 &&
		sms_it_i(s_i, 2) == 2 ? 0 : 1<<21);
	res |= (!ss_cmp(sms_it_s(s_s, 0), k[0]) &&
		!ss_cmp(sms_it_s(s_s, 1), k[1]) &&
		!ss_cmp(sms_it_s(s_s, 2), k[2]) ? 0 : 1<<22);
	/*
	 * Delete
	 */
	sms_delete_i(s_i32, 10);
	sms_delete_i(s_u32, 20);
	sms_delete_i(s_i, 0);
	sms_delete_s(s_s, k[1]);
	res |= (!sms_count_i(s_i32, 10) && !sms_count_i(s_u32, 20) &&
		!sms_count_i(s_i, 0) && !sms_count_s(s_s, k[1]) ? 0 : 1<<23);
	/*
	 * Release memory
	 */
	sms_free(&s_i32, &s_u32, &s_i, &s_s, &s_s2, &s_a3, &s_a10);
	return res;
}

static int test_endianess()
{
	int res = 0;
	union s_u32 a;
	a.b[0] = 0;
	a.b[1] = 1;
	a.b[2] = 2;
	a.b[3] = 3;
	unsigned ua = a.a32;
#if S_IS_LITTLE_ENDIAN
	unsigned ub = 0x03020100;
#else
	unsigned ub = 0x00010203;
#endif
	if (ua != ub)
		res |= 1;
	if (S_HTON_U32(ub) != 0x00010203)
		res |= 2;
	return res;
}

static int test_alignment()
{
	int res = 0;
	char ac[sizeof(uint32_t)] = { 0 }, bc[sizeof(uint32_t)] = { 0 },
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

static int test_sbitio()
{
	unsigned char tmp[512];
	memset(tmp, -1, sizeof(tmp));
	/*
	 * Write to serial buffer
	 */
	sbio_t bio;
	sbio_write_init(&bio, tmp);
	/* Write 0xa as 1 bit x 4 */
	sbio_write(&bio, 0, 1);
	sbio_write(&bio, 1, 1);
	sbio_write(&bio, 0, 1);
	sbio_write(&bio, 1, 1);
	/* Write 5 as 4 bits at once */
	sbio_write(&bio, 5, 4);
	/* Write 0x1234 as 8 bits x 2 */
	sbio_write(&bio, 0x12, 8);
	sbio_write(&bio, 0x34, 8);
	/* Write 0x5678 as 16 bits at once */
	sbio_write(&bio, 0x5678, 16);
	sbio_write_close(&bio);
	/*
	 * Read from serial buffer
	 */
	sbio_read_init(&bio, tmp);
	size_t r32 = sbio_read(&bio, 32);
	size_t r8 = sbio_read(&bio, 8);
	int res = r32 != 0x7834125A ? 1 : r8 != 0x56 ? 2 :
		  sbio_off(&bio) != 5 ? 4 : 0;
	return res;
}

static int test_lsb_msb()
{
	uint8_t tv8[9] =
		{ 0x00, 0x01, 0x0f, 0x01, 0x0f, 0x80, 0xf0, 0x80, 0xf0 },
		tv8l[9] =
		{ 0x00, 0x01, 0x01, 0x01, 0x01, 0x80, 0x10, 0x80, 0x10 },
		tv8m[9] =
		{ 0x00, 0x01, 0x08, 0x01, 0x08, 0x80, 0x80, 0x80, 0x80 };
	uint16_t tv16[9] =
		{ 0x0000, 0x0001, 0x000f, 0x0ff1, 0x0fff, 0x8000, 0xf000,
		  0x8ff0, 0xfff0 },
		 tv16l[9] =
		{ 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x8000, 0x1000,
		  0x0010, 0x0010 },
		 tv16m[9] =
		{ 0x0000, 0x0001, 0x0008, 0x0800, 0x0800, 0x8000, 0x8000,
		  0x8000, 0x8000 };
	uint32_t tv32[9] =
		{ 0x00000000, 0x00000001, 0x0000000f, 0x0ffffff1, 0x0fffffff,
		  0x80000000, 0xf0000000, 0x8ffffff0, 0xfffffff0 },
		 tv32l[9] =
		{ 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
		  0x80000000, 0x10000000, 0x00000010, 0x00000010 },
		 tv32m[9] =
		{ 0x00000000, 0x00000001, 0x00000008, 0x08000000, 0x08000000,
		  0x80000000, 0x80000000, 0x80000000, 0x80000000 };
	uint64_t tv64[9] =
		{ 0x0000000000000000LL, 0x0000000000000001LL, 0x000000000000000fLL,
		  0x0ffffffffffffff1LL, 0x0fffffffffffffffLL, 0x8000000000000000LL,
		  0xf000000000000000LL, 0x8ffffffffffffff0LL, 0xfffffffffffffff0LL },
		 tv64l[9] =
		{ 0x0000000000000000LL, 0x0000000000000001LL, 0x0000000000000001LL,
		  0x0000000000000001LL, 0x0000000000000001LL, 0x8000000000000000LL,
		  0x1000000000000000LL, 0x0000000000000010LL, 0x0000000000000010LL },
		 tv64m[9] =
		{ 0x0000000000000000LL, 0x0000000000000001LL, 0x0000000000000008LL,
		  0x0800000000000000LL, 0x0800000000000000LL, 0x8000000000000000LL,
		  0x8000000000000000LL, 0x8000000000000000LL, 0x8000000000000000LL };
	int res = 0;
	size_t i;
	#define LSBMSB_TEST(tvx, tvxl, tvxm, lsb, msb, err)		\
		for (i = 0; i < sizeof(tvx)/sizeof(tvx[0]); i++)	\
		if (lsb(tvx[i]) != tvxl[i] || msb(tvx[i]) != tvxm[i]) {	\
			res |= err;					\
			break;						\
		}
	LSBMSB_TEST(tv8, tv8l, tv8m, s_lsb8, s_msb8, 1<<0)
	LSBMSB_TEST(tv16, tv16l, tv16m, s_lsb16, s_msb16, 1<<1)
	LSBMSB_TEST(tv32, tv32l, tv32m, s_lsb32, s_msb32, 1<<2)
	LSBMSB_TEST(tv64, tv64l, tv64m, s_lsb64, s_msb64, 1<<3)
	#undef LSBMSB_TEST
	return res;
}

/*
 * Test execution
 */

int main()
{
#ifndef S_MINIMAL
	/* setlocale: required for this test, not for using ss_* calls */
#ifdef S_POSIX_LOCALE_SUPPORT
	setlocale(LC_ALL, "en_US.UTF-8");
#else
	setlocale(LC_ALL, "");
#endif
	sbool_t unicode_support = S_TRUE;
	wint_t check[] = { 0xc0, 0x23a, 0x10a0, 0x1e9e };
	size_t chkl = 0;
	for (; chkl < sizeof(check)/sizeof(check[0]); chkl++)
		if (towlower(check[chkl]) == check[chkl]) {
			unicode_support = S_FALSE;
			break;
		}
	if (!unicode_support) {
		fprintf(stderr, "warning: OS without full built-in Unicode "
			"support (not required by libsrt, but used for "
			"some tests -those will be skipped-)\n");
	}
#endif
	STEST_START;
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
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 "a" U8_C_N_TILDE_D1, 1,
				     2, U8_C_N_TILDE_D1));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 "a" U8_C_N_TILDE_D1, 2,
				     1, U8_C_N_TILDE_D1 "a"));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 "a" U8_C_N_TILDE_D1, 2,
				     1000, U8_C_N_TILDE_D1 "a"));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 U8_C_N_TILDE_D1 "a", 1,
				     2, U8_C_N_TILDE_D1));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 U8_C_N_TILDE_D1 "a", 2,
				     1, U8_C_N_TILDE_D1 U8_C_N_TILDE_D1));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 U8_C_N_TILDE_D1 "a", 2,
				     1000, U8_C_N_TILDE_D1 U8_C_N_TILDE_D1));
	STEST_ASSERT(test_ss_erase_u(U8_C_N_TILDE_D1 U8_C_N_TILDE_D1 "a", 0,
				     1, U8_C_N_TILDE_D1 "a"));
	STEST_ASSERT(test_ss_free(0));
	STEST_ASSERT(test_ss_free(16));
	STEST_ASSERT(test_ss_len("hello", 5));
	STEST_ASSERT(test_ss_len(U8_HAN_24B62, 4));
	STEST_ASSERT(test_ss_len(U8_C_N_TILDE_D1 U8_S_N_TILDE_F1
		U8_S_I_DOTLESS_131 U8_C_I_DOTTED_130 U8_C_G_BREVE_11E
		U8_S_G_BREVE_11F U8_C_S_CEDILLA_15E U8_S_S_CEDILLA_15F
		U8_CENT_00A2 U8_EURO_20AC U8_HAN_24B62, 25)); /* bytes */
	STEST_ASSERT(test_ss_len_u("hello" U8_C_N_TILDE_D1, 6));
	STEST_ASSERT(test_ss_len_u(U8_HAN_24B62, 1));
	STEST_ASSERT(test_ss_len_u(U8_C_N_TILDE_D1 U8_S_N_TILDE_F1
		U8_S_I_DOTLESS_131 U8_C_I_DOTTED_130 U8_C_G_BREVE_11E
		U8_S_G_BREVE_11F U8_C_S_CEDILLA_15E U8_S_S_CEDILLA_15F
		U8_CENT_00A2 U8_EURO_20AC U8_HAN_24B62, 11)); /* Unicode chrs */
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
	STEST_ASSERT(test_ss_dup_int(9223372036854775807LL,
				     "9223372036854775807"));
	STEST_ASSERT(test_ss_dup_int(-9223372036854775807LL,
				     "-9223372036854775807"));
	ss_t *stmp = ss_alloca(128);
#define MK_TEST_SS_DUP_CPY_CAT(encc, decc, a, b) {		\
	STEST_ASSERT(test_ss_dup_##encc(a, b));			\
	STEST_ASSERT(test_ss_cpy_##encc(a, b)); 		\
	ss_cpy_c(&stmp, "abc");							\
	STEST_ASSERT(test_ss_cat_##encc(ss_crefa("abc"), a, ss_cat(&stmp, b)));	\
	ss_cpy_c(&stmp, "ABC");							\
	STEST_ASSERT(test_ss_cat_##encc(ss_crefa("ABC"), a, ss_cat(&stmp, b)));	\
	STEST_ASSERT(test_ss_dup_##decc(b, a));			\
	STEST_ASSERT(test_ss_cpy_##decc(b, a)); 		\
	ss_cpy_c(&stmp, "abc");							\
	STEST_ASSERT(test_ss_cat_##decc(ss_crefa("abc"), b, ss_cat(&stmp, a)));	\
	ss_cpy_c(&stmp, "ABC");							\
	STEST_ASSERT(test_ss_cat_##decc(ss_crefa("ABC"), b, ss_cat(&stmp, a))); }

	MK_TEST_SS_DUP_CPY_CAT(tolower, toupper, ss_crefa("HELLO"),
			       ss_crefa("hello"));
	MK_TEST_SS_DUP_CPY_CAT(enc_b64, dec_b64, ss_crefa("0123456789ABCDEF"),
			       ss_crefa("MDEyMzQ1Njc4OUFCQ0RFRg=="));
	MK_TEST_SS_DUP_CPY_CAT(enc_b64, dec_b64, ss_crefa("01"),
			       ss_crefa("MDE="));
	MK_TEST_SS_DUP_CPY_CAT(enc_hex, dec_hex, ss_crefa("\xf8"),
			       ss_crefa("f8"));
	MK_TEST_SS_DUP_CPY_CAT(enc_hex, dec_hex, ss_crefa("\xff\xff"),
			       ss_crefa("ffff"));
	MK_TEST_SS_DUP_CPY_CAT(enc_hex, dec_hex, ss_crefa("01z"),
			       ss_crefa("30317a"));
	MK_TEST_SS_DUP_CPY_CAT(enc_HEX, dec_hex, ss_crefa("0123456789ABCDEF"),
			       ss_crefa("30313233343536373839414243444546"));
	MK_TEST_SS_DUP_CPY_CAT(enc_HEX, dec_hex, ss_crefa("\xf8"),
			       ss_crefa("F8"));
	MK_TEST_SS_DUP_CPY_CAT(enc_HEX, dec_hex, ss_crefa("\xff\xff"),
			       ss_crefa("FFFF"));
	MK_TEST_SS_DUP_CPY_CAT(enc_HEX, dec_hex, ss_crefa("01z"),
			       ss_crefa("30317A"));
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_xml, dec_esc_xml,
			       ss_crefa("hi\"&'<>there"),
			       ss_crefa("hi&quot;&amp;&apos;&lt;&gt;there"));
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_json, dec_esc_json,
			       ss_crefa("\b\t\f\n\r\"\x5c"),
			       ss_crefa("\\b\\t\\f\\n\\r\\\"\x5c\x5c"));
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_url, dec_esc_url,
			       ss_crefa("0189ABCXYZ-_.~abcxyz \\/&!?<>"),
			       ss_crefa("0189ABCXYZ-_.~abcxyz%20%5C%2F%26"
					"%21%3F%3C%3E"));
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_dquote, dec_esc_dquote,
			       ss_crefa("\"how\" are you?"),
			       ss_crefa("\"\"how\"\" are you?"));
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_squote, dec_esc_squote,
			       ss_crefa("'how' are you?"),
			       ss_crefa("''how'' are you?"));
	const ss_t *ci[5] = { ss_crefa("hellohellohellohellohellohellohello!"),
			      ss_crefa("111111111111111111111111111111111111"),
			      ss_crefa("121212121212121212121212121212121212"),
			      ss_crefa("123123123123123123123123123123123123"),
			      ss_crefa("123412341234123412341234123412341234") };
	ss_t *co = ss_alloca(256);
	int j;
	for (j = 0; j < 5; j++) {
#ifdef _MSC_VER /* supress alloca() warning */
#pragma warning(disable: 6263)
#endif
		ss_enc_lzw(&co, ci[j]);
		MK_TEST_SS_DUP_CPY_CAT(enc_lzw, dec_lzw, ci[j], co);
		ss_enc_rle(&co, ci[j]);
		MK_TEST_SS_DUP_CPY_CAT(enc_rle, dec_rle, ci[j], co);
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
	STEST_ASSERT(test_ss_cpy_int(9223372036854775807LL,
				     "9223372036854775807"));
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
	char btmp1[400], btmp2[400];
	memset(btmp1, 48, sizeof(btmp1) - 1); /* 0's */
	memset(btmp2, 49, sizeof(btmp2) - 1); /* 1's */
	btmp1[sizeof(btmp1) - 1] = btmp2[sizeof(btmp2) - 1] = 0;
	STEST_ASSERT(test_ss_cat_c(btmp1, btmp2));
	STEST_ASSERT(test_ss_cat_wn());
	STEST_ASSERT(test_ss_cat_w(L"hello", L"all"));
	STEST_ASSERT(test_ss_cat_int("prefix", 1, "prefix1"));
	STEST_ASSERT(test_ss_cat_erase("x", "hello", 2, 2, "xheo"));
	STEST_ASSERT(test_ss_cat_erase_u());
	STEST_ASSERT(test_ss_cat_replace("x", "hello", "ll", "*LL*",
					 "xhe*LL*o"));
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
	STEST_ASSERT(test_ss_tolower("III", U8_S_I_DOTLESS_131 \
					    U8_S_I_DOTLESS_131 \
					    U8_S_I_DOTLESS_131));
	STEST_ASSERT(test_ss_toupper(U8_S_I_DOTLESS_131, "I"));
	STEST_ASSERT(test_ss_tolower(U8_C_I_DOTTED_130, "i"));
	STEST_ASSERT(test_ss_toupper("i", U8_C_I_DOTTED_130));
	STEST_ASSERT(test_ss_tolower(U8_C_G_BREVE_11E, U8_S_G_BREVE_11F));
	STEST_ASSERT(test_ss_toupper(U8_S_G_BREVE_11F, U8_C_G_BREVE_11E));
	STEST_ASSERT(test_ss_tolower(U8_C_S_CEDILLA_15E,
				     U8_S_S_CEDILLA_15F));
	STEST_ASSERT(test_ss_toupper(U8_S_S_CEDILLA_15F,
				     U8_C_S_CEDILLA_15E));
	STEST_ASSERT(!ss_set_turkish_mode(0));
#endif
	STEST_ASSERT(test_ss_clear(""));
	STEST_ASSERT(test_ss_clear("hello"));
	STEST_ASSERT(test_ss_check());
	STEST_ASSERT(test_ss_replace("where are you? where are we?", 0,
				      "where", "who",
				      "who are you? who are we?"));
	STEST_ASSERT(test_ss_replace("who are you? who are we?", 0,
				      "who", "where",
				      "where are you? where are we?"));
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
	const char *utf8[] = { "a", "$", U8_CENT_00A2, U8_EURO_20AC,
			       U8_HAN_24B62, U8_C_N_TILDE_D1,
			       U8_S_N_TILDE_F1 };
	const int32_t uc[] = { 'a', '$', 0xa2, 0x20ac, 0x24b62, 0xd1, 0xf1 };
	unsigned i = 0;
	for (; i < sizeof(utf8) / sizeof(utf8[0]); i++) {
		STEST_ASSERT(test_sc_utf8_to_wc(utf8[i], uc[i]));
		STEST_ASSERT(test_sc_wc_to_utf8(uc[i], utf8[i]));
	}
	/*
	 * Windows require specific locale for doing case conversions properly
	 */
#if !defined(_MSC_VER) && !defined(__CYGWIN__) && !defined(S_MINIMAL)
	if (unicode_support) {
		const unsigned wchar_range = sizeof(wchar_t) == 2 ? 0xd7ff :
								    0x3fffff;
		size_t test_tolower = 0;
		for (i = 0; i <= wchar_range; i++) {
			int32_t l0 = (int32_t)towlower((wint_t)i);
			int32_t l = sc_tolower((int)i);
			if (l != l0) {
				fprintf(stderr, "Warning: sc_tolower(%x): %x "
					"[%x system reported]\n", i,
					(unsigned)l, (unsigned)l0);
				test_tolower++;
			}
		}
	#if 0 /* do not break the build if system has no full Unicode support */
		STEST_ASSERT(test_tolower);
	#endif
		size_t test_toupper = 0;
		for (i = 0; i <= wchar_range; i++) {
			int32_t u0 = (int32_t)towupper((wint_t)i),
				u = sc_toupper((int)i);
			if (u != u0) {
				fprintf(stderr, "Warning sc_toupper(%x): %x "
					"[%x system reported]\n", i,
					(unsigned)u, (unsigned)u0);
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
	STEST_ASSERT(test_sv_push_pop_set_i());
	STEST_ASSERT(test_sv_push_pop_set_u());
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
	STEST_ASSERT(test_sm_shrink());
	STEST_ASSERT(test_sm_dup());
	STEST_ASSERT(test_sm_cpy());
	STEST_ASSERT(test_sm_count_u());
	STEST_ASSERT(test_sm_count_i());
	STEST_ASSERT(test_sm_count_s());
	STEST_ASSERT(test_sm_inc_ii32());
	STEST_ASSERT(test_sm_inc_uu32());
	STEST_ASSERT(test_sm_inc_ii());
	STEST_ASSERT(test_sm_inc_si());
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
	 * Low level stuff
	 */
	STEST_ASSERT(test_endianess());
	STEST_ASSERT(test_alignment());
	STEST_ASSERT(test_sbitio());
	STEST_ASSERT(test_lsb_msb());
	/*
	 * Report
	 */
#ifdef S_DEBUG
	fprintf(stderr, "sizeof(ss_t): %u (small mode)\n",
		(unsigned)sizeof(struct SDataSmall));
	fprintf(stderr, "sizeof(ss_t): %u (full mode)\n",
		(unsigned)sizeof(ss_t));
        fprintf(stderr, "max ss_t string length: " FMT_ZU "\n", SS_RANGE);
	fprintf(stderr, "sizeof(sb_t): %u\n", (unsigned)sizeof(sb_t));
	fprintf(stderr, "sizeof(sv_t): %u\n", (unsigned)sizeof(sv_t));
	fprintf(stderr, "sizeof(sm_t): %u\n", (unsigned)sizeof(sm_t));
	fprintf(stderr, "Errors: %i\n", ss_errors);
#endif
	return STEST_END;
}


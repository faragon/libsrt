/*
 * stest.c
 *
 * Tests (this will be splitted/simplified, currently is a way of
 * detecting if something gets broken; it will be converted into
 * a proper API validation).
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */
 
#include "../src/libsrt.h"

/*
 * Unit testing helpers
 */

#define STEST_START	int ss_errors = 0, ss_tmp = 0
#define STEST_END	ss_errors
#define STEST_ASSERT_BASE(a, pre_op) {					\
		pre_op;							\
		const int r = a;					\
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

/*
 * Test common data structures
 */

struct AA { int a, b; };
static const struct AA a1 = { 1, 2 }, a2 = { 3, 4 };

/*
 * Tests generated from templates
 */

/*
 * Following test template covers two cases: non-aliased (sa to sb) and
 * aliased (sa to sa)
 */
#define MK_TEST_SS_DUP_CODEC(suffix)						\
	static int test_ss_dup_##suffix(const char *a, const char *b) {		\
		ss_t *sa = ss_dup_c(a), *sb = ss_dup_##suffix(sa);		\
		int res = (!sa || !sb) ? 1 : (ss_##suffix(&sa, sa) ? 0 : 2) |	\
			(!strcmp(ss_to_c(sa), b) ? 0 : 4) |			\
			(!strcmp(ss_to_c(sb), b) ? 0 : 8);			\
		ss_free(&sa, &sb);						\
		return res;							\
	}

#define MK_TEST_SS_CPY_CODEC(suffix)						\
	static int test_ss_cpy_##suffix(const char *a, const char *b) {		\
		ss_t *sa = ss_dup_c(a), *sb = ss_dup_c("garbage i!&/()=");	\
		ss_cpy_##suffix(&sb, sa);					\
		int res = (!sa || !sb) ? 1 : (ss_##suffix(&sa, sa) ? 0 : 2) |	\
			(!strcmp(ss_to_c(sa), b) ? 0 : 4) |			\
			(!strcmp(ss_to_c(sb), b) ? 0 : 8);			\
		ss_free(&sa, &sb);						\
		return res;							\
	}

#define MK_TEST_SS_CAT_CODEC(suffix)					  \
	static int test_ss_cat_##suffix(const char *a, const char *b,	  \
					const char *expected) {		  \
		ss_t *sa = ss_dup_c(a), *sb = ss_dup_c(b);		  \
		ss_cat_##suffix(&sa, sb);				  \
		int res = (!sa || !sb) ? 1 :				  \
				(!strcmp(ss_to_c(sa), expected) ? 0 : 2); \
		ss_free(&sa, &sb);					  \
		return res;						  \
	}

#define MK_TEST_SS_DUP_CPY_CAT_CODEC(codec)	\
	MK_TEST_SS_DUP_CODEC(codec)		\
	MK_TEST_SS_CPY_CODEC(codec)		\
	MK_TEST_SS_CAT_CODEC(codec)

MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_b64)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_hex)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_HEX)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_xml)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_json)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_url)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_dquote)
MK_TEST_SS_DUP_CPY_CAT_CODEC(enc_esc_squote)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_b64)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_hex)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_xml)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_json)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_url)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_dquote)
MK_TEST_SS_DUP_CPY_CAT_CODEC(dec_esc_squote)

/*
 * Tests
 */

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

static int test_ss_grow(const size_t init_size, const size_t extra_size,
		 unsigned expected_struct_type)
{
	ss_t *a = ss_alloc(init_size);
	int res = a ? 0 : 1;
	res |= res ? 0 : (ss_grow(&a, extra_size) ? 0 : 2) |
			(a->is_full == expected_struct_type ? 0 : 4);
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
	const char *i0 = "\xc3\x91" "1234567890";
	const char *i1 = "\xc3\x91" "1234567890__";
	const char *i2 = "\xc3\x91" "12";
	const char *i3 = "\xc3\x91" "123";
	ss_t *a = ss_dup_c(i0), *b = ss_dup(a);
	int res = a && b ? 0 : 1;
	res |= res ? 0: ((ss_resize(&a, 14, '_') &&
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
	size_t in_wlen = ss_len_u(a);
	size_t expected_len = char_off >= in_wlen ? in_wlen :
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
	int res = !sa ? 1 : (ss_len(sa) == expected_size ? 0 : 1);
	ss_free(&sa);
	return res;
}

static int test_ss_len_u(const char *a, const size_t expected_size)
{
	ss_t *sa = ss_dup_c(a);
	int res = !sa ? 1 : (ss_len_u(sa) == expected_size ? 0 : 1);
	ss_free(&sa);
	return res;
}

static int test_ss_capacity()
{
	const size_t test_max_size = 100;
	ss_t *a = ss_alloc(test_max_size);
	ss_cpy_c(&a, "a");
	int res = !a ? 1 : (ss_capacity(a) == test_max_size ? 0 : 2);
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
	int res = (a && b) ? 0 : 1;
	res |= res ? 0 : (ss_max(a) == 10 ? 0 : 1);
	res |= res ? 0 : (ss_max(b) == SS_RANGE_FULL ? 0 : 2);
	ss_free(&b);
	return res;
}

static int test_ss_dup()
{
	ss_t *b = ss_dup_c("hello");
	ss_t *a = ss_dup(b);
	int res = !a ? 1 : (!strcmp("hello", ss_to_c(a)) ? 0 : 2);
	ss_free(&a, &b);
	return res;
}

static int test_ss_dup_sub()
{
	ss_t *b = ss_dup_c("hello");
	sv_t *sub = sv_alloc_t(SV_U64, 2);
	sv_push_u(&sub, 0);
	sv_push_u(&sub, 5);
	ss_t *a = ss_dup_sub(b, sub, 0);
	int res = !a ? 1 : (ss_len(a) == ss_nth_size(sub, 0) ? 0 : 2) |
			  (!strcmp("hello", ss_to_c(a)) ? 0 : 4);
	ss_free(&a, &b);
	sv_free(&sub);
	return res;
}

static int test_ss_dup_substr()
{
	ss_t *a = ss_dup_c("hello");
	ss_t *b = ss_dup_substr(a, 0, 5);
	int res = (!a || !b) ? 1 : (ss_len(a) == ss_len(b) ? 0 : 2) |
			  	(!strcmp("hello", ss_to_c(a)) ? 0 : 4);
	ss_free(&a, &b);
	return res;
}

static int test_ss_dup_substr_u()
{
	ss_t *b = ss_dup_c("hello");
	ss_t *a = ss_dup_substr(b, 0, 5);
	int res = !a ? 1 : (ss_len(a) == ss_len(b) ? 0 : 2) |
			  (!strcmp("hello", ss_to_c(a)) ? 0 : 4);
	ss_free(&a, &b);
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
	ss_t *a = ss_dup_cn("hello", 5);
	int res = !a ? 1 : (!strcmp("hello", ss_to_c(a)) ? 0 : 2);
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

static int test_ss_dup_int(const sint_t num, const char *expected)
{
	ss_t *a = ss_dup_int(num);
	int res = !a ? 1 : (!strcmp(ss_to_c(a), expected) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_dup_tolower(const char *a, const char *b)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_tolower(sa);
	int res = (!sa ||!sb) ? 1 : (ss_tolower(&sa) ? 0 : 2) |
			   (!strcmp(ss_to_c(sa), b) ? 0 : 4) |
			   (!strcmp(ss_to_c(sb), b) ? 0 : 8);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_dup_toupper(const char *a, const char *b)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_toupper(sa);
	int res = (!sa ||!sb) ? 1 : (ss_toupper(&sa) ? 0 : 2) |
			   (!strcmp(ss_to_c(sa), b) ? 0 : 4) |
			   (!strcmp(ss_to_c(sb), b) ? 0 : 8);
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
	ss_t *a = ss_dup_c("hel" "\xc3\xb1" "lo"), *b = ss_dup_erase_u(a, 2, 3);
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
	ss_t *a = ss_dup_c("\xc3\xb1" "hello"), *b = ss_dup_resize_u(a, 11, 'z'),
	     *c = ss_dup_resize_u(a, 3, 'z');
	int res = (!a || !b) ?
		1 :
		(!strcmp(ss_to_c(b), "\xc3\xb1" "hellozzzzz") ? 0 : 2) |
		(!strcmp(ss_to_c(c), "\xc3\xb1" "he") ? 0 : 4);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_dup_trim(const char *a, const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_trim(sa);
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sb), expected) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_dup_ltrim(const char *a, const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_ltrim(sa);
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sb), expected) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_dup_rtrim(const char *a, const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_rtrim(sa);
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sb), expected) ? 0 : 2);
	ss_free(&sa, &sb);
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

static int test_ss_dup_char(const sint32_t in, const char *expected)
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
	int f = open(STEST_FILE, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0777);
	if (f >= 0) {
		res = write(f, pattern, pattern_size) != pattern_size ? 2 :
		      lseek(f, 0, SEEK_SET) != 0 ? 4 :
		      (s = ss_dup_read(f, pattern_size)) == NULL ? 8 :
		      strcmp(ss_to_c(s), pattern) ? 16 : 0;
		close(f);
		unlink(STEST_FILE);
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

static int test_ss_cpy_sub()
{
	sv_t *v = NULL;
	ss_t *d = NULL;
	ss_t *a = ss_dup_c("how are you"), *b = ss_dup_c(" "),
		  *c = ss_dup_c("how");
	int res = (!a || !b) ? 1 : (ss_split(&v, a, b) == 3 ? 0 : 2);
	res |= res ? 0 : (ss_cpy_sub(&d, a, v, 0) ? 0 : 4);
	res |= res ? 0 : (!ss_cmp(c, d) ? 0 : 8);
	ss_free(&a, &b, &c, &d);
	sv_free(&v);
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
	res |= res ? 0 : ss_cpy_w(&a, in, in) && !strcmp(ss_to_c(a), tmp) ? 0 : 8;
	ss_free(&a);
	return res;
}

static int test_ss_cpy_int(const sint_t num, const char *expected)
{
	ss_t *a = NULL;
	ss_cpy_int(&a, num);
	int res = !a ? 1 : (!strcmp(ss_to_c(a), expected) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_cpy_tolower(const char *a, const char *b)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c("garbage i!&/()=");
	ss_cpy_tolower(&sb, sa);
	int res = (!sa ||!sb) ? 1 : (ss_tolower(&sa) ? 0 : 2) |
			   (!strcmp(ss_to_c(sa), b) ? 0 : 4) |
			   (!strcmp(ss_to_c(sb), b) ? 0 : 8);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cpy_toupper(const char *a, const char *b)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c("garbage i!&/()=");
	ss_cpy_toupper(&sb, sa);
	int res = (!sa ||!sb) ? 1 : (ss_toupper(&sa) ? 0 : 2) |
			   (!strcmp(ss_to_c(sa), b) ? 0 : 4) |
			   (!strcmp(ss_to_c(sb), b) ? 0 : 8);
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
	ss_t *a = ss_dup_c("hel" "\xc3\xb1" "lo"),
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
	ss_t *a = ss_dup_c("\xc3\xb1" "hello"), *b = ss_dup_c("garbage i!&/()="),
	     *c = ss_dup_c("garbage i!&/()=");
	ss_cpy_resize_u(&b, a, 11, 'z');
	ss_cpy_resize_u(&c, a, 3, 'z');
	int res = (!a || !b) ? 1 :
			(!strcmp(ss_to_c(b), "\xc3\xb1" "hellozzzzz") ? 0 : 2) |
			(!strcmp(ss_to_c(c), "\xc3\xb1" "he") ? 0 : 4);
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

static int test_ss_cpy_char(const sint32_t in, const char *expected)
{
	ss_t *a = ss_dup_c("zz");
	ss_cpy_char(&a, in);
	int res = !a ? 1 : !strcmp(ss_to_c(a), expected) ? 0 : 2;
	ss_free(&a);
	return res;
}

static int test_ss_cpy_read()
{
	return 0; /* TODO */
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
	res |= res ? 0 : (!strcmp(ss_to_c(sc), "cdef")) ? 0 : 64;
#ifdef S_DEBUG
	/* Check that multiple append uses just one -or zero- allocation: */
	size_t alloc_calls_after = dbg_cnt_alloc_calls;
	res |= res ? 0 :
		     (alloc_calls_after - alloc_calls_before) <= 1 ? 0 : 128;
#endif
	ss_free(&sa, &sb, &sc, &sd, &se, &sf);
	return res;
}

static int test_ss_cat_sub()
{
	sv_t *v = NULL;
	ss_t *a = ss_dup_c("how are you"), *b = ss_dup_c(" "),
		    *c = ss_dup_c("are you"), *d = ss_dup_c("are ");
	int res = (!a || !b) ? 1 : (ss_split(&v, a, b) == 3 ? 0 : 2);
	res |= res ? 0 : (ss_cat_sub(&d, a, v, 2) ? 0 : 4);
	res |= res ? 0 : (!ss_cmp(c, d) ? 0 : 8);
	ss_free(&a, &b, &c, &d);
	sv_free(&v);
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
	ss_t *a = ss_dup_c("how ar" "\xf0\xa4\xad\xa2" " you"),
	     *b = ss_dup_c(" "), *c = ss_dup_c("ar" "\xf0\xa4\xad\xa2" " you"),
	     *d = ss_dup_c("ar" "\xf0\xa4\xad\xa2" " ");
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
	ss_t *sa = ss_dup_c(a);
	int res = sa ? 0 : 1;
	char btmp[8192];
	sprintf(btmp, "%s%s%s", a, b, b);
	res |= res ? 0 : (ss_cat_c(&sa, b, b) ? 0 : 2) |
			(!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	ss_free(&sa);
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
	ss_t *sa = ss_dup_w(a);
	int res = sa ? 0 : 1;
	char btmp[8192];
	sprintf(btmp, "%ls%ls%ls%ls", a, b, b, b);
	res |= res ? 0 : (ss_cat_w(&sa, b, b, b) ? 0 : 2) |
			 (!strcmp(ss_to_c(sa), btmp) ? 0 : 4);
	ss_free(&sa);
	return res;
}

static int test_ss_cat_int(const char *in, const sint_t num,
			   const char *expected)
{
	ss_t *a = ss_dup_c(in);
	ss_cat_int(&a, num);
	int res = !a ? 1 : (!strcmp(ss_to_c(a), expected) ? 0 : 2);
	ss_free(&a);
	return res;
}

static int test_ss_cat_tolower(const char *a, const char *b,
			       const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	ss_cat_tolower(&sa, sb);
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sa), expected) ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_cat_toupper(const char *a, const char *b,
			       const char *expected)
{
	ss_t *sa = ss_dup_c(a), *sb = ss_dup_c(b);
	ss_cat_toupper(&sa, sb);
	int res = (!sa || !sb) ? 1 : (!strcmp(ss_to_c(sa), expected) ? 0 : 2);
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
	ss_t *a = ss_dup_c("hel" "\xc3\xb1" "lo"), *b = ss_dup_c("x");
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
	int res = (!a || !b) ? 1 : (!strcmp(ss_to_c(b), "xhellozzzzz") ? 0 : 2) |
				   (!strcmp(ss_to_c(c), "xhe") ? 0 : 4);
	ss_free(&a, &b, &c);
	return res;
}

static int test_ss_cat_resize_u()
{
	ss_t *a = ss_dup_c("\xc3\xb1" "hello"), *b = ss_dup_c("x"),
	     *c = ss_dup_c("x");
	ss_cat_resize_u(&b, a, 11, 'z');
	ss_cat_resize_u(&c, a, 3, 'z');
	int res = (!a || !b) ? 1 :
		    (!strcmp(ss_to_c(b), "x\xc3\xb1" "hellozzzzz") ? 0 : 2) |
		    (!strcmp(ss_to_c(c), "x\xc3\xb1" "he") ? 0 : 4);
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
	return 0; /* TODO */
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
	ss_clear(&sa);
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
	const size_t ssa = ss_len(a);
	wchar_t *out = a ? (wchar_t *)__sd_malloc(sizeof(wchar_t) * (ssa + 1)) :
			  NULL;
	size_t out_size = 0;
	int res = !a ? 1 : (ss_to_w(a, out, ssa + 1, &out_size) ? 0 : 2);
	res |= ((ssa > 0 && out_size > 0) ? 0 : 4);
	if (!res) {
		char b1[16384], b2[16384];
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
	int res = (!sa || !sb) ? 1 : (ss_find(sa, 0, sb) == expected_loc ? 0 : 2);
	ss_free(&sa, &sb);
	return res;
}

static int test_ss_split()
{
	sv_t *v = NULL;
	ss_t *a = ss_dup_c("how are you"), *b = ss_dup_c(" ");
	int res = (!a || !b) ? 1 : (ss_split(&v, a, b) == 3 ? 0 : 2);
	res |= res ? 0 :
		(ss_nth_offset(v, 0) == 0 && ss_nth_size(v, 0) == 3 &&
		 ss_nth_offset(v, 1) == 4 && ss_nth_size(v, 1) == 3 &&
		 ss_nth_offset(v, 2) == 8 && ss_nth_size(v, 2) == 3 ? 0 : 4);
	ss_free(&a, &b);
	sv_free(&v);
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
	ss_t *a = ss_dup_c("12" "\xc3\x91" "a");
	int res = !a ? 1 : (ss_popchar(&a) == 'a' ? 0 : 2) |
			   (ss_popchar(&a) == 0xd1 ? 0 : 4) |
			   (ss_popchar(&a) == '2' ? 0 : 8) |
			   (ss_popchar(&a) == '1' ? 0 : 16) |
			   (ss_popchar(&a) == EOF ? 0 : 32);
	ss_free(&a);
	return res;
}

static int test_ss_read()
{
	return 0; /* TODO */
}

static int test_ss_csum32()
{
	return 0; /* TODO */
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

#define TEST_SV_ALLOC(sv_alloc_x, sv_alloc_x_t, free_op)		\
	sv_t *a = sv_alloc_x(sizeof(struct AA), 10);			\
	sv_t *b = sv_alloc_x_t(SV_I8, 10);				\
	int res = (!a || !b) ? 1 : (sv_len(a) == 0 ? 0 : 2) |		\
				   (sv_push(&a, &a1) ? 0 : 4) |		\
				   (sv_len(a) == 1 ? 0 : 8) |		\
				   (!sv_push(&a, NULL) ? 0 : 16) |	\
				   (sv_len(a) == 1 ? 0 : 32);		\
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

#define TEST_SV_GROW(v, pushval, ntest, sv_alloc_f, data_id,		\
		     initial_reserve, sv_push_x)			\
	sv_t *v = sv_alloc_f(data_id, initial_reserve);			\
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
	TEST_SV_GROW(a, &a1, 0, sv_alloc, sizeof(struct AA), 1, sv_push);
	TEST_SV_GROW(b, 123, 1, sv_alloc_t, SV_U8, 1, sv_push_u);
	TEST_SV_GROW(c, 123, 2, sv_alloc_t, SV_I8, 1, sv_push_i);
	TEST_SV_GROW(d, 123, 3, sv_alloc_t, SV_U16, 1, sv_push_u);
	TEST_SV_GROW(e, 123, 4, sv_alloc_t, SV_I16, 1, sv_push_i);
	TEST_SV_GROW(f, 123, 5, sv_alloc_t, SV_U32, 1, sv_push_u);
	TEST_SV_GROW(g, 123, 6, sv_alloc_t, SV_I32, 1, sv_push_i);
	TEST_SV_GROW(h, 123, 7, sv_alloc_t, SV_U64, 1, sv_push_u);
	TEST_SV_GROW(i, 123, 8, sv_alloc_t, SV_I64, 1, sv_push_i);
	return res;
}

#define TEST_SV_RESERVE(v, ntest, sv_alloc_f, data_id, reserve)		\
	sv_t *v = sv_alloc_f(data_id, 1);			      	\
	res |= !v ? 1<<(ntest*3) :					\
		      (sv_reserve(&v, reserve) &&			\
		       sv_len(v) == 0 &&				\
		       sv_capacity(v) == reserve) ? 0 : 2<<(ntest*3);	\
	sv_free(&v);

static int test_sv_reserve()
{
	int res = 0;
	TEST_SV_RESERVE(a, 0, sv_alloc, sizeof(struct AA), 1);
	TEST_SV_RESERVE(b, 1, sv_alloc_t, SV_U8, 1);
	TEST_SV_RESERVE(c, 2, sv_alloc_t, SV_I8, 1);
	TEST_SV_RESERVE(d, 3, sv_alloc_t, SV_U16, 1);
	TEST_SV_RESERVE(e, 4, sv_alloc_t, SV_I16, 1);
	TEST_SV_RESERVE(f, 5, sv_alloc_t, SV_U32, 1);
	TEST_SV_RESERVE(g, 6, sv_alloc_t, SV_I32, 1);
	TEST_SV_RESERVE(h, 7, sv_alloc_t, SV_U64, 1);
	TEST_SV_RESERVE(i, 8, sv_alloc_t, SV_I64, 1);
	return res;
}

#define TEST_SV_SHRINK_TO_FIT(v, ntest, alloc, push, type, pushval, r)	\
	sv_t *v = alloc(type, r);					\
	res |= !v ? 1<<(ntest*3) :					\
		(push(&v, pushval) &&					\
		 sv_shrink(&v) &&					\
		 sv_capacity(v) == sv_len(v)) ? 0 : 2<<(ntest*3);	\
	sv_free(&v)

static int test_sv_shrink()
{
	int res = 0;
	TEST_SV_SHRINK_TO_FIT(a, 0, sv_alloc, sv_push,
			      sizeof(struct AA), &a1, 100);
	TEST_SV_SHRINK_TO_FIT(b, 1, sv_alloc_t, sv_push_i, SV_I8, 123, 100);
	TEST_SV_SHRINK_TO_FIT(c, 2, sv_alloc_t, sv_push_u, SV_U8, 123, 100);
	TEST_SV_SHRINK_TO_FIT(d, 3, sv_alloc_t, sv_push_i, SV_I16, 123, 100);
	TEST_SV_SHRINK_TO_FIT(e, 4, sv_alloc_t, sv_push_u, SV_U16, 123, 100);
	TEST_SV_SHRINK_TO_FIT(f, 5, sv_alloc_t, sv_push_i, SV_I32, 123, 100);
	TEST_SV_SHRINK_TO_FIT(g, 6, sv_alloc_t, sv_push_u, SV_U32, 123, 100);
	TEST_SV_SHRINK_TO_FIT(h, 7, sv_alloc_t, sv_push_i, SV_I64, 123, 100);
	TEST_SV_SHRINK_TO_FIT(i, 8, sv_alloc_t, sv_push_u, SV_U64, 123, 100);
	return res;
}

#define TEST_SV_LEN(v, ntest, alloc, push, type, pushval)	\
	sv_t *v = alloc(type, 1);				\
	res |= !v ? 1<<(ntest*3) :				\
		(push(&v, pushval) && push(&v, pushval) &&	\
		 push(&v, pushval) && push(&v, pushval) &&	\
		 sv_len(v) == 4) ? 0 : 2<<(ntest*3);		\
	sv_free(&v)

static int test_sv_len()
{
	int res = 0;
	TEST_SV_LEN(a, 0, sv_alloc, sv_push, sizeof(struct AA), &a1);
	TEST_SV_LEN(b, 1, sv_alloc_t, sv_push_i, SV_I8, 123);
	TEST_SV_LEN(c, 2, sv_alloc_t, sv_push_u, SV_U8, 123);
	TEST_SV_LEN(d, 3, sv_alloc_t, sv_push_i, SV_I16, 123);
	TEST_SV_LEN(e, 4, sv_alloc_t, sv_push_u, SV_U16, 123);
	TEST_SV_LEN(f, 5, sv_alloc_t, sv_push_i, SV_I32, 123);
	TEST_SV_LEN(g, 6, sv_alloc_t, sv_push_u, SV_U32, 123);
	TEST_SV_LEN(h, 7, sv_alloc_t, sv_push_i, SV_I64, 123);
	TEST_SV_LEN(i, 8, sv_alloc_t, sv_push_u, SV_U64, 123);
	return res;
}

#ifdef SD_ENABLE_HEURISTIC_GROW
#define SDCMP >=
#else
#define SDCMP ==
#endif

#define TEST_SV_CAPACITY(v, ntest, alloc, push, type, pushval)		\
	sv_t *v = alloc(type, 2);					\
	res |= !v ? 1<<(ntest*3) :					\
		(sv_capacity(v) == 2 &&	sv_reserve(&v, 100) &&		\
		 sv_capacity(v) SDCMP 100 && sv_shrink(&v) &&		\
		 sv_capacity(v) == 1) ? 0 : 2<<(ntest*3);		\
	sv_free(&v)

static int test_sv_capacity()
{
	int res = 0;
	TEST_SV_CAPACITY(a, 0, sv_alloc, sv_push, sizeof(struct AA), &a1);
	TEST_SV_CAPACITY(b, 1, sv_alloc_t, sv_push_i, SV_I8, 123);
	TEST_SV_CAPACITY(c, 2, sv_alloc_t, sv_push_u, SV_U8, 123);
	TEST_SV_CAPACITY(d, 3, sv_alloc_t, sv_push_i, SV_I16, 123);
	TEST_SV_CAPACITY(e, 4, sv_alloc_t, sv_push_u, SV_U16, 123);
	TEST_SV_CAPACITY(f, 5, sv_alloc_t, sv_push_i, SV_I32, 123);
	TEST_SV_CAPACITY(g, 6, sv_alloc_t, sv_push_u, SV_U32, 123);
	TEST_SV_CAPACITY(h, 7, sv_alloc_t, sv_push_i, SV_I64, 123);
	TEST_SV_CAPACITY(i, 8, sv_alloc_t, sv_push_u, SV_U64, 123);
	return res;
}

static int test_sv_len_left()
{
	const size_t init_alloc = 10;
	sv_t *a = sv_alloc_t(SV_I8, init_alloc);
	int res = !a ? 1 : (sv_capacity(a) - sv_len(a) == init_alloc ? 0 : 2) |
			   (sv_len_left(a) == init_alloc ? 0 : 4) |
			   (sv_push_i(&a, 1) ? 0 : 8) |
			   (sv_len_left(a) == (init_alloc - 1) ? 0 : 16);
	sv_free(&a);
	return res;
}

static int test_sv_set_len()
{
	const size_t init_alloc = 10;
	sv_t *a = sv_alloc_t(SV_I8, init_alloc);
	int res = !a ? 1 : (sv_len(a) == 0 ? 0 : 2) |
			   (!sv_set_len(a, init_alloc + 1) ? 0 : 4) |
			   (sv_set_len(a, init_alloc) ? 0 : 8) |
			   (sv_len_left(a) == 0 ? 0 : 16);
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
		unsigned ai = *(unsigned *)sv_get_buffer(a),
			 bi = *(unsigned *)sv_get_buffer(b),
			 air = *(const unsigned *)sv_get_buffer_r(a),
			 bir = *(const unsigned *)sv_get_buffer_r(b),
			 cir = *(unsigned *)sv_get_buffer(c),
			 dir = *(unsigned *)sv_get_buffer(d);
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

#define TEST_SV_DUP(v, ntest, alloc, push, check, check2, type, pushval)\
	sv_t *v = alloc(type, 0);					\
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
		    sizeof(struct AA), &a1);
	#define SVIAT(v) sv_i_at(v, 0) == val
	#define SVUAT(v) sv_u_at(v, 0) == (unsigned)val
	#define CHKPI(v) sv_pop_i(v) == sv_pop_i(v##2)
	#define CHKPU(v) sv_pop_u(v) == sv_pop_u(v##2)
	TEST_SV_DUP(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), CHKPI(b), SV_I8, val);
	TEST_SV_DUP(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), CHKPU(c), SV_U8, val);
	TEST_SV_DUP(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), CHKPI(d), SV_I16, val);
	TEST_SV_DUP(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), CHKPU(e), SV_U16, val);
	TEST_SV_DUP(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), CHKPI(f), SV_I32, val);
	TEST_SV_DUP(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), CHKPU(g), SV_U32, val);
	TEST_SV_DUP(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), CHKPI(h), SV_I64, val);
	TEST_SV_DUP(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), CHKPU(i), SV_U64, val);
	#undef SVIAT
	#undef SVUAT
	#undef CHKPI
	#undef CHKPU
	return res;
}

#define TEST_SV_DUP_ERASE(v, ntest, alloc, push, check, type, a, b)	\
	sv_t *v = alloc(type, 0);					\
	push(&v, a); push(&v, b); push(&v, b); push(&v, b); push(&v, b);\
	push(&v, b); push(&v, a); push(&v, b); push(&v, b); push(&v, b);\
	sv_t *v##2 = sv_dup_erase(v, 1, 5);				\
	res |= !v ? 1<<(ntest*3) : (check) ? 0 : 2<<(ntest*3);		\
	sv_free(&v, &v##2);

static int test_sv_dup_erase()
{
	int res = 0;
	const int r = 12, s = 34;
	TEST_SV_DUP_ERASE(z, 0, sv_alloc, sv_push,
	    ((const struct AA *)sv_at(z2, 0))->a ==
				((const struct AA *)sv_at(z2, 1))->a,
	    sizeof(struct AA), &a1, &a2);
	#define SVIAT(v) sv_i_at(v##2, 0) == sv_i_at(v##2, 1)
	#define SVUAT(v) sv_u_at(v##2, 0) == sv_u_at(v##2, 1)
	TEST_SV_DUP_ERASE(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), SV_I8, r, s);
	TEST_SV_DUP_ERASE(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), SV_U8, r, s);
	TEST_SV_DUP_ERASE(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), SV_I16, r, s);
	TEST_SV_DUP_ERASE(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), SV_U16, r, s);
	TEST_SV_DUP_ERASE(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), SV_I32, r, s);
	TEST_SV_DUP_ERASE(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), SV_U32, r, s);
	TEST_SV_DUP_ERASE(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), SV_I64, r, s);
	TEST_SV_DUP_ERASE(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), SV_U64, r, s);
	#undef SVIAT
	#undef SVUAT
	return res;
}

#define TEST_SV_DUP_RESIZE(v, ntest, alloc, push, type, a, b)		       \
	sv_t *v = alloc(type, 0);					       \
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
	TEST_SV_DUP_RESIZE(z, 0, sv_alloc, sv_push, sizeof(struct AA), &a1, &a2);
	TEST_SV_DUP_RESIZE(b, 1, sv_alloc_t, sv_push_i, SV_I8, r, s);
	TEST_SV_DUP_RESIZE(c, 2, sv_alloc_t, sv_push_u, SV_U8, r, s);
	TEST_SV_DUP_RESIZE(d, 3, sv_alloc_t, sv_push_i, SV_I16, r, s);
	TEST_SV_DUP_RESIZE(e, 4, sv_alloc_t, sv_push_u, SV_U16, r, s);
	TEST_SV_DUP_RESIZE(f, 5, sv_alloc_t, sv_push_i, SV_I32, r, s);
	TEST_SV_DUP_RESIZE(g, 6, sv_alloc_t, sv_push_u, SV_U32, r, s);
	TEST_SV_DUP_RESIZE(h, 7, sv_alloc_t, sv_push_i, SV_I64, r, s);
	TEST_SV_DUP_RESIZE(i, 8, sv_alloc_t, sv_push_u, SV_U64, r, s);
	return res;
}

#define TEST_SV_CPY(v, nt, alloc, push, type, a, b)		       	   \
	sv_t *v = alloc(type, 0);					   \
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
	TEST_SV_CPY(z, 0, sv_alloc, sv_push, sizeof(struct AA), &a1, &a2);
	TEST_SV_CPY(b, 1, sv_alloc_t, sv_push_i, SV_I8, r, s);
	TEST_SV_CPY(c, 2, sv_alloc_t, sv_push_u, SV_U8, r, s);
	TEST_SV_CPY(d, 3, sv_alloc_t, sv_push_i, SV_I16, r, s);
	TEST_SV_CPY(e, 4, sv_alloc_t, sv_push_u, SV_U16, r, s);
	TEST_SV_CPY(f, 5, sv_alloc_t, sv_push_i, SV_I32, r, s);
	TEST_SV_CPY(g, 6, sv_alloc_t, sv_push_u, SV_U32, r, s);
	TEST_SV_CPY(h, 7, sv_alloc_t, sv_push_i, SV_I64, r, s);
	TEST_SV_CPY(i, 8, sv_alloc_t, sv_push_u, SV_U64, r, s);
	return res;
}

#define TEST_SV_CPY_ERASE(v, ntest, alloc, push, check, type, a, b)	 \
	sv_t *v = alloc(type, 0);					 \
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
	    sizeof(struct AA), &a1, &a2);
	#define SVIAT(v) sv_i_at(v##2, 0) == sv_i_at(v##2, 1)
	#define SVUAT(v) sv_u_at(v##2, 0) == sv_u_at(v##2, 1)
	TEST_SV_CPY_ERASE(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), SV_I8, r, s);
	TEST_SV_CPY_ERASE(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), SV_U8, r, s);
	TEST_SV_CPY_ERASE(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), SV_I16, r, s);
	TEST_SV_CPY_ERASE(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), SV_U16, r, s);
	TEST_SV_CPY_ERASE(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), SV_I32, r, s);
	TEST_SV_CPY_ERASE(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), SV_U32, r, s);
	TEST_SV_CPY_ERASE(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), SV_I64, r, s);
	TEST_SV_CPY_ERASE(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), SV_U64, r, s);
	#undef SVIAT
	#undef SVUAT
	return res;
}

#define TEST_SV_CPY_RESIZE(v, ntest, alloc, push, type, a, b)		       \
	sv_t *v = alloc(type, 0);					       \
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
	TEST_SV_CPY_RESIZE(z, 0, sv_alloc, sv_push, sizeof(struct AA), &a1, &a2);
	TEST_SV_CPY_RESIZE(b, 1, sv_alloc_t, sv_push_i, SV_I8, r, s);
	TEST_SV_CPY_RESIZE(c, 2, sv_alloc_t, sv_push_u, SV_U8, r, s);
	TEST_SV_CPY_RESIZE(d, 3, sv_alloc_t, sv_push_i, SV_I16, r, s);
	TEST_SV_CPY_RESIZE(e, 4, sv_alloc_t, sv_push_u, SV_U16, r, s);
	TEST_SV_CPY_RESIZE(f, 5, sv_alloc_t, sv_push_i, SV_I32, r, s);
	TEST_SV_CPY_RESIZE(g, 6, sv_alloc_t, sv_push_u, SV_U32, r, s);
	TEST_SV_CPY_RESIZE(h, 7, sv_alloc_t, sv_push_i, SV_I64, r, s);
	TEST_SV_CPY_RESIZE(i, 8, sv_alloc_t, sv_push_u, SV_U64, r, s);
	return res;
}

#define TEST_SV_CAT(v, ntest, alloc, push, check, check2, type, pushval)\
	sv_t *v = alloc(type, 0);					\
	push(&v, pushval);						\
	sv_t *v##2 = sv_dup(v);						\
	res |= !v ? 1<<(ntest*3) :					\
		(sv_cat(&v, v##2) && sv_cat(&v, v) && sv_len(v) == 4 &&	\
		 (check) && (check2)) ? 0 : 2<<(ntest*3);		\
	sv_free(&v, &v##2);

static int test_sv_cat()
{
	int res = 0;
	const int val = 123;
	TEST_SV_CAT(z, 0, sv_alloc, sv_push,
		    ((const struct AA *)sv_at(z, 3))->a == a1.a,
		    ((const struct AA *)sv_pop(z))->a ==
					((const struct AA *)sv_pop(z2))->a,
		    sizeof(struct AA), &a1);
	#define SVIAT(v) sv_i_at(v, 3) == val
	#define SVUAT(v) sv_u_at(v, 3) == (unsigned)val
	#define CHKPI(v) sv_pop_i(v) == sv_pop_i(v##2)
	#define CHKPU(v) sv_pop_u(v) == sv_pop_u(v##2)
	TEST_SV_CAT(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), CHKPI(b), SV_I8, val);
	TEST_SV_CAT(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), CHKPU(c), SV_U8, val);
	TEST_SV_CAT(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), CHKPI(d), SV_I16, val);
	TEST_SV_CAT(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), CHKPU(e), SV_U16, val);
	TEST_SV_CAT(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), CHKPI(f), SV_I32, val);
	TEST_SV_CAT(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), CHKPU(g), SV_U32, val);
	TEST_SV_CAT(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), CHKPI(h), SV_I64, val);
	TEST_SV_CAT(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), CHKPU(i), SV_U64, val);
	#undef SVIAT
	#undef SVUAT
	#undef CHKPI
	#undef CHKPU
	return res;
}

#define TEST_SV_CAT_ERASE(v, ntest, alloc, push, check, type, a, b)	 \
	sv_t *v = alloc(type, 0), *v##2 = alloc(type, 0);		 \
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
	    sizeof(struct AA), &a1, &a2);
	#define SVIAT(v) sv_i_at(v##2, 0) == sv_i_at(v##2, 2)
	#define SVUAT(v) sv_u_at(v##2, 0) == sv_u_at(v##2, 2)
	TEST_SV_CAT_ERASE(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), SV_I8, r, s);
	TEST_SV_CAT_ERASE(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), SV_U8, r, s);
	TEST_SV_CAT_ERASE(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), SV_I16, r, s);
	TEST_SV_CAT_ERASE(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), SV_U16, r, s);
	TEST_SV_CAT_ERASE(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), SV_I32, r, s);
	TEST_SV_CAT_ERASE(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), SV_U32, r, s);
	TEST_SV_CAT_ERASE(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), SV_I64, r, s);
	TEST_SV_CAT_ERASE(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), SV_U64, r, s);
	#undef SVIAT
	#undef SVUAT
	return res;
}

#define TEST_SV_CAT_RESIZE(v, ntest, alloc, push, type, a, b)		       \
	sv_t *v = alloc(type, 0);					       \
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
	TEST_SV_CAT_RESIZE(z, 0, sv_alloc, sv_push, sizeof(struct AA), &a1, &a2);
	TEST_SV_CAT_RESIZE(b, 1, sv_alloc_t, sv_push_i, SV_I8, r, s);
	TEST_SV_CAT_RESIZE(c, 2, sv_alloc_t, sv_push_u, SV_U8, r, s);
	TEST_SV_CAT_RESIZE(d, 3, sv_alloc_t, sv_push_i, SV_I16, r, s);
	TEST_SV_CAT_RESIZE(e, 4, sv_alloc_t, sv_push_u, SV_U16, r, s);
	TEST_SV_CAT_RESIZE(f, 5, sv_alloc_t, sv_push_i, SV_I32, r, s);
	TEST_SV_CAT_RESIZE(g, 6, sv_alloc_t, sv_push_u, SV_U32, r, s);
	TEST_SV_CAT_RESIZE(h, 7, sv_alloc_t, sv_push_i, SV_I64, r, s);
	TEST_SV_CAT_RESIZE(i, 8, sv_alloc_t, sv_push_u, SV_U64, r, s);
	return res;
}

#define TEST_SV_ERASE(v, ntest, alloc, push, check, type, a, b)	 \
	sv_t *v = alloc(type, 0);					 \
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
	    sizeof(struct AA), &a1, &a2);
	#define SVIAT(v) sv_i_at(v, 0) == sv_i_at(v, 1)
	#define SVUAT(v) sv_u_at(v, 0) == sv_u_at(v, 1)
	TEST_SV_ERASE(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), SV_I8, r, s);
	TEST_SV_ERASE(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), SV_U8, r, s);
	TEST_SV_ERASE(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), SV_I16, r, s);
	TEST_SV_ERASE(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), SV_U16, r, s);
	TEST_SV_ERASE(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), SV_I32, r, s);
	TEST_SV_ERASE(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), SV_U32, r, s);
	TEST_SV_ERASE(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), SV_I64, r, s);
	TEST_SV_ERASE(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), SV_U64, r, s);
	#undef SVIAT
	#undef SVUAT
	return res;
}

#define TEST_SV_RESIZE(v, ntest, alloc, push, check, type, a, b)	 \
	sv_t *v = alloc(type, 0);					 \
	push(&v, b); push(&v, b); push(&v, a); push(&v, a); push(&v, a); \
	push(&v, a); push(&v, b); push(&v, a); push(&v, a); push(&v, a); \
	sv_resize(&v, 5);						 \
	res |= !v ? 1 << (ntest * 3) :					 \
		    ((check) ? 0 : 2 << (ntest * 3)) |			 \
		    (sv_size(v) == 5 ? 0 : 4 << (ntest * 3));		 \
	sv_free(&v);

static int test_sv_resize()
{
	int res = 0;
	const int r = 12, s = 34;
	TEST_SV_RESIZE(z, 0, sv_alloc, sv_push,
	    ((const struct AA *)sv_at(z, 0))->a ==
		    ((const struct AA *)sv_at(z, 1))->a,
	    sizeof(struct AA), &a1, &a2);
	#define SVIAT(v) sv_i_at(v, 0) == sv_i_at(v, 1)
	#define SVUAT(v) sv_u_at(v, 0) == sv_u_at(v, 1)
	TEST_SV_RESIZE(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), SV_I8, r, s);
	TEST_SV_RESIZE(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), SV_U8, r, s);
	TEST_SV_RESIZE(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), SV_I16, r, s);
	TEST_SV_RESIZE(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), SV_U16, r, s);
	TEST_SV_RESIZE(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), SV_I32, r, s);
	TEST_SV_RESIZE(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), SV_U32, r, s);
	TEST_SV_RESIZE(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), SV_I64, r, s);
	TEST_SV_RESIZE(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), SV_U64, r, s);
	#undef SVIAT
	#undef SVUAT
	return res;
}

#define TEST_SV_FIND(v, ntest, alloc, push, check, type, a, b)	 \
	sv_t *v = alloc(type, 0);					 \
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
		     sizeof(struct AA), &a1, &a2);
	#define SVIAT(v) sv_find_i(v, 0, s) == 6
	#define SVUAT(v) sv_find_u(v, 0, s) == 6
	TEST_SV_FIND(b, 1, sv_alloc_t, sv_push_i, SVIAT(b), SV_I8, r, s);
	TEST_SV_FIND(c, 2, sv_alloc_t, sv_push_u, SVUAT(c), SV_U8, r, s);
	TEST_SV_FIND(d, 3, sv_alloc_t, sv_push_i, SVIAT(d), SV_I16, r, s);
	TEST_SV_FIND(e, 4, sv_alloc_t, sv_push_u, SVUAT(e), SV_U16, r, s);
	TEST_SV_FIND(f, 5, sv_alloc_t, sv_push_i, SVIAT(f), SV_I32, r, s);
	TEST_SV_FIND(g, 6, sv_alloc_t, sv_push_u, SVUAT(g), SV_U32, r, s);
	TEST_SV_FIND(h, 7, sv_alloc_t, sv_push_i, SVIAT(h), SV_I64, r, s);
	TEST_SV_FIND(i, 8, sv_alloc_t, sv_push_u, SVUAT(i), SV_U64, r, s);
	#undef SVIAT
	#undef SVUAT
	return res;
}

static int test_sv_push_pop()
{
	struct AA *t = NULL;
	sv_t *a = sv_alloc(sizeof(struct AA), 10);
	sv_t *b = sv_alloca(sizeof(struct AA), 10);
	int res = (!a || !b) ? 1 : 0;
	if (!res) {
		res |= (sv_push(&a, &a2, &a2, &a1) ? 0 : 4);
		res |= (sv_len(a) == 3? 0 : 8);
		res |= (((t = (struct AA *)sv_pop(a)) &&
			t->a == a1.a && t->b == a1.b)? 0 : 16);
		res |= (sv_len(a) == 2? 0 : 32);
		res |= (sv_push(&b, &a2) ? 0 : 64);
		res |= (sv_len(b) == 1? 0 : 128);
		res |= (((t = (struct AA *)sv_pop(b)) &&
			 t->a == a2.a && t->b == a2.b)? 0 : 256);
		res |= (sv_len(b) == 0? 0 : 512);
	}
	sv_free(&a);
	return res;
}

static int test_sv_push_pop_i()
{
	enum eSV_Type t[] = { SV_I8, SV_I16, SV_I32, SV_I64 };
	int i = 0, ntests = sizeof(t)/sizeof(t[0]), res = 0;
	for (; i < ntests; i++) {
		sv_t *a = sv_alloc_t(t[i], 10);
		sv_t *b = sv_alloca_t(t[i], 10);
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
		} while (0);
		sv_free(&a);
	}
	return res;
}

static int test_sv_push_pop_u()
{
	suint_t init[] = { (suint_t)-1, (suint_t)-1, (suint_t)-1, (suint_t)-1 },
		expected[] = { 0xff, 0xffff, 0xffffffff, 0xffffffffffffffffLL };
	enum eSV_Type t[] = { SV_U8, SV_U16, SV_U32, SV_U64 };
	int i = 0, ntests = sizeof(t)/sizeof(t[0]), res = 0;
	for (; i < ntests; i++) {
		sv_t *a = sv_alloc_t(t[i], 10);
		sv_t *b = sv_alloca_t(t[i], 10);
		sint_t r;
		if (!(a && b && sv_push_u(&a, (suint_t)-1) &&
		      sv_push_u(&b, init[i]) &&
		      (r = (sint_t)sv_pop_u(a)) && r == sv_pop_i(b) &&
		      r == (sint_t)expected[i] &&
		      sv_len(a) == 0 && sv_len(b) == 0))
			res |= 1 << i;
		sv_free(&a);
	}
	return res;
}

static int test_sv_push_raw()
{
	/* TODO */
	return 0;
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
	ndx2s(l, sizeof(l), n->l);
	ndx2s(r, sizeof(r), n->r);
	ss_cat_printf(s, 128, "[%u: (%i, %c) -> (%s, %s; r:%u)]",
		     (unsigned)id, node->k, node->v, l, r, n->is_red );
	return *s;
}
#endif

static int test_st_alloc()
{
	struct STConf f = { 0, sizeof(struct MyNode1), (st_cmp_t)cmp1,
			    0, 0, 0, 0 };
	st_t *t = st_alloc(&f, 1000);
	struct MyNode1 n = { EMPTY_STN, 0, 0 };
	int res = !t ? 1 : st_len(t) != 0 ? 2 :
			   (!st_insert(&t, (const stn_t *)&n) ||
			    st_len(t) != 1) ? 4 : 0;
	st_free(&t);
	return res;
}

#define ST_ENV_TEST_AUX						\
	struct STConf f = { 0, sizeof(struct MyNode1),		\
			    (st_cmp_t)cmp1, 0, 0, 0, 0 };	\
	st_t *t = st_alloc(&f, 1000);				\
	if (!t)							\
		return 1;					\
	ss_t *log = NULL;					\
	struct MyNode1 n0 = { EMPTY_STN, 0, 0 };		\
	stn_t *n = (stn_t *)&n0;				\
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
#define ST_CHK_BRK(t, len, err)			\
		if (st_len(t) != (len)) {	\
			res = err;		\
			break;			\
		}

static int test_st_insert_del()
{
	ST_ENV_TEST_AUX;
	int res = 0;
	const struct MyNode1 *nr;
	int i = 0;
	const size_t tree_elems = 90;
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
		if (!nr || nr->v != (cbase + tree_elems - 1) || st_traverse_levelorder(t, NULL, NULL) != 1) {
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
		if (!nr || nr->v != (cbase + tree_elems - 1) || st_traverse_levelorder(t, NULL, NULL) != 1) {
			res = 6;
			break;
		}
		ST_A_DEL(tree_elems - 1);
		ST_CHK_BRK(t, 0, tree_elems);
		/* Case 3: add in order, delete in reverse order */
		for (i = 0; i < (int)tree_elems; i++)
			ST_A_INS(i, cbase + i);	/* ST_A_INS(2, 'c');  <- double rotation */
		ST_CHK_BRK(t, tree_elems, 8);
		for (i = (tree_elems - 1); i > 0; i--)
			ST_A_DEL(i);
		ST_CHK_BRK(t, 1, 9);
		n0.k = 0;
		nr = (const struct MyNode1 *)st_locate(t, &n0.n);
		if (!nr || nr->v != cbase || st_traverse_levelorder(t, NULL, NULL) != 1) {
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
	const struct MyNode1 *node = (const struct MyNode1 *)tp->cn;
	if (node)
		ss_cat_printf(log, 512, "%c", node->v);
	return 0;
}

static int test_st_traverse()
{
	ST_ENV_TEST_AUX;
	int res = 0;
	for (;;) {
		ST_A_INS(6, 'g'); ST_A_INS(7, 'h'); ST_A_INS(8, 'i');
		ST_A_INS(3, 'd'); ST_A_INS(4, 'e'); ST_A_INS(5, 'f');
		ST_A_INS(0, 'a'); ST_A_INS(1, 'b'); ST_A_INS(2, 'c');
		ss_cpy_c(&log, "");
		st_traverse_preorder(t, test_traverse, (void *)&log);

		const char *out = ss_to_c(log);
		if (strcmp(out, "ebadchgfi") != 0) {
			res |= 1;
			break;
		}
		ss_cpy_c(&log, "");
		st_traverse_levelorder(t, test_traverse, (void *)&log);
		out = ss_to_c(log);
		if (strcmp(out, "ebhadgicf") != 0) {
			res |= 2;
			break;
		}
		ss_cpy_c(&log, "");
		st_traverse_inorder(t, test_traverse, (void *)&log);
		out = ss_to_c(log);
		if (strcmp(out, "abcdefghi") != 0) {
			res |= 4;
			break;
		}
		ss_cpy_c(&log, "");
		st_traverse_postorder(t, test_traverse, (void *)&log);
		out = ss_to_c(log);
		if (strcmp(out, "acdbfgihe") != 0) {
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
		sm_t *m = sm_alloc_X(type, 1000);			\
		int res = 0;						\
		for (;;) {						\
			if (!m) { res = 1; break; }			\
			insert(&m, 1, 1001);				\
			if (sm_size(m) != 1) { res = 2; break; }	\
			insert(&m, 2, 1002);				\
			insert(&m, 3, 1003);				\
			if (at(m, 1) != 1001) { res = 3; break; }	\
			if (at(m, 2) != 1002) { res = 4; break; }	\
			if (at(m, 3) != 1003) { res = 5; break; }	\
			break;						\
		}							\
		sm_free_X(&m);						\
		return res;						\
	}

TEST_SM_ALLOC_X(test_sm_alloc_ii32, sm_alloc, SM_I32I32, sm_ii32_insert,
		sm_ii32_at, sm_free)
TEST_SM_ALLOC_X(test_sm_alloca_ii32, sm_alloca, SM_I32I32, sm_ii32_insert,
		sm_ii32_at, TEST_SM_ALLOC_DONOTHING)
TEST_SM_ALLOC_X(test_sm_alloc_uu32, sm_alloc, SM_U32U32, sm_uu32_insert,
		sm_uu32_at, sm_free)
TEST_SM_ALLOC_X(test_sm_alloca_uu32, sm_alloca, SM_U32U32, sm_uu32_insert,
		sm_uu32_at, TEST_SM_ALLOC_DONOTHING)
TEST_SM_ALLOC_X(test_sm_alloc_ii, sm_alloc, SM_IntInt, sm_ii_insert, sm_ii_at,
		sm_free)
TEST_SM_ALLOC_X(test_sm_alloca_ii, sm_alloca, SM_IntInt, sm_ii_insert,
		sm_ii_at, TEST_SM_ALLOC_DONOTHING)

#define TEST_SM_SHRINK_TO_FIT(m, ntest, atype, r)		\
	sm_t *m = sm_alloc(atype, r);				\
	res |= !m ? 1 << (ntest * 3) :				\
		(sm_capacity(m) == r ? 0 : 2 << (ntest * 3)) |	\
		(sm_shrink(&m) &&				\
		 sm_capacity(m) == 1 ? 0 : 4 << (ntest * 3));	\
	sm_free(&m)

static int test_sm_shrink()
{
	int res = 0;
	TEST_SM_SHRINK_TO_FIT(a, 0, SM_I32I32, 100);
	TEST_SM_SHRINK_TO_FIT(b, 1, SM_U32U32, 100);
	TEST_SM_SHRINK_TO_FIT(c, 2, SM_IntInt, 100);
	TEST_SM_SHRINK_TO_FIT(d, 3, SM_IntStr, 100);
	TEST_SM_SHRINK_TO_FIT(e, 4, SM_IntPtr, 100);
	TEST_SM_SHRINK_TO_FIT(f, 5, SM_StrInt, 100);
	TEST_SM_SHRINK_TO_FIT(g, 6, SM_StrStr, 100);
	TEST_SM_SHRINK_TO_FIT(h, 7, SM_StrPtr, 100);
	return res;
}

static int test_sm_dup()
{
	return 0; /* TODO */
}

static int test_sm_is_at()
{
	return 0; /* TODO */
}

static int test_sm_ip_at()
{
	return 0; /* TODO */
}

static int test_sm_si_at()
{
	return 0; /* TODO */
}

static int test_sm_ss_at()
{
	return 0; /* TODO */
}

static int test_sm_sp_at()
{
	return 0; /* TODO */
}

#define TEST_SM_X_COUNT(T, v)				\
	int res = 0;					\
	suint32_t i, tcount = 100;			\
	sm_t *m = sm_alloc(T, tcount);			\
	for (i = 0; i < tcount; i++)			\
		sm_uu32_insert(&m, (unsigned)i, v);	\
	for (i = 0; i < tcount; i++)			\
		if (!sm_u_count(m, i)) {		\
			res = 1 + (int)i;		\
			break;				\
		}					\
	sm_free(&m);					\
	return res;

static int test_sm_u_count()
{
	TEST_SM_X_COUNT(SM_U32U32, 1);
}

static int test_sm_i_count()
{
	TEST_SM_X_COUNT(SM_I32I32, 1);
}

static int test_sm_s_count()
{
	ss_t *s = ss_dup_c("a_1"), *t = ss_dup_c("a_2"), *u = ss_dup_c("a_3");
	sm_t *m = sm_alloc(SM_StrInt, 3);
	sm_si_insert(&m, s, 1);
	sm_si_insert(&m, t, 2);
	sm_si_insert(&m, u, 3);
	int res = sm_s_count(m, s) && sm_s_count(m, t) && sm_s_count(m, u) ? 0 : 1;
	ss_free(&s, &t, &u);
	sm_free(&m);
	return res;
}

static int test_sm_ii32_insert()
{
	return 0; /* TODO */
}

static int test_sm_uu32_insert()
{
	return 0; /* TODO */
}
static int test_sm_ii_insert()
{
	return 0; /* TODO */
}

static int test_sm_is_insert()
{
	return 0; /* TODO */
}

static int test_sm_si_insert()
{
	return 0; /* TODO */
}

static int test_sm_ss_insert()
{
	return 0; /* TODO */
}

static int test_sm_i_delete()
{
	return 0; /* TODO */
}

static int test_sm_s_delete()
{
	return 0; /* TODO */
}

static int test_sm_enum()
{
	return 0; /* TODO */
}

static int test_sm_sort_to_vectors()
{
	const size_t test_elems = 100;
	sm_t *m = sm_alloc(SM_I32I32, test_elems);
	sv_t *kv = NULL, *vv = NULL;
	sv_t *kv2 = NULL, *vv2 = NULL;
	int res = m ? 0 : 1;
	ssize_t i, j;
	for (;;) {
		/*
		 * Add elements
		 */
		for (i = test_elems; i > 0 && !res; i--) {
			if (!sm_ii32_insert(&m, (int)i, (int)-i) ||
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
			for (i = 0; i < j/*test_elems*/; i++) {
				int k = (int)sv_i_at(kv2, (size_t)i);
				int v = (int)sv_i_at(vv2, (size_t)i);
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
			if (!sm_i_delete(m, j) || !st_assert((st_t *)m)) {
				res |= 16;
				break;
			}
		}
		break;
	}
	sm_free(&m);
	sv_free(&kv, &vv, &kv2, &vv2);
	return res;
}

static int test_sm_double_rotation()
{
	const size_t test_elems = 15;/* 18;*/
	sm_t *m = sm_alloc(SM_I32I32, test_elems);
	sv_t *kv = NULL, *vv = NULL;
	sv_t *kv2 = NULL, *vv2 = NULL;
	int res = m ? 0 : 1;
	ssize_t i;
	for (;;) {
		/*
		 * Add elements
		 */
		for (i = test_elems; i > 0; i--) {
			if (!sm_ii32_insert(&m, (int)i, (int)-i) ||
			    !st_assert((st_t *)m)) {
				res = 2;
				break;
			}
		}
		if (res)
			break;
		/*
		 * Delete pair elements (it triggers double-rotation cases in the tree code of map implementation)
		 */
		for (i = test_elems; i > 0; i--) {
			if ((i % 2))
				continue;
			if (!sm_i_delete(m, i) || !st_assert((st_t *)m)) {
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
			if (!sm_ii32_insert(&m, (int)i, (int)-i) ||
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
			if (!sm_i_delete(m, i)) {
				res = 5;
				break;
			}
			if (!st_assert((st_t *)m))
			{
				res = 50;
				break;
			}
		}
		if (res)
			break;
		if (!st_assert((st_t *)m))
		{
			res = 100;
			break;
		}
		break;
	}
	sm_free(&m);
	sv_free(&kv, &vv, &kv2, &vv2);
	return res;
}

static int test_sdm_alloc()
{
	sdm_t *dm = sdm_alloc(SM_IntInt, 4, 1);
	RETURN_IF(!dm, 1);
	sm_t **submaps = sdm_submaps(dm);
	sint_t c = 0;
	size_t nelems = 1000000;
	for (; c < (sint_t)nelems; c++) {
		const size_t route = sdm_i_route(dm, c);
		sm_ii_insert(&submaps[route], c, c);
	}
	size_t sdmsz = sdm_size(dm);
	size_t nelems_check = 0;
	for (c = 0; c < (sint_t)sdmsz; c++)
		nelems_check += sm_size(submaps[c]);
	sdm_free(&dm);
	return nelems == nelems_check ? 0 : 2;
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
	char ac[sizeof(suint32_t)] = { 0 }, bc[sizeof(suint32_t)] = { 0 },
	     cc[sizeof(size_t)], dd[sizeof(suint64_t)];
	unsigned a = 0x12345678, b = 0x87654321;
	size_t c = ((size_t)-1) & a;
	suint64_t d = (suint64_t)a << 32 | b;
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
	return 0; /* TODO */
}

/*
 * Test execution
 */

int main()
{
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
	STEST_START;
	STEST_ASSERT(test_ss_alloc(0));
	STEST_ASSERT(test_ss_alloc(16));
	STEST_ASSERT(test_ss_alloca(32));
	STEST_ASSERT(test_ss_shrink());
#ifdef SD_ENABLE_HEURISTIC_GROW
	unsigned expected_low = 1;
#else
	unsigned expected_low = 0;
#endif
	STEST_ASSERT(test_ss_grow(100, 501, expected_low));
	STEST_ASSERT(test_ss_grow(100, 506, expected_low));
	STEST_ASSERT(test_ss_grow(100, 507, 1));
	STEST_ASSERT(test_ss_grow(100, 508, 1));
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
	STEST_ASSERT(test_ss_erase_u("\xc3\x91" "a" "\xc3\x91", 1, 2, "\xc3\x91"));
	STEST_ASSERT(test_ss_erase_u("\xc3\x91" "a" "\xc3\x91", 2, 1, "\xc3\x91" "a"));
	STEST_ASSERT(test_ss_erase_u("\xc3\x91" "a" "\xc3\x91", 2, 1000, "\xc3\x91" "a"));
	STEST_ASSERT(test_ss_erase_u("\xc3\x91" "\xc3\x91" "a", 1, 2, "\xc3\x91"));
	STEST_ASSERT(test_ss_erase_u("\xc3\x91" "\xc3\x91" "a", 2, 1, "\xc3\x91" "\xc3\x91"));
	STEST_ASSERT(test_ss_erase_u("\xc3\x91" "\xc3\x91" "a", 2, 1000, "\xc3\x91" "\xc3\x91"));
	STEST_ASSERT(test_ss_erase_u("\xc3\x91" "\xc3\x91" "a", 0, 1, "\xc3\x91" "a"));
	STEST_ASSERT(test_ss_free(0));
	STEST_ASSERT(test_ss_free(16));
	STEST_ASSERT(test_ss_len("hello", 5));
	STEST_ASSERT(test_ss_len_u("hello\xc3\x91", 6));
	STEST_ASSERT(test_ss_capacity());
	STEST_ASSERT(test_ss_len_left());
	STEST_ASSERT(test_ss_max());
	STEST_ASSERT(test_ss_dup());
	STEST_ASSERT(test_ss_dup_sub());
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

#define MK_TEST_SS_DUP_CPY_CAT(encc, decc, a, b)		\
	STEST_ASSERT(test_ss_dup_##encc(a, b));			\
	STEST_ASSERT(test_ss_cpy_##encc(a, b)); 		\
	STEST_ASSERT(test_ss_cat_##encc("abc", a, "abc" b));	\
	STEST_ASSERT(test_ss_cat_##encc("ABC", a, "ABC" b));	\
	STEST_ASSERT(test_ss_dup_##decc(b, a));			\
	STEST_ASSERT(test_ss_cpy_##decc(b, a)); 		\
	STEST_ASSERT(test_ss_cat_##decc("abc", b, "abc" a));	\
	STEST_ASSERT(test_ss_cat_##decc("ABC", b, "ABC" a));

	MK_TEST_SS_DUP_CPY_CAT(tolower, toupper, "HELLO", "hello");
	MK_TEST_SS_DUP_CPY_CAT(enc_b64, dec_b64, "0123456789ABCDEF", "MDEyMzQ1Njc4OUFCQ0RFRg==");
	MK_TEST_SS_DUP_CPY_CAT(enc_b64, dec_b64, "01", "MDE=");
	MK_TEST_SS_DUP_CPY_CAT(enc_hex, dec_hex, "\xff\xff", "ffff");
	MK_TEST_SS_DUP_CPY_CAT(enc_hex, dec_hex, "01z", "30317a");
	MK_TEST_SS_DUP_CPY_CAT(enc_HEX, dec_hex, "0123456789ABCDEF", "30313233343536373839414243444546");
	MK_TEST_SS_DUP_CPY_CAT(enc_HEX, dec_hex, "\xff\xff", "FFFF");
	MK_TEST_SS_DUP_CPY_CAT(enc_HEX, dec_hex, "01z", "30317A");
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_xml, dec_esc_xml, "hi\"&'<>there", "hi&quot;&amp;&apos;&lt;&gt;there");
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_json, dec_esc_json, "\b\t\f\n\r\"\x5c", "\\b\\t\\f\\n\\r\\\"\x5c\x5c");
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_url, dec_esc_url, "0189ABCXYZ-_.~abcxyz \\/&!?<>",
					    "0189ABCXYZ-_.~abcxyz%20%5C%2F%26%21%3F%3C%3E");
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_dquote, dec_esc_dquote, "\"how\" are you?", "\"\"how\"\" are you?");
	MK_TEST_SS_DUP_CPY_CAT(enc_esc_squote, dec_esc_squote, "'how' are you?", "''how'' are you?");

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
	if (sizeof(wchar_t) > 2) {
		STEST_ASSERT(test_ss_dup_char(0x24b62, "\xf0\xa4\xad\xa2"));
	}
	STEST_ASSERT(test_ss_dup_read("abc"));
	STEST_ASSERT(test_ss_dup_read("a\nb\tc\rd\te\ff"));
	STEST_ASSERT(test_ss_cpy(""));
	STEST_ASSERT(test_ss_cpy("hello"));
	STEST_ASSERT(test_ss_cpy_cn());
	STEST_ASSERT(test_ss_cpy_sub());
	STEST_ASSERT(test_ss_cpy_substr());
	STEST_ASSERT(test_ss_cpy_substr_u());
	STEST_ASSERT(test_ss_cpy_c(""));
	STEST_ASSERT(test_ss_cpy_c("hello"));
	STEST_ASSERT(test_ss_cpy_w(L"hello", "hello"));
	STEST_ASSERT(test_ss_cpy_int(0, "0"));
	STEST_ASSERT(test_ss_cpy_int(1, "1"));
	STEST_ASSERT(test_ss_cpy_int(-1, "-1"));
	STEST_ASSERT(test_ss_cpy_int(2147483647, "2147483647"));
	STEST_ASSERT(test_ss_cpy_int(-2147483647, "-2147483647"));
	STEST_ASSERT(test_ss_cpy_int(9223372036854775807LL, "9223372036854775807"));
	STEST_ASSERT(test_ss_cpy_int(-9223372036854775807LL, "-9223372036854775807"));
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
	if (sizeof(wchar_t) > 2) {
		STEST_ASSERT(test_ss_cpy_char(0x24b62, "\xf0\xa4\xad\xa2"));
	}
	STEST_ASSERT(test_ss_cat("hello", "all"));
	STEST_ASSERT(test_ss_cat_sub());
	STEST_ASSERT(test_ss_cat_substr());
	STEST_ASSERT(test_ss_cat_substr_u());
	STEST_ASSERT(test_ss_cat_cn());
	STEST_ASSERT(test_ss_cat_c("hello", "all"));
	char btmp1[400], btmp2[400];
	memset(btmp1, '0', sizeof(btmp1));
	memset(btmp2, '1', sizeof(btmp2));
	btmp1[sizeof(btmp1) - 1] = btmp2[sizeof(btmp2) - 1] = 0;
	STEST_ASSERT(test_ss_cat_c(btmp1, btmp2));
	STEST_ASSERT(test_ss_cat_wn());
	STEST_ASSERT(test_ss_cat_w(L"hello", L"all"));
	STEST_ASSERT(test_ss_cat_int("prefix", 1, "prefix1"));
	STEST_ASSERT(test_ss_cat_erase("x", "hello", 2, 2, "xheo"));
	STEST_ASSERT(test_ss_cat_erase_u());
	STEST_ASSERT(test_ss_cat_replace("x", "hello", "ll", "*LL*", "xhe*LL*o"));
	STEST_ASSERT(test_ss_cat_resize());
	STEST_ASSERT(test_ss_cat_resize_u());
	STEST_ASSERT(test_ss_cat_trim("aaa", " hello ", "aaahello"));
	STEST_ASSERT(test_ss_cat_ltrim("aaa", " hello ", "aaahello "));
	STEST_ASSERT(test_ss_cat_rtrim("aaa", " hello ", "aaa hello"));
	STEST_ASSERT(test_ss_cat_printf());
	STEST_ASSERT(test_ss_cat_printf_va());
	STEST_ASSERT(test_ss_cat_char());
	STEST_ASSERT(test_ss_popchar());
	STEST_ASSERT(test_ss_tolower("aBcDeFgHiJkLmNoPqRsTuVwXyZ", "abcdefghijklmnopqrstuvwxyz"));
	STEST_ASSERT(test_ss_toupper("aBcDeFgHiJkLmNoPqRsTuVwXyZ", "ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
#if !defined(S_MINIMAL_BUILD)
	STEST_ASSERT(test_ss_tolower("\xc3\x91", "\xc3\xb1"));
	STEST_ASSERT(test_ss_toupper("\xc3\xb1", "\xc3\x91"));
	STEST_ASSERT(!ss_set_turkish_mode(1));
	STEST_ASSERT(test_ss_tolower("I", "\xc4\xb1")); /* 0x49 -> 0x131 */
	STEST_ASSERT(test_ss_tolower("III", "\xc4\xb1" "\xc4\xb1" "\xc4\xb1"));
	STEST_ASSERT(test_ss_toupper("\xc4\xb1", "I"));
	STEST_ASSERT(test_ss_tolower("\xc4\xb0", "i")); /* 0x130 -> 0x69 */
	STEST_ASSERT(test_ss_toupper("i", "\xc4\xb0"));
	STEST_ASSERT(test_ss_tolower("\xc4\x9e", "\xc4\x9f")); /* 0x11e -> 0x11f */
	STEST_ASSERT(test_ss_toupper("\xc4\x9f", "\xc4\x9e"));
	STEST_ASSERT(test_ss_tolower("\xc5\x9e", "\xc5\x9f")); /* 0x15e -> 0x15f */
	STEST_ASSERT(test_ss_toupper("\xc5\x9f", "\xc5\x9e"));
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
#if !defined(S_NOT_UTF8_SPRINTF)
	if (unicode_support)
		STEST_ASSERT(test_ss_to_w("hello\xc3\x91"));
#endif
	STEST_ASSERT(test_ss_find("full text", "text", 5));
	STEST_ASSERT(test_ss_find("full text", "hello", S_NPOS));
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
	STEST_ASSERT(test_ss_read());
	STEST_ASSERT(test_ss_csum32());
	STEST_ASSERT(test_ss_null());
	/*                          $       cent        euro            chinese             N~          n~         */
	const char *utf8[] = { "a", "\x24", "\xc2\xa2", "\xe2\x82\xac", "\xf0\xa4\xad\xa2", "\xc3\x91", "\xc3\xb1" };
	const sint32_t uc[] = { 'a', 0x24, 0xa2, 0x20ac, 0x24b62, 0xd1, 0xf1 };
	unsigned i = 0;
	for (; i < sizeof(utf8) / sizeof(utf8[0]); i++) {
		STEST_ASSERT(test_sc_utf8_to_wc(utf8[i], uc[i]));
		STEST_ASSERT(test_sc_wc_to_utf8(uc[i], utf8[i]));
	}
	/*
	 * Windows require the specific locale for doing case conversions properly
	 */
#if !defined(_MSC_VER) && !defined(__CYGWIN__) && !defined(S_MINIMAL_BUILD)
	if (unicode_support) {
		const size_t wchar_range = sizeof(wchar_t) == 2 ? 0xd7ff : 0x3fffff;
		size_t test_tolower = 0;
		for (i = 0; i <= wchar_range; i++) {
			unsigned l0 = towlower(i), l = sc_tolower(i);
			if (l != l0) {
				fprintf(stderr, "%x %x [%x system reported]\n", i, l, l0);
				test_tolower++;
			}
		}
		STEST_ASSERT(test_tolower);
		size_t test_toupper = 0;
		for (i = 0; i <= wchar_range; i++) {
			unsigned u0 = towupper(i), u = sc_toupper(i);
			if (u != u0) {
				fprintf(stderr, "%x %x [%x system reported]\n", i, u, u0);
				test_toupper++;
			}
		}
		STEST_ASSERT(test_toupper);
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
	STEST_ASSERT(test_sv_len_left());
	STEST_ASSERT(test_sv_set_len());
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
	STEST_ASSERT(test_sv_find());
	STEST_ASSERT(test_sv_push_pop());
	STEST_ASSERT(test_sv_push_pop_i());
	STEST_ASSERT(test_sv_push_pop_u());
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
	STEST_ASSERT(test_sm_is_at());
	STEST_ASSERT(test_sm_ip_at());
	STEST_ASSERT(test_sm_si_at());
	STEST_ASSERT(test_sm_ss_at());
	STEST_ASSERT(test_sm_u_count());
	STEST_ASSERT(test_sm_i_count());
	STEST_ASSERT(test_sm_s_count());
	STEST_ASSERT(test_sm_sp_at());
	STEST_ASSERT(test_sm_ii32_insert());
	STEST_ASSERT(test_sm_uu32_insert());
	STEST_ASSERT(test_sm_ii_insert());
	STEST_ASSERT(test_sm_is_insert());
	STEST_ASSERT(test_sm_si_insert());
	STEST_ASSERT(test_sm_ss_insert());
	STEST_ASSERT(test_sm_i_delete());
	STEST_ASSERT(test_sm_s_delete());
	STEST_ASSERT(test_sm_enum());
	STEST_ASSERT(test_sm_sort_to_vectors());
	STEST_ASSERT(test_sm_double_rotation());
	/*
	 * Low level stuff
	 */
	STEST_ASSERT(test_endianess());
	STEST_ASSERT(test_alignment());
	STEST_ASSERT(test_sbitio());
	/*
	 * Distributed map
	 */
	STEST_ASSERT(test_sdm_alloc());
	/*
	 * Report
	 */
#ifdef S_DEBUG
	fprintf(stderr, "SS_RANGE_SMALL: %u, SS_RANGE_FULL: " FMT_ZU "\n",
		(unsigned)SS_RANGE_SMALL, (FMTSZ_T)SS_RANGE_FULL);
	fprintf(stderr, "SSTR: %u, SSTR_Small: %u, SSTR_Full: %u\n",
		(unsigned)sizeof(ss_t), (unsigned)sizeof(struct SSTR_Small),
		(unsigned)sizeof(struct SSTR_Full));
	fprintf(stderr, "Errors: %i\n", ss_errors);
#endif
	return STEST_END;
}


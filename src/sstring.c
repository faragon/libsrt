/*
 * sstring.c
 *
 * String handling.
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "sstring.h"
#include "saux/schar.h"
#include "saux/scommon.h"
#include "saux/senc.h"
#include "saux/shash.h"
#include "saux/ssearch.h"

/*
 * Togglable optimizations
 *
 * S_ENABLE_UTF8_7BIT_PARALLEL_CASE_OPTIMIZATIONS: ~20x faster
 * for UTF-8 characters below code 128 (ASCII). The only reason
 * for disabling it is if some bug appears.
 */

#define S_ENABLE_UTF8_7BIT_PARALLEL_CASE_OPTIMIZATIONS
#ifdef S_MINIMAL
#undef S_ENABLE_UTF8_7BIT_PARALLEL_CASE_OPTIMIZATIONS
#endif

/*
 * Macros
 */

#ifdef S_DEBUG
size_t dbg_cnt_alloc_calls = 0; /* debug alloc/realloc calls */
#endif

/*
 * Templates
 */

#define SS_CCAT_STACK 128

#define SS_COPYCAT_AUX_VARS(TYPE)                                              \
	va_list ap;                                                            \
	size_t *sizes, nargs = 0, extra_size, i, pstack[SS_CCAT_STACK],        \
		       *pheap = NULL;                                          \
	TYPE *next = s1

#define SS_COPYCAT_AUX(s, cat, TYPE, s1, STRLEN, SS_CAT_XN)                    \
	for (; s1;) {                                                          \
		va_start(ap, s1);                                              \
		while (!s_varg_tail_ptr_tag(next)) { /* last el. tag */        \
			next = (TYPE *)va_arg(ap, TYPE *);                     \
			nargs++;                                               \
		}                                                              \
		va_end(ap);                                                    \
		if (nargs <= SS_CCAT_STACK) {                                  \
			sizes = pstack;                                        \
		} else {                                                       \
			pheap = (size_t *)s_malloc(sizeof(size_t) * nargs);    \
			if (!pheap) {                                          \
				ss_set_alloc_errors(*s);                       \
				break;                                         \
			}                                                      \
			sizes = pheap;                                         \
		}                                                              \
		sizes[0] = STRLEN(s1);                                         \
		extra_size = sizes[0];                                         \
		va_start(ap, s1);                                              \
		for (i = 1; i < nargs; i++) {                                  \
			next = va_arg(ap, TYPE *);                             \
			if (next) {                                            \
				sizes[i] = STRLEN(next);                       \
				extra_size += sizes[i];                        \
			}                                                      \
		}                                                              \
		va_end(ap);                                                    \
		if ((cat && ss_grow(s, extra_size))                            \
		    || (!cat && ss_reserve(s, extra_size) >= extra_size)) {    \
			if (!cat)                                              \
				ss_clear(*s);                                  \
			SS_CAT_XN(s, s1, sizes[0]);                            \
			va_start(ap, s1);                                      \
			for (i = 1; i < nargs; i++) {                          \
				next = va_arg(ap, TYPE *);                     \
				if (!next)                                     \
					continue;                              \
				SS_CAT_XN(s, next, sizes[i]);                  \
			}                                                      \
			va_end(ap);                                            \
		} else {                                                       \
			if (extra_size > 0)                                    \
				ss_set_alloc_errors(*s);                       \
		}                                                              \
		if (pheap)                                                     \
			s_free(pheap);                                         \
		break;                                                         \
	}

#define SS_OVERFLOW_CHECK(s, at, inc)                                          \
	if (s_size_t_overflow(at, inc)) {                                      \
		if (*s) {                                                      \
			ss_set_alloc_errors(*s);                               \
			return *s;                                             \
		}                                                              \
		return ss_void;                                                \
	}

/*
 * Constants
 */

static srt_string ss_void0 = EMPTY_SS;
srt_string *ss_void = &ss_void0; /* empty string w/ alloc error set */

/*
 * Forward definitions for some static functions
 */

static srt_string *ss_reset(srt_string *s);

/*
 * Global variables (used only for Turkish mode)
 * (equivalent to work with Turkish locale when doing case conversions)
 */

static int32_t (*fsc_tolower)(int32_t c) = sc_tolower;
static int32_t (*fsc_toupper)(int32_t c) = sc_toupper;

/*
 * Internal functions
 */

static unsigned is_unicode_size_cached(const srt_string *s)
{
	return s ? s->d.f.flag1 : 0;
}

static unsigned has_encoding_errors(const srt_string *s)
{
	return s ? s->d.f.flag2 : 0;
}

static void set_unicode_size_cached(srt_string *s, srt_bool is_cached)
{
	if (s)
		s->d.f.flag1 = is_cached;
}

static void set_encoding_errors(srt_string *s, srt_bool has_errors)
{
	if (s)
		s->d.f.flag2 = has_errors;
}

static void set_reference_mode(srt_string *s, srt_bool is_reference,
			       srt_bool has_C_terminator)
{
	if (s) {
		s->d.f.flag3 = is_reference;
		s->d.f.flag4 = has_C_terminator;
	}
}

S_INLINE size_t get_unicode_size(const srt_string *s)
{
	return !s ? 0
		  : sdx_full_st(&s->d) ? s->unicode_size
				       : ((const struct SDataSmall *)s)->aux;
}

S_INLINE size_t get_str_off(const srt_string *s)
{
	S_ASSERT(ss_get_buffer_r(s) >= (const char *)s);
	return (size_t)(ss_get_buffer_r(s) - (const char *)s);
}

S_INLINE void inc_size(srt_string *s, size_t inc_size)
{
	ss_set_size(s, s_size_t_add(ss_size(s), inc_size, S_NPOS));
}

static void set_unicode_size(srt_string *s, size_t unicode_size)
{
	if (s) {
		if (!is_unicode_size_cached(s))
			return;
		if (sdx_full_st(&s->d))
			s->unicode_size = unicode_size;
		else
			((struct SDataSmall *)s)->aux =
				sdx_szt_to_u8_sat(unicode_size);
	}
}

static void inc_unicode_size(srt_string *s, size_t incr_size)
{
	set_unicode_size(s,
			 s_size_t_add(get_unicode_size(s), incr_size, S_NPOS));
}

static void dec_unicode_size(srt_string *s, size_t dec_size)
{
	size_t ss = get_unicode_size(s);
	S_ASSERT(ss >= dec_size || !is_unicode_size_cached(s));
	if (ss >= dec_size) {
		set_unicode_size(s, ss - dec_size);
	} else { /* This should never happen */
		set_unicode_size(s, 0);
		set_encoding_errors(s, S_TRUE);
	}
}

static srt_string *ss_reset(srt_string *s)
{
	if (s) { /* do not change 'ext_buffer' */
		set_unicode_size_cached(s, S_TRUE);
		set_encoding_errors(s, S_FALSE);
		set_unicode_size(s, 0);
		ss_set_size(s, 0);
	}
	return s;
}

size_t ss_reserve(srt_string **s, size_t max_size)
{
	srt_bool full_st;
	size_t ss, unicode_size, r;
	RETURN_IF(!s, 0);
	if (!*s) {
		*s = ss_alloc(max_size);
		RETURN_IF(!(*s), 0);
		ss = ss_max_size(*s);
		return ss >= max_size ? max_size : 0;
	}
	/*
	 * If passing from small struct to full struct,
	 * we'll have to update the unicode cached size.
	 */
	unicode_size = get_unicode_size(*s);
	full_st = sdx_full_st(&(*s)->d);
	r = sdx_reserve((srt_data **)s, max_size, sizeof(srt_string), 1);
	if (!full_st && r > 255)
		set_unicode_size(*s, unicode_size);
	return r;
}

size_t ss_grow(srt_string **s, size_t extra_size)
{
	srt_bool full_st;
	size_t size, unicode_size, new_size;
	RETURN_IF(!s, 0);
	RETURN_IF(!(*s), ss_reserve(s, extra_size));
	size = ss_size(*s);
	if (s_size_t_overflow(size, extra_size)) {
		ss_set_alloc_errors(*s);
		return 0;
	}
	/*
	 * If passing from small struct to full struct,
	 * we'll have to update the unicode cached size.
	 */
	unicode_size = get_unicode_size(*s);
	full_st = sdx_full_st(&(*s)->d);
	new_size = sdx_reserve((srt_data **)s, size + extra_size,
			       sizeof(srt_string), 1);
	if (!full_st && new_size > 255)
		set_unicode_size(*s, unicode_size);
	return new_size >= (size + extra_size) ? (new_size - size) : 0;
}

/* BEHAVIOR: aliasing is supported, e.g. append(&a, a) */
static srt_string *ss_cat_cn_raw(srt_string **s, const char *src,
				 size_t src_off, size_t src_size,
				 size_t src_usize)
{
	size_t off;
	ASSERT_RETURN_IF(!s, ss_void);
	if (src && src_size > 0) {
		off = *s ? ss_size(*s) : 0;
		if (ss_grow(s, src_size) && *s) {
			memmove(ss_get_buffer(*s) + off, src + src_off,
				src_size);
			inc_size(*s, src_size);
			if (is_unicode_size_cached(*s)) {
				if (src_usize > 0)
					inc_unicode_size(*s, src_usize);
				else
					set_unicode_size_cached(*s, S_FALSE);
			}
		}
	}
	return ss_check(s);
}

static srt_string *ss_cat_aliasing(srt_string **s, const srt_string *s0,
				   size_t s0_size, size_t s0_unicode_size,
				   const srt_string *src)
{
	return src == s0 ? ss_cat_cn_raw(s, ss_get_buffer(*s), 0, s0_size,
					 s0_unicode_size)
			 : ss_cat_cn_raw(s, ss_get_buffer_r(src), 0,
					 ss_size(src), get_unicode_size(src));
}

static size_t get_cmp_size(const srt_string *s1, const srt_string *s2)
{
	size_t s1_size, s2_size;
	if (!s1 || !s2)
		return 0;
	s1_size = ss_size(s1);
	s2_size = ss_size(s2);
	return S_MAX(s1_size, s2_size);
}

static size_t ss_utf8_to_wc(const char *s, size_t off, size_t max_off,
			    int32_t *unicode_out,
			    srt_string *s_unicode_error_out)
{
	size_t encoding_errors = 0;
	size_t csize =
		sc_utf8_to_wc(s, off, max_off, unicode_out, &encoding_errors);
	if (encoding_errors && s_unicode_error_out) {
		S_ERROR("broken UTF8");
		set_encoding_errors(s_unicode_error_out, S_TRUE);
	}
	return csize;
}

static srt_string *aux_toint(srt_string **s, srt_bool cat, int64_t num)
{
	int64_t n;
	char *p, btmp[128];
	size_t off, digits, at, out_size;
	ASSERT_RETURN_IF(!s, ss_void);
	p = btmp + sizeof(btmp) - 1;
	n = num < 0 ? -num : num;
	do {
		*p-- = '0' + n % 10;
		n /= 10;
	} while (n);
	if (num < 0)
		*p-- = '-';
	off = (size_t)((p - (char *)btmp) + 1);
	digits = sizeof(btmp) - off;
	at = (cat && *s) ? ss_size(*s) : 0;
	SS_OVERFLOW_CHECK(s, at, digits);
	out_size = s_size_t_add(at, digits, S_NPOS);
	if (ss_reserve(s, out_size) >= out_size && *s) {
		if (!cat)
			ss_reset(*s);
		memcpy(ss_get_buffer(*s) + at, btmp + off, digits);
		ss_set_size(*s, out_size);
		inc_unicode_size(*s, digits);
	}
	return *s;
}

static srt_string *aux_toXcase(srt_string **s, srt_bool cat,
			       const srt_string *src, int32_t (*towX)(int32_t))
{
	int c, c2;
	ssize_t extra;
	const char *ps;
	srt_string *out;
	srt_bool aliasing, is_cached_usize;
	char *po0, *po, *pout, u8[SSU8_MAX_SIZE];
	size_t ss, sso_max, cached_usize, at, at_ss, sso_req, i, csize, csize2;
#ifdef S_ENABLE_UTF8_7BIT_PARALLEL_CASE_OPTIMIZATIONS
	size_t i2;
#endif
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	ss = ss_size(src);
	sso_max = *s ? ss_max_size(*s) : 0;
	ps = ss_get_buffer_r(src);
	out = NULL;
	aliasing = *s == src;
	is_cached_usize = S_FALSE;
	extra = sc_utf8_calc_case_extra_size(ps, 0, ss, towX);
	cached_usize = 0;
	/* If possible, keep Unicode size cached: */
	if (*s) {
		if (is_unicode_size_cached(src)) {
			cached_usize = get_unicode_size(src);
			if (!cat) {
				is_cached_usize = S_TRUE;
			} else if (is_unicode_size_cached(*s)) {
				is_cached_usize = S_TRUE;
				cached_usize += get_unicode_size(*s);
			}
		}
		at = cat ? ss_size(*s) : 0;
	} else { /* copy */
		if (is_unicode_size_cached(src)) {
			is_cached_usize = S_TRUE;
			cached_usize = get_unicode_size(src);
		}
		at = 0;
	}
	/* Check if it is necessary to allocate more memory: */
	at_ss = s_size_t_add(at, ss, S_NPOS);
	sso_req = extra < 0 ? s_size_t_sub(at_ss, (size_t)(-extra))
			    : s_size_t_add(at_ss, (size_t)extra, S_NPOS);
	if (!*s || sso_req > sso_max || (aliasing && extra > 0)) {
		if (*s && (*s)->d.f.ext_buffer) { /* BEHAVIOR */
			S_ERROR("not enough memory: strings stored into a "
				"fixed-length buffer can not be resized.");
			ss_set_alloc_errors(*s);
			return ss_check(s);
		}
		out = ss_alloc(sso_req);
		if (!out) { /* BEHAVIOR */
			S_ERROR("not enough memory: can not "
				"change character case");
			if (*s)
				ss_set_alloc_errors(*s);
			return ss_check(s);
		}
		pout = ss_get_buffer(out);
		if (at > 0) /* cat */
			memcpy(pout, ss_get_buffer(*s), at);
		po0 = pout + at;
	} else {
		po0 = ss_get_buffer(*s) + at;
	}
	/* Case conversion loop: */
	i = 0;
	c = 0;
	po = po0;
	for (; i < ss;) {
#ifdef S_ENABLE_UTF8_7BIT_PARALLEL_CASE_OPTIMIZATIONS
		/*
		 * Alignment is handled into sc_parallel_toX(),
		 * so the cast to 32-bit container is not a problem.
		 */
		if (fsc_tolower == sc_tolower) { // because it doesn't apply for
						 // Turkish mode
			i2 = sc_parallel_toX(ps, i, ss, po, towX);
			if (i != i2) {
				po += (i2 - i);
				i = i2;
				if (i >= ss)
					break;
			}
		}
#endif
		csize = ss_utf8_to_wc(ps, i, ss, &c, *s);
		c2 = towX(c);
		if (c2 == c) {
			csize2 = csize;
			memmove(po, ps + i, csize2);
		} else {
			csize2 = sc_wc_to_utf8(c2, u8, 0, SSU8_MAX_SIZE);
			memcpy(po, u8, csize2);
		}
		i += csize;
		po += csize2;
	}
	if (out) { /* Case of using a secondary string was required */
		srt_string *s_bck = *s;
		*s = out;
		ss_free(&s_bck);
	}
	if (*s) {
		ss_set_size(*s, sso_req);
		set_unicode_size_cached(*s, is_cached_usize);
		set_unicode_size(*s, cached_usize);
	}
	return ss_check(s);
}

/*
 * The conversion runs backwards in order to cover the
 * aliasing case without extra memory allocation nor shift.
 */

static srt_string *aux_toenc(srt_string **s, srt_bool cat,
			     const srt_string *src, srt_enc_f f, srt_enc_f2 f2)
{
	srt_bool aliasing;
	srt_string *src_aux;
	unsigned char *s_out;
	const srt_string *src1;
	const unsigned char *src_buf, *s_in;
	size_t in_size, at, enc_size, out_size;
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	aliasing = *s == src ? S_TRUE : S_FALSE;
	src_buf = (const unsigned char *)ss_get_buffer_r(src);
	in_size = ss_size(src);
	at = (cat && *s) ? ss_size(*s) : 0;
	enc_size = f ? f(src_buf, in_size, NULL)
		     : f2 ? f2(src_buf, in_size, NULL, 0) : 0;
	out_size = s_size_t_add(at, enc_size, S_NPOS);
	if (enc_size > 0 && ss_reserve(s, out_size) >= out_size) {
		src_aux = NULL;
		if (aliasing) {
			/*
			 * For functions not supporting aliasing, use a
			 * copy for the input
			 */
			if (f == senc_lz || f == sdec_lz || f == senc_lzh) {
				ss_cpy(&src_aux, *s);
				src1 = src_aux;
			} else
				src1 = *s;
		} else {
			src1 = src;
		}
		s_in = (const unsigned char *)ss_get_buffer_r(src1);
		s_out = (unsigned char *)ss_get_buffer(*s) + at;
		enc_size = f ? f(s_in, in_size, s_out)
			     : f2(s_in, in_size, s_out, enc_size);
		if (at == 0) {
			set_unicode_size_cached(*s, S_TRUE);
			set_unicode_size(*s, in_size * 2);
		} else { /* cat */
			if (is_unicode_size_cached(*s) && at == ss_size(*s))
				set_unicode_size(*s, get_unicode_size(*s)
							     + in_size * 2);
			else
				set_unicode_size_cached(*s, S_FALSE);
		}
		out_size = at + enc_size;
		ss_set_size(*s, out_size);
		if (src_aux)
			ss_free(&src_aux);
		set_unicode_size_cached(*s, S_FALSE);
	}
	return ss_check(s);
}

static srt_string *aux_erase(srt_string **s, srt_bool cat,
			     const srt_string *src, size_t off, size_t n)
{
	char *po;
	srt_bool overflow;
	size_t ss0, at, src_size, copy_size, out_size;
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	ss0 = ss_size(src);
	at = (cat && *s) ? ss_size(*s) : 0;
	overflow = off + n > ss0;
	src_size = overflow ? ss0 - off : n;
	copy_size = ss0 - off - src_size;
	if (*s == src) {	      /* BEHAVIOR: aliasing: copy-only */
		if (off + n >= ss0) { /* tail clean cut */
			ss_set_size(*s, off);
		} else {
			char *ps = ss_get_buffer(*s);
			memmove(ps + off, ps + off + n, copy_size);
			ss_set_size(*s, ss0 - n);
		}
		set_unicode_size_cached(*s, S_FALSE);
	} else { /* copy or cat */
		out_size = at + off + copy_size;
		if (ss_reserve(s, out_size) >= out_size && *s) {
			po = ss_get_buffer(*s);
			memcpy(po + at, ss_get_buffer_r(src), off);
			memcpy(po + at + off, ss_get_buffer_r(src) + off + n,
			       copy_size);
			ss_set_size(*s, out_size);
			set_unicode_size_cached(*s, S_FALSE);
		}
	}
	return ss_check(s);
}

static srt_string *aux_erase_u(srt_string **s, srt_bool cat,
			       const srt_string *src, size_t char_off, size_t n)
{
	char *po;
	const char *ps;
	size_t sso0, ss0, head_size, actual_n, cus, cut_size, tail_size,
		out_size, prefix_usize, at;
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	ps = ss_get_buffer_r(src);
	sso0 = *s ? ss_size(*s) : 0;
	ss0 = ss_size(src);
	head_size = sc_unicode_count_to_utf8_size(ps, 0, ss0, char_off, NULL);
	RETURN_IF(head_size >= ss0, ss_check(s)); /* BEHAVIOR */
	actual_n = 0;
	cus = *s ? get_unicode_size(*s) : 0;
	cut_size =
		sc_unicode_count_to_utf8_size(ps, head_size, ss0, n, &actual_n);
	tail_size = ss0 - cut_size - head_size;
	out_size = ss0 - cut_size;
	prefix_usize = 0;
	if (*s == src) { /* aliasing: copy-only */
		po = ss_get_buffer(*s);
		memmove(po + head_size, ps + head_size + cut_size, tail_size);
	} else { /* copy/cat */
		at = (cat && *s) ? ss_size(*s) : 0;
		out_size += at;
		if (ss_reserve(s, out_size) >= out_size && *s) {
			po = ss_get_buffer(*s);
			memcpy(po + at, ss_get_buffer_r(src), head_size);
			memcpy(po + at + head_size,
			       ss_get_buffer_r(src) + head_size + cut_size,
			       tail_size);
			ss_set_size(*s, out_size);
			if (is_unicode_size_cached(*s)
			    && (at == 0 || at == sso0)) {
				prefix_usize = get_unicode_size(*s);
			} else {
				set_unicode_size_cached(*s, S_FALSE);
			}
		}
	}
	if (cus > actual_n)
		set_unicode_size(*s, prefix_usize + cus - actual_n);
	else /* BEHAVIOR: unicode char count invalidation */
		set_unicode_size_cached(*s, S_FALSE);
	ss_set_size(*s, out_size);
	return ss_check(s);
}

static srt_string *aux_replace(srt_string **s, srt_bool cat,
			       const srt_string *src, size_t off,
			       const srt_string *s1, const srt_string *s2)
{
	char *o, *o0;
	srt_string *out;
	srt_bool aliasing;
	ssize_t size_delta;
	const char *p0, *p2;
	size_t at, l1, l2, i, l, out_size, nfound, i_next;
	typedef void (*memcpy_t)(void *, const void *, size_t);
	memcpy_t f_cpy;
	ASSERT_RETURN_IF(!s, ss_void);
	if (!s1)
		s1 = ss_void;
	if (!s2)
		s2 = ss_void;
	if (!src)
		src = ss_void;
	at = (cat && *s) ? ss_size(*s) : 0;
	p0 = ss_get_buffer_r(src);
	p2 = ss_get_buffer_r(s2);
	RETURN_IF(!p0, ss_check(s)); /* BEHAVIOR: empty s1 */
	l1 = ss_size(s1);
	l2 = ss_size(s2);
	i = off;
	l = ss_size(src);
	out = NULL;
	size_delta = l2 > l1 ? (ssize_t)(l2 - l1) : -(ssize_t)(l1 - l2);
	aliasing = S_FALSE;
	out_size = at + l;
	if (l2 >= l1) { /* resize required */
		nfound = 0;
		/* scan required size */
		for (;; i += l1, nfound++)
			if ((i = ss_find(src, i, s1)) == S_NPOS)
				break;
		if (nfound == 0) /* 0 occurrences: return */
			return ss_check(s);
		if (size_delta >= 0)
			out_size += (size_t)size_delta * nfound;
		else
			out_size -= (size_t)(-size_delta) * nfound;
		/* allocate output string */
		out = ss_alloc(out_size);
		if (!out) {
			S_ERROR("not enough memory");
			ss_set_alloc_errors(*s);
			return ss_check(s);
		}
		o0 = o = ss_get_buffer(out);
		/* copy prefix data (cat) */
		if (at > 0)
			memcpy(o, ss_get_buffer_r(*s), at);
	} else {
		if (s && *s && *s == src) {
			aliasing = S_TRUE;
		} else {
			if (ss_reserve(s, out_size) < out_size) /* BEHAVIOR */
				return ss_check(s);
		}
		o0 = o = ss_get_buffer(*s);
		RETURN_IF(!o, ss_check(s));
	}
	if (aliasing) {
		f_cpy = (memcpy_t)memmove;
	} else {
		f_cpy = (memcpy_t)memcpy;
		o += at;
		if (off > 0) /* copy not affected data */
			memcpy(o, p0, off);
	}
	o += off;
	i_next = s1 == s2 ? S_NPOS : /* no replace */
			 ss_find(src, i + off, s1);
	for (i = off;;) {
		/* before match copy: */
		if (i_next == S_NPOS) {
			f_cpy(o, p0 + i, l - i);
			o += (l - i);
			break;
		}
		f_cpy(o, p0 + i, i_next - i);
		o += (i_next - i);
		/* replace: */
		if (p2)
			f_cpy(o, p2, l2);
		o += l2;
		i = i_next + l1;
		/* prepare next search: */
		i_next = ss_find(src, i, s1);
	}
	if (out) {
		srt_string *s_bck = *s;
		*s = out;
		ss_free(&s_bck);
	}
	ss_set_size(*s, (size_t)(o - o0));
	/* TODO: optimize cached size */
	set_unicode_size_cached(*s, S_FALSE);
	return *s;
}

static srt_string *aux_resize(srt_string **s, srt_bool cat,
			      const srt_string *src, size_t n, char fill_byte)
{
	srt_bool aliasing;
	size_t src_size, at, out_size;
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	src_size = ss_size(src);
	at = (cat && *s) ? ss_size(*s) : 0;
	out_size = s_size_t_add(at, n, S_NPOS); /* BEHAVIOR: on overflow */
	aliasing = *s == src;
	if (src_size < n) { /* fill */
		if (ss_reserve(s, out_size) >= out_size && *s) {
			char *o = ss_get_buffer(*s);
			if (!aliasing) {
				const char *p = ss_get_buffer_r(src);
				memcpy(o + at, p, src_size);
			}
			memset(o + at + src_size, fill_byte, n - src_size);
			ss_set_size(*s, out_size);
			set_unicode_size_cached(*s, S_FALSE);
			/* BEHAVIOR: size cache lost */
		} /* else: BEHAVIOR: not enough memory */
	} else {  /* else: cut (implicit) */
		if (ss_reserve(s, out_size) >= out_size && *s) {
			if (!aliasing)
				memcpy(ss_get_buffer(*s) + at,
				       ss_get_buffer_r(src), n);
			ss_set_size(*s, out_size);
			set_unicode_size_cached(*s, S_FALSE);
			/* BEHAVIOR: size cache lost */
		} /* else: BEHAVIOR: not enough memory */
	}
	return *s;
}

static srt_string *aux_resize_u(srt_string **s, srt_bool cat, srt_string *src,
				size_t u_chars, int fill_char)
{
	const char *ps;
	srt_bool aliasing;
	size_t at, char_size, current_u_chars, srcs, new_elems, at_inc,
		out_size, i, actual_unicode_count, head_size;
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	at = (cat && *s) ? ss_size(*s) : 0;
	char_size = sc_wc_to_utf8_size(fill_char);
	current_u_chars = ss_len_u(src);
	RETURN_IF(u_chars == current_u_chars, ss_check(s)); /* same */
	aliasing = *s == src;
	srcs = ss_size(src);
	if (current_u_chars < u_chars) { /* fill */
		new_elems = u_chars - current_u_chars;
		at_inc = srcs + new_elems * char_size;
		SS_OVERFLOW_CHECK(s, at, at_inc);
		out_size = at + at_inc;
		if (ss_reserve(s, out_size) >= out_size && *s) {
			if (!cat && !aliasing) /* copy */
				ss_clear(*s);
			if (!aliasing) {
				memcpy(ss_get_buffer(*s) + at,
				       ss_get_buffer_r(src), srcs);
				inc_unicode_size(*s, current_u_chars);
				inc_size(*s, srcs);
			}
			i = 0;
			for (; i < new_elems; i++)
				ss_cat_char(s, fill_char);
		}
	} else { /* cut */
		ps = ss_get_buffer_r(src);
		actual_unicode_count = 0;
		head_size = sc_unicode_count_to_utf8_size(
			ps, 0, srcs, u_chars, &actual_unicode_count);
		SS_OVERFLOW_CHECK(s, at, head_size);
		out_size = at + head_size;
		S_ASSERT(u_chars == actual_unicode_count);
		if (!aliasing) { /* copy or cat */
			if (ss_reserve(s, out_size) >= out_size && *s) {
				if (!cat && !aliasing) /* copy */
					ss_clear(*s);
				memcpy(ss_get_buffer(*s) + at, ps, head_size);
				inc_unicode_size(*s, actual_unicode_count);
				inc_size(*s, head_size);
			} /* else: BEHAVIOR */
		} else {  /* cut */
			ss_set_size(*s, head_size);
			set_unicode_size(*s, actual_unicode_count);
		}
	}
	return *s;
}

static srt_string *aux_ltrim(srt_string **s, srt_bool cat,
			     const srt_string *src)
{
	char *pt;
	const char *ps;
	srt_bool aliasing;
	size_t ss, at, cat_usize, i, out_size;
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	ss = ss_size(src);
	if (ss > 0) {
		aliasing = *s == src;
		ps = ss_get_buffer_r(src);
		for (i = 0; i < ss && isspace(ps[i]); i++)
			;
		if (cat) {
			if (*s) {
				at = ss_size(*s);
				cat_usize = get_unicode_size(*s);
			} else {
				at = cat_usize = 0;
			}
		} else {
			at = cat_usize = 0;
		}
		out_size = at + ss - i;
		if (ss_reserve(s, out_size) >= out_size && *s) {
			pt = ss_get_buffer(*s);
			if (!aliasing) /* copy or cat: shift data */
				memcpy(pt + at, ps + i, ss - i);
			else if (i > 0) /* copy: shift data */
				memmove(pt, ps + i, ss - i);
			ss_set_size(*s, at + ss - i);
			/* Set Unicode size only if can be computed for free */
			if (is_unicode_size_cached(*s)
			    && is_unicode_size_cached(src))
				set_unicode_size(
					*s,
					cat_usize + get_unicode_size(src) - i);
			else
				set_unicode_size_cached(*s, S_FALSE);
		} /* else: BEHAVIOR */
	} else {
		if (cat)
			ss_check(s);
		else
			ss_clear(*s);
	}
	return *s;
}

static srt_string *aux_rtrim(srt_string **s, srt_bool cat,
			     const srt_string *src)
{
	const char *ps;
	srt_bool aliasing;
	size_t ss, at, i, nspaces, copy_size, out_size, cat_usize;
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	ss = ss_size(src);
	at = (cat && *s) ? ss_size(*s) : 0;
	if (ss > 0) {
		aliasing = *s == src;
		ps = ss_get_buffer_r(src);
		i = ss - 1;
		for (; i > 0 && isspace(ps[i]); i--)
			;
		if (isspace(ps[i]))
			i--;
		nspaces = ss - i - 1;
		copy_size = ss - nspaces;
		out_size = at + copy_size;
		cat_usize = cat ? get_unicode_size(*s) : 0;
		if (ss_reserve(s, out_size) >= out_size && *s) {
			char *pt = ss_get_buffer(*s);
			if (!aliasing)
				memcpy(pt + at, ps, copy_size);
			ss_set_size(*s, out_size);
			/* Set Unicode size only if can be computed for free */
			if (is_unicode_size_cached(*s)
			    && is_unicode_size_cached(src))
				set_unicode_size(
					*s, cat_usize + get_unicode_size(src)
						    - nspaces);
			else
				set_unicode_size_cached(*s, S_FALSE);
		} /* else: BEHAVIOR */
	} else {
		if (cat)
			ss_check(s);
		else
			ss_clear(*s);
	}
	return *s;
}

static ssize_t aux_read(srt_string **s, srt_bool cat, FILE *h, size_t max_bytes)
{
	char *sc;
	ssize_t l = 0;
	size_t ss, off, max_off, def_buf, buf_size, cap, l0;
	if (h && max_bytes > 0) {
		ss = ss_size(*s);
		off = cat ? ss : 0;
		max_off = s_size_t_overflow(off, max_bytes) ? S_NPOS
							    : off + max_bytes;
		def_buf = 16384;
		for (; off < max_off;) {
			buf_size = !s_size_t_overflow(off, def_buf)
						   && off + def_buf < max_off
					   ? def_buf
					   : max_off - off;
			if (cat && (*s)->d.f.ext_buffer) {
				cap = ss_capacity_left(*s);
				buf_size = S_MIN(buf_size, cap);
			}
			if (ss_reserve(s, off + buf_size) >= off + buf_size) {
				if (feof(h))
					break;
				sc = ss_get_buffer(*s);
				l0 = (size_t)fread(sc + off, 1, buf_size, h);
				if (l0 > 0 && !ferror(h)) {
					off += l0;
					ss_set_size(*s, off);
					set_unicode_size_cached(*s, S_FALSE);
					l = l < 0 ? (ssize_t)l0
						  : l + (ssize_t)l0;
				} else {
					break;
				}
			} else { /* BEHAVIOR */
				S_ERROR("not enough memory");
				break;
			}
		}
	}
	if (l <= 0) {
		if (cat)
			ss_check(s);
		else
			ss_clear(*s);
	}
	return l;
}

	/*
	 * API functions
	 */

	/*
	 * Generated from template
	 */

#define MK_SS_DUP_CODEC(f, f_cpy)                                              \
	srt_string *f(const srt_string *src)                                   \
	{                                                                      \
		srt_string *s = NULL;                                          \
		return f_cpy(&s, src);                                         \
	}

#define MK_SS_CPY_CODEC(f, f_enc, f_enc2)                                      \
	srt_string *f(srt_string **s, const srt_string *src)                   \
	{                                                                      \
		return aux_toenc(s, S_FALSE, src, f_enc, f_enc2);              \
	}

#define MK_SS_CAT_CODEC(f, f_enc, f_enc2)                                      \
	srt_string *f(srt_string **s, const srt_string *src)                   \
	{                                                                      \
		return aux_toenc(s, S_TRUE, src, f_enc, f_enc2);               \
	}

#define MK_SS_CODEC(f, f_enc, f_enc2)                                          \
	srt_string *f(srt_string **s, const srt_string *src)                   \
	{                                                                      \
		return aux_toenc(s, S_FALSE, src, f_enc, f_enc2);              \
	}

#define MK_SS_DUP_CPY_CAT(suffix, f1, f2)                                      \
	MK_SS_DUP_CODEC(ss_dup_##suffix, ss_cpy_##suffix)                      \
	MK_SS_CPY_CODEC(ss_cpy_##suffix, f1, f2)                               \
	MK_SS_CAT_CODEC(ss_cat_##suffix, f1, f2)                               \
	MK_SS_CODEC(ss_##suffix, f1, f2)

MK_SS_DUP_CPY_CAT(enc_b64, senc_b64, NULL)
MK_SS_DUP_CPY_CAT(enc_hex, senc_hex, NULL)
MK_SS_DUP_CPY_CAT(enc_HEX, senc_HEX, NULL)
MK_SS_DUP_CPY_CAT(enc_lz, senc_lz, NULL)
MK_SS_DUP_CPY_CAT(enc_lzh, senc_lzh, NULL)
MK_SS_DUP_CPY_CAT(enc_esc_xml, NULL, senc_esc_xml)
MK_SS_DUP_CPY_CAT(enc_esc_json, NULL, senc_esc_json)
MK_SS_DUP_CPY_CAT(enc_esc_url, NULL, senc_esc_url)
MK_SS_DUP_CPY_CAT(enc_esc_dquote, NULL, senc_esc_dquote)
MK_SS_DUP_CPY_CAT(enc_esc_squote, NULL, senc_esc_squote)

MK_SS_DUP_CPY_CAT(dec_b64, sdec_b64, NULL)
MK_SS_DUP_CPY_CAT(dec_hex, sdec_hex, NULL)
MK_SS_DUP_CPY_CAT(dec_lz, sdec_lz, NULL)
MK_SS_DUP_CPY_CAT(dec_esc_xml, sdec_esc_xml, NULL)
MK_SS_DUP_CPY_CAT(dec_esc_json, sdec_esc_json, NULL)
MK_SS_DUP_CPY_CAT(dec_esc_url, sdec_esc_url, NULL)
MK_SS_DUP_CPY_CAT(dec_esc_dquote, sdec_esc_dquote, NULL)
MK_SS_DUP_CPY_CAT(dec_esc_squote, sdec_esc_squote, NULL)

/*
 * Allocation
 */

srt_string *ss_alloc(size_t initial_reserve)
{
	srt_string *s = ss_reset((srt_string *)sd_alloc(
		sizeof(srt_string), 1, initial_reserve, S_TRUE, 1));
	RETURN_IF(!s, ss_void);
	set_reference_mode(s, S_FALSE, S_FALSE);
	return s;
}

srt_string *ss_alloc_into_ext_buf(void *buf, size_t max_size)
{
	srt_string *s = (srt_string *)sd_alloc_into_ext_buf(
		buf, max_size, sizeof(srt_string), 1, S_TRUE);
	RETURN_IF(!s, ss_void);
	ss_reset((srt_string *)s);
	set_reference_mode(s, S_FALSE, S_FALSE);
	return s;
}

static const srt_string *aux_ss_ref_raw(srt_string_ref *s_ref, const char *buf,
					size_t buf_size,
					srt_bool has_C_terminator)
{
	RETURN_IF(!s_ref || !buf, ss_void); /* BEHAVIOR */
	memset(s_ref, 0, sizeof(*s_ref));
	s_ref->s.d.f.ext_buffer = 1;
	s_ref->s.d.f.st_mode = SData_Full;
	s_ref->cstr = buf;
	ss_set_size(&s_ref->s, buf_size);
	sd_set_max_size((srt_data *)&s_ref->s, buf_size);
	set_reference_mode(&s_ref->s, S_TRUE, has_C_terminator);
	return &s_ref->s;
}

const srt_string *ss_cref(srt_string_ref *s_ref, const char *c_str)
{
	return aux_ss_ref_raw(s_ref, c_str, strlen(c_str), S_TRUE);
}

const srt_string *ss_ref_buf(srt_string_ref *s_ref, const char *buf,
			     size_t buf_size)
{
	return aux_ss_ref_raw(s_ref, buf, buf_size, S_FALSE);
}

/*
 * Accessors
 */

int ss_at(const srt_string *s, size_t off)
{
	size_t ss;
	RETURN_IF(!s, 0);
	ss = ss_size(s);
	return off < ss ? ss_get_buffer_r(s)[off] : 0;
}

size_t ss_len_u(const srt_string *s)
{
	const char *p;
	srt_string *ws;
	size_t enc_errors, ss, cached_uc_size;
	ASSERT_RETURN_IF(!s, 0);
	if (is_unicode_size_cached(s))
		return get_unicode_size(s);
	/* Not cached, so cache it: */
	enc_errors = 0;
	p = ss_get_buffer_r(s);
	ss = ss_size(s);
	cached_uc_size = sc_utf8_count_chars(p, ss, &enc_errors);
	/*
	 * BEHAVIOR:
	 * Constness is kept regarding srt_string internal logical state. Said
	 * that, cached unicode size breaks the constness, as is an opaque
	 * operation. This is required for allowing caching string
	 * references (which are always 'const srt_string *', by construction)
	 */
	ws = (srt_string *)s;
	set_unicode_size_cached(ws, S_TRUE);
	set_unicode_size(ws, cached_uc_size);
	if (enc_errors) {
		/* BEHAVIOR:
		 * Unicode string length must be *always* less or equal to
		 * the UTF-8 number of bytes. Otherwise, it would mean an
		 * UTF-8 encoding error.
		 */
		set_encoding_errors(ws, S_TRUE);
	}
	return cached_uc_size;
}

size_t ss_max(const srt_string *s)
{
	return !s ? 0 : s->d.f.ext_buffer ? ss_max_size(s) : SS_RANGE;
}

S_INLINE size_t ss_real_off(const srt_string *s, size_t off)
{
	return off == S_NPOS ? ss_size(s) : off;
}

srt_bool ss_encoding_errors(const srt_string *s)
{
	return s && has_encoding_errors(s) ? S_TRUE : S_FALSE;
}

void ss_clear_errors(srt_string *s)
{
	if (s) {
		ss_reset_alloc_errors(s);
		set_encoding_errors(s, S_FALSE);
	}
}

/*
 * Allocation from a given source: s = ss_dup*
 * (equivalent to: s = NULL; ss_cpy*(&s, ...);)
 */

srt_string *ss_dup(const srt_string *src)
{
	srt_string *s = NULL;
	return ss_cpy(&s, src);
}

srt_string *ss_dup_substr(const srt_string *src, size_t off, size_t n)
{
	srt_string *s = NULL;
	return ss_cpy_substr(&s, src, off, n);
}

srt_string *ss_dup_substr_u(const srt_string *src, size_t char_off, size_t n)
{
	srt_string *s = NULL;
	return ss_cpy_substr_u(&s, src, char_off, n);
}

srt_string *ss_dup_cn(const char *src, size_t src_size)
{
	srt_string *s = NULL;
	return ss_cpy_cn(&s, src, src_size);
}

srt_string *ss_dup_c(const char *src)
{
	srt_string *s = NULL;
	return ss_cpy_c(&s, src);
}

srt_string *ss_dup_wn(const wchar_t *src, size_t src_size)
{
	srt_string *s = NULL;
	return ss_cpy_wn(&s, src, src_size);
}

srt_string *ss_dup_int(int64_t num)
{
	srt_string *s = NULL;
	return ss_cpy_int(&s, num);
}

srt_string *ss_dup_tolower(const srt_string *src)
{
	srt_string *s = NULL;
	return ss_cpy_tolower(&s, src);
}

srt_string *ss_dup_toupper(const srt_string *src)
{
	srt_string *s = NULL;
	return ss_cpy_toupper(&s, src);
}

srt_string *ss_dup_erase(const srt_string *src, size_t off, size_t n)
{
	srt_string *s = NULL;
	return aux_erase(&s, S_FALSE, src, off, n);
}

srt_string *ss_dup_erase_u(const srt_string *src, size_t char_off, size_t n)
{
	srt_string *s = NULL;
	return aux_erase_u(&s, S_FALSE, src, char_off, n);
}

srt_string *ss_dup_replace(const srt_string *src, size_t off,
			   const srt_string *s1, const srt_string *s2)
{
	srt_string *s = NULL;
	return aux_replace(&s, S_FALSE, src, off, s1, s2);
}

srt_string *ss_dup_resize(const srt_string *src, size_t n, char fill_byte)
{
	srt_string *s = NULL;
	return aux_resize(&s, S_FALSE, src, n, fill_byte);
}

srt_string *ss_dup_resize_u(srt_string *src, size_t n, int fill_char)
{
	srt_string *s = NULL;
	return aux_resize_u(&s, S_FALSE, src, n, fill_char);
}

srt_string *ss_dup_trim(const srt_string *src)
{
	srt_string *s = NULL;
	return ss_cpy_trim(&s, src);
}

srt_string *ss_dup_ltrim(const srt_string *src)
{
	srt_string *s = NULL;
	return aux_ltrim(&s, S_FALSE, src);
}

srt_string *ss_dup_rtrim(const srt_string *src)
{
	srt_string *s = NULL;
	return aux_rtrim(&s, S_FALSE, src);
}

srt_string *ss_dup_w(const wchar_t *src)
{
	srt_string *s = NULL;
	return ss_cpy_w(&s, src);
}

srt_string *ss_dup_printf(size_t size, const char *fmt, ...)
{
	srt_string *s = NULL;
	va_list ap;
	va_start(ap, fmt);
	ss_cpy_printf_va(&s, size, fmt, ap);
	va_end(ap);
	return s;
}

srt_string *ss_dup_printf_va(size_t size, const char *fmt, va_list ap)
{
	srt_string *s = NULL;
	return ss_cpy_printf_va(&s, size, fmt, ap);
}

srt_string *ss_dup_char(int c)
{
	srt_string *s = NULL;
	return ss_cpy_char(&s, c);
}

srt_string *ss_dup_read(FILE *handle, size_t max_bytes)
{
	srt_string *s = NULL;
	return ss_cpy_read(&s, handle, max_bytes);
}

/*
 * Assignment from a given source: ss_cpy*(s, ...)
 */

srt_string *ss_cpy(srt_string **s, const srt_string *src)
{
	RETURN_IF(!s, ss_void);
	RETURN_IF(*s == src && ss_check(s), *s); /* aliasing, same string */
	ss_clear(*s);
	RETURN_IF(!src, *s); /* BEHAVIOR: empty */
	return ss_cat(s, src);
}

srt_string *ss_cpy_substr(srt_string **s, const srt_string *src, size_t off,
			  size_t n)
{
	char *ps;
	size_t ss, copy_size;
	RETURN_IF(!s, ss_void);
	RETURN_IF(!src || !n, ss_reset(*s)); /* BEHAVIOR: empty */
	if (*s == src) {		     /* aliasing */
		ps = ss_get_buffer(*s);
		ss = ss_size(*s);
		RETURN_IF(off >= ss, ss_reset(*s)); /* BEHAVIOR: empty */
		copy_size = S_MIN(ss - off, n);
		memmove(ps, ps + off, copy_size);
		ss_set_size(*s, copy_size);
		/* BEHAVIOR: cache lost */
		set_unicode_size_cached(*s, S_FALSE);
	} else {
		ss_clear(*s);
		ss_cat_substr(s, src, off, n);
	}
	return ss_check(s);
}

srt_string *ss_cpy_substr_u(srt_string **s, const srt_string *src,
			    size_t char_off, size_t n)
{
	char *ps;
	size_t actual_unicode_count, ss, off, n_size, copy_size;
	RETURN_IF(!s, ss_void);
	RETURN_IF(!src || !n, ss_reset(*s)); /* BEHAVIOR: empty */
	if (*s == src) {		     /* aliasing */
		ps = ss_get_buffer(*s);
		actual_unicode_count = 0;
		ss = ss_size(*s);
		off = sc_unicode_count_to_utf8_size(ps, 0, ss, char_off,
						    &actual_unicode_count);
		n_size = sc_unicode_count_to_utf8_size(ps, off, ss, n,
						       &actual_unicode_count);
		RETURN_IF(off >= ss, ss_reset(*s)); /* BEHAVIOR: empty */
		copy_size = S_MIN(ss - off, n_size);
		memmove(ps, ps + off, copy_size);
		ss_set_size(*s, copy_size);
		if (n == actual_unicode_count) {
			set_unicode_size_cached(*s, S_TRUE);
			set_unicode_size(*s, n);
		} else { /* BEHAVIOR: cache lost */
			set_unicode_size_cached(*s, S_FALSE);
		}
	} else {
		ss_clear(*s);
		ss_cat_substr_u(s, src, char_off, n);
	}
	return ss_check(s);
}

srt_string *ss_cpy_cn(srt_string **s, const char *src, size_t src_size)
{
	RETURN_IF(!s, ss_void);
	ss_clear(*s);
	RETURN_IF(!src || !src_size, *s); /* BEHAVIOR: empty */
	ss_cat_cn(s, src, src_size);
	return ss_check(s);
}

srt_string *ss_cpy_c_aux(srt_string **s, const char *s1, ...)
{
	SS_COPYCAT_AUX_VARS(const char);
	ASSERT_RETURN_IF(!s, ss_void);
	SS_COPYCAT_AUX(s, S_FALSE, const char, s1, strlen, ss_cat_cn);
	return ss_check(s);
}

srt_string *ss_cpy_wn(srt_string **s, const wchar_t *src, size_t src_size)
{
	ASSERT_RETURN_IF(!s, ss_void);
	ss_clear(*s);
	return ss_cat_wn(s, src, src_size);
}

srt_string *ss_cpy_w_aux(srt_string **s, const wchar_t *s1, ...)
{
	SS_COPYCAT_AUX_VARS(const wchar_t);
	ASSERT_RETURN_IF(!s, ss_void);
	SS_COPYCAT_AUX(s, S_FALSE, const wchar_t, s1, wcslen, ss_cat_wn);
	return ss_check(s);
}

srt_string *ss_cpy_int(srt_string **s, int64_t num)
{
	return aux_toint(s, S_FALSE, num);
}

srt_string *ss_cpy_tolower(srt_string **s, const srt_string *src)
{
	return aux_toXcase(s, S_FALSE, src, fsc_tolower);
}

srt_string *ss_cpy_toupper(srt_string **s, const srt_string *src)
{
	return aux_toXcase(s, S_FALSE, src, fsc_toupper);
}

srt_string *ss_cpy_erase(srt_string **s, const srt_string *src, size_t off,
			 size_t n)
{
	return aux_erase(s, S_FALSE, src, off, n);
}

srt_string *ss_cpy_erase_u(srt_string **s, const srt_string *src,
			   size_t char_off, size_t n)
{
	return aux_erase_u(s, S_FALSE, src, char_off, n);
}

srt_string *ss_cpy_replace(srt_string **s, const srt_string *src, size_t off,
			   const srt_string *s1, const srt_string *s2)
{
	return aux_replace(s, S_FALSE, src, off, s1, s2);
}

srt_string *ss_cpy_resize(srt_string **s, const srt_string *src, size_t n,
			  char fill_byte)
{
	return aux_resize(s, S_FALSE, src, n, fill_byte);
}

srt_string *ss_cpy_resize_u(srt_string **s, srt_string *src, size_t n,
			    int fill_char)
{
	return aux_resize_u(s, S_FALSE, src, n, fill_char);
}

srt_string *ss_cpy_trim(srt_string **s, const srt_string *src)
{
	ASSERT_RETURN_IF(!s, ss_void);
	aux_ltrim(s, S_FALSE, src);
	return aux_rtrim(s, S_FALSE, *s);
}

srt_string *ss_cpy_ltrim(srt_string **s, const srt_string *src)
{
	return aux_ltrim(s, S_FALSE, src);
}

srt_string *ss_cpy_rtrim(srt_string **s, const srt_string *src)
{
	return aux_rtrim(s, S_FALSE, src);
}

srt_string *ss_cpy_printf(srt_string **s, size_t size, const char *fmt, ...)
{
	va_list ap;
	RETURN_IF(!s, ss_void);
	RETURN_IF((!size || !fmt) && ss_reset(*s), *s);
	if (*s) {
		ss_reserve(s, size);
		ss_clear(*s);
	}
	va_start(ap, fmt);
	ss_cat_printf_va(s, size, fmt, ap);
	va_end(ap);
	return *s;
}

srt_string *ss_cpy_printf_va(srt_string **s, size_t size, const char *fmt,
			     va_list ap)
{
	RETURN_IF(!s, ss_void);
	RETURN_IF((!size || !fmt) && ss_reset(*s), *s);
	if (*s) {
		ss_reserve(s, size);
		ss_clear(*s);
	}
	return ss_cat_printf_va(s, size, fmt, ap);
}

srt_string *ss_cpy_char(srt_string **s, int c)
{
	RETURN_IF(!s, ss_void);
	ss_clear(*s);
	if (ss_reserve(s, SSU8_MAX_SIZE) >= SSU8_MAX_SIZE)
		return ss_cat_char(s, c);
	return *s;
}

srt_string *ss_cpy_read(srt_string **s, FILE *handle, size_t max_bytes)
{
	aux_read(s, S_FALSE, handle, max_bytes);
	return *s;
}

/*
 * Concatenate/append from given source/s: a = ss_cat*
 */

srt_string *ss_cat_aux(srt_string **s, const srt_string *s1, ...)
{
	va_list ap;
	const srt_string *next, *s0;
	size_t extra_size, nargs, ss0, uss0, i;
	RETURN_IF(!s, ss_void);
	if (s1 && ss_size(s1) > 0) {
		extra_size = nargs = 0;
		next = s1;
		s0 = *s;
		va_start(ap, s1);
		while (!s_varg_tail_ptr_tag(next)) { /* last element tag */
			if (next)
				extra_size += ss_size(next);
			next = va_arg(ap, const srt_string *);
			nargs++;
		}
		va_end(ap);
		ss0 = *s ? ss_size(s0) : 0;
		uss0 = s0 ? get_unicode_size(s0) : 0;
		if (ss_grow(s, extra_size)) {
			ss_cat_aliasing(s, s0, ss0, uss0, s1);
			if (nargs == 1)
				return *s;
			va_start(ap, s1);
			i = 1;
			for (; i < nargs; i++) {
				next = va_arg(ap, const srt_string *);
				if (next)
					ss_cat_aliasing(s, s0, ss0, uss0, next);
			}
			va_end(ap);
		}
	}
	return ss_check(s);
}

srt_string *ss_cat_substr(srt_string **s, const srt_string *src, size_t sub_off,
			  size_t sub_size0)
{
	const char *src_str, *src_aux;
	size_t src_unicode_size, srcs, sub_size, src_off;
	ASSERT_RETURN_IF(!s, ss_void);
	RETURN_IF(!src || !sub_size0, ss_check(s)); /* no changes */
	src_str = NULL;
	srcs = ss_size(src);
	RETURN_IF(sub_off >= srcs, ss_check(s)); /* no changes */
	sub_size = sub_size0 == S_NPOS || (sub_off + sub_size0) > srcs
			   ? (srcs - sub_off)
			   : sub_size0;
	src_off = get_str_off(src) + sub_off;
	if (*s == src && !(*s)->d.f.ext_buffer) {
		/* Aliasing case: make grow the buffer in order
		 * to keep the reference to the data valid. */
		if (!ss_grow(s, sub_size))
			return *s; /* not enough memory */
		src_str = (const char *)*s;
	} else {
		src_aux = (const char *)src;
		src_str = (const char *)src_aux;
	}
	src_unicode_size = 0;
	return ss_cat_cn_raw(s, src_str, src_off, sub_size, src_unicode_size);
}

srt_string *ss_cat_substr_u(srt_string **s, const srt_string *src,
			    size_t char_off, size_t n)
{
	const char *psrc;
	size_t ssrc, off_size, actual_n, copy_size;
	ASSERT_RETURN_IF(!s, ss_void);
	if (src) {
		psrc = ss_get_buffer_r(src);
		ssrc = ss_size(src);
		off_size = sc_unicode_count_to_utf8_size(psrc, 0, ssrc,
							 char_off, NULL);
		/* BEHAVIOR: cut out of bounds, append nothing */
		if (off_size >= ssrc)
			return *s;
		actual_n = 0;
		copy_size = sc_unicode_count_to_utf8_size(psrc, off_size, ssrc,
							  n, &actual_n);
		ss_cat_cn_raw(s, psrc, off_size, copy_size, actual_n);
	}
	return ss_check(s);
}

srt_string *ss_cat_cn(srt_string **s, const char *src, size_t src_size)
{
	return ss_cat_cn_raw(s, src, 0, src_size, 0);
}

srt_string *ss_cat_c_aux(srt_string **s, const char *s1, ...)
{
	SS_COPYCAT_AUX_VARS(const char);
	ASSERT_RETURN_IF(!s, ss_void);
	SS_COPYCAT_AUX(s, S_TRUE, const char, s1, strlen, ss_cat_cn);
	return ss_check(s);
}

srt_string *ss_cat_wn(srt_string **s, const wchar_t *src, size_t src_size)
{
	int32_t uc32;
	wchar_t h, cl;
	size_t i, char_count, l;
	char utf8[SSU8_MAX_SIZE];
	ASSERT_RETURN_IF(!s, ss_void);
	if (src && ss_grow(s, src_size * SSU8_MAX_SIZE) && *s) {
		i = 0;
		char_count = 0;
		for (; i < src_size; i++) {
			if (sizeof(wchar_t) == 2) { /* UTF-16 */
				h = src[i];
				/*
				 * Basic Multilingual Plane (BMP), or
				 * full Unicode
				 */
				if (SSU16_SIMPLE(h)) {
					uc32 = (int32_t)h;
				} else {
					/*
					 * BEHAVIOR: ignore or cut incomplete
					 * UTF-16 characters
					 */
					if (!SSU16_VALID_HS(h))
						continue;
					if (i + 1 == src_size)
						break;
					cl = src[i + 1];
					if (!SSU16_VALID_LS(cl))
						continue;
					i++;
					/*
					 * Once validated, convert it
					 */
					uc32 = SSU16_TO_U32(h, cl);
				}
			} else { /* UTF-32 */
				uc32 = (int32_t)src[i];
			}
			l = sc_wc_to_utf8(uc32, utf8, 0, SSU8_MAX_SIZE);
			memcpy(ss_get_buffer(*s) + ss_size(*s), utf8, l);
			inc_size(*s, l);
			char_count++;
		}
		inc_unicode_size(*s, char_count);
	}
	return ss_check(s);
}

srt_string *ss_cat_w_aux(srt_string **s, const wchar_t *s1, ...)
{
	SS_COPYCAT_AUX_VARS(const wchar_t);
	ASSERT_RETURN_IF(!s, ss_void);
	SS_COPYCAT_AUX(s, S_TRUE, const wchar_t, s1, wcslen, ss_cat_wn);
	return ss_check(s);
}

srt_string *ss_cat_int(srt_string **s, int64_t num)
{
	return aux_toint(s, S_TRUE, num);
}

srt_string *ss_cat_tolower(srt_string **s, const srt_string *src)
{
	return aux_toXcase(s, S_TRUE, src, fsc_tolower);
}

srt_string *ss_cat_toupper(srt_string **s, const srt_string *src)
{
	return aux_toXcase(s, S_TRUE, src, fsc_toupper);
}

srt_string *ss_cat_erase(srt_string **s, const srt_string *src, size_t off,
			 size_t n)
{
	return aux_erase(s, S_TRUE, src, off, n);
}

srt_string *ss_cat_erase_u(srt_string **s, const srt_string *src,
			   size_t char_off, size_t n)
{
	return aux_erase_u(s, S_TRUE, src, char_off, n);
}

srt_string *ss_cat_replace(srt_string **s, const srt_string *src, size_t off,
			   const srt_string *s1, const srt_string *s2)
{
	return aux_replace(s, S_TRUE, src, off, s1, s2);
}

srt_string *ss_cat_resize(srt_string **s, const srt_string *src, size_t n,
			  char fill_byte)
{
	return aux_resize(s, S_TRUE, src, n, fill_byte);
}

srt_string *ss_cat_resize_u(srt_string **s, srt_string *src, size_t n,
			    int fill_char)
{
	return aux_resize_u(s, S_TRUE, src, n, fill_char);
}

srt_string *ss_cat_trim(srt_string **s, const srt_string *src)
{
	ASSERT_RETURN_IF(!s, ss_void);
	aux_ltrim(s, S_TRUE, src);
	return aux_rtrim(s, S_FALSE, *s);
}

srt_string *ss_cat_ltrim(srt_string **s, const srt_string *src)
{
	return aux_ltrim(s, S_TRUE, src);
}

srt_string *ss_cat_rtrim(srt_string **s, const srt_string *src)
{
	return aux_rtrim(s, S_TRUE, src);
}

srt_string *ss_cat_printf(srt_string **s, size_t size, const char *fmt, ...)
{
	va_list ap;
	ASSERT_RETURN_IF(!s, ss_void);
	va_start(ap, fmt);
	ss_cat_printf_va(s, size, fmt, ap);
	va_end(ap);
	return *s;
}

srt_string *ss_cat_printf_va(srt_string **s, size_t size, const char *fmt,
			     va_list ap)
{
	int sz;
	char *p;
	size_t off;
	ASSERT_RETURN_IF(!s, ss_void);
	S_ASSERT(s && size > 0 && fmt);
	if (size > 0 && fmt) {
		off = *s ? ss_size(*s) : 0;
		if (ss_grow(s, size)) {
			p = ss_get_buffer(*s) + off;
			sz = vsnprintf(p, size, fmt, ap);
			if (sz >= 0)
				inc_size(*s, (size_t)sz);
			else
				set_encoding_errors(*s, S_TRUE);
			set_unicode_size_cached(*s, S_FALSE);
		} else {
			ss_set_alloc_errors(*s);
		}
	}
	return ss_check(s);
}

srt_string *ss_cat_char(srt_string **s, int c)
{
	char tmp[6];
	RETURN_IF(!s, ss_void);
	return ss_cat_cn(s, tmp, sc_wc_to_utf8(c, tmp, 0, sizeof(tmp)));
}

srt_string *ss_cat_read(srt_string **s, FILE *handle, size_t max_bytes)
{
	aux_read(s, S_TRUE, handle, max_bytes);
	return *s;
}

/*
 * Transformation
 */

srt_string *ss_tolower(srt_string **s)
{
	return aux_toXcase(s, S_FALSE, *s, fsc_tolower);
}

srt_string *ss_toupper(srt_string **s)
{
	return aux_toXcase(s, S_FALSE, *s, fsc_toupper);
}

srt_bool ss_set_turkish_mode(srt_bool enable_turkish_mode)
{
	if (enable_turkish_mode) {
		fsc_tolower = sc_tolower_tr;
		fsc_toupper = sc_toupper_tr;
	} else {
		fsc_tolower = sc_tolower;
		fsc_toupper = sc_toupper;
	}
	return fsc_tolower != 0 && fsc_toupper != 0;
}

void ss_clear(srt_string *s)
{
	ss_reset(s);
}

srt_string *ss_check(srt_string **s)
{
	size_t ss, ssmax;
	ASSERT_RETURN_IF(!s, ss_void);
	if (!*s) {
		*s = ss_void;
		return *s;
	}
	ss = ss_size(*s);
	ssmax = ss_max_size(*s);
	S_ASSERT(ss <= ssmax);
	if (ss > ssmax) { /* This should never happen */
		ss_set_size(*s, ssmax);
		set_unicode_size_cached(*s, S_FALSE); /* Invalidate cache */
	}
	return *s;
}

srt_string *ss_erase(srt_string **s, size_t off, size_t n)
{
	return aux_erase(s, S_FALSE, *s, off, n);
}

srt_string *ss_erase_u(srt_string **s, size_t char_off, size_t n)
{
	return aux_erase_u(s, S_FALSE, *s, char_off, n);
}

srt_string *ss_replace(srt_string **s, size_t off, const srt_string *s1,
		       const srt_string *s2)
{
	return aux_replace(s, S_FALSE, *s, off, s1, s2);
}

srt_string *ss_resize(srt_string **s, size_t n, char fill_byte)
{
	return aux_resize(s, S_FALSE, *s, n, fill_byte);
}

srt_string *ss_resize_u(srt_string **s, size_t u_chars, int fill_char)
{
	return aux_resize_u(s, S_FALSE, *s, u_chars, fill_char);
}

srt_string *ss_trim(srt_string **s)
{
	ASSERT_RETURN_IF(!s, ss_void);
	aux_rtrim(s, S_FALSE, *s);
	return aux_ltrim(s, S_FALSE, *s);
}

srt_string *ss_ltrim(srt_string **s)
{
	return aux_ltrim(s, S_FALSE, *s);
}

srt_string *ss_rtrim(srt_string **s)
{
	return aux_rtrim(s, S_FALSE, *s);
}

/*
 * Export
 */

const char *ss_to_c(const srt_string *s)
{
	char *buf;
	size_t size;
	ASSERT_RETURN_IF(!s || s->d.f.st_mode == SData_VoidData, "");
	/*
	 * BEHAVIOR:
	 * - References based on C strings are allowed using ss_to_c(),
	 * however, references using raw data are not, returning an
	 * empty string instead. In case of requiring accessing to
	 * the buffer, ss_get_buffer_r(s) could be used instead.
	 */
	RETURN_IF(ss_is_ref(s), ss_is_cref(s) ? ss_get_buffer_r(s) : "");
	/*
	 * BEHAVIOR:
	 * Constness is kept regarding srt_string internal logical state. Said
	 * that, in order to ensure safe behavior, a 0 terminator is forced
	 * in case the string was not previously terminated.
	 * WARNING: be aware of that in the case of storing srt_string
	 * string on ROM memory or using read-only mmap (int that case
	 * you'll have to put terminating 0 by yourself after every
	 * srt_string on ROM or mmap'ed).
	 */
	buf = (char *)ss_get_buffer_r(s);
	size = ss_size(s);
	buf[size] = 0; /* C string terminator */
	return buf;
}

const wchar_t *ss_to_w(const srt_string *s, wchar_t *o, size_t nmax, size_t *n)
{
	int32_t c;
	const char *p;
	size_t o_s, i, ss, l;
	wchar_t *o_aux = NULL;
	S_ASSERT(s && o && nmax > 0);
	if (s && o && nmax > 0) {
		o_s = 0;
		i = 0;
		p = ss_get_buffer_r(s);
		ss = ss_size(s);
		for (; i < ss && i < nmax;) {
			c = 0;
			l = ss_utf8_to_wc(p, i, ss, &c, NULL);
			if (sizeof(wchar_t) == 2) {
				if ((c & ~0xffff) == 0) { /* simple */
					o[o_s++] = (wchar_t)c;
				} else {		   /* full */
					if (i + 1 == nmax) /* cut input */
						break;
					o[o_s++] = SSU16_HS0 | (c >> 10);
					o[o_s++] = SSU16_LS0 | (c & 0x3ffff);
				}
			} else {
				o[o_s++] = c;
			}
			i += l;
		}
		if (o_s == nmax)
			o_s--;
		o[o_s] = 0; /* zero-terminated string */
		if (n)
			*n = o_s;
		o_aux = o;
	}
	return o_aux ? o_aux : S_NULL_WC; /* Ensure valid string */
}

/*
 * Search
 */

size_t ss_find(const srt_string *s, size_t off, const srt_string *tgt)
{
	return ss_findr(s, off, S_NPOS, tgt);
}

size_t ss_findb(const srt_string *s, size_t off)
{
	return ss_findrb(s, off, S_NPOS);
}

size_t ss_findc(const srt_string *s, size_t off, char c)
{
	return ss_findrc(s, off, S_NPOS, c);
}

size_t ss_findcx(const srt_string *s, size_t off, uint8_t c_min, uint8_t c_max)
{
	return ss_findrcx(s, off, S_NPOS, c_min, c_max);
}

size_t ss_findu(const srt_string *s, size_t off, int c)
{
	return ss_findru(s, off, S_NPOS, c);
}

size_t ss_findnb(const srt_string *s, size_t off)
{
	return ss_findrnb(s, off, S_NPOS);
}

size_t ss_find_cn(const srt_string *s, size_t off, const char *t, size_t ts)
{
	return ss_findr_cn(s, off, S_NPOS, t, ts);
}

size_t ss_findr(const srt_string *s, size_t off, size_t max_off,
		const srt_string *tgt)
{
	size_t ss, ts;
	const char *s0, *t0;
	RETURN_IF(!s || !tgt, S_NPOS);
	ss = ss_real_off(s, max_off);
	ts = ss_size(tgt);
	RETURN_IF(!ss || !ts || (off + ts) > ss, S_NPOS);
	s0 = ss_get_buffer_r(s);
	t0 = ss_get_buffer_r(tgt);
	return ss_find_csum_fast(s0, off, ss, t0, ts);
}

#define SS_FINDRX_AUX_VARS const char *p0, *pm, *p
#define SS_FINDRX_AUX(LOOP_STOP_COND)                                          \
	{                                                                      \
		RETURN_IF(!s || off == S_NPOS || max_off < off, S_NPOS);       \
		p0 = ss_get_buffer_r(s);                                       \
		p = p0 + off;                                                  \
		pm = p0 + ss_real_off(s, max_off);                             \
		for (; p < pm; p++)                                            \
			if (LOOP_STOP_COND)                                    \
				break;                                         \
		return p >= p0 ? (size_t)(p - p0) : S_NPOS;                    \
	}

size_t ss_findrb(const srt_string *s, size_t off, size_t max_off)
{
	SS_FINDRX_AUX_VARS;
	SS_FINDRX_AUX(*p == 9 || *p == 10 || *p == 13 || *p == 32);
}

size_t ss_findrc(const srt_string *s, size_t off, size_t max_off, char c)
{
	SS_FINDRX_AUX_VARS;
	SS_FINDRX_AUX(*p == c);
}

size_t ss_findrcx(const srt_string *s, size_t off, size_t max_off,
		  uint8_t c_min, uint8_t c_max)
{
	SS_FINDRX_AUX_VARS;
	SS_FINDRX_AUX(*p >= c_min && *p <= c_max);
}

size_t ss_findru(const srt_string *s, size_t off, size_t max_off, int c)
{
	srt_string *tmps;
	SS_FINDRX_AUX_VARS;
	/* ASCII search */
	if (c < 128)
		SS_FINDRX_AUX(*p == c);
	/* Unicode search */
	RETURN_IF(!s || off == S_NPOS, S_NPOS);
	tmps = ss_alloca(6);
	ss_cat_char(&tmps, c);
	return ss_findr(s, off, max_off, tmps);
}

size_t ss_findrnb(const srt_string *s, size_t off, size_t max_off)
{
	SS_FINDRX_AUX_VARS;
	SS_FINDRX_AUX(*p != 9 && *p != 10 && *p != 13 && *p != 32);
}

size_t ss_findr_cn(const srt_string *s, size_t off, size_t max_off,
		   const char *t, size_t ts)
{
	size_t ss;
	RETURN_IF(!s || !t, S_NPOS);
	ss = ss_real_off(s, max_off);
	RETURN_IF(!ss || !ts || (off + ts) > ss, S_NPOS);
	return ss_find_csum_fast(ss_get_buffer_r(s), off, ss, t, ts);
}

size_t ss_split(const srt_string *src, const srt_string *separator,
		srt_string_ref out_substrings[], size_t max_refs)
{
	const char *p;
	size_t i, nelems = 0, off, sz, src_size = ss_size(src),
		  sep_size = ss_size(separator);
	if (src_size > 0 && sep_size > 0) {
		p = ss_get_buffer_r(src);
		for (i = 0; i < src_size;) {
			off = ss_find(src, i, separator);
			sz = (off != S_NPOS ? off : src_size) - i;
			if (nelems < max_refs) {
				ss_ref_buf(&out_substrings[nelems], p + i, sz);
				nelems++;
			} else {
				/* BEHAVOIR: out of reference space */
				break;
			}
			if (off == S_NPOS) { /* no more separators found */
				break;
			}
			i = off + sep_size;
		}
	}
	return nelems;
}

/*
 * Format
 */

int ss_printf(srt_string **s, size_t size, const char *fmt, ...)
{
	va_list ap;
	int out_size = -1;
	if (s && ss_reserve(s, size) >= size) {
		ss_clear(*s);
		va_start(ap, fmt);
		if (ss_cat_printf_va(s, size, fmt, ap))
			out_size = (int)ss_size(*s); /* BEHAVIOR */
		va_end(ap);
	}
	return out_size;
}

/*
 * Compare
 */

int ss_cmp(const srt_string *s1, const srt_string *s2)
{
	return ss_ncmp(s1, 0, s2, get_cmp_size(s1, s2));
}

int ss_cmpi(const srt_string *s1, const srt_string *s2)
{
	return ss_ncmpi(s1, 0, s2, get_cmp_size(s1, s2));
}

int ss_ncmp(const srt_string *s1, size_t s1off, const srt_string *s2, size_t n)
{
	int res = 0;
	size_t s1_size, s2_size, s1_n, s2_n;
	S_ASSERT(s1 && s2);
	if (s1 && s2) {
		s1_size = ss_size(s1);
		s2_size = ss_size(s2);
		s1_n = S_MIN(s1_size, s1off + n) - s1off;
		s2_n = S_MIN(s2_size, n);
		res = memcmp(ss_get_buffer_r(s1) + s1off, ss_get_buffer_r(s2),
			     S_MIN(s1_n, s2_n));
		if (res == 0 && s1_n != s2_n)
			res = s1_n < s2_n ? -1 : 1;
	} else { /* #BEHAVIOR: comparing null input */
		res = s1 == s2 ? 0 : s1 ? 1 : -1;
	}
	return res;
}

int ss_ncmpi(const srt_string *s1, size_t s1off, const srt_string *s2, size_t n)
{
	const char *s1_str, *s2_str;
	int res = 0, u1, u2, utf8_cut;
	size_t s1_size, s2_size, s1_max, s2_max, i, j, chs1, chs2;
	S_ASSERT(s1 && s2);
	if (s1 && s2) {
		s1_size = ss_size(s1);
		s2_size = ss_size(s2);
		s1_max = S_MIN(s1_size, s1off + n);
		s2_max = S_MIN(s2_size, n);
		s1_str = ss_get_buffer_r(s1);
		s2_str = ss_get_buffer_r(s2);
		i = s1off;
		j = 0;
		u1 = u2 = utf8_cut = 0;
		for (; i < s1_max && j < s2_max;) {
			chs1 = ss_utf8_to_wc(s1_str, i, s1_max, &u1, NULL);
			chs2 = ss_utf8_to_wc(s2_str, j, s2_max, &u2, NULL);
			if ((i + chs1) > s1_max || (j + chs2) > s2_max) {
				res = chs1 == chs2 ? 0 : chs1 < chs2 ? -1 : 1;
				utf8_cut = 1;
				/* BEHAVIOR: ignore last cutted chars */
				break;
			}
			if ((res = (int)(fsc_tolower((int32_t)u1)
					 - fsc_tolower((int32_t)u2)))
			    != 0) {
				break; /* difference found */
			}
			i += chs1;
			j += chs2;
		}
		/* equals until the cut case: */
		if (!utf8_cut && res == 0 && j < n && s1_max != s2_max)
			res = (s1_max - s1off) < s2_max ? -1 : 1;
	} else {
		res = s1 == s2 ? 0 : s1 ? 1 : -1;
	}
	return res;
}

/*
 * Unicode character I/O
 */

int ss_getchar(const srt_string *s, size_t *autoinc_off)
{
	int c;
	size_t ss;
	const char *p;
	RETURN_IF(!s || !autoinc_off, EOF);
	ss = ss_size(s);
	RETURN_IF(*autoinc_off >= ss, EOF);
	c = EOF;
	p = ss_get_buffer_r(s);
	*autoinc_off += ss_utf8_to_wc(p, *autoinc_off, ss, &c, NULL);
	return c;
}

int ss_putchar(srt_string **s, int c)
{
	return (s && ss_cat_char(s, c) && ss_size(*s) > 0) ? c : EOF;
}

int ss_popchar(srt_string **s)
{
	size_t off;
	char *s_str;
	RETURN_IF(!s || !*s || ss_size(*s) == 0, EOF);
	off = ss_size(*s) - 1;
	s_str = ss_get_buffer(*s);
	for (; off != S_SIZET_MAX; off--) {
		if (SSU8_VALID_START(s_str[off])) {
			int u_char = EOF;
			ss_utf8_to_wc(s_str, off, ss_size(*s), &u_char, *s);
			ss_set_size(*s, off);
			dec_unicode_size(*s, 1);
			return u_char;
		}
	}
	return EOF;
}

/*
 * I/O
 */

ssize_t ss_read(srt_string **s, FILE *handle, size_t max_bytes)
{
	return aux_read(s, S_FALSE, handle, max_bytes);
}

ssize_t ss_write(FILE *handle, const srt_string *s, size_t offset, size_t bytes)
{
	size_t ss, wr_size, ws;
	RETURN_IF(!handle, -1);
	ss = s ? ss_size(s) : 0;
	wr_size = handle && offset >= ss ? 0 : S_MIN(ss - offset, bytes);
	ws = fwrite(ss_get_buffer_r(s), 1, wr_size, handle);
	return ws > 0 && !ferror(handle) ? (ssize_t)ws : -1;
}

/*
 * Hashing
 */

uint32_t ss_crc32(const srt_string *s)
{
	return ss_crc32r(s, 0, 0, S_NPOS);
}

uint32_t ss_crc32r(const srt_string *s, uint32_t crc, size_t off1, size_t off2)
{
	size_t ss, offx;
	RETURN_IF(!s || off1 >= off2, 0);
	ss = ss_size(s);
	RETURN_IF(off1 >= ss, 0);
	offx = off2 == S_NPOS ? ss : off2;
	return sh_crc32(crc, ss_get_buffer_r(s) + off1, offx - off1);
}

uint32_t ss_adler32(const srt_string *s)
{
	return ss_adler32r(s, 0, 0, S_NPOS);
}

uint32_t ss_adler32r(const srt_string *s, uint32_t adler, size_t off1,
		     size_t off2)
{
	size_t ss, offx;
	RETURN_IF(!s || off1 >= off2, 0);
	ss = ss_size(s);
	RETURN_IF(off1 >= ss, 0);
	offx = off2 == S_NPOS ? ss : off2;
	return sh_adler32(adler, ss_get_buffer_r(s) + off1, offx - off1);
}

uint32_t ss_fnv1(const srt_string *s)
{
	return ss_fnv1r(s, S_FNV1_INIT, 0, S_NPOS);
}

uint32_t ss_fnv1r(const srt_string *s, uint32_t fnv, size_t off1, size_t off2)
{
	size_t ss, offx;
	RETURN_IF(!s || off1 >= off2, 0);
	ss = ss_size(s);
	RETURN_IF(off1 >= ss, 0);
	offx = off2 == S_NPOS ? ss : off2;
	return sh_fnv1(fnv, ss_get_buffer_r(s) + off1, offx - off1);
}

uint32_t ss_fnv1a(const srt_string *s)
{
	return ss_fnv1ar(s, S_FNV1_INIT, 0, S_NPOS);
}

uint32_t ss_fnv1ar(const srt_string *s, uint32_t fnv, size_t off1, size_t off2)
{
	size_t ss, offx;
	RETURN_IF(!s || off1 >= off2, 0);
	ss = ss_size(s);
	RETURN_IF(off1 >= ss, 0);
	offx = off2 == S_NPOS ? ss : off2;
	return sh_fnv1a(fnv, ss_get_buffer_r(s) + off1, offx - off1);
}

uint32_t ss_mh3_32(const srt_string *s)
{
	return ss_mh3_32r(s, S_MH3_32_INIT, 0, S_NPOS);
}

uint32_t ss_mh3_32r(const srt_string *s, uint32_t acc, size_t off1, size_t off2)
{
	size_t ss, offx;
	RETURN_IF(!s || off1 >= off2, 0);
	ss = ss_size(s);
	RETURN_IF(off1 >= ss, 0);
	offx = off2 == S_NPOS ? ss : off2;
	return sh_mh3_32(acc, ss_get_buffer_r(s) + off1, offx - off1);
}

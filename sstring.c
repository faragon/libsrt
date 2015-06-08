/*
 * sstring.c
 *
 * String handling.
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "schar.h"
#include "sstring.h"
#include "ssearch.h"
#include "scommon.h"
#include "senc.h"
#include "shash.h"

/*
 * Togglable optimizations
 *
 * S_ENABLE_UTF8_7BIT_PARALLEL_CASE_OPTIMIZATIONS: ~20x faster
 * for UTF-8 characters below code 128 (ASCII). The only reason
 * for disabling it is if some bug appears.
 */

#define S_ENABLE_UTF8_7BIT_PARALLEL_CASE_OPTIMIZATIONS
#ifdef S_MINIMAL_BUILD
#undef S_ENABLE_UTF8_7BIT_PARALLEL_CASE_OPTIMIZATIONS
#endif

/*
 * Constants
 */

#define S_SPLIT_MIN_ALLOC_ELEMS	32

/*
 * Macros
 */

#ifdef S_DEBUG
size_t dbg_cnt_alloc_calls = 0;      /* debug alloc/realloc calls */
#endif

/*
 * Templates
 */

#define SS_CCAT_STACK 128

#define SS_COPYCAT_AUX(s, cat, TYPE, nargs, s1, STRLEN, SS_CAT_XN)		\
	for (;s1;) {								\
		va_list ap;							\
		size_t *sizes,							\
		        pstack[SS_CCAT_STACK],					\
		       *pheap = NULL;						\
		TYPE *s_next;							\
		if (nargs <= SS_CCAT_STACK) {					\
			sizes = pstack;						\
		} else {							\
			pheap = (size_t *)malloc(sizeof(size_t) * nargs);	\
			if (!pheap) {						\
				ss_set_alloc_errors(*s);			\
				break;						\
			}							\
			sizes = pheap;						\
		}								\
		sizes[0] = STRLEN(s1);						\
		size_t extra_size = sizes[0];					\
		va_start(ap, s1);						\
		size_t i;							\
		for (i = 1; i < nargs; i++) {					\
			s_next = va_arg(ap, TYPE *);				\
			if (s_next) {						\
				sizes[i] = STRLEN(s_next);			\
				extra_size += sizes[i];				\
			}							\
		}								\
		va_end(ap);							\
		if (cat && ss_grow(s, extra_size) ||				\
		    ss_reserve(s, extra_size) >= extra_size) {			\
			if (!cat)						\
				ss_clear(s);					\
			SS_CAT_XN(s, s1, sizes[0]);				\
			va_start(ap, s1);					\
			for (i = 1; i < nargs; i++) {				\
				s_next = va_arg(ap, TYPE *);			\
				if (!s_next)					\
					continue;				\
				SS_CAT_XN(s, s_next, sizes[i]);			\
			}							\
			va_end(ap);						\
		} else {							\
			if (extra_size > 0)					\
				ss_set_alloc_errors(*s);			\
		}								\
		if (pheap)							\
			free(pheap);						\
		break;								\
	}

/*
 * Constants
 */

struct SSTR_Small ss_void0 = EMPTY_SS;
ss_t *ss_void = (ss_t *)&ss_void0; /* empty string with alloc error set */

/*
 * Forward definitions for some static functions
 */

static size_t get_max_size(const ss_t *s);
static void ss_reset(ss_t *s, const size_t alloc_size, const sbool_t ext_buf);
static void ss_reconfig(ss_t *s, size_t new_alloc_size, const size_t new_mt_sz);

/*
 * Global variables (used only for Turkish mode)
 * (equivalent to work with Turkish locale when doing case conversions)
 */

static sint32_t (*fsc_tolower)(const sint32_t c) = sc_tolower;
static sint32_t (*fsc_toupper)(const sint32_t c) = sc_toupper;
static struct sd_conf ssf = {	get_max_size,
				ss_reset,
				ss_reconfig,
				ss_alloc,
				ss_dup_s,
				ss_check,
				(sd_t *)&ss_void0,
				S_ALLOC_SMALL,
				SS_RANGE_SMALL, 
				SS_METAINFO_SMALL,
				SS_METAINFO_FULL,
				1,
				0,
				0
				};

/*
 * Internal functions
 */

static unsigned is_unicode_size_cached(const ss_t *s)
{
	return s->other1;
}

static unsigned get_unicode_size_h(const ss_t *s)
{
	return s->other2;
}

static unsigned has_encoding_errors(const ss_t *s)
{
	return s->other3;
}

static void set_unicode_size_cached(ss_t *s, const sbool_t is_cached)
{
	s->other1 = is_cached;
}

static void set_unicode_size_h(ss_t *s, const sbool_t is_h_bit_on)
{
	s->other2 = is_h_bit_on;
}

static void set_encoding_errors(ss_t *s, const sbool_t has_errors)
{
	s->other3 = has_errors;
}

static size_t get_size(const ss_t *s)
{
	return sd_get_size((const sd_t *)s);
}

static size_t get_alloc_size(const ss_t *s)
{
	return sd_get_alloc_size((const sd_t *)s);
}

static size_t get_max_size(const ss_t *s)
{
	const size_t alloc_size = get_alloc_size(s);
	return alloc_size - sd_alloc_size_to_mt_size(alloc_size, &ssf);
}

static size_t get_unicode_size(const ss_t *s)
{
	return s->is_full ? ((struct SSTR_Full *)s)->unicode_size :
	((get_unicode_size_h(s) << 8) | ((struct SSTR_Small *)s)->unicode_size_l);
}

static char *get_str(ss_t *s)
{
	return s->is_full ? ((struct SSTR_Full *)s)->str :
				     ((struct SSTR_Small *)s)->str;
}

static const char *get_str_r(const ss_t *s)
{
	return s->is_full ? ((struct SSTR_Full *)s)->str :
				     ((struct SSTR_Small *)s)->str;
}

static size_t get_str_off(const ss_t *s)
{
	return get_str_r(s) - (const char *)s;
}

static void set_size(ss_t *s, const size_t size)
{
	sd_set_size((sd_t *)s, size);
}

static void inc_size(ss_t *s, const size_t inc_size)
{
	set_size(s, get_size(s) + inc_size);
}

static void set_alloc_size(ss_t *s, const size_t alloc_size)
{
	sd_set_alloc_size((sd_t *)s, alloc_size);
}

static void ss_set_alloc_errors(ss_t *s)
{
	if (s)
		s->alloc_errors = 1;
}

static void set_unicode_size(ss_t *s, const size_t unicode_size)
{
	if (!is_unicode_size_cached(s))
		return;
	if (s->is_full) {
		((struct SSTR_Full *)s)->unicode_size = unicode_size;
	} else {
		set_unicode_size_h(s, (unicode_size & 0x100) ? 1 : 0);
		((struct SSTR_Small *)s)->unicode_size_l = unicode_size & 0xff;
	}
}

static void inc_unicode_size(ss_t *s, const size_t incr_size)
{
	/* BEHAVIOR: overflow */
	set_unicode_size(s, get_unicode_size(s) + incr_size);
}

static void dec_unicode_size(ss_t *s, const size_t dec_size)
{
	const size_t ss = get_unicode_size(s);
	S_ASSERT(ss >= dec_size || !is_unicode_size_cached(s));
	if (ss >= dec_size) {
		set_unicode_size(s, ss - dec_size);
	} else { /* This should never happen */
		set_unicode_size(s, 0);
		set_encoding_errors(s, S_TRUE);
	}
}

static void ss_reset(ss_t *s, const size_t alloc_size, const sbool_t ext_buf)
{
	S_ASSERT(s);
	if (s) { /* do not change 'ext_buffer' */
		sd_reset((sd_t *)s, sd_alloc_size_to_is_big(alloc_size, &ssf),
			 alloc_size, ext_buf);
		set_unicode_size_cached(s, S_TRUE);
		if (s->is_full) {
			set_unicode_size_h(s, 0);
		}
		set_encoding_errors(s, S_FALSE);
		set_unicode_size(s, 0);
	}
}

static void ss_reconfig(ss_t *s, size_t new_alloc_size, const size_t new_mt_sz)
{
	size_t size = get_size(s), unicode_size = get_unicode_size(s); char *s_str = get_str(s);
	/* Compute offset on target location: */
	struct SSTR_Small sm;
	struct SSTR_Full sf;
	const size_t s2_off = new_mt_sz == SS_METAINFO_SMALL?
				sm.str -(char *)&sm : sf.str - (char *)&sf;
	/* Convert string: */
	memmove((char *)s + s2_off, s_str, size);
	s->is_full = sd_alloc_size_to_is_big(new_alloc_size, &ssf);
	set_size(s, size);
	set_alloc_size(s, new_alloc_size);
	set_unicode_size(s, unicode_size);
}

/* BEHAVIOR: aliasing is supported, e.g. append(&a, a) */
static ss_t *ss_cat_cn_raw(ss_t **s, const char *src, const size_t src_off,
			   const size_t src_size, const size_t src_usize)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (src && src_size > 0) {
		const size_t off = *s ? get_size(*s) : 0;
		if (ss_grow(s, src_size)) {
			memmove(get_str(*s) + off, src + src_off, src_size);
			inc_size(*s, src_size);
			if (is_unicode_size_cached(*s)) {
				if (src_usize > 0)
					inc_unicode_size(*s, src_usize);
				else
					set_unicode_size_cached(*s, S_FALSE);
			}
		} else {
			S_ERROR("not enough memory");
			ss_set_alloc_errors(*s);
		}
	}
	return ss_check(s);
}

static ss_t *ss_cat_aliasing(ss_t **s, const ss_t *s0, const size_t s0_size,
			     const size_t s0_unicode_size, const ss_t *src)
{
	return src == s0 ? ss_cat_cn_raw(s, get_str(*s), 0, s0_size,
					    s0_unicode_size) :
			   ss_cat_cn_raw(s, get_str_r(src), 0, get_size(src),
					    get_unicode_size(src));
}

static size_t get_cmp_size(const ss_t *s1, const ss_t *s2)
{
	if (!s1 || !s2)
		return 0;
	size_t s1_size = get_size(s1), s2_size = get_size(s2);
	return S_MAX(s1_size, s2_size);
}

static size_t ss_utf8_to_wc(const char *s, const size_t off,
			    const size_t max_off, int *unicode_out,
			    ss_t *s_unicode_error_out)
{
	int encoding_errors = 0;
	const size_t csize = sc_utf8_to_wc(s, off, max_off, unicode_out,
							&encoding_errors);
	if (encoding_errors) {
		S_ERROR("broken UTF8");
		set_encoding_errors(s_unicode_error_out, S_TRUE);
	}
	return csize;
}

static ss_t *aux_toint(ss_t **s, const sbool_t cat, const sint_t num)
{
	ASSERT_RETURN_IF(!s, ss_void);
	char btmp[128], *p = btmp + sizeof(btmp) - 1;
	sint_t n = num < 0 ? -num : num;
	do {
		*p-- = '0' + n % 10;
		n /= 10;
	} while (n);
	if (num < 0)
		*p-- = '-';
	const size_t off = (p - btmp) + 1,
		     digits = sizeof(btmp) - off,
		     at = (cat && *s) ? get_size(*s) : 0,
		     out_size = at + digits;
        if (ss_reserve(s, out_size) >= out_size) {
		memcpy(get_str(*s) + at, btmp + off, digits);
		set_size(*s, out_size);
		inc_unicode_size(*s, digits);
	}
	return *s;
}

static ss_t *aux_toXcase(ss_t **s, const sbool_t cat, const ss_t *src,
							sint32_t (*towX)(sint32_t))
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	const size_t ss = get_size(src),
		     sso_max = *s ? get_max_size(*s) : 0;
	const char *ps = get_str_r(src);
	ss_t *out = NULL;
	const sbool_t aliasing = *s == src;
	unsigned char is_cached_usize = 0;
	ssize_t extra = sc_utf8_calc_case_extra_size(ps, 0, ss, towX);
	size_t cached_usize = 0,
	       at;
	/* If possible, keep Unicode size cached: */
	if (*s) {
		if (is_unicode_size_cached(*s) && is_unicode_size_cached(src)) {
			is_cached_usize = 1;
			cached_usize = get_unicode_size(src) +
				       get_unicode_size(*s);
		}
		at = cat ? get_size(*s) : 0;
	} else { /* copy */
		if (is_unicode_size_cached(src)) {
			is_cached_usize = 1;
			cached_usize = get_unicode_size(src);
		}
		at = 0;
	}
	/* Check if it is necessary to allocate more memory: */
	size_t sso_req = at + ss + extra;
	char *po0;
	if (!*s || sso_req > sso_max || (aliasing && extra > 0)) { /* BEHAVIOR */
		if (*s && (*s)->ext_buffer) {
			S_ERROR("not enough memory: strings stored in the "
				"stored in fixed-length buffer can not be "
				"resized.");
			ss_set_alloc_errors(*s);
			return ss_check(s);
		}
		out = ss_alloc(sso_req);
		if (!out) {	/* BEHAVIOR */
			S_ERROR("not enough memory: can not "
				"change character case");
			if (*s)
				ss_set_alloc_errors(*s);
			return ss_check(s);
		}
		char *pout = get_str(out);
		if (at > 0) /* cat */
			memcpy(pout, get_str(*s), at);
		po0 = pout + at;
	} else {
		po0 = get_str(*s) + at;
	}
	/* Case conversion loop: */
	size_t i = 0;
	int c = 0;
	char *po = po0,
	      u8[SSU8_MAX_SIZE];
	for (; i < ss;) {
#ifdef S_ENABLE_UTF8_7BIT_PARALLEL_CASE_OPTIMIZATIONS
		unsigned *pou = (unsigned *)po;
		const size_t i2 = sc_parallel_toX(ps, i, ss, pou, towX);
		if (i != i2) {
			po += (i2 - i);
			i = i2;
			if (i >= ss)
				break;
		}
#endif
		const size_t csize = ss_utf8_to_wc(ps, i, ss, &c, *s);
		const int c2 = towX(c);
		if (c2 == c) {
			i += csize;
			continue;
		}
		const size_t csize2 = sc_wc_to_utf8(c2, u8, 0, SSU8_MAX_SIZE);
		memcpy(po, u8, csize2);
		po += csize2;
		i += csize;
	}
	if (out) {	/* Case of using a secondary string was required */
		ss_t *s_bck = *s;
		*s = out;
		ss_free(&s_bck);
	}
	if (*s)
	{
		set_size(*s, sso_req);
		set_unicode_size_cached(*s, is_cached_usize);
		set_unicode_size(*s, cached_usize);
	}
	return ss_check(s);
}

/*
 * The conversion runs backwards in order to cover the
 * aliasing case without extra memory allocation or shift.
 */

static ss_t *aux_toenc(ss_t **s, const sbool_t cat, const ss_t *src, senc_f_t f)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	const int aliasing = *s == src;
	const size_t in_size = get_size(src),
		     at = (cat && *s) ? get_size(*s) : 0,
		     out_size = at + f(NULL, in_size, NULL);
	if (ss_reserve(s, out_size) >= out_size) {
		const ss_t *src1 = aliasing ? *s : src;
		f((unsigned char *)get_str_r(src1), in_size,
		  (unsigned char *)get_str(*s) + at);
		if (at == 0) {
			set_unicode_size_cached(*s, S_TRUE);
			set_unicode_size(*s, in_size * 2);
		} else { /* cat */
			if (is_unicode_size_cached(*s) &&
			    at == get_size(*s))
				set_unicode_size(*s, get_unicode_size(*s) +
						     in_size * 2);
			else
				set_unicode_size_cached(*s, S_FALSE);
		}
		set_size(*s, out_size);
	}
	return ss_check(s);
}

static ss_t *aux_erase(ss_t **s, const sbool_t cat, const ss_t *src,
		       const size_t off, const size_t n)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	const size_t ss0 = get_size(src),
			   at = (cat && *s) ? get_size(*s) : 0;
	const sbool_t overflow = off + n > ss0;
	const size_t src_size = overflow ? ss0 - off : n,
		     copy_size = ss0 - off - src_size;
	if (*s == src) { /* BEHAVIOR: aliasing: copy-only */
		if (off + n >= ss0) { /* tail clean cut */
			set_size(*s, off);
		} else {
			char *ps = get_str(*s);
			memmove(ps + off, ps + off + n, copy_size);
			set_size(*s, ss0 - n);
		}
		set_unicode_size_cached(*s, S_FALSE);
	} else { /* copy or cat */
		const size_t out_size = at + off + copy_size;
		if (ss_reserve(s, out_size) >= out_size) {
			char *po = get_str(*s);
			memcpy(po + at, get_str_r(src), off);
			memcpy(po + at + off,
			       get_str_r(src) + off + n, copy_size);
			set_size(*s, out_size);
			set_unicode_size_cached(*s, S_FALSE);
		}
	}
	return ss_check(s);
}

static ss_t *aux_erase_u(ss_t **s, const sbool_t cat, const ss_t *src,
			 const size_t char_off, const size_t n)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	const char *ps = get_str_r(src);
	const size_t sso0 = *s ? get_size(*s) : 0,
		     ss0 = get_size(src),
		     head_size = sc_unicode_count_to_utf8_size(ps, 0, ss0,
							char_off, NULL);
	ASSERT_RETURN_IF(head_size >= ss0, ss_check(s)); /* cut out of bounds */
	size_t actual_n = 0;
	const size_t cus = *s ? get_unicode_size(*s) : 0,
		     cut_size = sc_unicode_count_to_utf8_size(ps,
				head_size, ss0, n, &actual_n),
		     tail_size = ss0 - cut_size - head_size;
	size_t out_size = ss0 - cut_size,
	       prefix_usize = 0;
	unsigned char cached_usize = S_FALSE;
	if (*s == src) { /* aliasing: copy-only */
		char *po = get_str(*s);
		memmove(po + head_size, ps + head_size + cut_size, tail_size);
	} else { /* copy/cat */
		const size_t at = (cat && *s) ? get_size(*s) : 0;
		out_size += at;
		if (ss_reserve(s, out_size) >= out_size) {
			char *po = get_str(*s);
			memcpy(po + at, get_str_r(src), head_size);
			memcpy(po + at + head_size,
			       get_str_r(src) + head_size + cut_size,
			       tail_size);
			set_size(*s, out_size);
			if (is_unicode_size_cached(*s) &&
			    (at == 0 || at == sso0)) {
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
	set_size(*s, out_size);
	return ss_check(s);
}

static ss_t *aux_replace(ss_t **s, const sbool_t cat, const ss_t *src,
			 const size_t off, const ss_t *s1, const ss_t *s2)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (!s1)
		s1 = ss_void;
	if (!s2)
		s2 = ss_void;
	if (!src)
		src = ss_void;
	const size_t at = (cat && *s) ? get_size(*s) : 0;
	const char *p0 = get_str_r(src),
		   *p2 = get_str_r(s2);
	const size_t l1 = get_size(s1), l2 = get_size(s2);
	size_t i = off, l = get_size(src);
	ss_t *out = NULL;
	ssize_t size_delta = l2 - l1;
	sbool_t aliasing = S_FALSE;
	size_t out_size = at + l;
	char *o, *o0;
	if (l2 >= l1) { /* resize required */
		size_t nfound = 0;
		/* scan required size */
		for (;; i+= l1, nfound++)
			if ((i = ss_find(src, i, s1)) == S_NPOS)
				break;
		if (nfound == 0)	/* 0 occurrences: return */
			return ss_check(s);
		out_size += (size_delta * nfound);
		/* allocate output string */
		out = ss_alloc(out_size);
		if (!out) {
			S_ERROR("not enough memory");
			ss_set_alloc_errors(*s);
			return ss_check(s);
		}
		o0 = o = get_str(out);
		/* copy prefix data (cat) */
		if (at > 0)
			memcpy(o, get_str_r(*s), at);
	} else {
		o0 = o = get_str(*s);
		if (s && *s && *s == src) {
			aliasing = S_TRUE;
		} else {
			if (ss_reserve(s, out_size) < out_size) /* BEHAVIOR */
				return ss_check(s);
		}
	}
	typedef void (*memcpy_t)(void *, const void *, size_t);
	memcpy_t f_cpy;
	if (aliasing) {
		f_cpy = (memcpy_t)memmove;
	} else {
		f_cpy = (memcpy_t)memcpy;
		o += at;
		if (off > 0)	/* copy not affected data */
			memcpy(o, p0, off);
	}
	o += off;
	size_t i_next = s1 == s2? S_NPOS : /* no replace */
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
		f_cpy(o, p2, l2);
		o += l2;
		i = i_next + l1;
		/* prepare next search: */
		i_next = ss_find(src, i, s1);
	}
	if (out) {
		ss_t *s_bck = *s;
		*s = out;
		ss_free(&s_bck);
	}
	set_size(*s, (size_t)(o - o0));
	return *s;
}

static ss_t *aux_resize(ss_t **s, const sbool_t cat, const ss_t *src,
			const size_t n, char fill_byte)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	const size_t src_size = get_size(src),
		     at = (cat && *s) ? get_size(*s) : 0,
		     out_size = at + n;  /* BEHAVIOR: n overflow TODO */
	const sbool_t aliasing = *s == src;
	if (src_size < n) { /* fill */
		if (ss_reserve(s, out_size) >= out_size) {
			const char *p;
			char *o = get_str(*s);
			if (aliasing) {
				p = o;
			} else {
				p = get_str_r(src);
				memcpy(o + at, p, src_size);
			}
			memset(o + at + src_size, fill_byte,
				n - src_size);
			set_size(*s, out_size);
		}
	} else { /* else: cut (implicit) */
		if (ss_reserve(s, out_size) >= out_size) {
			if (!aliasing)
				memcpy(get_str(*s) + at, get_str_r(src),
								n);
			set_size(*s, out_size);
		}
	}
	return *s;
}

static ss_t *aux_resize_u(ss_t **s, const sbool_t cat, const ss_t *src,
			  const size_t u_chars, int fill_char)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	const size_t at = (cat && *s) ? get_size(*s) : 0,
		     char_size = sc_wc_to_utf8_size(fill_char),
		     current_u_chars = ss_len_u(src);
	RETURN_IF(u_chars == current_u_chars, ss_check(s)); /* same */
	const sbool_t aliasing = *s == src;
	const size_t src_size = get_size(src);
	if (current_u_chars < u_chars) { /* fill */
		const size_t new_elems = u_chars - current_u_chars,
			     out_size = at + src_size + new_elems * char_size;
		if (ss_reserve(s, out_size) >= out_size) {
			if (!cat && !aliasing) /* copy */
				ss_clear(s);
			if (!aliasing) {
				memcpy(get_str(*s) + at, get_str_r(src), src_size);
				inc_unicode_size(*s, current_u_chars);
				inc_size(*s, src_size);
			}
			size_t i = 0;
			for (; i < new_elems; i++)
				ss_cat_char(s, fill_char);
		}
	} else { /* cut */
		const char *ps = get_str_r(src);
		size_t actual_unicode_count = 0;
		const size_t head_size = sc_unicode_count_to_utf8_size(ps, 0, src_size,
						u_chars, &actual_unicode_count),
			     out_size = at + head_size;
		S_ASSERT(u_chars == actual_unicode_count);
		if (!aliasing) { /* copy or cat */
			if (ss_reserve(s, out_size) >= out_size) {
				if (!cat && !aliasing) /* copy */
					ss_clear(s);
				memcpy(get_str(*s) + at, ps, head_size);
				inc_unicode_size(*s, actual_unicode_count);
				inc_size(*s, head_size);
			}
		} else { /* cut */
			set_size(*s, head_size);
			set_unicode_size(*s, actual_unicode_count);
		}
	}
	return *s;
}

static ss_t *aux_ltrim(ss_t **s, const sbool_t cat, const ss_t *src)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	const size_t ss = get_size(src);
	if (ss > 0) {
		const sbool_t aliasing = *s == src;
		const char *ps = get_str_r(src);
		size_t i = 0;
		for (; i < ss && isspace(ps[i]); i++);
		size_t at, cat_usize;
		if (cat) {
			if (*s) {
				at = get_size(*s);
				cat_usize = get_unicode_size(*s);
			} else {
				at = cat_usize = 0;
			}
		} else {
			at = cat_usize = 0;
		}
		const size_t out_size = at + ss - i,
			     src_usize = get_unicode_size(src);
		if (ss_reserve(s, out_size) >= out_size) {
			char *pt = get_str(*s);
			if (!aliasing) /* copy or cat: shift data */
				memcpy(pt + at, ps + i, ss - i);
			else
				if (i > 0) /* copy: shift data */
					memmove(pt, ps + i, ss - i);
			set_size(*s, at + ss - i);
			set_unicode_size(*s, cat_usize + src_usize - i);
		}
	} else {
		if (cat)
			ss_check(s);
		else
			ss_clear(s);
	}
	return *s;
}

static ss_t *aux_rtrim(ss_t **s, const sbool_t cat, const ss_t *src)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (!src)
		src = ss_void;
	const size_t ss = get_size(src),
		     at = (cat && *s) ? get_size(*s) : 0;
	if (ss > 0) {
		const sbool_t aliasing = *s == src;
		const char *ps = get_str_r(src);
		size_t i = ss - 1;
		for (; i > 0 && isspace(ps[i]); i--);
		if (isspace(ps[i]))
			i--;
		const size_t nspaces = ss - i - 1,
			     copy_size = ss - nspaces,
			     out_size = at + copy_size,
			     cat_usize = cat ? get_unicode_size(*s) : 0,
			     src_usize = *s ? get_unicode_size(*s) : 0;
		if (ss_reserve(s, out_size) >= out_size) {
			char *pt = get_str(*s);
			if (!aliasing)
				 memcpy(pt + at, ps, copy_size);
			set_size(*s, out_size);
			set_unicode_size(*s, cat_usize + src_usize - nspaces);
		}
	} else {
		if (cat)
			ss_check(s);
		else
			ss_clear(s);
	}
	return *s;
}

/*
 * API functions
 */

/*
 * Generated from template
 */

SD_BUILDFUNCS(ss)

/*
#API: |Free one or more strings|string;more strings (optional)||O(1)|
void ss_free(ss_t **c, ...)

#API: |Ensure space for extra elements|string;number of extra eelements|extra size allocated|O(1)|
size_t ss_grow(ss_t **c, const size_t extra_elems)

#API: |Ensure space for elements|string;absolute element reserve|reserved elements|O(1)|
size_t ss_reserve(ss_t **c, const size_t max_elems)

#API: |Free unused space|string|same string (optional usage)|O(1)|
ss_t *ss_shrink_to_fit(ss_t **c)

#API: |Get string size|string|string bytes used in UTF8 format|O(1)|
size_t ss_get_size(const ss_t *c)

#API: |Set string size (bytes used in UTF8 format)|string||O(1)|
void ss_set_size(ss_t *c, const size_t s)

#API: |Equivalent to ss_get_size|string|Number of bytes (UTF-8 string length)|O(1)|
size_t ss_get_len(const ss_t *c)

#API: |Allocate string (stack)|space preAllocated to store n elements|allocated string|O(1)|
ss_t *ss_alloca(const size_t initial_reserve)
*/

/*
 * Allocation
 */


/* #API: |Allocate string (heap)|space preallocated to store n elements|allocated string|O(1)| */

ss_t *ss_alloc(const size_t initial_reserve)
{
	return (ss_t *)sd_alloc(0, 0, initial_reserve, &ssf);
}

/* #API: |Allocate string into external buffer|external buffer; external buffer size|allocated string|O(1)| */

ss_t *ss_alloc_into_ext_buf(void *buffer, const size_t buffer_size)
{
	return (ss_t *)sd_alloc_into_ext_buf(buffer, buffer_size, &ssf);
}

/*
 * Accessors
 */

/* #API: |String length (Unicode)|string|number of Unicode characters|O(1)| */

size_t ss_len_u(const ss_t *s)
{
	S_ASSERT(s);
	if (!s)
		return 0;
	if (is_unicode_size_cached(s))
		return get_unicode_size(s);
	/* Not cached, so cache it: */
	ss_t *w = (ss_t *)s; /* object will not be change but cached data */
	const char *p = get_str_r(w);
	const size_t ss = get_size(w),
		     cached_uc_size = sc_utf8_count_chars(p, ss);
	set_unicode_size_cached(w, S_TRUE);
	set_unicode_size(w, cached_uc_size);
	return cached_uc_size;
}

/* #API: |Allocated space|string|current allocated space (bytes)|O(1)| */

size_t ss_capacity(const ss_t *s)
{
	S_ASSERT(s);
	return s ? get_max_size(s) : 0;
}

/* #API: |Preallocated space left|string|allocated space left|O(1)| */

size_t ss_len_left(const ss_t *s)
{
	ASSERT_RETURN_IF(!s, 0);
	const size_t size = get_size(s),
		     max_size = get_max_size(s);
	S_ASSERT(s && max_size > size);
	return (s && max_size > size) ? max_size - size : 0;
}

/* #API: |Get the maximum possible string size|string|max string size (bytes)|O(1)| */

size_t ss_max(const ss_t *s)
{
	return !s ? 0 : s->ext_buffer ? get_max_size(s) : SS_RANGE_FULL;
}

/* #API: |Explicit set length (intended for external I/O raw acccess)|string;new length||O(1)| */

void ss_set_len(ss_t *s, const size_t new_len)
{
	if (s) {
		const size_t max_size = get_max_size(s);
		set_size(s, S_MIN(new_len, max_size));
		set_unicode_size_cached(s, S_FALSE); /* BEHAVIOR: cache lost */
	}
}

/* #API: |Get string buffer access|string|pointer to the insternal string buffer (UTF-8 or raw data)|O(1)| */

char *ss_get_buffer(ss_t *s)
{
	return s ? get_str(s) : NULL;
}

/* #API: |Check if string had allocation errors|string|S_TRUE: has errors; S_FALSE: no errors|O(1)| */

sbool_t ss_alloc_errors(const ss_t *s)
{
	return (!s || s->alloc_errors) ? S_TRUE : S_FALSE;
}

/* #API: |Check if string had UTF8 encoding errors|string|S_TRUE: has errors; S_FALSE: no errors|O(1)| */

sbool_t ss_encoding_errors(const ss_t *s)
{
	return s && has_encoding_errors(s) ? S_TRUE : S_FALSE;
}

/* #API: |Clear allocation/encoding error flags|string||O(1)| */

void ss_clear_errors(ss_t *s)
{
	if (s) {
		s->alloc_errors = S_FALSE;
		set_encoding_errors(s, S_FALSE); 
	}
}

/*
 * Allocation from a given source: s = ss_dup*
 * (equivalent to: s = NULL; ss_cpy*(&s, ...);)
 */

/* #API: |Duplicate string|string|Output result|O(n)| */

ss_t *ss_dup_s(const ss_t *src)
{
	ss_t *s = NULL;
	return ss_cpy(&s, src);
}

/* #API: |Duplicate from substring|string;substring offsets;select nth substring|Output result|O(n)| */

ss_t *ss_dup_sub(const ss_t *src, const sv_t *offs, const size_t nth)
{
	ss_t *s = NULL;
	return ss_cpy_sub(&s, src, offs, nth);
}

/* #API: |Duplicate from substring|string;byte offset;number of bytes|output result|O(n)| */

ss_t *ss_dup_substr(const ss_t *src, const size_t off, const size_t n)
{
	ss_t *s = NULL;
	return ss_cpy_substr(&s, src, off, n);
}

/* #API: |Duplicate from substring|string;character offset;number of characters|output result|O(n)| */

ss_t *ss_dup_u(const ss_t *src, const size_t char_off, const size_t n)
{
	ss_t *s = NULL;
	return ss_cpy_substr_u(&s, src, char_off, n);
}

/* #API: |Duplicate from C string|C string buffer;number of bytes|output result|O(n)| */

ss_t *ss_dup_cn(const char *src, const size_t src_size)
{
	ss_t *s = NULL;
	return ss_cpy_cn(&s, src, src_size);
}

/* #API: |Duplicate from C String (ASCII-z)|C string|output result|O(n)| */

ss_t *ss_dup_c(const char *src)
{
	ss_t *s = NULL;
	return ss_cpy_c(&s, src);
}

/* #API: |Duplicate from Unicode "wide char" string|"wide char" string; number of characters|output result|O(n)| */

ss_t *ss_dup_wn(const wchar_t *src, const size_t src_size)
{
	ss_t *s = NULL;
	return ss_cpy_wn(&s, src, src_size);
}

/* #API: |Duplicate from integer|integer|output result|O(1)| */
ss_t *ss_dup_int(const sint_t num)
{
	ss_t *s = NULL;
	return ss_cpy_int(&s, num);
}

/* #API: |Duplicate string with lowercase conversion|string|output result|O(n)| */

ss_t *ss_dup_tolower(const ss_t *src)
{
	ss_t *s = NULL;
	return ss_cpy_tolower(&s, src);
}

/* #API: |Duplicate string with uppercase conversion|string|output result|O(n)| */

ss_t *ss_dup_toupper(const ss_t *src)
{
	ss_t *s = NULL;
	return ss_cpy_toupper(&s, src);
}

#define MK_SS_DUP_TO_ENC(f, f_cpy)	\
	ss_t *f(const ss_t *src) {	\
		ss_t *s = NULL;		\
		return f_cpy(&s, src);	\
	}

MK_SS_DUP_TO_ENC(ss_dup_tob64, ss_cpy_tob64);
MK_SS_DUP_TO_ENC(ss_dup_tohex, ss_cpy_tohex);
MK_SS_DUP_TO_ENC(ss_dup_toHEX, ss_cpy_toHEX);


/*

#API: |Duplicate string with base64 conversion|string|output result|O(n)|
ss_t *ss_dup_tob64(const ss_t *src)

#API: |Duplicate string with hex conversion|string|output result|O(n)|
ss_t *ss_dup_tohex(const ss_t *src)

#API: |Duplicate string with hex conversion|string|output result|O(n)|
ss_t *ss_dup_toHEX(const ss_t *src)

*/

/* #API: |Duplicate from string erasing portion from input|string;byte offset;number of bytes|output result|O(n)| */

ss_t *ss_dup_erase(const ss_t *src, const size_t off, const size_t n)
{
	ss_t *s = NULL;
	return aux_erase(&s, S_FALSE, src, off, n);
}

/* #API: |Duplicate from Unicode "wide char" string erasing portion from input|string;character offset;number of characters|output result|O(n)| */

ss_t *ss_dup_erase_u(const ss_t *src, const size_t char_off, const size_t n)
{
	ss_t *s = NULL;
	return aux_erase_u(&s, S_FALSE, src, char_off, n);
}

/* #API: |Duplicate and apply replace operation after offset|string; offset (bytes); needle; needle replacement|output result|O(n)| */

ss_t *ss_dup_replace(const ss_t *src, const size_t off, const ss_t *s1, const ss_t *s2)
{
	ss_t *s = NULL;
	return aux_replace(&s, S_FALSE, src, off, s1, s2);
}

/* #API: |Duplicate and resize (byte addressing)|string; new size (bytes); fill byte|output result|O(n)| */

ss_t *ss_dup_resize(const ss_t *src, const size_t n, char fill_byte)
{
	ss_t *s = NULL;
	return aux_resize(&s, S_FALSE, src, n, fill_byte);
}

/* #API: |Duplicate and resize (Unicode addressing)|string; new size (characters); fill character|output result|O(n)| */

ss_t *ss_dup_resize_u(const ss_t *src, const size_t n, int fill_char)
{
	ss_t *s = NULL;
	return aux_resize_u(&s, S_FALSE, src, n, fill_char);
}

/* #API: |Duplicate and trim string|string|output result|O(n)| */

ss_t *ss_dup_trim(const ss_t *src)
{
	ss_t *s = NULL;
	return ss_cpy_trim(&s, src);
}

/* #API: |Duplicate and apply trim at left side|string|output result|O(n)| */

ss_t *ss_dup_ltrim(const ss_t *src)
{
	ss_t *s = NULL;
	return aux_ltrim(&s, S_FALSE, src);
}

/* #API: |Duplicate and apply trim at right side|string|output result|O(n)| */

ss_t *ss_dup_rtrim(const ss_t *src)
{
	ss_t *s = NULL;
	return aux_rtrim(&s, S_FALSE, src);
}

/* #API: |Duplicate from "wide char" Unicode string|"wide char" string|output result|O(n)| */

ss_t *ss_dup_w(const wchar_t *src)
{
	ss_t *s = NULL;
	return ss_cpy_w(&s, src);
}

/* #API: |Duplicate from printf formatting|printf space (bytes); printf format; optional printf parameters|output result|O(n)| */

ss_t *ss_dup_printf(const size_t size, const char *fmt, ...)
{
	ss_t *s = NULL;
	va_list ap;
	va_start(ap, fmt);
	ss_cpy_printf_va(&s, size, fmt, ap);
	va_end(ap);
	return s;
}

/* #API: |Duplicate from printf_va formatting|printf_va space (bytes); printf format; variable argument reference|output result|O(n)| */

ss_t *ss_dup_printf_va(const size_t size, const char *fmt, va_list ap)
{
	ss_t *s = NULL;
	return ss_cpy_printf_va(&s, size, fmt, ap);
}

/* #API: |Duplicate string from character|Unicode character|output result|O(1)| */

ss_t *ss_dup_char(const int c)
{
	ss_t *s = NULL;
	return ss_cpy_char(&s, c);
}

/*
 * Assignment from a given source: ss_cpy*(s, ...)
 */

/* #API: |Overwrite string with a string copy|output string; input string|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy(ss_t **s, const ss_t *src)
{
	ASSERT_RETURN_IF(!s, ss_void);
	RETURN_IF(*s == src && ss_check(s), *s); /* aliasing, same string */
	RETURN_IF(!src, ss_clear(s)); /* BEHAVIOR: empty */
	if (*s)
		ss_clear(s);
	return ss_cat(s, src);
}

/* #API: |Overwrite string with a substring copy|output string; input string; separator offsets; Nth substring to be used|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_sub(ss_t **s, const ss_t *src, const sv_t *offs, const size_t nth)
{
	ASSERT_RETURN_IF(!s, ss_void);
	ASSERT_RETURN_IF((!src || !offs), ss_clear(s)); /* BEHAVIOR: empty */
	const size_t elems = sv_get_size(offs) / 2;
	ASSERT_RETURN_IF(nth >= elems, ss_clear(s)); /* BEHAVIOR: empty */
	const size_t off = (size_t)sv_u_at(offs, nth * 2);
	const size_t size = (size_t)sv_u_at(offs, nth * 2 + 1);
	return ss_cpy_substr(s, src, off, size);
}

/* #API: |Overwrite string with a substring copy (byte mode)|output string; input string; input string start offset (bytes); number of bytes to be copied|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_substr(ss_t **s, const ss_t *src, const size_t off, const size_t n)
{
	ASSERT_RETURN_IF(!s, ss_void);
	ASSERT_RETURN_IF(!src || !n, ss_clear(s)); /* BEHAVIOR: empty */
	if (*s == src) { /* aliasing */
		char *ps = get_str(*s);
		const size_t ss = get_size(*s);
		ASSERT_RETURN_IF(off >= ss, ss_clear(s)); /* BEHAVIOR: empty */
		const size_t copy_size = S_MIN(ss - off, n);
		memmove(ps, ps + off, copy_size);
		set_size(*s, copy_size);
		set_unicode_size_cached(*s, S_FALSE); /* BEHAVIOR: cache lost */
	} else {
		if (*s)
			ss_clear(s);
		ss_cat_substr(s, src, off, n);
	}
	return ss_check(s);
}

/* #API: |Overwrite string with a substring copy (character mode)|output string; input string; input string start offset (characters); number of characters to be copied|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_substr_u(ss_t **s, const ss_t *src, const size_t char_off, const size_t n)
{
	ASSERT_RETURN_IF(!s, ss_void);
	ASSERT_RETURN_IF(!src || !n, ss_clear(s)); /* BEHAVIOR: empty */
	if (*s == src) { /* aliasing */
		char *ps = get_str(*s);
		size_t actual_unicode_count = 0;
		const size_t ss = get_size(*s);
		const size_t off = sc_unicode_count_to_utf8_size(ps, 0,
					ss, char_off, &actual_unicode_count);
		const size_t n_size = sc_unicode_count_to_utf8_size(ps,
			off, ss, n, &actual_unicode_count);
		ASSERT_RETURN_IF(off >= ss, ss_clear(s)); /* BEHAVIOR: empty */
		const size_t copy_size = S_MIN(ss - off, n_size);
		memmove(ps, ps + off, copy_size);
		set_size(*s, copy_size);
		if (n == actual_unicode_count) {
			set_unicode_size_cached(*s, S_TRUE);
			set_unicode_size(*s, n);
		} else { /* BEHAVIOR: cache lost */
			set_unicode_size_cached(*s, S_FALSE);
		}
	} else {
		if (*s)
			ss_clear(s);
		ss_cat_substr_u(s, src, char_off, n);
	}
	return ss_check(s);
}


/* #API: |Overwrite string with C string copy (strict aliasing is assumed)|output string; input string; input string bytes|output string reference (optional usage)|On(n)| */

ss_t *ss_cpy_cn(ss_t **s, const char *src, const size_t src_size)
{
	ASSERT_RETURN_IF(!s, ss_void);
	ASSERT_RETURN_IF(!src || !src_size, ss_clear(s)); /* BEHAVIOR: empty */
	if (*s)
		ss_clear(s);
	ss_cat_cn(s, src, src_size);
	return ss_check(s);
}

/*
#API: |Overwrite string with multiple C string copy (strict aliasing is assumed)|output string; input strings (one or more C strings)|output string reference (optional usage)|O(n)|
ss_t *ss_cpy_c(ss_t **s, ...)
*/

ss_t *ss_cpy_c_aux(ss_t **s, const size_t nargs, const char *s1, ...)
{
	ASSERT_RETURN_IF(!s, ss_void);
	SS_COPYCAT_AUX(s, S_FALSE, const char, nargs, s1, strlen, ss_cat_cn);
	return ss_check(s);
}

/* #API: |Overwrite string with "wide char" Unicode string copy (strict aliasing is assumed)|output string; input string ("wide char" Unicode); input string number of characters|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_wn(ss_t **s, const wchar_t *src, const size_t src_size)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (*s)
		ss_clear(s);
	return ss_cat_wn(s, src, src_size);
}

/*
#API: |Overwrite string with multiple C "wide char" Unicode string copy (strict aliasing is assumed)|output string; input strings (one or more C "wide char" strings)|output string reference (optional usage)|O(n)|
ss_t *ss_cpy_w(ss_t **s, ...)
*/

ss_t *ss_cpy_w_aux(ss_t **s, const size_t nargs, const wchar_t *s1, ...)
{
	ASSERT_RETURN_IF(!s, ss_void);
	SS_COPYCAT_AUX(s, S_FALSE, const wchar_t, nargs, s1, wcslen, ss_cat_wn);
	return ss_check(s);
}

/* #API: |Overwrite string with integer to string copy|output string; integer (any signed integer size)|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_int(ss_t **s, const sint_t num)
{
	return aux_toint(s, S_FALSE, num);
}

/* #API: |Overwrite string with input string lowercase conversion copy|output string; input string|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_tolower(ss_t **s, const ss_t *src)
{
	return aux_toXcase(s, S_FALSE, src, fsc_tolower);
}

/* #API: |Overwrite string with input string uppercase conversion copy|output string; input string|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_toupper(ss_t **s, const ss_t *src)
{
	return aux_toXcase(s, S_FALSE, src, fsc_toupper);
}

#define MK_SS_CPY_TO_ENC(f, f_enc)				\
	ss_t *f(ss_t **s, const ss_t *src) {			\
		return aux_toenc(s, S_FALSE, src, f_enc);	\
	}

MK_SS_CPY_TO_ENC(ss_cpy_tob64, senc_b64);
MK_SS_CPY_TO_ENC(ss_cpy_tohex, senc_hex);
MK_SS_CPY_TO_ENC(ss_cpy_toHEX, senc_HEX);

/*
#API: |Overwrite string with input string base64 conversion copy|output string; input string|output string reference (optional usage)|O(n)|
ss_t *ss_cpy_tob64(ss_t **s, const ss_t *src)

#API: |Overwrite string with input string hexadecimal (lowercase) conversion copy|output string; input string|output string reference (optional usage)|O(n)|
ss_t *ss_cpy_tohex(ss_t **s, const ss_t *src)

#API: |Overwrite string with input string hexadecimal (uppercase) conversion copy|output string; input string|output string reference (optional usage)|O(n)|
ss_t *ss_cpy_toHEX(ss_t **s, const ss_t *src)
*/

/* #API: |Overwrite string with input string copy applying a erase operation (byte/UTF-8 mode)|output string; input string; input string erase start byte offset; number of bytes to erase|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_erase(ss_t **s, const ss_t *src, const size_t off, const size_t n)
{
	return aux_erase(s, S_FALSE, src, off, n);
}

/* #API: |Overwrite string with input string copy applying a erase operation (character mode)|output string; input string; input string erase start character offset; number of characters to erase|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_erase_u(ss_t **s, const ss_t *src, const size_t char_off, const size_t n)
{
	return aux_erase_u(s, S_FALSE, src, char_off, n);
}

/* #API: |Overwrite string with input string plus replace operation|output string; input string; input string; offset for starting the replace operation (0 for the whole input string); pattern to be replaced; patter replacement||O(n)| */

ss_t *ss_cpy_replace(ss_t **s, const ss_t *src, const size_t off, const ss_t *s1, const ss_t *s2)
{
	return aux_replace(s, S_FALSE, src, off, s1, s2);
}

/* #API: |Overwrite string with input string copy plus resize operation (byte/UTF-8 mode)|output string; input string; number of bytes of input string; byte for refill|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_resize(ss_t **s, const ss_t *src, const size_t n, char fill_byte)
{
	return aux_resize(s, S_FALSE, src, n, fill_byte);
}

/* #API: |Overwrite string with input string copy plus resize operation (byte/UTF-8 mode)|output string; input string; number of bytes of input string; byte for refill|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_resize_u(ss_t **s, const ss_t *src, const size_t n, int fill_char)
{
	return aux_resize_u(s, S_FALSE, src, n, fill_char);
}

/* #API: |Overwrite string with input string plus trim (left and right) space removal operation|output string; input string|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_trim(ss_t **s, const ss_t *src)
{
	ASSERT_RETURN_IF(!s, ss_void);
	aux_ltrim(s, S_FALSE, src);
	return aux_rtrim(s, S_FALSE, *s);
}

/* #API: |Overwrite string with input string plus left-trim space removal operation|output string; input string|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_ltrim(ss_t **s, const ss_t *src)
{
	return aux_ltrim(s, S_FALSE, src);
}

/* #API: |Overwrite string with input string plus right-trim space removal operation|output string; input string|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_rtrim(ss_t **s, const ss_t *src)
{
	return aux_rtrim(s, S_FALSE, src);
}

/* #API: |Overwrite string with printf operation|output string; printf output size (bytes); printf format; printf parameters|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_printf(ss_t **s, const size_t size, const char *fmt, ...)
{
	ASSERT_RETURN_IF(!s, ss_void);
	ASSERT_RETURN_IF((!size || !fmt) && ss_clear(s), *s);
	if (*s) {
		ss_reserve(s, size);
		ss_clear(s);
	}
	va_list ap;
	va_start(ap, fmt);
	ss_cat_printf_va(s, size, fmt, ap);
	va_end(ap);
	return *s;
}

/* #API: |Overwrite string with printf_va operation|output string; printf output size (bytes); printf format; printf_va parameters|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_printf_va(ss_t **s, const size_t size, const char *fmt, va_list ap)
{
	ASSERT_RETURN_IF(!s, ss_void);
	ASSERT_RETURN_IF((!size || !fmt) && ss_clear(s), *s);
	if (*s) {
		ss_reserve(s, size);
		ss_clear(s);
	}
	return ss_cat_printf_va(s, size, fmt, ap);
}

/* #API: |Overwrite string with a string with just one character|output string; input character|output string reference (optional usage)|O(n)| */

ss_t *ss_cpy_char(ss_t **s, const int c)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (ss_reserve(s, SSU8_MAX_SIZE) >= SSU8_MAX_SIZE) {
		ss_clear(s);
		return ss_cat_char(s, c);
	}
	return *s;
}

/*
 * Concatenate/append from given source/s: a = ss_cat*
 */

/*
#API: |Concatenate to string one or more strings|output string; input string; optional input strings|output string reference (optional usage)|O(n)|
ss_t *ss_cat(ss_t **s, const ss_t *s1, ...)
*/

ss_t *ss_cat_aux(ss_t **s, const size_t nargs, const ss_t *s1, ...)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (s1 && get_size(s1) > 0) {
		va_list ap;
		size_t extra_size = get_size(s1);
		const ss_t *s_next, *s0 = *s;
		const size_t ss0 = *s ? get_size(s0) : 0,
			     uss0 = s0 ? get_unicode_size(s0) : 0;
		va_start(ap, s1);
		size_t i = 1;
		for (; i < nargs; i++) {
			s_next = va_arg(ap, const ss_t *);
			if (s_next)
				extra_size += get_size(s_next);
		}
		va_end(ap);
		if (ss_grow(s, extra_size)) {
			ss_cat_aliasing(s, s0, ss0, uss0, s1);
			if (nargs == 1)
				return *s;
			va_start(ap, s1);
			for (i = 1; i < nargs; i++) {
				s_next = va_arg(ap, const ss_t *);
				if (s_next)
					ss_cat_aliasing(s, s0, ss0, uss0, s_next);
			}
			va_end(ap);
		}
	}
	return ss_check(s);
}

/* #API: |Concatenate substring token|output string; input string; substring offsets;select nth substring|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_sub(ss_t **s, const ss_t *src, const sv_t *offs, const size_t nth)
{
	ASSERT_RETURN_IF(!s, ss_void);
	RETURN_IF((!src || !offs), ss_check(s)); /* same string */
	char *src_str = NULL, *src_aux;
	const size_t elems = sv_get_size(offs) / 2;
	ASSERT_RETURN_IF(nth >= elems, ss_clear(s)); /* BEHAVIOR: empty */
	size_t src_off = get_str_off(src), src_size;
	if (nth < elems) {
		src_off += (size_t)sv_u_at(offs, nth * 2);
		src_size = (size_t)sv_u_at(offs, nth * 2 + 1);
	} else {
		src_size = get_size(src);
	}
	if (*s == src && *s && !(*s)->ext_buffer) {
		/* Aliasing case: make grow the buffer in order
		   to keep the reference to the data valid. */
		if (!ss_grow(s, src_size))
			return *s; /* not enough memory */
		src_str = (char *)*s;
	} else {
		src_aux = (char *)src;
		src_str = (char *)src_aux;
	}
	const size_t src_unicode_size = is_unicode_size_cached(src) ?
					get_unicode_size(src) : 0;
	return ss_cat_cn_raw(s, src_str, src_off, src_size, src_unicode_size);
}

/* #API: |Concatenate substring (byte/UTF-8 mode)|output string; input string; input string substring byte offset; input string substring size (bytes)|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_substr(ss_t **s, const ss_t *src, const size_t sub_off, const size_t sub_size)
{
	ASSERT_RETURN_IF(!s, ss_void);
	RETURN_IF(!src, ss_check(s)); /* same string */
	char *src_str = NULL, *src_aux;
	size_t src_unicode_size = 0;
	const size_t src_off = get_str_off(src) + sub_off; 
	if (*s == src && !(*s)->ext_buffer) {
		/* Aliasing case: make grow the buffer in order
		   to keep the reference to the data valid. */
		if (!ss_grow(s, sub_size))
			return *s; /* not enough memory */
		src_str = (char *)*s;
	} else {
		src_aux = (char *)src;
		src_str = (char *)src_aux;
	}
	src_unicode_size = 0;
	return ss_cat_cn_raw(s, src_str, src_off, sub_size, src_unicode_size);
}

/* #API: |Concatenate substring (Unicode character mode)|output string; input string; input substring character offset; input string substring size (characters)|output string reference (optional usage)||O(n)| */

ss_t *ss_cat_substr_u(ss_t **s, const ss_t *src, const size_t char_off,
							const size_t n)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (src) {
		const char *psrc = get_str_r(src);
		const size_t ssrc = get_size(src),
			     off_size = sc_unicode_count_to_utf8_size(psrc, 0,
							ssrc, char_off, NULL);
		/* BEHAVIOR: cut out of bounds, append nothing */
		if (off_size >= ssrc)
			return *s;
		size_t actual_n = 0;
		const size_t copy_size = sc_unicode_count_to_utf8_size(psrc,
						off_size, ssrc, n, &actual_n);
		ss_cat_cn_raw(s, psrc, off_size, copy_size, actual_n);
	}
	return ss_check(s);
}

/* #API: |Concatenate C substring (byte/UTF-8 mode)|output string; input C string; input string size (bytes)|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_cn(ss_t **s, const char *src, const size_t src_size)
{
	char *src_aux = (char *)src;
	return ss_cat_cn_raw(s, src_aux, 0, src_size, 0);
}

/*
#API: |Concatenate multiple C strings (byte/UTF-8 mode)|output string; input string; optional input strings|output string reference (optional usage)|O(n)|
ss_t *ss_cat_c(ss_t **s, const char *s1, ...)
*/

ss_t *ss_cat_c_aux(ss_t **s, const size_t nargs, const char *s1, ...)
{
	ASSERT_RETURN_IF(!s, ss_void);
	SS_COPYCAT_AUX(s, S_TRUE, const char, nargs, s1, strlen, ss_cat_cn);
	return ss_check(s);
}

/* #API: |Concatenate "wide char" C substring (Unicode character mode)|output string; input "wide char" C string; input string size (characters)|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_wn(ss_t **s, const wchar_t *src, const size_t src_size)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (src) {
		if (ss_grow(s, src_size * SSU8_MAX_SIZE)) {
			char utf8[SSU8_MAX_SIZE];
			size_t i = 0,
			       char_count = 0;
			for (; i < src_size; i++) {
				const size_t l = sc_wc_to_utf8(src[i], utf8,
							     0, SSU8_MAX_SIZE);
				memcpy(get_str(*s) + get_size(*s), utf8, l);
				inc_size(*s, l);
				char_count++;
			}
			inc_unicode_size(*s, char_count++);
		} else {
			S_ERROR("not enough memory");
			ss_set_alloc_errors(*s);
		}
	}
	return ss_check(s);
}

/*
#API: |Concatenate multiple "wide char" C strings (Unicode character mode)|output string; input "wide char" C string; optional input "wide char" C strings|output string reference (optional usage)|O(n)|
ss_t *ss_cat_w(ss_t **s, const char *s1, ...)
*/

ss_t *ss_cat_w_aux(ss_t **s, const size_t nargs, const wchar_t *s1, ...)
{
	ASSERT_RETURN_IF(!s, ss_void);
	SS_COPYCAT_AUX(s, S_TRUE, const wchar_t, nargs, s1, wcslen, ss_cat_wn);
	return ss_check(s);
}

/* #API: |Concatenate integer|output string; integer (any signed integer size)|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_int(ss_t **s, const sint_t num)
{
	return aux_toint(s, S_TRUE, num);
}

/* #API: |Concatenate "lowercased" string|output string; input string|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_tolower(ss_t **s, const ss_t *src)
{
        return aux_toXcase(s, S_TRUE, src, fsc_tolower);
}

/* #API: |Concatenate "uppercased" string|output string; input string|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_toupper(ss_t **s, const ss_t *src)
{
	return aux_toXcase(s, S_TRUE, src, fsc_toupper);
}

#define MK_SS_CAT_TO_ENC(f, f_enc)				\
	ss_t *f(ss_t **s, const ss_t *src) {			\
		return aux_toenc(s, S_TRUE, src, f_enc);	\
	}

MK_SS_CAT_TO_ENC(ss_cat_tob64, senc_b64);
MK_SS_CAT_TO_ENC(ss_cat_tohex, senc_hex);
MK_SS_CAT_TO_ENC(ss_cat_toHEX, senc_HEX);

/*
#API: |Concatenate string with input string base64 conversion copy|output string; input string|output string reference (optional usage)|O(n)|
ss_t *ss_cat_tob64(ss_t **s, const ss_t *src)

#API: |Concatenate string with input string hexadecimal (lowercase) conversion copy|output string; input string|output string reference (optional usage)|O(n)|
ss_t *ss_cat_tohex(ss_t **s, const ss_t *src)

#API: |Concatenate string with input string hexadecimal (uppercase) conversion copy|output string; input string|output string reference (optional usage)|O(n)|
ss_t *ss_cat_toHEX(ss_t **s, const ss_t *src)
*/

/* #API: |Concatenate string with erase operation (byte/UTF-8 mode)|output string; input string; input string byte offset for erase start; erase count (bytes)|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_erase(ss_t **s, const ss_t *src, const size_t off, const size_t n)
{
	return aux_erase(s, S_TRUE, src, off, n);
}

/* #API: |Concatenate string with erase operation (Unicode character mode)|output string; input string; input character string offset for erase start; erase count (characters)|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_erase_u(ss_t **s, const ss_t *src, const size_t char_off,
								const size_t n)
{
	return aux_erase_u(s, S_TRUE, src, char_off, n);
}

/* #API: |Concatenate string with replace operation|output string; input string; offset for starting the replace operation (0 for the whole input string); pattern to be replaced; patter replacement|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_replace(ss_t **s, const ss_t *src, const size_t off, const ss_t *s1, const ss_t *s2)
{
	return aux_replace(s, S_TRUE, src, off, s1, s2);
}

/* #API: |Concatenate string with input string copy plus resize operation (byte/UTF-8 mode)|output string; input string; number of bytes of input string; byte for refill|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_resize(ss_t **s, const ss_t *src, const size_t n, char fill_byte)
{
	return aux_resize(s, S_TRUE, src, n, fill_byte);
}

/* #API: |Concatenate string with input string copy plus resize operation (Unicode character)|output string; input string; number of characters of input string; character for refill|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_resize_u(ss_t **s, const ss_t *src, const size_t n, int fill_char)
{
	return aux_resize_u(s, S_TRUE, src, n, fill_char);
}

/* #API: |Concatenate string with input string plus trim (left and right) space removal operation|output string; input string|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_trim(ss_t **s, const ss_t *src)
{
	ASSERT_RETURN_IF(!s, ss_void);
	aux_ltrim(s, S_TRUE, src);
	return aux_rtrim(s, S_FALSE, *s);
}

/* #API: |Concatenate string with input string plus left-trim space removal operation|output string; input string|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_ltrim(ss_t **s, const ss_t *src)
{
	return aux_ltrim(s, S_TRUE, src);
}

/* #API: |Concatenate string with input string plus right-trim space removal operation|output string; input string|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_rtrim(ss_t **s, const ss_t *src)
{
	return aux_rtrim(s, S_TRUE, src);
}

/* #API: |Concatenate string with printf operation|output string; printf output size (bytes); printf format; printf parameters|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_printf(ss_t **s, const size_t size, const char *fmt, ...)
{
	ASSERT_RETURN_IF(!s, ss_void);
	va_list ap;
	va_start(ap, fmt);
	ss_cat_printf_va(s, size, fmt, ap);
	va_end(ap);
	return *s;
}

/* #API: |Concatenate string with printf_va operation|output string; printf output size (bytes); printf format; printf_va parameters|output string reference (optional usage)|O(n)| */

ss_t *ss_cat_printf_va(ss_t **s, const size_t size, const char *fmt, va_list ap)
{
	ASSERT_RETURN_IF(!s, ss_void);
	S_ASSERT(s && size > 0 && fmt);
	if (size > 0 && fmt) {
		const size_t off = *s ? get_size(*s) : 0;
		if (ss_grow(s, size)) {
			char *p = get_str(*s) + off;
			const int sz = vsnprintf(p, size, fmt, ap);
			if (sz >= 0)
				inc_size(*s, sz);
			else
				set_encoding_errors(*s, S_TRUE);
			set_unicode_size_cached(*s, S_FALSE);
		} else {
			ss_set_alloc_errors(*s);
		}
	}
	return ss_check(s);
}

/* #API: |Concatenate string with a string with just one character|output string; input character|output string reference (optional usage)|O(1)| */

ss_t *ss_cat_char(ss_t **s, const int c)
{
	ASSERT_RETURN_IF(!s, ss_void);
	const wchar_t src[1] = { c };
	return ss_cat_wn(s, src, 1);
}

/*
 * Transformation
 */

/* #API: |Convert string to lowercase|output string|output string reference (optional usage)|O(n)| */

ss_t *ss_tolower(ss_t **s)
{
        return aux_toXcase(s, S_FALSE, *s, fsc_tolower);
}

/* #API: |Convert string to uppercase|output string|output string reference (optional usage)|O(n)| */

ss_t *ss_toupper(ss_t **s)
{
        return aux_toXcase(s, S_FALSE, *s, fsc_toupper);
}

#define MK_SS_TO_ENC(f, f_enc)				\
	ss_t *f(ss_t **s, const ss_t *src) {			\
		return aux_toenc(s, S_FALSE, src, f_enc);	\
	}

MK_SS_TO_ENC(ss_tob64, senc_b64);
MK_SS_TO_ENC(ss_tohex, senc_hex);
MK_SS_TO_ENC(ss_toHEX, senc_HEX);

/*
#API: |Convert to base64|output string; input string|output string reference (optional usage)|O(n)|
ss_t *ss_tob64(ss_t **s, const ss_t *src)

#API: |Convert to hexadecimal (lowercase)|output string; input string|output string reference (optional usage)|O(n)|
ss_t *ss_tohex(ss_t **s, const ss_t *src)

#API: |Convert to hexadecimal (uppercase)|output string; input string|output string reference (optional usage)|O(n)|
ss_t *ss_toHEX(ss_t **s, const ss_t *src)
*/

/* #API: |Set Turkish mode locale (related to case conversion)|S_TRUE: enable; S_FALSE: disable|S_TRUE: conversion functions OK, S_FALSE: error (missing functions)|O(1)| */

sbool_t ss_set_turkish_mode(const sbool_t enable_turkish_mode)
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

/* #API: |Clear string|output string|output string reference (optional usage)|O(n)| */

ss_t *ss_clear(ss_t **s)
{
	ASSERT_RETURN_IF(!s || !*s, ss_check(s));
	ss_reset(*s, get_alloc_size(*s), (*s)->ext_buffer);
	return *s;
}

/* Replaces a NULL string with an empty string of size 0 */

/* #API: |Check and fix string|output string|output string reference (optional usage)|O(n)| */

ss_t *ss_check(ss_t **s)
{
	ASSERT_RETURN_IF(!s, ss_void);
	if (!*s) {
		*s = ss_void;
		return *s;
	}
	const size_t ss = get_size(*s), ssmax = get_max_size(*s);
	S_ASSERT(ss <= ssmax);
	if (ss > ssmax) { /* This should never happen */
		S_ASSERT(ss > ssmax);
		set_size(*s, ssmax);
		set_unicode_size_cached(*s, S_FALSE); /* Invalidate cache */
	}
	return *s;
}

/* #API: |Erase portion of a string (byte/UTF-8 mode)|input/output string; byte offset where to start the cut; number of bytes to be cut|output string reference (optional usage)|O(n)| */

ss_t *ss_erase(ss_t **s, const size_t off, const size_t n)
{
	return aux_erase(s, S_FALSE, *s, off, n);
}

/* #API: |Erase portion of a string (Unicode character mode)|input/output string; character offset where to start the cut; number of characters to be cut|output string reference (optional usage)|O(n)| */

ss_t *ss_erase_u(ss_t **s, const size_t char_off, const size_t n)
{
	return aux_erase_u(s, S_FALSE, *s, char_off, n);
}

/* #API: |Replace into string|input/output string; byte offset where to start applying the replace operation; target pattern; replacement pattern|output string reference (optional usage)|O(n)| */

ss_t *ss_replace(ss_t **s, const size_t off, const ss_t *s1, const ss_t *s2)
{
	return aux_replace(s, S_FALSE, *s, off, s1, s2);
}

/* #API: |Resize string (byte/UTF-8 mode)|input/output string; new size in bytes; fill byte|output string reference (optional usage)|O(n)| */

ss_t *ss_resize(ss_t **s, const size_t n, char fill_byte)
{
	return aux_resize(s, S_FALSE, *s, n, fill_byte);
}

/* #API: |Resize string (Unicode character mode)|input/output string; new size in characters; fill character|output string reference (optional usage)|O(n)| */

ss_t *ss_resize_u(ss_t **s, const size_t u_chars, int fill_char)
{
	return aux_resize_u(s, S_FALSE, *s, u_chars, fill_char);
}

/* #API: |Remove spaces from left and right side|input/output string|output string reference (optional usage)|O(n)| */

ss_t *ss_trim(ss_t **s)
{
	ASSERT_RETURN_IF(!s, ss_void);
	aux_rtrim(s, S_FALSE, *s);
	return aux_ltrim(s, S_FALSE, *s);
}

/* #API: |Remove spaces from left side|input/output string|output string reference (optional usage)|O(n)| */

ss_t *ss_ltrim(ss_t **s)
{
	return aux_ltrim(s, S_FALSE, *s);
}

/* #API: |Remove spaces from right side|input/output string|output string reference (optional usage)|O(n)| */

ss_t *ss_rtrim(ss_t **s)
{
	return aux_rtrim(s, S_FALSE, *s);
}

/*
 * Export
 */

/* #API: |Give a C-compatible zero-ended string reference (byte/UTF-8 mode)|input string|Zero-ended C compatible string reference (UTF-8)|O(1)| */

const char *ss_to_c(ss_t *s)
{
	ASSERT_RETURN_IF(!s, S_NULL_C); /* Ensure valid string */
	RETURN_IF(s == ss_void, "");
	char *s_str = get_str(s);
	const size_t s_size = get_size(s);
	s_str[s_size] = 0;	/* Ensure ASCIIz-ness */
	return s_str;		/* +1 space is guaranteed */
}

/* #API: |Give a C-compatible zero-ended string reference ("wide char" Unicode mode)|input string; output string buffer; output string max characters; output string size|Zero'ended C compatible string reference ("wide char" Unicode mode)|O(n)| */

const wchar_t *ss_to_w(const ss_t *s, wchar_t *o, const size_t nmax, size_t *n)
{
	wchar_t *o_aux = NULL;
	S_ASSERT(s && o && nmax > 0);
	if (s && o && nmax > 0) {
		size_t o_s = 0,
		       i = 0;
		const char *p = get_str_r(s);
		const size_t ss = get_size(s);
		for (; i < ss && i < nmax;) {
			int c = 0;
			const size_t l = ss_utf8_to_wc(p, i, ss, &c, (ss_t *)s);
			o[o_s++] = c;
			i += l;
		}
		if (o_s == nmax)
			o_s--;
		o[o_s] = 0;	/* zero-terminated string */
		if (n)
			*n = o_s;
		o_aux = o;
	}
	return o_aux ? o_aux : S_NULL_WC; /* Ensure valid string */
}

/*
 * Search
 */

/* #API: |Find substring into string|input string; search offset start; target string|Offset location if found, S_NPOS if not found|O(n)| */

size_t ss_find(const ss_t *s, const size_t off, const ss_t *tgt)
{
	if (!s || !tgt)
		return S_NPOS;
	const size_t ss = get_size(s), ts = get_size(tgt);
	if (ss == 0 || ts == 0 || (off + ts) > ss)
		return S_NPOS;
	const char *s0 = get_str_r(s), *t0 = get_str_r(tgt);
	return ss_find_csum_fast(s0, off, ss, t0, ts);
}

/* #API: |Split/tokenize: break string by separators|output offset structure;input string; separator|Number of elements|O(n)| */

size_t ss_split(sv_t **v, const ss_t *src, const ss_t *separator)
{
	S_ASSERT(v && src && separator);
	if (!v || !src || !separator)
		return 0;
	if (*v == NULL)
		*v = sv_alloc_t(SV_U64, S_SPLIT_MIN_ALLOC_ELEMS);
	ASSERT_RETURN_IF(!*v, 0);
	const size_t src_size = get_size(src),
		     separator_size = get_size(separator);
	S_ASSERT(src_size > 0 && separator_size > 0);
	if (src_size > 0 && separator_size > 0) {
		size_t i = 0;
		for (; i < src_size;) {
			const size_t off = ss_find(src, i, separator);
			const size_t sz = (off != S_NPOS ? off : src_size) - i;
			if (!sv_push_u(v, i) || !sv_push_u(v, sz)) {
				S_ERROR("not enough memory");
				sv_set_size(*v, 0);
				break;
			}
			if (off == S_NPOS) {	/* no more separators */
				break;
			}
			i = off + separator_size;
		}
	}
	return sv_get_size(*v) / 2;
}

/* #API: |Substring size|substring offset data; n-th element|Size in bytes of the n-th element|O(1)| */

size_t ss_nth_size(const sv_t *offsets, const size_t nth)
{
	RETURN_IF(!offsets, 0);
	const size_t elems = sv_get_size(offsets) / 2;
	return nth < elems ? (size_t)sv_u_at(offsets, nth * 2 + 1) : 0;
}

/* #API: |Substring offset|substring offset data; n-th element|Offset in bytes of the n-th element|O(1)| */

size_t ss_nth_offset(const sv_t *offsets, const size_t nth)
{
	RETURN_IF(!offsets, 0);
	return (size_t)sv_u_at(offsets, nth * 2);
}

/*
 * Format
 */

/* #API: |printf operation on string|output string; printf "format"; printf "format" parameters|output string reference (optional usage)|O(n)| */

int ss_printf(ss_t **s, size_t size, const char *fmt, ...)
{
	int out_size = -1;
	if (s && ss_reserve(s, size) >= size) {
		if (*s)
			ss_clear(s);
		va_list ap;
		va_start(ap, fmt);
		if (ss_cat_printf_va(s, size, fmt, ap))
			out_size = (int)get_size(*s); /* BEHAVIOR */
		va_end(ap);
	}
	return out_size;
}

/*
 * Compare
 */

/* #API: |String compare|string 1; string 2|0: s1 = s2; < 0: s1 lower than s2; > 0: s1 greater than s2|O(n)| */

int ss_cmp(const ss_t *s1, const ss_t *s2)
{
	return ss_ncmp(s1, 0, s2, get_cmp_size(s1, s2));
}

/* #API: |Case-insensitive string compare|string 1; string 2|0: s1 = s2; < 0: s1 lower than s2; > 0: s1 greater than s2|O(n)| */

int ss_cmpi(const ss_t *s1, const ss_t *s2)
{
	return ss_ncmpi(s1, 0, s2, get_cmp_size(s1, s2));
}

/* #API: |Partial string compare|string 1; string 1 offset (bytes; string 2; comparison size (bytes)|0: s1 = s2; < 0: s1 lower than s2; > 0: s1 greater than s2|O(n)| */

int ss_ncmp(const ss_t *s1, const size_t s1off, const ss_t *s2, const size_t n)
{
	int res = 0;
	S_ASSERT(s1 && s2);
	if (s1 && s2) {
		const size_t s1_size = get_size(s1),
			     s2_size = get_size(s2),
			     s1_n = S_MIN(s1_size, s1off + n) - s1off,
			     s2_n = S_MIN(s2_size, n);
		res = memcmp(get_str_r(s1) + s1off, get_str_r(s2),
			     S_MIN(s1_n, s2_n));
		if (res == 0 && s1_n != s2_n)
			res = s1_n < s2_n ? -1 : 1;
	} else { /* #BEHAVIOR: comparing null input */
		res = s1 == s2 ? 0 : s1 ? 1 : -1;
	}
	return res;
}

/* #API: |Partial case insensitive string compare|string 1; string 1 offset (bytes; string 2; comparison size (bytes)|0: s1 = s2; < 0: s1 lower than s2; > 0: s1 greater than s2|O(n)| */

int ss_ncmpi(const ss_t *s1, const size_t s1off, const ss_t *s2, const size_t n)
{
	int res = 0;
	S_ASSERT(s1 && s2);
	if (s1 && s2) {
		const size_t s1_size = get_size(s1),
			     s2_size = get_size(s2),
			     s1_max = S_MIN(s1_size, s1off + n),
			     s2_max = S_MIN(s2_size, n);
		const char *s1_str = get_str_r(s1), *s2_str = get_str_r(s2);
		size_t i = s1off, j = 0;
		int u1 = 0, u2 = 0, utf8_cut = 0;
		for (; i < s1_max && j < s2_max;) {
			const size_t chs1 = ss_utf8_to_wc(s1_str, i,
						s1_max, &u1, (ss_t *)s1),
				     chs2 = ss_utf8_to_wc(s2_str, j,
						s2_max, &u2, (ss_t *)s2);
			if ((i + chs1) > s1_max || (j + chs2) > s2_max) {
				res = chs1 == chs2 ? 0 : chs1 < chs2 ? -1 : 1;
				utf8_cut = 1;
				/* BEHAVIOR: ignore last cutted chars */
				break;
			}
			if ((res = towlower(u1) - towlower(u2)) != 0) {
				break;	/* difference found */
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
 * I/O: Unicode
 */

/* #API: |Get next Unicode character|input string; iterator|Output character, or EOF if no more characters left|O(1)| */

int ss_getchar(const ss_t *s, size_t *autoinc_off)
{
	if (!s || !autoinc_off)
		return EOF;
	const size_t ss = get_size(s);
	if (*autoinc_off >= ss)
		return EOF;
	int c = EOF;
	const char *p = get_str_r(s);
	*autoinc_off += ss_utf8_to_wc(p, *autoinc_off, ss, &c, (ss_t *)s);
	return c;
}

/* #API: |Put/add character into string|output string; Unicode character|Echo of the output character or EOF if overflow error|O(1)| */

int ss_putchar(ss_t **s, const int c)
{
	return (s && ss_cat_char(s, c) && get_size(*s) > 0) ? c : EOF;
}

/* #API: |Extract last character from string|input/output string|Extracted character if OK, EOF if empty|O(1)| */

int ss_popchar(ss_t **s)
{
	if (!s || !*s || get_size(*s) == 0)
		return EOF;
	size_t off = get_size(*s) - 1;
	char *s_str = get_str(*s);
	for (; off != -1; off--) {
		if (SSU8_VALID_START(s_str[off])) {
			int u_char = EOF;
			ss_utf8_to_wc(s_str, off, get_size(*s), &u_char, *s);
			set_size(*s, off);
			dec_unicode_size(*s, 1);
			return u_char;
		}
	}
	return EOF;
}

/*
 * Hashing
 */

/* #API: |Simple hash: 32-bit checksum|string; 0: compare all string; 1 <= n < N compare first N elements |32-bit hash|O(n)| */

unsigned ss_csum32(const ss_t *s, const size_t n)
{
	RETURN_IF(!s, 0);
	const size_t ss = ss_get_size(s), cmpsz = n ? S_MIN(ss, n) : ss;
	return sh_csum32(get_str_r(s), cmpsz);
}

/*
 * Aux
 */

/* #API: |Empty string reference||output string reference|O(1)| */

const ss_t *ss_empty()
{
	return ss_void;
}


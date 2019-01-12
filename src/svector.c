/*
 * svector.c
 *
 * Vector handling.
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "svector.h"
#include "saux/scommon.h"
#include "saux/ssort.h"

#ifndef SV_DEFAULT_SIGNED_VAL
#define SV_DEFAULT_SIGNED_VAL 0
#endif
#ifndef SV_DEFAULT_UNSIGNED_VAL
#define SV_DEFAULT_UNSIGNED_VAL 0
#endif

/*
 * Static functions forward declaration
 */

static srt_data *aux_dup_sd(const srt_data *d);
static srt_vector *sv_check(srt_vector **v);

/*
 * Constants
 */

#define sv_void (srt_vector *)sd_void

/*
 * Push-related function pointers
 */

#define SV_STx(f, TS, TL)                                                      \
	static void f(void *st, TL *c)                                         \
	{                                                                      \
		*(TS *)(st) = (TS) * (c);                                      \
	}

/* clang-format off */
SV_STx(svsti8, char, const int64_t)
SV_STx(svstu8, unsigned char, const uint64_t)
SV_STx(svsti16, short, const int64_t)
SV_STx(svstu16, unsigned short, const uint64_t)
SV_STx(svsti32, int32_t, const int64_t)
SV_STx(svstu32, uint32_t, const uint64_t)
SV_STx(svsti64, int64_t, const int64_t)
SV_STx(svstu64, uint64_t, const uint64_t)
typedef void (*T_SVSTX)(void *, const int64_t *);
/* clang-format on */

static T_SVSTX svstx_f[SV_LAST_INT + 1] = {
	svsti8,  (T_SVSTX)svstu8,  svsti16, (T_SVSTX)svstu16,
	svsti32, (T_SVSTX)svstu32, svsti64, (T_SVSTX)svstu64};

/*
 * Pop-related function pointers
 */

#define SV_LDx(f, TL, TO)                                                      \
	static TO f(const void *ld, size_t index)                              \
	{                                                                      \
		return (TO)(((TL *)ld)[index]);                                \
	}

/* clang-format off */
SV_LDx(svldi8, const signed char, int64_t)
SV_LDx(svldu8, const unsigned char, uint64_t)
SV_LDx(svldi16, const short, int64_t)
SV_LDx(svldu16, const unsigned short, uint64_t)
SV_LDx(svldi32, const int32_t, int64_t)
SV_LDx(svldu32, const uint32_t, uint64_t)
SV_LDx(svldi64, const int64_t, int64_t)
SV_LDx(svldu64, const uint64_t, uint64_t)
typedef int64_t (*T_SVLDX)(const void *, size_t);
/* clang-format on */

static T_SVLDX svldx_f[SV_LAST_INT + 1] = {
	svldi8,  (T_SVLDX)svldu8,  svldi16, (T_SVLDX)svldu16,
	svldi32, (T_SVLDX)svldu32, svldi64, (T_SVLDX)svldu64};

/*
 * Internal functions
 */

#define BUILD_CMP_I(FN, T)                                                     \
	static int FN(const void *a, const void *b)                            \
	{                                                                      \
		int64_t r = *((const T *)a) - *((const T *)b);                 \
		return r < 0 ? -1 : r > 0 ? 1 : 0;                             \
	}

#define BUILD_CMP_U(FN, T)                                                     \
	static int FN(const void *a, const void *b)                            \
	{                                                                      \
		T a2 = *((const T *)a), b2 = *((const T *)b);                  \
		return a2 < b2 ? -1 : a2 == b2 ? 0 : 1;                        \
	}

BUILD_CMP_I(__sv_cmp_i8, signed char)
BUILD_CMP_I(__sv_cmp_i16, short)
BUILD_CMP_I(__sv_cmp_i32, int)
BUILD_CMP_I(__sv_cmp_i64, int64_t)
BUILD_CMP_U(__sv_cmp_u8, unsigned char)
BUILD_CMP_U(__sv_cmp_u16, unsigned short)
BUILD_CMP_U(__sv_cmp_u32, unsigned int)
BUILD_CMP_U(__sv_cmp_u64, uint64_t)

static uint8_t svt_sizes[SV_LAST_INT + 1] = {
	sizeof(char),		sizeof(unsigned char), sizeof(short),
	sizeof(unsigned short), sizeof(int),	   sizeof(unsigned int),
	sizeof(int64_t),	sizeof(uint64_t)};

static srt_vector_cmp svt_cmpf[SV_LAST_INT + 1] = {
	__sv_cmp_i8,  __sv_cmp_u8,  __sv_cmp_i16, __sv_cmp_u16,
	__sv_cmp_i32, __sv_cmp_u32, __sv_cmp_i64, __sv_cmp_u64};

static srt_vector *sv_alloc_base(enum eSV_Type t, size_t elem_size,
				 size_t init_size, const srt_vector_cmp f)
{
	size_t alloc_size = sd_alloc_size_raw(sizeof(srt_vector), elem_size,
					      init_size, S_FALSE);
	void *buf = s_malloc(alloc_size);
	srt_vector *v = sv_alloc_raw(t, S_FALSE, buf, elem_size, init_size, f);
	if (!v || v == sv_void)
		s_free(buf);
	return v;
}

static void sv_copy_elems(srt_vector *v, size_t v_off, const srt_vector *src,
			  size_t src_off, size_t n)
{
	if (v)
		s_copy_elems(sv_get_buffer(v), v_off, sv_get_buffer_r(src),
			     src_off, n, v->d.elem_size);
}

static void sv_move_elems(srt_vector *v, size_t v_off, const srt_vector *src,
			  size_t src_off, size_t n)
{
	if (v && src)
		s_move_elems(sv_get_buffer(v), v_off, sv_get_buffer_r(src),
			     src_off, n, v->d.elem_size);
}

static srt_vector *sv_check(srt_vector **v)
{
	ASSERT_RETURN_IF(!v || !*v, sv_void);
	return *v;
}

void sv_clear(srt_vector *v)
{
	if (v)
		sv_set_size(v, 0);
}

static srt_vector *aux_dup(const srt_vector *src, size_t n_elems)
{
	size_t ss = sv_size(src), size = n_elems < ss ? n_elems : ss;
	srt_vector *v =
		src->d.sub_type == SV_GEN
			? sv_alloc(src->d.elem_size, ss, src->vx.cmpf)
			: sv_alloc_t((enum eSV_Type)src->d.sub_type, ss);
	if (v) {
		sv_copy_elems(v, 0, src, 0, size);
		sv_set_size(v, size);
	} else {
		v = sv_void; /* alloc error */
	}
	return v;
}

static srt_data *aux_dup_sd(const srt_data *d)
{
	return (srt_data *)aux_dup((const srt_vector *)d, sd_size(d));
}

static size_t aux_reserve(srt_vector **v, const srt_vector *src,
			  size_t max_size)
{
	const srt_vector *s;
	ASSERT_RETURN_IF(!v, 0);
	s = *v && *v != sv_void ? *v : NULL;
	if (s == NULL) { /* reserve from NULL/sv_void */
		ASSERT_RETURN_IF(!src, 0);
		*v = src->d.sub_type == SV_GEN
			     ? sv_alloc(src->d.elem_size, max_size,
					src->vx.cmpf)
			     : sv_alloc_t((enum eSV_Type)src->d.sub_type,
					  max_size);
		return sv_max_size(*v);
	}
	return sd_reserve((srt_data **)v, max_size, 0);
}

static srt_vector *aux_cat(srt_vector **v, srt_bool cat, const srt_vector *src,
			   size_t ss0)
{
	size_t s_src, ss, at, out_size, raw_space, new_max_size;
	srt_bool aliasing;
	ASSERT_RETURN_IF(!v, sv_void);
	if (!*v) /* duplicate source */
		return *v = (srt_vector *)aux_dup_sd((const srt_data *)src);
	s_src = sv_size(src);
	ss = s_src < ss0 ? s_src : ss0;
	if (src && (*v)->d.sub_type == src->d.sub_type) {
		aliasing = *v == src;
		at = (cat && *v) ? sv_size(*v) : 0;
		if (s_size_t_overflow(at, ss)) {
			sd_set_alloc_errors((srt_data *)*v);
			return *v;
		}
		out_size = at + ss;
		if (aux_reserve(v, src, out_size) >= out_size) {
			if (!aliasing)
				sv_copy_elems(*v, at, src, 0, ss);
			else if (at > 0)
				sv_move_elems(*v, at, *v, 0, ss);
			sv_set_size(*v, out_size);
		}
	} else {
		RETURN_IF(cat, sv_check(v)); /* BEHAVIOR: cat wrong type */
		sv_clear(*v);
		RETURN_IF(!src, sv_check(v)); /* BEHAVIOR: null input */
		raw_space = (*v)->d.elem_size * (*v)->d.max_size;
		new_max_size = raw_space / src->d.elem_size;
		(*v)->d.elem_size = src->d.elem_size;
		(*v)->d.max_size = new_max_size;
		(*v)->d.sub_type = src->d.sub_type;
		(*v)->vx.cmpf = src->vx.cmpf;
		if (sv_reserve(v, ss) >= ss) {
			/* BEHAVIOR: only if enough space */
			sv_copy_elems(*v, 0, src, 0, ss);
			sv_set_size(*v, ss);
		}
	}
	return *v;
}

static srt_vector *aux_erase(srt_vector **v, srt_bool cat,
			     const srt_vector *src, size_t off, size_t n)
{
	size_t ss0, at, at_off, off_n, src_size, erase_size, out_size;
	srt_bool overflow;
	ASSERT_RETURN_IF(!v, sv_void);
	if (!src)
		src = sv_void;
	ss0 = sv_size(src);
	at = (cat && *v) ? sv_size(*v) : 0;
	/* BEHAVIOR: overflow handling */
	overflow = off + n > ss0;
	src_size = overflow ? ss0 - off : n;
	erase_size = s_size_t_sub(s_size_t_sub(ss0, off), src_size);
	off_n = s_size_t_add(off, n, S_NPOS);
	if (*v == src) {	    /* BEHAVIOR: aliasing: copy-only */
		if (off_n >= ss0) { /* tail clean cut */
			sv_set_size(*v, off);
		} else {
			sv_move_elems(*v, off, *v, off_n, erase_size);
			sv_set_size(*v, ss0 - n);
		}
	} else { /* copy or cat */
		at_off = s_size_t_add(at, off, S_NPOS);
		out_size = s_size_t_add(at_off, erase_size, S_NPOS);
		if (aux_reserve(v, src, out_size) >= out_size) {
			sv_copy_elems(*v, at, src, 0, off);
			sv_copy_elems(*v, at_off, src, off_n, erase_size);
			sv_set_size(*v, out_size);
		}
	}
	return sv_check(v);
}

static srt_vector *aux_resize(srt_vector **v, srt_bool cat,
			      const srt_vector *src, size_t n0)
{
	void *po;
	const void *psrc;
	srt_bool aliasing;
	size_t src_size, at, n, out_size, elem_size;
	RETURN_IF(!v, sv_void);
	if (!src) {
		RETURN_IF(cat, sv_check(v));
		sv_clear(*v);
		return *v;
	}
	src_size = sv_size(src);
	at = (cat && *v) ? sv_size(*v) : 0;
	n = n0 < src_size ? n0 : src_size;
	if (!s_size_t_overflow(at, n)) { /* BEHAVIOR don't resize if overflow */
		out_size = at + n;
		aliasing = *v == src;
		if (aux_reserve(v, src, out_size) >= out_size) {
			if (!aliasing) {
				po = sv_get_buffer(*v);
				psrc = sv_get_buffer_r(src);
				elem_size = src->d.elem_size;
				s_copy_elems(po, at, psrc, 0, n, elem_size);
			}
			sv_set_size(*v, out_size);
		}
	}
	return *v;
}

static char *ptr_to_elem(srt_vector *v, size_t i)
{
	return (char *)sv_get_buffer(v) + i * v->d.elem_size;
}

/* WARNING: this is intentionally unprotected.
 */
static const char *ptr_to_elem_r(const srt_vector *v, size_t i)
{
	return (const char *)sv_get_buffer_r(v) + i * v->d.elem_size;
}

uint8_t sv_elem_size(enum eSV_Type t)
{
	return t > SV_LAST_INT ? 0 : svt_sizes[t];
}

/*
 * Allocation
 */

srt_vector *sv_alloc_raw(enum eSV_Type t, srt_bool ext_buf, void *buffer,
			 size_t elem_size, size_t max_size,
			 const srt_vector_cmp f)
{
	srt_vector *v;
	RETURN_IF(!elem_size || !buffer, sv_void);
	v = (srt_vector *)buffer;
	sd_reset((srt_data *)v, sizeof(srt_vector), elem_size, max_size,
		 ext_buf, S_FALSE);
	v->d.sub_type = (uint8_t)t;
	v->vx.cmpf = t <= SV_LAST_INT ? svt_cmpf[t] : f;
	return v;
}

srt_vector *sv_alloc(size_t elem_size, size_t initial_num_elems_reserve,
		     const srt_vector_cmp f)
{
	return sv_alloc_base(SV_GEN, elem_size, initial_num_elems_reserve, f);
}

srt_vector *sv_alloc_t(enum eSV_Type t, size_t initial_num_elems_reserve)
{
	return sv_alloc_base(t, sv_elem_size(t), initial_num_elems_reserve, 0);
}

/*
 * Allocation from other sources: "dup"
 */

srt_vector *sv_dup(const srt_vector *src)
{
	srt_vector *v = NULL;
	return sv_cpy(&v, src);
}

srt_vector *sv_dup_erase(const srt_vector *src, size_t off, size_t n)
{
	srt_vector *v = NULL;
	return aux_erase(&v, S_FALSE, src, off, n);
}

srt_vector *sv_dup_resize(const srt_vector *src, size_t n)
{
	srt_vector *v = NULL;
	return aux_resize(&v, S_FALSE, src, n);
}

/*
 * Assignment
 */

srt_vector *sv_cpy(srt_vector **v, const srt_vector *src)
{
	return aux_cat(v, S_FALSE, src, sv_len(src));
}

srt_vector *sv_cpy_erase(srt_vector **v, const srt_vector *src, size_t off,
			 size_t n)
{
	return aux_erase(v, S_FALSE, src, off, n);
}

srt_vector *sv_cpy_resize(srt_vector **v, const srt_vector *src, size_t n)
{
	return aux_resize(v, S_FALSE, src, n);
}

/*
 * Append
 */

srt_vector *sv_cat_aux(srt_vector **v, const srt_vector *v1, ...)
{
	va_list ap;
	const srt_vector *v0, *next;
	size_t v0s;
	ASSERT_RETURN_IF(!v, sv_void);
	v0 = *v;
	v0s = v0 ? sv_size(v0) : 0;
	va_start(ap, v1);
	next = v1;
	while (!s_varg_tail_ptr_tag(next)) { /* last element tag */
		if (next) {		     /* cat next with aliasing check */
			const srt_vector *nexta = next == v0 ? *v : next;
			size_t nexta_s = next == v0 ? v0s : sv_len(next);
			aux_cat(v, S_TRUE, nexta, nexta_s);
		}
		next = (srt_vector *)va_arg(ap, srt_vector *);
	}
	va_end(ap);
	return sv_check(v);
}

srt_vector *sv_cat_erase(srt_vector **v, const srt_vector *src, size_t off,
			 size_t n)
{
	return aux_erase(v, S_TRUE, src, off, n);
}

srt_vector *sv_cat_resize(srt_vector **v, const srt_vector *src, size_t n)
{
	return aux_resize(v, S_TRUE, src, n);
}

/*
 * Transformation
 */

srt_vector *sv_erase(srt_vector **v, size_t off, size_t n)
{
	return aux_erase(v, S_FALSE, (v ? *v : NULL), off, n);
}

srt_vector *sv_resize(srt_vector **v, size_t n)
{
	return aux_resize(v, S_FALSE, (v ? *v : NULL), n);
}

srt_vector *sv_sort(srt_vector *v)
{
	void *buf;
	size_t buf_size, elem_size;
	RETURN_IF(!v || !v->vx.cmpf, sv_check(v ? &v : NULL));
	buf = (void *)sv_get_buffer(v);
	buf_size = sv_size(v);
	elem_size = v->d.elem_size;
#ifndef S_MINIMAL
	switch (v->d.sub_type) {
	case SV_I8:
		ssort_i8((int8_t *)buf, buf_size);
		break;
	case SV_U8:
		ssort_u8((uint8_t *)buf, buf_size);
		break;
	case SV_I16:
		ssort_i16((int16_t *)buf, buf_size);
		break;
	case SV_U16:
		ssort_u16((uint16_t *)buf, buf_size);
		break;
	case SV_I32:
		ssort_i32((int32_t *)buf, buf_size);
		break;
	case SV_U32:
		ssort_u32((uint32_t *)buf, buf_size);
		break;
	case SV_I64:
		ssort_i64((int64_t *)buf, buf_size);
		break;
	case SV_U64:
		ssort_u64((uint64_t *)buf, buf_size);
		break;
	default:
		qsort(buf, buf_size, elem_size, v->vx.cmpf);
	}
#else
	qsort(buf, buf_size, elem_size, v->vx.cmpf);
#endif
	return v;
}

/*
 * Search
 */

size_t sv_find(const srt_vector *v, size_t off, const void *target)
{
	size_t pos, size, elem_size, off_max, i;
	const void *p;
	RETURN_IF(!v || v->d.sub_type > SV_GEN, S_NPOS);
	pos = S_NPOS;
	size = sv_size(v);
	p = sv_get_buffer_r(v);
	elem_size = v->d.elem_size;
	RETURN_IF(!elem_size, S_NPOS);
	off_max = size * v->d.elem_size;
	i = off * elem_size;
	for (; i < off_max; i += elem_size)
		if (!memcmp((const char *)p + i, target, elem_size))
			return i / elem_size; /* found */
	return pos;
}

#define SV_FIND_iu(v, off, target)                                             \
	char i8;                                                               \
	unsigned char u8;                                                      \
	short i16;                                                             \
	unsigned short u16;                                                    \
	int i32;                                                               \
	unsigned u32;                                                          \
	int64_t i64;                                                           \
	uint64_t u64;                                                          \
	void *src;                                                             \
	RETURN_IF(!v || v->d.sub_type > SV_U64, S_NPOS);                       \
	switch (v->d.sub_type) {                                               \
	case SV_I8:                                                            \
		i8 = (char)target;                                             \
		src = &i8;                                                     \
		break;                                                         \
	case SV_U8:                                                            \
		u8 = (unsigned char)target;                                    \
		src = &u8;                                                     \
		break;                                                         \
	case SV_I16:                                                           \
		i16 = (short)target;                                           \
		src = &i16;                                                    \
		break;                                                         \
	case SV_U16:                                                           \
		u16 = (unsigned short)target;                                  \
		src = &u16;                                                    \
		break;                                                         \
	case SV_I32:                                                           \
		i32 = (int)target;                                             \
		src = &i32;                                                    \
		break;                                                         \
	case SV_U32:                                                           \
		u32 = (unsigned)target;                                        \
		src = &u32;                                                    \
		break;                                                         \
	case SV_I64:                                                           \
		i64 = (int64_t)target;                                         \
		src = &i64;                                                    \
		break;                                                         \
	case SV_U64:                                                           \
		u64 = (uint64_t)target;                                        \
		src = &u64;                                                    \
		break;                                                         \
	default:                                                               \
		src = NULL;                                                    \
	}

size_t sv_find_i(const srt_vector *v, size_t off, int64_t target)
{
	SV_FIND_iu(v, off, target);
	return sv_find(v, off, src);
}

size_t sv_find_u(const srt_vector *v, size_t off, uint64_t target)
{
	SV_FIND_iu(v, off, target);
	return sv_find(v, off, src);
}

/*
 * Compare
 */

int sv_ncmp(const srt_vector *v1, size_t v1off, const srt_vector *v2,
	    size_t v2off, size_t n)
{
	int r;
	const char *v1p, *v2p;
	size_t sv1, sv2, sv1_left, sv2_left, cmp_len, cmp_bytes;
	ASSERT_RETURN_IF(!v1 && !v2, 0);
	ASSERT_RETURN_IF(!v1, -1);
	ASSERT_RETURN_IF(!v2, 1);
	sv1 = sv_size(v1);
	sv2 = sv_size(v2);
	sv1_left = v1off < sv1 ? sv1 - v1off : 0;
	sv2_left = v2off < sv2 ? sv2 - v2off : 0;
	cmp_len = S_MIN3(n, sv1_left, sv2_left);
	cmp_bytes = v1->d.elem_size * cmp_len;
	v1p = ptr_to_elem_r(v1, v1off);
	v2p = ptr_to_elem_r(v2, v2off);
	r = memcmp(v1p, v2p, cmp_bytes);
	return r || cmp_len == n ? r : sv1 > sv2 ? 1 : -1;
}

int sv_cmp(const srt_vector *v, size_t a_off, size_t b_off)
{
	size_t vs;
	ASSERT_RETURN_IF(!v, 0);
	vs = sv_size(v);
	RETURN_IF(a_off >= vs && b_off >= vs, 0);
	RETURN_IF(a_off >= vs, 1);
	RETURN_IF(b_off >= vs, -1);
	return v->vx.cmpf(ptr_to_elem_r(v, a_off), ptr_to_elem_r(v, b_off));
}

/*
 * Vector "at": element access to given position
 */

const void *sv_at(const srt_vector *v, size_t index)
{
	RETURN_IF(!v, sv_void);
	return (const void *)ptr_to_elem_r(v, index);
}

#define SV_AT_INT_CHECK(v)                                                     \
	RETURN_IF(v->d.sub_type > SV_LAST_INT, SV_DEFAULT_SIGNED_VAL)

#define SV_IU_AT(T, def_val)                                                   \
	size_t size;                                                           \
	const void *p;                                                         \
	RETURN_IF(!v, def_val);                                                \
	SV_AT_INT_CHECK(v);                                                    \
	size = sv_size(v);                                                     \
	RETURN_IF(index >= size, def_val);                                     \
	p = sv_get_buffer_r(v);                                                \
	return (T)(svldx_f[(int)v->d.sub_type](p, index));

int64_t sv_at_i(const srt_vector *v, size_t index)
{
	SV_IU_AT(int64_t, SV_DEFAULT_SIGNED_VAL);
}

uint64_t sv_at_u(const srt_vector *v, size_t index)
{
	SV_IU_AT(uint64_t, SV_DEFAULT_UNSIGNED_VAL);
}

#undef SV_IU_AT

	/*
	 * Vector "set": set element value at given position
	 */

#define SV_SET_CHECK(v, index)                                                 \
	RETURN_IF(!v || !*v, S_FALSE);                                         \
	if (index >= sv_size(*v)) {                                            \
		size_t new_size = index + 1;                                   \
		RETURN_IF(sv_reserve(v, new_size) < new_size, S_FALSE);        \
		sv_set_size(*v, new_size);                                     \
	}

#define SV_SET_INT_CHECK(v, index)                                             \
	SV_SET_CHECK(v, index)                                                 \
	RETURN_IF((*v)->d.sub_type > SV_LAST_INT, SV_DEFAULT_SIGNED_VAL)

srt_bool sv_set(srt_vector **v, size_t index, const void *value)
{
	RETURN_IF(!value, S_FALSE);
	SV_SET_CHECK(v, index);
	memcpy(ptr_to_elem(*v, index), value, (*v)->d.elem_size);
	return S_TRUE;
}

#define SV_IU_SET(v, index, val)                                               \
	SV_SET_INT_CHECK(v, index)                                             \
	svstx_f[(int)(*v)->d.sub_type](ptr_to_elem(*v, index),                 \
				       (const int64_t *)&val);

srt_bool sv_set_i(srt_vector **v, size_t index, int64_t value)
{
	SV_IU_SET(v, index, value);
	return S_TRUE;
}

srt_bool sv_set_u(srt_vector **v, size_t index, uint64_t value)
{
	SV_IU_SET(v, index, value);
	return S_TRUE;
}

#undef SV_IU_SET
#undef SV_SET_CHECK
#undef SV_SET_INT_CHECK

	/*
	 * Vector "push": add element in the last position
	 */

#define SV_PUSH_GROW(v, n) RETURN_IF(!v || !sv_grow(v, n), S_FALSE);
#define SV_INT_CHECK(v) RETURN_IF((*v)->d.sub_type > SV_LAST_INT, S_FALSE);
#define SV_PUSH_START_VARS                                                     \
	size_t sz;                                                             \
	char *p
#define SV_PUSH_START(v)                                                       \
	sz = sv_size(*v);                                                      \
	p = ptr_to_elem(*v, sz);

#define SV_PUSH_END(v, n) sv_set_size(*v, sz + n);

srt_bool sv_push_raw(srt_vector **v, const void *src, size_t n)
{
	SV_PUSH_START_VARS;
	size_t elem_size;
	RETURN_IF(!src || !n, S_FALSE);
	SV_PUSH_GROW(v, n);
	SV_PUSH_START(v);
	SV_PUSH_END(v, n);
	elem_size = (*v)->d.elem_size;
	memcpy(p, src, elem_size * n);
	return S_TRUE;
}

size_t sv_push_aux(srt_vector **v, const void *c1, ...)
{
	va_list ap;
	size_t op_cnt;
	const void *next;
	RETURN_IF(!v || !*v, S_FALSE);
	op_cnt = 0;
	va_start(ap, c1);
	next = c1;
	while (!s_varg_tail_ptr_tag(next)) { /* last element tag */
		if (next && sv_push_raw(v, next, 1))
			op_cnt++;
		next = (void *)va_arg(ap, void *);
	}
	va_end(ap);
	return op_cnt;
}

srt_bool sv_push_i(srt_vector **v, int64_t c)
{
	SV_PUSH_START_VARS;
	SV_PUSH_GROW(v, 1);
	SV_INT_CHECK(v);
	SV_PUSH_START(v);
	SV_PUSH_END(v, 1);
	svstx_f[(int)(*v)->d.sub_type](p, &c);
	return S_TRUE;
}

srt_bool sv_push_u(srt_vector **v, uint64_t c)
{
	SV_PUSH_START_VARS;
	SV_PUSH_GROW(v, 1);
	SV_INT_CHECK(v);
	SV_PUSH_START(v);
	SV_PUSH_END(v, 1);
	svstx_f[(int)(*v)->d.sub_type](p, (const int64_t *)&c);
	return S_TRUE;
}

#undef SV_PUSH_START
#undef SV_PUSH_END
#undef SV_PUSH_IU

	/*
	 * Vector "pop": extract element from last position
	 */

#define SV_POP_START(def_val)                                                  \
	size_t sz;                                                             \
	char *p;                                                               \
	ASSERT_RETURN_IF(!v, def_val);                                         \
	sz = sv_size(v);                                                       \
	ASSERT_RETURN_IF(!sz, def_val);                                        \
	p = ptr_to_elem(v, sz - 1);

#define SV_POP_END sv_set_size(v, sz - 1);

#define SV_POP_IU(T)                                                           \
	SV_AT_INT_CHECK(v);                                                    \
	return (T)(svldx_f[(int)v->d.sub_type](p, 0));

void *sv_pop(srt_vector *v)
{
	SV_POP_START(NULL);
	SV_POP_END;
	return p;
}

int64_t sv_pop_i(srt_vector *v)
{
	SV_POP_START(SV_DEFAULT_SIGNED_VAL);
	SV_POP_END;
	SV_POP_IU(int64_t);
}

uint64_t sv_pop_u(srt_vector *v)
{
	SV_POP_START(SV_DEFAULT_UNSIGNED_VAL);
	SV_POP_END;
	SV_POP_IU(uint64_t);
}

/*
 * svector.c
 *
 * Vector handling.
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
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
#ifndef SV_DEFAULT_FLOAT_VAL
#define SV_DEFAULT_FLOAT_VAL 0.0
#endif
#ifndef SV_DEFAULT_DOUBLE_VAL
#define SV_DEFAULT_DOUBLE_VAL 0.0
#endif

/*
 * Internal constants
 */

#ifndef S_MINIMAL
struct SVecCtx {
	ssort_f sortf;
};

struct SVecCtx vec_ctx[SV_NumTypes] = {
	{(ssort_f)ssort_i8},  /*SV_I8*/
	{(ssort_f)ssort_u8},  /*SV_U8*/
	{(ssort_f)ssort_i16}, /*SV_I16*/
	{(ssort_f)ssort_u16}, /*SV_U16*/
	{(ssort_f)ssort_i32}, /*SV_I32*/
	{(ssort_f)ssort_u32}, /*SV_U32*/
	{(ssort_f)ssort_i64}, /*SV_I64*/
	{(ssort_f)ssort_u64}, /*SV_U64*/
	{NULL},		      /*SV_F*/
	{NULL},		      /*SV_D*/
	{NULL}		      /*SV_GEN*/
};
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
 * Internal functions
 */

#define BUILD_CMP(FN, T)                                                       \
	static int FN(const void *a, const void *b)                            \
	{                                                                      \
		T a2, b2;                                                      \
		memcpy(&a2, a, sizeof(T));                                     \
		memcpy(&b2, b, sizeof(T));                                     \
		return a2 < b2 ? -1 : a2 > b2 ? 1 : 0;                         \
	}

BUILD_CMP(__sv_cmp_i8, int8_t)
BUILD_CMP(__sv_cmp_i16, int16_t)
BUILD_CMP(__sv_cmp_i32, int32_t)
BUILD_CMP(__sv_cmp_i64, int64_t)
BUILD_CMP(__sv_cmp_u8, uint8_t)
BUILD_CMP(__sv_cmp_u16, uint16_t)
BUILD_CMP(__sv_cmp_u32, uint32_t)
BUILD_CMP(__sv_cmp_u64, uint64_t)
BUILD_CMP(__sv_cmp_f, float)
BUILD_CMP(__sv_cmp_d, double)

static uint8_t svt_sizes[SV_LAST_NUM + 1] = {
	sizeof(int8_t),  sizeof(uint8_t),  sizeof(int16_t), sizeof(uint16_t),
	sizeof(int32_t), sizeof(uint32_t), sizeof(int64_t), sizeof(uint64_t),
	sizeof(float),   sizeof(double)};

static srt_vector_cmp svt_cmpf[SV_LAST_NUM + 1] = {
	__sv_cmp_i8,  __sv_cmp_u8,  __sv_cmp_i16, __sv_cmp_u16, __sv_cmp_i32,
	__sv_cmp_u32, __sv_cmp_i64, __sv_cmp_u64, __sv_cmp_f,   __sv_cmp_d};

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
	return t > SV_LAST_NUM ? 0 : svt_sizes[t];
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
	v->vx.cmpf = t <= SV_LAST_NUM ? svt_cmpf[t] : f;
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
	if (v->d.sub_type <= SV_U64)
		vec_ctx[v->d.sub_type].sortf(buf, buf_size);
	else
		qsort(buf, buf_size, elem_size, v->vx.cmpf);
#else
	qsort(buf, buf_size, elem_size, v->vx.cmpf);
#endif
	return v;
}

/*
 * Search
 */

static size_t sv_find_aux(const srt_vector *v, size_t off, const void *tgt,
			  size_t tgt_size)
{
	size_t pos, size, elem_size, off_max, i;
	const void *p;
	RETURN_IF(!v || v->d.sub_type > SV_GEN, S_NPOS);
	pos = S_NPOS;
	size = sv_size(v);
	p = sv_get_buffer_r(v);
	elem_size = v->d.elem_size;
	RETURN_IF(!elem_size || (tgt_size && elem_size != tgt_size), S_NPOS);
	off_max = size * v->d.elem_size;
	i = off * elem_size;
	for (; i < off_max; i += elem_size)
		if (!memcmp((const char *)p + i, tgt, elem_size))
			return i / elem_size; /* found */
	return pos;
}

size_t sv_find(const srt_vector *v, size_t off, const void *target)
{
	return sv_find_aux(v, off, target, 0);
}

#define SV_BUILD_FIND(FN, T)                                                   \
	size_t FN(const srt_vector *v, size_t off, T target)                   \
	{                                                                      \
		return sv_find_aux(v, off, (const void *)&target,              \
				   sizeof(target));                            \
	}

SV_BUILD_FIND(sv_find_i8, int8_t)
SV_BUILD_FIND(sv_find_u8, uint8_t)
SV_BUILD_FIND(sv_find_i16, int16_t)
SV_BUILD_FIND(sv_find_u16, uint16_t)
SV_BUILD_FIND(sv_find_i32, int32_t)
SV_BUILD_FIND(sv_find_u32, uint32_t)
SV_BUILD_FIND(sv_find_i64, int64_t)
SV_BUILD_FIND(sv_find_u64, uint64_t)
SV_BUILD_FIND(sv_find_f, float)
SV_BUILD_FIND(sv_find_d, double)

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

#define SV_BUILD_AT(FN, T, DEF)                                                \
	T FN(const srt_vector *v, size_t index)                                \
	{                                                                      \
		T acc;                                                         \
		const void *p;                                                 \
		p = v ? ptr_to_elem_r(v, index) : NULL;                        \
		RETURN_IF(!v, DEF);                                            \
		memcpy(&acc, p, sizeof(acc));                                  \
		return acc;                                                    \
	}

SV_BUILD_AT(sv_at_i8, int8_t, SV_DEFAULT_SIGNED_VAL)
SV_BUILD_AT(sv_at_u8, uint8_t, SV_DEFAULT_UNSIGNED_VAL)
SV_BUILD_AT(sv_at_i16, int16_t, SV_DEFAULT_SIGNED_VAL)
SV_BUILD_AT(sv_at_u16, uint16_t, SV_DEFAULT_UNSIGNED_VAL)
SV_BUILD_AT(sv_at_i32, int32_t, SV_DEFAULT_SIGNED_VAL)
SV_BUILD_AT(sv_at_u32, uint32_t, SV_DEFAULT_UNSIGNED_VAL)
SV_BUILD_AT(sv_at_i64, int64_t, SV_DEFAULT_SIGNED_VAL)
SV_BUILD_AT(sv_at_u64, uint64_t, SV_DEFAULT_UNSIGNED_VAL)
SV_BUILD_AT(sv_at_f, float, SV_DEFAULT_FLOAT_VAL)
SV_BUILD_AT(sv_at_d, double, SV_DEFAULT_DOUBLE_VAL)

/*
 * Vector "set": set element value at given position
 */

static srt_bool sv_set_aux(srt_vector **v, size_t index, const void *value,
			   size_t value_size)
{
	RETURN_IF(!v || !*v || !value
			  || (value_size && value_size != (*v)->d.elem_size),
		  S_FALSE);
	if (index >= sv_size(*v)) {
		size_t new_size = index + 1;
		RETURN_IF(sv_reserve(v, new_size) < new_size, S_FALSE);
		sv_set_size(*v, new_size);
	}
	memcpy(ptr_to_elem(*v, index), value, (*v)->d.elem_size);
	return S_TRUE;
}

srt_bool sv_set(srt_vector **v, size_t index, const void *value)
{
	return sv_set_aux(v, index, value, 0);
}

#define SV_BUILD_SET(FN, T)                                                    \
	srt_bool FN(srt_vector **v, size_t index, T value)                     \
	{                                                                      \
		return sv_set_aux(v, index, &value, sizeof(value));            \
	}

SV_BUILD_SET(sv_set_i8, int8_t)
SV_BUILD_SET(sv_set_u8, uint8_t)
SV_BUILD_SET(sv_set_i16, int16_t)
SV_BUILD_SET(sv_set_u16, uint16_t)
SV_BUILD_SET(sv_set_i32, int32_t)
SV_BUILD_SET(sv_set_u32, uint32_t)
SV_BUILD_SET(sv_set_i64, int64_t)
SV_BUILD_SET(sv_set_u64, uint64_t)
SV_BUILD_SET(sv_set_f, float)
SV_BUILD_SET(sv_set_d, double)

/*
 * Vector "push": add element in the last position
 */

static srt_bool sv_push_raw0(srt_vector **v, const void *src, size_t n,
			     size_t elem_size)
{
	char *p;
	size_t sz;
	RETURN_IF(!src || !n || !v || !sv_grow(v, n), S_FALSE);
	sz = sv_size(*v);
	p = ptr_to_elem(*v, sz);
	sv_set_size(*v, sz + n);
	RETURN_IF(elem_size && elem_size != (*v)->d.elem_size, S_FALSE);
	memcpy(p, src, (*v)->d.elem_size * n);
	return S_TRUE;
}

srt_bool sv_push_raw(srt_vector **v, const void *src, size_t n)
{
	return sv_push_raw0(v, src, n, 0);
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
		if (next && sv_push_raw0(v, next, 1, 0))
			op_cnt++;
		next = (void *)va_arg(ap, void *);
	}
	va_end(ap);
	return op_cnt;
}

#define SV_BUILD_PUSH(FN, T)                                                   \
	srt_bool FN(srt_vector **v, T value)                                   \
	{                                                                      \
		return sv_push_raw0(v, (const void *)&value, 1,                \
				    sizeof(value));                            \
	}

SV_BUILD_PUSH(sv_push_i8, int8_t)
SV_BUILD_PUSH(sv_push_u8, uint8_t)
SV_BUILD_PUSH(sv_push_i16, int16_t)
SV_BUILD_PUSH(sv_push_u16, uint16_t)
SV_BUILD_PUSH(sv_push_i32, int32_t)
SV_BUILD_PUSH(sv_push_u32, uint32_t)
SV_BUILD_PUSH(sv_push_i64, int64_t)
SV_BUILD_PUSH(sv_push_u64, uint64_t)
SV_BUILD_PUSH(sv_push_f, float)
SV_BUILD_PUSH(sv_push_d, double)

/*
 * Vector "pop": extract element from last position
 */

void *sv_pop(srt_vector *v)
{
	char *p;
	size_t sz;
	ASSERT_RETURN_IF(!v, NULL);
	sz = sv_size(v);
	ASSERT_RETURN_IF(!sz, NULL);
	p = ptr_to_elem(v, sz - 1);
	sv_set_size(v, sz - 1);
	return p;
}

#define SV_BUILD_POP(FN, T, DEF_VAL)                                           \
	T FN(srt_vector *v)                                                    \
	{                                                                      \
		T acc;                                                         \
		void *p = sv_pop(v);                                           \
		RETURN_IF(!p, DEF_VAL);                                        \
		memcpy(&acc, p, sizeof(T));                                    \
		return acc;                                                    \
	}

SV_BUILD_POP(sv_pop_i8, int8_t, SV_DEFAULT_SIGNED_VAL)
SV_BUILD_POP(sv_pop_u8, uint8_t, SV_DEFAULT_UNSIGNED_VAL)
SV_BUILD_POP(sv_pop_i16, int16_t, SV_DEFAULT_SIGNED_VAL)
SV_BUILD_POP(sv_pop_u16, uint16_t, SV_DEFAULT_UNSIGNED_VAL)
SV_BUILD_POP(sv_pop_i32, int32_t, SV_DEFAULT_SIGNED_VAL)
SV_BUILD_POP(sv_pop_u32, uint32_t, SV_DEFAULT_UNSIGNED_VAL)
SV_BUILD_POP(sv_pop_i64, int64_t, SV_DEFAULT_SIGNED_VAL)
SV_BUILD_POP(sv_pop_u64, uint64_t, SV_DEFAULT_UNSIGNED_VAL)
SV_BUILD_POP(sv_pop_f, float, SV_DEFAULT_FLOAT_VAL)
SV_BUILD_POP(sv_pop_d, double, SV_DEFAULT_DOUBLE_VAL)

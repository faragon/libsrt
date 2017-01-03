/*
 * svector.c
 *
 * Vector handling.
 *
 * Copyright (c) 2015-2017, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
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

static sd_t *aux_dup_sd(const sd_t *d);
static sv_t *sv_check(sv_t **v);

/*
 * Constants
 */

#define sv_void (sv_t *)sd_void

/*
 * Push-related function pointers
 */

#define SV_STx(f, TS, TL)			\
	static void f(void *st, TL *c)		\
	{					\
		*(TS *)(st) = (TS)*(c);		\
	}

SV_STx(svsti8, char, const int64_t)
SV_STx(svstu8, unsigned char, const uint64_t)
SV_STx(svsti16, short, const int64_t)
SV_STx(svstu16, unsigned short, const uint64_t)
SV_STx(svsti32, int32_t, const int64_t)
SV_STx(svstu32, uint32_t, const uint64_t)
SV_STx(svsti64, int64_t, const int64_t)
SV_STx(svstu64, uint64_t, const uint64_t)

typedef void (*T_SVSTX)(void *, const int64_t *);

static T_SVSTX svstx_f[SV_LAST_INT + 1] = {	svsti8, (T_SVSTX)svstu8,
						svsti16, (T_SVSTX)svstu16,
						svsti32, (T_SVSTX)svstu32,
						svsti64, (T_SVSTX)svstu64
					};

/*
 * Pop-related function pointers
 */

#define SV_LDx(f, TL, TO)						\
	static TO f(const void *ld, const size_t index)			\
	{								\
		return (TO)(((TL *)ld)[index]);				\
	}

SV_LDx(svldi8, const signed char, int64_t)
SV_LDx(svldu8, const unsigned char, uint64_t)
SV_LDx(svldi16, const short, int64_t)
SV_LDx(svldu16, const unsigned short, uint64_t)
SV_LDx(svldi32, const int32_t, int64_t)
SV_LDx(svldu32, const uint32_t, uint64_t)
SV_LDx(svldi64, const int64_t, int64_t)
SV_LDx(svldu64, const uint64_t, uint64_t)

typedef int64_t (*T_SVLDX)(const void *, const size_t);

static T_SVLDX svldx_f[SV_LAST_INT + 1] = { svldi8, (T_SVLDX)svldu8,
					    svldi16, (T_SVLDX)svldu16,
					    svldi32, (T_SVLDX)svldu32,
					    svldi64, (T_SVLDX)svldu64
					  };

/*
 * Internal functions
 */

#define BUILD_CMP_I(FN, T)					\
	static int FN(const void *a, const void *b) {		\
		int64_t r = *((const T *)a) - *((const T *)b);	\
		return r < 0 ? -1 : r > 0 ? 1 : 0;		\
	}

#define BUILD_CMP_U(FN, T)					\
	static int FN(const void *a, const void *b) {		\
		T a2 = *((const T *)a), b2 = *((const T *)b);	\
		return a2 < b2 ? -1 : a2 == b2 ? 0 : 1;		\
	}

BUILD_CMP_I(__sv_cmp_i8, signed char)
BUILD_CMP_I(__sv_cmp_i16, short)
BUILD_CMP_I(__sv_cmp_i32, int)
BUILD_CMP_I(__sv_cmp_i64, int64_t)
BUILD_CMP_U(__sv_cmp_u8, unsigned char)
BUILD_CMP_U(__sv_cmp_u16, unsigned short)
BUILD_CMP_U(__sv_cmp_u32, unsigned int)
BUILD_CMP_U(__sv_cmp_u64, uint64_t)

static uint8_t svt_sizes[SV_LAST_INT + 1] = {	sizeof(char),
						sizeof(unsigned char),
						sizeof(short),
						sizeof(unsigned short),
						sizeof(int),
						sizeof(unsigned int),
						sizeof(int64_t),
						sizeof(uint64_t) };

static sv_cmp_t svt_cmpf[SV_LAST_INT + 1] = {	__sv_cmp_i8, __sv_cmp_u8,
						__sv_cmp_i16, __sv_cmp_u16,
						__sv_cmp_i32, __sv_cmp_u32,
						__sv_cmp_i64, __sv_cmp_u64 };

static sv_t *sv_alloc_base(const enum eSV_Type t, const size_t elem_size,
			   const size_t init_size,
			   const sv_cmp_t f)
{
	const size_t alloc_size = sd_alloc_size_raw(sizeof(sv_t), elem_size,
						    init_size, S_FALSE);
	return sv_alloc_raw(t, S_FALSE, s_malloc(alloc_size),
			    elem_size, init_size, f);
}

static void sv_copy_elems(sv_t *v, const size_t v_off, const sv_t *src,
		     	  const size_t src_off, const size_t n)
{
	if (v)
		s_copy_elems(sv_get_buffer(v), v_off, sv_get_buffer_r(src),
			     src_off, n, v->d.elem_size);
}

static void sv_move_elems(sv_t *v, const size_t v_off, const sv_t *src,
		     	  const size_t src_off, const size_t n)
{
	if (v && src)
		s_move_elems(sv_get_buffer(v), v_off, sv_get_buffer_r(src),
			     src_off, n, v->d.elem_size);
}

static sv_t *sv_check(sv_t **v)
{
	ASSERT_RETURN_IF(!v || !*v, sv_void);
	return *v;
}

void sv_clear(sv_t *v)
{
	if (v)
		sv_set_size(v, 0);
}

static sv_t *aux_dup(const sv_t *src, const size_t n_elems)
{
	const size_t ss = sv_size(src),
		     size = n_elems < ss ? n_elems : ss;
	sv_t *v = src->d.sub_type == SV_GEN ?
			sv_alloc(src->d.elem_size, ss, src->vx.cmpf) :
			sv_alloc_t((enum eSV_Type)src->d.sub_type, ss);
	if (v) {
		sv_copy_elems(v, 0, src, 0, size);
		sv_set_size(v, size);
	} else {
		v = sv_void;	/* alloc error */
	}
	return v;
}

static sd_t *aux_dup_sd(const sd_t *d)
{
	return (sd_t *)aux_dup((const sv_t *)d, sd_size(d));
}

static size_t aux_reserve(sv_t **v, const sv_t *src, const size_t max_size)
{
	ASSERT_RETURN_IF(!v, 0);
	const sv_t *s = *v && *v != sv_void ? *v : NULL;
	if (s == NULL) { /* reserve from NULL/sv_void */
		ASSERT_RETURN_IF(!src, 0);
		*v = src->d.sub_type == SV_GEN ?
			sv_alloc(src->d.elem_size, max_size, src->vx.cmpf) :
			sv_alloc_t((enum eSV_Type)src->d.sub_type, max_size);
		return sv_max_size(*v);
	}
	return sd_reserve((sd_t **)v, max_size, 0);
}

static sv_t *aux_cat(sv_t **v, const sbool_t cat, const sv_t *src,
		     const size_t ss0)
{
	ASSERT_RETURN_IF(!v, sv_void);
	if (!*v)  /* duplicate source */
		return *v = (sv_t *)aux_dup_sd((const sd_t *)src);
	const size_t s_src = sv_size(src),
		     ss = s_src < ss0 ? s_src : ss0;
	if (src && (*v)->d.sub_type == src->d.sub_type) {
		const sbool_t aliasing = *v == src;
		const size_t at = (cat && *v) ? sv_size(*v) : 0;
		if (s_size_t_overflow(at, ss)) {
			sd_set_alloc_errors((sd_t *)*v);
			return *v;
		}
		const size_t out_size = at + ss;
		if (aux_reserve(v, src, out_size) >= out_size) {
			if (!aliasing)
				sv_copy_elems(*v, at, src, 0, ss);
			else
				if (at > 0)
					sv_move_elems(*v, at, *v, 0, ss);
			sv_set_size(*v, at + ss);
		}
	} else {
		RETURN_IF(cat, sv_check(v)); /* BEHAVIOR: cat wrong type */
		sv_clear(*v);
		RETURN_IF(!src, sv_check(v)); /* BEHAVIOR: null input */
		size_t raw_space = (*v)->d.elem_size * (*v)->d.max_size,
		       new_max_size = raw_space / src->d.elem_size;
		(*v)->d.elem_size = src->d.elem_size;
		(*v)->d.max_size = new_max_size;
		(*v)->d.sub_type = src->d.sub_type;
		(*v)->vx.cmpf = src->vx.cmpf;
		if (sv_reserve(v, ss) >= ss) { /* BEHAVIOR: only if enough space */
			sv_copy_elems(*v, 0, src, 0, ss);
			sv_set_size(*v, ss);
		}
	}
	return *v;
}

static sv_t *aux_erase(sv_t **v, const sbool_t cat, const sv_t *src,
                       const size_t off, const size_t n)
{
	ASSERT_RETURN_IF(!v, sv_void);
	if (!src)
		src = sv_void;
	const size_t ss0 = sv_size(src),
	at = (cat && *v) ? sv_size(*v) : 0;
	const sbool_t overflow = off + n > ss0;
	const size_t src_size = overflow ? ss0 - off : n,
		     erase_size = ss0 - off - src_size;
	if (*v == src) { /* BEHAVIOR: aliasing: copy-only */
		if (off + n >= ss0) { /* tail clean cut */
			sv_set_size(*v, off);
		} else {
			sv_move_elems(*v, off, *v, off + n, erase_size);
			sv_set_size(*v, ss0 - n);
		}
	} else { /* copy or cat */
		const size_t out_size = at + off + erase_size;
		if (aux_reserve(v, src, out_size) >= out_size) {
			sv_copy_elems(*v, at, src, 0, off);
			sv_copy_elems(*v, at + off, src, off + n, erase_size);
			sv_set_size(*v, out_size);
		}
	}
	return sv_check(v);
}

static sv_t *aux_resize(sv_t **v, const sbool_t cat, const sv_t *src,
			const size_t n0)
{
	RETURN_IF(!v, sv_void);
	if (!src) {
		RETURN_IF(cat, sv_check(v));
		sv_clear(*v);
		return *v;
	}
	const size_t src_size = sv_size(src),
		     at = (cat && *v) ? sv_size(*v) : 0;
	const size_t n = n0 < src_size ? n0 : src_size;
	if (!s_size_t_overflow(at, n)) { /* overflow check */
		const size_t out_size = at + n;
		const sbool_t aliasing = *v == src;
		if (aux_reserve(v, src, out_size) >= out_size) {
			if (!aliasing) {
				void *po = sv_get_buffer(*v);
				const void *psrc = sv_get_buffer_r(src);
				const size_t elem_size = src->d.elem_size;
				s_copy_elems(po, at, psrc, 0, n, elem_size);
			}
			sv_set_size(*v, out_size);
		}
	}
	return *v;
}

static char *ptr_to_elem(sv_t *v, const size_t i)
{
	return (char *)sv_get_buffer(v) + i * v->d.elem_size;
}

static const char *ptr_to_elem_r(const sv_t *v, const size_t i)
{
	return (const char *)sv_get_buffer_r(v) + i * v->d.elem_size;
}

uint8_t sv_elem_size(const enum eSV_Type t)
{
	return t > SV_LAST_INT ? 0 : svt_sizes[t];
}

/*
 * Allocation
 */

sv_t *sv_alloc_raw(const enum eSV_Type t, const sbool_t ext_buf, void *buffer,
		   const size_t elem_size, const size_t max_size,
		   const sv_cmp_t f)
{
	RETURN_IF(!elem_size || !buffer, sv_void);
        sv_t *v = (sv_t *)buffer;
	sd_reset((sd_t *)v, sizeof(sv_t), elem_size, max_size, ext_buf,
		 S_FALSE);
	v->d.sub_type = t;
	v->vx.cmpf = t <= SV_LAST_INT ? svt_cmpf[t] : f;
	return v;
}

sv_t *sv_alloc(const size_t elem_size, const size_t initial_num_elems_reserve,
		 const sv_cmp_t f)
{
	return sv_alloc_base(SV_GEN, elem_size, initial_num_elems_reserve, f);
}

sv_t *sv_alloc_t(const enum eSV_Type t, const size_t initial_num_elems_reserve)
{
	return sv_alloc_base(t, sv_elem_size(t), initial_num_elems_reserve, 0);
}

/*
 * Allocation from other sources: "dup"
 */

sv_t *sv_dup(const sv_t *src)
{
	sv_t *v = NULL;
	return sv_cpy(&v, src);
}

sv_t *sv_dup_erase(const sv_t *src, const size_t off, const size_t n)
{
	sv_t *v = NULL;
	return aux_erase(&v, S_FALSE, src, off, n);
}

sv_t *sv_dup_resize(const sv_t *src, const size_t n)
{
	sv_t *v = NULL;
	return aux_resize(&v, S_FALSE, src, n);
}

/*
 * Assignment
 */

sv_t *sv_cpy(sv_t **v, const sv_t *src)
{
	return aux_cat(v, S_FALSE, src, sv_len(src));
}

sv_t *sv_cpy_erase(sv_t **v, const sv_t *src, const size_t off, const size_t n)
{
	return aux_erase(v, S_FALSE, src, off, n);
}

sv_t *sv_cpy_resize(sv_t **v, const sv_t *src, const size_t n)
{
	return aux_resize(v, S_FALSE, src, n);
}

/*
 * Append
 */

sv_t *sv_cat_aux(sv_t **v, const sv_t *v1, ...)
{
	ASSERT_RETURN_IF(!v, sv_void);
	const sv_t *v0 = *v;
	const size_t v0s = v0 ? sv_size(v0) : 0;
        va_list ap;
        va_start(ap, v1);
        const sv_t *next = v1;
        while (!s_varg_tail_ptr_tag(next)) { /* last element tag */
                if (next) { /* cat next with aliasing check */
			const sv_t *nexta = next == v0 ? *v : next;
			const size_t nexta_s = next == v0 ? v0s : sv_len(next);
			aux_cat(v, S_TRUE, nexta, nexta_s);
                }
                next = (sv_t *)va_arg(ap, sv_t *);
        }
        va_end(ap);
	return sv_check(v);
}

sv_t *sv_cat_erase(sv_t **v, const sv_t *src, const size_t off, const size_t n)
{
	return aux_erase(v, S_TRUE, src, off, n);
}

sv_t *sv_cat_resize(sv_t **v, const sv_t *src, const size_t n)
{
	return aux_resize(v, S_TRUE, src, n);
}

/*
 * Transformation
 */

sv_t *sv_erase(sv_t **v, const size_t off, const size_t n)
{
	return aux_erase(v, S_FALSE, (v ? *v : NULL), off, n);
}

sv_t *sv_resize(sv_t **v, const size_t n)
{
	return aux_resize(v, S_FALSE, (v ? *v : NULL), n);
}

sv_t *sv_sort(sv_t *v)
{
	RETURN_IF(!v || !v->vx.cmpf, sv_check(v ? &v : NULL));
	void *buf = (void *)sv_get_buffer(v);
	size_t buf_size = sv_size(v), elem_size = v->d.elem_size;
	/*
	 * TODO/FIXME: integer sort optimizations disabled, until validated
	 */
#ifndef S_MINIMAL
	switch (v->d.sub_type) {
	case SV_I8:  ssort_i8((int8_t *)buf, buf_size); break;
	case SV_U8:  ssort_u8((uint8_t *)buf, buf_size); break;
	case SV_I16: ssort_i16((int16_t *)buf, buf_size); break;
	case SV_U16: ssort_u16((uint16_t *)buf, buf_size); break;
	case SV_I32: ssort_i32((int32_t *)buf, buf_size); break;
	case SV_U32: ssort_u32((uint32_t *)buf, buf_size); break;
	case SV_I64: ssort_i64((int64_t *)buf, buf_size); break;
	case SV_U64: ssort_u64((uint64_t *)buf, buf_size); break;
	default:     qsort(buf, buf_size, elem_size, v->vx.cmpf);
	}
#else
	qsort(buf, buf_size, elem_size, v->vx.cmpf);
#endif
	return v;
}

/*
 * Search
 */

size_t sv_find(const sv_t *v, const size_t off, const void *target)
{
	RETURN_IF(!v || v->d.sub_type > SV_GEN, S_NPOS);
	size_t pos = S_NPOS;
	const size_t size = sv_size(v);
	const void *p = sv_get_buffer_r(v);
	const size_t elem_size = v->d.elem_size;
	RETURN_IF(!elem_size, S_NPOS);
	const size_t off_max = size * v->d.elem_size;
	size_t i = off * elem_size;
	for (; i < off_max; i += elem_size)
		if (!memcmp((const char *)p + i, target, elem_size))
			return i / elem_size;	/* found */
	return pos;
}

#define SV_FIND_iu(v, off, target)					\
	RETURN_IF(!v || v->d.sub_type > SV_U64, S_NPOS);		\
	char i8; unsigned char u8; short i16; unsigned short u16;	\
	int i32; unsigned u32; int64_t i64; uint64_t u64;		\
	void *src;							\
	switch (v->d.sub_type) {					\
	case SV_I8: i8 = (char)target; src = &i8; break;		\
	case SV_U8: u8 = (unsigned char)target; src = &u8; break;	\
	case SV_I16: i16 = (short)target; src = &i16; break;		\
	case SV_U16: u16 = (unsigned short)target; src = &u16; break;	\
	case SV_I32: i32 = (int)target; src = &i32; break;		\
	case SV_U32: u32 = (unsigned)target; src = &u32; break;		\
	case SV_I64: i64 = (int64_t)target; src = &i64; break;		\
	case SV_U64: u64 = (uint64_t)target; src = &u64; break;		\
	default: src = NULL;						\
	}

size_t sv_find_i(const sv_t *v, const size_t off, const int64_t target)
{
	SV_FIND_iu(v, off, target);
	return sv_find(v, off, src);
}

size_t sv_find_u(const sv_t *v, const size_t off, const uint64_t target)
{
	SV_FIND_iu(v, off, target);
	return sv_find(v, off, src);
}

#undef SV_FIND_iu

/*
 * Compare
 */

int sv_ncmp(const sv_t *v1, const size_t v1off, const sv_t *v2,
	    const size_t v2off, const size_t n)
{
	ASSERT_RETURN_IF(!v1 && !v2, 0);
	ASSERT_RETURN_IF(!v1, -1);
	ASSERT_RETURN_IF(!v2, 1);
	const size_t sv1 = sv_size(v1), sv2 = sv_size(v2),
		     sv1_left = v1off < sv1 ? sv1 - v1off : 0,
		     sv2_left = v2off < sv2 ? sv2 - v2off : 0,
		     cmp_len = S_MIN3(n, sv1_left, sv2_left),
		     cmp_bytes = v1->d.elem_size * cmp_len;
	const char *v1p = ptr_to_elem_r(v1, v1off),
		   *v2p = ptr_to_elem_r(v2, v2off);
	int r = memcmp(v1p, v2p, cmp_bytes);
	return r || cmp_len == n ? r : sv1 > sv2 ? 1 : -1;
}

int sv_cmp(const sv_t *v, const size_t a_off, const size_t b_off)
{
	ASSERT_RETURN_IF(!v, 0);
	const size_t vs = sv_size(v);
	RETURN_IF(a_off >= vs && b_off >= vs, 0);
	RETURN_IF(a_off >= vs, 1);
	RETURN_IF(b_off >= vs, -1);
	return v->vx.cmpf(ptr_to_elem_r(v, a_off), ptr_to_elem_r(v, b_off));
}

/*
 * Vector "at": element access to given position
 */

const void *sv_at(const sv_t *v, const size_t index)
{
	RETURN_IF(!v, sv_void);
	return (const void *)ptr_to_elem_r(v, index);
}

#define SV_AT_INT_CHECK(v)	\
	RETURN_IF(v->d.sub_type > SV_LAST_INT, SV_DEFAULT_SIGNED_VAL)

#define SV_IU_AT(T, def_val)					\
	RETURN_IF(!v, def_val);					\
	SV_AT_INT_CHECK(v);					\
	const size_t size = sv_size(v);				\
	RETURN_IF(index >= size, def_val);			\
	const void *p = sv_get_buffer_r(v);			\
	return (T)(svldx_f[(int)v->d.sub_type](p, index));

int64_t sv_at_i(const sv_t *v, const size_t index)
{
	SV_IU_AT(int64_t, SV_DEFAULT_SIGNED_VAL);
}

uint64_t sv_at_u(const sv_t *v, const size_t index)
{
	SV_IU_AT(uint64_t, SV_DEFAULT_UNSIGNED_VAL);
}

#undef SV_IU_AT

/*
 * Vector "set": set element value at given position
 */

#define SV_SET_CHECK(v, index)						\
	RETURN_IF(!v || !*v, S_FALSE);					\
	if (index >= sv_size(*v)) {					\
		size_t new_size = index + 1;				\
		RETURN_IF(sv_reserve(v, new_size) < new_size, S_FALSE);	\
		sv_set_size(*v, new_size);				\
	}

#define SV_SET_INT_CHECK(v, index)					\
	SV_SET_CHECK(v, index)						\
	RETURN_IF((*v)->d.sub_type > SV_LAST_INT, SV_DEFAULT_SIGNED_VAL)

sbool_t sv_set(sv_t **v, const size_t index, const void *value)
{
	RETURN_IF(!value, S_FALSE);
	SV_SET_CHECK(v, index);
	memcpy(ptr_to_elem(*v, index), value, (*v)->d.elem_size);
	return S_TRUE;
}

#define SV_IU_SET(v, index, val)					\
	SV_SET_INT_CHECK(v, index)					\
	svstx_f[(int)(*v)->d.sub_type](ptr_to_elem(*v, index),		\
						(const int64_t *)&val);

sbool_t sv_set_i(sv_t **v, const size_t index, int64_t value)
{
	SV_IU_SET(v, index, value);
	return S_TRUE;
}

sbool_t sv_set_u(sv_t **v, const size_t index, uint64_t value)
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

#define SV_PUSH_GROW(v, n)	\
	RETURN_IF(!v || !sv_grow(v, n), S_FALSE);
#define SV_INT_CHECK(v)	\
	RETURN_IF((*v)->d.sub_type > SV_LAST_INT, S_FALSE);
#define SV_PUSH_START(v)		\
	const size_t sz = sv_size(*v);	\
	char *p = ptr_to_elem(*v, sz);

#define SV_PUSH_END(v, n)	\
	sv_set_size(*v, sz + n);

sbool_t sv_push_raw(sv_t **v, const void *src, const size_t n)
{
	RETURN_IF(!src || !n, S_FALSE);
	SV_PUSH_GROW(v, n);
	SV_PUSH_START(v);
	SV_PUSH_END(v, n);
	const size_t elem_size = (*v)->d.elem_size;
	memcpy(p, src, elem_size * n);
	return S_TRUE;
}

size_t sv_push_aux(sv_t **v, const void *c1, ...)
{
	RETURN_IF(!v || !*v, S_FALSE);
	size_t op_cnt = 0;
        va_list ap;
        va_start(ap, c1);
        const void *next = c1;
        while (!s_varg_tail_ptr_tag(next)) { /* last element tag */
                if (next && sv_push_raw(v, next, 1))
			op_cnt++;
                next = (void *)va_arg(ap, void *);
        }
        va_end(ap);
	return op_cnt;
}

sbool_t sv_push_i(sv_t **v, const int64_t c)
{
	SV_PUSH_GROW(v, 1);
	SV_INT_CHECK(v);
	SV_PUSH_START(v);
	SV_PUSH_END(v, 1);
	svstx_f[(int)(*v)->d.sub_type](p, &c);
	return S_TRUE;
}

sbool_t sv_push_u(sv_t **v, const uint64_t c)
{
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

#define SV_POP_START(def_val)			\
	ASSERT_RETURN_IF(!v, def_val);		\
	const size_t sz = sv_size(v);		\
	ASSERT_RETURN_IF(!sz, def_val);		\
	char *p = ptr_to_elem(v, sz - 1);

#define SV_POP_END		\
	sv_set_size(v, sz - 1);

#define SV_POP_IU(T)						\
	SV_AT_INT_CHECK(v);					\
	return (T)(svldx_f[(int)v->d.sub_type](p,  0));

void *sv_pop(sv_t *v)
{
	SV_POP_START(NULL);
	SV_POP_END;
	return p;
}

int64_t sv_pop_i(sv_t *v)
{
	SV_POP_START(SV_DEFAULT_SIGNED_VAL);
	SV_POP_END;
	SV_POP_IU(int64_t);
}

uint64_t sv_pop_u(sv_t *v)
{
	SV_POP_START(SV_DEFAULT_UNSIGNED_VAL);
	SV_POP_END;
	SV_POP_IU(uint64_t);
}


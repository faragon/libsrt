/*
 * svector.c
 *
 * Vector handling.
 *
 * Copyright (c) 2015-2016 F. Aragon. All rights reserved.
 */ 

#include "svector.h"
#include "scommon.h"

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

static size_t svt_sizes[SV_LAST_INT + 1] = {	sizeof(char),
						sizeof(unsigned char),
						sizeof(short),
						sizeof(unsigned short),
						sizeof(int),
						sizeof(unsigned int),
						sizeof(int64_t),
						sizeof(uint64_t)
					};
static struct SVector sv_void0 = EMPTY_SV;
static sv_t *sv_void = (sv_t *)&sv_void0;
static struct sd_conf svf = {	(size_t (*)(const sd_t *))__sv_get_max_size,
				NULL,
				NULL,
				NULL,
				(sd_t *(*)(const sd_t *))aux_dup_sd,
				(sd_t *(*)(sd_t **))sv_check,
				(sd_t *)&sv_void0,
				0,
				0,
				0,
				0,
				0,
				sizeof(struct SVector),
				offsetof(struct SVector, elem_size),
				};

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
	static TO *f(const void *ld, TO *out, const size_t index)	\
	{								\
		 *out = ((TL *)ld)[index];				\
		return out;						\
	}

SV_LDx(svldi8, const char, int64_t)
SV_LDx(svldu8, const unsigned char, uint64_t)
SV_LDx(svldi16, const short, int64_t)
SV_LDx(svldu16, const unsigned short, uint64_t)
SV_LDx(svldi32, const int32_t, int64_t)
SV_LDx(svldu32, const uint32_t, uint64_t)
SV_LDx(svldi64, const int64_t, int64_t)
SV_LDx(svldu64, const uint64_t, uint64_t)

typedef int64_t *(*T_SVLDX)(const void *, int64_t *, const size_t);

static T_SVLDX svldx_f[SV_LAST_INT + 1] = { svldi8, (T_SVLDX)svldu8,
					    svldi16, (T_SVLDX)svldu16,
					    svldi32, (T_SVLDX)svldu32,
					    svldi64, (T_SVLDX)svldu64
					  };

/*
 * Internal functions
 */

#define BUILD_CMP_I(FN, T)				\
	static int FN(const void *a, const void *b) {	\
		int64_t r = *((T *)a) - *((T *)b);	\
		return r < 0 ? -1 : r > 0 ? 1 : 0;	\
	}

#define BUILD_CMP_U(FN, T)				\
	static int FN(const void *a, const void *b) {	\
		T a2 = *((T *)a), b2 = *((T *)b);	\
		return a2 < b2 ? -1 : a2 == b2 ? 0 : 1;	\
	}

BUILD_CMP_I(__sv_cmp_i8, char)
BUILD_CMP_I(__sv_cmp_i16, short)
BUILD_CMP_I(__sv_cmp_i32, int)
BUILD_CMP_I(__sv_cmp_i64, int64_t)
BUILD_CMP_I(__sv_cmp_u8, unsigned char)
BUILD_CMP_I(__sv_cmp_u16, unsigned short)
BUILD_CMP_I(__sv_cmp_u32, unsigned int)
BUILD_CMP_I(__sv_cmp_u64, uint64_t)

static size_t get_size(const sv_t *v)
{
	return ((const struct SData_Full *)v)->size;
}

static void set_size(sv_t *v, const size_t size)
{
	if (v)
		((struct SData_Full *)v)->size = size;
}

static sv_t *sv_alloc_base(const enum eSV_Type t, const size_t elem_size,
			   const size_t initial_num_elems_reserve,
			   const sv_cmp_t f)
{
	const size_t alloc_size = SDV_HEADER_SIZE + elem_size *
						    initial_num_elems_reserve;
	return sv_alloc_raw(t, elem_size, S_FALSE, __sd_malloc(alloc_size),
			    alloc_size, f);
}

static void sv_copy_elems(sv_t *v, const size_t v_off, const sv_t *src,
		     	  const size_t src_off, const size_t n)
{
	s_copy_elems(__sv_get_buffer(v), v_off, __sv_get_buffer_r(src),
		     src_off, n, v->elem_size);
}

static void sv_move_elems(sv_t *v, const size_t v_off, const sv_t *src,
		     	  const size_t src_off, const size_t n)
{
	s_move_elems(__sv_get_buffer(v), v_off, __sv_get_buffer_r(src),
		     src_off, n, v->elem_size);
}

static sv_t *sv_check(sv_t **v)
{
	ASSERT_RETURN_IF(!v || !*v, sv_void);
	return *v;
}

static sv_t *sv_clear(sv_t **v)
{
	ASSERT_RETURN_IF(!v, sv_void);
	if (!*v)
		*v = sv_void;
	else
		set_size(*v, 0);
	return *v;
}

static sv_t *aux_dup(const sv_t *src, const size_t n_elems)
{
	const size_t ss = get_size(src),
		     size = n_elems < ss ? n_elems : ss;
	sv_t *v = src->sv_type == SV_GEN ?
			sv_alloc(src->elem_size, ss, src->cmpf) :
			sv_alloc_t((enum eSV_Type)src->sv_type, ss);
	if (v) {
		v->aux = src->aux;
		v->aux2 = src->aux2;
		sv_copy_elems(v, 0, src, 0, size);
		set_size(v, size);
	} else {
		v = sv_void;	/* alloc error */
	}
	return v;
}

static sd_t *aux_dup_sd(const sd_t *d)
{
	return (sd_t *)aux_dup((const sv_t *)d, sd_get_size(d));
}

static size_t aux_reserve(sv_t **v, const sv_t *src, const size_t max_elems)
{
	ASSERT_RETURN_IF(!v, 0);
	const sv_t *s = *v && *v != sv_void ? *v : NULL;
	if (s == NULL) { /* reserve from NULL/sv_void */
		ASSERT_RETURN_IF(!src, 0);
		*v = src->sv_type == SV_GEN ?
			sv_alloc(src->elem_size, max_elems, src->cmpf) :
			sv_alloc_t((enum eSV_Type)src->sv_type, max_elems);
		return __sv_get_max_size(*v);
	}
	return sd_reserve((sd_t **)v, max_elems, &svf);
}

static sv_t *aux_cat(sv_t **v, const sbool_t cat, const sv_t *src,
		     const size_t ss)
{
	ASSERT_RETURN_IF(!v, sv_void);
	if (!*v)  /* duplicate source */
		return *v = (sv_t *)aux_dup_sd((const sd_t *)src);
	if (src && (*v)->sv_type == src->sv_type) {
		const sbool_t aliasing = *v == src;
		const size_t at = (cat && *v) ? get_size(*v) : 0;
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
			set_size(*v, at + ss);
		}
	} else {
		return cat ? sv_check(v) : sv_clear(v);
	}
	return *v;
}

static size_t aux_grow(sv_t **v, const sv_t *src, const size_t extra_elems)
{
	ASSERT_RETURN_IF(!v, 0);
	ASSERT_RETURN_IF(!*v || *v == sv_void, aux_reserve(v, src,
							   extra_elems));
	return sv_grow(v, extra_elems);
}

static sv_t *aux_erase(sv_t **v, const sbool_t cat, const sv_t *src,
                       const size_t off, const size_t n)
{
	ASSERT_RETURN_IF(!v, sv_void);
	if (!src)
		src = sv_void;
	const size_t ss0 = get_size(src),
	at = (cat && *v) ? get_size(*v) : 0;
	const sbool_t overflow = off + n > ss0;
	const size_t src_size = overflow ? ss0 - off : n,
		     erase_size = ss0 - off - src_size;
	if (*v == src) { /* BEHAVIOR: aliasing: copy-only */
		if (off + n >= ss0) { /* tail clean cut */
			set_size(*v, off);
		} else {
			sv_move_elems(*v, off, *v, off + n, erase_size);
			set_size(*v, ss0 - n);
		}
	} else { /* copy or cat */
		const size_t out_size = at + off + erase_size;
		if (aux_reserve(v, src, out_size) >= out_size) {
			sv_copy_elems(*v, at, src, 0, off);
			sv_copy_elems(*v, at + off, src, off + n, erase_size);
			set_size(*v, out_size);
		}
	}
	return sv_check(v);
}

static sv_t *aux_resize(sv_t **v, const sbool_t cat, const sv_t *src,
			const size_t n0)
{
	ASSERT_RETURN_IF(!v, sv_void);
	ASSERT_RETURN_IF(!src, (cat ? sv_check(v) : sv_clear(v)));
	const size_t src_size = get_size(src),
		     at = (cat && *v) ? get_size(*v) : 0;
	const size_t n = n0 < src_size ? n0 : src_size;
	if (!s_size_t_overflow(at, n)) { /* overflow check */
		const size_t out_size = at + n;
		const sbool_t aliasing = *v == src;
		if (aux_reserve(v, src, out_size) >= out_size) {
			if (!aliasing) {
				void *po = __sv_get_buffer(*v);
				const void *psrc = __sv_get_buffer_r(src);
				const size_t elem_size = src->elem_size;
				s_copy_elems(po, at, psrc, 0, n, elem_size);
			}
			set_size(*v, out_size);
		}
	}
	return *v;
}

static char *ptr_to_elem(sv_t *v, const size_t i)
{
	return (char *)__sv_get_buffer(v) + i * v->elem_size;
}

static const char *ptr_to_elem_r(const sv_t *v, const size_t i)
{
	return (const char *)__sv_get_buffer_r(v) + i * v->elem_size;
}

/*
 * Allocation
 */

sv_t *sv_alloc_raw(const enum eSV_Type t, const size_t elem_size,
		   const sbool_t ext_buf, void *buffer,
		   const size_t buffer_size, const sv_cmp_t f)
{
	RETURN_IF(!elem_size || !buffer, sv_void);
        sv_t *v = (sv_t *)buffer;
	sd_reset((sd_t *)v, S_TRUE, buffer_size, ext_buf);
	v->sv_type = t;
	v->elem_size = elem_size;
	v->aux = v->aux2 = 0;
	v->cmpf = t == SV_I8 ? __sv_cmp_i8 : t == SV_U8 ? __sv_cmp_u8 :
		  t == SV_I16 ? __sv_cmp_i16 : t == SV_U16 ? __sv_cmp_u16 :
		  t == SV_I32 ? __sv_cmp_i32 : t == SV_U32 ? __sv_cmp_u32 :
		  t == SV_I64 ? __sv_cmp_i64 : t == SV_U64 ? __sv_cmp_u64 : f;
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

SD_BUILDFUNCS(sv)

/*
 * Accessors
 */

size_t sv_capacity(const sv_t *v)
{
	return !v || !v->elem_size ? 0 : __sv_get_max_size(v);
}

size_t sv_len_left(const sv_t *v)
{
	return !v || !v->elem_size ? 0 : __sv_get_max_size(v) - sv_len(v);
}

sbool_t sv_set_len(sv_t *v, const size_t elems)
{
	const size_t max_size = __sv_get_max_size(v);
	const sbool_t resize_ok = elems <= max_size ? S_TRUE : S_FALSE;
	sd_set_size((sd_t *)v, S_MIN(elems, max_size));
	return resize_ok;
}

const void *sv_get_buffer_r(const sv_t *v)
{
	return !v ? NULL : __sv_get_buffer_r(v);
}

void *sv_get_buffer(sv_t *v)
{
	return !v ? NULL : __sv_get_buffer(v);
}

size_t sv_get_buffer_size(const sv_t *v)
{
	return v ? sv_len(v) * v->elem_size : 0;
}

size_t sv_elem_size(const enum eSV_Type t)
{
	return t > SV_LAST_INT ? 0 : svt_sizes[t];
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

sv_t *sv_cat_aux(sv_t **v, const size_t nargs, const sv_t *v1, ...)
{
	ASSERT_RETURN_IF(!v, sv_void);
	const sv_t *v0 = *v;
	const size_t v0s = v0 ? get_size(v0) : 0;
	if (aux_grow(v, v1, nargs)) {
		const sv_t *v1a = v1 == v0 ? *v : v1;
		const size_t v1as = v1 == v0 ? v0s : sv_len(v1);
		if (nargs == 1)
			return aux_cat(v, S_TRUE, v1a, v1as);
		size_t i = 1;
		va_list ap;
		va_start(ap, v1);
		aux_cat(v, S_TRUE, v1a, v1as);
		for (; i < nargs; i++) {
			const sv_t *vnext = va_arg(ap, const sv_t *);
			const sv_t *vnexta = vnext == v0 ? *v : vnext;
			const size_t vns = vnext == v0 ? v0s : sv_len(vnext);
			aux_cat(v, S_TRUE, vnexta, vns);
		}
		va_end(ap);
	}
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

sv_t *sv_sort(sv_t **v)
{
	RETURN_IF(!v || !*v || !(*v)->cmpf, sv_check(v));
	qsort(__sv_get_buffer(*v), get_size(*v), (*v)->elem_size, (*v)->cmpf);
	return *v;
}

/*
 * Search
 */

size_t sv_find(const sv_t *v, const size_t off, const void *target)
{
	RETURN_IF(!v || v->sv_type < SV_I8 || v->sv_type > SV_GEN, S_NPOS);
	size_t pos = S_NPOS;
	const size_t size = get_size(v);
	const void *p = __sv_get_buffer_r(v);
	const size_t elem_size = v->elem_size;
	const size_t off_max = size * v->elem_size;
	size_t i = off * elem_size;
	for (; i < off_max; i += elem_size)
		if (!memcmp((const char *)p + i, target, elem_size))
			return i / elem_size;	/* found */
	return pos;
}

#define SV_FIND_iu(v, off, target)					\
	RETURN_IF(!v || v->sv_type < SV_I8 ||				\
		  v->sv_type > SV_U64, S_NPOS);				\
	char i8; unsigned char u8; short i16; unsigned short u16;	\
	int i32; unsigned u32; int64_t i64; uint64_t u64;			\
	void *src;							\
	switch (v->sv_type) {						\
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
	const size_t sv1 = get_size(v1), sv2 = get_size(v2),
		     sv1_left = v1off < sv1 ? sv1 - v1off : 0,
		     sv2_left = v2off < sv2 ? sv2 - v2off : 0,
		     cmp_len = S_MIN3(n, sv1_left, sv2_left),
		     cmp_bytes = v1->elem_size * cmp_len;
	const char *v1p = ptr_to_elem_r(v1, v1off),
		   *v2p = ptr_to_elem_r(v2, v2off);
	int r = memcmp(v1p, v2p, cmp_bytes);
	return r || cmp_len == n ? r : sv1 > sv2 ? 1 : -1;
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
	RETURN_IF(v->sv_type > SV_LAST_INT, SV_DEFAULT_SIGNED_VAL)

#define SV_IU_AT(T, def_val)					\
	RETURN_IF(!v, def_val);					\
	SV_AT_INT_CHECK(v);					\
	const size_t size = get_size(v);			\
	RETURN_IF(index >= size, def_val);			\
	const void *p = __sv_get_buffer_r(v);			\
	int64_t tmp;						\
	return *(T *)(svldx_f[(int)v->sv_type](p, &tmp, index));

int64_t sv_i_at(const sv_t *v, const size_t index)
{
	SV_IU_AT(int64_t, SV_DEFAULT_SIGNED_VAL);
}

uint64_t sv_u_at(const sv_t *v, const size_t index)
{
	SV_IU_AT(uint64_t, SV_DEFAULT_UNSIGNED_VAL);
}

#undef SV_IU_AT

/*
 * Vector "set": set element value at given position
 */

#define SV_SET_CHECK(v, index)						\
	RETURN_IF(!v || !*v, S_FALSE);					\
	if (index >= get_size(*v)) {					\
		size_t new_size = index + 1;				\
		RETURN_IF(sv_reserve(v, new_size) < new_size, S_FALSE);	\
		set_size(*v, new_size);					\
	}

#define SV_SET_INT_CHECK(v, index)					\
	SV_SET_CHECK(v, index)						\
	RETURN_IF((*v)->sv_type > SV_LAST_INT, SV_DEFAULT_SIGNED_VAL)

sbool_t sv_set(sv_t **v, const size_t index, const void *value)
{
	RETURN_IF(!value, S_FALSE);
	SV_SET_CHECK(v, index);
	memcpy(ptr_to_elem(*v, index), value, (*v)->elem_size);
	return S_TRUE;
}

#define SV_IU_SET(v, index, val)					\
	SV_SET_INT_CHECK(v, index)					\
	svstx_f[(int)(*v)->sv_type](ptr_to_elem(*v, index),		\
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
	RETURN_IF((*v)->sv_type > SV_LAST_INT, S_FALSE);
#define SV_PUSH_START(v)		\
	const size_t sz = get_size(*v);	\
	char *p = ptr_to_elem(*v, sz);

#define SV_PUSH_END(v, n)	\
	set_size(*v, sz + n);

sbool_t sv_push_raw(sv_t **v, const void *src, const size_t n)
{
	RETURN_IF(!src || !n, S_FALSE);
	SV_PUSH_GROW(v, n);
	SV_PUSH_START(v);
	SV_PUSH_END(v, n);
	const size_t elem_size = (*v)->elem_size;
	memcpy(p, src, elem_size * n);
	return S_TRUE;
}

size_t sv_push_aux(sv_t **v, const size_t nargs, const void *c1, ...)
{
	RETURN_IF(!v || !*v || !nargs, S_FALSE);
	size_t op_cnt = 0;
	va_list ap;
	if (sv_grow(v, nargs) == 0)
		return op_cnt;
	op_cnt += (sv_push_raw(v, c1, 1) ? 1 : 0);
	if (nargs == 1) /* just one elem: avoid loop */	
		return op_cnt;
	va_start(ap, c1);
	size_t i = 1;
	for (; i < nargs; i++) {
		const void *next = (const void *)va_arg(ap, const void *);
		op_cnt += (sv_push_raw(v, next, 1) ? 1 : 0);
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
	svstx_f[(int)(*v)->sv_type](p, &c);
	return S_TRUE;
}

sbool_t sv_push_u(sv_t **v, const uint64_t c)
{
	SV_PUSH_GROW(v, 1);
	SV_INT_CHECK(v);
	SV_PUSH_START(v);
	SV_PUSH_END(v, 1);
	svstx_f[(int)(*v)->sv_type](p, (const int64_t *)&c);
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
	const size_t sz = get_size(v);		\
	ASSERT_RETURN_IF(!sz, def_val);		\
	char *p = ptr_to_elem(v, sz - 1);

#define SV_POP_END		\
	set_size(v, sz - 1);

#define SV_POP_IU(T)						\
	SV_AT_INT_CHECK(v);					\
	int64_t tmp;						\
	return *(T *)(svldx_f[(int)v->sv_type](p, &tmp, 0));

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


#ifndef SSTRING_H
#define SSTRING_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * sstring.h
 *
 * String handling.
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "scommon.h"
#include "sdata.h"

/*
 * Constants
 */

#define SS_METAINFO_SMALL	sizeof(struct SSTR_Small)
#define SS_METAINFO_FULL	sizeof(struct SSTR_Full)
#define SS_RANGE_SMALL		(S_ALLOC_SMALL - SS_METAINFO_SMALL)
#define SS_RANGE_FULL		(S_ALLOC_FULL - SS_METAINFO_FULL)

/*
 * Macros (not user-serviceable)
 */

#if 1 /* TODO: to be removed (only stest.c relies on it) */
#define SD_SIZE_TO_ALLOC_SIZE(size, small_range_max, mt_small, mt_full) \
	(size + (size <= small_range_max ? mt_small : mt_full))
#define SS_SIZE_TO_ALLOC_SIZE(s_size)					\
	SD_SIZE_TO_ALLOC_SIZE(s_size, SS_RANGE_SMALL,			\
			      SS_METAINFO_SMALL, SS_METAINFO_FULL)
#endif

/*
 * String base structure
 */

struct SSTR_Small
{
	struct SData_Small ds;
	unsigned char unicode_size_l;
	/* string buffer is 4-byte aligned */
	char str[1]; /* +1: '\0' */
};

struct  SSTR_Full
{
	struct SData_Full df;
	size_t unicode_size;
	/* string buffer is "sizeof(size_t)"-byte aligned */
	char str[1]; /* +1: '\0' */
};

#if 0	/* FIXME: rewrite using sv_t vector */
/*
 * Subtring: reference within a string
 */

struct SSUB
{
	size_t off;
	size_t size;
};

/*
 * Substring vector structure (ss_split)
 */

struct SSUBV
{
	struct SData_Full df;
	size_t size;
	size_t max_size;
	size_t elem_size;
	struct SSUB subs[1];
};
#endif

#define EMPTY_SS \
	{ { { 0, 1, 0, 0, 1, 1, 1, 0 }, 0, SS_METAINFO_SMALL }, 0, { 0 } }

#if 0	/* FIXME: rewrite using sv_t vector */
#define EMPTY_SSUB \
	{ 0, 0 }
#define EMPTY_SSUBV \
	{ EMPTY_SData_Full(sizeof(struct SSUBV)), 0, 0, 0, EMPTY_SSUB }
#endif

/*
 * Types
 */

typedef struct SData ss_t; /* "Hidden" structure (accessors are provided) */

/*
 * Variable argument functions
 */

#ifdef S_USE_VA_ARGS
#define ss_free(...) ss_free_aux(S_NARGS_SPW(__VA_ARGS__), __VA_ARGS__)
#define ssv_free(...) ssv_free_aux(S_NARGS_SSVPW(__VA_ARGS__), __VA_ARGS__)
#define ss_cpy_c(s, ...) ss_cpy_c_aux(s, S_NARGS_CR(__VA_ARGS__), __VA_ARGS__)
#define ss_cpy_w(s, ...) ss_cpy_w_aux(s, S_NARGS_WR(__VA_ARGS__), __VA_ARGS__)
#define ss_cat(s, ...) ss_cat_aux(s, S_NARGS_SR(__VA_ARGS__), __VA_ARGS__)
#define ss_cat_c(s, ...) ss_cat_c_aux(s, S_NARGS_CR(__VA_ARGS__), __VA_ARGS__)
#define ss_cat_w(s, ...) ss_cat_w_aux(s, S_NARGS_WR(__VA_ARGS__), __VA_ARGS__)
#else
#define ss_free(...)
#define ssv_free(...)
#define ss_cpy_c(s, ...) return NULL;
#define ss_cpy_w(s, ...) return NULL;
#define ss_cat(s, ...) return NULL;
#define ss_cat_c(s, ...) return NULL;
#define ss_cat_w(s, ...) return NULL;
#endif

/*
 * Allocation
 */

ss_t *ss_alloc(const size_t initial_heap_reserve);
#define	ss_alloca(stack_reserve)					    \
	ss_alloc_into_ext_buf(alloca(SS_SIZE_TO_ALLOC_SIZE(stack_reserve)), \
			      SS_SIZE_TO_ALLOC_SIZE(stack_reserve))
ss_t *ss_alloc_into_ext_buf(void *buffer, const size_t buffer_size);
ss_t *ss_shrink_to_fit(ss_t **s);
size_t ss_grow(ss_t **s, const size_t extra_size);
size_t ss_reserve(ss_t **s, const size_t max_size);
void ss_free_aux(const size_t nargs, ss_t **s, ...);
void ss_free_call(ss_t **s);

#if 0	/* FIXME: rewrite using sv_t vector */
struct SSUBV *ssv_alloc(const size_t max_strings);
struct SSUBV *ssv_reserve(struct SSUBV **v, const size_t max_size);
void ssv_free_aux(const size_t nargs, struct SSUBV **s, ...);
void ssv_free_call(struct SSUBV **s);
#endif

/*
 * Accessors
 */

size_t ss_len(const ss_t *s);
size_t ss_len_u(const ss_t *s);
size_t ss_capacity(const ss_t *s);
size_t ss_len_left(const ss_t *s);
size_t ss_max(const ss_t *s);
void ss_set_len(ss_t *s, const size_t bytes_in_use);
char *ss_get_buffer(ss_t *s);
sbool_t ss_alloc_errors(const ss_t *s);
sbool_t ss_encoding_errors(const ss_t *s);
void ss_clear_errors(ss_t *s);

#if 0	/* FIXME: rewrite using sv_t vector */
size_t ssv_size(const struct SSUBV *v);
size_t ssv_capacity(const struct SSUBV *v);
size_t ssv_get_sub_off(const struct SSUBV *v, const size_t id);
size_t ssv_get_sub_size(const struct SSUBV *v, const size_t id);
const struct SSUB *ssv_get_sub(const struct SSUBV *v, const size_t id);
#endif

/*
 * Allocation from other sources: "dup"
 */

ss_t *ss_dup_s(const ss_t *src);
#if 0   /* FIXME: rewrite using sv_t vector */
ss_t *ss_dup_sub(const ss_t *src, const struct SSUB *sub);
#endif
ss_t *ss_dup_substr(const ss_t *src, const size_t off, const size_t n);
ss_t *ss_dup_substr_u(const ss_t *src, const size_t char_off, const size_t n);
ss_t *ss_dup_cn(const char *src, const size_t src_size);
ss_t *ss_dup_c(const char *src);
ss_t *ss_dup_wn(const wchar_t *src, const size_t src_size);
ss_t *ss_dup_w(const wchar_t *src);
ss_t *ss_dup_int(const sint_t num);
ss_t *ss_dup_tolower(const ss_t *src);
ss_t *ss_dup_toupper(const ss_t *src);
ss_t *ss_dup_tob64(const ss_t *src);
ss_t *ss_dup_tohex(const ss_t *src);
ss_t *ss_dup_toHEX(const ss_t *src);
ss_t *ss_dup_erase(const ss_t *src, const size_t off, const size_t n);
ss_t *ss_dup_erase_u(const ss_t *src, const size_t char_off, const size_t n);
ss_t *ss_dup_replace(const ss_t *src, const size_t off, const ss_t *s1, const ss_t *s2);
ss_t *ss_dup_resize(const ss_t *src, const size_t n, char fill_byte);
ss_t *ss_dup_resize_u(const ss_t *src, const size_t n, int fill_char);
ss_t *ss_dup_trim(const ss_t *src);
ss_t *ss_dup_ltrim(const ss_t *src);
ss_t *ss_dup_rtrim(const ss_t *src);
ss_t *ss_dup_printf(const size_t size, const char *fmt, ...);
ss_t *ss_dup_printf_va(const size_t size, const char *fmt, va_list ap);
ss_t *ss_dup_char(const int c);

/*
 * Assignment
 */

ss_t *ss_cpy(ss_t **s, const ss_t *src);
#if 0   /* FIXME: rewrite using sv_t vector */
ss_t *ss_cpy_sub(ss_t **s, const ss_t *src, const struct SSUB *sub);
#endif
ss_t *ss_cpy_substr(ss_t **s, const ss_t *src, const size_t off, const size_t n);
ss_t *ss_cpy_substr_u(ss_t **s, const ss_t *src, const size_t char_off, const size_t n);
ss_t *ss_cpy_cn(ss_t **s, const char *src, const size_t src_size);
ss_t *ss_cpy_c_aux(ss_t **s, const size_t nargs, const char *s1, ...);
ss_t *ss_cpy_wn(ss_t **s, const wchar_t *src, const size_t src_size);
ss_t *ss_cpy_int(ss_t **s, const sint_t num);
ss_t *ss_cpy_tolower(ss_t **s, const ss_t *src);
ss_t *ss_cpy_toupper(ss_t **s, const ss_t *src);
ss_t *ss_cpy_tob64(ss_t **s, const ss_t *src);
ss_t *ss_cpy_tohex(ss_t **s, const ss_t *src);
ss_t *ss_cpy_toHEX(ss_t **s, const ss_t *src);
ss_t *ss_cpy_erase(ss_t **s, const ss_t *src, const size_t off, const size_t n);
ss_t *ss_cpy_erase_u(ss_t **s, const ss_t *src, const size_t char_off, const size_t n);
ss_t *ss_cpy_replace(ss_t **s, const ss_t *src, const size_t off, const ss_t *s1, const ss_t *s2);
ss_t *ss_cpy_resize(ss_t **s, const ss_t *src, const size_t n, char fill_byte);
ss_t *ss_cpy_resize_u(ss_t **s, const ss_t *src, const size_t n, int fill_char);
ss_t *ss_cpy_trim(ss_t **s, const ss_t *src);
ss_t *ss_cpy_ltrim(ss_t **s, const ss_t *src);
ss_t *ss_cpy_rtrim(ss_t **s, const ss_t *src);
ss_t *ss_cpy_w_aux(ss_t **s, const size_t nargs, const wchar_t *s1, ...);
ss_t *ss_cpy_printf(ss_t **s, const size_t size, const char *fmt, ...);
ss_t *ss_cpy_printf_va(ss_t **s, const size_t size, const char *fmt, va_list ap);
ss_t *ss_cpy_char(ss_t **s, const int c);

/*
 * Append
 */

ss_t *ss_cat_aux(ss_t **s, const size_t nargs, const ss_t *s1, ...);
#if 0   /* FIXME: rewrite using sv_t vector */
ss_t *ss_cat_sub(ss_t **s, const ss_t *src, const struct SSUB *sub);
#endif
ss_t *ss_cat_substr(ss_t **s, const ss_t *src, const size_t off, const size_t n);
ss_t *ss_cat_substr_u(ss_t **s, const ss_t *src, const size_t char_off, const size_t n);
ss_t *ss_cat_cn(ss_t **s, const char *src, const size_t src_size);
ss_t *ss_cat_c_aux(ss_t **s, const size_t nargs, const char *s1, ...);
ss_t *ss_cat_w_aux(ss_t **s, const size_t nargs, const wchar_t *s1, ...);
ss_t *ss_cat_wn(ss_t **s, const wchar_t *src, const size_t src_size);
ss_t *ss_cat_int(ss_t **s, const sint_t num);
ss_t *ss_cat_tolower(ss_t **s, const ss_t *src);
ss_t *ss_cat_toupper(ss_t **s, const ss_t *src);
ss_t *ss_cat_tob64(ss_t **s, const ss_t *src);
ss_t *ss_cat_tohex(ss_t **s, const ss_t *src);
ss_t *ss_cat_toHEX(ss_t **s, const ss_t *src);
ss_t *ss_cat_erase(ss_t **s, const ss_t *src, const size_t off, const size_t n);
ss_t *ss_cat_erase_u(ss_t **s, const ss_t *src, const size_t char_off, const size_t n);
ss_t *ss_cat_replace(ss_t **s, const ss_t *src, const size_t off, const ss_t *s1, const ss_t *s2);
ss_t *ss_cat_resize(ss_t **s, const ss_t *src, const size_t n, char fill_byte);
ss_t *ss_cat_resize_u(ss_t **s, const ss_t *src, const size_t n, int fill_char);
ss_t *ss_cat_trim(ss_t **s, const ss_t *src);
ss_t *ss_cat_ltrim(ss_t **s, const ss_t *src);
ss_t *ss_cat_rtrim(ss_t **s, const ss_t *src);
ss_t *ss_cat_printf(ss_t **s, const size_t size, const char *fmt, ...);
ss_t *ss_cat_printf_va(ss_t **s, const size_t size, const char *fmt, va_list ap);
ss_t *ss_cat_char(ss_t **s, const int c);

/*
 * Transformation
 */

ss_t *ss_tolower(ss_t **s);
ss_t *ss_toupper(ss_t **s);
ss_t *ss_tob64(ss_t **s, const ss_t *src);
ss_t *ss_tohex(ss_t **s, const ss_t *src);
ss_t *ss_toHEX(ss_t **s, const ss_t *src);
sbool_t ss_set_turkish_mode(const int enable_turkish_mode);
ss_t *ss_clear(ss_t **s);
ss_t *ss_check(ss_t **s);
ss_t *ss_erase(ss_t **s, const size_t off, const size_t n);
ss_t *ss_erase_u(ss_t **s, const size_t char_off, const size_t n);
ss_t *ss_replace(ss_t **s, const size_t off, const ss_t *s1, const ss_t *s2);
ss_t *ss_resize(ss_t **s, const size_t n, char fill_byte);
ss_t *ss_resize_u(ss_t **s, const size_t n, int fill_char);
ss_t *ss_trim(ss_t **s);
ss_t *ss_ltrim(ss_t **s);
ss_t *ss_rtrim(ss_t **s);

/*
 * Export
 */

const char *ss_to_c(ss_t *s);
const wchar_t *ss_to_w(const ss_t *s, wchar_t *o, const size_t nmax, size_t *n);

/*
 * Search
 */

size_t ss_find(const ss_t *s, const size_t off, const ss_t *tgt);
#if 0   /* FIXME: rewrite using sv_t vector */
size_t ss_split(struct SSUBV **v, const ss_t *src, const ss_t *separator);
#endif

/*
 * Compare
 */

int ss_cmp(const ss_t *s1, const ss_t *s2);
int ss_cmpi(const ss_t *s1, const ss_t *s2);
int ss_ncmp(const ss_t *s1, const size_t s1off, const ss_t *s2, const size_t n);
int ss_ncmpi(const ss_t *s1, const size_t s1off, const ss_t *s2, const size_t n);

/*
 * snprintf equivalent
 */

int ss_printf(ss_t **s, size_t size, const char *fmt, ...);

/*
 * Character I/O
 */

int ss_getchar(const ss_t *s, size_t *autoinc_off);
int ss_putchar(ss_t **s, const int c);
int ss_popchar(ss_t **s);

/*
 * Aux
 */

const ss_t *ss_empty();

#ifdef S_DEBUG
extern size_t dbg_cnt_alloc_calls;	/* alloc or realloc calls */
#endif

#ifdef __cplusplus
};      /* extern "C" { */
#endif
#endif	/* SSTRING_H */


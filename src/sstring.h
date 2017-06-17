#ifndef SSTRING_H
#define SSTRING_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * sstring.h
 *
 * #SHORTDOC string handling
 *
 * #DOC Provided functions allow efficient operations on strings. Internal
 * #DOC string format is binary, supporting arbitrary data. Operations
 * #DOC on strings involving format interpretation, e.g. string length is
 * #DOC interpreted as UTF-8 when calling to the Unicode function ss_len_u(),
 * #DOC and as raw data when calling the functions not using Unicode
 * #DOC interpretation (ss_len()/ss_size()). Strings below 256 bytes take just
 * #DOC 5 bytes for internal structure, and 5 * sizeof(size_t) for bigger
 * #DOC strings. Unicode-size is cached between operations, when possible, so
 * #DOC in those cases UTF-8 string length computation would be O(1).
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "svector.h"
#include "saux/scommon.h"
#include "saux/sdata.h"

/*
 * String base structure
 *
 * Observations:
 * - The small string container is implemented using struct SDataSmall,
 *   defined in sdata.h. The compatibility is possible because both
 *   struct SDataSmall and struct SDataFull share the header, where
 *   is defined if using the smaller container or the full one.
 * - Usage of struct SDataFlags type-specific elements:
 *	flag1: Unicode size is cached
 *	flag2: string has UTF-8 encoding errors (e.g. after some operation)
 *	flag3: string reference (built using ss_cref[a]() or ss_ref[a]())
 *	flag4: string reference with C terminator (built using ss_cref[a]())
 */

struct SString
{
	struct SDataFull d;
	size_t unicode_size;
};

struct SStringRef
{
	struct SString s;
	const char *cstr;
};

#define SS_RANGE	(sizeof(size_t) - sizeof(ss_t))
#define EMPTY_SS	{ EMPTY_SDataFull, 0 }

/*
 * Types
 */

typedef struct SString ss_t;		/* Opaque structures (accessors are provided) */
typedef struct SStringRef ss_ref_t;

/*
 * Aux
 */

extern ss_t *ss_void;

/*
 * Variable argument functions
 */

#ifdef S_USE_VA_ARGS
#define ss_free(...) ss_free_aux(__VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#define ss_cpy_c(s, ...) ss_cpy_c_aux(s, __VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#define ss_cpy_w(s, ...) ss_cpy_w_aux(s, __VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#define ss_cat(s, ...) ss_cat_aux(s, __VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#define ss_cat_c(s, ...) ss_cat_c_aux(s, __VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#define ss_cat_w(s, ...) ss_cat_w_aux(s, __VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#else
#define ss_free(...)
#define ss_cpy_c(s, ...) return NULL;
#define ss_cpy_w(s, ...) return NULL;
#define ss_cat(s, ...) return NULL;
#define ss_cat_c(s, ...) return NULL;
#define ss_cat_w(s, ...) return NULL;
#endif

/*
 * Generated from template
 */

SD_BUILDFUNCS_DYN(ss, 1)

size_t ss_grow(ss_t **c, const size_t extra_elems);
size_t ss_reserve(ss_t **c, const size_t max_elems);

/*
#API: |Free one or more strings (heap)|string;more strings (optional)|-|O(1)|1;2|
void ss_free(ss_t **s, ...)

#API: |Reserve space for extra elements (relative to current string size)|string;number of extra elements|extra size allocated|O(1)|1;2|
size_t ss_grow(ss_t **s, const size_t extra_elems)

#API: |Reserve space for at least N bytes (absolute reserve)|string;absolute element reserve|reserved elements|O(1)|1;2|
size_t ss_reserve(ss_t **s, const size_t max_elems)

#API: |Free unused space|string|same string (optional usage)|O(1)|1;2|
ss_t *ss_shrink(ss_t **s)

#API: |Get string size|string|string bytes used in UTF8 format|O(1)|1;2|
size_t ss_size(const ss_t *s)

#NOTAPI: |Set string size (bytes used in UTF8 format)|string;new size|-|O(1)|1;2|
void ss_set_size(ss_t *s, const size_t s)

#API: |Equivalent to ss_size|string|Number of bytes (UTF-8 string length)|O(1)|1;2|
size_t ss_len(const ss_t *s)

#API: |Get allocated space|vector|current allocated space (in bytes)|O(1)|1;2|
size_t ss_capacity(const ss_t *s);

#API: |Get preallocated space left|string|allocated space left (in bytes)|O(1)|1;2|
size_t ss_capacity_left(const ss_t *s);

#API: |Tells if a string is empty (zero elements)|string|S_TRUE: empty string; S_FALSE: not empty|O(1)|1;2|
sbool_t ss_empty(const ss_t *s)

#API: |Get string buffer access|string|pointer to the insternal string buffer (UTF-8 or raw data)|O(1)|1;2|
char *ss_get_buffer(ss_t *s);

#API: |Get string buffer access (read-only)|string|pointer to the internal string buffer (UTF-8 or raw data)|O(1)|1;2|
const char *ss_get_buffer_r(const ss_t *s);

#API: |Get string buffer size|string|Number of bytes in use for storing all string characters|O(1)|1;2|
size_t ss_get_buffer_size(const ss_t *v);
*/

/*
 * Allocation
 */

/* #API: |Allocate string (heap)|space preallocated to store n elements|allocated string|O(1)|1;2| */
ss_t *ss_alloc(const size_t initial_heap_reserve);

/*
#API: |Allocate string (stack)|space preallocated to store n elements|allocated string|O(1)|1;2|
ss_t *ss_alloca(const size_t max_size)
 */
/*
 * +1 extra byte allocated for the \0 required by ss_to_c()
 */
#define	ss_alloca(max_size)						   \
	ss_alloc_into_ext_buf(alloca(sd_alloc_size_raw(sizeof(ss_t), 1,	   \
						   max_size, S_TRUE) + 1), \
			      max_size)

ss_t *ss_alloc_into_ext_buf(void *buf, const size_t max_size);

/* #API: |Create a reference from C string. This is intended for avoid duplicating C strings when working with ss_t functions|string reference to be built (can be on heap or stack, it is a small structure); input C string (0 terminated ASCII or UTF-8 string)|ss_t string derived from ss_ref_t|O(1)|1;2| */
const ss_t *ss_cref(ss_ref_t *s_ref, const char *c_str);

/* #API: |Create a reference from C string using implicit stack allocation for the reference handling (be cafeful not using this inside a loop -for loops you can e.g. use ss_build_ref() instead of this, using a local variable allocated in the stack for the reference-)|input C string (0 terminated ASCII or UTF-8 string)|ss_t string derived from ss_ref_t|O(1)|1;2|
const ss_t *ss_crefa(const char *c_str)
*/
#define ss_crefa(c_str)	\
	ss_cref((ss_ref_t *)alloca(sizeof(ss_ref_t)), c_str)

/* #API: |Create a reference from raw data, i.e. not assuming is 0 terminated. WARNING: when using raw references when calling ss_to_c() will return a "" string (safety)), so, if you need a reference to the internal raw buffer use ss_get_buffer_r() instead|string reference to be built (can be on heap or stack, it is a small structure);input raw data buffer;input buffer size (bytes)|ss_t string derived from ss_ref_t|O(1)|1;2| */
const ss_t *ss_ref_buf(ss_ref_t *s_ref, const char *buf, const size_t buf_size);

/* #API: |Create a reference from raw data, i.e. not 0 terminated, using implicit stack allocation for the reference handling (be cafeful not using this inside a loop -for loops you can e.g. use ss_build_ref() instead of this, using a local variable allocated in the stack for the reference-). WARNING: when using raw references when calling ss_to_c() will return a "" string (safety), so, if you need a reference to the internal raw buffer use ss_get_buffer_r() instead|input raw data buffer;input buffer size (bytes)|ss_t string derived from ss_ref_t|O(1)|1;2|
const ss_t *ss_refa_buf(const char *buf, const size_t buf_size)
*/
#define ss_refa_buf(buf, buf_size)	\
	ss_ref_buf((ss_ref_t *)alloca(sizeof(ss_ref_t)), buf, buf_size)

/* #API: |Get string reference from string reference container|string reference container|ss_t string derived from ss_ref_t|O(1)|1;2| */
S_INLINE const ss_t *ss_ref(const ss_ref_t *s_ref)
{
	RETURN_IF(!s_ref, ss_void);
	return &s_ref->s;
}

/*
 * Accessors
 */

/* #API: |Random access to byte|string; offset (bytes)|0..255: byte retrieved ok; < 0: out of range|O(1)|1;2| */
int ss_at(const ss_t *s, size_t off);

/* #API: |String length (Unicode)|string|number of Unicode characters|O(1) if cached, O(n) if not previously computed|1;2| */
size_t ss_len_u(const ss_t *s);

/* #API: |Get the maximum possible string size|string|max string size (bytes)|O(1)|1;2| */
size_t ss_max(const ss_t *s);

/* #NOTAPI: |Normalize offset: cut offset if bigger than string size|string; offset|Normalized offset (range: 0..string size)|O(1)|1;2|
size_t ss_real_off(const ss_t *s, const size_t off);
*/

/* #API: |Check if string had allocation errors|string|S_TRUE: has errors; S_FALSE: no errors|O(1)|1;2|
sbool_t ss_alloc_errors(const ss_t *s);
*/

/* #API: |Check if string had UTF8 encoding errors|string|S_TRUE: has errors; S_FALSE: no errors|O(1)|1;2| */
sbool_t ss_encoding_errors(const ss_t *s);

/* #API: |Clear allocation/encoding error flags|string|-|O(1)|1;2| */
void ss_clear_errors(ss_t *s);

/*
 * Allocation from other sources: "dup"
 */

/* #API: |Duplicate string|string|Output result|O(n)|1;2| */
ss_t *ss_dup(const ss_t *src);

/* #API: |Duplicate from substring|string;byte offset;number of bytes|output result|O(n)|1;2| */
ss_t *ss_dup_substr(const ss_t *src, const size_t off, const size_t n);

/* #API: |Duplicate from substring|string;character offset;number of characters|output result|O(n)|1;2| */
ss_t *ss_dup_substr_u(const ss_t *src, const size_t char_off, const size_t n);

/* #API: |Duplicate from C string|C string buffer;number of bytes|output result|O(n)|1;2| */
ss_t *ss_dup_cn(const char *src, const size_t src_size);

/* #API: |Duplicate from C String (ASCII-z)|C string|output result|O(n)|1;2| */
ss_t *ss_dup_c(const char *src);

/* #API: |Duplicate from Unicode "wide char" string (UTF-16 for 16-bit wchar_t, and UTF-32 for 32-bit wchar_t)|"wide char" string; number of characters|output result|O(n)|1;2| */
ss_t *ss_dup_wn(const wchar_t *src, const size_t src_size);

/* #API: |Duplicate from "wide char" Unicode string (UTF-16 for 16-bit wchar_t, and UTF-32 for 32-bit wchar_t)|"wide char" string|output result|O(n)|1;2| */
ss_t *ss_dup_w(const wchar_t *src);

/* #API: |Duplicate from integer|integer|output result|O(1)|1;2| */
ss_t *ss_dup_int(const int64_t num);

/* #API: |Duplicate string with lowercase conversion|string|output result|O(n)|1;2| */
ss_t *ss_dup_tolower(const ss_t *src);

/* #API: |Duplicate string with uppercase conversion|string|output result|O(n)|1;2| */
ss_t *ss_dup_toupper(const ss_t *src);

/* #API: |Duplicate string with base64 encoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_enc_b64(const ss_t *src);

/* #API: |Duplicate string with hex encoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_enc_hex(const ss_t *src);

/* #API: |Duplicate string with hex encoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_enc_HEX(const ss_t *src);

/* #API: |Duplicate string with LZW encoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_enc_lzw(const ss_t *src);

/* #API: |Duplicate string with RLE encoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_enc_rle(const ss_t *src);

/* #API: |Duplicate string with JSON escape encoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_enc_esc_json(const ss_t *src);

/* #API: |Duplicate string with XML escape encoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_enc_esc_xml(const ss_t *src);

/* #API: |Duplicate string with URL escape encoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_enc_esc_url(const ss_t *src);

/* #API: |Duplicate string escaping " as ""|string|output result|O(n)|1;2| */
ss_t *ss_dup_enc_esc_dquote(const ss_t *src);

/* #API: |Duplicate string escaping ' as ''|string|output result|O(n)|1;2| */
ss_t *ss_dup_enc_esc_squote(const ss_t *src);

/* #API: |Duplicate string with base64 decoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_dec_b64(const ss_t *src);

/* #API: |Duplicate string with hex decoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_dec_hex(const ss_t *src);

/* #API: |Duplicate string with LZW decoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_dec_lzw(const ss_t *src);

/* #API: |Duplicate string with RLE decoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_dec_rle(const ss_t *src);

/* #API: |Duplicate string with JSON escape decoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_dec_esc_json(const ss_t *src);

/* #API: |Duplicate string with XML escape decoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_dec_esc_xml(const ss_t *src);

/* #API: |Duplicate string with URL escape decoding|string|output result|O(n)|1;2| */
ss_t *ss_dup_dec_esc_url(const ss_t *src);

/* #API: |Duplicate string unescaping "" as "|string|output result|O(n)|1;2| */
ss_t *ss_dup_dec_esc_dquote(const ss_t *src);

/* #API: |Duplicate string unescaping '' as '|string|output result|O(n)|1;2| */
ss_t *ss_dup_dec_esc_squote(const ss_t *src);

/* #API: |Duplicate from string erasing portion from input|string;byte offset;number of bytes|output result|O(n)|1;2| */
ss_t *ss_dup_erase(const ss_t *src, const size_t off, const size_t n);

/* #API: |Duplicate from Unicode "wide char" string erasing portion from input|string;character offset;number of characters|output result|O(n)|1;2| */
ss_t *ss_dup_erase_u(const ss_t *src, const size_t char_off, const size_t n);

/* #API: |Duplicate and apply replace operation after offset|string; offset (bytes); needle; needle replacement|output result|O(n)|1;2| */
ss_t *ss_dup_replace(const ss_t *src, const size_t off, const ss_t *s1, const ss_t *s2);

/* #API: |Duplicate and resize (byte addressing)|string; new size (bytes); fill byte|output result|O(n)|1;2| */
ss_t *ss_dup_resize(const ss_t *src, const size_t n, char fill_byte);

/* #API: |Duplicate and resize (Unicode addressing)|string; new size (characters); fill character|output result|O(n)|1;2| */
ss_t *ss_dup_resize_u(ss_t *src, const size_t n, int fill_char);

/* #API: |Duplicate and trim string|string|output result|O(n)|1;2| */
ss_t *ss_dup_trim(const ss_t *src);

/* #API: |Duplicate and apply trim at left side|string|output result|O(n)|1;2| */
ss_t *ss_dup_ltrim(const ss_t *src);

/* #API: |Duplicate and apply trim at right side|string|output result|O(n)|1;2| */
ss_t *ss_dup_rtrim(const ss_t *src);

/* #API: |Duplicate from printf formatting|printf space (bytes); printf format; optional printf parameters|output result|O(n)|1;2| */
ss_t *ss_dup_printf(const size_t size, const char *fmt, ...);

/* #API: |Duplicate from printf_va formatting|printf_va space (bytes); printf format; variable argument reference|output result|O(n)|1;2| */
ss_t *ss_dup_printf_va(const size_t size, const char *fmt, va_list ap);

/* #API: |Duplicate string from character|Unicode character|output result|O(1)|1;2| */
ss_t *ss_dup_char(const int c);

/* #API: |Duplicate from reading from file handle|file handle; read max size (in bytes)|output result|O(n): WARNING: involves external file I/O|1;2| */
ss_t *ss_dup_read(FILE *handle, const size_t max_bytes);

/*
 * Assignment
 */

/* #API: |Overwrite string with a string copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with a substring copy (byte mode)|output string; input string; input string start offset (bytes); number of bytes to be copied|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_substr(ss_t **s, const ss_t *src, const size_t off, const size_t n);

/* #API: |Overwrite string with a substring copy (character mode)|output string; input string; input string start offset (characters); number of characters to be copied|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_substr_u(ss_t **s, const ss_t *src, const size_t char_off, const size_t n);

/* #API: |Overwrite string with C string copy (strict aliasing is assumed)|output string; input string; input string bytes|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_cn(ss_t **s, const char *src, const size_t src_size);

/*
#API: |Overwrite string with multiple C string copy (strict aliasing is assumed)|output string; input strings (one or more C strings)|output string reference (optional usage)|O(n)|1;2|
ss_t *ss_cpy_c(ss_t **s, ...)
*/
ss_t *ss_cpy_c_aux(ss_t **s, const char *s1, ...);

/* #API: |Overwrite string with "wide char" Unicode string copy (strict aliasing is assumed) (UTF-16 for 16-bit wchar_t, and UTF-32 for 32-bit wchar_t)|output string; input string ("wide char" Unicode); input string number of characters|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_wn(ss_t **s, const wchar_t *src, const size_t src_size);

/* #API: |Overwrite string with integer to string copy|output string; integer (any signed integer size)|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_int(ss_t **s, const int64_t num);

/* #API: |Overwrite string with input string lowercase conversion copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_tolower(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string uppercase conversion copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_toupper(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string base64 encoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_enc_b64(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string hexadecimal (lowercase) encoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_enc_hex(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string hexadecimal (uppercase) encoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_enc_HEX(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string LZW encoded copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_enc_lzw(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string RLE encoded copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_enc_rle(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string JSON escape encoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_enc_esc_json(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string XML escape encoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_enc_esc_xml(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string URL escape encoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_enc_esc_url(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string escaping " as ""|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_enc_esc_dquote(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string escaping ' as ''|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_enc_esc_squote(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string base64 decoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_dec_b64(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string hexadecimal (lowercase) decoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_dec_hex(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string LZW decoded copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_dec_lzw(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string RLE decoded copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_dec_rle(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string JSON escape decoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_dec_esc_json(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string XML escape decoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_dec_esc_xml(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string URL escape decoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_dec_esc_url(ss_t **s, const ss_t *src);

/* #API: |Overwrite string unescaping "" as "|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_dec_esc_dquote(ss_t **s, const ss_t *src);

/* #API: |Overwrite string unescaping '' as '|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_dec_esc_squote(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string copy applying a erase operation (byte/UTF-8 mode)|output string; input string; input string erase start byte offset; number of bytes to erase|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_erase(ss_t **s, const ss_t *src, const size_t off, const size_t n);

/* #API: |Overwrite string with input string copy applying a erase operation (character mode)|output string; input string; input string erase start character offset; number of characters to erase|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_erase_u(ss_t **s, const ss_t *src, const size_t char_off, const size_t n);

/* #API: |Overwrite string with input string plus replace operation|output string; input string; offset for starting the replace operation (0 for the whole input string); pattern to be replaced; patter replacement|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_replace(ss_t **s, const ss_t *src, const size_t off, const ss_t *s1, const ss_t *s2);

/* #API: |Overwrite string with input string copy plus resize operation (byte/UTF-8 mode)|output string; input string; number of bytes of input string; byte for refill|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_resize(ss_t **s, const ss_t *src, const size_t n, char fill_byte);

/* #API: |Overwrite string with input string copy plus resize operation (byte/UTF-8 mode)|output string; input string; number of bytes of input string; byte for refill|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_resize_u(ss_t **s, ss_t *src, const size_t n, int fill_char);

/* #API: |Overwrite string with input string plus trim (left and right) space removal operation|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_trim(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string plus left-trim space removal operation|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_ltrim(ss_t **s, const ss_t *src);

/* #API: |Overwrite string with input string plus right-trim space removal operation|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_rtrim(ss_t **s, const ss_t *src);

/*
#API: |Overwrite string with multiple C "wide char" Unicode string copy (strict aliasing is assumed) (UTF-16 for 16-bit wchar_t, and UTF-32 for 32-bit wchar_t)|output string; input strings (one or more C "wide char" strings)|output string reference (optional usage)|O(n)|1;2|
ss_t *ss_cpy_w(ss_t **s, ...)
*/
ss_t *ss_cpy_w_aux(ss_t **s, const wchar_t *s1, ...);

/* #API: |Overwrite string with printf operation|output string; printf output size (bytes); printf format; printf parameters|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_printf(ss_t **s, const size_t size, const char *fmt, ...);

/* #API: |Overwrite string with printf_va operation|output string; printf output size (bytes); printf format; printf_va parameters|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_printf_va(ss_t **s, const size_t size, const char *fmt, va_list ap);

/* #API: |Overwrite string with a string with just one character|output string; input character|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cpy_char(ss_t **s, const int c);

/* #API: |Read from file handle|output string; file handle; read max size (in bytes)|output result|O(n): WARNING: involves external file I/O|1;2| */
ss_t *ss_cpy_read(ss_t **s, FILE *handle, const size_t max_bytes);

/*
 * Append
 */

/*
#API: |Concatenate to string one or more strings|output string; input string; optional input strings|output string reference (optional usage)|O(n)|1;2|
ss_t *ss_cat(ss_t **s, const ss_t *s1, ...)
*/
ss_t *ss_cat_aux(ss_t **s, const ss_t *s1, ...);

/* #API: |Concatenate substring (byte/UTF-8 mode)|output string; input string; input string substring byte offset; input string substring size (bytes)|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_substr(ss_t **s, const ss_t *src, const size_t off, const size_t n);

/* #API: |Concatenate substring (Unicode character mode)|output string; input string; input substring character offset; input string substring size (characters)|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_substr_u(ss_t **s, const ss_t *src, const size_t char_off, const size_t n);

/* #API: |Concatenate C substring (byte/UTF-8 mode)|output string; input C string; input string size (bytes)|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_cn(ss_t **s, const char *src, const size_t src_size);

/*
#API: |Concatenate multiple C strings (byte/UTF-8 mode)|output string; input string; optional input strings|output string reference (optional usage)|O(n)|1;2|
ss_t *ss_cat_c(ss_t **s, const char *s1, ...)
*/
ss_t *ss_cat_c_aux(ss_t **s, const char *s1, ...);

/*
#API: |Concatenate multiple "wide char" C strings (Unicode character mode)|output string; input "wide char" C string (UTF-16 for 16-bit wchar_t, and UTF-32 for 32-bit wchar_t); optional input "wide char" C strings|output string reference (optional usage)|O(n)|1;2|
ss_t *ss_cat_w(ss_t **s, const char *s1, ...)
*/
ss_t *ss_cat_w_aux(ss_t **s, const wchar_t *s1, ...);

/* #API: |Concatenate "wide char" C substring (Unicode character mode)|output string; input "wide char" C string; input string size (characters) (UTF-16 for 16-bit wchar_t, and UTF-32 for 32-bit wchar_t)|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_wn(ss_t **s, const wchar_t *src, const size_t src_size);

/* #API: |Concatenate integer|output string; integer (any signed integer size)|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_int(ss_t **s, const int64_t num);

/* #API: |Concatenate "lowercased" string|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_tolower(ss_t **s, const ss_t *src);

/* #API: |Concatenate "uppercased" string|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_toupper(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string base64 encoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_enc_b64(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string hexadecimal (lowercase) encoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_enc_hex(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string hexadecimal (uppercase) encoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_enc_HEX(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string LZW encoded copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_enc_lzw(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string RLE encoded copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_enc_rle(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string JSON escape encoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_enc_esc_json(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string XML escape encoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_enc_esc_xml(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string URL escape encoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_enc_esc_url(ss_t **s, const ss_t *src);

/* #API: |Concatenate string escaping " as ""|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_enc_esc_dquote(ss_t **s, const ss_t *src);

/* #API: |Concatenate string escaping ' as ''|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_enc_esc_squote(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string base64 decoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_dec_b64(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string hexadecimal (lowercase) decoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_dec_hex(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string LZW decoded copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_dec_lzw(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string RLE decoded copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_dec_rle(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string JSON escape decoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_dec_esc_json(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string XML escape decoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_dec_esc_xml(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string URL escape decoding copy|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_dec_esc_url(ss_t **s, const ss_t *src);

/* #API: |Concatenate string unescaping "" as "|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_dec_esc_dquote(ss_t **s, const ss_t *src);

/* #API: |Concatenate string unescaping '' as '|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_dec_esc_squote(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with erase operation (byte/UTF-8 mode)|output string; input string; input string byte offset for erase start; erase count (bytes)|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_erase(ss_t **s, const ss_t *src, const size_t off, const size_t n);

/* #API: |Concatenate string with erase operation (Unicode character mode)|output string; input string; input character string offset for erase start; erase count (characters)|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_erase_u(ss_t **s, const ss_t *src, const size_t char_off, const size_t n);

/* #API: |Concatenate string with replace operation|output string; input string; offset for starting the replace operation (0 for the whole input string); pattern to be replaced; patter replacement|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_replace(ss_t **s, const ss_t *src, const size_t off, const ss_t *s1, const ss_t *s2);

/* #API: |Concatenate string with input string copy plus resize operation (byte/UTF-8 mode)|output string; input string; number of bytes of input string; byte for refill|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_resize(ss_t **s, const ss_t *src, const size_t n, char fill_byte);

/* #API: |Concatenate string with input string copy plus resize operation (Unicode character)|output string; input string; number of characters of input string; character for refill|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_resize_u(ss_t **s, ss_t *src, const size_t n, int fill_char);

/* #API: |Concatenate string with input string plus trim (left and right) space removal operation|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_trim(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string plus left-trim space removal operation|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_ltrim(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with input string plus right-trim space removal operation|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_rtrim(ss_t **s, const ss_t *src);

/* #API: |Concatenate string with printf operation|output string; printf output size (bytes); printf format; printf parameters|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_printf(ss_t **s, const size_t size, const char *fmt, ...);

/* #API: |Concatenate string with printf_va operation|output string; printf output size (bytes); printf format; printf_va parameters|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_cat_printf_va(ss_t **s, const size_t size, const char *fmt, va_list ap);

/* #API: |Concatenate string with a string with just one character|output string; input character|output string reference (optional usage)|O(1)|1;2| */
ss_t *ss_cat_char(ss_t **s, const int c);

/* #API: |Cat data read from file handle|output string; file handle; read max size (in bytes)|output result|O(n): WARNING: involves external file I/O|1;2| */
ss_t *ss_cat_read(ss_t **s, FILE *handle, const size_t max_bytes);

/*
 * Transformation
 */

/* #API: |Convert string to lowercase|output string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_tolower(ss_t **s);

/* #API: |Convert string to uppercase|output string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_toupper(ss_t **s);

/* #API: |Convert to base64|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_enc_b64(ss_t **s, const ss_t *src);

/* #API: |Convert to hexadecimal (lowercase)|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_enc_hex(ss_t **s, const ss_t *src);

/* #API: |Convert to hexadecimal (uppercase)|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_enc_HEX(ss_t **s, const ss_t *src);

/* #API: |Convert to LZW|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_enc_lzw(ss_t **s, const ss_t *src);

/* #API: |Convert to RLE|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_enc_rle(ss_t **s, const ss_t *src);

/* #API: |Convert/escape for JSON encoding|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_enc_esc_json(ss_t **s, const ss_t *src);

/* #API: |Convert/escape for XML encoding|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_enc_esc_xml(ss_t **s, const ss_t *src);

/* #API: |Convert/escape for URL encoding|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_enc_esc_url(ss_t **s, const ss_t *src);

/* #API: |Convert/escape escaping " as ""|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_enc_esc_dquote(ss_t **s, const ss_t *src);

/* #API: |Convert/escape escaping ' as ''|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_enc_esc_squote(ss_t **s, const ss_t *src);

/* #API: |Decode from base64|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_dec_b64(ss_t **s, const ss_t *src);

/* #API: |Decode from hexadecimal (lowercase)|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_dec_hex(ss_t **s, const ss_t *src);

/* #API: |Decode from LZW|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_dec_lzw(ss_t **s, const ss_t *src);

/* #API: |Decode from RLE|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_dec_rle(ss_t **s, const ss_t *src);

/* #API: |Unescape from JSON encoding|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_dec_esc_json(ss_t **s, const ss_t *src);

/* #API: |Unescape from XML encoding|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_dec_esc_xml(ss_t **s, const ss_t *src);

/* #API: |Unescape from URL encoding|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_dec_esc_url(ss_t **s, const ss_t *src);

/* #API: |Unescape "" as "|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_dec_esc_dquote(ss_t **s, const ss_t *src);

/* #API: |Unescape '' as '|output string; input string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_dec_esc_squote(ss_t **s, const ss_t *src);

/* #API: |Set Turkish mode locale (related to case conversion)|S_TRUE: enable turkish mode, S_FALSE: disable|S_TRUE: conversion functions OK, S_FALSE: error (missing functions)|O(1)|1;2| */
sbool_t ss_set_turkish_mode(const sbool_t enable_turkish_mode);

/* #API: |Clear string|output string|output string reference (optional usage)|O(n)|1;2| */
void ss_clear(ss_t *s);

/* #API: |Check and fix string (if input string is NULL, replaces it with a empty valid string)|output string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_check(ss_t **s);

/* #API: |Erase portion of a string (byte/UTF-8 mode)|input/output string; byte offset where to start the cut; number of bytes to be cut|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_erase(ss_t **s, const size_t off, const size_t n);

/* #API: |Erase portion of a string (Unicode character mode)|input/output string; character offset where to start the cut; number of characters to be cut|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_erase_u(ss_t **s, const size_t char_off, const size_t n);

/* #API: |Replace into string|input/output string; byte offset where to start applying the replace operation; target pattern; replacement pattern|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_replace(ss_t **s, const size_t off, const ss_t *s1, const ss_t *s2);

/* #API: |Resize string (byte/UTF-8 mode)|input/output string; new size in bytes; fill byte|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_resize(ss_t **s, const size_t n, char fill_byte);

/* #API: |Resize string (Unicode character mode)|input/output string; new size in characters; fill character|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_resize_u(ss_t **s, const size_t n, int fill_char);

/* #API: |Remove spaces from left and right side|input/output string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_trim(ss_t **s);

/* #API: |Remove spaces from left side|input/output string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_ltrim(ss_t **s);

/* #API: |Remove spaces from right side|input/output string|output string reference (optional usage)|O(n)|1;2| */
ss_t *ss_rtrim(ss_t **s);

/*
 * Export
 */

/* #API: |Give a C-compatible zero-ended string reference (byte/UTF-8 mode)|input string|Zero-ended C compatible string reference (UTF-8)|O(1)|1;2| */
const char *ss_to_c(const ss_t *s);

/* #API: |Give a C-compatible zero-ended string reference ("wide char" Unicode mode) (UTF-16 for 16-bit wchar_t, and UTF-32 for 32-bit wchar_t)|input string; output string buffer; output string max characters; output string size|Zero'ended C compatible string reference ("wide char" Unicode mode)|O(n)|1;2| */
const wchar_t *ss_to_w(const ss_t *s, wchar_t *o, const size_t nmax, size_t *n);

/*
 * Search
 */

/* #API: |Find substring into string|input string; search offset start; target string|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_find(const ss_t *s, const size_t off, const ss_t *tgt);

/* #API: |Find blank (9, 10, 13, 32) character into string|input string; search offset start|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_findb(const ss_t *s, const size_t off);

/* #API: |Find first byte between a min and a max value|input string; search offset start; target byte mininum value; target byte maximum value|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_findcx(const ss_t *s, const size_t off, const unsigned char c_min, const unsigned char c_max);

/* #API: |Find byte into string|input string; search offset start; target character|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_findc(const ss_t *s, const size_t off, const char c);

/* #API: |Find Unicode character into string|input string; search offset start; target character|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_findu(const ss_t *s, const size_t off, const int c);

/* #API: |Find non-blank (9, 10, 13, 32) character into string|input string; search offset start|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_findnb(const ss_t *s, const size_t off);

/* #API: |Find n bytes|input string; search offset start; target buffer; target buffer size (bytes)|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_find_cn(const ss_t *s, const size_t off, const char *t, const size_t ts);

/* #API: |Find substring into string (in range)|input string; search offset start; max offset (S_NPOS for end of string); target string|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_findr(const ss_t *s, const size_t off, const size_t max_off, const ss_t *tgt);

/* #API: |Find blank (9, 10, 13, 32) character into string (in range)|input string; search offset start; max offset (S_NPOS for end of string)|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_findrb(const ss_t *s, const size_t off, const size_t max_off);

/* #API: |Find first byte between a min and a max value|input string; search offset start; max offset (S_NPOS for end of string); target byte mininum value; target byte maximum value|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_findrcx(const ss_t *s, const size_t off, const size_t max_off, const unsigned char c_min, const unsigned char c_max);

/* #API: |Find byte into string (in range)|input string; search offset start; max offset (S_NPOS for end of string); target character|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_findrc(const ss_t *s, const size_t off, const size_t max_off, const char c);

/* #API: |Find Unicode character into string (in range)|input string; search offset start; max offset (S_NPOS for end of string); target character|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_findru(const ss_t *s, const size_t off, const size_t max_off, const int c);

/* #API: |Find non-blank (9, 10, 13, 32) character into string (in range)|input string; search offset start; max offset (S_NPOS for end of string)|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_findrnb(const ss_t *s, const size_t off, const size_t max_off);

/* #API: |Find n bytes|input string; search offset start; max offset (S_NPOS for end of string); target buffer; target buffer size (bytes)|Offset location if found, S_NPOS if not found|O(n)|1;2| */
size_t ss_findr_cn(const ss_t *s, const size_t off, const size_t max_off, const char *t, const size_t ts);

/* #API: |Split/tokenize: break string by separators|input string; separator; output substring references; number of output substrings|Number of elements|O(n)|1;2| */
size_t ss_split(const ss_t *src, const ss_t *separator, ss_ref_t out_substrings[], const size_t max_refs);

/*
 * Compare
 */

/* #API: |String compare|string 1; string 2|0: s1 = s2; < 0: s1 lower than s2; > 0: s1 greater than s2|O(n)|1;2| */
int ss_cmp(const ss_t *s1, const ss_t *s2);

/* #API: |Case-insensitive string compare|string 1; string 2|0: s1 = s2; < 0: s1 lower than s2; > 0: s1 greater than s2|O(n)|1;2| */
int ss_cmpi(const ss_t *s1, const ss_t *s2);

/* #API: |Partial string compare|string 1; string 1 offset (bytes; string 2; comparison size (bytes)|0: s1 = s2; < 0: s1 lower than s2; > 0: s1 greater than s2|O(n)|1;2| */
int ss_ncmp(const ss_t *s1, const size_t s1off, const ss_t *s2, const size_t n);

/* #API: |Partial case insensitive string compare|string 1; string 1 offset (bytes; string 2; comparison size (bytes)|0: s1 = s2; < 0: s1 lower than s2; > 0: s1 greater than s2|O(n)|1;2| */
int ss_ncmpi(const ss_t *s1, const size_t s1off, const ss_t *s2, const size_t n);

/*
 * snprintf equivalent
 */

/* #API: |printf operation on string|output string; printf max output size -including the 0 terminator- (bytes); printf "format";printf "format" parameters|output string reference (optional usage)|O(n)|1;2| */
int ss_printf(ss_t **s, size_t size, const char *fmt, ...);

/*
 * Unicode character I/O
 */

/* #API: |Get next Unicode character|input string; iterator|Output character, or EOF if no more characters left|O(1)|1;2| */
int ss_getchar(const ss_t *s, size_t *autoinc_off);

/* #API: |Append Unicode character to string|output string; Unicode character|Echo of the output character or EOF if overflow error|O(1)|1;2| */
int ss_putchar(ss_t **s, const int c);

/* #API: |Extract last character from string|input/output string|Extracted character if OK, EOF if empty|O(1)|1;2| */
int ss_popchar(ss_t **s);

/*
 * I/O
 */

/* #API: |Read from file handle|output string; file handle; read max size (in bytes)|output result|O(n): WARNING: involves external file I/O|1;2| */
ssize_t ss_read(ss_t **s, FILE *handle, const size_t max_bytes);

/* #API: |Write to file|output file; string; string offset; bytes to write|written bytes < 0: error|O(n): WARNING: involves external file I/O|1;2| */
ssize_t ss_write(FILE *handle, const ss_t *s, const size_t offset, const size_t bytes);

/*
 * Hashing
 */

/* #API: |String CRC-32 checksum|string|32-bit hash|O(n)|1;2| */
uint32_t ss_crc32(const ss_t *s);

/* #API: |CRC-32 checksum for substring|string; CRC resulting from previous chained CRC calls (use S_CRC32_INIT for the first call); start offset; end offset|32-bit hash|O(n)|1;2| */
uint32_t ss_crc32r(const ss_t *s, uint32_t crc, size_t off1, size_t off2);

/* #API: |String Adler32 checksum|string|32-bit hash|O(n)|1;2| */
uint32_t ss_adler32(const ss_t *s);

/* #API: |Adler32 checksum for substring|string; Adler32 resulting from previous chained Adler32 calls (use S_ADLER32_INIT for the first call); start offset; end offset|32-bit hash|O(n)|1;2| */
uint32_t ss_adler32r(const ss_t *s, uint32_t adler, size_t off1, size_t off2);


/*
 * Inlined functions
 */

S_INLINE sbool_t ss_is_ref(const ss_t *s)
{
	return s && s->d.f.flag3 != 0 ? S_TRUE : S_FALSE;
}

S_INLINE sbool_t ss_is_cref(const ss_t *s)
{
	return s && s->d.f.flag4 != 0 ? S_TRUE : S_FALSE;
}

S_INLINE char *ss_get_buffer(ss_t *s)
{
	/*
	 * Constness breaking will be addressed once the ss_to_c gets fixed.
	 */
	return ss_is_ref(s) ? (char *)((struct SStringRef *)s)->cstr :
		sdx_get_buffer((sd_t *)s);
}

S_INLINE const char *ss_get_buffer_r(const ss_t *s)
{
	return ss_is_ref(s) ? ((const struct SStringRef *)s)->cstr :
		sdx_get_buffer_r((const sd_t *)s);
}

/*
 * Aux
 */

#ifdef S_DEBUG
extern size_t dbg_cnt_alloc_calls;	/* alloc or realloc calls */
#endif

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif	/* SSTRING_H */


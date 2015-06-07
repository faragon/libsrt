/*
 * senc.c
 *
 * Buffer encoding/decoding
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "senc.h"
#include "sbitio.h"
#include <stdlib.h>

#define SLZW_ENABLE_RLE
#if defined(SLZW_ENABLE_RLE) /*&& defined(S_UNALIGNED_MEMORY_ACCESS)*/
#define SLZW_ENABLE_RLE_ENC
#endif

/*
 * Constants
 */

static const char b64e[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b',
	'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
	'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
	};
static const char b64d[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0,
	0, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 0, 0, 0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 32, 33,
	34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
	51, 0, 0, 0, 0, 0
	};
static const char n2h_l[16] = {
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 97, 98, 99, 100, 101, 102
	};
static const char n2h_u[16] = {
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 65, 66, 67, 68, 69, 70
	};
static const char h2n[64] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

#define SLZW_MAX_TREE_BITS	12
#define SLZW_CODE_LIMIT		(1 << SLZW_MAX_TREE_BITS)
#define SLZW_MAX_CODE		(SLZW_CODE_LIMIT - 1)
#define SLZW_ROOT_NODE_BITS	8
#define SLZW_OP_START		(1 << SLZW_ROOT_NODE_BITS)
#define SLZW_RESET		SLZW_OP_START
#define SLZW_STOP		(SLZW_RESET + 1)
#ifdef SLZW_ENABLE_RLE
	#define SLZW_RLE	(SLZW_STOP + 1)
	#define SLZW_RLE3       (SLZW_RLE + 1)
	#define SLZW_RLE4       (SLZW_RLE3 + 1)
	#define SLZW_RLE_CSIZE	16
	#define SRLE_RUN_BITS_D2 9
	#if S_BPWORD >= 8
		typedef suint_t srle_cmp_t;
	#else
		typedef suint32_t srle_cmp_t;
	#endif
	#define SLZW_OP_END	SLZW_RLE4
#else
	#define SLZW_OP_END	SLZW_STOP
#endif
#define SLZW_FIRST		(SLZW_OP_END + 1)
#define SLZW_LUT_CHILD_ELEMS	256
#define SLZW_MAX_LUTS		(SLZW_CODE_LIMIT / 8)
#define SLZW_ESC_RATIO		(1 << (17 - SLZW_MAX_TREE_BITS))

/*
 * Macros
 */

#define EB64C1(a)	(a >> 2)
#define EB64C2(a, b)	((a & 3) << 4 | b >> 4)
#define EB64C3(b, c)	((b & 0xf) << 2 | c >> 6)
#define EB64C4(c)	(c & 0x3f)
#define DB64C1(a, b)	(a << 2 | b >> 4)
#define DB64C2(b, c)	(b << 4 | c >> 2)
#define DB64C3(c, d)	(c << 6 | d)

#define SLZW_ENC_RESET(node_lutref, lut_stack_in_use,	\
		       node_stack_in_use, next_code, curr_code_len) {	\
		memset(node_lutref, 0, 256 * sizeof(node_lutref[0]));	\
		lut_stack_in_use = 1;					\
		node_stack_in_use = 256;				\
		curr_code_len = SLZW_ROOT_NODE_BITS + 1;		\
		next_code = SLZW_FIRST;					\
	}

#define SLZW_DEC_RESET(curr_code_len, last_code, next_inc_code,	\
		       parents) {					\
		int j;							\
		curr_code_len = SLZW_ROOT_NODE_BITS + 1;		\
		last_code = SLZW_CODE_LIMIT;				\
		next_inc_code = SLZW_FIRST;				\
		for (j = 0; j < 256; j += 4) 				\
			parents[j] = parents[j + 1] = parents[j + 2] =	\
				     parents[j + 3] = SLZW_CODE_LIMIT;	\
	}

/*
 * Internal functions
 */

static int hex2nibble(const int h)
{
	return h2n[(h - 48) & 0x3f];
}

static size_t senc_hex_aux(const unsigned char *s, const size_t ss, unsigned char *o, const char *t)
{
	RETURN_IF(!s, ss * 2);
	ASSERT_RETURN_IF(!o, 0);
	const size_t out_size = ss * 2;
	size_t i = ss, j = out_size;
	size_t tail = ss - (ss % 2);
	#define ENCHEX_LOOP(ox, ix) {		\
		const int next = s[ix - 1];	\
		o[ox - 2] = t[next >> 4];	\
		o[ox - 1] = t[next & 0x0f];	\
		}
	if (tail)
		for (; i > 0; i--, j -= 2)
			ENCHEX_LOOP(j, i);
	for (; i > 0; i -= 2, j -= 4) {
		ENCHEX_LOOP(j - 2, i - 1);
		ENCHEX_LOOP(j, i);
	}
	#undef ENCHEX_LOOP
	return out_size;
}

static void slzw_setseq256s8(unsigned *p)
{
	unsigned j, acc = S_HTON_U32(0x00010203);
	for (j = 0; j < 256 / 4; j++) {
		p[j] = acc;
		acc += 0x04040404;
	}
}

static size_t slzw_bitio_read(const unsigned char *b, size_t *i, size_t *acc, size_t *accbuf, size_t cbits, size_t *normal_count, size_t *esc_count)
{
	size_t c;
#if SLZW_ESC_RATIO > 0
	if (*esc_count <= *normal_count / SLZW_ESC_RATIO) {
		c = sbitio_read(b, i, acc, accbuf, 8);
		if (c == 255) {
			c = sbitio_read(b, i, acc, accbuf, cbits);
			(*esc_count)++;
		} else {
			(*normal_count)++;
		}
	} else
#endif
	{
		c = sbitio_read(b, i, acc, accbuf, cbits);
		(*normal_count)++;
	}
	return c;
}



static void slzw_bitio_write(unsigned char *b, size_t *i, size_t *acc, size_t c, size_t cbits, size_t *normal_count, size_t *esc_count)
{
#if SLZW_ESC_RATIO > 0
	if (*esc_count <= *normal_count / SLZW_ESC_RATIO) {
		if (c < 255) {
	                sbitio_write(b, i, acc, c, 8);
			(*normal_count)++;
		} else {
	                sbitio_write(b, i, acc, 255, 8);
			sbitio_write(b, i, acc, c, cbits);
			(*esc_count)++;
		}
	} else
#endif
	{
		sbitio_write(b, i, acc, c, cbits);
		(*normal_count)++;
	}
#if 0
	char *label = "";
	switch (c) {
	#define zCASE(a) case a: label = #a; break
	zCASE(SLZW_RESET); zCASE(SLZW_STOP); zCASE(SLZW_RLE); zCASE(SLZW_RLE3); zCASE(SLZW_RLE4);
	}
	fprintf(stderr, "%04u (%02u) %s\n", (unsigned)c, (unsigned)cbits, label);
#endif
}

/*
 * Base64 encoding/decoding
 */

size_t senc_b64(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s, (ss / 3 + (ss % 3 ? 1 : 0)) * 4);
	ASSERT_RETURN_IF(!o, 0);
	const size_t ssod4 = (ss / 3) * 4;
	const size_t ssd3 = ss - (ss % 3);
	const size_t tail = ss - ssd3;
	size_t i = ssd3, j = ssod4 + (tail ? 4 : 0);
	const size_t out_size = j;
	unsigned si0, si1, si2;
	switch (tail) {
	case 2: si0 = s[ssd3], si1 = s[ssd3 + 1];
		o[j - 4] = b64e[EB64C1(si0)];
		o[j - 3] = b64e[EB64C2(si0, si1)];
		o[j - 2] = b64e[EB64C3(si1, 0)];
		o[j - 1] = '=';
		j -= 4;
		break;
	case 1: si0 = s[ssd3];
		o[j - 4] = b64e[EB64C1(si0)];
		o[j - 3] = b64e[EB64C2(si0, 0)];
		o[j - 2] = '=';
		o[j - 1] = '=';
		j -= 4;
	}
	for (; i > 0; i -= 3, j -= 4) {
		si0 = s[i - 3], si1 = s[i - 2], si2 = s[i - 1];
		o[j - 4] = b64e[EB64C1(si0)];
		o[j - 3] = b64e[EB64C2(si0, si1)];
		o[j - 2] = b64e[EB64C3(si1, si2)];
		o[j - 1] = b64e[EB64C4(si2)];
	}
	return out_size;
}

size_t sdec_b64(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s, (ss / 4) * 3);
	ASSERT_RETURN_IF(!o, 0);
	size_t i = 0, j = 0;
	const size_t ssd4 = ss - (ss % 4);
	const size_t tail = s[ss - 2] == '=' || s[ss - 1] == '=' ? 4 : 0;
	for (; i  < ssd4 - tail; i += 4, j += 3) {
		const int a = b64d[s[i]], b = b64d[s[i + 1]],
			  c = b64d[s[i + 2]], d = b64d[s[i + 3]];
		o[j] = DB64C1(a, b);
		o[j + 1] = DB64C2(b, c);
		o[j + 2] = DB64C3(c, d);
	}
	if (tail) {
		const int a = b64d[s[i] & 0x7f], b = b64d[s[i + 1] & 0x7f];
		o[j++] = DB64C1(a, b);
		if (s[i + 2] != '=') {
			const int c = b64d[s[i + 2] & 0x7f];
			o[j++] = DB64C2(b, c);
		}
	}
	return j;
}

/*
 * Hexadecimal encoding/decoding
 */

size_t senc_hex(const unsigned char *s, const size_t ss, unsigned char *o)
{
	return senc_hex_aux(s, ss, o, n2h_l);
}

size_t senc_HEX(const unsigned char *s, const size_t ss, unsigned char *o)
{
	return senc_hex_aux(s, ss, o, n2h_u);
}

size_t sdec_hex(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s, ss / 2);
	ASSERT_RETURN_IF(!o, 0);
	const size_t ssd2 = ss - (ss % 2),
		     ssd4 = ss - (ss % 4);
	ASSERT_RETURN_IF(!ssd2, 0);
	size_t i = 0, j = 0;
	#define SDEC_HEX_L(n, m)	\
		o[j + n] = (hex2nibble(s[i + m]) << 4) | \
			    hex2nibble(s[i + m + 1]);
	for (; i < ssd4; i += 4, j += 2) {
		SDEC_HEX_L(0, 0);
		SDEC_HEX_L(1, 2);
	}
	for (; i < ssd2; i += 2, j += 1)
		SDEC_HEX_L(0, 0);
	#undef SDEC_HEX_L
	return j;
}

/*
 * LZW encoding/decoding
 */

typedef short slzw_ndx_t;

size_t senc_lzw(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s || !o || !ss, 0);
	/*
	 * Node structure (separated elements and not a "struct", in order to
	 * save space because of data alignment)
	 *
	 * node_codes[i]: LZW output code for the node
	 * node_lutref[i]: 0: empty, < 0: -next_node, > 0: 256-child LUT ref.
	 * node_child[i]: if node_lutref[0] < 0: next node byte (one-child node)
	 */
        slzw_ndx_t node_codes[SLZW_CODE_LIMIT], node_lutref[SLZW_CODE_LIMIT],
		   lut_stack[SLZW_MAX_LUTS][SLZW_LUT_CHILD_ELEMS];
        unsigned char node_child[SLZW_CODE_LIMIT];
	/*
	 * Stack allocation control
	 */
	slzw_ndx_t node_stack_in_use, lut_stack_in_use;
	/*
	 * Output encoding control
	 */
	size_t normal_count = 0, esc_count = 0;
	size_t oi = 0, next_code, curr_code_len = SLZW_ROOT_NODE_BITS + 1, acc;
	sbitio_write_init(&acc);
	/*
	 * Initialize data structures
	 */
	size_t j;
	for (j = 0; j < 256; j += 4) {
		node_codes[j] = j;
		node_codes[j + 1] = j + 1;
		node_codes[j + 2] = j + 2;
		node_codes[j + 3] = j + 3;
	}
	SLZW_ENC_RESET(node_lutref, lut_stack_in_use,
		       node_stack_in_use, next_code, curr_code_len);
	/*
	 * Encoding loop
	 */
	size_t i = 0;
	slzw_ndx_t curr_node;
	for (; i < ss;) {
#ifdef SLZW_ENABLE_RLE_ENC
		if ((i + SLZW_RLE_CSIZE) < ss) {
			suint32_t *p0 = (suint32_t *)(s + i);
			suint32_t *p3 = (suint32_t *)(s + i + 3);
			int eq4 = p0[0] == p0[1];
			int eq3 = !eq4 && p0[0] == p3[0];
			for (; eq4 || eq3;) {
				srle_cmp_t *u = (srle_cmp_t *)(s + i),
				*v = (srle_cmp_t *)(s + i + 1),
				u0 = u[0];
				size_t cx, rle_mode, j;
				size_t max_cs = i + S_NBIT(SRLE_RUN_BITS_D2 * 2),
				       ss2 = S_MIN(ss, max_cs) - sizeof(srle_cmp_t);
				if (eq4) {
					j = i + sizeof(suint32_t) * 2;
					if (u0 == v[0]) {
						rle_mode = SLZW_RLE;
						j &= S_ALIGNMASK;
						cx = 1;
					} else {
						rle_mode = SLZW_RLE4;
						cx = 4;
					}
					for (; j < ss2 ; j += sizeof(srle_cmp_t))
						if (u0 != *(srle_cmp_t *)(s + j))
							break;
				} else {
					if (p0[1] != p3[1] || p0[2] != p3[2])
						break;
					j = i + 3 * 4;
					rle_mode = SLZW_RLE3;
					cx = 3;
					for (; j < ss2 ; j += 3)
						if (p0[0] != *(suint32_t *)(s + j))
							break;
				}
				if (j - i >= SLZW_RLE_CSIZE) {
					size_t count_cs = (j - i - cx) / cx,
					       count_csx = count_cs * cx,
					       ch = count_cs >> SRLE_RUN_BITS_D2,
					       cl = count_cs & S_NBITMASK(SRLE_RUN_BITS_D2);
					slzw_bitio_write(o, &oi, &acc, rle_mode, curr_code_len, &normal_count, &esc_count);
					sbitio_write(o, &oi, &acc, cl, SRLE_RUN_BITS_D2);
					sbitio_write(o, &oi, &acc, ch, SRLE_RUN_BITS_D2);
					sbitio_write(o, &oi, &acc, s[i], 8);
					if (cx >= 3) {
						sbitio_write(o, &oi, &acc, s[i + 1], 8);
						sbitio_write(o, &oi, &acc, s[i + 2], 8);
						if (cx >= 4)
							sbitio_write(o, &oi, &acc, s[i + 3], 8);
					}
					i += count_csx;
				}
				break;
			}
			if (i == ss)
				break;
		}
#endif
		/*
		 * Locate pattern
		 */
		unsigned in_byte = s[i++];
		curr_node = in_byte;
		slzw_ndx_t r;
		for (; i < ss; i++) {
			in_byte = s[i];
			const slzw_ndx_t nlut = node_lutref[curr_node];
			if (nlut < 0 && in_byte == node_child[curr_node]) {
				curr_node = -nlut;
				continue;
			}
			if (nlut <= 0 || !(r = lut_stack[nlut][in_byte]))
				break;
			curr_node = r;
		}
		if (i == ss)
			break;
		/*
		 * Add new code to the tree
		 */
		const slzw_ndx_t new_node = node_stack_in_use++;
		if (node_lutref[curr_node] <= 0) {
			if (node_lutref[curr_node] == 0) { /* empty */
				node_lutref[curr_node] = -new_node;
				node_child[curr_node] = in_byte;
			} else { /* single node: convert it to LUT */
				slzw_ndx_t alt_n = -node_lutref[curr_node];
				slzw_ndx_t new_lut = lut_stack_in_use++;
				node_lutref[curr_node] = new_lut;
				memset(&lut_stack[new_lut], 0, sizeof(lut_stack[0]));
#ifdef _MSC_VER
#pragma warning(suppress: 6001)
#endif
				lut_stack[new_lut][node_child[curr_node]] = alt_n;
			}
		}
		if (node_lutref[curr_node] > 0)
			lut_stack[node_lutref[curr_node]][in_byte] = new_node;
		/*
		 * Write pattern code to the output stream
		 */
		node_codes[new_node] = next_code;
		node_lutref[new_node] = 0;
		slzw_bitio_write(o, &oi, &acc, node_codes[curr_node], curr_code_len, &normal_count, &esc_count);
		if (next_code == (1 << curr_code_len))
			curr_code_len++;
		/*
		 * Reset tree if tree code limit is reached or if running
		 * out of LUTs
		 */
		if (++next_code == SLZW_MAX_CODE ||
		    lut_stack_in_use == SLZW_MAX_LUTS) {
			slzw_bitio_write(o, &oi, &acc, SLZW_RESET, curr_code_len, &normal_count, &esc_count);
		        SLZW_ENC_RESET(node_lutref,
				       lut_stack_in_use, node_stack_in_use,
				       next_code, curr_code_len);
		}
	}
	/*
	 * Write last code, the "end of information" mark, and fill bits with 0
	 */
	slzw_bitio_write(o, &oi, &acc, node_codes[curr_node], curr_code_len, &normal_count, &esc_count);
	slzw_bitio_write(o, &oi, &acc, SLZW_STOP, curr_code_len, &normal_count, &esc_count);
	sbitio_write_close(o, &oi, &acc);
	return oi;
}

size_t sdec_lzw(const unsigned char *s, const size_t ss, unsigned char *o)
{
	RETURN_IF(!s || !o || !ss, 0);
	size_t i, acc, accbuf, oi;
	size_t normal_count = 0, esc_count = 0;
	size_t last_code, curr_code_len = SLZW_ROOT_NODE_BITS + 1, next_inc_code;
	slzw_ndx_t parents[SLZW_CODE_LIMIT];
	unsigned char xbyte[SLZW_CODE_LIMIT], pattern[SLZW_CODE_LIMIT], lastwc = 0;
	/*
	 * Init read buffer
	 */
	oi = 0;
	sbitio_read_init(&acc, &accbuf);
	/*
	 * Initialize root node
	 */
	slzw_setseq256s8((unsigned *)xbyte);
	SLZW_DEC_RESET(curr_code_len, last_code, next_inc_code, parents);
	/*
	 * Code expand loop
	 */
	for (i = 0; i < ss;) {
		size_t new_code;
		new_code = slzw_bitio_read(s, &i, &acc, &accbuf, curr_code_len, &normal_count, &esc_count);
		if (new_code < SLZW_OP_START || new_code > SLZW_OP_END) {
			if (last_code == SLZW_CODE_LIMIT) {
				o[oi++] = lastwc = xbyte[new_code];
				last_code = new_code;
				continue;
			}
			size_t code, pattern_off = SLZW_CODE_LIMIT - 1;
			if (new_code == next_inc_code) {
				pattern[pattern_off--] = lastwc;
				code = last_code;
			} else {
				code = new_code;
			}
			for (; code >= SLZW_FIRST;) {
				pattern[pattern_off--] = xbyte[code];
				code = parents[code];
			}
			pattern[pattern_off--] = lastwc = xbyte[next_inc_code] = xbyte[code];
			parents[next_inc_code] = last_code;
			if (next_inc_code < SLZW_CODE_LIMIT)
				next_inc_code++;
			if (next_inc_code == 1 << curr_code_len &&
			    next_inc_code < SLZW_CODE_LIMIT) {
				curr_code_len++;
			}
			last_code = new_code;
			/*
			 * Write LZW pattern
			 */
			size_t write_size = SLZW_CODE_LIMIT - 1 - pattern_off;
			memcpy(o + oi, pattern + pattern_off + 1, write_size);
			oi += write_size;
			continue;
		}
#ifdef SLZW_ENABLE_RLE
		/*
		 * Write RLE pattern
		 */
		if (new_code >= SLZW_RLE && new_code <= SLZW_RLE4) {
			union { unsigned a32; unsigned char b[4]; } rle;
			size_t count, cl, ch;
			cl = sbitio_read(s, &i, &acc, &accbuf, SRLE_RUN_BITS_D2);
			ch = sbitio_read(s, &i, &acc, &accbuf, SRLE_RUN_BITS_D2);
			count = ch << SRLE_RUN_BITS_D2 | cl;
			rle.b[0] = sbitio_read(s, &i, &acc, &accbuf, 8);
			if (new_code == SLZW_RLE) {
				memset(o + oi, rle.b[0], count);
			} else {
				rle.b[1] = sbitio_read(s, &i, &acc, &accbuf, 8);
				rle.b[2] = sbitio_read(s, &i, &acc, &accbuf, 8);
				if (new_code == SLZW_RLE4) {
					count *= 4;
					rle.b[3] = sbitio_read(s, &i, &acc, &accbuf, 8);
					s_memset32(o + oi, rle.a32, count);
				} else {
					count *= 3;
					s_memset24(o + oi, rle.b, count);
				}
			}
			oi += count;
			continue;
		}
#endif
		if (new_code == SLZW_RESET) {
			SLZW_DEC_RESET(curr_code_len, last_code, next_inc_code, parents);
			continue;
		}
		if (new_code == SLZW_STOP)
			break;
	}
	return oi;
}

#ifdef STANDALONE_TEST

#define BUF_IN_SIZE (120LL * 1024 * 1024)
#define BUF_OUT_SIZE (BUF_IN_SIZE * 2)

static int syntax_error(const char **argv, const int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Error [%i] Syntax: %s [-eb|-db|-eh|-eH|-dh|-ez|-dz]\nExamples:\n"
		"%s -eb <in >out.b64\n%s -db <in.b64 >out\n"
		"%s -eh <in >out.hex\n%s -eH <in >out.HEX\n"
		"%s -dh <in.hex >out\n%s -dh <in.HEX >out\n"
		"%s -ez <in >in.z\n%s -dz <in.z >out\n",
		exit_code, v0, v0, v0, v0, v0, v0, v0, v0, v0);
	return exit_code;
}

/* TODO: buffered I/O instead of unbuffered */
int main(int argc, const char **argv)
{
	size_t lo = 0;
	char *buf0 = (char *)malloc(BUF_IN_SIZE + BUF_OUT_SIZE);
	char *buf = buf0;
	char *bufo = buf0 + BUF_IN_SIZE;
	int f_in = 0, f_out = 1;
	ssize_t l;
	if (argc < 2 || (l = posix_read(f_in, buf, BUF_IN_SIZE)) < 0)
		return syntax_error(argv, 1);
	const int mode = !strncmp(argv[1], "-eb", 3) ? 1 :
			 !strncmp(argv[1], "-db", 3) ? 2 :
			 !strncmp(argv[1], "-eh", 3) ? 3 :
			 !strncmp(argv[1], "-eH", 3) ? 4 :
			 !strncmp(argv[1], "-dh", 3) ? 5 :
			 !strncmp(argv[1], "-ez", 3) ? 6 :
			 !strncmp(argv[1], "-dz", 3) ? 7 : 0;
	if (!mode)
		return syntax_error(argv, 2);
	size_t i = 0, imax;
	switch (argv[1][3] == 'x') {
	case 'x': imax = 100000; break;
	case 'X': imax = 1000000; break;
	default:  imax = 1;
	}
	for (; i < imax; i++) {
		switch (mode) {
		case 1: lo = senc_b64(buf, (size_t)l, bufo); break;
		case 2: lo = sdec_b64(buf, (size_t)l, bufo); break;
		case 3: lo = senc_hex(buf, (size_t)l, bufo); break;
		case 4: lo = senc_HEX(buf, (size_t)l, bufo); break;
		case 5: lo = sdec_hex(buf, (size_t)l, bufo); break;
		case 6: lo = senc_lzw(buf, (size_t)l, bufo); break;
		case 7: lo = sdec_lzw(buf, (size_t)l, bufo); break;
		}
	}
	int r = posix_write(f_out, bufo, lo);
	fprintf(stderr, "in: %u * %u bytes, out: %u * %u bytes [write result: %u]\n",
		(unsigned)l, (unsigned)i, (unsigned)lo, (unsigned)i, (unsigned)r);
	return 0;
}
#endif


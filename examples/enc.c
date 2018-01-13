/*
 * enc.c
 *
 * Buffer encoding/decoding example using libsrt
 *
 * Copyright (c) 2015-2018 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "../src/libsrt.h"
#include "../src/saux/senc.h"

/* IBUF_SIZE: 3 * 4 because of LCM for b64 */
#if S_BPWORD > 4
#define IBUF_SIZE	(3 * 4 * 100 * 1000 * 1000L)  /* 1.2 GB for 64 bit */
#else
#define IBUF_SIZE	(3 * 4 * 150 * 1000) /* 1.8 MB for 32 bit systems */
#endif
#define OBUF_SIZE	(IBUF_SIZE * 6) /* Max req: xml escape */
#define ESC_MAX_SIZE	16
#define XBUF_SIZE	(S_MAX(IBUF_SIZE, OBUF_SIZE) + ESC_MAX_SIZE)

#if SZ_DEBUG_STATS
	extern size_t lz_st_lit[8], lz_st_lit_bytes;
	extern size_t lz_st_ref[8], lz_st_ref_bytes;
#endif

static int syntax_error(const char **argv, const int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Error [%i] Syntax: %s [-eb|-db|-eh|-eH|-dh|-ex|-dx|-ej|-dj|"
		"-eu|-du|-ez|-dz|-crc32|-adler32]\nExamples:\n"
		"%s -eb <in >out.b64\n%s -db <in.b64 >out\n"
		"%s -eh <in >out.hex\n%s -eH <in >out.HEX\n"
		"%s -dh <in.hex >out\n%s -dh <in.HEX >out\n"
		"%s -ex <in >out.xml.esc\n%s -dx <in.xml.esc >out\n"
		"%s -ej <in >out.json.esc\n%s -dx <in.json.esc >out\n"
		"%s -eu <in >out.url.esc\n%s -du <in.url.esc >out\n"
		"%s -ez <in >in.lz\n%s -dz <in.lz >out\n"
		"%s -ezh <in >in.lz\n%s -dz <in.lz >out\n"
		"%s -crc32 <in\n%s -crc32 <in >out\n"
		"%s -adler32 <in\n%s -adler32 <in >out\n",
		exit_code, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0,
		v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0);
	return exit_code;
}

int main(int argc, const char **argv)
{
	int exit_code = 0;
	uint8_t esc, dle[S_PK_U64_MAX_BYTES], *dlep;
	const uint8_t *dlepc;
	ss_ref_t ref;
	size_t lmax = 0;
	size_t li = 0, lo = 0, j, l, l2, ss0, off;
	const char *b;
	int cf;
	sbool_t done;
	uint32_t acc;
	uint32_t (*f32)(const ss_t *, uint32_t, size_t, size_t) = NULL;
	ss_t *in = ss_alloc(IBUF_SIZE * 2), *out = ss_alloc(IBUF_SIZE * 2);
	ss_t *(*ss_codec1_f)(ss_t **s, const ss_t *src) = NULL;
	ss_t *(*ss_codec2_f)(ss_t **s, const ss_t *src) = NULL;
	ss_t *(*ss_codec3_f)(ss_t **s, const ss_t *src) = NULL;
	ss_t *(*ss_codec4_f)(ss_t **s, const ss_t *src) = NULL;
	if (argc < 2)
		return syntax_error(argv, 1);
	if (!strncmp(argv[1], "-crc32", 6)) {
		acc = S_CRC32_INIT;
		f32 = ss_crc32r;
	} else if (!strncmp(argv[1], "-adler32", 8)) {
		acc = S_ADLER32_INIT;
		f32 = ss_adler32r;
	}
	if (f32) {
		uint32_t buf_size = 8192;
		ss_t *buf = ss_alloca(buf_size);
		while (ss_read(&buf, stdin, buf_size))
			acc = f32(buf, acc, 0, S_NPOS);
		printf("%08x\n", acc);
		return 0;
	}
	if (!strncmp(argv[1], "-eb", 3))
		ss_codec1_f = ss_enc_b64;
	else if (!strncmp(argv[1], "-db", 4))
		ss_codec1_f = ss_dec_b64;
	else if (!strncmp(argv[1], "-eh", 4))
		ss_codec1_f = ss_enc_hex;
	else if (!strncmp(argv[1], "-eH", 4))
		ss_codec1_f = ss_enc_HEX;
	else if (!strncmp(argv[1], "-dh", 4))
		ss_codec1_f = ss_dec_hex;
	else if (!strncmp(argv[1], "-ex", 4))
		ss_codec1_f = ss_enc_esc_xml;
	else if (!strncmp(argv[1], "-dx", 4))
		ss_codec2_f = ss_dec_esc_xml;
	else if (!strncmp(argv[1], "-ej", 4))
		ss_codec1_f = ss_enc_esc_json;
	else if (!strncmp(argv[1], "-dj", 4))
		ss_codec2_f = ss_dec_esc_json;
	else if (!strncmp(argv[1], "-eu", 4))
		ss_codec1_f = ss_enc_esc_url;
	else if (!strncmp(argv[1], "-du", 4))
		ss_codec2_f = ss_dec_esc_url;
	else if (!strncmp(argv[1], "-ezh", 5))
		ss_codec3_f = ss_enc_lzh;
	else if (!strncmp(argv[1], "-ez", 4))
		ss_codec3_f = ss_enc_lz;
	else if (!strncmp(argv[1], "-dz", 4))
		ss_codec4_f = ss_dec_lz;
	else
		return syntax_error(argv, 2);
	esc = ss_codec2_f == ss_dec_esc_xml ? '&' :
			    ss_codec2_f == ss_dec_esc_json ? '\\' : '%';
	cf = ss_codec1_f ? 1 : ss_codec2_f ? 2 :
	     ss_codec3_f ? 3 : ss_codec4_f ? 4 : 0;
	done = S_FALSE;
	while (!done) {
		switch (cf) {
		case 1:
		case 2:
			ss0 = ss_size(in);
			ss_cat_read(&in, stdin, IBUF_SIZE);
			l = ss_size(in);
			li += (l - ss0);
			if (!l) {
				done = S_TRUE;
				continue;
			}
			if (ss_codec1_f) {
				ss_codec1_f(&out, in);
				ss_clear(in);
				break;
			}
			b = ss_get_buffer_r(in);
			off = 0;
			if (l > ESC_MAX_SIZE)
				for (j = (size_t)(l - ESC_MAX_SIZE + 1);
				     j < l; j++)
					if (b[j] == esc && b[j - 1] != esc) {
						off = (size_t)(l - j);
						break;
					}
			l -= off;
			ss_codec2_f(&out, ss_ref_buf(&ref, b, l));
			if (off > 0)
				ss_cpy_substr(&in, in, l, S_NPOS);
			else
				ss_clear(in);
			break;
		case 3: /* lz encoding */
			ss_cpy_read(&in, stdin, IBUF_SIZE);
			l = ss_size(in);
			if (!l) {
				done = S_TRUE;
				continue;
			}
			li += l;
			ss_codec3_f(&out, in);
			lmax = ss_size(out);
			dlep = dle;
			s_st_pk_u64(&dlep, lmax);
			l = (dlep - dle); /* dyn LE size */
			if (!fwrite(dle, 1, l, stdout) || ferror(stdout)) {
				fprintf(stderr, "Write error\n");
				exit_code = 6;
				done = S_TRUE;
				continue;
			}
			lo += l;
			ss_clear(in);
			break;
		case 4: /* lz decoding */
			l = fread(dle, 1, 1, stdin);
			if (!l || ferror(stdin)) {
				done = S_TRUE;
				continue;
			}
			li += l;
			l = s_pk_u64_size(dle);
			if (!l) {
				done = S_TRUE;
				continue;
			}
			if (l > 1) {
				l2 = fread(dle + 1, 1, l - 1, stdin);
				li += l2;
				if (l != (l2 + 1)) {
					exit_code = 2;
					fprintf(stderr, "Format error\n");
					done = S_TRUE;
					continue;
				}
				l = l2 + 1;
			}
			dlepc = dle;
			lmax = s_ld_pk_u64(&dlepc, l);
			if (lmax > XBUF_SIZE) {
				fprintf(stderr, "Format error\n");
				exit_code = 3;
				done = S_TRUE;
				continue;
			}
			ss_cpy_read(&in, stdin, lmax);
			l = ss_size(in);
			li += l;
			if (l != lmax || ferror(stdin)) {
				fprintf(stderr, "Read error\n");
				exit_code = 4;
				done = S_TRUE;
				continue;
			}
			ss_codec4_f(&out, in);
			break;
		default:
			fprintf(stderr, "Logic error\n");
			exit_code = 8;
			done = S_TRUE;
			continue;
		}
		if (ss_write(stdout, out, 0, S_NPOS) < 0) {
			fprintf(stderr, "Write error\n");
			exit_code = 6;
			break;
		}
		lo += ss_size(out);
		ss_clear(out);
	}
	fprintf(stderr, "in: %zu bytes, out: %zu bytes\n", li, lo);
#if SZ_DEBUG_STATS
	fprintf(stderr,
		"lzlit: [8: %zu] [16: %zu] [24: %zu] [32: %zu] [bytes: %zu]\n",
		lz_st_lit[0], lz_st_lit[1], lz_st_lit[2], lz_st_lit[3],
		lz_st_lit_bytes);
	fprintf(stderr,
		"lzref: [8: %zu] [16: %zu] [24: %zu] [32: %zu] [40: %zu] "
		"[48: %zu] [56: %zu] [64: %zu] [72: %zu] [bytes: %zu]\n",
		lz_st_ref[0], lz_st_ref[1], lz_st_ref[2], lz_st_ref[3],
		lz_st_ref[4], lz_st_ref[5], lz_st_ref[6], lz_st_ref[7],
		lz_st_ref[8], lz_st_ref_bytes);
#endif
	ss_free(&in, &out);
	return exit_code;
}


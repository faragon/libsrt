/*
 * enc.c
 *
 * Buffer encoding/decoding example using libsrt
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "../src/libsrt.h"

#define IBUF_SIZE	(3 * 4 * 1024) /* 3 * 4: LCM for b64 */
#define OBUF_SIZE	(IBUF_SIZE * 6) /* Max req: xml escape */
#define ESC_MAX_SIZE	16
#define XBUF_SIZE	(S_MAX(IBUF_SIZE, OBUF_SIZE) + ESC_MAX_SIZE)

static int syntax_error(const char **argv, const int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Error [%i] Syntax: %s [-eb|-db|-eh|-eH|-dh|-ex|-dx|-ej|-dj|"
		"-eu|-du|-er|-dr|-ez|-dz|-crc32|-adler32]\nExamples:\n"
		"%s -eb <in >out.b64\n%s -db <in.b64 >out\n"
		"%s -eh <in >out.hex\n%s -eH <in >out.HEX\n"
		"%s -dh <in.hex >out\n%s -dh <in.HEX >out\n"
		"%s -ex <in >out.xml.esc\n%s -dx <in.xml.esc >out\n"
		"%s -ej <in >out.json.esc\n%s -dx <in.json.esc >out\n"
		"%s -eu <in >out.url.esc\n%s -du <in.url.esc >out\n"
		"%s -er <in >in.r\n%s -dr <in.r >out\n"
		"%s -ez <in >in.z\n%s -dz <in.z >out\n"
		"%s -crc32 <in\n%s -crc32 <in >out\n"
		"%s -adler32 <in\n%s -adler32 <in >out\n",
		exit_code, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0,
		v0, v0, v0, v0, v0, v0, v0, v0, v0);
	return exit_code;
}

int main(int argc, const char **argv)
{
	if (argc < 2)
		return syntax_error(argv, 1);
	uint32_t acc;
	uint32_t (*f32)(const ss_t *, uint32_t, size_t, size_t) = NULL;
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
	ss_t *in = ss_alloc(IBUF_SIZE * 2), *out = ss_alloc(IBUF_SIZE * 2);
	ss_t *(*ss_codec1_f)(ss_t **s, const ss_t *src) = NULL;
	ss_t *(*ss_codec2_f)(ss_t **s, const ss_t *src) = NULL;
	ss_t *(*ss_codec3_f)(ss_t **s, const ss_t *src) = NULL;
	ss_t *(*ss_codec4_f)(ss_t **s, const ss_t *src) = NULL;
	if (!strncmp(argv[1], "-eb", 3))
		ss_codec1_f = ss_enc_b64;
	else if (!strncmp(argv[1], "-db", 3))
		ss_codec1_f = ss_dec_b64;
	else if (!strncmp(argv[1], "-eh", 3))
		ss_codec1_f = ss_enc_hex;
	else if (!strncmp(argv[1], "-eH", 3))
		ss_codec1_f = ss_enc_HEX;
	else if (!strncmp(argv[1], "-dh", 3))
		ss_codec1_f = ss_dec_hex;
	else if (!strncmp(argv[1], "-ex", 3))
		ss_codec1_f = ss_enc_esc_xml;
	else if (!strncmp(argv[1], "-dx", 3))
		ss_codec2_f = ss_dec_esc_xml;
	else if (!strncmp(argv[1], "-ej", 3))
		ss_codec1_f = ss_enc_esc_json;
	else if (!strncmp(argv[1], "-dj", 3))
		ss_codec2_f = ss_dec_esc_json;
	else if (!strncmp(argv[1], "-eu", 3))
		ss_codec1_f = ss_enc_esc_url;
	else if (!strncmp(argv[1], "-du", 3))
		ss_codec2_f = ss_dec_esc_url;
	else if (!strncmp(argv[1], "-ez", 3))
		ss_codec3_f = ss_enc_lzw;
	else if (!strncmp(argv[1], "-dz", 3))
		ss_codec4_f = ss_dec_lzw;
	else if (!strncmp(argv[1], "-er", 3))
		ss_codec3_f = ss_enc_rle;
	else if (!strncmp(argv[1], "-dr", 3))
		ss_codec4_f = ss_dec_rle;
	else
		return syntax_error(argv, 2);
	int exit_code = 0;
	unsigned char esc = ss_codec2_f == ss_dec_esc_xml ? '&' :
			    ss_codec2_f == ss_dec_esc_json ? '\\' : '%';
	ss_ref_t ref;
	uint32_t l32 = 0;
	size_t li = 0, lo = 0, j, l, ss0, off;
	const char *b;
	int cf = ss_codec1_f ? 1 : ss_codec2_f ? 2 :
		 ss_codec3_f ? 3 : ss_codec4_f ? 4 : 0;
	sbool_t done = S_FALSE;
	while (!done) {
		switch (cf){
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
		case 3: /* lzw/rle encoding */
			ss_cpy_read(&in, stdin, IBUF_SIZE);
			l = ss_size(in);
			if (!l) {
				done = S_TRUE;
				continue;
			}
			li += l;
			ss_codec3_f(&out, in);
			l32 = (uint32_t)ss_size(out);
			l32 = S_HTON_U32(l32);
			if (!fwrite(&l32, 1, 4, stdout) ||  ferror(stdout)) {
				fprintf(stderr, "Write error\n");
				exit_code = 6;
				done = S_TRUE;
				continue;
			}
			lo += 4;
			ss_clear(in);
			break;
		case 4: /* lzw/rle decoding */
			l = fread(&l32, 1, 4, stdin);
			if (!l || ferror(stdin)) {
				done = S_TRUE;
				continue;
			}
			li += l;
			if (l != 4) {
				exit_code = 2;
				fprintf(stderr, "Format error\n");
				done = S_TRUE;
				continue;
			}
			l32 = S_NTOH_U32(l32);
			if (l32 > XBUF_SIZE) {
				fprintf(stderr, "Format error\n");
				exit_code = 3;
				done = S_TRUE;
				continue;
			}
			ss_cpy_read(&in, stdin, l32);
			l = ss_size(in);
			li += l;
			if (l != l32 || ferror(stdin)) {
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
	fprintf(stderr, "in: %u bytes, out: %u bytes\n",
		(unsigned)li, (unsigned)lo);
	ss_free(&in, &out);
	return exit_code;
}


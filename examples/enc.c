/*
 * enc.c
 *
 * Buffer encoding/decoding example using libsrt
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "../src/libsrt.h"

#define IBUF_SIZE	(3 * 4 * 1024) /* 3 * 4: LCM */
#define OBUF_SIZE	(IBUF_SIZE * 6) /* Max req: xml escape */
#define ESC_MAX_SIZE	16
#define XBUF_SIZE	(S_MAX(IBUF_SIZE, OBUF_SIZE) + ESC_MAX_SIZE)

static int syntax_error(const char **argv, const int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Error [%i] Syntax: %s [-eb|-db|-eh|-eH|-dh|-ex|-dx|-ej|-dj|"
#if 0 /* will be added once LZW/RLE is added properly to the API */
		"-eu|-du|-er|-dr|-ez|-dz]\nExamples:\n"
#else
		"-eu|-du]\nExamples:\n"
#endif
		"%s -eb <in >out.b64\n%s -db <in.b64 >out\n"
		"%s -eh <in >out.hex\n%s -eH <in >out.HEX\n"
		"%s -dh <in.hex >out\n%s -dh <in.HEX >out\n"
		"%s -ex <in >out.xml.esc\n%s -dx <in.xml.esc >out\n"
		"%s -ej <in >out.json.esc\n%s -dx <in.json.esc >out\n"
		"%s -eu <in >out.url.esc\n%s -du <in.url.esc >out\n"
#if 0 /* will be added once LZW/RLE is added properly to the API */
		, "%s -er <in >in.r\n%s -dr <in.r >out\n"
		"%s -ez <in >in.z\n%s -dz <in.z >out\n",
		exit_code, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0,
		v0, v0, v0, v0, v0);
#else
		, exit_code, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0,
		v0);
#endif
	return exit_code;
}

int main(int argc, const char **argv)
{
	if (argc < 2)
		return syntax_error(argv, 1);
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
#if 0 /* will be added once LZW/RLE is added properly to the API */
	else if (!strncmp(argv[1], "-ez", 3))
		ss_codec3_f = ss_enc_lzw;
	else if (!strncmp(argv[1], "-dz", 3))
		ss_codec4_f = ss_dec_lzw;
	else if (!strncmp(argv[1], "-er", 3))
		ss_codec3_f = ss_enc_rle;
	else if (!strncmp(argv[1], "-dr", 3))
		ss_codec4_f = ss_dec_rle;
#endif
	else
		return syntax_error(argv, 2);
	int exit_code = 0;
	unsigned char esc = ss_codec2_f == ss_dec_esc_xml ? '&' :
			    ss_codec2_f == ss_dec_esc_json ? '\\' : '%';
	ss_ref_t ref;
	size_t li = 0, lo = 0, j;
	if (ss_codec1_f || ss_codec2_f || ss_codec3_f || ss_codec4_f)
	for (;;) {
		if (ss_codec1_f || ss_codec2_f) {
			size_t ss0 = ss_size(in);
			ss_cat_read(&in, stdin, IBUF_SIZE);
			size_t l = ss_size(in);
			li += (l - ss0);
			if (!l || ferror(stdin))
				goto done;
			if (ss_codec1_f) {
				ss_codec1_f(&out, in);
				ss_clear(in);
			} else {
				const char *b = ss_get_buffer_r(in);
				size_t off = 0;
				if (l > ESC_MAX_SIZE)
					for (j = (size_t)(l - ESC_MAX_SIZE + 1);
					     j < l; j++)
						if (b[j] == esc &&
						    b[j - 1] != esc) {
							off = (size_t)(l - j);
							break;
						}
				l -= off;
				ss_codec2_f(&out, ss_ref_buf(&ref, b, l));
				if (off > 0)
					ss_cpy_substr(&in, in, l, S_NPOS);
				else
					ss_clear(in);
			}
		} else {
#if 0 /* will be added once LZW/RLE is added properly to the API */
			uint32_t l32 = 0;
			if (ss_codec3_f) { /* lzw/rle encoding */
				ss_cpy_read(&in, stdin, IBUF_SIZE);
				size_t l = ss_size(in);
				if (!l)
					goto done;
				li += l;
				ss_codec3_f(&out, in);
				l32 = (uint32_t)ss_size(out);
				l32 = S_HTON_U32(l32);
				if (!fwrite(&l32, 1, 4, stdout) ||
				    ferror(stdout)) {
					fprintf(stderr, "Write error\n");
					exit_code = 6;
					goto done;
				}
				lo += 4;
				ss_clear(in);
			} else { /* lzw/rle decoding */
				size_t l = fread(&l32, 1, 4, stdin);
				if (!l || ferror(stdin))
					goto done;
				li += l;
				if (l != 4) {
					exit_code = 2;
					fprintf(stderr, "Format error\n");
					goto done;
				}
				l32 = S_NTOH_U32(l32);
				if (l32 > XBUF_SIZE) {
					fprintf(stderr, "Format error\n");
					exit_code = 3;
					goto done;
				}
				ss_cpy_read(&in, stdin, l32);
				l = ss_size(in);
				li += l;
				if (l != l32 || ferror(stdin)) {
					fprintf(stderr, "Read error\n");
					exit_code = 4;
					goto done;
				}
				ss_codec4_f(&out, in);
			}
#else
			break;
#endif
		}
		if (ss_write(stdout, out, 0, S_NPOS) < 0) {
			fprintf(stderr, "Write error\n");
			exit_code = 6;
			goto done;
		}
		lo += ss_size(out);
		ss_clear(out);
	}
done:
	fprintf(stderr, "in: %u bytes, out: %u bytes\n",
		(unsigned)li, (unsigned)lo);
	ss_free(&in, &out);
	return exit_code;
}


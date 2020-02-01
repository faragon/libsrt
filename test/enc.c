/*
 * enc.c
 *
 * Buffer encoding/decoding example using libsrt
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "../src/libsrt.h"
#include "../src/saux/senc.h" /* for debug stats */

#define LZHBUF_SIZE S_NPOS /* -ezh high compression: infinite buffer size */
#define LZBUF_SIZE 1000000 /* -ez fast compression: 1 MB buffer size */
#define IBUF_SIZE (3 * 4 * 2 * 1024) /* 3 * 4 because of LCM for base64 */
#define ESC_MAX_SIZE 16 /* maximum size for an escape sequence: 16 bytes */

static int syntax_error(const char **argv, const int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Buffer encoding/decoding (libsrt example)\n\n"
		"Syntax: %s [-eb|-db|-eh|-eH|-dh|-ex|-dx|-ej|-dj|"
		"-eu|-du|-ez|-dz|-crc32|-adler32|-fnv|-fnv1a]\n\nExamples:\n"
		"%s -eb <in >out.b64\n%s -db <in.b64 >out\n"
		"%s -eh <in >out.hex\n%s -eH <in >out.HEX\n"
		"%s -dh <in.hex >out\n%s -dh <in.HEX >out\n"
		"%s -ex <in >out.xml.esc\n%s -dx <in.xml.esc >out\n"
		"%s -ej <in >out.json.esc\n%s -dj <in.json.esc >out\n"
		"%s -eu <in >out.url.esc\n%s -du <in.url.esc >out\n"
		"%s -ez <in >in.lz\n%s -dz <in.lz >out\n"
		"%s -ezh <in >in.lz\n%s -dz <in.lz >out\n"
		"%s -crc32 <in\n%s -crc32 <in >out\n"
		"%s -adler32 <in\n%s -adler32 <in >out\n"
		"%s -fnv1 <in\n%s -fnv1 <in >out\n"
		"%s -fnv1a <in\n%s -fnv1a <in >out\n"
		"%s -mh3_32 <in\n%s -mh3_32 <in >out\n",
		v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0,
		v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0);
	return exit_code;
}

int main(int argc, const char **argv)
{
	srt_bool done;
	const char *b;
	srt_string_ref ref;
	const uint8_t *dlepc;
	int exit_code = 0, cf;
	uint8_t esc, dle[S_PK_U64_MAX_BYTES], *dlep;
	size_t lmax = 0, li = 0, lo = 0, j, l, l2, ss0, off;
	uint32_t acc = 0;
	uint32_t (*f32)(const srt_string *, uint32_t, size_t, size_t) = NULL;
	srt_string *in = NULL, *out = NULL;
	srt_string *(*ss_codec1_f)(srt_string **, const srt_string *) = NULL;
	srt_string *(*ss_codec2_f)(srt_string **, const srt_string *) = NULL;
	srt_string *(*ss_codec3_f)(srt_string **, const srt_string *) = NULL;
	srt_string *(*ss_codec4_f)(srt_string **, const srt_string *) = NULL;
	size_t lzbufsize = LZBUF_SIZE;
	if (argc < 2)
		return syntax_error(argv, 1);
	if (!strncmp(argv[1], "-crc32", 6)) {
		acc = S_CRC32_INIT;
		f32 = ss_crc32r;
	} else if (!strncmp(argv[1], "-adler32", 8)) {
		acc = S_ADLER32_INIT;
		f32 = ss_adler32r;
	} else if (!strncmp(argv[1], "-fnv1", 6)) {
		acc = S_FNV1_INIT;
		f32 = ss_fnv1r;
	} else if (!strncmp(argv[1], "-fnv1a", 7)) {
		acc = S_FNV1_INIT;
		f32 = ss_fnv1ar;
	} else if (!strncmp(argv[1], "-mh3_32", 8)) {
		acc = S_MH3_32_INIT;
		f32 = ss_mh3_32r;
	}
	if (f32) {
		in = ss_alloca(IBUF_SIZE);
		while (ss_read(&in, stdin, IBUF_SIZE))
			acc = f32(in, acc, 0, S_NPOS);
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
	else if (!strncmp(argv[1], "-ezh", 5)) {
		ss_codec3_f = ss_enc_lzh;
		lzbufsize = LZHBUF_SIZE;
	} else if (!strncmp(argv[1], "-ez", 4))
		ss_codec3_f = ss_enc_lz;
	else if (!strncmp(argv[1], "-dz", 4))
		ss_codec4_f = ss_dec_lz;
	else
		return syntax_error(argv, 2);
	esc = ss_codec2_f == ss_dec_esc_xml
		      ? '&'
		      : ss_codec2_f == ss_dec_esc_json ? '\\' : '%';
	cf = ss_codec1_f
		     ? 1
		     : ss_codec2_f ? 2 : ss_codec3_f ? 3 : ss_codec4_f ? 4 : 0;
	done = S_FALSE;
	ss_reserve(&in, IBUF_SIZE);
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
			/* For codecs handling escape characters it is
			 * necessary to check the escape sequence is not cut
			 * at boundaries.
			 */
			b = ss_get_buffer_r(in);
			off = 0;
			if (l > ESC_MAX_SIZE)
				for (j = (size_t)(l - ESC_MAX_SIZE + 1); j < l;
				     j++)
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
		case 3: /* data compression */
			ss_cpy_read(&in, stdin, lzbufsize);
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
			/* dyn LE size */
			l = (size_t)(dlep > dle ? dlep - dle : 0);
			if (!fwrite(dle, 1, l, stdout) || ferror(stdout)) {
				fprintf(stderr, "Write error\n");
				exit_code = 6;
				done = S_TRUE;
				continue;
			}
			lo += l;
			ss_clear(in);
			break;
		case 4: /* data decompression */
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
	fprintf(stderr, "in: " FMT_ZU " bytes, out: " FMT_ZU " bytes\n", li,
		lo);
#ifdef S_USE_VA_ARGS
	ss_free(&in, &out);
#else
	ss_free(&in);
	ss_free(&out);
#endif
	return exit_code;
}

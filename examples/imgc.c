/*
 * imgc.c
 *
 * Image processing example using libsrt
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 *
 * Observations:
 * - In order to have PNG and JPEG support: make imgc HAS_JPG=1 HAS_PNG=1
 */

#include "imgtools.h"

static int exit_with_error(const char **argv, const char *msg, int exit_code);

int main(int argc, const char **argv)
{
	size_t in_size = 0, enc_bytes, rgb_bytes;
	ssize_t written;
	struct RGB_Info ri;
	srt_string *iobuf = NULL, *rgb_buf = NULL;
	int exit_code = 1;
	FILE *fin = NULL, *fout = NULL;
	const char *exit_msg = "not enough parameters";
	int filter = F_None;
	enum ImgTypes t_in, t_out;
	srt_bool ro;

#define IMGC_XTEST(test, m, c)                                                 \
	if (test) {                                                            \
		exit_msg = m;                                                  \
		exit_code = c;                                                 \
		break;                                                         \
	}

	for (;;) {
		if (argc < 2)
			break;
		if (argc > 3) {
			filter = atoi(argv[3]);
			IMGC_XTEST(filter >= F_NumElems, "invalid filter", 10);
		}
		ro = argc < 3 ? S_TRUE : S_FALSE;
		t_in = file_type(argv[1]);
		t_out = ro ? IMG_none : file_type(argv[2]);
		IMGC_XTEST(t_in == IMG_error || t_out == IMG_error,
			   "invalid parameters", t_in == IMG_error ? 2 : 3);

		fin = fopen(argv[1], "rb");
		iobuf = ss_dup_read(fin, MAX_FILE_SIZE);
		in_size = ss_size(iobuf);
		IMGC_XTEST(!in_size, "input read error", 4);

		rgb_bytes = any2rgb(&rgb_buf, &ri, iobuf, t_in);
		IMGC_XTEST(!rgb_bytes, "can not process input file", 5);

		if (ro)
			printf("%s: ", argv[1]);
		enc_bytes = rgb2type(&iobuf, t_out, rgb_buf, &ri, filter);
		IMGC_XTEST(!enc_bytes, "output file encoding error", 6);

		fout = fopen(argv[2], "wb+");
		written = ro ? 0 : ss_write(fout, iobuf, 0, ss_size(iobuf));
		IMGC_XTEST(!ro
				   && (written < 0
				       || (size_t)written != ss_size(iobuf)),
			   "write error", 7);

		exit_code = 0;
		if (!ro)
			IF_DEBUG_IMGC(fprintf(
				stderr,
				"%s (%ix%i %ibpp %ich) %u bytes"
				" > %s %u bytes\n",
				argv[1], (int)ri.width, (int)ri.height,
				(int)ri.bpp, (int)ri.chn, (unsigned)in_size,
				argv[2], (unsigned)ss_size(iobuf)));
		break;
	}
#ifdef S_USE_VA_ARGS
	ss_free(&iobuf, &rgb_buf);
#else
	ss_free(&iobuf);
	ss_free(&rgb_buf);
#endif
	if (fin)
		fclose(fin);
	if (fout)
		fclose(fout);
	return exit_code ? exit_with_error(argv, exit_msg, exit_code) : 0;
}

static int exit_with_error(const char **argv, const char *msg, int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Image conversion (libsrt example)\n\n"
		"Error [%i]: %s\nSyntax: %s in." FMT_IMGS_IN " [out."
		FMT_IMGS_OUT " [filter]]\n"
		"Filters: 0 none, 1 left->right DPCM, 2 reverse of (1), "
		"3 left->right xor DPCM,\n\t 4 reverse of (3), 5 up->down DPCM"
		", 6 reverse of (5),\n\t 7 up->down xor DPCM, 8 reverse of (7)"
		", 9 left/up/up-left average DPCM,\n\t 10 reverse of (9), "
		"11 Paeth DPCM, 12 reverse of (11),\n\t 13 red substract, 14 "
		"reverse of (13), 15 green substract, 16 reverse\n\t of (15), "
		"17 blue substract, 18 reverse of (17)\n\n"
		"Examples:\n"
		"%s input.pgm  # Show image statistics\n"
		IF_PNG("%s input.png output.ppm  # Convert png to ppm\n")
		IF_JPG("%s input.jpg output.tga  # Convert jpg to tga\n")
		"%s input.ppm output.tga  # Convert ppm to tga\n"
		"%s input.ppm output.tga 5  # Convert ppm to tga + filter\n",
		exit_code, msg, v0, v0, IF_PNG(v0 MY_COMMA) IF_JPG(v0 MY_COMMA)
		v0, v0);
	return exit_code;
}

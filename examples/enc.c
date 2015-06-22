/*
 * enc.c
 *
 * Buffer encoding/decoding example using libsrt
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include <libsrt.h>

#define BUF_IN_SIZE (120LL * 1024 * 1024)

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
	if (argc < 2)
		return syntax_error(argv, 1);
	int mode = 0;
	size_t buf_out_max = 0;
	if (!strncmp(argv[1], "-eb", 3))
		mode = 1, buf_out_max = BUF_IN_SIZE * 2;
	else if (!strncmp(argv[1], "-db", 3))
		mode = 2, buf_out_max = BUF_IN_SIZE / 2;
	else if (!strncmp(argv[1], "-eh", 3))
		mode = 3, buf_out_max = (BUF_IN_SIZE * 8) / 6 + 16;
	else if (!strncmp(argv[1], "-eH", 3))
		mode = 4, buf_out_max = (BUF_IN_SIZE * 8) / 6 + 16;
	else if (!strncmp(argv[1], "-dh", 3))
		mode = 5, buf_out_max = (BUF_IN_SIZE * 6) / 8 + 16;
	else if (!strncmp(argv[1], "-ez", 3))
		mode = 6, buf_out_max = (BUF_IN_SIZE * 105) /100;
	else if (!strncmp(argv[1], "-dz", 3))
		mode = 7, buf_out_max = (BUF_IN_SIZE * 100) /105;
	else if (!strncmp(argv[1], "-er", 3))
		mode = 8, buf_out_max = (BUF_IN_SIZE * 105) /100;
	else if (!strncmp(argv[1], "-dr", 3))
		mode = 9, buf_out_max = (BUF_IN_SIZE * 100) /105;
	if (!mode)
		return syntax_error(argv, 2);
	size_t lo = 0, i = 0, imax;
	unsigned char *buf0 = (unsigned char *)malloc(BUF_IN_SIZE + buf_out_max),
		      *buf = buf0, *bufo = buf0 + BUF_IN_SIZE;
	int f_in = 0, f_out = 1;
	ssize_t l = posix_read(f_in, buf, BUF_IN_SIZE);
	if (l <= 0) {
		free(buf0);
		return syntax_error(argv, 3);
	}
	switch (argv[1][3]) {
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
		case 8: lo = senc_rle(buf, (size_t)l, bufo); break;
		case 9: lo = sdec_rle(buf, (size_t)l, bufo); break;
		}
	}
	int r = posix_write(f_out, bufo, lo);
	fprintf(stderr, "in: %u * %u bytes, out: %u * %u bytes [write result: %u]\n",
		(unsigned)l, (unsigned)i, (unsigned)lo, (unsigned)i, (unsigned)r);
	free(buf0);
	return 0;
}


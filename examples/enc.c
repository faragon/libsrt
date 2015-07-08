/*
 * enc.c
 *
 * Buffer encoding/decoding example using libsrt
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include <libsrt.h>

#define SENC_b64 1
#define SDEC_b64 2
#define SENC_hex 3
#define SENC_HEX 4
#define SDEC_hex 5
#define SENC_lzw 6
#define SDEC_lzw 7
#define SENC_rle 8
#define SDEC_rle 9

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

int main(int argc, const char **argv)
{
	if (argc < 2)
		return syntax_error(argv, 1);
	int mode = 0;
	if (!strncmp(argv[1], "-eb", 3))
		mode = SENC_b64;
	else if (!strncmp(argv[1], "-db", 3))
		mode = SDEC_b64;
	else if (!strncmp(argv[1], "-eh", 3))
		mode = SENC_hex;
	else if (!strncmp(argv[1], "-eH", 3))
		mode = SENC_HEX;
	else if (!strncmp(argv[1], "-dh", 3))
		mode = SDEC_hex;
	else if (!strncmp(argv[1], "-ez", 3))
		mode = SENC_lzw;
	else if (!strncmp(argv[1], "-dz", 3))
		mode = SDEC_lzw;
	else if (!strncmp(argv[1], "-er", 3))
		mode = SENC_rle;
	else if (!strncmp(argv[1], "-dr", 3))
		mode = SDEC_rle;
	if (!mode)
		return syntax_error(argv, 2);
	unsigned l32;
	size_t li = 0, lo = 0, lo2;
	size_t ibuf_size = 3 * 4 * 4096; /* 3 * 4: LCM */
	size_t obuf_size = ibuf_size * 2; /* Max req: bin to hex */
	size_t xbuf_size = S_MAX(ibuf_size, obuf_size);
	size_t is; /* input elem size */
	unsigned char buf[xbuf_size], bufo[xbuf_size];
	int exit_code = 0, l;
	switch (mode) {
	case SENC_b64: is = 3; break;
	case SDEC_b64: is = 4; break;
	case SENC_hex: case SENC_HEX: is = 1; break;
	case SDEC_hex: is = 2; break;
	default: is = 0;
	}
	for (;;) {
		if (is) {
			l = read(0, buf, ibuf_size);
			if (l <= 0) {
				goto done;
			}
			li += l;
			switch (mode) {
			case SENC_b64:
				lo2 = senc_b64(buf, (size_t)l, bufo);
				break;
			case SDEC_b64:
				lo2 = sdec_b64(buf, (size_t)l, bufo);
				break;
			case SENC_hex:
				lo2 = senc_hex(buf, (size_t)l, bufo);
				break;
			case SENC_HEX:
				lo2 = senc_HEX(buf, (size_t)l, bufo);
				break;
			case SDEC_hex:
				lo2 = sdec_hex(buf, (size_t)l, bufo);
				break;
			default:
				exit_code = 1;
				goto done;
			}
		} else {
			switch (mode) {
			case SENC_lzw:
			case SENC_rle:
				l = read(0, &buf, ibuf_size);
				if (l <= 0)
					goto done;
				li += l;
				lo2 = mode == SENC_lzw ?
					senc_lzw(buf, (size_t)l, bufo + 4) :
					senc_rle(buf, (size_t)l, bufo + 4);
				l32 = (unsigned)lo2;
				l32 = S_HTON_U32(l32);
				S_ST_U32(bufo, l32);
				lo2 += 4;
				break;
			case SDEC_lzw:
			case SDEC_rle:
				l = read(0, &l32, 4);
				if (l <= 0)
					goto done;
				if (l != 4) {
					exit_code = 2;
					fprintf(stderr, "Format error\n");
					goto done;
				}
				li += l;
				l32 = S_NTOH_U32(l32);
				if (l32 > xbuf_size) {
					fprintf(stderr, "Format error\n");
					exit_code = 3;
					goto done;
				}
				l = read(0, buf, l32);
				if ((unsigned)l != l32) {
					fprintf(stderr, "Read error\n");
					exit_code = 4;
					goto done;
				}
				li += l;
				lo2 = mode == SDEC_lzw ?
					sdec_lzw(buf, (size_t)l, bufo) :
					sdec_rle(buf, (size_t)l, bufo);
				break;
			default:
				exit_code = 5;
				goto done;
			}
		}
		l = write(1, bufo, lo2);
		if (l <= 0) {
			fprintf(stderr, "Write error (%i)\n", (int)l);
			exit_code = 6;
			goto done;
		}
		lo += lo2;
	}
done:
	fprintf(stderr, "in: %u bytes, out: %u bytes\n",
		(unsigned)li, (unsigned)lo);
	return exit_code;
}


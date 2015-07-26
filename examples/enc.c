/*
 * enc.c
 *
 * Buffer encoding/decoding example using libsrt
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "../src/libsrt.h"

enum EncMode
{
	SENC_none,
	SENC_b64,
	SDEC_b64,
	SENC_hex,
	SENC_HEX,
	SDEC_hex,
	SENC_xml,
	SDEC_xml,
	SENC_json,
	SDEC_json,
	SENC_url,
	SDEC_url,
	SENC_lzw,
	SDEC_lzw,
	SENC_rle,
	SDEC_rle
};

#define IBUF_SIZE	(3 * 4 * 1024) /* 3 * 4: LCM */
#define OBUF_SIZE	(IBUF_SIZE * 6) /* Max req: xml escape */
#define ESC_MAX_SIZE	16
#define XBUF_SIZE	(S_MAX(IBUF_SIZE, OBUF_SIZE) + ESC_MAX_SIZE)

static int syntax_error(const char **argv, const int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Error [%i] Syntax: %s [-eb|-db|-eh|-eH|-dh|-ex|-dx|-ej|-dj|"
		"-eu|-du|-ez|-dz]\nExamples:\n"
		"%s -eb <in >out.b64\n%s -db <in.b64 >out\n"
		"%s -eh <in >out.hex\n%s -eH <in >out.HEX\n"
		"%s -dh <in.hex >out\n%s -dh <in.HEX >out\n"
		"%s -ex <in >out.xml.esc\n%s -dx <in.xml.esc >out\n"
		"%s -ej <in >out.json.esc\n%s -dx <in.json.esc >out\n"
		"%s -ez <in >in.z\n%s -dz <in.z >out\n",
		exit_code, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0);
	return exit_code;
}

int main(int argc, const char **argv)
{
	if (argc < 2)
		return syntax_error(argv, 1);
	enum EncMode mode = SENC_none;
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
	else if (!strncmp(argv[1], "-ex", 3))
		mode = SENC_xml;
	else if (!strncmp(argv[1], "-dx", 3))
		mode = SDEC_xml;
	else if (!strncmp(argv[1], "-ej", 3))
		mode = SENC_json;
	else if (!strncmp(argv[1], "-dj", 3))
		mode = SDEC_json;
	else if (!strncmp(argv[1], "-eu", 3))
		mode = SENC_url;
	else if (!strncmp(argv[1], "-du", 3))
		mode = SDEC_url;
	else if (!strncmp(argv[1], "-ez", 3))
		mode = SENC_lzw;
	else if (!strncmp(argv[1], "-dz", 3))
		mode = SDEC_lzw;
	else if (!strncmp(argv[1], "-er", 3))
		mode = SENC_rle;
	else if (!strncmp(argv[1], "-dr", 3))
		mode = SDEC_rle;
	if (mode == SENC_none)
		return syntax_error(argv, 2);
	unsigned l32;
	size_t li = 0, lo = 0, lo2;
	size_t is; /* input elem size */
	unsigned char buf[XBUF_SIZE], bufo[XBUF_SIZE], esc;
	int exit_code = 0;
	ssize_t j, l, off = 0;
	switch (mode) {
	case SENC_b64: is = 3; break;
	case SDEC_b64: is = 4; break;
	case SENC_hex:
	case SENC_HEX: is = 1; break;
	case SDEC_hex: is = 2; break;
	case SENC_xml:
	case SDEC_xml:
	case SENC_json:
	case SDEC_json:
	case SENC_url:
	case SDEC_url: is = 1; break;
	default: is = 0;
	}
	for (;;) {
		if (is) {
			l = read(0, buf + off, IBUF_SIZE);
			if (l <= 0)
				goto done;
			l += off;
			li += (size_t)l;
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
			case SENC_xml:
				lo2 = senc_esc_xml(buf, (size_t)l, bufo, 0);
				break;
			case SENC_json:
				lo2 = senc_esc_json(buf, (size_t)l, bufo, 0);
				break;
			case SENC_url:
				lo2 = senc_esc_url(buf, (size_t)l, bufo, 0);
				break;
			case SDEC_xml:
			case SDEC_json:
			case SDEC_url:
				off = 0;
				esc = mode == SDEC_xml ? '&' :
				      mode == SDEC_json ? '\\' : '%';
				if (l > ESC_MAX_SIZE)
					for (j = l - ESC_MAX_SIZE + 1; j < l; j++)
						if (buf[j] == esc && buf[j - 1] != esc) {
							off = l - j;
							break;
						}
				l -= off;
				lo2 = mode == SDEC_xml ?
					sdec_esc_xml(buf, (size_t)l, bufo) :
				      mode == SDEC_json ?
					sdec_esc_json(buf, (size_t)l, bufo) :
					sdec_esc_url(buf, (size_t)l, bufo);
				if (off > 0)
					memcpy(buf, buf + (size_t)l, (size_t)off);
				break;
			default:
				exit_code = 1;
				goto done;
			}
		} else {
			switch (mode) {
			case SENC_lzw:
			case SENC_rle:
				l = read(0, buf, IBUF_SIZE);
				if (l <= 0)
					goto done;
				li += (size_t)l;
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
				li += (size_t)l;
				l32 = S_NTOH_U32(l32);
				if (l32 > XBUF_SIZE) {
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
				li += (size_t)l;
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


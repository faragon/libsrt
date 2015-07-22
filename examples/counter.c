/*
 * counter.c
 *
 * Code counting example using libsrt
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include "../src/libsrt.h"

static int syntax_error(const char **argv, const int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr, "Code counter (libsrt example). Returns: elements "
		"processed, unique elements\nError [%i] Syntax: %s unit_size "
		"diff_max_stop\n(1 <= unit_size <= 4; diff_max_stop = 0: no "
		"max, != 0: max)\n"
		"Examples (count colors, fast randomness test, etc.):\n"
		"%s 1 0 <in  (count unique bytes)\n"
		"%s 1 128 <in (count unique bytes, stop after 128)\n"
		"%s 2 0 <in (count 2-byte unique elements, e.g. RGB565)\n"
		"%s 2 1024 <in (count 2-byte unique elements, stop after 1024)\n"
		"%s 3 0 <in (count 3-byte unique elements, e.g. RGB888)\n"
		"%s 3 256  <in (count 3-byte unique elements, stop after 256)\n"
		"%s 4 0 <in (count 4-byte unique elements, e.g. RGBA888)\n"
		"%s 4 1000 <in (count 4-byte unique elements, stop after 1000)\n",
		exit_code, v0, v0, v0, v0, v0, v0, v0, v0, v0);
	return exit_code;
}

int main(int argc, const char **argv)
{
	if (argc < 3)
		return syntax_error(argv, 5);
	int csize = atoi(argv[1]), climit0 = atoi(argv[2]);
	if (csize < 1 || csize > 4)
		return syntax_error(argv, 6);
	if (climit0 < 0)
		return syntax_error(argv, 7);
	sb_t *bs = sb_alloc(0);
	int exit_code = 0;
	size_t count = 0;
	size_t cmax = csize == 4 ? 0xffffffff :
				   0xffffffff & ((1 << (csize * 8)) - 1);
	size_t climit = climit0 ? S_MIN((size_t)climit0, cmax) : cmax;
	sb_eval(&bs, cmax);
	unsigned char buf[3 * 4 * 128];
	int i;
	ssize_t l;
	for (;;) {
		l = read(0, buf, sizeof(buf));
		l = (l / csize) * csize;
		if (l <= 0)
			break;
		#define CNTLOOP(inc, val)			\
			for (i = 0; i < l; i += inc) {		\
				sb_set(&bs, val);		\
				count++;			\
				if (sb_popcount(bs) >= climit)	\
					goto done;		\
			}
		switch (csize) {
		case 1:	CNTLOOP(1, buf[i]);
			break;
		case 2:	CNTLOOP(2, (size_t)(buf[i] << 8 | buf[i + 1]));
			break;
		case 3:	CNTLOOP(3, (size_t)(buf[i] << 16 | buf[i + 1] << 8 | buf[i + 2]));
			break;
		case 4:	CNTLOOP(4, (size_t)buf[i] << 24 |
				   (size_t)buf[i + 1] << 16 |
				   (size_t)buf[i + 2] << 8 |
				   (size_t)buf[i + 3]);
			break;
		default:
			goto done;
		}
		#undef CNTLOOP
	}
done:
	printf(FMT_ZU ", " FMT_ZU, count, sb_popcount(bs));
	sb_free(&bs);
	return exit_code;
}


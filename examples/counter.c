/*
 * counter.c
 *
 * Code counting example using libsrt
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include <libsrt.h>

static int syntax_error(const char **argv, const int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Code counter (libsrt example). Returns: elements processed, unique elements\n"
		"Error [%i] Syntax: %s unit_size diff_max_stop\nExamples:\n"
		"%s 1 0 <in >out  (count bytes, stop when after covering all different 2^8)\n"
		"%s 1 128 <in >out (count bytes, stop when covering 128 different values)\n"
		"%s 2 0 <in >out (count 2-byte elements, e.g. RGB565, stop after covering all 2^16 codes)\n"
		"%s 2 1024 <in >out (count 2-byte elements, stop after covering 1024 different delements)\n"
		"%s 3 0 <in >out (count 3-byte elements, e.g. count RGB888 pixels)\n"
		"%s 3 256  <in >out (count 3-byte elements, stop when finding 256 different elements)\n"
		"%s 4 0 <in >out (count 4-byte elements, e.g. count RGBA888 pixels)\n"
		"%s 4 10000 <in >out (count 4-byte elements, stopping after 10000 different elements\n",
		exit_code, v0, v0, v0, v0, v0, v0, v0, v0, v0);
	return exit_code;
}

int main(int argc, const char **argv)
{
	if (argc < 3)
		return syntax_error(argv, 5);
	int csize = atoi(argv[1]), climit = atoi(argv[2]);
	if (csize < 1)
		return syntax_error(argv, 6);
	if (climit < 0)
		return syntax_error(argv, 7);
	sb_t *bs = sb_alloc(0);
	int exit_code = 0;
	size_t i, acc, next, cs8 = csize * 8, count = 0;
	for (;; count++) {
		acc = 0;
		for (i = 0; i < cs8; i += 8) {
			next = getchar();
			if (next == EOF) {
				exit_code = 1;
				goto done;
			}
			acc |= (next & 0xff) << i;
		}
		sb_set(&bs, acc);
		if (climit && sb_popcount(bs) >= climit)
			break;
	}
done:
	printf(FMT_ZU ", " FMT_ZU, count, sb_popcount(bs));
	sb_free(&bs);
	return exit_code;
}


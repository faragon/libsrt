/*
 * histogram.c
 *
 * Histogram example using libsrt
 *
 * Copyright (c) 2015-2018 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "../src/libsrt.h"

static int syntax_error(const char **argv, const int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr, "Histogram (libsrt example). Returns: list of elements "
		"and its count\nError [%i] Syntax: %s element_size  "
		"# (1 <= element_size <= 8)\n\nExamples:\n"
		"%s 1 <in >h.txt  # (1-byte element histogram, e.g. 8bpp bitmap)\n"
		"%s 2 <in >h.txt  # (2-byte element histogram, e.g. 16bpp bitmap)\n"
		"%s 3 <in >h.txt  # (3-byte element histogram, e.g. 24bpp bitmap)\n"
		"%s 4 <in >h.txt  # (4-byte element histogram, e.g. 32bpp bitmap)\n"
		"%s 5 <in >h.txt  # (5-byte element histogram)\n"
		"%s 6 <in >h.txt  # (6-byte element histogram)\n"
		"%s 7 <in >h.txt  # (7-byte element histogram)\n"
		"%s 8 <in >h.txt  # (8-byte element histogram)\n"
		"head -c 256MB </dev/urandom | %s 1 >h.txt  # Check random gen. quality\n",
		exit_code, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0);
	return exit_code;
}

static sbool_t cb32(uint32_t k, uint32_t v, void *context)
{
	S_UNUSED(context);
	printf("%08x %u\n", (unsigned)k, (unsigned)v);
	return S_TRUE;
}

static sbool_t cb64(int64_t k, int64_t v, void *context)
{
	S_UNUSED(context);
	printf("%08x%08x " FMT_U "\n",
	       (unsigned)(k >> 32), (unsigned)k, (uint64_t)v);
	return S_TRUE;
}

int main(int argc, const char **argv)
{
	sm_t *m;
	size_t i, l;
	int csize, exit_code;
	uint8_t buf[2 * 3 * 4 * 5 * 7]; /* buffer size: LCM(1..8) */
	if (argc != 2)
		return syntax_error(argv, 5);
	csize = atoi(argv[1]);
	if (csize < 1 || csize > 8)
		return syntax_error(argv, 6);
	exit_code = 0;
	m = sm_alloc(csize <= 4 ? SM_UU32 : SM_II, 0);
	for (;;) {
		l = fread(buf, 1, sizeof(buf), stdin);
		l = (l / (size_t)csize) * (size_t)csize;
		if (!l)
			break;
		#define CNTLOOP(inc, k, f)		\
			for (i = 0; i < l; i += inc)	\
				f(&m, k, 1);
		switch (csize) {
		case 1:	CNTLOOP(1, buf[i], sm_inc_uu32);
			break;
		case 2:	CNTLOOP(2, S_LD_LE_U16(buf + i), sm_inc_uu32);
			break;
		case 3:	CNTLOOP(3, S_LD_LE_U32(buf + i) & 0xffffff,
				sm_inc_uu32);
			break;
		case 4:	CNTLOOP(4, S_LD_LE_U32(buf + i), sm_inc_uu32);
			break;
		case 5:	CNTLOOP(5, S_LD_LE_U64(buf + i) & 0xffffffffffLL,
				sm_inc_ii);
			break;
		case 6:	CNTLOOP(6, S_LD_LE_U64(buf + i) & 0xffffffffffffLL,
				sm_inc_ii);
			break;
		case 7:	CNTLOOP(7, S_LD_LE_U64(buf + i) & 0xffffffffffffffLL,
				sm_inc_ii);
			break;
		case 8:	CNTLOOP(8, S_LD_LE_U64(buf + i), sm_inc_ii);
			break;
		default:
			goto done;
		}
		#undef CNTLOOP
	}
done:
	if (csize <= 4)
		sm_itr_uu32(m, 0, UINT32_MAX, cb32, NULL);
	else
		sm_itr_ii(m, INT64_MIN, INT64_MAX, cb64, NULL);
	sm_free(&m);
	return exit_code;
}


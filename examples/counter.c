/*
 * counter.c
 *
 * Code counting example using libsrt
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "../src/libsrt.h"

static int syntax_error(const char **argv, int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Code counter (libsrt example). Returns: elements "
		"processed, unique elements\n\nSyntax: %s unit_size "
		"diff_max_stop [-bs|-s|-hs]\n(1 <= unit_size <= 4; "
		"diff_max_stop = 0: no max, != 0: max; -bs: bitset (default), "
		"-s: set, -hs: hash set)\n\n"
		"Examples (count colors, fast randomness test, etc.):\n"
		"%s 1 0 <in  (count unique bytes)\n"
		"%s 1 128 <in (count unique bytes, stop after 128)\n"
		"%s 2 0 <in (count 2-byte unique elements, e.g. RGB565)\n"
		"%s 2 1024 <in (count 2-byte unique elem., stop after 1024)\n"
		"%s 3 0 <in (count 3-byte unique elements, e.g. RGB888)\n"
		"%s 3 256  <in (count 3-byte unique elem., stop after 256)\n"
		"%s 4 0 <in (count 4-byte unique elements, e.g. RGBA888)\n"
		"%s 4 1000 <in (count 4-byte unique elem., stop after 1000)\n"
		"head -c 1G </dev/urandom | %s 3 0 -bs # fastest (memory "
		"inneficient for big sets, set implemented via bit set)\n"
		"head -c 1G </dev/urandom | %s 3 0 -hs # 10x slower than bs "
		"(memory efficient, set implemented via hash table)\n"
		"head -c 1G </dev/urandom | %s 3 0 -s  # 100x slower than bs "
		"(memory efficient, set implemented via red black tree)\n",
		v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0);
	return exit_code;
}

#define BS_COUNTER_SET(bs, val) sb_set(&bs, val)
#define BS_COUNTER_POPCOUNT(bs) sb_popcount(bs)
#define S_COUNTER_SET(s, val) sms_insert_u32(&s, val)
#define S_COUNTER_POPCOUNT(s) sms_size(s)
#define HS_COUNTER_SET(hs, val) shs_insert_u32(&hs, val)
#define HS_COUNTER_POPCOUNT(hs) shs_size(hs)
#define CNTLOOP(prefix, i, count, done, climit, s, inc, val)                   \
	for (i = 0; i < l; i += inc) {                                         \
		prefix##_COUNTER_SET(s, val);                                  \
		count++;                                                       \
		if (prefix##_COUNTER_POPCOUNT(s) >= climit)                    \
			goto done;                                             \
	}

enum eCntMode { CM_BitSet, CM_Set, CM_HashSet };

int main(int argc, const char **argv)
{
	srt_set *s = NULL;
	srt_hset *hs = NULL;
	srt_bitset *bs = NULL;
	int csize, exit_code, climit0;
	enum eCntMode mode = CM_BitSet;
	size_t count, cmax, climit, i, l;
	unsigned char buf[3 * 4 * 128];
	if (argc < 3)
		return syntax_error(argv, 5);
	csize = atoi(argv[1]);
	climit0 = atoi(argv[2]);
	if (csize < 1 || csize > 4)
		return syntax_error(argv, 6);
	if (climit0 < 0)
		return syntax_error(argv, 7);
	if (argc >= 4) {
		if (!strcmp(argv[3], "-s"))
			mode = CM_Set;
		else if (!strcmp(argv[3], "-hs"))
			mode = CM_HashSet;
	}
	exit_code = 0;
	count = 0;
	cmax = csize == 4 ? 0xffffffff : 0xffffffff & ((1 << (csize * 8)) - 1);
	climit = climit0 ? S_MIN((size_t)climit0, cmax) : cmax;
	switch (mode) {
	case CM_BitSet:
		bs = sb_alloc(0);
		sb_eval(&bs, cmax);
		break;
	case CM_Set:
		s = sms_alloc(SMS_U32, 0);
		break;
	case CM_HashSet:
		hs = shs_alloc(SHS_U32, 0);
		break;
	}
	for (;;) {
		l = fread(buf, 1, sizeof(buf), stdin);
		l = (l / (size_t)csize) * (size_t)csize;
		if (!l)
			break;
		switch (csize << 4 | mode) {
		case 1 << 4 | CM_BitSet:
			CNTLOOP(BS, i, count, done, climit, bs, 1, buf[i]);
			break;
		case 2 << 4 | CM_BitSet:
			CNTLOOP(BS, i, count, done, climit, bs, 2,
				(uint32_t)buf[i] << 8 | (uint32_t)buf[i + 1]);
			break;
		case 3 << 4 | CM_BitSet:
			CNTLOOP(BS, i, count, done, climit, bs, 3,
				(uint32_t)buf[i] << 16
					| (uint32_t)buf[i + 1] << 8
					| (uint32_t)buf[i + 2]);
			break;
		case 4 << 4 | CM_BitSet:
			CNTLOOP(BS, i, count, done, climit, bs, 4,
				(uint32_t)buf[i] << 24
					| (uint32_t)buf[i + 1] << 16
					| (uint32_t)buf[i + 2] << 8
					| (uint32_t)buf[i + 3]);
			break;
		case 1 << 4 | CM_Set:
			CNTLOOP(S, i, count, done, climit, s, 1, buf[i]);
			break;
		case 2 << 4 | CM_Set:
			CNTLOOP(S, i, count, done, climit, s, 2,
				(uint32_t)buf[i] << 8 | (uint32_t)buf[i + 1]);
			break;
		case 3 << 4 | CM_Set:
			CNTLOOP(S, i, count, done, climit, s, 3,
				(uint32_t)buf[i] << 16
					| (uint32_t)buf[i + 1] << 8
					| (uint32_t)buf[i + 2]);
			break;
		case 4 << 4 | CM_Set:
			CNTLOOP(S, i, count, done, climit, s, 4,
				(uint32_t)buf[i] << 24
					| (uint32_t)buf[i + 1] << 16
					| (uint32_t)buf[i + 2] << 8
					| (uint32_t)buf[i + 3]);
			break;
		case 1 << 4 | CM_HashSet:
			CNTLOOP(HS, i, count, done, climit, hs, 1, buf[i]);
			break;
		case 2 << 4 | CM_HashSet:
			CNTLOOP(HS, i, count, done, climit, hs, 2,
				(uint32_t)buf[i] << 8 | (uint32_t)buf[i + 1]);
			break;
		case 3 << 4 | CM_HashSet:
			CNTLOOP(HS, i, count, done, climit, hs, 3,
				(uint32_t)(buf[i] << 16
					   | (uint32_t)buf[i + 1] << 8
					   | (uint32_t)buf[i + 2]));
			break;
		case 4 << 4 | CM_HashSet:
			CNTLOOP(HS, i, count, done, climit, hs, 4,
				(uint32_t)buf[i] << 24
					| (uint32_t)buf[i + 1] << 16
					| (uint32_t)buf[i + 2] << 8
					| (uint32_t)buf[i + 3]);
			break;
		default:
			goto done;
		}
	}
done:
	switch (mode) {
	case CM_BitSet:
		printf(FMT_ZU ", " FMT_ZU, count, BS_COUNTER_POPCOUNT(bs));
		sb_free(&bs);
		break;
	case CM_Set:
		printf(FMT_ZU ", " FMT_ZU, count, S_COUNTER_POPCOUNT(s));
		sm_free(&s);
		break;
	case CM_HashSet:
		printf(FMT_ZU ", " FMT_ZU, count, HS_COUNTER_POPCOUNT(hs));
		shs_free(&hs);
		break;
	}
	return exit_code;
}

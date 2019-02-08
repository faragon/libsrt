/*
 * counter.h
 *
 * Code counting examples, one using libsrt, and the other using C++/STL
 *
 * This is a template for both the C and C++ versions. The C version
 * use libsrt types and functions, and the C++ use the STL library. The
 * purpose is to compare libsrt vs C++/STL.
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#ifndef COUNTER_H
#define COUNTER_H

#include "../src/libsrt.h"
#ifdef __cplusplus
#include <bitset>
#include <set>
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
#define S_COUNTER_CPP_HS
#include <unordered_set>
#endif
#define CNT_TAG "C++"
#define BS_COUNTER_SET(bs, val) bs->set(val)
#define BS_COUNTER_POPCOUNT(bs) bs->count()
#define BS_COUNTER_POPCOUNTi(bs) ((size_t)0)
#define S_COUNTER_SET(s, val) s->insert(val)
#define S_COUNTER_POPCOUNT(s) s->size()
#define S_COUNTER_POPCOUNTi(s) s->size()
#define HS_COUNTER_SET(hs, val) hs->insert(val)
#define HS_COUNTER_POPCOUNT(hs) hs->size()
#define HS_COUNTER_POPCOUNTi(hs) hs->size()
#else
#include "../src/libsrt.h"
#define CNT_TAG "libsrt"
#define BS_COUNTER_SET(bs, val) sb_set(&bs, val)
#define BS_COUNTER_POPCOUNT(bs) sb_popcount(bs)
#define BS_COUNTER_POPCOUNTi(bs) sb_popcount(bs)
#define S_COUNTER_SET(s, val) sms_insert_u32(&s, val)
#define S_COUNTER_POPCOUNT(s) sms_size(s)
#define S_COUNTER_POPCOUNTi(s) sms_size(s)
#define HS_COUNTER_SET(hs, val) shs_insert_u32(&hs, val)
#define HS_COUNTER_POPCOUNT(hs) shs_size(hs)
#define HS_COUNTER_POPCOUNTi(hs) shs_size(hs)
#endif

static int syntax_error(const char **argv, int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Code counter (" CNT_TAG " example). Returns: elements "
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

#define CNTLOOP(csize, buf, l, prefix, i, count, climit, s, inc, val)          \
	do {                                                                   \
		l = fread(buf, 1, sizeof(buf), stdin);                         \
		l = (l / (ssize_t)csize) * (ssize_t)csize;                     \
		for (i = 0; i < l; i += inc) {                                 \
			prefix##_COUNTER_SET(s, val);                          \
			count++;                                               \
			if (prefix##_COUNTER_POPCOUNTi(s) >= climit) {         \
				l = 0;                                         \
				break;                                         \
			}                                                      \
		}                                                              \
	} while (l > 0)

enum eCntMode { CM_BitSet, CM_Set, CM_HashSet };

int main(int argc, const char **argv)
{
#ifdef __cplusplus
	std::set<uint32_t> *s = NULL;
	std::bitset<(uint32_t)-1> *bs = NULL;
#ifdef S_COUNTER_CPP_HS
	std::unordered_set<uint32_t> *hs = NULL;
#endif
#else
	srt_set *s = NULL;
	srt_hset *hs = NULL;
	srt_bitset *bs = NULL;
#endif
	ssize_t i, l;
	enum eCntMode mode = CM_BitSet;
	unsigned char buf[3 * 4 * 128];
	uint64_t count = 0, cmax, climit;
	int csize, exit_code = 0, climit0;
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
	cmax = (uint64_t)1 << (csize * 8);
	climit = climit0 ? S_MIN((uint64_t)climit0, cmax) : cmax;
	switch (mode) {
	case CM_BitSet:
#ifdef __cplusplus
		bs = new std::bitset<(uint32_t)-1>();
#else
		bs = sb_alloc(cmax);
#endif
		break;
	case CM_Set:
#ifdef __cplusplus
		s = new std::set<uint32_t>();
#else
		s = sms_alloc(SMS_U32, 0);
#endif
		break;
	case CM_HashSet:
#ifdef __cplusplus
#ifdef S_COUNTER_CPP_HS
		hs = new std::unordered_set<uint32_t>();
#else
		fprintf(stderr,
			"std::unordered_set not available, build it "
			"with CPP0X=1 or CPP11=1\n");
		return 1;
#endif
#else
		hs = shs_alloc(SHS_U32, 0);
#endif
		break;
	}
	switch (csize << 4 | mode) {
	case 1 << 4 | CM_BitSet:
		CNTLOOP(csize, buf, l, BS, i, count, climit, bs, 1, buf[i]);
		break;
	case 2 << 4 | CM_BitSet:
		CNTLOOP(csize, buf, l, BS, i, count, climit, bs, 2,
			(uint32_t)buf[i] << 8 | (uint32_t)buf[i + 1]);
		break;
	case 3 << 4 | CM_BitSet:
		CNTLOOP(csize, buf, l, BS, i, count, climit, bs, 3,
			(uint32_t)buf[i] << 16 | (uint32_t)buf[i + 1] << 8
				| (uint32_t)buf[i + 2]);
		break;
	case 4 << 4 | CM_BitSet:
		CNTLOOP(csize, buf, l, BS, i, count, climit, bs, 4,
			(uint32_t)buf[i] << 24 | (uint32_t)buf[i + 1] << 16
				| (uint32_t)buf[i + 2] << 8
				| (uint32_t)buf[i + 3]);
		break;
	case 1 << 4 | CM_Set:
		CNTLOOP(csize, buf, l, S, i, count, climit, s, 1, buf[i]);
		break;
	case 2 << 4 | CM_Set:
		CNTLOOP(csize, buf, l, S, i, count, climit, s, 2,
			(uint32_t)buf[i] << 8 | (uint32_t)buf[i + 1]);
		break;
	case 3 << 4 | CM_Set:
		CNTLOOP(csize, buf, l, S, i, count, climit, s, 3,
			(uint32_t)buf[i] << 16 | (uint32_t)buf[i + 1] << 8
				| (uint32_t)buf[i + 2]);
		break;
	case 4 << 4 | CM_Set:
		CNTLOOP(csize, buf, l, S, i, count, climit, s, 4,
			(uint32_t)buf[i] << 24 | (uint32_t)buf[i + 1] << 16
				| (uint32_t)buf[i + 2] << 8
				| (uint32_t)buf[i + 3]);
		break;
#if !defined(__cplusplus) || defined(S_COUNTER_CPP_HS)
	case 1 << 4 | CM_HashSet:
		CNTLOOP(csize, buf, l, HS, i, count, climit, hs, 1, buf[i]);
		break;
	case 2 << 4 | CM_HashSet:
		CNTLOOP(csize, buf, l, HS, i, count, climit, hs, 2,
			(uint32_t)buf[i] << 8 | (uint32_t)buf[i + 1]);
		break;
	case 3 << 4 | CM_HashSet:
		CNTLOOP(csize, buf, l, HS, i, count, climit, hs, 3,
			(uint32_t)(buf[i] << 16 | (uint32_t)buf[i + 1] << 8
				   | (uint32_t)buf[i + 2]));
		break;
	case 4 << 4 | CM_HashSet:
		CNTLOOP(csize, buf, l, HS, i, count, climit, hs, 4,
			(uint32_t)buf[i] << 24 | (uint32_t)buf[i + 1] << 16
				| (uint32_t)buf[i + 2] << 8
				| (uint32_t)buf[i + 3]);
		break;
#endif
	}
	if (bs) {
		printf(FMT_U ", " FMT_ZU "\n", count, BS_COUNTER_POPCOUNT(bs));
#ifdef __cplusplus
		delete bs;
#else
		sb_free(&bs);
#endif
	} else if (s) {
		printf(FMT_U ", " FMT_ZU "\n", count, S_COUNTER_POPCOUNT(s));
#ifdef __cplusplus
		delete s;
#else
		sm_free(&s);
#endif
	} else {
#ifdef __cplusplus
#ifdef S_COUNTER_CPP_HS
		printf(FMT_U ", " FMT_ZU "\n", count, HS_COUNTER_POPCOUNT(hs));
		delete hs;
#endif
#else
		printf(FMT_U ", " FMT_ZU "\n", count, HS_COUNTER_POPCOUNT(hs));
		shs_free(&hs);
#endif
	}
	return exit_code;
}

#endif /* #ifndef COUNTER_H */

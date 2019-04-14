/*
 * histogram.h
 *
 * Histogram examples, one using libsrt, and the other using C++/STL
 *
 * This is a template for both the C and C++ versions. The C version
 * use libsrt types and functions, and the C++ use the STL library. The
 * purpose is to compare libsrt vs C++/STL.
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "../src/libsrt.h"
#ifdef __cplusplus
#include <map>
#define S_M64 m64
#define S_HM64 hm64
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
#define S_HISTOGRAM_CPP_HM
#include <unordered_map>
#endif
#define HIST_TAG "C++"
#else
#define S_M64 m
#define S_HM64 hm
#define HIST_TAG "libsrt"
#endif

static int syntax_error(const char **argv, int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Histogram (" HIST_TAG " example). Returns: list of elements "
		"and its count\n\nSyntax: %s element_size [-hm|-m]  "
		"# (1 <= element_size <= 8; -hm: use a hash map (default), "
		"-m: use a map -slower-)\n\nExamples:\n"
		"%s 1 <in >h.txt  # (1-byte element h. e.g. 8bpp bitmap)\n"
		"%s 2 <in >h.txt  # (2-byte element h. e.g. 16bpp bitmap)\n"
		"%s 3 <in >h.txt  # (3-byte element h. e.g. 24bpp bitmap)\n"
		"%s 4 <in >h.txt  # (4-byte element h. e.g. 32bpp bitmap)\n"
		"%s 5 <in >h.txt  # (5-byte element histogram)\n"
		"%s 6 <in >h.txt  # (6-byte element histogram)\n"
		"%s 7 <in >h.txt  # (7-byte element histogram)\n"
		"%s 8 <in >h.txt  # (8-byte element histogram)\n"
		"%s 8 -m <in >h.txt  # (8-byte element histogram, using a map "
		"instead of a hash map)\nhead -c 256MB </dev/urandom | %s 1 "
		">h.txt  # Check random gen. quality\n",
		v0, v0, v0, v0, v0, v0, v0, v0, v0, v0, v0);
	return exit_code;
}

#ifdef __cplusplus
#define S_HIST_INC(f, m, key, inc, T) (*m)[key] += ((T)(inc))
#else
#define S_HIST_INC(f, m, key, inc, T) f(&m, key, ((T)(inc)))

static srt_bool cb32(uint32_t k, uint32_t v, void *context)
{
	S_UNUSED(context);
	printf("%08x %u\n", (unsigned)k, (unsigned)v);
	return S_TRUE;
}

static srt_bool cb64(int64_t k, int64_t v, void *context)
{
	unsigned vh = (unsigned)(v / 1000000000),
		 vl = (unsigned)(v % 1000000000);
	S_UNUSED(context);
	if (vh)
		printf("%08x%08x %u%u\n", (unsigned)(k >> 32), (unsigned)k, vh,
		       vl);
	else
		printf("%08x%08x %u\n", (unsigned)(k >> 32), (unsigned)k, vl);
	return S_TRUE;
}
#endif

/*
 * Local optimization: instead of calling N times to the  map for the case of
 * repeated elements, we keep the count and set the counter for all the
 * repeated elements at once (3x speed-up for non random data)
 */
#define CNTLOOP(i, l, csize, buf, rep_cnt, m, inc, k, ki, kip, f, T)           \
	do {                                                                   \
		l = fread(buf, 1, sizeof(buf), stdin);                         \
		l = (l / (size_t)csize) * (size_t)csize;                       \
		rep_cnt = 0;                                                   \
		for (i = 0; i < l; i += inc) {                                 \
			ki = k;                                                \
			if (ki == kip) {                                       \
				rep_cnt++;                                     \
				continue;                                      \
			}                                                      \
			if (rep_cnt) {                                         \
				S_HIST_INC(f, m, kip, rep_cnt, T);             \
				rep_cnt = 0;                                   \
			}                                                      \
			kip = ki;                                              \
			S_HIST_INC(f, m, ki, 1, T);                            \
		}                                                              \
		if (rep_cnt)                                                   \
			S_HIST_INC(f, m, kip, rep_cnt, T);                     \
	} while (l > 0)

enum eHistMode { HM_Map, HM_HashMap };

int main(int argc, const char **argv)
{
#ifdef __cplusplus
	std::map<uint32_t, uint32_t> *m = NULL;
	std::map<uint64_t, uint64_t> *m64 = NULL;
#ifdef S_HISTOGRAM_CPP_HM
	std::unordered_map<uint32_t, uint32_t> *hm = NULL;
	std::unordered_map<uint64_t, uint64_t> *hm64 = NULL;
#endif
#else
	srt_map *m = NULL;
	srt_hmap *hm = NULL;
#endif
	int csize, exit_code;
	enum eHistMode mode = HM_HashMap;
	size_t i, l, rep_cnt = 0;
	uint32_t k32 = 0, kp32 = 0;
	int64_t k64 = 0, kp64 = 0;
	uint8_t buf[2 * 3 * 4 * 5 * 7 * 16]; /* buffer size: LCM(1..8) */
	if (argc < 2)
		return syntax_error(argv, 5);
	if (argc >= 3) {
		if (!strcmp(argv[2], "-m"))
			mode = HM_Map;
		else if (!strcmp(argv[2], "-hm"))
			mode = HM_HashMap;
		else
			syntax_error(argv, 6);
	}
	csize = atoi(argv[1]);
	if (csize < 1 || csize > 8)
		return syntax_error(argv, 6);
	exit_code = 0;
	if (mode == HM_HashMap) {
#ifdef __cplusplus
#ifdef S_HISTOGRAM_CPP_HM
		if (csize <= 4)
			hm = new std::unordered_map<uint32_t, uint32_t>();
		else
			hm64 = new std::unordered_map<uint64_t, uint64_t>();
#else
		fprintf(stderr,
			"std::unordered_map not available, build it "
			"with CPP0X=1 or CPP11=1\n");
#endif
#else
		hm = shm_alloc(csize <= 4 ? SHM_UU32 : SHM_II, 0);
#endif
	} else {
#ifdef __cplusplus
		if (csize <= 4)
			m = new std::map<uint32_t, uint32_t>();
		else
			m64 = new std::map<uint64_t, uint64_t>();
#else
		m = sm_alloc(csize <= 4 ? SM_UU32 : SM_II, 0);
#endif
	}
	switch ((unsigned)csize << 4 | mode) {
	case 1 << 4 | HM_Map:
		CNTLOOP(i, l, csize, buf, rep_cnt, m, 1, buf[i], k32, kp32,
			sm_inc_uu32, uint32_t);
		break;
	case 2 << 4 | HM_Map:
		CNTLOOP(i, l, csize, buf, rep_cnt, m, 2, S_LD_LE_U16(buf + i),
			k32, kp32, sm_inc_uu32, uint32_t);
		break;
	case 3 << 4 | HM_Map:
		CNTLOOP(i, l, csize, buf, rep_cnt, m, 3,
			S_LD_LE_U32(buf + i) & 0xffffff, k32, kp32,
			sm_inc_uu32, uint32_t);
		break;
	case 4 << 4 | HM_Map:
		CNTLOOP(i, l, csize, buf, rep_cnt, m, 4, S_LD_LE_U32(buf + i),
			k32, kp32, sm_inc_uu32, uint32_t);
		break;
	case 5 << 4 | HM_Map:
		CNTLOOP(i, l, csize, buf, rep_cnt, S_M64, 5,
			(int64_t)S_LD_LE_U64(buf + i) & 0xffffffffffLL, k64,
			kp64, sm_inc_ii, int64_t);
		break;
	case 6 << 4 | HM_Map:
		CNTLOOP(i, l, csize, buf, rep_cnt, S_M64, 6,
			(int64_t)S_LD_LE_U64(buf + i) & 0xffffffffffffLL, k64,
			kp64, sm_inc_ii, int64_t);
		break;
	case 7 << 4 | HM_Map:
		CNTLOOP(i, l, csize, buf, rep_cnt, S_M64, 7,
			(int64_t)S_LD_LE_U64(buf + i) & 0xffffffffffffffLL, k64,
			kp64, sm_inc_ii, int64_t);
		break;
	case 8 << 4 | HM_Map:
		CNTLOOP(i, l, csize, buf, rep_cnt, S_M64, 8,
			(int64_t)S_LD_LE_U64(buf + i), k64, kp64, sm_inc_ii,
			int64_t);
		break;
#if !defined(__cplusplus) || defined(S_HISTOGRAM_CPP_HM)
	case 1 << 4 | HM_HashMap:
		CNTLOOP(i, l, csize, buf, rep_cnt, hm, 1, buf[i], k32, kp32,
			shm_inc_uu32, uint32_t);
		break;
	case 2 << 4 | HM_HashMap:
		CNTLOOP(i, l, csize, buf, rep_cnt, hm, 2, S_LD_LE_U16(buf + i),
			k32, kp32, shm_inc_uu32, uint32_t);
		break;
	case 3 << 4 | HM_HashMap:
		CNTLOOP(i, l, csize, buf, rep_cnt, hm, 3,
			S_LD_LE_U32(buf + i) & 0xffffff, k32, kp32,
			shm_inc_uu32, uint32_t);
		break;
	case 4 << 4 | HM_HashMap:
		CNTLOOP(i, l, csize, buf, rep_cnt, hm, 4, S_LD_LE_U32(buf + i),
			k32, kp32, shm_inc_uu32, uint32_t);
		break;
	case 5 << 4 | HM_HashMap:
		CNTLOOP(i, l, csize, buf, rep_cnt, S_HM64, 5,
			(int64_t)S_LD_LE_U64(buf + i) & 0xffffffffffLL, k64,
			kp64, shm_inc_ii, int64_t);
		break;
	case 6 << 4 | HM_HashMap:
		CNTLOOP(i, l, csize, buf, rep_cnt, S_HM64, 6,
			(int64_t)S_LD_LE_U64(buf + i) & 0xffffffffffffLL, k64,
			kp64, shm_inc_ii, int64_t);
		break;
	case 7 << 4 | HM_HashMap:
		CNTLOOP(i, l, csize, buf, rep_cnt, S_HM64, 7,
			(int64_t)S_LD_LE_U64(buf + i) & 0xffffffffffffffLL, k64,
			kp64, shm_inc_ii, int64_t);
		break;
	case 8 << 4 | HM_HashMap:
		CNTLOOP(i, l, csize, buf, rep_cnt, S_HM64, 8,
			(int64_t)S_LD_LE_U64(buf + i), k64, kp64, shm_inc_ii,
			int64_t);
		break;
#endif
	default:
		break;
	}
#ifdef __cplusplus
#ifdef S_HISTOGRAM_CPP_HM
	if (hm || hm64) {
		if (hm) {
			std::unordered_map<uint32_t, uint32_t>::iterator it =
				hm->begin();
			for (; it != hm->end(); it++)
				printf("%08x %u\n", (unsigned)it->first,
				       (unsigned)it->second);
			delete hm;
		} else {
			std::unordered_map<uint64_t, uint64_t>::iterator it =
				hm64->begin();
			for (; it != hm64->end(); it++)
				printf("%016" PRIx64 " %02" PRIx64  "\n",
				       it->first, it->second);
			delete hm;
			delete hm64;
		}
	} else
#endif
#else
	if (hm) {
		if (csize <= 4)
			shm_itp_uu32(hm, 0, S_NPOS, cb32, NULL);
		else
			shm_itp_ii(hm, 0, S_NPOS, cb64, NULL);
		shm_free(&hm);
	} else
#endif
	{
#ifdef __cplusplus
		if (m) {
			std::map<uint32_t, uint32_t>::iterator it = m->begin();
			for (; it != m->end(); it++)
				printf("%08x %u\n", (unsigned)it->first,
				       (unsigned)it->second);
			delete m;
		} else {
			std::map<uint64_t, uint64_t>::iterator it =
								m64->begin();
			for (; it != m64->end(); it++)
				printf("%016" PRIx64 " %" PRIx64  "\n",
				       it->first, it->second);
			delete m64;
		}
#else
		if (csize <= 4)
			sm_itr_uu32(m, 0, UINT32_MAX, cb32, NULL);
		else
			sm_itr_ii(m, INT64_MIN, INT64_MAX, cb64, NULL);
		sm_free(&m);
#endif
	}
	return exit_code;
}

#endif /* #ifndef HISTOGRAM_H */

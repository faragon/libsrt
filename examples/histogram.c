/*
 * histogram.c
 *
 * Histogram example using libsrt
 *
 * It shows how different data structures (map and hash map) behave in
 * memory usage and execution speed.
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "../src/libsrt.h"

static int syntax_error(const char **argv, int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr,
		"Histogram (libsrt example). Returns: list of elements "
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

static srt_bool cb32(uint32_t k, uint32_t v, void *context)
{
	S_UNUSED(context);
	printf("%08x %u\n", (unsigned)k, (unsigned)v);
	return S_TRUE;
}

static srt_bool cb64(int64_t k, int64_t v, void *context)
{
	S_UNUSED(context);
	printf("%08x%08x " FMT_U "\n", (unsigned)(k >> 32), (unsigned)k,
	       (uint64_t)v);
	return S_TRUE;
}

/*
 * Local optimization: instead of calling N times to the  map for the case of
 * repeated elements, we keep the count and set the counter for all the
 * repeated elements at once (3x speed-up for non random data)
 */
#define CNTLOOP(m, inc, k, ki, kip, f)                                         \
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
				f(&m, kip, rep_cnt);                           \
				rep_cnt = 0;                                   \
			}                                                      \
			kip = ki;                                              \
			f(&m, ki, 1);                                          \
		}                                                              \
		if (rep_cnt)                                                   \
			f(&m, kip, rep_cnt);                                   \
	} while (l > 0)

enum eHistMode { HM_Map, HM_HashMap };

int main(int argc, const char **argv)
{
	srt_map *m = NULL;
	srt_hmap *hm = NULL;
	int csize, exit_code;
	enum eHistMode mode = HM_HashMap;
	size_t i, l, rep_cnt = 0;
	uint32_t k32 = 0, kp32 = 0;
	uint64_t k64 = 0, kp64 = 0;
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
	if (mode == HM_HashMap)
		hm = shm_alloc(csize <= 4 ? SHM_UU32 : SHM_II, 0);
	else
		m = sm_alloc(csize <= 4 ? SM_UU32 : SM_II, 0);
	switch (csize << 4 | mode) {
	case 1 << 4 | HM_Map:
		CNTLOOP(m, 1, buf[i], k32, kp32, sm_inc_uu32);
		break;
	case 2 << 4 | HM_Map:
		CNTLOOP(m, 2, S_LD_LE_U16(buf + i), k32, kp32, sm_inc_uu32);
		break;
	case 3 << 4 | HM_Map:
		CNTLOOP(m, 3, S_LD_LE_U32(buf + i) & 0xffffff, k32, kp32,
			sm_inc_uu32);
		break;
	case 4 << 4 | HM_Map:
		CNTLOOP(m, 4, S_LD_LE_U32(buf + i), k32, kp32, sm_inc_uu32);
		break;
	case 5 << 4 | HM_Map:
		CNTLOOP(m, 5, S_LD_LE_U64(buf + i) & 0xffffffffffLL, k64, kp64,
			sm_inc_ii);
		break;
	case 6 << 4 | HM_Map:
		CNTLOOP(m, 6, S_LD_LE_U64(buf + i) & 0xffffffffffffLL, k64,
			kp64, sm_inc_ii);
		break;
	case 7 << 4 | HM_Map:
		CNTLOOP(m, 7, S_LD_LE_U64(buf + i) & 0xffffffffffffffLL, k64,
			kp64, sm_inc_ii);
		break;
	case 8 << 4 | HM_Map:
		CNTLOOP(m, 8, S_LD_LE_U64(buf + i), k64, kp64, sm_inc_ii);
		break;
	case 1 << 4 | HM_HashMap:
		CNTLOOP(hm, 1, buf[i], k32, kp32, shm_inc_uu32);
		break;
	case 2 << 4 | HM_HashMap:
		CNTLOOP(hm, 2, S_LD_LE_U16(buf + i), k32, kp32, shm_inc_uu32);
		break;
	case 3 << 4 | HM_HashMap:
		CNTLOOP(hm, 3, S_LD_LE_U32(buf + i) & 0xffffff, k32, kp32,
			shm_inc_uu32);
		break;
	case 4 << 4 | HM_HashMap:
		CNTLOOP(hm, 4, S_LD_LE_U32(buf + i), k32, kp32, shm_inc_uu32);
		break;
	case 5 << 4 | HM_HashMap:
		CNTLOOP(hm, 5, S_LD_LE_U64(buf + i) & 0xffffffffffLL, k64, kp64,
			shm_inc_ii);
		break;
	case 6 << 4 | HM_HashMap:
		CNTLOOP(hm, 6, S_LD_LE_U64(buf + i) & 0xffffffffffffLL, k64,
			kp64, shm_inc_ii);
		break;
	case 7 << 4 | HM_HashMap:
		CNTLOOP(hm, 7, S_LD_LE_U64(buf + i) & 0xffffffffffffffLL, k64,
			kp64, shm_inc_ii);
		break;
	case 8 << 4 | HM_HashMap:
		CNTLOOP(hm, 8, S_LD_LE_U64(buf + i), k64, kp64, shm_inc_ii);
		break;
	default:
		break;
	}
	if (hm) {
		if (csize <= 4)
			shm_itp_uu32(hm, 0, S_NPOS, cb32, NULL);
		else
			shm_itp_ii(hm, 0, S_NPOS, cb64, NULL);
		shm_free(&hm);
	} else {
		if (csize <= 4)
			sm_itr_uu32(m, 0, UINT32_MAX, cb32, NULL);
		else
			sm_itr_ii(m, INT64_MIN, INT64_MAX, cb64, NULL);
		sm_free(&m);
	}
	return exit_code;
}

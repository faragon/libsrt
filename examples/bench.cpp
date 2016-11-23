/*
 * bench.cpp
 *
 * Benchmarks of libsrt vs C++ STL
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "../src/libsrt.h"
#include <map>
#include <set>
#include <string>
#if __cplusplus >= 201103L
#include <unordered_map>
#endif

#if defined(__linux__) || defined(__unix__) || defined(_POSIX_VERSION)
#include <time.h>
#include <stdio.h>
#define BENCH_INIT		\
	struct timespec ta, tb
#define BENCH_TIME_US				\
	((tb.tv_sec - ta.tv_sec) * 1000000 + (tb.tv_nsec - ta.tv_nsec) / 1000)
#define BENCH_FN(test_fn, count)					\
	clock_gettime(CLOCK_REALTIME, &ta);				\
	test_fn(count);							\
	clock_gettime(CLOCK_REALTIME, &tb);				\
	printf("test %s: %u.%03u seconds (%u elements)\n", #test_fn,	\
	       (unsigned)(BENCH_TIME_MS / 1000),			\
	       (unsigned)(BENCH_TIME_MS % 1000),			\
	       (unsigned)count)
#else
#define BENCH_INIT
#define BENCH_FN
#define BENCH_TIME_US 0
#endif
#define BENCH_TIME_MS		\
	(BENCH_TIME_US / 1000)

void ctest_map_ii32(size_t count)
{
	sm_t *m = sm_alloc(SM_II32, 0);
	for (size_t i = 0; i < count; i++)
		sm_insert_ii32(&m, (int32_t)i, (int32_t)i);
	sm_free(&m);
}

void cxxtest_map_ii32(size_t count)
{
	std::map <int32_t, int32_t> m;
	for (size_t i = 0; i < count; i++)
		m[i] = (int32_t)i;
}

void ctest_map_ii64(size_t count)
{
	sm_t *m = sm_alloc(SM_II, 0);
	for (size_t i = 0; i < count; i++)
		sm_insert_ii32(&m, (int64_t)i, (int64_t)i);
	sm_free(&m);
}

void cxxtest_map_ii64(size_t count)
{
	std::map <int64_t, int64_t> m;
	for (size_t i = 0; i < count; i++)
		m[i] = (int64_t)i;
}

void ctest_map_s16(size_t count)
{
	ss_t *btmp = ss_alloca(512);
	sm_t *m = sm_alloc(SM_SS, 0);
	for (size_t i = 0; i < count; i++) {
		ss_printf(&btmp, 512, "%016i", (int)i);
		sm_insert_ss(&m, btmp, btmp);
	}
	sm_free(&m);
}

void cxxtest_map_s16(size_t count)
{
	char btmp[512];
	std::map <std::string, std::string> m;
	for (size_t i = 0; i < count; i++) {
		sprintf(btmp, "%016i", (int)i);
		m[btmp] = btmp;
	}
}

void ctest_map_s64(size_t count)
{
	ss_t *btmp = ss_alloca(512);
	sm_t *m = sm_alloc(SM_SS, 0);
	for (size_t i = 0; i < count; i++) {
		ss_printf(&btmp, 512, "%064i", (int)i);
		sm_insert_ss(&m, btmp, btmp);
	}
	sm_free(&m);
}

void cxxtest_map_s64(size_t count)
{
	char btmp[512];
	std::map <std::string, std::string> m;
	for (size_t i = 0; i < count; i++) {
		sprintf(btmp, "%064i", (int)i);
		m[btmp] = btmp;
	}
}

#if __cplusplus >= 201103L

void cxxtest_umap_ii32(size_t count)
{
	std::unordered_map <int32_t, int32_t> m;
	for (size_t i = 0; i < count; i++)
		m[i] = (int32_t)i;
}

void cxxtest_umap_ii64(size_t count)
{
	std::unordered_map <int64_t, int64_t> m;
	for (size_t i = 0; i < count; i++)
		m[i] = (int64_t)i;
}

void cxxtest_umap_s16(size_t count)
{
	char btmp[512];
	std::unordered_map <std::string, std::string> m;
	for (size_t i = 0; i < count; i++) {
		sprintf(btmp, "%016i", (int)i);
		m[btmp] = btmp;
	}
}

void cxxtest_umap_s64(size_t count)
{
	char btmp[512];
	std::unordered_map <std::string, std::string> m;
	for (size_t i = 0; i < count; i++) {
		sprintf(btmp, "%064i", (int)i);
		m[btmp] = btmp;
	}
}

#endif

int main(int argc, char *argv[])
{
	BENCH_INIT;
	size_t count = 1000000;
	BENCH_FN(ctest_map_ii32, count);
	BENCH_FN(cxxtest_map_ii32, count);
#if __cplusplus >= 201103L
	BENCH_FN(cxxtest_umap_ii32, count);
#endif
	BENCH_FN(ctest_map_ii64, count);
	BENCH_FN(cxxtest_map_ii64, count);
#if __cplusplus >= 201103L
	BENCH_FN(cxxtest_umap_ii64, count);
#endif
	BENCH_FN(ctest_map_s16, count);
	BENCH_FN(cxxtest_map_s16, count);
#if __cplusplus >= 201103L
	BENCH_FN(cxxtest_umap_s16, count);
#endif
	BENCH_FN(ctest_map_s64, count);
	BENCH_FN(cxxtest_map_s64, count);
#if __cplusplus >= 201103L
	BENCH_FN(cxxtest_umap_s64, count);
#endif
	return 0;
}


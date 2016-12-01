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
#include <vector>
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
	((tb.tv_sec - ta.tv_sec) * 1000000 +	\
	 (tb.tv_nsec - ta.tv_nsec) / 1000)
#define BENCH_FN(test_fn, count, nread, delete_all)	\
	clock_gettime(CLOCK_REALTIME, &ta);		\
	test_fn(count, nread, delete_all);		\
	clock_gettime(CLOCK_REALTIME, &tb);		\
	printf("| %s | %zu | %zu | - | %u.%03u |\n",	\
	       #test_fn, count, count * nread,		\
	       (unsigned)(BENCH_TIME_MS / 1000),	\
	       (unsigned)(BENCH_TIME_MS % 1000))
#else
#define BENCH_INIT
#define BENCH_FN(test_fn, count, nread, delete_all)	\
	test_fn(count, nread, delete_all)
#define BENCH_TIME_US 0
#endif
#define BENCH_TIME_MS (BENCH_TIME_US / 1000)

void ctest_map_ii32(size_t count, size_t read_ntimes, bool delete_all)
{
	sm_t *m = sm_alloc(SM_II32, 0);
	for (size_t i = 0; i < count; i++)
		sm_insert_ii32(&m, (int32_t)i, (int32_t)i);
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++)
			(void)sm_at_ii32(m, (int32_t)i);
	if (delete_all)
		for (size_t i = 0; i < count; i++)
			sm_delete_i(m, (int32_t)i);
	sm_free(&m);
}

void cxxtest_map_ii32(size_t count, size_t read_ntimes, bool delete_all)
{
	std::map <int32_t, int32_t> m;
	for (size_t i = 0; i < count; i++)
		m[i] = (int32_t)i;
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++)
			(void)m[i];
	if (delete_all)
		for (size_t i = 0; i < count; i++)
			m.erase((int32_t)i);
}

void ctest_map_ii64(size_t count, size_t read_ntimes, bool delete_all)
{
	sm_t *m = sm_alloc(SM_II, 0);
	for (size_t i = 0; i < count; i++)
		sm_insert_ii(&m, (int64_t)i, (int64_t)i);
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++)
			(void)sm_at_ii(m, (int64_t)i);
	if (delete_all)
		for (size_t i = 0; i < count; i++)
			sm_delete_i(m, (int64_t)i);
	sm_free(&m);
}

void cxxtest_map_ii64(size_t count, size_t read_ntimes, bool delete_all)
{
	std::map <int64_t, int64_t> m;
	for (size_t i = 0; i < count; i++)
		m[i] = (int64_t)i;
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++)
			(void)m[i];
	if (delete_all)
		for (size_t i = 0; i < count; i++)
			m.erase((int64_t)i);
}

void ctest_map_s16(size_t count, size_t read_ntimes, bool delete_all)
{
	ss_t *btmp = ss_alloca(512);
	sm_t *m = sm_alloc(SM_SS, 0);
	for (size_t i = 0; i < count; i++) {
		ss_printf(&btmp, 512, "%016i", (int)i);
		sm_insert_ss(&m, btmp, btmp);
	}
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++) {
			ss_printf(&btmp, 512, "%016i", (int)i);
			(void)sm_at_ss(m, btmp);
		}
	if (delete_all)
		for (size_t i = 0; i < count; i++) {
			ss_printf(&btmp, 512, "%016i", (int)i);
			sm_delete_s(m, btmp);
		}
	sm_free(&m);
}

void cxxtest_map_s16(size_t count, size_t read_ntimes, bool delete_all)
{
	char btmp[512];
	std::map <std::string, std::string> m;
	for (size_t i = 0; i < count; i++) {
		sprintf(btmp, "%016i", (int)i);
		m[btmp] = btmp;
	}
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++) {
			sprintf(btmp, "%016i", (int)i);
			(void)m[btmp];
		}
	if (delete_all)
		for (size_t i = 0; i < count; i++) {
			sprintf(btmp, "%016i", (int)i);
			m.erase(btmp);
		}
}

void ctest_map_s64(size_t count, size_t read_ntimes, bool delete_all)
{
	ss_t *btmp = ss_alloca(512);
	sm_t *m = sm_alloc(SM_SS, 0);
	for (size_t i = 0; i < count; i++) {
		ss_printf(&btmp, 512, "%064i", (int)i);
		sm_insert_ss(&m, btmp, btmp);
	}
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++) {
			ss_printf(&btmp, 512, "%064i", (int)i);
			(void)sm_at_ss(m, btmp);
		}
	if (delete_all)
		for (size_t i = 0; i < count; i++) {
			ss_printf(&btmp, 512, "%064i", (int)i);
			sm_delete_s(m, btmp);
		}
	sm_free(&m);
}

void cxxtest_map_s64(size_t count, size_t read_ntimes, bool delete_all)
{
	char btmp[512];
	std::map <std::string, std::string> m;
	for (size_t i = 0; i < count; i++) {
		sprintf(btmp, "%064i", (int)i);
		m[btmp] = btmp;
	}
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++) {
			sprintf(btmp, "%064i", (int)i);
			(void)m[btmp];
		}
	if (delete_all)
		for (size_t i = 0; i < count; i++) {
			sprintf(btmp, "%064i", (int)i);
			m.erase(btmp);
		}
}

#if __cplusplus >= 201103L

void cxxtest_umap_ii32(size_t count, size_t read_ntimes, bool delete_all)
{
	std::unordered_map <int32_t, int32_t> m;
	for (size_t i = 0; i < count; i++)
		m[i] = (int32_t)i;
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++)
			(void)m.count(i);
	if (delete_all)
		for (size_t i = 0; i < count; i++)
			m.erase((int32_t)i);
}

void cxxtest_umap_ii64(size_t count, size_t read_ntimes, bool delete_all)
{
	std::unordered_map <int64_t, int64_t> m;
	for (size_t i = 0; i < count; i++)
		m[i] = (int64_t)i;
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++)
			(void)m.count(i);
	if (delete_all)
		for (size_t i = 0; i < count; i++)
			m.erase((int64_t)i);
}

void cxxtest_umap_s16(size_t count, size_t read_ntimes, bool delete_all)
{
	char btmp[512];
	std::unordered_map <std::string, std::string> m;
	for (size_t i = 0; i < count; i++) {
		sprintf(btmp, "%016i", (int)i);
		m[btmp] = btmp;
	}
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++) {
			sprintf(btmp, "%016i", (int)i);
			(void)m.count(btmp);
		}
	if (delete_all)
		for (size_t i = 0; i < count; i++) {
			sprintf(btmp, "%016i", (int)i);
			m.erase(btmp);
		}
}

void cxxtest_umap_s64(size_t count, size_t read_ntimes, bool delete_all)
{
	char btmp[512];
	std::unordered_map <std::string, std::string> m;
	for (size_t i = 0; i < count; i++) {
		sprintf(btmp, "%064i", (int)i);
		m[btmp] = btmp;
	}
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++) {
			sprintf(btmp, "%064i", (int)i);
			(void)m.count(btmp);
		}
	if (delete_all)
		for (size_t i = 0; i < count; i++) {
			sprintf(btmp, "%064i", (int)i);
			m.erase(btmp);
		}
}

#endif

void ctest_set_i32(size_t count, size_t read_ntimes, bool delete_all)
{
	sms_t *m = sms_alloc(SMS_I32, 0);
	for (size_t i = 0; i < count; i++)
		sms_insert_i32(&m, (int32_t)i);
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++)
			(void)sms_count_i(m, (int32_t)i);
	if (delete_all)
		for (size_t i = 0; i < count; i++)
			sms_delete_i(m, (int32_t)i);
	sms_free(&m);
}

void cxxtest_set_i32(size_t count, size_t read_ntimes, bool delete_all)
{
	std::set <int32_t> m;
	for (size_t i = 0; i < count; i++)
		m.insert((int32_t)i);
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++)
			(void)m.count(i);
	if (delete_all)
		for (size_t i = 0; i < count; i++)
			m.erase((int32_t)i);
}

void ctest_set_i64(size_t count, size_t read_ntimes, bool delete_all)
{
	sms_t *m = sms_alloc(SMS_I, 0);
	for (size_t i = 0; i < count; i++)
		sms_insert_i32(&m, (int64_t)i);
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++)
			(void)sms_count_i(m, (int64_t)i);
	if (delete_all)
		for (size_t i = 0; i < count; i++)
			sms_delete_i(m, (int64_t)i);
	sms_free(&m);
}

void cxxtest_set_i64(size_t count, size_t read_ntimes, bool delete_all)
{
	std::set <int64_t> m;
	for (size_t i = 0; i < count; i++)
		m.insert((int64_t)i);
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++)
			(void)m.count(i);
	if (delete_all)
		for (size_t i = 0; i < count; i++)
			m.erase((int64_t)i);
}

void ctest_set_s16(size_t count, size_t read_ntimes, bool delete_all)
{
	ss_t *btmp = ss_alloca(512);
	sms_t *m = sms_alloc(SMS_S, 0);
	for (size_t i = 0; i < count; i++) {
		ss_printf(&btmp, 512, "%016i", (int)i);
		sms_insert_s(&m, btmp);
	}
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++) {
			ss_printf(&btmp, 512, "%016i", (int)i);
			(void)sms_count_s(m, btmp);
		}
	if (delete_all)
		for (size_t i = 0; i < count; i++) {
			ss_printf(&btmp, 512, "%016i", (int)i);
			sms_delete_s(m, btmp);
		}
	sms_free(&m);
}

void cxxtest_set_s16(size_t count, size_t read_ntimes, bool delete_all)
{
	char btmp[512];
	std::set <std::string> m;
	for (size_t i = 0; i < count; i++) {
		sprintf(btmp, "%016i", (int)i);
		m.insert(btmp);
	}
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++) {
			sprintf(btmp, "%016i", (int)i);
			(void)m.count(btmp);
		}
	if (delete_all)
		for (size_t i = 0; i < count; i++) {
			sprintf(btmp, "%016i", (int)i);
			m.erase(btmp);
		}
}

void ctest_set_s64(size_t count, size_t read_ntimes, bool delete_all)
{
	ss_t *btmp = ss_alloca(512);
	sms_t *m = sms_alloc(SMS_S, 0);
	for (size_t i = 0; i < count; i++) {
		ss_printf(&btmp, 512, "%064i", (int)i);
		sms_insert_s(&m, btmp);
	}
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++) {
			ss_printf(&btmp, 512, "%064i", (int)i);
			(void)sms_count_s(m, btmp);
		}
	if (delete_all)
		for (size_t i = 0; i < count; i++) {
			ss_printf(&btmp, 512, "%064i", (int)i);
			sms_delete_s(m, btmp);
		}
	sms_free(&m);
}

void cxxtest_set_s64(size_t count, size_t read_ntimes, bool delete_all)
{
	char btmp[512];
	std::set <std::string> m;
	for (size_t i = 0; i < count; i++) {
		sprintf(btmp, "%064i", (int)i);
		m.insert(btmp);
	}
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++) {
			sprintf(btmp, "%064i", (int)i);
			(void)m.count(btmp);
		}
	if (delete_all)
		for (size_t i = 0; i < count; i++) {
			sprintf(btmp, "%064i", (int)i);
			m.erase(btmp);
		}
}

void ctest_vector_i(enum eSV_Type t, size_t count, size_t read_ntimes, bool delete_all)
{
	sv_t *v = sv_alloc_t(t, 0);
	for (size_t i = 0; i < count; i++)
		sv_push_i(&v, (int32_t)i);
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++)
			(void)sv_at_i(v, i);
	if (delete_all)
		for (size_t i = 0; i < count; i++)
			(void)sv_pop_i(v);
	sv_free(&v);
}

void ctest_vector_i8(size_t count, size_t read_ntimes, bool delete_all)
{
	ctest_vector_i(SV_I32, count, read_ntimes, delete_all);
}

void ctest_vector_i16(size_t count, size_t read_ntimes, bool delete_all)
{
	ctest_vector_i(SV_I16, count, read_ntimes, delete_all);
}

void ctest_vector_i32(size_t count, size_t read_ntimes, bool delete_all)
{
	ctest_vector_i(SV_I32, count, read_ntimes, delete_all);
}

void ctest_vector_i64(size_t count, size_t read_ntimes, bool delete_all)
{
	ctest_vector_i(SV_I64, count, read_ntimes, delete_all);
}

template <typename T>
void cxxtest_vector(size_t count, size_t read_ntimes, bool delete_all)
{
	std::vector <T> v;
	for (size_t i = 0; i < count; i++)
		v.push_back((T)i);
	for (size_t j = 0; j < read_ntimes; j++)
		for (size_t i = 0; i < count; i++)
			(void)v.at(i);
	if (delete_all)
		for (size_t i = 0; i < count; i++)
			(void)v.pop_back();
}

void cxxtest_vector_i8(size_t count, size_t read_ntimes, bool delete_all)
{
	cxxtest_vector<int8_t>(count, read_ntimes, delete_all);
}

void cxxtest_vector_i16(size_t count, size_t read_ntimes, bool delete_all)
{
	cxxtest_vector<int16_t>(count, read_ntimes, delete_all);
}

void cxxtest_vector_i32(size_t count, size_t read_ntimes, bool delete_all)
{
	cxxtest_vector<int32_t>(count, read_ntimes, delete_all);
}

void cxxtest_vector_i64(size_t count, size_t read_ntimes, bool delete_all)
{
	cxxtest_vector<int64_t>(count, read_ntimes, delete_all);
}

int main(int argc, char *argv[])
{
	BENCH_INIT;
	const size_t ntests = 3;
	size_t count[ntests] = { 1000000, 1000000, 1000000 };
	size_t nread[ntests] = { 0, 10, 0 };
	bool delete_all[ntests] = { false, false, true };
	char label[ntests][512];
	snprintf(label[0], 512, "Insert %zu elements, cleanup", count[0]);
	snprintf(label[1], 512, "Insert %zu elements, read all elements %zu "
		 "times, cleanup", count[1], nread[1]);
	snprintf(label[2], 512, "Insert %zu elements, delete all elements one "
		 "by one, cleanup", count[1]);
	for (size_t i = 0; i < ntests; i++) {
		printf("\n%s\n| Test | Insert count | Read count | Memory (MB) "
		       "| Execution time (s) |\n|:---:|:---:|:---:|:---:|:---:|"
		       "\n", label[i]);
		BENCH_FN(ctest_map_ii32, count[i], nread[i], delete_all[i]);
		BENCH_FN(cxxtest_map_ii32, count[i], nread[i], delete_all[i]);
#if __cplusplus >= 201103L
		BENCH_FN(cxxtest_umap_ii32, count[i], nread[i], delete_all[i]);
#endif
		BENCH_FN(ctest_map_ii64, count[i], nread[i], delete_all[i]);
		BENCH_FN(cxxtest_map_ii64, count[i], nread[i], delete_all[i]);
#if __cplusplus >= 201103L
		BENCH_FN(cxxtest_umap_ii64, count[i], nread[i], delete_all[i]);
#endif
		BENCH_FN(ctest_map_s16, count[i], nread[i], delete_all[i]);
		BENCH_FN(cxxtest_map_s16, count[i], nread[i], delete_all[i]);
#if __cplusplus >= 201103L
		BENCH_FN(cxxtest_umap_s16, count[i], nread[i], delete_all[i]);
#endif
		BENCH_FN(ctest_map_s64, count[i], nread[i], delete_all[i]);
		BENCH_FN(cxxtest_map_s64, count[i], nread[i], delete_all[i]);
#if __cplusplus >= 201103L
		BENCH_FN(cxxtest_umap_s64, count[i], nread[i], delete_all[i]);
#endif
		BENCH_FN(ctest_set_i32, count[i], nread[i], delete_all[i]);
		BENCH_FN(cxxtest_set_i32, count[i], nread[i], delete_all[i]);
		BENCH_FN(ctest_set_i64, count[i], nread[i], delete_all[i]);
		BENCH_FN(cxxtest_set_i64, count[i], nread[i], delete_all[i]);
		BENCH_FN(ctest_set_s16, count[i], nread[i], delete_all[i]);
		BENCH_FN(cxxtest_set_s16, count[i], nread[i], delete_all[i]);
		BENCH_FN(ctest_set_s64, count[i], nread[i], delete_all[i]);
		BENCH_FN(cxxtest_set_s64, count[i], nread[i], delete_all[i]);
		BENCH_FN(ctest_vector_i8, count[i], nread[i], delete_all[i]);
		BENCH_FN(cxxtest_vector_i8, count[i], nread[i], delete_all[i]);
		BENCH_FN(ctest_vector_i16, count[i], nread[i], delete_all[i]);
		BENCH_FN(cxxtest_vector_i16, count[i], nread[i], delete_all[i]);
		BENCH_FN(ctest_vector_i32, count[i], nread[i], delete_all[i]);
		BENCH_FN(cxxtest_vector_i32, count[i], nread[i], delete_all[i]);
		BENCH_FN(ctest_vector_i64, count[i], nread[i], delete_all[i]);
		BENCH_FN(cxxtest_vector_i64, count[i], nread[i], delete_all[i]);
	}
	return 0;
}


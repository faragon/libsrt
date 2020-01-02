/*
 * bench.cc
 *
 * Benchmarks of libsrt vs C++ STL
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "../src/libsrt.h"
#include "../test/utf8_examples.h"
#include <algorithm>
#include <bitset>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <sstream>

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
#define S_BENCH_CPP_HM
#endif

#ifdef S_BENCH_CPP_HM
#include <unordered_map>
#include <unordered_set>
#endif

#if (defined(__linux__) || defined(__unix__) || defined(_POSIX_VERSION)) && \
    !defined(__APPLE_CC__)
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#define BENCH_INIT		\
	struct timespec ta, tb;	\
	bool t_res = false
#define BENCH_TIME_US				\
	((tb.tv_sec - ta.tv_sec) * 1000000 +	\
	 (tb.tv_nsec - ta.tv_nsec) / 1000)
#define BENCH_FN(test_fn, count, tid)			\
	clock_gettime(CLOCK_REALTIME, &ta);			\
	t_res = test_fn(count, tid);				\
	clock_gettime(CLOCK_REALTIME, &tb);			\
	if (t_res)						\
		printf("| %s | " FMT_ZU " | - | %u.%06u |\n",	\
		       #test_fn, count, 			\
		       BENCH_TIME_US_I, BENCH_TIME_US_D);	\
	fflush(stdout)
#define HOLD_EXEC(tid)				\
	if (TIdTest(tid, TId_SleepForever))	\
		for (; true; sleep(1))
#else
#define BENCH_INIT
#define BENCH_FN(test_fn, count, tid)	\
	test_fn(count, tid)
#define BENCH_TIME_US 0
#define HOLD_EXEC(tid)
#endif
#define BENCH_TIME_MS (BENCH_TIME_US / 1000)
#define BENCH_TIME_US_I ((unsigned)(BENCH_TIME_US / 1000000))
#define BENCH_TIME_US_D ((unsigned)(BENCH_TIME_US % 1000000))
#define S_TEST_ELEMS 500000
#define S_TEST_ELEMS_SHORT (S_TEST_ELEMS / 10000)

#define TId_Base		(1<<0)
#define TId_Read10Times		(1<<1)
#define TId_DeleteOneByOne	(1<<2)
#define TId_SleepForever	(1<<3)
#define TId_Sort10Times		(1<<4)
#define TId_ReverseSort		(1<<5)
#define TId_Sort10000Times	(1<<6)
#define TId2Count(id) ((id & TId_Read10Times) != 0 ? 10 : 0)
#define TIdTest(id, key) ((id & key) == key)

#define LIBSRTM_BENCH(FN, TID, TK, TV, INSF, ATF, DELF)	\
	bool FN(size_t count, int tid) { \
		RETURN_IF(!TIdTest(tid, TId_Base) && \
			  !TIdTest(tid, TId_Read10Times) && \
			  !TIdTest(tid, TId_DeleteOneByOne), false); \
		srt_map *m = sm_alloc(TID, 0); \
		for (size_t i = 0; i < count; i++) \
			INSF(&m, (TK)i, (TV)i); \
		for (size_t j = 0; j < TId2Count(tid); j++) \
			for (size_t i = 0; i < count; i++) \
				(void)ATF(m, (TK)i); \
		if (TIdTest(tid, TId_DeleteOneByOne)) \
			for (size_t i = 0; i < count; i++) \
				DELF(m, (TK)i); \
		HOLD_EXEC(tid); \
		sm_free(&m); \
		return true; \
	}

LIBSRTM_BENCH(libsrt_map_ii32, SM_II32, int32_t, int32_t, sm_insert_ii32,
	      sm_at_ii32, sm_delete_i)
LIBSRTM_BENCH(libsrt_map_uu32, SM_UU32, uint32_t, uint32_t, sm_insert_uu32,
	      sm_at_uu32, sm_delete_i)
LIBSRTM_BENCH(libsrt_map_ii64, SM_II, int64_t, int64_t, sm_insert_ii,
	      sm_at_ii, sm_delete_i)
LIBSRTM_BENCH(libsrt_map_ff, SM_FF, float, float, sm_insert_ff,
	      sm_at_ff, sm_delete_f)
LIBSRTM_BENCH(libsrt_map_dd, SM_DD, double, double, sm_insert_dd,
	      sm_at_dd, sm_delete_d)

template <class TK, class TV>
bool cxx_map_ii(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base) && !TIdTest(tid, TId_Read10Times) &&
		  !TIdTest(tid, TId_DeleteOneByOne), false);
	std::map <TK, TV> m;
	for (size_t i = 0; i < count; i++)
		m[(TK)i] = (TV)i;
	for (size_t j = 0; j < TId2Count(tid); j++)
		for (size_t i = 0; i < count; i++)
			(void)m[i];
	if (TIdTest(tid, TId_DeleteOneByOne))
		for (size_t i = 0; i < count; i++)
			m.erase((TK)i);
	HOLD_EXEC(tid);
	return true;
}

#define cxx_map_ii32 cxx_map_ii<int32_t, int32_t>
#define cxx_map_uu32 cxx_map_ii<uint32_t, uint32_t>
#define cxx_map_ii64 cxx_map_ii<int64_t, int64_t>
#define cxx_map_ff cxx_map_ii<float, float>
#define cxx_map_dd cxx_map_ii<double, double>

#define LIBSRTMS_BENCH(FN, FMT)	\
	bool FN(size_t count, int tid) { \
		RETURN_IF(!TIdTest(tid, TId_Base) && \
			  !TIdTest(tid, TId_Read10Times) && \
			  !TIdTest(tid, TId_DeleteOneByOne), false); \
		srt_string *btmp = ss_alloca(512); \
		srt_map *m = sm_alloc(SM_SS, 0); \
		for (size_t i = 0; i < count; i++) { \
			ss_printf(&btmp, 512, FMT, (int)i); \
			sm_insert_ss(&m, btmp, btmp); \
		} \
		for (size_t j = 0; j < TId2Count(tid); j++) \
			for (size_t i = 0; i < count; i++) { \
				ss_printf(&btmp, 512, "%016i", (int)i); \
				(void)sm_at_ss(m, btmp); \
			} \
		if (TIdTest(tid, TId_DeleteOneByOne)) \
			for (size_t i = 0; i < count; i++) { \
				ss_printf(&btmp, 512, "%016i", (int)i); \
				sm_delete_s(m, btmp); \
			} \
		HOLD_EXEC(tid); \
		sm_free(&m); \
		return true; \
	}

LIBSRTMS_BENCH(libsrt_map_s16, "%016i")
LIBSRTMS_BENCH(libsrt_map_s64, "%064i")

#define CXXMS_BENCH(FN, FMT)	\
	bool FN(size_t count, int tid) { \
		RETURN_IF(!TIdTest(tid, TId_Base) && \
			  !TIdTest(tid, TId_Read10Times) && \
			  !TIdTest(tid, TId_DeleteOneByOne), false); \
		char btmp[512]; \
		std::map <std::string, std::string> m; \
		for (size_t i = 0; i < count; i++) { \
			sprintf(btmp, FMT, (int)i); \
			m[btmp] = btmp; \
		} \
		for (size_t j = 0; j < TId2Count(tid); j++) \
			for (size_t i = 0; i < count; i++) { \
				sprintf(btmp, FMT, (int)i); \
				(void)m[btmp]; \
			} \
		if (TIdTest(tid, TId_DeleteOneByOne)) \
			for (size_t i = 0; i < count; i++) { \
				sprintf(btmp, "%016i", (int)i); \
				m.erase(btmp); \
			} \
		HOLD_EXEC(tid); \
		return true; \
	}

CXXMS_BENCH(cxx_map_s16, "%016i")
CXXMS_BENCH(cxx_map_s64, "%064i")

#define LIBSRTHM_BENCH(FN, TID, TK, TV, INSF, ATF, DELF)	\
	bool FN(size_t count, int tid) { \
		RETURN_IF(!TIdTest(tid, TId_Base) && \
			  !TIdTest(tid, TId_Read10Times) && \
			  !TIdTest(tid, TId_DeleteOneByOne), false); \
		srt_hmap *m = shm_alloc(TID, 0); \
		for (size_t i = 0; i < count; i++) \
			INSF(&m, (TK)i, (TV)i); \
		for (size_t j = 0; j < TId2Count(tid); j++) \
			for (size_t i = 0; i < count; i++) \
				(void)ATF(m, (TK)i); \
		if (TIdTest(tid, TId_DeleteOneByOne)) \
			for (size_t i = 0; i < count; i++) \
				DELF(m, (TK)i); \
		HOLD_EXEC(tid); \
		shm_free(&m); \
		return true;\
	}

LIBSRTHM_BENCH(libsrt_hmap_ii32, SHM_II32, int32_t, int32_t, shm_insert_ii32,
		shm_at_ii32, shm_delete_i32)
LIBSRTHM_BENCH(libsrt_hmap_uu32, SHM_UU32, uint32_t, uint32_t, shm_insert_uu32,
		shm_at_uu32, shm_delete_i32)
LIBSRTHM_BENCH(libsrt_hmap_ii64, SHM_II, int64_t, int64_t, shm_insert_ii,
		shm_at_ii, shm_delete_i)
LIBSRTHM_BENCH(libsrt_hmap_ff, SHM_FF, float, float, shm_insert_ff,
		shm_at_ff, shm_delete_f)
LIBSRTHM_BENCH(libsrt_hmap_dd, SHM_DD, double, double, shm_insert_dd,
		shm_at_dd, shm_delete_d)

#define LIBSRTHMS_BENCH(FN, FMT)	\
	bool FN(size_t count, int tid) { \
		RETURN_IF(!TIdTest(tid, TId_Base) && \
			  !TIdTest(tid, TId_Read10Times) && \
			  !TIdTest(tid, TId_DeleteOneByOne), false); \
		srt_string *btmp = ss_alloca(512); \
		srt_hmap *m = shm_alloc(SHM_SS, 0); \
		for (size_t i = 0; i < count; i++) { \
			ss_printf(&btmp, 512, FMT, (int)i); \
			shm_insert_ss(&m, btmp, btmp); \
		} \
		for (size_t j = 0; j < TId2Count(tid); j++) \
			for (size_t i = 0; i < count; i++) { \
				ss_printf(&btmp, 512, FMT, (int)i); \
				(void)shm_at_ss(m, btmp); \
			} \
		if (TIdTest(tid, TId_DeleteOneByOne)) \
			for (size_t i = 0; i < count; i++) { \
				ss_printf(&btmp, 512, FMT, (int)i); \
				shm_delete_s(m, btmp); \
			} \
		HOLD_EXEC(tid); \
		shm_free(&m); \
		return true; \
	}

LIBSRTHMS_BENCH(libsrt_hmap_s16, "%016i")
LIBSRTHMS_BENCH(libsrt_hmap_s64, "%064i")

#ifdef S_BENCH_CPP_HM

template <class TK, class TV>
bool cxx_umap_ii(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base) && !TIdTest(tid, TId_Read10Times) &&
		  !TIdTest(tid, TId_DeleteOneByOne), false);
	std::unordered_map <TK, TV> m;
	for (size_t i = 0; i < count; i++)
		m[(TK)i] = (TV)i;
	for (size_t j = 0; j < TId2Count(tid); j++)
		for (size_t i = 0; i < count; i++)
			(void)m.count(i);
	if (TIdTest(tid, TId_DeleteOneByOne))
		for (size_t i = 0; i < count; i++)
			m.erase((TK)i);
	HOLD_EXEC(tid);
	return true;
}

#define cxx_umap_ii32 cxx_umap_ii<int32_t, int32_t>
#define cxx_umap_uu32 cxx_umap_ii<uint32_t, uint32_t>
#define cxx_umap_ii64 cxx_umap_ii<int64_t, int64_t>
#define cxx_umap_ff cxx_umap_ii<float, float>
#define cxx_umap_dd cxx_umap_ii<double, double>

#define CXXHMS_BENCH(FN, FMT)	\
	bool FN(size_t count, int tid) { \
		RETURN_IF(!TIdTest(tid, TId_Base) && \
			  !TIdTest(tid, TId_Read10Times) && \
			  !TIdTest(tid, TId_DeleteOneByOne), false); \
		char btmp[512]; \
		std::unordered_map <std::string, std::string> m; \
		for (size_t i = 0; i < count; i++) { \
			sprintf(btmp, "%016i", (int)i); \
			m[btmp] = btmp; \
		} \
		for (size_t j = 0; j < TId2Count(tid); j++) \
			for (size_t i = 0; i < count; i++) { \
				sprintf(btmp, "%016i", (int)i); \
				(void)m.count(btmp); \
			} \
		if (TIdTest(tid, TId_DeleteOneByOne)) \
			for (size_t i = 0; i < count; i++) { \
				sprintf(btmp, "%016i", (int)i); \
				m.erase(btmp); \
			} \
		HOLD_EXEC(tid); \
		return true; \
	}

CXXHMS_BENCH(cxx_umap_s16, "%016i")
CXXHMS_BENCH(cxx_umap_s64, "%064i")

template <class T>
bool cxx_uset_i(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base) && !TIdTest(tid, TId_Read10Times) &&
		  !TIdTest(tid, TId_DeleteOneByOne), false);
	std::unordered_set <T> m;
	for (size_t i = 0; i < count; i++)
		m.insert((T)i);
	for (size_t j = 0; j < TId2Count(tid); j++)
		for (size_t i = 0; i < count; i++)
			(void)m.count((T)i);
	if (TIdTest(tid, TId_DeleteOneByOne))
		for (size_t i = 0; i < count; i++)
			m.erase((T)i);
	HOLD_EXEC(tid);
	return true;
}

#define cxx_uset_i32 cxx_uset_i<int32_t>
#define cxx_uset_u32 cxx_uset_i<uint32_t>
#define cxx_uset_i64 cxx_uset_i<int64_t>
#define cxx_uset_f cxx_uset_i<float>
#define cxx_uset_d cxx_uset_i<double>

#define CXXHSS_BENCH(FN, FMT)	\
	bool FN(size_t count, int tid) { \
		RETURN_IF(!TIdTest(tid, TId_Base) && \
			  !TIdTest(tid, TId_Read10Times) && \
			  !TIdTest(tid, TId_DeleteOneByOne), false); \
		char btmp[512]; \
		std::unordered_set <std::string> m; \
		for (size_t i = 0; i < count; i++) { \
			sprintf(btmp, FMT, (int)i); \
			m.insert(btmp); \
		} \
		for (size_t j = 0; j < TId2Count(tid); j++) \
			for (size_t i = 0; i < count; i++) { \
				sprintf(btmp, FMT, (int)i); \
				(void)m.count(btmp); \
			} \
		if (TIdTest(tid, TId_DeleteOneByOne)) \
			for (size_t i = 0; i < count; i++) { \
				sprintf(btmp, FMT, (int)i); \
				m.erase(btmp); \
			} \
		HOLD_EXEC(tid); \
		return true; \
	}

CXXHSS_BENCH(cxx_uset_s16, "%016i")
CXXHSS_BENCH(cxx_uset_s64, "%064i")

#endif // #ifdef S_BENCH_CPP_HM

#define LIBSRTS_BENCH(FN, TID, TK, INSF, COUNTF, DELF)	\
	bool FN(size_t count, int tid) { \
		RETURN_IF(!TIdTest(tid, TId_Base) && \
			  !TIdTest(tid, TId_Read10Times) && \
			  !TIdTest(tid, TId_DeleteOneByOne), false); \
		srt_set *m = sms_alloc(TID, 0); \
		for (size_t i = 0; i < count; i++) \
			INSF(&m, (TK)i); \
		for (size_t j = 0; j < TId2Count(tid); j++) \
			for (size_t i = 0; i < count; i++) \
				(void)COUNTF(m, (TK)i); \
		if (TIdTest(tid, TId_DeleteOneByOne)) \
			for (size_t i = 0; i < count; i++) \
				DELF(m, (TK)i); \
		HOLD_EXEC(tid); \
		sms_free(&m); \
		return true; \
	}

LIBSRTS_BENCH(libsrt_set_i32, SMS_I32, int32_t, sms_insert_i32,
		sms_count_i32, sms_delete_i)
LIBSRTS_BENCH(libsrt_set_u32, SMS_U32, uint32_t, sms_insert_u32,
		sms_count_u32, sms_delete_i)
LIBSRTS_BENCH(libsrt_set_i64, SMS_I, int64_t, sms_insert_i,
		sms_count_i, sms_delete_i)
LIBSRTS_BENCH(libsrt_set_f, SMS_F, float, sms_insert_f,
		sms_count_f, sms_delete_f)
LIBSRTS_BENCH(libsrt_set_d, SMS_D, double, sms_insert_d,
		sms_count_d, sms_delete_d)

template <class T>
bool cxx_set_i(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base) && !TIdTest(tid, TId_Read10Times) &&
		  !TIdTest(tid, TId_DeleteOneByOne), false);
	std::set <T> m;
	for (size_t i = 0; i < count; i++)
		m.insert((T)i);
	for (size_t j = 0; j < TId2Count(tid); j++)
		for (size_t i = 0; i < count; i++)
			(void)m.count((T)i);
	if (TIdTest(tid, TId_DeleteOneByOne))
		for (size_t i = 0; i < count; i++)
			m.erase((T)i);
	HOLD_EXEC(tid);
	return true;
}

#define cxx_set_i32 cxx_set_i<int32_t>
#define cxx_set_u32 cxx_set_i<uint32_t>
#define cxx_set_i64 cxx_set_i<int64_t>
#define cxx_set_f cxx_set_i<float>
#define cxx_set_d cxx_set_i<double>

#define LIBSRTSS_BENCH(FN, FMT)	\
	bool FN(size_t count, int tid) { \
		RETURN_IF(!TIdTest(tid, TId_Base) && \
			  !TIdTest(tid, TId_Read10Times) && \
			  !TIdTest(tid, TId_DeleteOneByOne), false); \
		srt_string *btmp = ss_alloca(512); \
		srt_set *m = sms_alloc(SMS_S, 0); \
		for (size_t i = 0; i < count; i++) { \
			ss_printf(&btmp, 512, FMT, (int)i); \
			sms_insert_s(&m, btmp); \
		} \
		for (size_t j = 0; j < TId2Count(tid); j++) \
			for (size_t i = 0; i < count; i++) { \
				ss_printf(&btmp, 512, FMT, (int)i); \
				(void)sms_count_s(m, btmp); \
			} \
		if (TIdTest(tid, TId_DeleteOneByOne)) \
			for (size_t i = 0; i < count; i++) { \
				ss_printf(&btmp, 512, FMT, (int)i); \
				sms_delete_s(m, btmp); \
			} \
		HOLD_EXEC(tid); \
		sms_free(&m); \
		return true; \
	}

LIBSRTSS_BENCH(libsrt_set_s16, "%016i")
LIBSRTSS_BENCH(libsrt_set_s64, "%064i")

#define CXXS_BENCH(FN, FMT)	\
	bool FN(size_t count, int tid) { \
		RETURN_IF(!TIdTest(tid, TId_Base) && \
			  !TIdTest(tid, TId_Read10Times) && \
			  !TIdTest(tid, TId_DeleteOneByOne), false); \
		char btmp[512]; \
		std::set <std::string> m; \
		for (size_t i = 0; i < count; i++) { \
			sprintf(btmp, FMT, (int)i); \
			m.insert(btmp); \
		} \
		for (size_t j = 0; j < TId2Count(tid); j++) \
			for (size_t i = 0; i < count; i++) { \
				sprintf(btmp, FMT, (int)i); \
				(void)m.count(btmp); \
			} \
		if (TIdTest(tid, TId_DeleteOneByOne)) \
			for (size_t i = 0; i < count; i++) { \
				sprintf(btmp, FMT, (int)i); \
				m.erase(btmp); \
			} \
		HOLD_EXEC(tid); \
		return true; \
	}

CXXS_BENCH(cxx_set_s16, "%016i")
CXXS_BENCH(cxx_set_s64, "%064i")

#define LIBSRTHS_BENCH(FN, TID, TK, INSF, COUNTF, DELF)	\
	bool FN(size_t count, int tid) { \
		RETURN_IF(!TIdTest(tid, TId_Base) && \
			  !TIdTest(tid, TId_Read10Times) && \
			  !TIdTest(tid, TId_DeleteOneByOne), false); \
		srt_hset *m = shs_alloc(TID, 0); \
		for (size_t i = 0; i < count; i++) \
			INSF(&m, (TK)i); \
		for (size_t j = 0; j < TId2Count(tid); j++) \
			for (size_t i = 0; i < count; i++) \
				(void)COUNTF(m, (TK)i); \
		if (TIdTest(tid, TId_DeleteOneByOne)) \
			for (size_t i = 0; i < count; i++) \
				DELF(m, (TK)i); \
		HOLD_EXEC(tid); \
		shs_free(&m); \
		return true; \
	}

LIBSRTHS_BENCH(libsrt_hset_i32, SHS_I32, int32_t, shs_insert_i32, shs_count_i32,
		shs_delete_i32)
LIBSRTHS_BENCH(libsrt_hset_u32, SHS_U32, uint32_t, shs_insert_u32,
		shs_count_u32, shs_delete_u32)
LIBSRTHS_BENCH(libsrt_hset_i64, SHS_I, int64_t, shs_insert_i, shs_count_i,
		shs_delete_i)
LIBSRTHS_BENCH(libsrt_hset_f, SHS_F, float, shs_insert_f, shs_count_f,
		shs_delete_f)
LIBSRTHS_BENCH(libsrt_hset_d, SHS_D, double, shs_insert_d, shs_count_d,
		shs_delete_d)

#define LIBSRTHSS_BENCH(FN, FMT)	\
	bool FN(size_t count, int tid) { \
		RETURN_IF(!TIdTest(tid, TId_Base) && \
			  !TIdTest(tid, TId_Read10Times) && \
			  !TIdTest(tid, TId_DeleteOneByOne), false); \
		srt_string *btmp = ss_alloca(512); \
		srt_hset *m = shs_alloc(SHS_S, 0); \
		for (size_t i = 0; i < count; i++) { \
			ss_printf(&btmp, 512, FMT, (int)i); \
			shs_insert_s(&m, btmp); \
		} \
		for (size_t j = 0; j < TId2Count(tid); j++) \
			for (size_t i = 0; i < count; i++) { \
				ss_printf(&btmp, 512, FMT, (int)i); \
				(void)shs_count_s(m, btmp); \
			} \
		if (TIdTest(tid, TId_DeleteOneByOne)) \
			for (size_t i = 0; i < count; i++) { \
				ss_printf(&btmp, 512, FMT, (int)i); \
				shs_delete_s(m, btmp); \
			} \
		HOLD_EXEC(tid); \
		shs_free(&m); \
		return true; \
	}

LIBSRTHSS_BENCH(libsrt_hset_s16, "%016i")
LIBSRTHSS_BENCH(libsrt_hset_s64, "%064i")

#define LIBSRTV_BENCH(FN, t, PUSH, POP, AT)	\
	bool FN(size_t count, int tid) {	\
	RETURN_IF(!TIdTest(tid, TId_Base) &&			\
		  !TIdTest(tid, TId_Read10Times) &&		\
		  !TIdTest(tid, TId_DeleteOneByOne) &&		\
		  !TIdTest(tid, TId_Sort10Times) &&		\
		  !TIdTest(tid, TId_Sort10000Times), false);	\
	srt_vector *v = sv_alloc_t(t, 0);			\
	if (TIdTest(tid, TId_ReverseSort))			\
		for (size_t i = 0; i < count; i++)		\
			PUSH(&v, (int32_t)count - 1 - (int32_t)i);	\
	else								\
		for (size_t i = 0; i < count; i++)			\
			PUSH(&v, (int32_t)i);				\
	for (size_t j = 0; j < TId2Count(tid); j++)			\
		for (size_t i = 0; i < count; i++)			\
			(void)AT(v, i);					\
	if (TIdTest(tid, TId_DeleteOneByOne))				\
		for (size_t i = 0; i < count; i++)			\
			(void)POP(v);					\
	if (TIdTest(tid, TId_Sort10Times) ||				\
	    TIdTest(tid, TId_Sort10000Times)) {				\
		size_t cnt = TIdTest(tid, TId_Sort10Times) ? 10 : 10000;\
		for (size_t i = 0; i < cnt; i++)			\
			sv_sort(v);					\
	}								\
	HOLD_EXEC(tid);							\
	sv_free(&v);							\
	return true;							\
}

LIBSRTV_BENCH(libsrt_vector_i8, SV_I8, sv_push_i8, sv_pop_i8, sv_at_i8)
LIBSRTV_BENCH(libsrt_vector_u8, SV_U8, sv_push_u8, sv_pop_u8, sv_at_u8)
LIBSRTV_BENCH(libsrt_vector_i16, SV_I16, sv_push_i16, sv_pop_i16, sv_at_i16)
LIBSRTV_BENCH(libsrt_vector_u16, SV_U16, sv_push_u16, sv_pop_u16, sv_at_u16)
LIBSRTV_BENCH(libsrt_vector_i32, SV_I32, sv_push_i32, sv_pop_i32, sv_at_i32)
LIBSRTV_BENCH(libsrt_vector_u32, SV_U32, sv_push_u32, sv_pop_u32, sv_at_u32)
LIBSRTV_BENCH(libsrt_vector_i64, SV_I64, sv_push_i64, sv_pop_i64, sv_at_i64)
LIBSRTV_BENCH(libsrt_vector_u64, SV_U64, sv_push_u64, sv_pop_u64, sv_at_u64)
LIBSRTV_BENCH(libsrt_vector_f, SV_F, sv_push_f, sv_pop_f, sv_at_f)
LIBSRTV_BENCH(libsrt_vector_d, SV_D, sv_push_d, sv_pop_d, sv_at_d)

template <typename T>
bool cxx_vector(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base) && !TIdTest(tid, TId_Read10Times) &&
		  !TIdTest(tid, TId_DeleteOneByOne) &&
		  !TIdTest(tid, TId_Sort10Times) &&
		  !TIdTest(tid, TId_Sort10000Times), false);
	std::vector <T> v;
	if (TIdTest(tid, TId_ReverseSort))
		for (size_t i = 0; i < count; i++)
			v.push_back((T)count - 1 - (T)i);
	else
		for (size_t i = 0; i < count; i++)
			v.push_back((T)i);
	for (size_t j = 0; j < TId2Count(tid); j++)
		for (size_t i = 0; i < count; i++)
			(void)v.at(i);
	if (TIdTest(tid, TId_DeleteOneByOne))
		for (size_t i = 0; i < count; i++)
			(void)v.pop_back();
	if (TIdTest(tid, TId_Sort10Times) || TIdTest(tid, TId_Sort10000Times)) {
		size_t cnt = TIdTest(tid, TId_Sort10Times) ? 10 : 10000;
		for (size_t i = 0; i < cnt; i++)
			std::sort(v.begin(), v.end());
	}
	HOLD_EXEC(tid);
	return true;
}

#define cxx_vector_i8 cxx_vector<int8_t>
#define cxx_vector_u8 cxx_vector<uint8_t>
#define cxx_vector_i16 cxx_vector<int16_t>
#define cxx_vector_u16 cxx_vector<uint16_t>
#define cxx_vector_i32 cxx_vector<int32_t>
#define cxx_vector_u32 cxx_vector<uint32_t>
#define cxx_vector_i64 cxx_vector<int64_t>
#define cxx_vector_u64 cxx_vector<uint64_t>
#define cxx_vector_f cxx_vector<float>
#define cxx_vector_d cxx_vector<double>

struct StrGenTest
{
	uint8_t raw[32];
};

int sv_cmp_StrGenTest(const void *a, const void *b)
{
	return memcmp(a, b, 32);
}

struct StrGenTestCpp
{
	uint8_t raw[32];
	bool operator < (const StrGenTestCpp &against) const {
		return memcmp(raw, against.raw, sizeof(raw)) < 0;
	}
};

bool libsrt_vector_gen(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base) && !TIdTest(tid, TId_Read10Times) &&
		  !TIdTest(tid, TId_DeleteOneByOne) &&
		  !TIdTest(tid, TId_Sort10Times) &&
		  !TIdTest(tid, TId_Sort10000Times), false);
	struct StrGenTest aux;
	memset(&aux, 0, sizeof(aux));
	srt_vector *v = sv_alloc(sizeof(struct StrGenTest), 0,
				 sv_cmp_StrGenTest);
	for (size_t i = 0; i < count; i++)
		sv_push(&v, &aux);
	for (size_t j = 0; j < TId2Count(tid); j++)
		for (size_t i = 0; i < count; i++)
			(void)sv_at(v, i);
	if (TIdTest(tid, TId_DeleteOneByOne))
		for (size_t i = 0; i < count; i++)
			(void)sv_pop(v);
	if (TIdTest(tid, TId_Sort10Times) || TIdTest(tid, TId_Sort10000Times)) {
		size_t cnt = TIdTest(tid, TId_Sort10Times) ? 10 : 10000;
		for (size_t i = 0; i < cnt; i++)
			sv_sort(v);
	}
	HOLD_EXEC(tid);
	sv_free(&v);
	return true;
}

bool cxx_vector_gen(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base) && !TIdTest(tid, TId_Read10Times) &&
		  !TIdTest(tid, TId_DeleteOneByOne) &&
		  !TIdTest(tid, TId_Sort10Times) &&
		  !TIdTest(tid, TId_Sort10000Times), false);
	struct StrGenTestCpp aux;
	memset(&aux, 0, sizeof(aux));
	std::vector <struct StrGenTestCpp> v;
	for (size_t i = 0; i < count; i++)
		v.push_back(aux);
	for (size_t j = 0; j < TId2Count(tid); j++)
		for (size_t i = 0; i < count; i++)
			(void)v.at(i);
	if (TIdTest(tid, TId_DeleteOneByOne))
		for (size_t i = 0; i < count; i++)
			(void)v.pop_back();
	if (TIdTest(tid, TId_Sort10Times) || TIdTest(tid, TId_Sort10000Times)) {
		size_t cnt = TIdTest(tid, TId_Sort10Times) ? 10 : 10000;
		for (size_t i = 0; i < cnt; i++)
			std::sort(v.begin(), v.end());
	}
	HOLD_EXEC(tid);
	return true;
}

const char
	*haystack_easymatch1_long =
	"Alice was beginning to get very tired of sitting by her sister on the"
	" bank, and of having nothing to do. Once or twice she had peeped into"
	" the book her sister was reading, but it had no pictures or conversat"
	"ions in it, \"and what is the use of a book,\" thought Alice, \"witho"
	"ut pictures or conversations?\"",
	*haystack_easymatch2_long =
	U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1
	U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1
	U8_HAN_611B U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1 U8_MIX1,
	*haystack_hardmatch1_long =
	"111111x11111131111111111111111111111111111111111111111111111111111111"
	"111111111111111111111111411111111111111111111111111111111111111111111"
	"111111111111111111111111111111111111111111111111111111111111111111111"
	"12k1",
	*haystack_hardmatch1_short = "11111111111111111112k1",
	*haystack_hardmatch2_long =
	"abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcda"
	"bcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdab"
	"cdabcdabcdabcdabcdabcdabcdabcddcbadcbadcbadcba",
	*haystack_hardmatch3_long =
	U8_MANY_UNDERSCORES U8_MANY_UNDERSCORES U8_MANY_UNDERSCORES
								"1234567890",
	*needle_easymatch1a = " a ",
	*needle_easymatch1b = "conversations?",
	*needle_easymatch2a = U8_HAN_611B,
	*needle_easymatch2b = U8_MIX1 U8_HAN_611B,
	*needle_hardmatch1a = "1111111112k1",
	*needle_hardmatch1b = "112k1",
	*needle_hardmatch2 = "dcba",
	*needle_hardmatch3 = U8_MANY_UNDERSCORES "123";

bool libsrt_string_search(const char *haystack, const char *needle,
			  size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
	const srt_string *h = ss_crefa(haystack), *n = ss_crefa(needle);
	for (size_t i = 0; i < count; i++)
		ss_find(h, 0, n);
	return true;
}

bool c_string_search(const char *haystack, const char *needle,
		     size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
	for (size_t i = 0; i < count; i++)
		strstr(haystack, needle) || putchar(0);
	return true;
}

bool cxx_string_search(const char *haystack, const char *needle,
		       size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
	std::string h = haystack, n = needle;
	for (size_t i = 0; i < count; i++)
		h.find(n);
	return true;
}

bool libsrt_string_search_easymatch_long_1a(size_t count, int tid)
{
	return libsrt_string_search(haystack_easymatch1_long,
				    needle_easymatch1a, count, tid);
}

bool libsrt_string_search_easymatch_long_1b(size_t count, int tid)
{
	return libsrt_string_search(haystack_easymatch1_long,
				    needle_easymatch1b, count, tid);
}

bool libsrt_string_search_easymatch_long_2a(size_t count, int tid)
{
	return libsrt_string_search(haystack_easymatch2_long,
				    needle_easymatch2a, count, tid);
}

bool libsrt_string_search_easymatch_long_2b(size_t count, int tid)
{
	return libsrt_string_search(haystack_easymatch2_long,
				    needle_easymatch2b, count, tid);
}

bool libsrt_string_search_hardmatch_long_1a(size_t count, int tid)
{
	return libsrt_string_search(haystack_hardmatch1_long,
				    needle_hardmatch1a, count, tid);
}

bool libsrt_string_search_hardmatch_long_1b(size_t count, int tid)
{
	return libsrt_string_search(haystack_hardmatch1_long,
				    needle_hardmatch1b, count, tid);
}

bool libsrt_string_search_hardmatch_short_1a(size_t count, int tid)
{
	return libsrt_string_search(haystack_hardmatch1_short,
				    needle_hardmatch1a, count, tid);
}

bool libsrt_string_search_hardmatch_short_1b(size_t count, int tid)
{
	return libsrt_string_search(haystack_hardmatch1_short,
				    needle_hardmatch1b, count, tid);
}

bool libsrt_string_search_hardmatch_long_2(size_t count, int tid)
{
	return libsrt_string_search(haystack_hardmatch2_long, needle_hardmatch2,
				    count, tid);
}

bool libsrt_string_search_hardmatch_long_3(size_t count, int tid)
{
	return libsrt_string_search(haystack_hardmatch3_long, needle_hardmatch3,
				    count, tid);
}

bool c_string_search_easymatch_long_1a(size_t count, int tid)
{
	return c_string_search(haystack_easymatch1_long, needle_easymatch1a,
			       count, tid);
}

bool c_string_search_easymatch_long_1b(size_t count, int tid)
{
	return c_string_search(haystack_easymatch1_long, needle_easymatch1b,
			       count, tid);
}

bool c_string_search_easymatch_long_2a(size_t count, int tid)
{
	return c_string_search(haystack_easymatch2_long, needle_easymatch2a,
			       count, tid);
}

bool c_string_search_easymatch_long_2b(size_t count, int tid)
{
	return c_string_search(haystack_easymatch2_long, needle_easymatch2b,
			       count, tid);
}

bool c_string_search_hardmatch_long_1a(size_t count, int tid)
{
	return c_string_search(haystack_hardmatch1_long, needle_hardmatch1a,
			       count, tid);
}

bool c_string_search_hardmatch_long_1b(size_t count, int tid)
{
	return c_string_search(haystack_hardmatch1_long, needle_hardmatch1b,
			       count, tid);
}

bool c_string_search_hardmatch_short_1a(size_t count, int tid)
{
	return c_string_search(haystack_hardmatch1_short, needle_hardmatch1a,
			       count, tid);
}

bool c_string_search_hardmatch_short_1b(size_t count, int tid)
{
	return c_string_search(haystack_hardmatch1_short, needle_hardmatch1b,
			       count, tid);
}

bool c_string_search_hardmatch_long_2(size_t count, int tid)
{
	return c_string_search(haystack_hardmatch2_long, needle_hardmatch2,
			       count, tid);
}

bool c_string_search_hardmatch_long_3(size_t count, int tid)
{
	return c_string_search(haystack_hardmatch3_long, needle_hardmatch3,
			       count, tid);
}

bool cxx_string_search_easymatch_long_1a(size_t count, int tid)
{
	return cxx_string_search(haystack_easymatch1_long, needle_easymatch1a,
				 count, tid);
}

bool cxx_string_search_easymatch_long_1b(size_t count, int tid)
{
	return cxx_string_search(haystack_easymatch1_long, needle_easymatch1b,
				 count, tid);
}

bool cxx_string_search_easymatch_long_2a(size_t count, int tid)
{
	return cxx_string_search(haystack_easymatch2_long, needle_easymatch2a,
				 count, tid);
}

bool cxx_string_search_easymatch_long_2b(size_t count, int tid)
{
	return cxx_string_search(haystack_easymatch2_long, needle_easymatch2b,
				 count, tid);
}

bool cxx_string_search_hardmatch_long_1a(size_t count, int tid)
{
	return cxx_string_search(haystack_hardmatch1_long, needle_hardmatch1a,
				 count, tid);
}

bool cxx_string_search_hardmatch_long_1b(size_t count, int tid)
{
	return cxx_string_search(haystack_hardmatch1_long, needle_hardmatch1b,
				 count, tid);
}

bool cxx_string_search_hardmatch_short_1a(size_t count, int tid)
{
	return cxx_string_search(haystack_hardmatch1_short, needle_hardmatch1a,
				 count, tid);
}

bool cxx_string_search_hardmatch_short_1b(size_t count, int tid)
{
	return cxx_string_search(haystack_hardmatch1_short, needle_hardmatch1b,
				 count, tid);
}

bool cxx_string_search_hardmatch_long_2(size_t count, int tid)
{
	return cxx_string_search(haystack_hardmatch2_long, needle_hardmatch2,
				 count, tid);
}

bool cxx_string_search_hardmatch_long_3(size_t count, int tid)
{
	return cxx_string_search(haystack_hardmatch3_long, needle_hardmatch3,
				 count, tid);
}

const char
	case_test_ascii_str[95 + 1] =
	" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
	"abcdefghijklmnopqrstuvwxyz{|}~",
	case_test_utf8_str[95 + 1] =
	U8_MIX_28_bytes U8_MIX_28_bytes U8_MIX_28_bytes "12345678901";

bool libsrt_string_loweruppercase(const char *in, size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
	srt_string *s = ss_alloca(95);
	ss_cpy(&s, ss_crefa(in));
	for (size_t i = 0; i < count; i++) {
		ss_tolower(&s);
		ss_toupper(&s);
	}
	return true;
}

void cstolower(char *s)
{
	for (; *s; s++)
		*s = tolower(*s);
}

void cstoupper(char *s)
{
	for (; *s; s++)
		*s = toupper(*s);
}

bool c_string_loweruppercase(const char *in, size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
	char s[95 + 1];
	strncpy(s, in, 95);
	s[95] = 0;
	for (size_t i = 0; i < count; i++) {
		cstolower(s);
		cstoupper(s);
		if (s[0] == s[1])
			putchar(0);
	}
	return true;
}

void cxxtolower(std::string &s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

void cxxtoupper(std::string &s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

bool cxx_string_loweruppercase(const char *in, size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
	std::string s = in;
	for (size_t i = 0; i < count; i++) {
		cxxtolower(s);
		cxxtoupper(s);
	}
	return true;
}

bool libsrt_string_loweruppercase_ascii(size_t count, int tid)
{
	return libsrt_string_loweruppercase(case_test_ascii_str, count, tid);
}

bool libsrt_string_loweruppercase_utf8(size_t count, int tid)
{
	return libsrt_string_loweruppercase(case_test_utf8_str, count, tid);
}

bool c_string_loweruppercase_ascii(size_t count, int tid)
{
	return c_string_loweruppercase(case_test_ascii_str, count, tid);
}

bool c_string_loweruppercase_utf8(size_t count, int tid)
{
	return c_string_loweruppercase(case_test_utf8_str, count, tid);
}

bool cxx_string_loweruppercase_ascii(size_t count, int tid)
{
	return cxx_string_loweruppercase(case_test_ascii_str, count, tid);
}

bool cxx_string_loweruppercase_utf8(size_t count, int tid)
{
	return cxx_string_loweruppercase(case_test_utf8_str, count, tid);
}

const char *cat_test[7] = {
	"In a village of La Mancha, the name of which I have no desire to call "
	"to mind, there lived not long since one of those gentlemen that keep a"
	" lance in the lance-rack, an old buckler, a lean hack, and a greyhound"
	" for coursing. ",
	"An olla of rather more beef than mutton, a salad on most nights, scrap"
	"s on Saturdays, lentils on Fridays, and a pigeon or so extra on Sunday"
	"s, made away with three-quarters of his income. ",
	"The rest of it went in a doublet of fine cloth and velvet breeches and"
	" shoes to match for holidays, while on week-days he made a brave figur"
	"e in his best homespun.i ",
	"He had in his house a housekeeper past forty, a niece under twenty, an"
	"d a lad for the field and market-place, who used to saddle the hack as"
	" well as handle the bill-hook. ",
	"The age of this gentleman of ours was bordering on fifty; he was of a "
	"hardy habit, spare, gaunt-featured, a very early riser and a great spo"
	"rtsman. ",
	"They will have it his surname was Quixada or Quesada (for here there i"
	"s some difference of opinion among the authors who write on the subjec"
	"t), although from reasonable conjectures it seems plain that he was ca"
	"lled Quexana. ",
	"This, however, is of but little importance to our tale; it will be eno"
	"ugh not to stray a hair's breadth from the truth in the telling of it."
	};
const size_t cat_test_ops = sizeof(cat_test) / sizeof(cat_test[0]);

bool libsrt_string_cat(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
	srt_string *s[cat_test_ops], *out = NULL;
	for (size_t i = 0; i < cat_test_ops; i++)
		s[i] = ss_dup_c(cat_test[i]);
	for (size_t i = 0; i < count; i++)
		ss_cat(&out, s[0], s[1], s[2], s[3], s[4], s[5], s[6]);
	for (size_t i = 0; i < cat_test_ops; i++)
		ss_free(&s[i]);
	ss_free(&out);
	return true;
}

bool c_string_cat(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
	char out[32 * 1024]; /* yes, not safe, just for the benchmark */
	for (size_t i = 0; i < count; i++) {
		out[0] = 0;
		for (size_t k = 0; k < cat_test_ops; k++)
			strcat(out, cat_test[k]);
	}
	return true;
}

bool cxx_string_cat(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
        std::string s[cat_test_ops], out;
	for (size_t i = 0; i < cat_test_ops; i++)
		s[i] = cat_test[i];
	for (size_t i = 0; i < count; i++)
		out = s[0] + s[1] + s[2] + s[3] + s[4] + s[5] + s[6];
	return true;
}

bool cxx_stringstream_cat(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
        std::string s[cat_test_ops], out;
	std::stringstream ss;
	for (size_t i = 0; i < cat_test_ops; i++)
		s[i] = cat_test[i];
	for (size_t i = 0; i < count; i++) {
		ss.clear();
		ss.str("");
		ss << s[0] << s[1] << s[2] << s[3] << s[4] << s[5] << s[6];
		out = ss.str();
	}
	return true;
}

bool libsrt_bitset(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
	srt_bitset *b = sb_alloc(count);
	for (size_t i = 0; i < count; i++)
		sb_set(&b, i);
	for (size_t j = 0; j < TId2Count(tid); j++)
		for (size_t i = 0; i < count; i++)
			sb_test(b, i) || putchar(0);
	sb_free(&b);
	HOLD_EXEC(tid);
	return true;
}

bool cxx_bitset(size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
	std::bitset <S_TEST_ELEMS/*count*/> b; // Compile-time size required
	for (size_t i = 0; i < count; i++)
		b[i] = 1;
	for (size_t j = 0; j < TId2Count(tid); j++)
		for (size_t i = 0; i < count; i++)
			b[i] || putchar(0);
	HOLD_EXEC(tid);
	return true;
}

bool libsrt_bitset_popcountN(size_t pc_count, size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
	srt_bitset *b = sb_alloc(count);
	for (size_t i = 0; i < count; i++)
		sb_set(&b, i);
	for (size_t j = 0; j < pc_count; j++)
		sb_popcount(b) || putchar(0);
	HOLD_EXEC(tid);
	sb_free(&b);
	return true;
}

bool cxx_bitset_popcountN(size_t pc_count, size_t count, int tid)
{
	RETURN_IF(!TIdTest(tid, TId_Base), false);
	std::bitset <S_TEST_ELEMS/*count*/> b; // Compile-time size required
	for (size_t i = 0; i < count; i++)
		b[i] = 1;
	for (size_t j = 0; j < pc_count; j++)
		b.count() || putchar(0);
	HOLD_EXEC(tid);
	return true;
}

bool libsrt_bitset_popcount100(size_t count, int tid)
{
	return libsrt_bitset_popcountN(100, count, tid);
}

bool cxx_bitset_popcount100(size_t count, int tid)
{
	return cxx_bitset_popcountN(100, count, tid);
}

bool libsrt_bitset_popcount10000(size_t count, int tid)
{
	return libsrt_bitset_popcountN(10000, count, tid);
}

bool cxx_bitset_popcount10000(size_t count, int tid)
{
	return cxx_bitset_popcountN(10000, count, tid);
}

int main(int argc, char *argv[])
{
	BENCH_INIT;
	const size_t ntests = 6,
		     count[ntests] = { S_TEST_ELEMS,
				       S_TEST_ELEMS,
				       S_TEST_ELEMS,
				       S_TEST_ELEMS,
				       S_TEST_ELEMS,
				       S_TEST_ELEMS_SHORT };
	int tid[ntests] = { TId_Base,
			    TId_Read10Times,
			    TId_DeleteOneByOne,
			    TId_Sort10Times,
			    TId_Sort10Times | TId_ReverseSort,
			    TId_Sort10000Times };
	char label[ntests][512];
	snprintf(label[0], 512,
		 "Insert or process " FMT_ZU " elements, cleanup", count[0]);
	snprintf(label[1], 512,
		 "Insert " FMT_ZU " elements, read all elements %i "
		 "times, cleanup", count[1], TId2Count(tid[1]));
	snprintf(label[2], 512,
		 "Insert or process " FMT_ZU " elements, delete all "
		 "elements one by one, cleanup", count[2]);
	snprintf(label[3], 512,
		 "Insert or process " FMT_ZU " elements, sort 10 times,"
		 " cleanup", count[3]);
	snprintf(label[4], 512,
		 "Insert or process " FMT_ZU " elements (reverse order)"
		 ", sort 10 times, cleanup", count[4]);
	snprintf(label[5], 512,
		 "Insert or process " FMT_ZU " elements, sort 10000 "
		 "times, cleanup", count[5]);
	for (size_t i = 0; i < ntests; i++) {
		printf("\n%s\n| Test | Insert count | Memory (MiB) | Execution "
		       "time (s) |\n|:---:|:---:|:---:|:---:|\n", label[i]);
		BENCH_FN(libsrt_map_ii32, count[i], tid[i]);
		BENCH_FN(cxx_map_ii32, count[i], tid[i]);
		BENCH_FN(libsrt_hmap_ii32, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_umap_ii32, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_map_uu32, count[i], tid[i]);
		BENCH_FN(cxx_map_uu32, count[i], tid[i]);
		BENCH_FN(libsrt_hmap_uu32, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_umap_uu32, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_map_ii64, count[i], tid[i]);
		BENCH_FN(cxx_map_ii64, count[i], tid[i]);
		BENCH_FN(libsrt_hmap_ii64, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_umap_ii64, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_map_ff, count[i], tid[i]);
		BENCH_FN(cxx_map_ff, count[i], tid[i]);
		BENCH_FN(libsrt_hmap_ff, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_umap_ff, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_map_dd, count[i], tid[i]);
		BENCH_FN(cxx_map_dd, count[i], tid[i]);
		BENCH_FN(libsrt_hmap_dd, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_umap_dd, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_map_s16, count[i], tid[i]);
		BENCH_FN(cxx_map_s16, count[i], tid[i]);
		BENCH_FN(libsrt_hmap_s16, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_umap_s16, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_map_s64, count[i], tid[i]);
		BENCH_FN(cxx_map_s64, count[i], tid[i]);
		BENCH_FN(libsrt_hmap_s64, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_umap_s64, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_set_i32, count[i], tid[i]);
		BENCH_FN(cxx_set_i32, count[i], tid[i]);
		BENCH_FN(libsrt_hset_i32, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_uset_i32, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_set_u32, count[i], tid[i]);
		BENCH_FN(cxx_set_u32, count[i], tid[i]);
		BENCH_FN(libsrt_hset_u32, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_uset_u32, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_set_i64, count[i], tid[i]);
		BENCH_FN(cxx_set_i64, count[i], tid[i]);
		BENCH_FN(libsrt_hset_i64, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_uset_i64, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_set_f, count[i], tid[i]);
		BENCH_FN(cxx_set_f, count[i], tid[i]);
		BENCH_FN(libsrt_hset_f, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_uset_f, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_set_d, count[i], tid[i]);
		BENCH_FN(cxx_set_d, count[i], tid[i]);
		BENCH_FN(libsrt_hset_d, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_uset_d, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_set_s16, count[i], tid[i]);
		BENCH_FN(cxx_set_s16, count[i], tid[i]);
		BENCH_FN(libsrt_hset_s16, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_uset_s16, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_set_s64, count[i], tid[i]);
		BENCH_FN(cxx_set_s64, count[i], tid[i]);
		BENCH_FN(libsrt_hset_s64, count[i], tid[i]);
#ifdef S_BENCH_CPP_HM
		BENCH_FN(cxx_uset_s64, count[i], tid[i]);
#endif
		BENCH_FN(libsrt_vector_i8, count[i], tid[i]);
		BENCH_FN(cxx_vector_i8, count[i], tid[i]);
		BENCH_FN(libsrt_vector_u8, count[i], tid[i]);
		BENCH_FN(cxx_vector_u8, count[i], tid[i]);
		BENCH_FN(libsrt_vector_i16, count[i], tid[i]);
		BENCH_FN(cxx_vector_i16, count[i], tid[i]);
		BENCH_FN(libsrt_vector_u16, count[i], tid[i]);
		BENCH_FN(cxx_vector_u16, count[i], tid[i]);
		BENCH_FN(libsrt_vector_i32, count[i], tid[i]);
		BENCH_FN(cxx_vector_i32, count[i], tid[i]);
		BENCH_FN(libsrt_vector_u32, count[i], tid[i]);
		BENCH_FN(cxx_vector_u32, count[i], tid[i]);
		BENCH_FN(libsrt_vector_i64, count[i], tid[i]);
		BENCH_FN(cxx_vector_i64, count[i], tid[i]);
		BENCH_FN(libsrt_vector_u64, count[i], tid[i]);
		BENCH_FN(cxx_vector_u64, count[i], tid[i]);
		BENCH_FN(libsrt_vector_f, count[i], tid[i]);
		BENCH_FN(cxx_vector_f, count[i], tid[i]);
		BENCH_FN(libsrt_vector_d, count[i], tid[i]);
		BENCH_FN(cxx_vector_d, count[i], tid[i]);
		BENCH_FN(libsrt_vector_gen, count[i], tid[i]);
		BENCH_FN(cxx_vector_gen, count[i], tid[i]);
		BENCH_FN(libsrt_string_search_easymatch_long_1a, count[i],
			 tid[i]);
		BENCH_FN(c_string_search_easymatch_long_1a, count[i], tid[i]);
		BENCH_FN(cxx_string_search_easymatch_long_1a, count[i], tid[i]);
		BENCH_FN(libsrt_string_search_easymatch_long_1b, count[i],
			 tid[i]);
		BENCH_FN(c_string_search_easymatch_long_1b, count[i], tid[i]);
		BENCH_FN(cxx_string_search_easymatch_long_1b, count[i], tid[i]);
		BENCH_FN(libsrt_string_search_easymatch_long_2a, count[i],
			 tid[i]);
		BENCH_FN(c_string_search_easymatch_long_2a, count[i], tid[i]);
		BENCH_FN(cxx_string_search_easymatch_long_2a, count[i], tid[i]);
		BENCH_FN(libsrt_string_search_easymatch_long_2b, count[i],
			 tid[i]);
		BENCH_FN(c_string_search_easymatch_long_2b, count[i], tid[i]);
		BENCH_FN(cxx_string_search_easymatch_long_2b, count[i], tid[i]);
		BENCH_FN(libsrt_string_search_hardmatch_long_1a, count[i],
			 tid[i]);
		BENCH_FN(c_string_search_hardmatch_long_1a, count[i], tid[i]);
		BENCH_FN(cxx_string_search_hardmatch_long_1a, count[i], tid[i]);
		BENCH_FN(libsrt_string_search_hardmatch_long_1b, count[i],
			 tid[i]);
		BENCH_FN(c_string_search_hardmatch_long_1b, count[i], tid[i]);
		BENCH_FN(cxx_string_search_hardmatch_long_1b, count[i], tid[i]);
		BENCH_FN(libsrt_string_search_hardmatch_short_1a, count[i],
			 tid[i]);
		BENCH_FN(c_string_search_hardmatch_short_1a, count[i], tid[i]);
		BENCH_FN(cxx_string_search_hardmatch_short_1a, count[i],
			 tid[i]);
		BENCH_FN(libsrt_string_search_hardmatch_short_1b, count[i],
			 tid[i]);
		BENCH_FN(c_string_search_hardmatch_short_1b, count[i], tid[i]);
		BENCH_FN(cxx_string_search_hardmatch_short_1b, count[i],
			 tid[i]);
		BENCH_FN(libsrt_string_search_hardmatch_long_2, count[i],
			 tid[i]);
		BENCH_FN(c_string_search_hardmatch_long_2, count[i], tid[i]);
		BENCH_FN(cxx_string_search_hardmatch_long_2, count[i], tid[i]);
		BENCH_FN(libsrt_string_search_hardmatch_long_3, count[i],
			 tid[i]);
		BENCH_FN(c_string_search_hardmatch_long_3, count[i], tid[i]);
		BENCH_FN(cxx_string_search_hardmatch_long_3, count[i], tid[i]);
		BENCH_FN(libsrt_string_loweruppercase_ascii, count[i], tid[i]);
		BENCH_FN(c_string_loweruppercase_ascii, count[i], tid[i]);
		BENCH_FN(cxx_string_loweruppercase_ascii, count[i], tid[i]);
		BENCH_FN(libsrt_string_loweruppercase_utf8, count[i], tid[i]);
		BENCH_FN(c_string_loweruppercase_utf8, count[i], tid[i]);
		BENCH_FN(cxx_string_loweruppercase_utf8, count[i], tid[i]);
		BENCH_FN(libsrt_bitset, count[i], tid[i]);
		BENCH_FN(cxx_bitset, count[i], tid[i]);
		BENCH_FN(libsrt_bitset_popcount100, count[i], tid[i]);
		BENCH_FN(cxx_bitset_popcount100, count[i], tid[i]);
		BENCH_FN(libsrt_bitset_popcount10000, count[i], tid[i]);
		BENCH_FN(cxx_bitset_popcount10000, count[i], tid[i]);
		BENCH_FN(libsrt_string_cat, count[i] / 10, tid[i]);
		BENCH_FN(c_string_cat, count[i] / 10, tid[i]);
		BENCH_FN(cxx_string_cat, count[i] / 10, tid[i]);
		BENCH_FN(cxx_stringstream_cat, count[i] / 10, tid[i]);
	}
	return 0;
}


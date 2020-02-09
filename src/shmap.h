#ifndef SHMAP_H
#define SHMAP_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * shmap.h
 *
 * #SHORTDOC hash map handling (key-value storage)
 *
 * #DOC Map functions handle key-value storage, which is implemented as a
 * #DOC hash table (O(n), with O(1) amortized time complexity for
 * #DOC insert/read/delete)
 * #DOC
 * #DOC
 * #DOC Supported key/value modes (enum eSHM_Type):
 * #DOC
 * #DOC
 * #DOC	SHM_II32: int32_t key, int32_t value
 * #DOC
 * #DOC	SHM_UU32: uint32_t key, uint32_t value
 * #DOC
 * #DOC	SHM_II: int64_t key, int64_t value
 * #DOC
 * #DOC	SHM_FF: float (single-precision floating point) key, float value
 * #DOC
 * #DOC	SHM_DD: double (double-precision floating point) key, double value
 * #DOC
 * #DOC	SHM_IS: int64_t key, string value
 * #DOC
 * #DOC	SHM_IP: int64_t key, pointer value
 * #DOC
 * #DOC	SHM_SI: string key, int64_t value
 * #DOC
 * #DOC	SHM_DS: double key, string value
 * #DOC
 * #DOC	SHM_DP: double key, pointer value
 * #DOC
 * #DOC	SHM_SD: string key, double value
 * #DOC
 * #DOC	SHM_SS: string key, string value
 * #DOC
 * #DOC	SHM_SP: string key, pointer value
 * #DOC
 * #DOC
 * #DOC Callback types for the shm_itp_*() functions:
 * #DOC
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_ii32)(int32_t k, int32_t v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_uu32)(uint32_t k, uint32_t v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_ii)(int64_t k, int64_t v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_ff)(float k, float v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_dd)(double k, double v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_is)(int64_t k, const srt_string *, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_ip)(int64_t k, const void *, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_si)(const srt_string *, int64_t v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_ds)(double k, const srt_string *, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_dp)(double k, const void *, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_sd)(const srt_string *, double v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_ss)(const srt_string *, const srt_string *, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_hmap_it_sp)(const srt_string *, const void *, void *context);
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "saux/scommon.h"
#include "saux/sstringo.h"

/*
 * Structures and types
 */

enum eSHM_Type0 {
	SHM0_II32,
	SHM0_UU32,
	SHM0_II,
	SHM0_IS,
	SHM0_IP,
	SHM0_SI,
	SHM0_SS,
	SHM0_SP,
	SHM0_I32,
	SHM0_U32,
	SHM0_I,
	SHM0_S,
	SHM0_FF,
	SHM0_DD,
	SHM0_DS,
	SHM0_DP,
	SHM0_SD,
	SHM0_F,
	SHM0_D,
	SHM0_NumTypes
};

enum eSHM_Type {
	SHM_II32 = SHM0_II32,
	SHM_UU32 = SHM0_UU32,
	SHM_II = SHM0_II,
	SHM_IS = SHM0_IS,
	SHM_IP = SHM0_IP,
	SHM_SI = SHM0_SI,
	SHM_SS = SHM0_SS,
	SHM_SP = SHM0_SP,
	SHM_FF = SHM0_FF,
	SHM_DD = SHM0_DD,
	SHM_DS = SHM0_DS,
	SHM_DP = SHM0_DP,
	SHM_SD = SHM0_SD
};

struct SHMapI {
	int64_t k;
};
struct SHMapS {
	srt_stringo1 k;
};
struct SHMapi {
	int32_t k;
};
struct SHMapu {
	uint32_t k;
};
struct SHMapii {
	struct SHMapi x;
	int32_t v;
};
struct SHMapuu {
	struct SHMapu x;
	uint32_t v;
};
struct SHMapII {
	struct SHMapI x;
	int64_t v;
};
struct SHMapIS {
	struct SHMapI x;
	srt_stringo1 v;
};
struct SHMapIP {
	struct SHMapI x;
	const void *v;
};
struct SHMapSI {
	struct SHMapS x;
	int64_t v;
};
struct SHMapSS {
	srt_stringo kv;
};
struct SHMapSP {
	struct SHMapS x;
	const void *v;
};
struct SHMapF {
	float k;
};
struct SHMapD {
	double k;
};
struct SHMapFF {
	struct SHMapF x;
	float v;
};
struct SHMapDD {
	struct SHMapD x;
	double v;
};
struct SHMapDS {
	struct SHMapD x;
	srt_stringo1 v;
};
struct SHMapDP {
	struct SHMapD x;
	const void *v;
};
struct SHMapSD {
	struct SHMapS x;
	double v;
};

typedef struct S_HMap srt_hmap;

typedef uint32_t shm_eloc_t_; /* element location offset */

struct SHMBucket {
	/*
	 * Location where the bucket associated data is stored
	 */
	shm_eloc_t_ loc;
	/*
	 * Hash of the element (the bucket id would be the N highest bits)
	 */
	uint32_t hash;
	/*
	 * Bucket collision counter
	 * 0: Zero elements associated to the bucket. This means that no
	 *    element with the hash associated to the bucket has been inserted
	 * >= 1: Number of elements associated to the bucket.
	 */
	uint32_t cnt;
};

/*
 * srt_hmap memory layout:
 *
 * | SDataFull | struct fields | struct SHMBucket [N] | elements [M] |
 */

typedef srt_bool (*shm_eq_f)(const void *key, const void *node);
typedef void (*shm_del_f)(void *node);
typedef uint32_t (*shm_hash_f)(const void *node);
typedef const void *(*shm_n2key_f)(const void *node);

struct S_HMap {
	struct SDataFull d;
	uint32_t hbits; /* hash table bits */
	uint32_t hmask; /* hash table bitmask */
	size_t rh_threshold; /* (1 << hbits) * rh_threshold_pct) / 100 */
	size_t rh_threshold_pct;
};

/*
 * Configuration
 */

#define SHM_HASH_32(k)	sh_hash32((uint32_t)(k))
#define SHM_HASH_64(k)	sh_hash64((uint64_t)(k))
#define SHM_HASH_F(k)	sh_hash_f(k)
#define SHM_HASH_D(k)	sh_hash_d(k)

#ifdef S_FORCE_USING_MURMUR3
#define SHM_HASH_S ss_mh3_32
#else
#define SHM_HASH_S ss_fnv1a
#endif

/*
 * Allocation
 */

S_INLINE uint8_t shm_elem_size(int t)
{
	switch (t) {
	case SHM0_II32:
		return sizeof(struct SHMapii);
	case SHM0_UU32:
		return sizeof(struct SHMapuu);
	case SHM0_II:
		return sizeof(struct SHMapII);
	case SHM0_IS:
		return sizeof(struct SHMapIS);
	case SHM0_IP:
		return sizeof(struct SHMapIP);
	case SHM0_SI:
		return sizeof(struct SHMapSI);
	case SHM0_SS:
		return sizeof(struct SHMapSS);
	case SHM0_SP:
		return sizeof(struct SHMapSP);
	case SHM0_I32:
		return sizeof(struct SHMapi);
	case SHM0_U32:
		return sizeof(struct SHMapu);
	case SHM0_I:
		return sizeof(struct SHMapI);
	case SHM0_S:
		return sizeof(struct SHMapS);
	case SHM0_FF:
		return sizeof(struct SHMapFF);
	case SHM0_DD:
		return sizeof(struct SHMapDD);
	case SHM0_DS:
		return sizeof(struct SHMapDS);
	case SHM0_DP:
		return sizeof(struct SHMapDP);
	case SHM0_SD:
		return sizeof(struct SHMapSD);
	case SHM0_F:
		return sizeof(struct SHMapF);
	case SHM0_D:
		return sizeof(struct SHMapD);
	default:
		break;
	}
	return 0;
}

S_INLINE size_t sh_hdr0_size()
{
	size_t as = sizeof(void *);
	return (sizeof(srt_hmap) / as) * as + (sizeof(srt_hmap) % as ? as : 0);
}

S_INLINE size_t sh_hdr_size(int t, uint64_t np2_elems)
{
	size_t h0s = sh_hdr0_size(), hs, es = shm_elem_size(t), hsr;
	uint64_t hs64 = h0s + np2_elems * sizeof(struct SHMBucket);
	hs = (size_t)hs64;
	RETURN_IF((uint64_t)hs != hs64, 0);
	hsr = es ? hs % es : 0;
	return hsr ? hs - hsr + es : hs;
}

#define BUILD_GET_BUCKETS(fn, TMOD)					\
	S_INLINE TMOD struct SHMBucket *fn(TMOD srt_hmap *hm) {		\
		return (TMOD struct SHMBucket *)((TMOD uint8_t *)hm +	\
						sh_hdr0_size());	\
	}

BUILD_GET_BUCKETS(shm_get_buckets,)
BUILD_GET_BUCKETS(shm_get_buckets_r, const)

S_INLINE unsigned shm_s2hb(size_t max_size)
{
	unsigned hbits = slog2_ceil(max_size);
	return hbits ? hbits : 1;
}

/*
#API: |Allocate hash map (stack)|hash map type; initial reserve|hmap|O(n)|1;2|
srt_hmap *shm_alloca(enum eSHM_Type t, size_t n);
*/
#define shm_alloca(type, max_size)					       \
	shm_alloc_raw(type, S_TRUE,					       \
		     s_alloca(sd_alloc_size_raw(			       \
				sh_hdr_size(type, snextpow2(max_size)),	       \
				shm_elem_size(type), max_size, S_FALSE)),      \
		     sh_hdr_size(type, (size_t)snextpow2(max_size)),	       \
		     shm_elem_size(type), max_size, shm_s2hb(max_size))

srt_hmap *shm_alloc_raw(int t, srt_bool ext_buf, void *buffer, size_t hdr_size,
			size_t elem_size, size_t max_size, size_t np2_size);

srt_hmap *shm_alloc_aux(int t, size_t init_size);

/* #api: |allocate hash map (heap)|hash map type; initial reserve|hmap|O(n)|1;2| */
S_INLINE srt_hmap *shm_alloc(enum eSHM_Type t, size_t init_size)
{
	return shm_alloc_aux((int)t, init_size);
}

SD_BUILDFUNCS_FULL_ST(shm, srt_hmap, 0)

/*
#API: |Ensure space for extra elements|hash map;number of extra elements|extra size allocated|O(1)|1;2|
size_t shm_grow(srt_hmap **hm, size_t extra_elems)

#API: |Ensure space for elements|hash map;absolute element reserve|reserved elements|O(1)|1;2|
size_t shm_reserve(srt_hmap **hm, size_t max_elems)

#API: |Make the hmap use the minimum possible memory|hmap|hmap reference (optional usage)|O(1) for allocators using memory remap; O(n) for naive allocators|1;2|
srt_hmap *shm_shrink(srt_hmap **hm);

#API: |Get hmap size|hmap|Hash map number of elements|O(1)|1;2|
size_t shm_size(const srt_hmap *hm);

#API: |Get hmap size|hmap|Hash map current max number of elements|O(1)|1;2|
size_t shm_max_size(const srt_hmap *hm);

#API: |Allocated space|hmap|current allocated space (vector elements)|O(1)|1;2|
size_t shm_capacity(const srt_hmap *hm);

#API: |Preallocated space left|hmap|allocated space left|O(1)|1;2|
size_t shm_capacity_left(const srt_hmap *hm);

#API: |Tells if a hash map is empty (zero elements)|hmap|S_TRUE: empty; S_FALSE: not empty|O(1)|1;2|
srt_bool shm_empty(const srt_hmap *hm)
*/

/* #API: |Duplicate hash map|input map|output map|O(n)|1;2| */
srt_hmap *shm_dup(const srt_hmap *src);

/* #API: |Clear/reset map (keeping map type)|hmap||O(1) for simple maps, O(n) for maps having nodes with strings|0;1| */
void shm_clear(srt_hmap *hm);

/*
#API: |Free one or more hash maps|hash map; more hash maps (optional)|-|O(1) for simple dmaps, O(n) for dmaps having nodes with strings|1;2|
void shm_free(srt_hmap **hm, ...)
*/
#ifdef S_USE_VA_ARGS
#define shm_free(...) shm_free_aux(__VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#else
#define shm_free(m) shm_free_aux(m, S_INVALID_PTR_VARG_TAIL)
#endif
void shm_free_aux(srt_hmap **s, ...);

/*
 * Copy
 */

/* #API: |Overwrite map with a map copy|output hash map; input map|output map reference (optional usage)|O(n)|0;1| */
srt_hmap *shm_cpy(srt_hmap **hm, const srt_hmap *src);

/*
 * Random access
 */

const void *shm_at(const srt_hmap *hm, uint32_t h, const void *key, uint32_t *tl);

S_INLINE const void *shm_at_s(const srt_hmap *hm, uint32_t h, const void *key, uint32_t *tl)
{
	return hm ? shm_at(hm, h, key, tl) : NULL;
}

/* #API: |Access to element (SHM_II32)|hash map; key|value|O(n), O(1) average amortized|1;2| */
S_INLINE int32_t shm_at_ii32(const srt_hmap *hm, int32_t k)
{
	const struct SHMapii *e = (const struct SHMapii *)
				shm_at_s(hm, SHM_HASH_32(k), &k, NULL);
	return e ? e->v : 0;
}

/* #API: |Access to element (SHM_UU32)|hash map; key|value|O(n), O(1) average amortized|1;2| */
S_INLINE uint32_t shm_at_uu32(const srt_hmap *hm, uint32_t k)
{
	const struct SHMapuu *e = (const struct SHMapuu *)
				shm_at_s(hm, SHM_HASH_32(k), &k, NULL);
	return e ? e->v : 0;
}

/* #API: |Access to element (SHM_II)|hash map; key|value|O(n), O(1) average amortized|1;2| */
S_INLINE int64_t shm_at_ii(const srt_hmap *hm, int64_t k)
{
	const struct SHMapII *e = (const struct SHMapII *)
				shm_at_s(hm, SHM_HASH_64(k), &k, NULL);
	return e ? e->v : 0;
}

/* #API: |Access to element (SHM_FF)|hash map; key|value|O(n), O(1) average amortized|1;2| */
S_INLINE float shm_at_ff(const srt_hmap *hm, float k)
{
	const struct SHMapFF *e = (const struct SHMapFF *)
				shm_at_s(hm, SHM_HASH_F(k), &k, NULL);
	return e ? e->v : 0;
}

/* #API: |Access to element (SHM_DD)|hash map; key|value|O(n), O(1) average amortized|1;2| */
S_INLINE double shm_at_dd(const srt_hmap *hm, double k)
{
	const struct SHMapDD *e = (const struct SHMapDD *)
				shm_at_s(hm, SHM_HASH_D(k), &k, NULL);
	return e ? e->v : 0;
}

/* #API: |Access to element (SHM_IS)|hash map; key|value|O(n), O(1) average amortized|0;1| */
S_INLINE const srt_string *shm_at_is(const srt_hmap *hm, int64_t k)
{
	const struct SHMapIS *e = (const struct SHMapIS *)
				shm_at_s(hm, SHM_HASH_64(k), &k, NULL);
	return e ? sso1_get(&e->v) : 0;
}

/* #API: |Access to element (SHM_IP)|hash map; key|value pointer|O(n), O(1) average amortized|0;1| */
S_INLINE const void *shm_at_ip(const srt_hmap *hm, int64_t k)
{
	const struct SHMapIP *e = (const struct SHMapIP *)
				shm_at_s(hm, SHM_HASH_64(k), &k, NULL);
	return e ? e->v : 0;
}

/* #API: |Access to element (SHM_SI)|hash map; key|value|O(n), O(1) average amortized|0;1| */
S_INLINE int64_t shm_at_si(const srt_hmap *hm, const srt_string *k)
{
	const struct SHMapSI *e = (const struct SHMapSI *)
					shm_at_s(hm, SHM_HASH_S(k), k, NULL);
	return e ? e->v : 0;
}

/* #API: |Access to element (SHM_DS)|hash map; key|value|O(n), O(1) average amortized|0;1| */
S_INLINE const srt_string *shm_at_ds(const srt_hmap *hm, double k)
{
	const struct SHMapDS *e = (const struct SHMapDS *)
				shm_at_s(hm, SHM_HASH_D(k), &k, NULL);
	return e ? sso1_get(&e->v) : 0;
}

/* #API: |Access to element (SHM_DP)|hash map; key|value pointer|O(n), O(1) average amortized|0;1| */
S_INLINE const void *shm_at_dp(const srt_hmap *hm, double k)
{
	const struct SHMapDP *e = (const struct SHMapDP *)
				shm_at_s(hm, SHM_HASH_D(k), &k, NULL);
	return e ? e->v : 0;
}

/* #API: |Access to element (SHM_SD)|hash map; key|value|O(n), O(1) average amortized|0;1| */
S_INLINE double shm_at_sd(const srt_hmap *hm, const srt_string *k)
{
	const struct SHMapSD *e = (const struct SHMapSD *)
					shm_at_s(hm, SHM_HASH_S(k), k, NULL);
	return e ? e->v : 0;
}

/* #API: |Access to element (SHM_SS)|hash map; key|value|O(n), O(1) average amortized|1;2| */
S_INLINE const srt_string *shm_at_ss(const srt_hmap *hm, const srt_string *k)
{
	const struct SHMapSS *e = (const struct SHMapSS *)
					shm_at_s(hm, SHM_HASH_S(k), k, NULL);
	return e ? sso_get_s2(&e->kv) : ss_void;
}

/* #API: |Access to element (SHM_SP)|hash map; key|value pointer|O(n), O(1) average amortized|1;2| */
S_INLINE const void *shm_at_sp(const srt_hmap *hm, const srt_string *k)
{
	const struct SHMapSP *e = (const struct SHMapSP *)
					shm_at_s(hm, SHM_HASH_S(k), k, NULL);
	return e ? e->v : 0;
}

/*
 * Existence check
 */

/* #API: |Map element count/check (SHM_UU32)|hash map; key|S_TRUE: element found; S_FALSE: not in the map|O(n), O(1) average amortized|1;2| */
S_INLINE size_t shm_count_u32(const srt_hmap *hm, uint32_t k)
{
	return shm_at_s(hm, SHM_HASH_32(k), &k, NULL) ? 1 : 0;
}

/* #API: |Map element count/checks (SHM_II32)|hash map; key|S_TRUE: element found; S_FALSE: not in the map|O(n), O(1) average amortized|1;2| */
S_INLINE size_t shm_count_i32(const srt_hmap *hm, int32_t k)
{
	return shm_at_s(hm, SHM_HASH_32(k), &k, NULL) ? 1 : 0;
}

/* #API: |Map element count/check (SHM_I*)|hash map; key|S_TRUE: element found; S_FALSE: not in the map|O(n), O(1) average amortized|1;2| */
S_INLINE size_t shm_count_i(const srt_hmap *hm, int64_t k)
{
	return shm_at_s(hm, SHM_HASH_64(k), &k, NULL) ? 1 : 0;
}

/* #API: |Map element count/check (SHM_FF)|hash map; key|S_TRUE: element found; S_FALSE: not in the map|O(n), O(1) average amortized|1;2| */
S_INLINE size_t shm_count_f(const srt_hmap *hm, float k)
{
	return shm_at_s(hm, SHM_HASH_F(k), &k, NULL) ? 1 : 0;
}

/* #API: |Map element count/check (SHM_D*)|hash map; key|S_TRUE: element found; S_FALSE: not in the map|O(n), O(1) average amortized|1;2| */
S_INLINE size_t shm_count_d(const srt_hmap *hm, double k)
{
	return shm_at_s(hm, SHM_HASH_D(k), &k, NULL) ? 1 : 0;
}

/* #API: |Map element count/check (SHM_S*)|hash map; key|S_TRUE: element found; S_FALSE: not in the map|O(n), O(1) average amortized|1;2| */
S_INLINE size_t shm_count_s(const srt_hmap *hm, const srt_string *k)
{
	return shm_at_s(hm, SHM_HASH_S(k), k, NULL) ? 1 : 0;
}

/*
 * Insert
 */

/* #API: |Insert into map (SHM_II32)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_ii32(srt_hmap **hm, int32_t k, int32_t v);

/* #API: |Insert into map (SHM_UU32)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_uu32(srt_hmap **hm, uint32_t k, uint32_t v);

/* #API: |Insert into map (SHM_II)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_ii(srt_hmap **hm, int64_t k, int64_t v);

/* #API: |Insert into map (SHM_IS)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_is(srt_hmap **hm, int64_t k, const srt_string *v);

/* #API: |Insert into map (SHM_IP)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_ip(srt_hmap **hm, int64_t k, const void *v);

/* #API: |Insert into map (SHM_SI)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_si(srt_hmap **hm, const srt_string *k, int64_t v);

/* #API: |Insert into map (SHM_SS)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_ss(srt_hmap **hm, const srt_string *k, const srt_string *v);

/* #API: |Insert into map (SHM_SP)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_sp(srt_hmap **hm, const srt_string *k, const void *v);

/* #API: |Insert into map (SHM_FF)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_ff(srt_hmap **hm, float k, float v);

/* #API: |Insert into map (SHM_DD)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_dd(srt_hmap **hm, double k, double v);

/* #API: |Insert into map (SHM_DS)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_ds(srt_hmap **hm, double k, const srt_string *v);

/* #API: |Insert into map (SHM_DP)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_dp(srt_hmap **hm, double k, const void *v);

/* #API: |Insert into map (SHM_SD)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_insert_sd(srt_hmap **hm, const srt_string *k, double v);

/* Hash set support (proxy) */

srt_bool shm_insert_i32(srt_hmap **hm, int32_t k);
srt_bool shm_insert_u32(srt_hmap **hm, uint32_t k);
srt_bool shm_insert_i(srt_hmap **hm, int64_t k);
srt_bool shm_insert_s(srt_hmap **hm, const srt_string *k);
srt_bool shm_insert_f(srt_hmap **hm, float k);
srt_bool shm_insert_d(srt_hmap **hm, double k);

/*
 * Increment
 */

/* #API: |Increment value map element (SHM_II32)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_inc_ii32(srt_hmap **hm, int32_t k, int32_t v);

/* #API: |Increment map element (SHM_UU32)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_inc_uu32(srt_hmap **hm, uint32_t k, uint32_t v);

/* #API: |Increment map element (SHM_II)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_inc_ii(srt_hmap **hm, int64_t k, int64_t v);

/* #API: |Increment map element (SHM_SI)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_inc_si(srt_hmap **hm, const srt_string *k, int64_t v);

/* #API: |Increment map element (SHM_FF)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_inc_ff(srt_hmap **hm, float k, float v);

/* #API: |Increment map element (SHM_DD)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_inc_dd(srt_hmap **hm, double k, double v);

/* #API: |Increment map element (SHM_SD)|hash map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(n), O(1) average amortized|1;2| */
srt_bool shm_inc_sd(srt_hmap **hm, const srt_string *k, double v);

/*
 * Delete
 */

/* #API: |Delete map element (SHM_II32)|hash map; key|S_TRUE: found and deleted; S_FALSE: not found|O(n), O(1) average amortized|1;2| */
srt_bool shm_delete_i32(srt_hmap *hm, int32_t k);

/* #API: |Delete map element (SHM_UU32)|hash map; key|S_TRUE: found and deleted; S_FALSE: not found|O(n), O(1) average amortized|1;2| */
srt_bool shm_delete_u32(srt_hmap *hm, uint32_t k);

/* #API: |Delete map element (SHM_I*)|hash map; key|S_TRUE: found and deleted; S_FALSE: not found|O(n), O(1) average amortized|1;2| */
srt_bool shm_delete_i(srt_hmap *hm, int64_t k);

/* #API: |Delete map element (SHM_FF)|hash map; key|S_TRUE: found and deleted; S_FALSE: not found|O(n), O(1) average amortized|1;2| */
srt_bool shm_delete_f(srt_hmap *hm, float k);

/* #API: |Delete map element (SHM_D*)|hash map; key|S_TRUE: found and deleted; S_FALSE: not found|O(n), O(1) average amortized|1;2| */
srt_bool shm_delete_d(srt_hmap *hm, double k);

/* #API: |Delete map element (SHM_S*)|hash map; key|S_TRUE: found and deleted; S_FALSE: not found|O(n), O(1) average amortized|1;2| */
srt_bool shm_delete_s(srt_hmap *hm, const srt_string *k);

/*
 * Enumeration
 */

S_INLINE const uint8_t *shm_enum_r(const srt_hmap *h, size_t i)
{
	return h && i < shm_size(h) ? shm_get_buffer_r(h) + i * h->d.elem_size :
				      NULL;
}

#define S_SHM_ENUM_AUX_K(NT, m, i, n_k, def_k)		\
	const NT *n = (const NT *)shm_enum_r(m, i);	\
	RETURN_IF(!n, def_k);				\
	return n_k

#define S_SHM_ENUM_AUX_V(t, NT, m, i, n_v, def_v)	\
	const NT *n;					\
	RETURN_IF(!m || t != m->d.sub_type, def_v);	\
	n = (const NT *)shm_enum_r(m, i);		\
	RETURN_IF(!n, def_v);				\
	return n_v

/* #API: |Enumerate map keys (SHM_II32)|hash map; element, 0 to n - 1|int32_t|O(1)|1;2| */
S_INLINE int32_t shm_it_i32_k(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_K(struct SHMapi, hm, i, n->k, 0);
}

/* #API: |Enumerate map values (SHM_II32)|hash map; element, 0 to n - 1|int32_t|O(1)|1;2| */
S_INLINE int32_t shm_it_ii32_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_II32, struct SHMapii, hm, i, n->v, 0);
}

/* #API: |Enumerate map keys (SHM_UU32)|hash map; element, 0 to n - 1|uint32_t|O(1)|1;2| */
S_INLINE uint32_t shm_it_u32_k(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_K(struct SHMapu, hm, i, n->k, 0);
}

/* #API: |Enumerate map values (SHM_UU32)|hash map; element, 0 to n - 1|uint32_t|O(1)|1;2| */
S_INLINE uint32_t shm_it_uu32_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_UU32, struct SHMapuu, hm, i, n->v, 0);
}

/* #API: |Enumerate map keys (SHM_I*)|hash map; element, 0 to n - 1|int64_t|O(1)|1;2| */
S_INLINE int64_t shm_it_i_k(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_K(struct SHMapI, hm, i, n->k, 0);
}

/* #API: |Enumerate map values (SHM_II)|hash map; element, 0 to n - 1|int64_t|O(1)|1;2| */
S_INLINE int64_t shm_it_ii_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_II, struct SHMapII, hm, i, n->v, 0);
}

/* #API: |Enumerate map values (SHM_IS)|hash map; element, 0 to n - 1|string|O(1)|1;2| */
S_INLINE const srt_string *shm_it_is_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_IS, struct SHMapIS, hm, i,
			sso_get((const srt_stringo *)&n->v), ss_void);
}

/* #API: |Enumerate map values (SHM_IP)|hash map; element, 0 to n - 1|pointer|O(1)|1;2| */
S_INLINE const void *shm_it_ip_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_IP, struct SHMapIP, hm, i, n->v, NULL);
}

/* #API: |Enumerate map keys (SHM_S*)|hash map; element, 0 to n - 1|string|O(1)|1;2| */
S_INLINE const srt_string *shm_it_s_k(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_K(struct SHMapS, hm, i,
			 sso_get((const srt_stringo *)&n->k), ss_void);
}

/* #API: |Enumerate map values (SHM_SI)|hash map; element, 0 to n - 1|int64_t|O(1)|1;2| */
S_INLINE int64_t shm_it_si_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_SI, struct SHMapSI, hm, i, n->v, 0);
}

/* #API: |Enumerate map values (SHM_SS)|hash map; element, 0 to n - 1|string|O(1)|1;2| */
S_INLINE const srt_string *shm_it_ss_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_SS, struct SHMapSS, hm, i, sso_get_s2(&n->kv),
			 ss_void);
}

/* #API: |Enumerate map (SHM_SP)|hash map; element, 0 to n - 1|pointer|O(1)|1;2| */
S_INLINE const void *shm_it_sp_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_SP, struct SHMapSP, hm, i, n->v, NULL);
}

/* #API: |Enumerate map keys (SHM_FF)|hash map; element, 0 to n - 1|float|O(1)|1;2| */
S_INLINE float shm_it_f_k(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_K(struct SHMapF, hm, i, n->k, 0);
}

/* #API: |Enumerate map values (SHM_FF)|hash map; element, 0 to n - 1|float|O(1)|1;2| */
S_INLINE float shm_it_ff_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_FF, struct SHMapFF, hm, i, n->v, 0);
}

/* #API: |Enumerate map keys (SHM_D*)|hash map; element, 0 to n - 1|double|O(1)|1;2| */
S_INLINE double shm_it_d_k(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_K(struct SHMapD, hm, i, n->k, 0);
}

/* #API: |Enumerate map values (SHM_DD)|hash map; element, 0 to n - 1|double|O(1)|1;2| */
S_INLINE double shm_it_dd_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_DD, struct SHMapDD, hm, i, n->v, 0);
}

/* #API: |Enumerate map values (SHM_DS)|hash map; element, 0 to n - 1|double|O(1)|1;2| */
S_INLINE const srt_string *shm_it_ds_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_DS, struct SHMapDS, hm, i,
			sso_get((const srt_stringo *)&n->v), ss_void);
}

/* #API: |Enumerate map values (SHM_DP)|hash map; element, 0 to n - 1|double|O(1)|1;2| */
S_INLINE const void *shm_it_dp_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_DP, struct SHMapDP, hm, i, n->v, 0);
}

/* #API: |Enumerate map values (SHM_SD)|hash map; element, 0 to n - 1|int64_t|O(1)|1;2| */
S_INLINE double shm_it_sd_v(const srt_hmap *hm, size_t i)
{
	S_SHM_ENUM_AUX_V(SHM_SD, struct SHMapSD, hm, i, n->v, 0);
}

/*
 * Enumeration, with callback helper
 */

typedef srt_bool (*srt_hmap_it_ii32)(int32_t k, int32_t v, void *context);
typedef srt_bool (*srt_hmap_it_uu32)(uint32_t k, uint32_t v, void *context);
typedef srt_bool (*srt_hmap_it_ii)(int64_t k, int64_t v, void *context);
typedef srt_bool (*srt_hmap_it_ff)(float k, float v, void *context);
typedef srt_bool (*srt_hmap_it_dd)(double k, double v, void *context);
typedef srt_bool (*srt_hmap_it_is)(int64_t k, const srt_string *, void *context);
typedef srt_bool (*srt_hmap_it_ip)(int64_t k, const void *, void *context);
typedef srt_bool (*srt_hmap_it_si)(const srt_string *, int64_t v, void *context);
typedef srt_bool (*srt_hmap_it_ds)(double k, const srt_string *, void *context);
typedef srt_bool (*srt_hmap_it_dp)(double k, const void *, void *context);
typedef srt_bool (*srt_hmap_it_sd)(const srt_string *, double v, void *context);
typedef srt_bool (*srt_hmap_it_ss)(const srt_string *, const srt_string *, void *context);
typedef srt_bool (*srt_hmap_it_sp)(const srt_string *, const void *, void *context);

/* #API: |Enumerate map elements in portions (SHM_II32)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_ii32(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_ii32 f, void *context);

/* #API: |Enumerate map elements in portions (SHM_UU32)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_uu32(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_uu32 f, void *context);

/* #API: |Enumerate map elements in portions (SHM_II)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_ii(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_ii f, void *context);

/* #API: |Enumerate map elements in portions (SHM_FF)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_ff(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_ff f, void *context);

/* #API: |Enumerate map elements in portions (SHM_DD)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_dd(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_dd f, void *context);

/* #API: |Enumerate map elements in portions (SHM_IS)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_is(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_is f, void *context);

/* #API: |Enumerate map elements in portions (SHM_IP)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_ip(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_ip f, void *context);

/* #API: |Enumerate map elements in portions (SHM_SI)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_si(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_si f, void *context);

/* #API: |Enumerate map elements in portions (SHM_DS)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_ds(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_ds f, void *context);

/* #API: |Enumerate map elements in portions (SHM_DP)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_dp(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_dp f, void *context);

/* #API: |Enumerate map elements in portions (SHM_SD)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_sd(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_sd f, void *context);

/* #API: |Enumerate map elements in portions (SHM_SS)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_ss(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_ss f, void *context);

/* #API: |Enumerate map elements in portions (SHM_SP)|map; index start; index end; callback function; callback function context|Elements processed|O(n)|1;2| */
size_t shm_itp_sp(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_sp f, void *context);

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* #ifndef SHMAP_H */


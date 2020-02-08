#ifndef SMAP_H
#define SMAP_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * smap.h
 *
 * #SHORTDOC map handling (key-value storage)
 *
 * #DOC Map functions handle key-value storage, which is implemented as a
 * #DOC Red-Black tree (O(log n) time complexity for insert/read/delete)
 * #DOC
 * #DOC
 * #DOC Supported key/value modes (enum eSM_Type):
 * #DOC
 * #DOC
 * #DOC	SM_II32: int32_t key, int32_t value
 * #DOC
 * #DOC	SM_UU32: uint32_t key, uint32_t value
 * #DOC
 * #DOC	SM_II: int64_t key, int64_t value
 * #DOC
 * #DOC	SM_FF: float (single-precision floating point) key, float value
 * #DOC
 * #DOC	SM_DD: double (double-precision floating point) key, double value
 * #DOC
 * #DOC	SM_IS: int64_t key, string value
 * #DOC
 * #DOC	SM_IP: int64_t key, pointer value
 * #DOC
 * #DOC	SM_SI: string key, int64_t value
 * #DOC
 * #DOC	SM_DS: double key, string value
 * #DOC
 * #DOC	SM_DP: double key, pointer value
 * #DOC
 * #DOC	SM_SD: string key, double value
 * #DOC
 * #DOC	SM_SS: string key, string value
 * #DOC
 * #DOC	SM_SP: string key, pointer value
 * #DOC
 * #DOC
 * #DOC Callback types for the sm_itr_*() functions:
 * #DOC
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_ii32)(int32_t k, int32_t v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_uu32)(uint32_t k, uint32_t v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_ii)(int64_t k, int64_t v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_ff)(float k, float v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_dd)(double k, double v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_is)(int64_t k, const srt_string *, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_ip)(int64_t k, const void *, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_si)(const srt_string *, int64_t v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_ds)(double k, const srt_string *, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_dp)(double k, const void *, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_sd)(const srt_string *, double v, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_ss)(const srt_string *, const srt_string *, void *context);
 * #DOC
 * #DOC	typedef srt_bool (*srt_map_it_sp)(const srt_string *, const void *, void *context);
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "saux/stree.h"
#include "saux/sstringo.h"
#include "sstring.h"
#include "svector.h"

/*
 * Structures
 */

enum eSM_Type0 {
	SM0_II32,
	SM0_UU32,
	SM0_II,
	SM0_IS,
	SM0_IP,
	SM0_SI,
	SM0_SS,
	SM0_SP,
	SM0_I,
	SM0_I32,
	SM0_U32,
	SM0_S,
	SM0_F,
	SM0_D,
	SM0_FF,
	SM0_DD,
	SM0_DP,
	SM0_DS,
	SM0_SD,
	SM0_NumTypes
};

enum eSM_Type {
	SM_II32 = SM0_II32,
	SM_UU32 = SM0_UU32,
	SM_II = SM0_II,
	SM_IS = SM0_IS,
	SM_IP = SM0_IP,
	SM_SI = SM0_SI,
	SM_SS = SM0_SS,
	SM_SP = SM0_SP,
	SM_FF = SM0_FF,
	SM_DD = SM0_DD,
	SM_DS = SM0_DS,
	SM_DP = SM0_DP,
	SM_SD = SM0_SD
};

struct SMapI {
	srt_tnode n;
	int64_t k;
};

struct SMapS {
	srt_tnode n;
	srt_stringo1 k;
};

struct SMapi {
	srt_tnode n;
	int32_t k;
};

struct SMapu {
	srt_tnode n;
	uint32_t k;
};

struct SMapii {
	struct SMapi x;
	int32_t v;
};

struct SMapuu {
	struct SMapu x;
	uint32_t v;
};

struct SMapII {
	struct SMapI x;
	int64_t v;
};

struct SMapIS {
	struct SMapI x;
	srt_stringo1 v;
};

struct SMapIP {
	struct SMapI x;
	const void *v;
};

struct SMapSI {
	struct SMapS x;
	int64_t v;
};

struct SMapSS {
	srt_tnode n;
	srt_stringo s;
};

struct SMapSP {
	struct SMapS x;
	const void *v;
};

struct SMapF {
	srt_tnode n;
	float k;
};

struct SMapD {
	srt_tnode n;
	double k;
};

struct SMapFF {
	struct SMapF x;
	float v;
};

struct SMapDD {
	struct SMapD x;
	double v;
};

struct SMapDS {
	struct SMapD x;
	srt_stringo1 v;
};

struct SMapDP {
	struct SMapD x;
	const void *v;
};

struct SMapSD {
	struct SMapS x;
	double v;
};

typedef srt_tree srt_map; /* Opaque structure (accessors are provided) */
			  /* (map is implemented as a tree)	     */

typedef srt_bool (*srt_map_it_ii32)(int32_t k, int32_t v, void *context);
typedef srt_bool (*srt_map_it_uu32)(uint32_t k, uint32_t v, void *context);
typedef srt_bool (*srt_map_it_ii)(int64_t k, int64_t v, void *context);
typedef srt_bool (*srt_map_it_is)(int64_t k, const srt_string *, void *context);
typedef srt_bool (*srt_map_it_ip)(int64_t k, const void *, void *context);
typedef srt_bool (*srt_map_it_si)(const srt_string *, int64_t v, void *context);
typedef srt_bool (*srt_map_it_ss)(const srt_string *, const srt_string *, void *context);
typedef srt_bool (*srt_map_it_sp)(const srt_string *, const void *, void *context);
typedef srt_bool (*srt_map_it_ff)(float k, float v, void *context);
typedef srt_bool (*srt_map_it_dd)(double k, double v, void *context);
typedef srt_bool (*srt_map_it_ds)(double k, const srt_string *, void *context);
typedef srt_bool (*srt_map_it_dp)(double k, const void *, void *context);
typedef srt_bool (*srt_map_it_sd)(const srt_string *, double v, void *context);

/*
 * Allocation
 */

/*
#API: |Allocate map (stack)|map type; initial reserve|map|O(1)|1;2|
srt_map *sm_alloca(enum eSM_Type t, size_t n);
*/
#define sm_alloca(type, max_size)                                              \
	sm_alloc_raw(type, S_TRUE,                                             \
		     s_alloca(sd_alloc_size_raw(sizeof(srt_map),               \
						sm_elem_size(type), max_size,  \
						S_FALSE)),                     \
		     sm_elem_size(type), max_size)

srt_map *sm_alloc_raw0(enum eSM_Type0 t, srt_bool ext_buf,
		       void *buffer, size_t elem_size,
		       size_t max_size);

S_INLINE srt_map *sm_alloc_raw(enum eSM_Type t, srt_bool ext_buf,
			       void *buffer, size_t elem_size,
			       size_t max_size)
{
	return sm_alloc_raw0((enum eSM_Type0)t, ext_buf, buffer, elem_size,
			     max_size);
}

srt_map *sm_alloc0(enum eSM_Type0 t, size_t initial_num_elems_reserve);

/* #API: |Allocate map (heap)|map type; initial reserve|map|O(1)|1;2| */
S_INLINE srt_map *sm_alloc(enum eSM_Type t, size_t initial_num_elems_reserve)
{
	return sm_alloc0((enum eSM_Type0)t, initial_num_elems_reserve);
}

/* #NOTAPI: |Get map node size from map type|map type|bytes required for storing a single node|O(1)|1;2| */
S_INLINE uint8_t sm_elem_size(int t)
{
	switch (t) {
	case SM0_II32:
		return sizeof(struct SMapii);
	case SM0_UU32:
		return sizeof(struct SMapuu);
	case SM0_II:
		return sizeof(struct SMapII);
	case SM0_IS:
		return sizeof(struct SMapIS);
	case SM0_IP:
		return sizeof(struct SMapIP);
	case SM0_SI:
		return sizeof(struct SMapSI);
	case SM0_SS:
		return sizeof(struct SMapSS);
	case SM0_SP:
		return sizeof(struct SMapSP);
	case SM0_I32:
		return sizeof(struct SMapi);
	case SM0_U32:
		return sizeof(struct SMapu);
	case SM0_I:
		return sizeof(struct SMapI);
	case SM0_S:
		return sizeof(struct SMapS);
	case SM0_F:
		return sizeof(struct SMapF);
	case SM0_D:
		return sizeof(struct SMapD);
	case SM0_FF:
		return sizeof(struct SMapFF);
	case SM0_DD:
		return sizeof(struct SMapDD);
	case SM0_DS:
		return sizeof(struct SMapDS);
	case SM0_DP:
		return sizeof(struct SMapDP);
	case SM0_SD:
		return sizeof(struct SMapSD);
	default:
		break;
	}
	return 0;
}

/* #API: |Duplicate map|input map|output map|O(n)|1;2| */
srt_map *sm_dup(const srt_map *src);

/* #API: |Reset/clean map (keeping map type)|map|-|O(1) for simple maps, O(n) for maps having nodes with strings|1;2| */
void sm_clear(srt_map *m);

/*
#API: |Free one or more maps (heap)|map; more maps (optional)|-|O(1) for simple maps, O(n) for maps having nodes with strings|1;2|
void sm_free(srt_map **m, ...)
*/
#ifdef S_USE_VA_ARGS
#define sm_free(...) sm_free_aux(__VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#else
#define sm_free(m) sm_free_aux(m, S_INVALID_PTR_VARG_TAIL)
#endif
void sm_free_aux(srt_map **m, ...);

SD_BUILDFUNCS_FULL_ST(sm, srt_map, 0)

/*
#API: |Ensure space for extra elements|map;number of extra elements|extra size allocated|O(1)|1;2|
size_t sm_grow(srt_map **m, size_t extra_elems)

#API: |Ensure space for elements|map;absolute element reserve|reserved elements|O(1)|1;2|
size_t sm_reserve(srt_map **m, size_t max_elems)

#API: |Make the map use the minimum possible memory|map|map reference (optional usage)|O(1) for allocators using memory remap; O(n) for naive allocators|1;2|
srt_map *sm_shrink(srt_map **m);

#API: |Get map size|map|Map number of elements|O(1)|1;2|
size_t sm_size(const srt_map *m);

#API: |Allocated space|map|current allocated space (vector elements)|O(1)|1;2|
size_t sm_capacity(const srt_map *m);

#API: |Preallocated space left|map|allocated space left|O(1)|1;2|
size_t sm_capacity_left(const srt_map *m);

#API: |Tells if a map is empty (zero elements)|map|S_TRUE: empty; S_FALSE: not empty|O(1)|1;2|
srt_bool sm_empty(const srt_map *m)
*/

/*
 * Copy
 */

/* #API: |Overwrite map with a map copy|output map; input map|output map reference (optional usage)|O(n)|1;2| */
srt_map *sm_cpy(srt_map **m, const srt_map *src);

/*
 * Random access
 */

/* #API: |Access to map element (SM_II32)|map; key|value|O(log n)|1;2| */
int32_t sm_at_ii32(const srt_map *m, int32_t k);

/* #API: |Access to map element (SM_UU32)|map; key|value|O(log n)|1;2| */
uint32_t sm_at_uu32(const srt_map *m, uint32_t k);

/* #API: |Access to map element (SM_II)|map; key|value|O(log n)|1;2| */
int64_t sm_at_ii(const srt_map *m, int64_t k);

/* #API: |Access to map element (SM_FF)|map; key|value|O(log n)|1;2| */
float sm_at_ff(const srt_map *m, float k);

/* #API: |Access to map element (SM_DD)|map; key|value|O(log n)|1;2| */
double sm_at_dd(const srt_map *m, double k);

/* #API: |Access to map element (SM_IS)|map; key|value|O(log n)|1;2| */
const srt_string *sm_at_is(const srt_map *m, int64_t k);

/* #API: |Access to map element (SM_IP)|map; key|value|O(log n)|1;2| */
const void *sm_at_ip(const srt_map *m, int64_t k);

/* #API: |Access to map element (SM_SI)|map; key|value|O(log n)|1;2| */
int64_t sm_at_si(const srt_map *m, const srt_string *k);

/* #API: |Access to map element (SM_SS)|map; key|value|O(log n)|1;2| */
const srt_string *sm_at_ss(const srt_map *m, const srt_string *k);

/* #API: |Access to map element (SM_SP)|map; key|value|O(log n)|1;2| */
const void *sm_at_sp(const srt_map *m, const srt_string *k);

/* #API: |Access to map element (SM_DS)|map; key|value|O(log n)|1;2| */
const srt_string *sm_at_ds(const srt_map *m, double k);

/* #API: |Access to map element (SM_DP)|map; key|value|O(log n)|1;2| */
const void *sm_at_dp(const srt_map *m, double k);

/* #API: |Access to map element (SM_SD)|map; key|value|O(log n)|1;2| */
double sm_at_sd(const srt_map *m, const srt_string *k);

/*
 * Existence check
 */

/* #API: |Map element count/check (SM_II32)|map; key|S_TRUE: element found; S_FALSE: not in the map|O(log n)|1;2| */
size_t sm_count_i32(const srt_map *m, int32_t k);

/* #API: |Map element count/check|map (SM_UU32); key|S_TRUE: element found; S_FALSE: not in the map|O(log n)|1;2| */
size_t sm_count_u32(const srt_map *m, uint32_t k);

/* #API: |Map element count/check|map (SM_II); key|S_TRUE: element found; S_FALSE: not in the map|O(log n)|1;2| */
size_t sm_count_i(const srt_map *m, int64_t k);

/* #API: |Map element count/check|map (SM_FF); key|S_TRUE: element found; S_FALSE: not in the map|O(log n)|1;2| */
size_t sm_count_f(const srt_map *m, float k);

/* #API: |Map element count/check|map (SM_D*); key|S_TRUE: element found; S_FALSE: not in the map|O(log n)|1;2| */
size_t sm_count_d(const srt_map *m, double k);

/* #API: |Map element count/check|map (SM_S*); key|S_TRUE: element found; S_FALSE: not in the map|O(log n)|1;2| */
size_t sm_count_s(const srt_map *m, const srt_string *k);

/*
 * Insert
 */

/* #API: |Insert map element (SM_II32)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_ii32(srt_map **m, int32_t k, int32_t v);

/* #API: |Insert map element (SM_UU32)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_uu32(srt_map **m, uint32_t k, uint32_t v);

/* #API: |Insert map element|map (SM_II); key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_ii(srt_map **m, int64_t k, int64_t v);

/* #API: |Insert map element (SM_FF)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_ff(srt_map **m, float k, float v);

/* #API: |Insert map element (SM_DD)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_dd(srt_map **m, double k, double v);

/* #API: |Insert map element (SM_IS)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_is(srt_map **m, int64_t k, const srt_string *v);

/* #API: |Insert map element (SM_IP)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_ip(srt_map **m, int64_t k, const void *v);

/* #API: |Insert map element (SM_SI)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_si(srt_map **m, const srt_string *k, int64_t v);

/* #API: |Insert map element (SM_DS)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_ds(srt_map **m, double k, const srt_string *v);

/* #API: |Insert map element (SM_DP)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_dp(srt_map **m, double k, const void *v);

/* #API: |Insert map element (SM_SD)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_sd(srt_map **m, const srt_string *k, double v);

/* #API: |Insert map element (SM_SS)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_ss(srt_map **m, const srt_string *k, const srt_string *v);

/* #API: |Insert map element (SM_SP)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_insert_sp(srt_map **m, const srt_string *k, const void *v);

/* #API: |Increment map element (SM_II32)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_inc_ii32(srt_map **m, int32_t k, int32_t v);

/* #API: |Increment map element (SM_UU32)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_inc_uu32(srt_map **m, uint32_t k, uint32_t v);

/* #API: |Increment map element (SM_II)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_inc_ii(srt_map **m, int64_t k, int64_t v);

/* #API: |Increment map element (SM_FF)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_inc_ff(srt_map **m, float k, float v);

/* #API: |Increment map element (SM_DD)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_inc_dd(srt_map **m, double k, double v);

/* #API: |Increment map element (SM_SI)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_inc_si(srt_map **m, const srt_string *k, int64_t v);

/* #API: |Increment map element (SM_SD)|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
srt_bool sm_inc_sd(srt_map **m, const srt_string *k, double v);

/*
 * Delete
 */

/* #API: |Delete map element (SM_II32)|map; key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
srt_bool sm_delete_i32(srt_map *m, int32_t k);

/* #API: |Delete map element (SM_UU32)|map; key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
srt_bool sm_delete_u32(srt_map *m, uint32_t k);

/* #API: |Delete map element (SM_I*)|map; key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
srt_bool sm_delete_i(srt_map *m, int64_t k);

/* #API: |Delete map element (SM_FF)|map; key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|0;1| */
srt_bool sm_delete_f(srt_map *m, float k);

/* #API: |Delete map element (SM_D*)|map; key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|0;1| */
srt_bool sm_delete_d(srt_map *m, double k);

/* #API: |Delete map element (SM_S*)|map; key|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
srt_bool sm_delete_s(srt_map *m, const srt_string *k);

/*
 * Enumeration / export data
 */

/* #API: |Enumerate map elements in a given key range (SM_II32)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_ii32(const srt_map *m, int32_t key_min, int32_t key_max, srt_map_it_ii32 f, void *context);

/* #API: |Enumerate map elements in a given key range (SM_UU32)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_uu32(const srt_map *m, uint32_t key_min, uint32_t key_max, srt_map_it_uu32 f, void *context);

/* #API: |Enumerate map elements in a given key range (SM_II)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_ii(const srt_map *m, int64_t key_min, int64_t key_max, srt_map_it_ii f, void *context);

/* #API: |Enumerate map elements in a given key range (SM_FF)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_ff(const srt_map *m, float key_min, float key_max, srt_map_it_ff f, void *context);

/* #API: |Enumerate map elements in a given key range (SM_DD)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_dd(const srt_map *m, double key_min, double key_max, srt_map_it_dd f, void *context);

/* #API: |Enumerate map elements in a given key range (SM_IS)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_is(const srt_map *m, int64_t key_min, int64_t key_max, srt_map_it_is f, void *context);

/* #API: |Enumerate map elements in a given key range (SM_IP)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_ip(const srt_map *m, int64_t key_min, int64_t key_max, srt_map_it_ip f, void *context);

/* #API: |Enumerate map elements in a given key range (SM_SI)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_si(const srt_map *m, const srt_string *key_min, const srt_string *key_max, srt_map_it_si f, void *context);

/* #API: |Enumerate map elements in a given key range (SM_DS)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_ds(const srt_map *m, double key_min, double key_max, srt_map_it_ds f, void *context);

/* #API: |Enumerate map elements in a given key range (SM_DP)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_dp(const srt_map *m, double key_min, double key_max, srt_map_it_dp f, void *context);

/* #API: |Enumerate map elements in a given key range (SM_SD)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_sd(const srt_map *m, const srt_string *key_min, const srt_string *key_max, srt_map_it_sd f, void *context);

/* #API: |Enumerate map elements in a given key range (SM_SS)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_ss(const srt_map *m, const srt_string *key_min, const srt_string *key_max, srt_map_it_ss f, void *context);

/* #API: |Enumerate map elements in a given key range (SM_SP)|map; key lower bound; key upper bound; callback function; callback function context|Elements processed|O(log n) + O(log m); additional 2 * O(log n) space required, allocated on the stack, i.e. fast|1;2| */
size_t sm_itr_sp(const srt_map *m, const srt_string *key_min, const srt_string *key_max, srt_map_it_sp f, void *context);

/* #NOTAPI: |Sort map to vector (used for test coverage, not as documented API)|map; output vector for keys; output vector for values|Number of map elements|O(n)|0;1| */
ssize_t sm_sort_to_vectors(const srt_map *m, srt_vector **kv, srt_vector **vv);

/*
 * Auxiliary inlined functions
 */

	/*
	 * Unordered enumeration is inlined in order to get almost as fast
	 * as array access after compiler optimization.
	 */

#define S_SM_ENUM_AUX_K(NT, m, i, n_k, def_k)                                  \
	const NT *n = (const NT *)st_enum_r(m, i);                             \
	RETURN_IF(!n, def_k);                                                  \
	return n_k

#define S_SM_ENUM_AUX_V(t, NT, m, i, n_v, def_v)                               \
	const NT *n;                                                           \
	RETURN_IF(!m || t != m->d.sub_type, def_v);                            \
	n = (const NT *)st_enum_r(m, i);                                       \
	RETURN_IF(!n, def_v);                                                  \
	return n_v

/* #API: |Enumerate map keys (SM_II32)|map; element, 0 to n - 1|int32_t|O(1)|1;2| */
S_INLINE int32_t sm_it_i32_k(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_K(struct SMapi, m, i, n->k, 0);
}

/* #API: |Enumerate map values (SM_II32)|map; element, 0 to n - 1|int32_t|O(1)|1;2| */
S_INLINE int32_t sm_it_ii32_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_II32, struct SMapii, m, i, n->v, 0);
}

/* #API: |Enumerate map keys (SM_UU32)|map; element, 0 to n - 1|uint32_t|O(1)|1;2| */
S_INLINE uint32_t sm_it_u32_k(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_K(struct SMapu, m, i, n->k, 0);
}

/* #API: |Enumerate map values (SM_UU32)|map; element, 0 to n - 1|uint32_t|O(1)|1;2| */
S_INLINE uint32_t sm_it_uu32_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_UU32, struct SMapuu, m, i, n->v, 0);
}

/* #API: |Enumerate map keys (SM_I*)|map; element, 0 to n - 1|int64_t|O(1)|1;2| */
S_INLINE int64_t sm_it_i_k(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_K(struct SMapI, m, i, n->k, 0);
}

/* #API: |Enumerate map values (SM_II)|map; element, 0 to n - 1|int64_t|O(1)|1;2| */
S_INLINE int64_t sm_it_ii_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_II, struct SMapII, m, i, n->v, 0);
}

/* #API: |Enumerate map keys (SM_FF)|map; element, 0 to n - 1|float|O(1)|1;2| */
S_INLINE float sm_it_f_k(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_K(struct SMapF, m, i, n->k, 0);
}

/* #API: |Enumerate map values (SM_FF)|map; element, 0 to n - 1|float|O(1)|1;2| */
S_INLINE float sm_it_ff_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_FF, struct SMapFF, m, i, n->v, 0);
}

/* #API: |Enumerate map keys (SM_D*)|map; element, 0 to n - 1|double|O(1)|1;2| */
S_INLINE double sm_it_d_k(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_K(struct SMapD, m, i, n->k, 0);
}

/* #API: |Enumerate map values (SM_DD)|map; element, 0 to n - 1|double|O(1)|1;2| */
S_INLINE double sm_it_dd_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_DD, struct SMapDD, m, i, n->v, 0);
}

/* #API: |Enumerate map values (SM_IS)|map; element, 0 to n - 1|string|O(1)|1;2| */
S_INLINE const srt_string *sm_it_is_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_IS, struct SMapIS, m, i,
			sso_get((const srt_stringo *)&n->v), ss_void);
}

/* #API: |Enumerate map values (SM_IP)|map; element, 0 to n - 1|pointer|O(1)|1;2| */
S_INLINE const void *sm_it_ip_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_IP, struct SMapIP, m, i, n->v, NULL);
}

/* #API: |Enumerate map values (SM_DS)|map; element, 0 to n - 1|string|O(1)|1;2| */
S_INLINE const srt_string *sm_it_ds_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_DS, struct SMapDS, m, i,
			sso_get((const srt_stringo *)&n->v), ss_void);
}

/* #API: |Enumerate map values (SM_DP)|map; element, 0 to n - 1|pointer|O(1)|1;2| */
S_INLINE const void *sm_it_dp_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_DP, struct SMapDP, m, i, n->v, NULL);
}

/* #API: |Enumerate map keys (SM_S*)|map; element, 0 to n - 1|string|O(1)|1;2| */
S_INLINE const srt_string *sm_it_s_k(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_K(struct SMapS, m, i, sso_get((const srt_stringo *)&n->k),
			ss_void);
}

/* #API: |Enumerate map values (SM_SI)|map; element, 0 to n - 1|int64_t|O(1)|1;2| */
S_INLINE int64_t sm_it_si_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_SI, struct SMapSI, m, i, n->v, 0);
}

/* #API: |Enumerate map values (SM_SD)|map; element, 0 to n - 1|int64_t|O(1)|1;2| */
S_INLINE double sm_it_sd_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_SD, struct SMapSD, m, i, n->v, 0);
}

/* #API: |Enumerate map values (SM_SS)|map; element, 0 to n - 1|string|O(1)|1;2| */
S_INLINE const srt_string *sm_it_ss_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_SS, struct SMapSS, m, i, sso_get_s2(&n->s), ss_void);
}

/* #API: |Enumerate map (SM_SP)|map; element, 0 to n - 1|pointer|O(1)|1;2| */
S_INLINE const void *sm_it_sp_v(const srt_map *m, srt_tndx i)
{
	S_SM_ENUM_AUX_V(SM_SP, struct SMapSP, m, i, n->v, NULL);
}

	/*
	 * Templates (internal usage)
	 */

#define SM_ENUM_INORDER_XX(FN, CALLBACK_T, MAP_TYPE, KEY_T, TR_CMP_MIN,        \
			   TR_CMP_MAX, TR_CALLBACK)                            \
	size_t FN(const srt_map *m, KEY_T kmin, KEY_T kmax, CALLBACK_T f,      \
		  void *context)                                               \
	{                                                                      \
		ssize_t level;                                                 \
		size_t ts, nelems, rbt_max_depth;                              \
		struct STreeScan *p;                                           \
		const srt_tnode *cn;                                           \
		int cmpmin, cmpmax;                                            \
		RETURN_IF(!m, 0);			 /* null tree */       \
		RETURN_IF(m->d.sub_type != MAP_TYPE, 0); /* wrong type */      \
		ts = sm_size(m);                                               \
		RETURN_IF(!ts, S_FALSE); /* empty tree */                      \
		level = 0;                                                     \
		nelems = 0;                                                    \
		rbt_max_depth = 2 * (slog2(ts) + 1);                           \
		p = (struct STreeScan *)s_alloca(sizeof(struct STreeScan)      \
						 * (rbt_max_depth + 3));       \
		ASSERT_RETURN_IF(!p, 0); /* BEHAVIOR: stack error */           \
		p[0].p = ST_NIL;                                               \
		p[0].c = m->root;                                              \
		p[0].s = STS_ScanStart;                                        \
		while (level >= 0) {                                           \
			S_ASSERT(level <= (ssize_t)rbt_max_depth);             \
			switch (p[level].s) {                                  \
			case STS_ScanStart:                                    \
				cn = get_node_r(m, p[level].c);                \
				cmpmin = TR_CMP_MIN;                           \
				cmpmax = TR_CMP_MAX;                           \
				if (cn->x.l != ST_NIL && cmpmin > 0) {         \
					p[level].s = STS_ScanLeft;             \
					level++;                               \
					cn = get_node_r(m, p[level - 1].c);    \
					p[level].c = cn->x.l;                  \
				} else {                                       \
					/* node with null left children */     \
					if (cmpmin >= 0 && cmpmax <= 0) {      \
						if (f && !TR_CALLBACK)         \
							return nelems;         \
						nelems++;                      \
					}                                      \
					if (cn->r != ST_NIL && cmpmax < 0) {   \
						p[level].s = STS_ScanRight;    \
						level++;                       \
						cn = get_node_r(               \
							m, p[level - 1].c);    \
						p[level].c = cn->r;            \
					} else {                               \
						p[level].s = STS_ScanDone;     \
						level--;                       \
						continue;                      \
					}                                      \
				}                                              \
				p[level].p = p[level - 1].c;                   \
				p[level].s = STS_ScanStart;                    \
				continue;                                      \
			case STS_ScanLeft:                                     \
				cn = get_node_r(m, p[level].c);                \
				cmpmin = TR_CMP_MIN;                           \
				cmpmax = TR_CMP_MAX;                           \
				if (cmpmin >= 0 && cmpmax <= 0) {              \
					if (f && !TR_CALLBACK)                 \
						return nelems;                 \
					nelems++;                              \
				}                                              \
				if (cn->r != ST_NIL && cmpmax < 0) {           \
					p[level].s = STS_ScanRight;            \
					level++;                               \
					p[level].p = p[level - 1].c;           \
					cn = get_node_r(m, p[level - 1].c);    \
					p[level].c = cn->r;                    \
					p[level].s = STS_ScanStart;            \
				} else {                                       \
					p[level].s = STS_ScanDone;             \
					level--;                               \
					continue;                              \
				}                                              \
				continue;                                      \
			case STS_ScanRight:                                    \
			case STS_ScanDone:                                     \
				p[level].s = STS_ScanDone;                     \
				level--;                                       \
				continue;                                      \
			}                                                      \
		}                                                              \
		return nelems;                                                 \
	}

#define BUILD_SMAP_CMPN(FN, TS, TK)				\
	S_INLINE int FN(const TS *a, TK b) {			\
		return a->k > b ? 1 : a->k < b ? -1 : 0;	\
	}

BUILD_SMAP_CMPN(cmp_ni_i, struct SMapi, int32_t)
BUILD_SMAP_CMPN(cmp_nu_u, struct SMapu, uint32_t)
BUILD_SMAP_CMPN(cmp_nI_I, struct SMapI, int64_t)
BUILD_SMAP_CMPN(cmp_nF_F, struct SMapF, float)
BUILD_SMAP_CMPN(cmp_nD_D, struct SMapD, double)

S_INLINE int cmp_ns_s(const struct SMapS *a, const srt_string *b)
{
	return ss_cmp(sso_get((const srt_stringo *)&a->k), b);
}

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* #ifndef SMAP_H */

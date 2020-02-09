/*
 * shmap.c
 *
 * Hash map handling
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "shmap.h"
#include "saux/shash.h"
#include "saux/sstringo.h"

/*
 * Internal constants
 */

struct SHMapCtx {
	shm_eq_f eqf;
	shm_del_f delf;
	shm_hash_f hashf;
	shm_n2key_f n2kf;
};

/*
 * Constants and macros
 */
#define SHM_MAX_ELEMS 0xffffffff
#define SHM_LOC_EMPTY 0			    /* do not change this */
#define SHM_REHASH_DEFAULT_THRESHOLD_PCT 90 /* rehash at 90% of buckets */
#define shm_void (srt_hmap *)sd_void

/*
 * Internal functions
 */

S_INLINE size_t h2bid(uint32_t h32, size_t hbits)
{
	return h32 >> (32 - hbits);
}

static srt_bool eq_64(const void *key, const void *node)
{
	return memcmp(key, node, sizeof(int64_t)) ? S_FALSE : S_TRUE;
}

static srt_bool eq_32(const void *key, const void *node)
{
	return memcmp(key, node, sizeof(int32_t)) ? S_FALSE : S_TRUE;
}

static srt_bool eq_f(const void *key, const void *node)
{
	return memcmp(key, node, sizeof(float)) ? S_FALSE : S_TRUE;
}

static srt_bool eq_d(const void *key, const void *node)
{
	return memcmp(key, node, sizeof(double)) ? S_FALSE : S_TRUE;
}

static srt_bool eq_sso(const void *key, const void *node)
{
	return sso_eq((const srt_string *)key, (const srt_stringo *)node);
}

static srt_bool eq_sso1(const void *key, const void *node)
{
	return sso1_eq((const srt_string *)key, (const srt_stringo1 *)node);
}

static void shmcb_set_ii32(void *loc, const void *key, const void *value)
{
	struct SHMapii *e = (struct SHMapii *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	memcpy(&e->v, value, sizeof(e->v));
}

static void shmcb_set_uu32(void *loc, const void *key, const void *value)
{
	struct SHMapuu *e = (struct SHMapuu *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	memcpy(&e->v, value, sizeof(e->v));
}

static void shmcb_set_ii64(void *loc, const void *key, const void *value)
{
	struct SHMapII *e = (struct SHMapII *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	memcpy(&e->v, value, sizeof(e->v));
}

static void shmcb_set_is(void *loc, const void *key, const void *value)
{
	struct SHMapIS *e = (struct SHMapIS *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	sso1_set(&e->v, (const srt_string *)value);
}

static void shmcb_set_ip(void *loc, const void *key, const void *value)
{
	struct SHMapIP *e = (struct SHMapIP *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	e->v = value;
}

static void shmcb_set_si(void *loc, const void *key, const void *value)
{
	struct SHMapSI *e = (struct SHMapSI *)loc;
	sso1_set(&e->x.k, (const srt_string *)key);
	memcpy(&e->v, value, sizeof(e->v));
}

static void shmcb_set_ss(void *loc, const void *key, const void *value)
{
	struct SHMapSS *e = (struct SHMapSS *)loc;
	sso_set(&e->kv, (const srt_string *)key, (const srt_string *)value);
}

static void shmcb_set_sp(void *loc, const void *key, const void *value)
{
	struct SHMapSP *e = (struct SHMapSP *)loc;
	sso1_set(&e->x.k, (const srt_string *)key);
	e->v = value;
}

static void shmcb_set_i32(void *loc, const void *key)
{
	struct SHMapi *e = (struct SHMapi *)loc;
	memcpy(&e->k, key, sizeof(e->k));
}

static void shmcb_set_u32(void *loc, const void *key)
{
	struct SHMapu *e = (struct SHMapu *)loc;
	memcpy(&e->k, key, sizeof(e->k));
}

static void shmcb_set_i64(void *loc, const void *key)
{
	struct SHMapI *e = (struct SHMapI *)loc;
	memcpy(&e->k, key, sizeof(e->k));
}

static void shmcb_set_s(void *loc, const void *key)
{
	struct SHMapS *e = (struct SHMapS *)loc;
	sso1_set(&e->k, (const srt_string *)key);
}

static void shmcb_set_f(void *loc, const void *key)
{
	struct SHMapF *e = (struct SHMapF *)loc;
	memcpy(&e->k, key, sizeof(e->k));
}

static void shmcb_set_d(void *loc, const void *key)
{
	struct SHMapD *e = (struct SHMapD *)loc;
	memcpy(&e->k, key, sizeof(e->k));
}

static void shmcb_set_ff(void *loc, const void *key, const void *value)
{
	struct SHMapFF *e = (struct SHMapFF *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	memcpy(&e->v, value, sizeof(e->v));
}

static void shmcb_set_dd(void *loc, const void *key, const void *value)
{
	struct SHMapDD *e = (struct SHMapDD *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	memcpy(&e->v, value, sizeof(e->v));
}

static void shmcb_set_ds(void *loc, const void *key, const void *value)
{
	struct SHMapDS *e = (struct SHMapDS *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	sso1_set(&e->v, (const srt_string *)value);
}

static void shmcb_set_dp(void *loc, const void *key, const void *value)
{
	struct SHMapDP *e = (struct SHMapDP *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	e->v = value;
}

static void shmcb_set_sd(void *loc, const void *key, const void *value)
{
	struct SHMapSD *e = (struct SHMapSD *)loc;
	sso1_set(&e->x.k, (const srt_string *)key);
	memcpy(&e->v, value, sizeof(e->v));
}

static void shmcb_inc_ii32(void *loc, const void *value)
{
	int32_t vi;
	struct SHMapii *e = (struct SHMapii *)loc;
	memcpy(&vi, value, sizeof(vi));
	e->v += vi;
}

static void shmcb_inc_uu32(void *loc, const void *value)
{
	uint32_t vi;
	struct SHMapuu *e = (struct SHMapuu *)loc;
	memcpy(&vi, value, sizeof(vi));
	e->v += vi;
}

static void shmcb_inc_ii64(void *loc, const void *value)
{
	int64_t vi;
	struct SHMapII *e = (struct SHMapII *)loc;
	memcpy(&vi, value, sizeof(vi));
	e->v += vi;
}

static void shmcb_inc_si(void *loc, const void *value)
{
	int64_t vi;
	struct SHMapSI *e = (struct SHMapSI *)loc;
	memcpy(&vi, value, sizeof(vi));
	e->v += vi;
}

static void shmcb_inc_ff(void *loc, const void *value)
{
	float vi;
	struct SHMapFF *e = (struct SHMapFF *)loc;
	memcpy(&vi, value, sizeof(vi));
	e->v += vi;
}

static void shmcb_inc_dd(void *loc, const void *value)
{
	double vi;
	struct SHMapDD *e = (struct SHMapDD *)loc;
	memcpy(&vi, value, sizeof(vi));
	e->v += vi;
}

static void shmcb_inc_sd(void *loc, const void *value)
{
	double vi;
	struct SHMapSD *e = (struct SHMapSD *)loc;
	memcpy(&vi, value, sizeof(vi));
	e->v += vi;
}

static void del_is(void *node)
{
	sso1_free((srt_stringo1 *)&((struct SHMapIS *)node)->v);
}

static void del_ds(void *node)
{
	sso1_free((srt_stringo1 *)&((struct SHMapDS *)node)->v);
}

static void del_sx(void *node)
{
	sso1_free(&((struct SHMapS *)node)->k);
}

static void del_ss(void *node)
{
	sso_free(&((struct SHMapSS *)node)->kv);
}

static void del_nop(void *node)
{
	(void)node;
}

static uint32_t hash_32(const void *node)
{
	return SHM_HASH_32(S_LD_U32(node));
}

static uint32_t hash_64(const void *node)
{
	return SHM_HASH_64(S_LD_U64(node));
}

static uint32_t hash_fp(const void *node)
{
	return SHM_HASH_F(S_LD_F(node));
}

static uint32_t hash_dfp(const void *node)
{
	return SHM_HASH_D(S_LD_D(node));
}

static uint32_t hash_s1(const void *node)
{
	return SHM_HASH_S(sso1_get((const srt_stringo1 *)node));
}

static uint32_t hash_ss(const void *node)
{
	return SHM_HASH_S(sso_get((const srt_stringo *)node));
}

static const void *n2key_direct(const void *node)
{
	return node;
}

static const void *n2key_s1(const void *node)
{
	return sso1_get((const srt_stringo1 *)node);
}

static const void *n2key_ss(const void *node)
{
	return sso_get((const srt_stringo *)node);
}

const struct SHMapCtx shm_ctx[SHM0_NumTypes] = {
	{eq_32, del_nop, hash_32, n2key_direct},  /*SHM0_II32*/
	{eq_32, del_nop, hash_32, n2key_direct},  /*SHM0_UU32*/
	{eq_64, del_nop, hash_64, n2key_direct},  /*SHM0_II*/
	{eq_64, del_is, hash_64, n2key_direct},   /*SHM0_IS*/
	{eq_64, del_nop, hash_64, n2key_direct},  /*SHM0_IP*/
	{eq_sso1, del_sx, hash_s1, n2key_s1},     /*SHM0_SI*/
	{eq_sso, del_ss, hash_ss, n2key_ss},      /*SHM0_SS*/
	{eq_sso1, del_sx, hash_s1, n2key_s1},     /*SHM0_SP*/
	{eq_32, del_nop, hash_32, n2key_direct},  /*SHM0_I32*/
	{eq_32, del_nop, hash_32, n2key_direct},  /*SHM0_U32*/
	{eq_64, del_nop, hash_64, n2key_direct},  /*SHM0_I*/
	{eq_sso1, del_sx, hash_s1, n2key_s1},     /*SHM0_S*/
	{eq_f, del_nop, hash_fp, n2key_direct},   /*SHM0_FF*/
	{eq_d, del_nop, hash_dfp, n2key_direct},  /*SHM0_DD*/
	{eq_d, del_ds, hash_dfp, n2key_direct},   /*SHM0_DS*/
	{eq_d, del_nop, hash_dfp, n2key_direct},  /*SHM0_DP*/
	{eq_sso1, del_sx, hash_s1, n2key_s1},     /*SHM0_SD*/
	{eq_f, del_nop, hash_fp, n2key_direct},   /*SHM0_F*/
	{eq_d, del_nop, hash_dfp, n2key_direct}}; /*SHM0_D*/

static void aux_reg_hash(srt_hmap *hm, const void *key, uint32_t h32,
			 uint32_t loc)
{
	uint8_t *eloc;
	size_t bid, l, hmask;
	struct SHMBucket *b = shm_get_buckets(hm);
	shm_eq_f eqf = shm_ctx[hm->d.sub_type].eqf;
	bid = h2bid(h32, hm->hbits);
	hmask = hm->hmask;
	for (l = bid; b[l].loc; l = (l + 1) & hmask)
		if (b[l].hash == h32) {
			eloc = shm_get_buffer(hm)
			       + (b[l].loc - 1) * hm->d.elem_size;
			if (eqf(key, eloc))
				goto skip_bucket_inc; /* found, overwrite */
		}
	b[bid].cnt++;
skip_bucket_inc:
	b[l].loc = loc + 1;
	b[l].hash = h32;
}

static void aux_rehash(srt_hmap *hm)
{
	shm_eloc_t_ i;
	shm_hash_f hashf;
	uint8_t *data = shm_get_buffer(hm);
	struct SHMBucket *b = shm_get_buckets(hm);
	size_t nbuckets, elem_size = hm->d.elem_size, nelems = shm_size(hm);
	uint64_t nb64 = (uint64_t)1 << hm->hbits;
	nbuckets = (size_t)nb64;
	S_ASSERT((uint64_t)nbuckets == nb64);
	hm->hmask = hm->hbits == 32 ? (uint32_t)-1 : (uint32_t)(nbuckets - 1);
	hm->rh_threshold = s_size_t_pct(nbuckets, hm->rh_threshold_pct);
	hashf = shm_ctx[hm->d.sub_type].hashf;
	/*
	 * Reset the hash table buckets, and rehash all elements
	 */
	memset(b, 0, sizeof(struct SHMBucket) * nbuckets);
	for (i = 0; i < nelems; i++, data += elem_size)
		aux_reg_hash(hm, data, hashf(data), i);
}

static srt_bool aux_insert_check(srt_hmap **hm)
{
	srt_hmap *h2;
	size_t h2bits, hs1, hs2, hsd, sxz, sxzm, sz;
	RETURN_IF(!shm_grow(hm, 1) || !hm || !*hm, S_FALSE);
	sz = shm_size(*hm);
	/* Check if rehash is not required */
	if (sz < (*hm)->rh_threshold)
		return S_TRUE;
	if ((*hm)->hbits == 32) {
		(*hm)->rh_threshold = SHM_MAX_ELEMS;
		RETURN_IF(sz == (*hm)->rh_threshold, S_FALSE);
		return S_TRUE;
	}
	/* Rehash required: realloc for twice the bucket size */
	if ((*hm)->d.f.ext_buffer) {
		S_ERROR("out of memory on fixed-size allocated space");
		shm_set_alloc_errors(*hm);
		return S_FALSE;
	}
	sxz = shm_size(*hm) * (*hm)->d.elem_size;
	sxzm = shm_max_size(*hm) * (*hm)->d.elem_size;
	hs1 = (*hm)->d.header_size;
	h2bits = (*hm)->hbits + 1;
	hs2 = sh_hdr_size((*hm)->d.sub_type, (uint64_t)1 << h2bits);
	h2 = (srt_hmap *)s_realloc(*hm, hs2 + sxzm);
	RETURN_IF(!h2, S_FALSE); /* Not enough memory */
	*hm = h2;
#if 1
	/*
	 * Memory map:
	 * | headerA | buckets #A |    data #A    |
	 * <-- hs0 -->            <----- sxzm ---->
	 * <--------- hs1 -------->
	 *
	 * | headerB | buckets #B = 2 * b#A |    data #B    |
	 * <-- hs0 -->                      <----- sxzm ---->
	 * <---------------- hs2 ----------->
	 *                        <-- ds1 -->
	 *                                  <-ds2->
	 *                                        <-- ds1'-->
	 */
	/* move ds1 from the head, to the tail */
	hsd = hs2 - hs1;
	if (sxz <= hsd)
		memmove((uint8_t *)h2 + hs2, (uint8_t *)h2 + hs1, sxz);
	else
		memmove((uint8_t *)h2 + hs1 + sxz, (uint8_t *)h2 + hs1, hsd);
#else
	/* not optimized: */
	memmove((uint8_t *)h2 + hs2, (uint8_t *)h2 + hs1, sxz);
#endif
	/* Reconfigure the data structure */
	h2->d.header_size = hs2;
	h2->hbits = (uint32_t)h2bits;
	/* Rehash elements */
	aux_rehash(h2);
	return S_TRUE;
}

S_INLINE srt_bool shm_chk_t(const srt_hmap *h, int t)
{
	return h && h->d.sub_type == t ? S_TRUE : S_FALSE;
}

typedef uint32_t (*hash_f)(const void *data);

/* 'hm' already checked externally */
const void *shm_at(const srt_hmap *hm, uint32_t h, const void *key,
		   uint32_t *tl)
{
	const uint8_t *data, *eloc;
	size_t bid = h2bid(h, hm->hbits), eoff, es, hbits, hcnt, hmask, hmax, l;
	const struct SHMBucket *b = shm_get_buckets_r(hm);
	shm_eq_f eqf = shm_ctx[hm->d.sub_type].eqf;
	RETURN_IF(!b[bid].cnt, NULL); /* Hash not in the HT */
	hmax = b[bid].cnt;
	data = shm_get_buffer_r(hm);
	es = hm->d.elem_size;
	hmask = hm->hmask;
	hbits = hm->hbits;
	for (hcnt = 0, l = bid; hcnt < hmax; l = (l + 1) & hmask) {
		if (b[l].loc == SHM_LOC_EMPTY || h2bid(b[l].hash, hbits) != bid)
			continue;
		/* Possible match */
		hcnt++;
		if (b[l].hash == h) {
			eoff = b[l].loc - 1;
			eloc = data + eoff * es;
			if (eqf(key, eloc)) {
				if (tl)
					*tl = (uint32_t)l;
				return eloc;
			}
		}
	}
	return NULL;
}

static srt_bool del(srt_hmap *hm, uint32_t h, const void *key)
{
	shm_eq_f eqf;
	shm_del_f delf;
	shm_hash_f hashf;
	shm_n2key_f n2kf;
	struct SHMBucket *b;
	const uint8_t *tloc;
	uint32_t ht, l0, tl = 0;
	size_t bid, e_off, es, hcnt, hmax, l, ss, hbits, hmask;
	uint8_t *data, *dloc, *hole, *tail;
	RETURN_IF(!hm || hm->d.sub_type >= SHM0_NumTypes, S_FALSE);
	delf = shm_ctx[hm->d.sub_type].delf;
	eqf = shm_ctx[hm->d.sub_type].eqf;
	hashf = shm_ctx[hm->d.sub_type].hashf;
	n2kf = shm_ctx[hm->d.sub_type].n2kf;
	hbits = hm->hbits;
	bid = h2bid(h, hbits);
	b = shm_get_buckets(hm);
	RETURN_IF(!b[bid].cnt, S_FALSE); /* Hash not in the HT */
	hmax = b[bid].cnt;
	data = shm_get_buffer(hm);
	es = hm->d.elem_size;
	hmask = hm->hmask;
	for (hcnt = 0, l = bid; hcnt < hmax; l = (l + 1) & hmask) {
		if (b[l].loc == SHM_LOC_EMPTY || h2bid(b[l].hash, hbits) != bid)
			continue;
		/* Possible match */
		hcnt++;
		if (b[l].hash == h) {
			e_off = b[l].loc - 1;
			dloc = data + e_off * es;
			if (eqf(key, dloc)) {
				l0 = b[l].loc;
				b[bid].cnt--;
				b[l].loc = 0;
				delf(dloc);
				/* Fill the hole with the latest elem */
				ss = shm_size(hm);
				if (ss > 1 && ss != l0) {
					hole = data + e_off * es;
					tail = data + (ss - 1) * es;
					ht = hashf(tail);
					tloc = (const uint8_t *)shm_at(
						hm, ht, n2kf(tail), &tl);
					(void)tloc;
#if 0
					/*
					 * This should never happen. Otherwise
					 * it would mean memory corruption.
					 */
					if (tloc != tail || b[tl].loc == l0)
						abort();
#endif
					memcpy(hole, tail, es);
					b[tl].loc = l0;
				}
				shm_set_size(hm, ss - 1);
				return S_TRUE;
			}
		}
	}
	return S_FALSE;
}

/*
 * Public functions
 */

srt_hmap *shm_alloc_raw(int t, srt_bool ext_buf, void *buffer, size_t hdr_size,
			size_t elem_size, size_t max_size, size_t hbits)
{
	srt_hmap *h;
	RETURN_IF(!elem_size || !buffer, shm_void);
	h = (srt_hmap *)buffer;
	sd_reset((srt_data *)h, hdr_size, elem_size, max_size, ext_buf,
		 S_FALSE);
	h->d.sub_type = (uint8_t)t;
	h->rh_threshold_pct = SHM_REHASH_DEFAULT_THRESHOLD_PCT;
	h->hbits = (uint32_t)hbits;
	aux_rehash(h);
	return h;
}

srt_hmap *shm_alloc_aux(int t, size_t init_size)
{
	size_t elem_size = shm_elem_size(t), hbits = shm_s2hb(init_size),
	       hs = sh_hdr_size(t, (uint64_t)1 << hbits),
	       as = sd_alloc_size_raw(hs, elem_size, init_size, S_FALSE);
	void *buf = s_malloc(as);
	srt_hmap *h =
		shm_alloc_raw(t, S_FALSE, buf, hs, elem_size, init_size, hbits);
	if (!h || h == shm_void)
		s_free(buf);
	return h;
}

void shm_clear(srt_hmap *hm)
{
	size_t es;
	shm_del_f delf;
	uint8_t *p, *pt, t;
	if (!hm)
		return;
	p = shm_get_buffer(hm);
	es = hm->d.elem_size;
	pt = p + shm_size(hm) * es;
	t = hm->d.sub_type;
	delf = t < SHM0_NumTypes ? shm_ctx[t].delf : NULL;
	if (delf && delf != del_nop)
		for (; p < pt; p += es)
			delf(p);
	shm_set_size(hm, 0);
}

void shm_free_aux(srt_hmap **hm, ...)
{
	va_list ap;
	srt_hmap **next = hm;
	va_start(ap, hm);
	while (!s_varg_tail_ptr_tag(next)) { /* last element tag */
		shm_clear(*next); /* release associated dyn. memory */
		sd_free((srt_data **)next);
		next = (srt_hmap **)va_arg(ap, srt_hmap **);
	}
	va_end(ap);
}

srt_hmap *shm_dup(const srt_hmap *src)
{
	srt_hmap *hm = NULL;
	return shm_cpy(&hm, src);
}

S_INLINE size_t shm_current_alloc_size(const srt_hmap *h)
{
	return h ? h->d.header_size + h->d.elem_size * shm_max_size(h) : 0;
}

static srt_bool shm_cpy_reconfig(srt_hmap **hm, const srt_hmap *src)
{
	srt_hmap *hra;
	uint8_t t = src->d.sub_type;
	uint64_t hs64 = snextpow2(shm_size(src));
	size_t tgt0_cas, src0_cas, np2, hbits, hdr_size, es, elems, data_size,
		min_alloc_size;
	np2 = (size_t)hs64;
	hbits = slog2(np2);
	RETURN_IF(!hm || (uint64_t)np2 != hs64, S_FALSE);
	tgt0_cas = shm_current_alloc_size(*hm);
	src0_cas = shm_current_alloc_size(src);
	hdr_size = sh_hdr_size(t, np2);
	es = src->d.elem_size;
	elems = shm_size(src);
	data_size = es * elems;
	min_alloc_size = hdr_size + data_size;
	/* Target cleanup, before the copy */
	shm_clear(*hm);
	/* Make room for the copy */
	if ((*hm)->d.f.ext_buffer) {
		/* Using stack-allocated: check for enough space */
		RETURN_IF(min_alloc_size > tgt0_cas, S_FALSE);
		(*hm)->d.sub_type = src->d.sub_type;
		(*hm)->d.elem_size = src->d.elem_size;
		(*hm)->d.max_size = (tgt0_cas - hdr_size) / es;
	} else {
		/* Check if requiring extra expace */
		if (min_alloc_size > tgt0_cas) {
			hra = (srt_hmap *)s_realloc(*hm, src0_cas);
			RETURN_IF(!hra, S_FALSE);
			*hm = hra;
			(*hm)->d.max_size = (src0_cas - hdr_size) / es;
		} else {
			(*hm)->d.max_size = (tgt0_cas - hdr_size) / es;
		}
		(*hm)->d.sub_type = src->d.sub_type;
		(*hm)->d.elem_size = src->d.elem_size;
	}
	(*hm)->d.header_size = hdr_size;
	(*hm)->hbits = (uint32_t)hbits;
	return S_TRUE;
}

srt_hmap *shm_cpy(srt_hmap **hm, const srt_hmap *src)
{
	uint8_t t;
	uint8_t *data_tgt;
	const uint8_t *data_src;
	size_t i, hs, hdr0_size, es, ss;
	struct SHMapS *h_s;
	struct SHMapIS *h_is;
	struct SHMapSI *h_si;
	struct SHMapSP *h_sp;
	struct SHMapSS *h_ss;
	struct SHMapDS *h_ds;
	struct SHMapSD *h_sd;
	RETURN_IF(!hm || !src, NULL); /* BEHAVIOR */
	RETURN_IF(*hm == src, *hm);
	t = src->d.sub_type;
	hs = shm_size(src);
	es = src->d.elem_size;
	ss = shm_size(src);
	RETURN_IF(hs > SHM_MAX_ELEMS, NULL); /* BEHAVIOR */
	if (*hm) {
		/* De-allocate target nodes, if necessary */
		RETURN_IF(!shm_cpy_reconfig(hm, src), NULL);
	} else {
		*hm = shm_alloc_aux(t, ss);
		RETURN_IF(!*hm, NULL); /* BEHAVIOR: allocation error */
	}
	RETURN_IF(shm_max_size(*hm) < ss, *hm); /* BEHAVIOR: not enough space */
	/* Copy data */
	data_tgt = shm_get_buffer(*hm);
	data_src = shm_get_buffer_r(src);
	/* bulk copy */
	memcpy(data_tgt, data_src, es * ss);
	shm_set_size(*hm, ss);
	/* cases potentially requiring adaptation */
	switch (t) {
	case SHM0_S:
		h_s = (struct SHMapS *)data_tgt;
		for (i = 0; i < ss; i++)
			sso_dupa1(&h_s[i].k);
		break;
	case SHM0_IS:
		h_is = (struct SHMapIS *)data_tgt;
		for (i = 0; i < ss; sso_dupa1(&h_is[i++].v))
			;
		break;
	case SHM0_SP:
		h_sp = (struct SHMapSP *)data_tgt;
		for (i = 0; i < ss; sso_dupa1(&h_sp[i++].x.k))
			;
		break;
	case SHM0_SI:
		h_si = (struct SHMapSI *)data_tgt;
		for (i = 0; i < ss; sso_dupa1(&h_si[i++].x.k))
			;
		break;
	case SHM0_SS:
		h_ss = (struct SHMapSS *)data_tgt;
		for (i = 0; i < ss; sso_dupa(&h_ss[i++].kv))
			;
		break;
	case SHM0_DS:
		h_ds = (struct SHMapDS *)data_tgt;
		for (i = 0; i < ss; sso_dupa1(&h_ds[i++].v))
			;
		break;
	case SHM0_SD:
		h_sd = (struct SHMapSD *)data_tgt;
		for (i = 0; i < ss; sso_dupa1(&h_sd[i++].x.k))
			;
		break;
	default:
		break;
	}
	/* rehash */
	if ((*hm)->d.header_size == src->d.header_size) {
		/* Same header size: hash table buckets bulk copy */
		hdr0_size = sh_hdr0_size();
		memcpy((uint8_t *)*hm + hdr0_size,
		       (const uint8_t *)src + hdr0_size,
		       src->d.header_size - hdr0_size);
	} else {
		/* Different bucket size, rehash required */
		aux_rehash(*hm);
	}
	return *hm;
}

/*
 * Insert
 */

typedef void (*shm_set1_f)(void *loc, const void *key);

static srt_bool shm_insert1(srt_hmap **hm, int t, const void *k, uint32_t h32,
			    shm_set1_f setf)
{
	void *l;
	size_t i;
	RETURN_IF(!hm || !*hm || !shm_chk_t(*hm, t), S_FALSE);
	RETURN_IF(!aux_insert_check(hm), S_FALSE);
	l = (void *)shm_at(*hm, h32, k, NULL);
	if (!l) {
		i = shm_size(*hm);
		aux_reg_hash(*hm, k, h32, (shm_eloc_t_)i);
		shm_set_size(*hm, i + 1);
		l = shm_get_buffer(*hm) + i * (*hm)->d.elem_size;
	}
	setf(l, k);
	return S_TRUE;
}

typedef void (*shm_set_f)(void *loc, const void *key, const void *value);

static srt_bool shm_insert(srt_hmap **hm, int t, const void *k, uint32_t h32,
			   const void *v, shm_set_f setf)
{
	void *l;
	size_t i;
	RETURN_IF(!hm || !*hm || !shm_chk_t(*hm, t), S_FALSE);
	RETURN_IF(!aux_insert_check(hm), S_FALSE);
	l = (void *)shm_at(*hm, h32, k, NULL);
	if (!l) {
		i = shm_size(*hm);
		aux_reg_hash(*hm, k, h32, (shm_eloc_t_)i);
		shm_set_size(*hm, i + 1);
		l = shm_get_buffer(*hm) + i * (*hm)->d.elem_size;
	}
	setf(l, k, v);
	return S_TRUE;
}

/*
 * Increment
 */

typedef void (*shm_inc_f)(void *loc, const void *value);

static srt_bool shm_inc(srt_hmap **hm, int t, const void *k, uint32_t h32,
			const void *v, shm_set_f setf, shm_inc_f incf)
{
	void *l;
	RETURN_IF(!hm || !*hm || !shm_chk_t(*hm, t), S_FALSE);
	RETURN_IF(!aux_insert_check(hm), S_FALSE);
	l = (void *)shm_at(*hm, h32, k, NULL);
	if (!l) /* not found: create new elem */
		return shm_insert(hm, t, k, h32, v, setf);
	incf(l, v);
	return S_TRUE;
}

/*
 * Insert
 */

srt_bool shm_insert_ii32(srt_hmap **hm, int32_t k, int32_t v)
{
	return shm_insert(hm, SHM0_II32, &k, SHM_HASH_32(k), &v,
			  shmcb_set_ii32);
}

srt_bool shm_insert_uu32(srt_hmap **hm, uint32_t k, uint32_t v)
{
	return shm_insert(hm, SHM0_UU32, &k, SHM_HASH_32(k), &v,
			  shmcb_set_uu32);
}

srt_bool shm_insert_ii(srt_hmap **hm, int64_t k, int64_t v)
{
	return shm_insert(hm, SHM0_II, &k, SHM_HASH_64(k), &v, shmcb_set_ii64);
}

srt_bool shm_insert_is(srt_hmap **hm, int64_t k, const srt_string *v)
{
	return shm_insert(hm, SHM0_IS, &k, SHM_HASH_64(k), v, shmcb_set_is);
}

srt_bool shm_insert_ip(srt_hmap **hm, int64_t k, const void *v)
{
	return shm_insert(hm, SHM0_IP, &k, SHM_HASH_64(k), v, shmcb_set_ip);
}

srt_bool shm_insert_si(srt_hmap **hm, const srt_string *k, int64_t v)
{
	return shm_insert(hm, SHM0_SI, k, SHM_HASH_S(k), &v, shmcb_set_si);
}

srt_bool shm_insert_ss(srt_hmap **hm, const srt_string *k, const srt_string *v)
{
	return shm_insert(hm, SHM0_SS, k, SHM_HASH_S(k), v, shmcb_set_ss);
}

srt_bool shm_insert_sp(srt_hmap **hm, const srt_string *k, const void *v)
{
	return shm_insert(hm, SHM0_SP, k, SHM_HASH_S(k), v, shmcb_set_sp);
}

srt_bool shm_insert_ff(srt_hmap **hm, float k, float v)
{
	return shm_insert(hm, SHM0_FF, &k, SHM_HASH_F(k), &v, shmcb_set_ff);
}

srt_bool shm_insert_dd(srt_hmap **hm, double k, double v)
{
	return shm_insert(hm, SHM0_DD, &k, SHM_HASH_D(k), &v, shmcb_set_dd);
}

srt_bool shm_insert_ds(srt_hmap **hm, double k, const srt_string *v)
{
	return shm_insert(hm, SHM0_DS, &k, SHM_HASH_D(k), v, shmcb_set_ds);
}

srt_bool shm_insert_dp(srt_hmap **hm, double k, const void *v)
{
	return shm_insert(hm, SHM0_DP, &k, SHM_HASH_D(k), v, shmcb_set_dp);
}

srt_bool shm_insert_sd(srt_hmap **hm, const srt_string *k, double v)
{
	return shm_insert(hm, SHM0_SD, k, SHM_HASH_S(k), &v, shmcb_set_sd);
}

/*
 * Increment
 */

srt_bool shm_inc_ii32(srt_hmap **hm, int32_t k, int32_t v)
{
	return shm_inc(hm, SHM0_II32, &k, SHM_HASH_32(k), &v, shmcb_set_ii32,
		       shmcb_inc_ii32);
}

srt_bool shm_inc_uu32(srt_hmap **hm, uint32_t k, uint32_t v)
{
	return shm_inc(hm, SHM0_UU32, &k, SHM_HASH_32(k), &v, shmcb_set_uu32,
		       shmcb_inc_uu32);
}

srt_bool shm_inc_ii(srt_hmap **hm, int64_t k, int64_t v)
{
	return shm_inc(hm, SHM0_II, &k, SHM_HASH_64(k), &v, shmcb_set_ii64,
		       shmcb_inc_ii64);
}

srt_bool shm_inc_si(srt_hmap **hm, const srt_string *k, int64_t v)
{
	return shm_inc(hm, SHM0_SI, k, SHM_HASH_S(k), &v, shmcb_set_si,
		       shmcb_inc_si);
}

srt_bool shm_inc_ff(srt_hmap **hm, float k, float v)
{
	return shm_inc(hm, SHM0_FF, &k, SHM_HASH_F(k), &v, shmcb_set_ff,
		       shmcb_inc_ff);
}

srt_bool shm_inc_dd(srt_hmap **hm, double k, double v)
{
	return shm_inc(hm, SHM0_DD, &k, SHM_HASH_D(k), &v, shmcb_set_dd,
		       shmcb_inc_dd);
}

srt_bool shm_inc_sd(srt_hmap **hm, const srt_string *k, double v)
{
	return shm_inc(hm, SHM0_SD, k, SHM_HASH_S(k), &v, shmcb_set_sd,
		       shmcb_inc_sd);
}

/*
 * Insertion
 */

srt_bool shm_insert_i32(srt_hmap **hm, int32_t k)
{
	return shm_insert1(hm, SHM0_I32, &k, SHM_HASH_32(k), shmcb_set_i32);
}

srt_bool shm_insert_u32(srt_hmap **hm, uint32_t k)
{
	return shm_insert1(hm, SHM0_U32, &k, SHM_HASH_32(k), shmcb_set_u32);
}

srt_bool shm_insert_i(srt_hmap **hm, int64_t k)
{
	return shm_insert1(hm, SHM0_I, &k, SHM_HASH_64(k), shmcb_set_i64);
}

srt_bool shm_insert_s(srt_hmap **hm, const srt_string *k)
{
	return shm_insert1(hm, SHM0_S, k, SHM_HASH_S(k), shmcb_set_s);
}

srt_bool shm_insert_f(srt_hmap **hm, float k)
{
	return shm_insert1(hm, SHM0_F, &k, SHM_HASH_F(k), shmcb_set_f);
}

srt_bool shm_insert_d(srt_hmap **hm, double k)
{
	return shm_insert1(hm, SHM0_D, &k, SHM_HASH_D(k), shmcb_set_d);
}

/*
 * Delete
 */

srt_bool shm_delete_i32(srt_hmap *hm, int32_t k)
{
	return del(hm, SHM_HASH_32(k), &k);
}

srt_bool shm_delete_u32(srt_hmap *hm, uint32_t k)
{
	return del(hm, SHM_HASH_32(k), &k);
}

srt_bool shm_delete_i(srt_hmap *hm, int64_t k)
{
	return del(hm, SHM_HASH_64(k), &k);
}

srt_bool shm_delete_f(srt_hmap *hm, float k)
{
	return del(hm, SHM_HASH_F(k), &k);
}

srt_bool shm_delete_d(srt_hmap *hm, double k)
{
	return del(hm, SHM_HASH_D(k), &k);
}

srt_bool shm_delete_s(srt_hmap *hm, const srt_string *k)
{
	return del(hm, SHM_HASH_S(k), k);
}

	/*
	 * Enumeration
	 */

#define BUILD_SHM_ITP_X(FN, TID, TS, ITF, COND)                                \
	size_t FN(const srt_hmap *hm, size_t begin, size_t end, ITF f,         \
		  void *context)                                               \
	{                                                                      \
		const TS *e;                                                   \
		size_t cnt = 0, ms;                                            \
		const uint8_t *d0, *db, *de;                                   \
		RETURN_IF(!hm || TID != hm->d.sub_type, 0);                    \
		ms = shm_size(hm);                                             \
		RETURN_IF(begin > ms || begin >= end, 0);                      \
		if (end > ms)                                                  \
			end = ms;                                              \
		RETURN_IF(!f, end - begin);                                    \
		d0 = shm_get_buffer_r(hm);                                     \
		db = d0 + hm->d.elem_size * begin;                             \
		de = d0 + hm->d.elem_size * end;                               \
		for (; db < de; db += hm->d.elem_size, cnt++) {                \
			e = (const TS *)db;                                    \
			if (!(COND))                                           \
				break;                                         \
		}                                                              \
		return cnt;                                                    \
	}
BUILD_SHM_ITP_X(shm_itp_ii32, SHM_II32, struct SHMapii, srt_hmap_it_ii32,
		f(e->x.k, e->v, context))
BUILD_SHM_ITP_X(shm_itp_uu32, SHM_UU32, struct SHMapuu, srt_hmap_it_uu32,
		f(e->x.k, e->v, context))
BUILD_SHM_ITP_X(shm_itp_ii, SHM_II, struct SHMapII, srt_hmap_it_ii,
		f(e->x.k, e->v, context))
BUILD_SHM_ITP_X(shm_itp_ff, SHM_FF, struct SHMapFF, srt_hmap_it_ff,
		f(e->x.k, e->v, context))
BUILD_SHM_ITP_X(shm_itp_dd, SHM_DD, struct SHMapDD, srt_hmap_it_dd,
		f(e->x.k, e->v, context))
BUILD_SHM_ITP_X(shm_itp_is, SHM_IS, struct SHMapIS, srt_hmap_it_is,
		f(e->x.k, sso1_get((const srt_stringo1 *)&e->v), context))
BUILD_SHM_ITP_X(shm_itp_ip, SHM_IP, struct SHMapIP, srt_hmap_it_ip,
		f(e->x.k, e->v, context))
BUILD_SHM_ITP_X(shm_itp_si, SHM_SI, struct SHMapSI, srt_hmap_it_si,
		f(sso1_get((const srt_stringo1 *)&e->x.k), e->v, context))
BUILD_SHM_ITP_X(shm_itp_ds, SHM_DS, struct SHMapDS, srt_hmap_it_ds,
		f(e->x.k, sso1_get((const srt_stringo1 *)&e->v), context))
BUILD_SHM_ITP_X(shm_itp_dp, SHM_DP, struct SHMapDP, srt_hmap_it_dp,
		f(e->x.k, e->v, context))
BUILD_SHM_ITP_X(shm_itp_sd, SHM_SD, struct SHMapSD, srt_hmap_it_sd,
		f(sso1_get((const srt_stringo1 *)&e->x.k), e->v, context))
BUILD_SHM_ITP_X(shm_itp_ss, SHM_SS, struct SHMapSS, srt_hmap_it_ss,
		f(sso_get(&e->kv), sso_get_s2(&e->kv), context))
BUILD_SHM_ITP_X(shm_itp_sp, SHM_SP, struct SHMapSP, srt_hmap_it_sp,
		f(sso1_get((const srt_stringo1 *)&e->x.k), e->v, context))

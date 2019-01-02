/*
 * shmap.c
 *
 * Hash map handling
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "shmap.h"
#include "saux/shash.h"
#include "saux/sstringo.h"

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

static srt_bool eq_sso(const void *key, const void *node)
{
	return sso_eq((const srt_string *)key, (const srt_stringo *)node);
}

static srt_bool eq_sso1(const void *key, const void *node)
{
	return sso1_eq((const srt_string *)key, (const srt_stringo1 *)node);
}

void shmcb_set_ii32(void *loc, const void *key, const void *value)
{
	struct SHMapii *e = (struct SHMapii *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	memcpy(&e->v, value, sizeof(e->v));
}

void shmcb_set_uu32(void *loc, const void *key, const void *value)
{
	struct SHMapuu *e = (struct SHMapuu *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	memcpy(&e->v, value, sizeof(e->v));
}

void shmcb_set_ii64(void *loc, const void *key, const void *value)
{
	struct SHMapII *e = (struct SHMapII *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	memcpy(&e->v, value, sizeof(e->v));
}

void shmcb_set_is(void *loc, const void *key, const void *value)
{
	struct SHMapIS *e = (struct SHMapIS *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	sso1_set(&e->v, (const srt_string *)value);
}

void shmcb_set_ip(void *loc, const void *key, const void *value)
{
	struct SHMapIP *e = (struct SHMapIP *)loc;
	memcpy(&e->x.k, key, sizeof(e->x.k));
	e->v = value;
}

void shmcb_set_si(void *loc, const void *key, const void *value)
{
	struct SHMapSI *e = (struct SHMapSI *)loc;
	sso1_set(&e->x.k, (const srt_string *)key);
	memcpy(&e->v, value, sizeof(e->v));
}

void shmcb_set_ss(void *loc, const void *key, const void *value)
{
	struct SHMapSS *e = (struct SHMapSS *)loc;
	sso_set(&e->kv, (const srt_string *)key, (const srt_string *)value);
}

void shmcb_set_sp(void *loc, const void *key, const void *value)
{
	struct SHMapSP *e = (struct SHMapSP *)loc;
	sso1_set(&e->x.k, (const srt_string *)key);
	e->v = value;
}

void shmcb_set_i32(void *loc, const void *key)
{
	struct SHMapi *e = (struct SHMapi *)loc;
	memcpy(&e->k, key, sizeof(e->k));
}

void shmcb_set_u32(void *loc, const void *key)
{
	struct SHMapu *e = (struct SHMapu *)loc;
	memcpy(&e->k, key, sizeof(e->k));
}

void shmcb_set_i64(void *loc, const void *key)
{
	struct SHMapI *e = (struct SHMapI *)loc;
	memcpy(&e->k, key, sizeof(e->k));
}

void shmcb_set_s(void *loc, const void *key)
{
	struct SHMapS *e = (struct SHMapS *)loc;
	sso1_set(&e->k, (const srt_string *)key);
}

void shmcb_inc_ii32(void *loc, const void *value)
{
	int32_t vi;
	struct SHMapii *e = (struct SHMapii *)loc;
	memcpy(&vi, value, sizeof(vi));
	e->v += vi;
}

void shmcb_inc_uu32(void *loc, const void *value)
{
	uint32_t vi;
	struct SHMapuu *e = (struct SHMapuu *)loc;
	memcpy(&vi, value, sizeof(vi));
	e->v += vi;
}

void shmcb_inc_ii64(void *loc, const void *value)
{
	int64_t vi;
	struct SHMapII *e = (struct SHMapII *)loc;
	memcpy(&vi, value, sizeof(vi));
	e->v += vi;
}

void shmcb_inc_si(void *loc, const void *value)
{
	int64_t vi;
	struct SHMapSI *e = (struct SHMapSI *)loc;
	memcpy(&vi, value, sizeof(vi));
	e->v += vi;
}

static void del_is(void *node)
{
	sso1_free((srt_stringo1 *)&((struct SHMapIS *)node)->v);
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
	return sh_hash32(S_LD_U32(node));
}

static uint32_t hash_64(const void *node)
{
	return sh_hash64(S_LD_U64(node));
}

static uint32_t hash_sx(const void *node)
{
	return SHM_SHASH(sso1_get((const srt_stringo1 *)node));
}

static uint32_t hash_ss(const void *node)
{
	return SHM_SHASH(sso_get((const srt_stringo *)node));
}

static const void *n2key_direct(const void *node)
{
	return node;
}

static const void *n2key_sx(const void *node)
{
	return sso1_get((const srt_stringo1 *)node);
}

static const void *n2key_ss(const void *node)
{
	return sso_get((const srt_stringo *)node);
}

static void aux_reg_hash(srt_hmap *hm, const void *key, uint32_t h32,
			 uint32_t loc)
{
	uint8_t *eloc;
	size_t bid, l, hmask;
	struct SHMBucket *b = shm_get_buckets(hm);
	bid = h2bid(h32, hm->hbits);
	hmask = hm->hmask;
	for (l = bid; b[l].loc; l = (l + 1) & hmask)
		if (b[l].hash == h32) {
			eloc = shm_get_buffer(hm)
			       + (b[l].loc - 1) * hm->d.elem_size;
			if (hm->eqf(key, eloc))
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
	const srt_string *s;
	uint8_t *data = shm_get_buffer(hm);
	struct SHMBucket *b = shm_get_buckets(hm);
	size_t nbuckets, elem_size = hm->d.elem_size, nelems = shm_size(hm);
	nbuckets = 1 << hm->hbits;
	hm->hmask = nbuckets - 1;
	hm->rh_threshold = s_size_t_pct(nbuckets, hm->rh_threshold_pct);
	/*
	 * Reset the hash table buckets, and rehash all elements
	 */
	memset(b, 0, sizeof(struct SHMBucket) * nbuckets);
	switch (hm->ksize) {
	case 4:
		for (i = 0; i < nelems; i++, data += elem_size)
			aux_reg_hash(hm, data, sh_hash32(S_LD_U32(data)), i);
		break;
	case 8:
		for (i = 0; i < nelems; i++, data += elem_size)
			aux_reg_hash(hm, data, sh_hash64(S_LD_U64(data)), i);
		break;
	case 0:
		for (i = 0; i < nelems; i++, data += elem_size) {
			s = sso_get((const srt_stringo *)data);
			aux_reg_hash(hm, s, SHM_SHASH(s), i);
		}
		break;
	default:
		/* this should not happen */
		return;
	}
}

static srt_bool aux_insert_check(srt_hmap **hm)
{
	srt_hmap *h2;
	size_t hs1, hs2, hsd, h2bits, sxz, sxzm, sz;
	RETURN_IF(!shm_grow(hm, 1), S_FALSE);
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
	hs2 = sh_hdr_size((*hm)->d.sub_type, 1 << h2bits);
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
	h2->hbits = h2bits;
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
	RETURN_IF(!b[bid].cnt, NULL); /* Hash not in the HT */
	hmax = b[bid].cnt;
	data = shm_get_buffer_r(hm);
	es = hm->d.elem_size;
	hmask = hm->hmask;
	hbits = hm->hbits;
	for (hcnt = 0, l = bid; hcnt < hmax; l = (l + 1) & hmask) {
		if (!b[l].loc || h2bid(b[l].hash, hbits) != bid)
			continue;
		/* Possible match */
		hcnt++;
		if (b[l].hash == h) {
			eoff = b[l].loc - 1;
			eloc = data + eoff * es;
			if (hm->eqf(key, eloc)) {
				if (tl)
					*tl = l;
				return eloc;
			}
		}
	}
	return NULL;
}

static srt_bool del(srt_hmap *hm, uint32_t h, const void *key)
{
	struct SHMBucket *b;
	uint32_t ht, l0, tl;
	size_t bid, e_off, es, hcnt, hmax, l, ss, hbits, hmask;
	uint8_t *data, *dloc, *hole, *tail, *tloc;
	RETURN_IF(!hm, S_FALSE);
	hbits = hm->hbits;
	bid = h2bid(h, hbits);
	b = shm_get_buckets(hm);
	RETURN_IF(!b[bid].cnt, S_FALSE); /* Hash not in the HT */
	hmax = b[bid].cnt;
	data = shm_get_buffer(hm);
	es = hm->d.elem_size;
	hmask = hm->hmask;
	for (hcnt = 0, l = bid; hcnt < hmax; l = (l + 1) & hmask) {
		if (!b[l].loc || h2bid(b[l].hash, hbits) != bid)
			continue;
		/* Possible match */
		hcnt++;
		if (b[l].hash == h) {
			e_off = b[l].loc - 1;
			dloc = data + e_off * es;
			if (hm->eqf(key, dloc)) {
				l0 = b[l].loc;
				b[bid].cnt--;
				b[l].loc = 0;
				hm->delf(dloc);
				/* Fill the hole with the latest elem */
				ss = shm_size(hm);
				if (ss > 1 && ss != l0) {
					hole = data + e_off * es;
					tail = data + (ss - 1) * es;
					ht = hm->hashf(tail);
					tloc = (uint8_t *)shm_at(
						hm, ht, hm->n2kf(tail), &tl);
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

S_INLINE void shm_tsetup(srt_hmap *h, int t)
{
	switch (t) {
	case SHM0_I32:
	case SHM0_U32:
	case SHM0_II32:
	case SHM0_UU32:
		h->ksize = 4;
		h->eqf = eq_32;
		h->delf = del_nop;
		h->hashf = hash_32;
		h->n2kf = n2key_direct;
		break;
	case SHM0_I:
	case SHM0_II:
	case SHM0_IS:
	case SHM0_IP:
		h->ksize = 8;
		h->eqf = eq_64;
		h->delf = t == SHM0_IS ? del_is : del_nop;
		h->hashf = hash_64;
		h->n2kf = n2key_direct;
		break;
	case SHM0_SI:
	case SHM0_SP:
	case SHM0_S:
		h->ksize = 0;
		h->eqf = eq_sso1;
		h->delf = del_sx;
		h->hashf = hash_sx;
		h->n2kf = n2key_sx;
		break;
	case SHM0_SS:
		h->ksize = 0;
		h->eqf = eq_sso;
		h->delf = del_ss;
		h->hashf = hash_ss;
		h->n2kf = n2key_ss;
		break;
	default:
		break;
	}
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
	h->d.sub_type = t;
	shm_tsetup(h, t);
	h->rh_threshold_pct = SHM_REHASH_DEFAULT_THRESHOLD_PCT;
	h->hbits = hbits;
	aux_rehash(h);
	return h;
}

srt_hmap *shm_alloc_aux(int t, size_t init_size)
{
	size_t elem_size = shm_elem_size(t), hbits = shm_s2hb(init_size),
	       hs = sh_hdr_size(t, 1 << hbits),
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
	uint8_t *p, *pt;
	if (!hm)
		return;
	p = shm_get_buffer(hm);
	es = hm->d.elem_size;
	pt = p + shm_size(hm) * es;
	switch (hm->d.sub_type) {
	case SHM0_SI:
	case SHM0_IS:
	case SHM0_SP:
	case SHM0_SS:
	case SHM0_S:
		for (; p < pt; p += es)
			hm->delf(p);
		break;
	}
	shm_set_size(hm, 0);
}

void shm_free_aux(srt_hmap **hm, ...)
{
	va_list ap;
	va_start(ap, hm);
	srt_hmap **next = hm;
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
	enum eSHM_Type0 t = (enum eSHM_Type0)src->d.sub_type;
	size_t tgt0_cas = shm_current_alloc_size(*hm),
	       src0_cas = shm_current_alloc_size(src),
	       np2 = snextpow2(shm_size(src)), hbits = slog2(np2),
	       hdr_size = sh_hdr_size(t, np2), es = src->d.elem_size,
	       elems = shm_size(src), data_size = es * elems,
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
	(*hm)->hbits = hbits;
	(*hm)->eqf = src->eqf;
	(*hm)->delf = src->delf;
	(*hm)->n2kf = src->n2kf;
	(*hm)->ksize = src->ksize;
	return S_TRUE;
}

srt_hmap *shm_cpy(srt_hmap **hm, const srt_hmap *src)
{
	int t;
	uint8_t *data_tgt;
	const uint8_t *data_src;
	size_t i, hs, hdr0_size, es, ss;
	struct SHMapS *h_s;
	struct SHMapIS *h_is;
	struct SHMapSI *h_si;
	struct SHMapSP *h_sp;
	struct SHMapSS *h_ss;
	RETURN_IF(!hm || !src, NULL); /* BEHAVIOR */
	RETURN_IF(*hm == src, *hm);
	t = (enum eSHM_Type0)src->d.sub_type;
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
	default:
		break;
	}
	/* rehash */
	if ((*hm)->d.header_size == src->d.header_size) {
		/* Same header size: hash table buckets bulk copy */
		hdr0_size = sh_hdr0_size();
		memcpy((uint8_t *)*hm + hdr0_size, (uint8_t *)src + hdr0_size,
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
	uint32_t i;
	RETURN_IF(!hm || !*hm || !shm_chk_t(*hm, t), S_FALSE);
	RETURN_IF(!aux_insert_check(hm), S_FALSE);
	l = (void *)shm_at(*hm, h32, k, NULL);
	if (!l) {
		i = shm_size(*hm);
		aux_reg_hash(*hm, k, h32, i);
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
	uint32_t i;
	RETURN_IF(!hm || !*hm || !shm_chk_t(*hm, t), S_FALSE);
	RETURN_IF(!aux_insert_check(hm), S_FALSE);
	l = (void *)shm_at(*hm, h32, k, NULL);
	if (!l) {
		i = shm_size(*hm);
		aux_reg_hash(*hm, k, h32, i);
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
	return shm_insert(hm, SHM0_II32, &k, sh_hash32((uint32_t)k), &v,
			  shmcb_set_ii32);
}

srt_bool shm_insert_uu32(srt_hmap **hm, uint32_t k, uint32_t v)
{
	return shm_insert(hm, SHM0_UU32, &k, sh_hash32(k), &v, shmcb_set_uu32);
}

srt_bool shm_insert_ii(srt_hmap **hm, int64_t k, int64_t v)
{
	return shm_insert(hm, SHM0_II, &k, sh_hash64((uint64_t)k), &v,
			  shmcb_set_ii64);
}

srt_bool shm_insert_is(srt_hmap **hm, int64_t k, const srt_string *v)
{
	return shm_insert(hm, SHM0_IS, &k, sh_hash64((uint64_t)k), v,
			  shmcb_set_is);
}

srt_bool shm_insert_ip(srt_hmap **hm, int64_t k, const void *v)
{
	return shm_insert(hm, SHM0_IP, &k, sh_hash64((uint64_t)k), v,
			  shmcb_set_ip);
}

srt_bool shm_insert_si(srt_hmap **hm, const srt_string *k, int64_t v)
{
	return shm_insert(hm, SHM0_SI, k, SHM_SHASH(k), &v, shmcb_set_si);
}

srt_bool shm_insert_ss(srt_hmap **hm, const srt_string *k, const srt_string *v)
{
	return shm_insert(hm, SHM0_SS, k, SHM_SHASH(k), v, shmcb_set_ss);
}

srt_bool shm_insert_sp(srt_hmap **hm, const srt_string *k, const void *v)
{
	return shm_insert(hm, SHM0_SP, k, SHM_SHASH(k), v, shmcb_set_sp);
}

/*
 * Increment
 */

srt_bool shm_inc_ii32(srt_hmap **hm, int32_t k, int32_t v)
{
	return shm_inc(hm, SHM0_II32, &k, sh_hash32((uint32_t)k), &v,
		       shmcb_set_ii32, shmcb_inc_ii32);
}

srt_bool shm_inc_uu32(srt_hmap **hm, uint32_t k, uint32_t v)
{
	return shm_inc(hm, SHM0_UU32, &k, sh_hash32(k), &v, shmcb_set_uu32,
		       shmcb_inc_uu32);
}

srt_bool shm_inc_ii(srt_hmap **hm, int64_t k, int64_t v)
{
	return shm_inc(hm, SHM0_II, &k, sh_hash64((uint64_t)k), &v,
		       shmcb_set_ii64, shmcb_inc_ii64);
}

srt_bool shm_inc_si(srt_hmap **hm, const srt_string *k, int64_t v)
{
	return shm_inc(hm, SHM0_SI, k, SHM_SHASH(k), &v, shmcb_set_si,
		       shmcb_inc_si);
}

srt_bool shm_insert_i32(srt_hmap **hm, int32_t k)
{
	return shm_insert1(hm, SHM0_I32, &k, sh_hash32((uint32_t)k),
			   shmcb_set_i32);
}

srt_bool shm_insert_u32(srt_hmap **hm, uint32_t k)
{
	return shm_insert1(hm, SHM0_U32, &k, sh_hash32((uint32_t)k),
			   shmcb_set_u32);
}

srt_bool shm_insert_i(srt_hmap **hm, int64_t k)
{
	return shm_insert1(hm, SHM0_I, &k, sh_hash64((uint64_t)k),
			   shmcb_set_i64);
}

srt_bool shm_insert_s(srt_hmap **hm, const srt_string *k)
{
	return shm_insert1(hm, SHM0_S, k, SHM_SHASH(k), shmcb_set_s);
}

/*
 * Delete
 */

srt_bool shm_delete_i(srt_hmap *hm, int64_t k)
{
	uint32_t k32;
	if (hm->ksize == 4) {
		k32 = (uint32_t)k;
		return del(hm, sh_hash32(k32), &k32);
	}
	return del(hm, sh_hash64(k), &k);
}

srt_bool shm_delete_s(srt_hmap *hm, const srt_string *k)
{
	return del(hm, SHM_SHASH(k), k);
}

	/*
	 * Enumeration
	 */

#define SHM_ITP_X(t, hm, f, begin, end)                                        \
	size_t cnt = 0, ms;                                                    \
	const uint8_t *d0, *db, *de;                                           \
	RETURN_IF(!hm || t != hm->d.sub_type, 0);                              \
	ms = shm_size(hm);                                                     \
	RETURN_IF(begin > ms || begin >= end, 0);                              \
	if (end > ms)                                                          \
		end = ms;                                                      \
	RETURN_IF(!f, end - begin);                                            \
	d0 = shm_get_buffer_r(hm);                                             \
	db = d0 + hm->d.elem_size * begin;                                     \
	de = d0 + hm->d.elem_size * end;                                       \
	for (; db < de; db += hm->d.elem_size, cnt++)

size_t shm_itp_ii32(const srt_hmap *m, size_t begin, size_t end,
		    srt_hmap_it_ii32 f, void *context)
{
	const struct SHMapii *e;
	SHM_ITP_X(SHM_II32, m, f, begin, end)
	{
		e = (const struct SHMapii *)db;
		if (!f(e->x.k, e->v, context))
			break;
	}
	return cnt;
}

size_t shm_itp_uu32(const srt_hmap *m, size_t begin, size_t end,
		    srt_hmap_it_uu32 f, void *context)
{
	const struct SHMapuu *e;
	SHM_ITP_X(SHM_UU32, m, f, begin, end)
	{
		e = (const struct SHMapuu *)db;
		if (!f(e->x.k, e->v, context))
			break;
	}
	return cnt;
}

size_t shm_itp_ii(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_ii f,
		  void *context)
{
	const struct SHMapii *e;
	SHM_ITP_X(SHM_II, m, f, begin, end)
	{
		e = (const struct SHMapii *)db;
		if (!f(e->x.k, e->v, context))
			break;
	}
	return cnt;
}

size_t shm_itp_is(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_is f,
		  void *context)
{
	const struct SHMapIS *e;
	SHM_ITP_X(SHM_IS, m, f, begin, end)
	{
		e = (const struct SHMapIS *)db;
		if (!f(e->x.k, sso1_get((const srt_stringo1 *)&e->v), context))
			break;
	}
	return cnt;
}

size_t shm_itp_ip(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_ip f,
		  void *context)
{
	const struct SHMapIP *e;
	SHM_ITP_X(SHM_IP, m, f, begin, end)
	{
		e = (const struct SHMapIP *)db;
		if (!f(e->x.k, e->v, context))
			break;
	}
	return cnt;
}

size_t shm_itp_si(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_si f,
		  void *context)
{
	const struct SHMapSI *e;
	SHM_ITP_X(SHM_SI, m, f, begin, end)
	{
		e = (const struct SHMapSI *)db;
		if (!f(sso1_get((const srt_stringo1 *)&e->x.k), e->v, context))
			break;
	}
	return cnt;
}

size_t shm_itp_ss(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_ss f,
		  void *context)
{
	const struct SHMapSS *e;
	SHM_ITP_X(SHM_SS, m, f, begin, end)
	{
		e = (const struct SHMapSS *)db;
		if (!f(sso_get(&e->kv), sso_get_s2(&e->kv), context))
			break;
	}
	return cnt;
}

size_t shm_itp_sp(const srt_hmap *m, size_t begin, size_t end, srt_hmap_it_sp f,
		  void *context)
{
	const struct SHMapSP *e;
	SHM_ITP_X(SHM_SP, m, f, begin, end)
	{
		e = (const struct SHMapSP *)db;
		if (!f(sso1_get((const srt_stringo1 *)&e->x.k), e->v, context))
			break;
	}
	return cnt;
}

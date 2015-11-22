/*
 * ifilters.h
 *
 * Image filters (example for libsrt)
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#ifndef IFILTERS_H
#define IFILTERS_H

#include "../src/libsrt.h"
#include "rgbi.h"

/* Constants
 */

static const char *z8 = "\x00\x00\x00\x00\x00\x00\x00";

/* Paeth predictor
 * a: left, b: up, c = up-left
 */
S_INLINE int paeth_predictor(int a, int b, int c)
{
	int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
	return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}

#define DPCM_ENC(sp, s) ((sp) - (s))
#define DPCM_DEC(sp, s) ((sp) + (s))
#define DXOR_ENC(sp, s) ((sp) ^ (s))
#define DXOR_DEC(sp, s) ((sp) ^ (s))
#define DAVG3_ENC(sp, a, b, c, T) ((sp) - ((a) + (b) + (c)) / 3)
#define DAVG3_DEC(sp, a, b, c, T) ((sp) + ((a) + (b) + (c)) / 3)
#define DPAETH_ENC(sp, a, b, c, T) ((sp) - (T)paeth_predictor(a, b, c))
#define DPAETH_DEC(sp, a, b, c, T) ((sp) + (T)paeth_predictor(a, b, c))
#define XRSUB_ENC(t, s, a)	\
	(t)[0] = (s)[0], (t)[1] = (s)[1] - (s)[0], (t)[2] = (s)[2] - (s)[0]
#define XRSUB_DEC(t, s, a)	\
	(t)[0] = (s)[0], (t)[1] = (s)[1] + (s)[0], (t)[2] = (s)[2] + (s)[0]
#define XGSUB_ENC(t, s, a)	\
	(t)[0] = (s)[0] - (s)[1], (t)[1] = (s)[1], (t)[2] = (s)[2] - (s)[1]
#define XGSUB_DEC(t, s, a)	\
	(t)[0] = (s)[0] + (s)[1], (t)[1] = (s)[1], (t)[2] = (s)[2] + (s)[1]
#define XBSUB_ENC(t, s, a)	\
	(t)[0] = (s)[0] - (s)[2], (t)[1] = (s)[1] - (s)[2], (t)[2] = (s)[2]
#define XBSUB_DEC(t, s, a)	\
	(t)[0] = (s)[0] + (s)[2], (t)[1] = (s)[1] + (s)[2], (t)[2] = (s)[2]

#define ALG_COMMON(p)					\
	RETURN_IF(!rgb || !ri,0);			\
	size_t i, bs = ri->bmp_size, rs = ri->row_size;	\
	RETURN_IF(ss_reserve(dpcm, bs) < bs, 0);	\
	ss_set_size(*dpcm, bs);				\
	const char *s = ss_get_buffer_r(rgb);		\
	char *t = ss_get_buffer(*dpcm);			\
	const short *sw = (const short *)s;		\
	short *tw = (short *)t, *pw = (short *)p;	\

#define HDALG_LOOP(ps, i, s, t, p, OP, T)				\
	rs = rs;							\
	for (i = 0; i < ps; i++)					\
		t[i] = OP(s[i], 0);					\
	for (; i < bs; i += ps) {					\
		t[i] = OP(s[i], p[i - ps]);				\
		if (ps >= 2) t[i + 1] = OP(s[i], p[i - ps + 1]);	\
		if (ps >= 3) t[i + 2] = OP(s[i], p[i - ps + 2]);	\
		if (ps >= 4) t[i + 3] = OP(s[i], p[i - ps + 3]);	\
	}

#define D3ALG_LOOP(ps, i, s, t, p, OP3, T)				  \
	for (i = 0; i < ps; i++)					  \
		t[i] = OP3(s[i], 0, 0, 0, T);				  \
	for (; i < rs; i += ps) {					  \
		t[i] = OP3(s[i], p[i - 4], 0, 0, T);			  \
		if (ps >= 2)						  \
			t[i + 1] = OP3(s[i + 1], p[i - ps + 1], 0, 0, T); \
		if (ps >= 3) 						  \
			t[i + 2] = OP3(s[i + 2], p[i - ps + 2], 0, 0, T); \
		if (ps >= 4)						  \
			t[i + 3] = OP3(s[i + 3], p[i - ps + 3], 0, 0, T); \
	}								  \
	for (; i < bs;) {						  \
		size_t next_line = i + rs;				  \
		if (next_line > bs)					  \
			next_line = bs;					  \
		for (; i < next_line; i += ps) {			  \
			t[i] = OP3(s[i], p[i - 4], p[i - rs],		  \
				   p[i - rs - 4], T);			  \
			if (ps >= 2)					  \
				t[i + 1] = OP3(s[i + 1], p[i - 4 + 1],	  \
					       p[i - rs + 1],		  \
					       p[i - rs - 4 + 1], T);	  \
			if (ps >= 3)					  \
				t[i + 2] = OP3(s[i + 2], p[i - 4 + 2],	  \
					       p[i - rs + 2],		  \
					       p[i - rs - 4 + 2], T);	  \
			if (ps >= 4)					  \
				t[i + 3] = OP3(s[i + 3], p[i - 4 + 3],	  \
					       p[i - rs + 3],		  \
					       p[i - rs - 4 + 3], T);	  \
		}							  \
	}

#define XRGB_LOOP(ps, i, s, t, p, OP, T)		\
	rs = rs; pw = pw; z8 = z8; /* pedantic */	\
	OP(t, s, (const T *)z8);			\
	for (i = ps; i < bs; i += ps)			\
		OP(t + i, s + i, p + i - ps);

#define BUILD_HALG(FN, LOOP, OP, p)					\
	static sbool_t FN(ss_t **dpcm, const ss_t *rgb,			\
		       const struct RGB_Info *ri)			\
	{								\
		ALG_COMMON(p);						\
		switch (ri->bpp / 8) {					\
		case 4: LOOP(4, i, s, t, p, OP, char); break;		\
		case 3: LOOP(3, i, s, t, p, OP, char); break;		\
		case 1: LOOP(1, i, s, t, p, OP, char); break;		\
		case 8: LOOP(4, i, sw, tw, pw, OP, short); break;	\
		case 6: LOOP(3, i, sw, tw, pw, OP, short); break;	\
		case 2: LOOP(1, i, sw, tw, pw, OP, short); break;	\
		default: return S_FALSE;				\
		}							\
		return S_TRUE;						\
	}

#define VLOOP(OP, bs, s, t, p)				\
	for (i = 0; i < rs; i++)			\
		t[i] = OP(s[i], 0);			\
	for (; i < bs;) {				\
		size_t next_line = i + bs;		\
		if (next_line > bs)			\
			next_line = bs;			\
		for (; i < next_line; i++)		\
			t[i] = OP(s[i], p[i - rs]);	\
	}						\

#define BUILD_VDALG(FN, LOOP, OP, p)					     \
	static sbool_t FN(ss_t **dpcm, const ss_t *rgb, 		     \
		       const struct RGB_Info *ri)			     \
	{								     \
		ALG_COMMON(p);						     \
		switch (ri->bpp / 8) {					     \
		case 4: case 3: case 1: LOOP(OP, bs, s, t, p); break;	     \
		case 8: case 6: case 2: LOOP(OP, bs / 2, sw, tw, pw); break; \
		default: return S_FALSE;				     \
		}							     \
		return S_TRUE;						     \
	}

BUILD_HALG(hrgb2dpcm, HDALG_LOOP, DPCM_ENC, s)
BUILD_HALG(hdpcm2rgb, HDALG_LOOP, DPCM_DEC, t)
BUILD_HALG(hrgb2dxor, HDALG_LOOP, DXOR_ENC, s)
BUILD_HALG(hdxor2rgb, HDALG_LOOP, DXOR_DEC, t)
BUILD_HALG(rgb2davg, D3ALG_LOOP, DAVG3_ENC, s)
BUILD_HALG(davg2rgb, D3ALG_LOOP, DAVG3_DEC, t)
BUILD_HALG(rgb2paeth, D3ALG_LOOP, DPAETH_ENC, s)
BUILD_HALG(paeth2rgb, D3ALG_LOOP, DPAETH_DEC, t)
BUILD_HALG(rgb2rsub, XRGB_LOOP, XRSUB_ENC, s)
BUILD_HALG(rsub2rgb, XRGB_LOOP, XRSUB_DEC, t)
BUILD_HALG(rgb2gsub, XRGB_LOOP, XGSUB_ENC, s)
BUILD_HALG(gsub2rgb, XRGB_LOOP, XGSUB_DEC, t)
BUILD_HALG(rgb2bsub, XRGB_LOOP, XBSUB_ENC, s)
BUILD_HALG(bsub2rgb, XRGB_LOOP, XBSUB_DEC, t)
BUILD_VDALG(vrgb2dpcm, VLOOP, DPCM_ENC, s)
BUILD_VDALG(vdpcm2rgb, VLOOP, DPCM_DEC, t)
BUILD_VDALG(vrgb2dxor, VLOOP, DXOR_ENC, s)
BUILD_VDALG(vdxor2rgb, VLOOP, DXOR_DEC, t)

#endif  /* IFILTERS_H */


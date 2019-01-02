/*
 * rgbi.h
 *
 * RGB bitmap description. Image processing example for libsrt.
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#ifndef RGBI_H
#define RGBI_H

#include "../src/libsrt.h"

struct RGB_Info {
	size_t width, height, bpp, Bpp, chn, bpc, Bpc, row_size, bmp_size;
};

S_INLINE srt_bool rgbi_valid(const struct RGB_Info *ri)
{
	return ri && ri->chn > 0 && ri->bpp >= 8 && ri->Bpp > 0 && ri->width > 0
			       && ri->height > 0 && ri->row_size > 0
		       ? S_TRUE
		       : S_FALSE;
}

S_INLINE void rgbi_set(struct RGB_Info *i, size_t w, size_t h, size_t b,
		       size_t c)
{
	if (i) {
		i->width = w;
		i->height = h;
		i->bpp = b;
		i->Bpp = b / 8;
		i->chn = c;
		i->bpc = b / c;
		i->Bpc = i->Bpp / c;
		i->row_size = (w * b) / 8;
		i->bmp_size = i->row_size * h;
	}
}

S_INLINE srt_bool rgbi_cmp(const struct RGB_Info *a, const struct RGB_Info *b)
{
	return a && b && a->width == b->width && a->height == b->height
	       && a->bpp == b->bpp && a->chn == b->chn && a->bpc == b->bpc;
}

/*
 * Pixel packing is related to counting colors and fast pixel comparisons.
 */

S_INLINE unsigned short rgbi_pack2(const unsigned char *p)
{
	return S_LD_LE_U16(p);
}

S_INLINE unsigned rgbi_pack3(const unsigned char *rgb)
{
	return rgbi_pack2(rgb) | ((unsigned)rgb[2] & 0xff) << 8;
}

S_INLINE unsigned rgbi_pack4(const unsigned char *rgba)
{
	return S_LD_LE_U32(rgba);
}

S_INLINE uint64_t rgbi_pack6(const unsigned char *rgb)
{
	return S_LD_LE_U32(rgb) | ((uint64_t)S_LD_LE_U16(rgb + 4)) << 32;
}

S_INLINE uint64_t rgbi_pack8(const unsigned char *rgba)
{
	return S_LD_LE_U32(rgba) | ((uint64_t)S_LD_LE_U32(rgba + 4)) << 32;
}

/* TODO: check if the compiler is able to optimize this properly */
S_INLINE
uint64_t rgbi_get(const char *buf, size_t x, size_t y, size_t rs, size_t ps)
{
	const unsigned char *b = (const unsigned char *)buf;
	switch (ps) {
	case 1:
		return (unsigned char)b[y * rs + x];
	case 2:
		return rgbi_pack2(b + y * rs + x * ps);
	case 3:
		return rgbi_pack3(b + y * rs + x * ps);
	case 4:
		return rgbi_pack4(b + y * rs + x * ps);
	case 6:
		return rgbi_pack6(b + y * rs + x * ps);
	case 8:
		return rgbi_pack8(b + y * rs + x * ps);
	default:
		break;
	}
	return 0;
}

#endif /* RGBI_H */

/*
 * rgbi.h
 *
 * RGB bitmap description. Example for libsrt.
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#ifndef RGBI_H
#define RGBI_H

#include "../src/libsrt.h"

struct RGB_Info
{
	size_t width, height, bpp, chn, bpc, row_size, bmp_size;
};

static sbool_t valid_rgbi(const struct RGB_Info *ri)
{
	return ri && ri->chn > 0 && ri->bpp >= 8 && ri->width > 0 &&
	       ri->height > 0 && ri->row_size > 0 ? S_TRUE : S_FALSE;
}

static void set_rgbi(struct RGB_Info *i, size_t w, size_t h, size_t b, size_t c)
{
	if (i) {
		i->width = w;
		i->height = h;
		i->bpp = b;
		i->chn = c;
		i->bpc = b / c;
		i->row_size = (w * b) / 8;
		i->bmp_size = i->row_size * h;
	}
}

#endif  /* RGBI_H */


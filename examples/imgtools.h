/*
 * imgtools.h
 *
 * Image processing example using libsrt
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 *
 * Observations:
 * - In order to have PNG and JPEG support: make imgc HAS_JPG=1 HAS_PNG=1
 */

#ifndef IMGTOOLS_H
#define IMGTOOLS_H

#include "../src/libsrt.h"
#include "ifilters.h"
#include "rgbi.h"

#define DEBUG_IMGC
#define MY_COMMA ,
#define FMT_IMGS(a) \
	"{tga|ppm|pgm" IF_PNG("|png") IF_JPG("|jpg|jpeg") a "}"
#define FMT_IMGS_IN FMT_IMGS("")
#define FMT_IMGS_OUT FMT_IMGS("|raw")
#define MAX_FILE_SIZE (1024 * 1024 * 1024)

#ifdef DEBUG_IMGC
	#define IF_DEBUG_IMGC(a) a
#else
	#define IF_DEBUG_IMGC(a)
#endif
#ifdef HAS_PNG
	#include <png.h>
	#include <zlib.h>
	#define IF_PNG(a) a
#else
	#define IF_PNG(a)
#endif
#ifdef HAS_JPG
	#include <jpeglib.h>
	#define IF_JPG(a) a
#else
	#define IF_JPG(a)
#endif

enum ImgTypes
{
	IMG_error, IMG_tga, IMG_ppm, IMG_pgm, IMG_png, IMG_jpg, IMG_raw,
	IMG_ll1, IMG_none
};

enum Filters
{
	F_None = 0, F_HDPCM = 1, F_HRDPCM = 2, F_HDXOR = 3, F_HRDXOR = 4, 
	F_VDPCM = 5, F_VRDPCM = 6, F_VDXOR = 7, F_VRDXOR = 8, F_AVG3 = 9,
	F_RAVG3 = 10, F_PAETH = 11, F_RPAETH = 12, F_RSUB = 13, F_RRSUB = 14,
	F_GSUB = 15, F_RGSUB = 16, F_BSUB = 17, F_RBSUB = 18, F_NumElems
};

size_t any2rgb(ss_t **rgb, struct RGB_Info *ri, const ss_t *in,
	       enum ImgTypes in_type);
size_t rgb2type(ss_t **out, enum ImgTypes out_type, const ss_t *rgb,
		const struct RGB_Info *ri, const int filter);
enum ImgTypes file_type(const char *file_name);
int rgbdiff(ss_t **out, const ss_t *rgb0, const struct RGB_Info *ri0,
	    const ss_t *rgb1, const struct RGB_Info *ri1);

#endif  /* IMGTOOLS_H */


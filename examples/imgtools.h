/*
 * imgtools.h
 *
 * Image processing example using libsrt
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 *
 * Observations:
 * - In order to have PNG and JPEG support: make imgc HAS_JPG=1 HAS_PNG=1
 */

#ifndef IMGTOOLS_H
#define IMGTOOLS_H

#include "../src/libsrt.h"
#include "rgbi.h"

#define DEBUG_IMGC
#define MY_COMMA ,
#define FMT_IMGS(a) "{tga|ppm|pgm" IF_PNG("|png") IF_JPG("|jpg|jpeg") a "}"
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

enum ImgTypes {
	IMG_error,
	IMG_tga,
	IMG_ppm,
	IMG_pgm,
	IMG_png,
	IMG_jpg,
	IMG_raw,
	IMG_ll1,
	IMG_none
};

enum Filters {
	F_None,
	F_HDPCM,
	F_HRDPCM,
	F_HDXOR,
	F_HRDXOR,
	F_VDPCM,
	F_VRDPCM,
	F_VDXOR,
	F_VRDXOR,
	F_AVG3,
	F_RAVG3,
	F_PAETH,
	F_RPAETH,
	F_RSUB,
	F_RRSUB,
	F_GSUB,
	F_RGSUB,
	F_BSUB,
	F_RBSUB,
	F_NumElems
};

size_t any2rgb(srt_string **rgb, struct RGB_Info *ri, const srt_string *in,
	       enum ImgTypes in_type);
size_t rgb2type(srt_string **out, enum ImgTypes out_type, const srt_string *rgb,
		const struct RGB_Info *ri, int filter);
enum ImgTypes file_type(const char *file_name);
int rgbdiff(srt_string **out, const srt_string *rgb0,
	    const struct RGB_Info *ri0, const srt_string *rgb1,
	    const struct RGB_Info *ri1);

#endif /* IMGTOOLS_H */

/*
 * imgc.c
 *
 * Image processing example using libsrt
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 *
 * Observations:
 * - In order to have PNG and JPEG support: make imgc HAS_JPG=1 HAS_PNG=1
 */

#include "../src/libsrt.h"
#include "ifilters.h"
#include "rgbi.h"

#define DEBUG_IMGC
#define MY_COMMA ,
#define FMT_IMGS(a) \
	"{tga|ppm|pgm" IF_PNG("|png") IF_JPG("|jpg|jpeg") IF_LL1("|ll1") a "}"
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
#ifdef HAS_LL1 /* not implemented, yet */
	#include "ll1.h"
	#define IF_LL1(a) a
#else
	#define IF_LL1(a)
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

static size_t any2rgb(ss_t **rgb, struct RGB_Info *ri, const ss_t *in,
		      enum ImgTypes in_type);
static size_t rgb2type(ss_t **out, enum ImgTypes out_type, const ss_t *rgb,
		       const struct RGB_Info *ri, const int filter);
static enum ImgTypes file_type(const char *file_name);
static int exit_with_error(const char **argv, const char *msg,
			   const int exit_code);

int main(int argc, const char **argv)
{
	size_t in_size = 0;
	struct RGB_Info ri;
	ss_t *iobuf = NULL, *rgb_buf = NULL;
	int exit_code = 1;
	FILE *fin = NULL, *fout = NULL;
	const char *exit_msg = "not enough parameters";
	int filter = F_None;
	#define IMGC_XTEST(test, m, c)	\
		if (test) { exit_msg = m; exit_code = c; break; }
	for (;;) {
		if (argc < 2)
			break;
		if (argc > 3) {
			filter = atoi(argv[3]);
			IMGC_XTEST(filter && filter >= F_NumElems,
				   "invalid filter", 10);
		}
		sbool_t ro = argc < 3 ? S_TRUE : S_FALSE;
		enum ImgTypes t_in = file_type(argv[1]),
			      t_out = ro ? IMG_none : file_type(argv[2]);
		IMGC_XTEST(t_in == IMG_error || t_out == IMG_error,
			   "invalid parameters", t_in == IMG_error ? 2 : 3);

		fin = fopen(argv[1], "rb");
		iobuf = ss_dup_read(fin, MAX_FILE_SIZE);
		in_size = ss_size(iobuf);
		IMGC_XTEST(!in_size, "input read error", 4);

		size_t rgb_bytes = any2rgb(&rgb_buf, &ri, iobuf, t_in);
		IMGC_XTEST(!rgb_bytes, "can not process input file", 5);

		if (ro)
			printf("%s: ", argv[1]);
		size_t enc_bytes = rgb2type(&iobuf, t_out, rgb_buf, &ri, filter);
		IMGC_XTEST(!enc_bytes, "output file encoding error", 6);

		fout = fopen(argv[2], "wb+");
		ssize_t written = ro ? 0 :
				       ss_write(fout, iobuf, 0, ss_size(iobuf));
		IMGC_XTEST(!ro && (written < 0 ||
				   (size_t)written != ss_size(iobuf)),
			   "write error", 7);

		exit_code = 0;
		if (!ro)
			IF_DEBUG_IMGC(
				fprintf(stderr, "%s (%ix%i %ibpp %ich) %u bytes"
					" > %s %u bytes\n", argv[1],
					(int)ri.width, (int)ri.height,
					(int)ri.bpp, (int)ri.chn,
					(unsigned)in_size, argv[2],
					(unsigned)ss_size(iobuf)));
		break;
	}
	ss_free(&iobuf, &rgb_buf);
	if (fin)
		fclose(fin);
	if (fout)
		fclose(fout);
	return exit_code ? exit_with_error(argv, exit_msg, exit_code) : 0;
}

static enum ImgTypes file_type(const char *file_name)
{
	size_t l = file_name ? strlen(file_name) : 0, sf = l > 3 ? l - 3 : 0;
	return !sf ? IMG_error :
	       !strncmp(file_name + sf, "tga", 3) ? IMG_tga :
	       !strncmp(file_name + sf, "ppm", 3) ? IMG_ppm :
	       !strncmp(file_name + sf, "pgm", 3) ? IMG_pgm :
	       IF_PNG(!strncmp(file_name + sf, "png", 3) ? IMG_png :)
	       IF_JPG(!strncmp(file_name + sf, "jpg", 3) ? IMG_jpg :)
	       IF_JPG(!strncmp(file_name + sf - 1, "jpeg", 4) ? IMG_jpg :)
	       IF_LL1(!strncmp(file_name + sf, "ll1", 3) ? IMG_ll1 :)
	       !strncmp(file_name + sf, "raw", 3) ? IMG_raw : IMG_error;
}

static int exit_with_error(const char **argv, const char *msg,
			   const int exit_code)
{
	const char *v0 = argv[0];
	fprintf(stderr, "Error [%i]: %s\nSyntax: %s in." FMT_IMGS_IN " [out."
		FMT_IMGS_OUT " [filter]]\nExamples:\n"
		"\tStats: %s input.pgm\n"
		IF_PNG("\tConvert: %s input.png output.ppm\n")
		IF_JPG("\tConvert: %s input.jpg output.tga\n")
		IF_LL1("\tConvert: %s input.ppm output.ll1\n")
		"\tConvert: %s input.ppm output.tga\n"
		"\tConvert + filter: %s input.ppm output.tga 5\n"
		"Filters: 0 (none), 1 left->right DPCM, 2 reverse of (1), "
		"3 left->right xor DPCM,\n\t 4 reverse of (3), 5 up->down DPCM, "
		"6 reverse of (5),\n\t 7 up->down xor DPCM, 8 reverse of (7), "
		"9 left/up/up-left average DPCM,\n\t 10 reverse of (9), "
		"11 Paeth DPCM, 12 reverse of (11),\n\t 13 red substract, 14 "
		"reverse of (13), 15 green substract, 16 reverse\n\t of (15), "
		"17 blue substract, 18 reverse of (17)\n",
		exit_code, msg, v0, v0, IF_PNG(v0 MY_COMMA) IF_JPG(v0 MY_COMMA)
		IF_LL1(v0 MY_COMMA) v0, v0);
	return exit_code;
}

/* TGA codec start */

enum TGA_DCRGB_Offs { TGA_ID = 0, TGA_CMAP = 1, TGA_TYPE = 2, TGA_W = 12,
		      TGA_H = 14, TGA_BPP = 16, TGA_DESC = 17, TGA_RGBHDR };
enum TGA_MISC { TGA_LOWER_LEFT = 0, TGA_TOP_LEFT = 0x20, TGA_NO_X_INFO = 0,
		TGA_NO_CMAP = 0, TGA_RAW_RGB = 2,  TGA_RAW_GRAY = 3 };

static sbool_t valid_tga(const char *h)
{
	return h[TGA_ID] == TGA_NO_X_INFO && h[TGA_CMAP] == TGA_NO_CMAP &&
		   (h[TGA_TYPE] == TGA_RAW_RGB || h[TGA_TYPE] == TGA_RAW_GRAY);
}

static sbool_t tga_rgb_swap(const size_t bpp, const size_t buf_size,
			    const char *s, char *t)
{
	size_t i;
	switch (bpp) {
	case 24:for (i = 0; i < buf_size; i += 3)
			t[i] = s[i + 2], t[i + 1] = s[i + 1], t[i + 2] = s[i];
		return S_TRUE;
	case 32:for (i = 0; i < buf_size; i += 4) /* RGBA <-> BGRA */
			t[i] = s[i + 2], t[i + 1] = s[i + 1],
			t[i + 2] = s[i], t[i + 3] = s[i + 3];
		return S_TRUE;
	default:return S_FALSE;
	}
}

static size_t tga2rgb(ss_t **rgb, struct RGB_Info *ri, const ss_t *tga)
{
	const char *t = ss_get_buffer_r(tga);
	const size_t ts = ss_size(tga);
	RETURN_IF(ts < TGA_RGBHDR || !valid_tga(t), 0);
	set_rgbi(ri, S_LD_LE_U16(t + TGA_W), S_LD_LE_U16(t + TGA_H),
		 (size_t)t[TGA_BPP], t[TGA_TYPE] == TGA_RAW_GRAY ? 1 :
							(size_t)t[TGA_BPP]/8);
	RETURN_IF(!valid_rgbi(ri), 0);
	size_t rgb_bytes = ri->row_size * ri->height;
	RETURN_IF(ts < rgb_bytes + TGA_RGBHDR, 0);
	RETURN_IF(ss_reserve(rgb, rgb_bytes) < rgb_bytes, 0);
	if ((t[TGA_DESC] & TGA_TOP_LEFT) != 0) {
		if (ri->chn == 1)
			ss_cpy_cn(rgb, t + TGA_RGBHDR, rgb_bytes);
		else
			tga_rgb_swap(ri->bpp, rgb_bytes,
				     ss_get_buffer_r(tga) + TGA_RGBHDR,
				     ss_get_buffer(*rgb));
	} else {
		ss_set_size(*rgb, 0);
		ssize_t i = (ssize_t)((ri->height - 1) * ri->row_size);
		for (; i >= 0; i -= ri->row_size)
			if (ri->chn == 1)
				ss_cat_cn(rgb, t + TGA_RGBHDR + i,
					  ri->row_size);
			else
				tga_rgb_swap(ri->bpp, ri->row_size,
					     t + TGA_RGBHDR + i,
					     ss_get_buffer(*rgb) + i);
	}
	ss_set_size(*rgb, rgb_bytes);
	return rgb_bytes;
}

static size_t rgb2tga(ss_t **tga, const ss_t *rgb, const struct RGB_Info *ri)
{
	RETURN_IF(!rgb || (!valid_rgbi(ri) && (ri->bpp / ri->chn) != 8), 0);
	RETURN_IF(ri->chn != 1 && ri->chn != 3 && ri->chn != 4, 0);
	size_t buf_size = ri->bmp_size + TGA_RGBHDR;
	RETURN_IF(ss_reserve(tga, buf_size) < buf_size, 0);
	char *h = ss_get_buffer(*tga);
	memset(h, 0, TGA_RGBHDR);
	h[TGA_ID] = TGA_NO_X_INFO;
	h[TGA_CMAP] = TGA_NO_CMAP;
	h[TGA_TYPE] = ri->chn == 1 ? TGA_RAW_GRAY : TGA_RAW_RGB;
	S_ST_LE_U16(h + TGA_W, (uint16_t)ri->width);
	S_ST_LE_U16(h + TGA_H, (uint16_t)ri->height);
	h[TGA_BPP] = (char)ri->bpp;
	h[TGA_DESC] = TGA_TOP_LEFT;
	ss_cpy_cn(tga, h, TGA_RGBHDR);
	if (ri->chn == 1)
		ss_cat(tga, rgb);
	else
		tga_rgb_swap(ri->bpp, ri->bmp_size, ss_get_buffer_r(rgb),
			     ss_get_buffer(*tga) + TGA_RGBHDR);
	ss_set_size(*tga, buf_size);
	return ss_size(*tga);
}

/* TGA codec end */

/* PGM/PPM codec start */

#define PPM_NFIELDS 3

static sbool_t valid_rgbi_for_ppm(const struct RGB_Info *ri)
{
	RETURN_IF(!valid_rgbi(ri), S_FALSE);
	RETURN_IF(ri->chn == 3 && ri->bpp != 24 && ri->bpp != 48, S_FALSE);
	RETURN_IF(ri->chn == 1 && ri->bpp != 8 && ri->bpp != 16, S_FALSE);
	return S_TRUE;
}

static size_t ppm2rgb(ss_t **rgb, struct RGB_Info *ri, const ss_t *ppm)
{
	RETURN_IF(!ppm || !ri, 0);
	const char *p = ss_get_buffer_r(ppm);
	size_t ps = ss_size(ppm);
	ri->chn = p[1] == '5' ? 1 : p[1] == '6' ? 3 : 0;
	RETURN_IF(ps < 16 || p[0] != 'P' || !ri->chn, 0);
	size_t off = 2, nf = 0, nl, nl2, i = 0;
	size_t f[PPM_NFIELDS];
	for (; i != S_NPOS && nf < PPM_NFIELDS && off < ps; off = nl + 1) {
		nl = ss_findc(ppm, off, '\n');
		if (nl == S_NPOS)
			break;
		nl2 = ss_findrc(ppm, off, nl, '#'); /* skip comments */
		nl2 = nl2 == S_NPOS ? nl : nl2;
		for (i = off; i < nl2;) { /* get fields */
			i = ss_findrbm(ppm, i, nl2, 0x30, 0xc0); /* digits */
			if (i == S_NPOS)
				break;
			f[nf++] = (size_t)atoi(p + i);
			if (nf == PPM_NFIELDS)
				break;
			i = ss_findrb(ppm, i + 1, nl2);
		}
	}
	RETURN_IF(nf != PPM_NFIELDS, 0);
	ri->bpp = ri->chn * (f[2] <= 255 ? 1 : f[2] <= 65535 ? 2 : 0) * 8;
	set_rgbi(ri, f[0], f[1], ri->bpp, ri->chn);
	RETURN_IF(!valid_rgbi(ri), 0);
	ss_cpy_cn(rgb, p + off, ps - off); /* Copy pixel data */
	return ss_size(*rgb);
}

static size_t rgb2ppm(ss_t **ppm, const ss_t *rgb, const struct RGB_Info *ri)
{
	RETURN_IF(!ppm || !rgb || !valid_rgbi_for_ppm(ri), 0);
	int colors_per_channel = (1 << (ri->bpp / ri->chn)) - 1;
	ss_printf(ppm, 512, "P%i %i %i %i\n", (ri->chn == 1 ? 5 : 6),
		  (int)ri->width, (int)ri->height, colors_per_channel);
	ss_cat(ppm, rgb);
	return ss_size(*ppm);
}

/* PGM/PPM codec end */

#ifdef HAS_PNG	/* PNG codec start */

struct aux_png_rio { size_t off; const ss_t *buf; };

static void aux_png_read(png_structp s, png_bytep d, png_size_t ds)
{
	struct aux_png_rio *pio = (struct aux_png_rio *)png_get_io_ptr(s);
	if (pio && pio->buf) {
		memcpy(d, ss_get_buffer_r(pio->buf) + pio->off, ds);
		pio->off += ds;
	}
}

static void aux_png_write(png_structp s, png_bytep d, png_size_t ds)
{
	ss_cat_cn((ss_t **)png_get_io_ptr(s), (char *)d, ds);
}

static size_t aux_png_get_chn(const png_info *pi)
{
	RETURN_IF(!pi, S_FALSE);
	int ct = pi->color_type;
	sbool_t alpha = (ct & PNG_COLOR_MASK_ALPHA) != 0 ? S_TRUE : S_FALSE;
	sbool_t gray = (ct & PNG_COLOR_MASK_COLOR) == 0 ? S_TRUE : S_FALSE;
	return (gray ? 1 : 3) + (alpha ? 1 : 0);
}

static size_t aux_png_get_depth(const png_info *pi)
{
	RETURN_IF(!pi, 0);
	return (pi->color_type & PNG_COLOR_MASK_PALETTE) != 0 ?
		8 : pi->bit_depth;
}

static sbool_t aux_png2rbi(const png_info *pi, struct RGB_Info *ri)
{
	RETURN_IF(!pi || !ri, S_FALSE);
	size_t depth = aux_png_get_depth(pi), chn = aux_png_get_chn(pi);
	set_rgbi(ri, pi->width, pi->height, depth * chn, chn);
	return valid_rgbi(ri);
}

static size_t aux_png_row_size(const png_info *pi)
{
	return aux_png_get_chn(pi) * (pi->width * pi->bit_depth) / 8;
}

static sbool_t aux_png_set_rows(png_bytep *rows, const png_info *pi,
				const char *rgb)
{
	RETURN_IF(!rows || !pi || !rgb, S_FALSE);
	size_t i = 0, rs = aux_png_row_size(pi);
	for (; i < pi->height; i++)
		rows[i] = (png_bytep)(rgb + i * rs);
	return S_TRUE;
}

static sbool_t aux_png_read_set_rows(png_bytep *rows, const png_info *pi,
				     ss_t **rgb)
{
	RETURN_IF(!rows || !pi || !rgb, S_FALSE);
	size_t rs = aux_png_row_size(pi), rgb_size = rs * pi->height;
	RETURN_IF(ss_reserve(rgb, rgb_size) < rgb_size, S_FALSE);
	ss_set_size(*rgb, rgb_size);
	return aux_png_set_rows(rows, pi, ss_get_buffer(*rgb));
}

static size_t png2rgb(ss_t **rgb, struct RGB_Info *ri, const ss_t *png)
{
	RETURN_IF(!png || !ss_size(png), 0);
	size_t out_size = 0;
	struct aux_png_rio pio = { 0, png };
	png_structp s = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	png_infop pi = png_create_info_struct(s);
	png_set_read_fn(s, (png_voidp)&pio, aux_png_read);
	png_read_info(s, pi);
	if (!aux_png2rbi(pi, ri)) {
		png_destroy_info_struct(s, &pi);
		png_destroy_read_struct(&s, NULL, NULL);
		return 0;
	}
	if ((pi->color_type & PNG_COLOR_MASK_PALETTE) != 0)
		png_set_expand(s); /* pal: expand to RGB */
	png_bytep *rows = (png_bytep *)alloca(sizeof(png_bytep) * pi->height);
	if (aux_png_read_set_rows(rows, pi, rgb)) {
		size_t i = png_set_interlace_handling(s);
		for (; i > 0; i--)
			png_read_rows(s, rows, NULL, pi->height);
		png_read_end(s, pi);
		out_size = ss_size(*rgb);
	}
	png_destroy_info_struct(s, &pi);
	png_destroy_read_struct(&s, NULL, NULL);
	return out_size;
}

static size_t rgb2png(ss_t **png, const ss_t *rgb, const struct RGB_Info *ri)
{
	RETURN_IF(!valid_rgbi(ri) ||!rgb, 0);
	size_t out_size = 0;
	png_structp s = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	png_infop pi = png_create_info_struct(s);
	if (png)
		ss_set_size(*png, 0);
	png_set_write_fn(s, (png_voidp)png, aux_png_write, NULL);
	int ctype = (ri->chn > 1 ? PNG_COLOR_MASK_COLOR : 0) |
		    (ri->chn == 2 || ri->chn == 4 ? PNG_COLOR_MASK_ALPHA : 0);
	png_set_IHDR(s, pi, (png_uint_32)ri->width, (png_uint_32)ri->height,
		     ri->bpc, ctype, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
		     PNG_FILTER_TYPE_BASE);
	png_set_filter(s, PNG_FILTER_TYPE_BASE, PNG_ALL_FILTERS);
	png_set_compression_level(s, Z_BEST_COMPRESSION);
	png_write_info(s, pi);
	png_bytep *rows = (png_bytep *)alloca(sizeof(png_bytep) * pi->height);
	if (aux_png_set_rows(rows, pi, ss_get_buffer_r(rgb))) {
		png_write_image(s, rows);
		png_write_end(s, pi);
		out_size = ss_size(*png);
	}
	png_destroy_info_struct(s, &pi);
	png_destroy_write_struct(&s, &pi);
	return out_size;
}

#endif /* #ifdef HAS_PNG */	/* PNG codec end */

#ifdef HAS_JPG	/* JPG codec start */

#define JPG_SMOOTH 		0
#define JPG_TMP_MAX_MEM		(1024*1024)
#define JPG_QUALITY		95
#define JPG_SUBS_HQ

static void jpg_error_exit(j_common_ptr jc)
{
	jc = jc; /* pedantic */
	fprintf(stderr, "jpeg critical error (!)\n");
	exit(1);
}

struct aux_jpeg
{
	/* encode */
	const struct RGB_Info *ri_enc;
	struct jpeg_destination_mgr jdest;
	/* decode */
	struct jpeg_source_mgr jsrc;
	/* common */
	const ss_t *in;
	ss_t **out;
	sbool_t errors;
	size_t out_size, max_out_size;
};

static void aux_jpegd_init(j_decompress_ptr jc)
{
	jc = jc; /* pedantic */
}

static boolean aux_jpegd_fill_input_buffer(j_decompress_ptr jc)
{
	struct aux_jpeg *jaux = (struct aux_jpeg *)jc->client_data;
	jaux->errors = S_TRUE;
	return TRUE;
}

static void aux_jpegd_skip_input_data(j_decompress_ptr jc, long size)
{
	struct aux_jpeg *jaux = (struct aux_jpeg *)jc->client_data;
	if (size < 0 || (size_t)size > jc->src->bytes_in_buffer) {
		jaux->errors = S_TRUE;
	} else {
		jc->src->next_input_byte += (size_t)size;
		jc->src->bytes_in_buffer -= (size_t)size;
	}
}

static void aux_jpegd_term_source(j_decompress_ptr jc)
{
	jc = jc; /* pedantic */
}

static void aux_jpege_init(j_compress_ptr jc)
{
	struct aux_jpeg *jaux = (struct aux_jpeg *)jc->client_data;
	if (jaux->out && *jaux->out) {
		jc->dest->next_output_byte =
					  (png_bytep)ss_get_buffer(*jaux->out);
		jc->dest->free_in_buffer = jaux->max_out_size;
	} else {
		jaux->errors = S_TRUE;
	}
}

static boolean aux_jpege_empty_output_buffer(j_compress_ptr jc)
{
	aux_jpege_init(jc);
	struct aux_jpeg *jaux = (struct aux_jpeg *)jc->client_data;
	jaux->errors = S_TRUE;
	return TRUE;
}

static void aux_jpege_term_destination(j_compress_ptr jc)
{
	struct aux_jpeg *jaux = (struct aux_jpeg *)jc->client_data;
	jaux->out_size = jaux->max_out_size - jc->dest->free_in_buffer;
}

void aux_jpeg_common_init(struct aux_jpeg *ja)
{
	memset(ja, 0, sizeof(*ja));
	ja->errors = S_FALSE;
	ja->out_size = 0;
}

sbool_t aux_jpeg_dec_init(struct jpeg_decompress_struct *jd,
			  struct aux_jpeg *ja, struct RGB_Info *ri,
			  const ss_t *jpg_in, ss_t **rgb_out)
{
	RETURN_IF(!ja || !ri || !jpg_in || !rgb_out, S_FALSE);
	jpeg_create_decompress(jd);
	jd->err->trace_level = 0;
	jd->err->error_exit = jpg_error_exit;
	ja->in = jpg_in;
	ja->out = rgb_out;
	jd->src = &ja->jsrc;
	aux_jpeg_common_init(ja);
	ja->jsrc.init_source = aux_jpegd_init;
	ja->jsrc.fill_input_buffer = aux_jpegd_fill_input_buffer;
	ja->jsrc.skip_input_data = aux_jpegd_skip_input_data;
	ja->jsrc.resync_to_restart = jpeg_resync_to_restart;
	ja->jsrc.term_source = aux_jpegd_term_source;
	ja->jsrc.next_input_byte = (JOCTET *)ss_get_buffer_r(jpg_in);
	ja->jsrc.bytes_in_buffer = ss_size(jpg_in);
	RETURN_IF(jpeg_read_header(jd, TRUE) != JPEG_HEADER_OK, 0);
	jpeg_start_decompress(jd);
	set_rgbi(ri, jd->output_width, jd->output_height,
		 jd->out_color_components * 8, jd->out_color_components);
	RETURN_IF(!valid_rgbi(ri), S_FALSE);
	size_t rgb_size = ri->row_size * ri->height;
	if (ss_reserve(rgb_out, rgb_size) >= rgb_size)
		return S_TRUE;
	jpeg_destroy_decompress(jd);
	return S_FALSE;
}

sbool_t aux_jpeg_enc_init(struct jpeg_compress_struct *jc,
			  struct aux_jpeg *ja, const struct RGB_Info *ri,
			  const ss_t *rgb, ss_t **jpg_out)
{
	RETURN_IF(!ja || !valid_rgbi(ri) || !rgb || !jpg_out, S_FALSE);
	ss_set_size(*jpg_out, 0);
	jpeg_create_compress(jc);
	jc->in_color_space = ri->chn == 1 ? JCS_GRAYSCALE : JCS_RGB;
	jpeg_set_defaults(jc);
	jc->input_components = ri->chn;
	jc->data_precision = ri->bpp / ri->chn;
	jc->image_width = ri->width;
	jc->image_height = ri->height;
	jpeg_default_colorspace(jc);
	jc->smoothing_factor = JPG_SMOOTH;
	jc->arith_code = FALSE;
	if (ri->chn == 1)
		jpeg_set_colorspace(jc, JCS_GRAYSCALE);
	jc->mem->max_memory_to_use = JPG_TMP_MAX_MEM;
	jc->restart_in_rows = 0;
	jc->optimize_coding = TRUE;
	jc->err->trace_level = 0;
	jc->err->error_exit = jpg_error_exit;
#ifdef S_HW_FPU
	jc->dct_method = JDCT_FLOAT;
#else
	jc->dct_method = JDCT_ISLOW;
#endif
#ifdef JPG_SUBS_HQ /* No chroma subsampling (better quality, +30% size) */
	jc->comp_info[0].h_samp_factor = jc->comp_info[0].v_samp_factor =
	jc->comp_info[1].h_samp_factor = jc->comp_info[1].v_samp_factor =
	jc->comp_info[2].h_samp_factor = jc->comp_info[2].v_samp_factor = 1;
#endif
	jpeg_set_quality(jc, JPG_QUALITY, TRUE);
	aux_jpeg_common_init(ja);
	/* Max out size: this is an heuristic (adjusted afterwards) */
	ja->max_out_size = (ri->width * ri->height * ri->bpp) / 8;
	RETURN_IF(ss_reserve(jpg_out, ja->max_out_size) < ja->max_out_size,
		  S_FALSE);
	ja->in = rgb;
	ja->out = jpg_out;
	ja->ri_enc = ri;
	ja->errors = S_FALSE;
	ja->jdest.init_destination = aux_jpege_init;
	ja->jdest.empty_output_buffer = aux_jpege_empty_output_buffer;
	ja->jdest.term_destination = aux_jpege_term_destination;
	jc->client_data = ja;
	jc->dest = &ja->jdest;
	return S_TRUE;
}

sbool_t valid_rgbi_for_jpg(const struct RGB_Info *ri)
{
	RETURN_IF(!valid_rgbi(ri), S_FALSE);
	RETURN_IF(ri->chn == 2 || ri->chn > 3, S_FALSE);
	RETURN_IF(ri->chn == 3 && ri->bpp != 24, S_FALSE);
	RETURN_IF(ri->chn == 1 && ri->bpp != 8, S_FALSE);
	return S_TRUE;
}

static size_t jpg2rgb(ss_t **rgb, struct RGB_Info *ri, const ss_t *jpg)
{
	RETURN_IF(!jpg || !ri, 0);
	struct jpeg_error_mgr je;
	struct jpeg_decompress_struct jd;
	struct aux_jpeg jaux;
	jd.err = jpeg_std_error(&je);
	RETURN_IF(!aux_jpeg_dec_init(&jd, &jaux, ri, jpg, rgb), 0);
	JSAMPROW r;
	char *rgbp = ss_get_buffer(*rgb);
	size_t i = 0, it = ri->row_size * ri->height;
	for (; i < it && jaux.errors == S_FALSE; i += ri->row_size) {
		r = (JSAMPROW)(rgbp + i);
		jpeg_read_scanlines(&jd, &r, 1);
	}
	if (jaux.errors == S_FALSE) {
		jpeg_finish_decompress(&jd);
		ss_set_size(*rgb, i);
	} else {
		i = 0;
	}
	jpeg_destroy_decompress(&jd);
	return i;
}

static size_t rgb2jpg(ss_t **jpg, const ss_t *rgb, const struct RGB_Info *ri)
{
	RETURN_IF(!valid_rgbi_for_jpg(ri) ||!rgb, 0);
	struct jpeg_error_mgr je;
	struct jpeg_compress_struct jc;
	struct aux_jpeg jaux;
	jc.err = jpeg_std_error(&je);
	RETURN_IF(!aux_jpeg_enc_init(&jc, &jaux, ri, rgb, jpg), 0);
	jpeg_start_compress(&jc, TRUE);
	jc.next_scanline = 0;
	size_t i = 0, i_max = ri->height * ri->row_size;
	const char *rgbp = ss_get_buffer_r(rgb);
	JSAMPROW r;
	for (; i < i_max && jaux.errors == S_FALSE; i += ri->row_size) {
		r = (JSAMPROW)(rgbp + i);
		jpeg_write_scanlines(&jc, &r, 1);
	}
	if (jaux.errors == S_FALSE) {
		jpeg_finish_compress(&jc);
		ss_set_size(*jpg, jaux.out_size);
		ss_shrink(jpg);
	} else {
		jaux.out_size = 0;
	}
	jpeg_destroy_compress(&jc);
	return jaux.out_size;
}

#endif /* #ifdef HAS_JPG */	/* JPG codec end */

#define RGB_COUNT_LOOP(i, p, ss, ps, bs, cmx, out)			\
	sb_clear(bs);							\
	for (i = 0; i < ss && sb_popcount(bs) < cmx; i += ps) {		\
		unsigned c = (unsigned)p[i] & 0xff;			\
		if (ps > 1) c |= ((unsigned)p[i + 1] & 0xff) << 8;	\
		if (ps > 2) c |= ((unsigned)p[i + 2] & 0xff) << 16;	\
		if (ps > 3) c |= ((unsigned)p[i + 3] & 0xff) << 24;	\
		sb_set(&bs, c);						\
	}								\
	out = sb_popcount(bs);

#define RGBC_COUNT_LOOP(i, p, ss, ps, bs, off, out)			\
	sb_clear(bs);							\
	for (i = 0; i < ss && sb_popcount(bs) < 1 << 8; i += ps) {	\
		sb_set(&bs, (unsigned char)p[i + off]);			\
	}								\
	out = sb_popcount(bs);

#define RGBC2_COUNT_LOOP(i, p, ss, ps, bs, off, out) {			\
	sb_clear(bs);							\
	size_t off2 = (off + 1) % 3;					\
	for (i = 0; i < ss && sb_popcount(bs) < 1 << 16; i += ps) {	\
		unsigned c = ((unsigned)p[i + off] & 0xff) |		\
			     ((unsigned)p[i + off2] & 0xff) << 8;	\
		sb_set(&bs, c);						\
	}								\
	out = sb_popcount(bs); }

static size_t rgb_info(const ss_t *rgb, const struct RGB_Info *ri)
{
	if (rgb && ri) {
		const size_t ss = ss_size(rgb), ps = ri->bpp / 8;
		size_t cmax = ri->bpp == 32 ? 0xffffffff :
					      0xffffffff &
					      ((1 << ri->bpp) - 1);
		size_t i, uqp = 0;
		const char *p0 = ss_get_buffer_r(rgb);
		const unsigned char *p = (const unsigned char *)p0;
		sb_t *bs = sb_alloc(0);
		sb_eval(&bs, cmax);
		switch (ps) {
		case 4: RGB_COUNT_LOOP(i, p, ss, ps, bs, cmax, uqp); break;
		case 3: RGB_COUNT_LOOP(i, p, ss, ps, bs, cmax, uqp); break;
		case 2: RGB_COUNT_LOOP(i, p, ss, ps, bs, cmax, uqp); break;
		case 1: RGB_COUNT_LOOP(i, p, ss, ps, bs, cmax, uqp); break;
		default:
			break;
		}
		unsigned pixels = (unsigned)(ri->width * ri->height);
		unsigned l = (unsigned)((uqp * 100) / pixels),
			 r = (unsigned)(((uqp * 10000) / pixels) % 100);
		printf("%ix%i %i bpp, %i chn; %u px; %u unique px"
		       " (%u.%u%%)", (int)ri->width, (int)ri->height,
		       (int)ri->bpp, (int)ri->chn, pixels,
		       (unsigned)uqp, l, r);
		if (ps == 3 || ps == 4) {
			size_t ur, ug, ub, ua, urg, ugb, urb;
			RGBC_COUNT_LOOP(i, p, ss, ps, bs, 0, ur);
			RGBC_COUNT_LOOP(i, p, ss, ps, bs, 0, ug);
			RGBC_COUNT_LOOP(i, p, ss, ps, bs, 0, ub);
			RGBC2_COUNT_LOOP(i, p, ss, ps, bs, 0, urg);
			RGBC2_COUNT_LOOP(i, p, ss, ps, bs, 1, ugb);
			RGBC2_COUNT_LOOP(i, p, ss, ps, bs, 2, urb);
			if (ps == 4) {
				RGBC_COUNT_LOOP(i, p, ss, ps, bs, 0, ua);
			} else {
				ua = 0;
			}
			printf("; %u ur; %u ug; %u ub; %u urg; %u ugb; %u urb;"
			       " %u ua", (unsigned)ur, (unsigned)ug,
			       (unsigned)ub, (unsigned)urg, (unsigned)ugb,
			       (unsigned)urb, (unsigned)ua);
		}
		printf("\n");
	} else {
		fprintf(stderr, "Error: no pixel data\n");
	}
	return ss_size(rgb);
}

static size_t any2rgb(ss_t **rgb, struct RGB_Info *ri, const ss_t *in,
		      enum ImgTypes it)
{
	return it == IMG_tga ? tga2rgb(rgb, ri, in) :
	       it == IMG_ppm || it == IMG_pgm ? ppm2rgb(rgb, ri, in) :
	       IF_PNG(it == IMG_png ? png2rgb(rgb, ri, in) :)
	       IF_JPG(it == IMG_jpg ? jpg2rgb(rgb, ri, in) :)
	       IF_LL1(it == IMG_ll1 ? ll12rgb(rgb, ri, in) :) 0;
}

static size_t rgb2type(ss_t **out, enum ImgTypes ot, const ss_t *rgb0,
		       const struct RGB_Info *ri, const int f)
{
	ss_t *rgb_aux = NULL;
	const ss_t *rgb;
	switch (f) {	/* Apply filter, if any */
	case F_HDPCM: case F_HRDPCM: case F_HDXOR: case F_HRDXOR: case F_VDPCM:
	case F_VRDPCM: case F_VDXOR: case F_VRDXOR: case F_AVG3: case F_RAVG3:
	case F_PAETH: case F_RPAETH: case F_RSUB: case F_RRSUB: case F_GSUB:
	case F_RGSUB: case F_BSUB: case F_RBSUB:
		rgb = (f == F_HDPCM ? hrgb2dpcm : f == F_HRDPCM ? hdpcm2rgb :
		       f == F_HDXOR ? hrgb2dxor : f == F_HRDXOR ? hdxor2rgb :
		       f == F_VDPCM ? vrgb2dpcm : f == F_VRDPCM ? vdpcm2rgb :
		       f == F_VDXOR ? vrgb2dxor : f == F_VRDXOR ? vdxor2rgb :
		       f == F_AVG3 ? rgb2davg : f == F_RAVG3 ? davg2rgb :
		       f == F_PAETH ? rgb2paeth : f == F_RPAETH ? paeth2rgb :
		       f == F_RSUB ? rgb2rsub : f == F_RRSUB ? rsub2rgb :
		       f == F_GSUB ? rgb2gsub : f == F_RGSUB ? gsub2rgb :
		       f == F_BSUB ? rgb2bsub : bsub2rgb)(&rgb_aux, rgb0, ri) ?
		      rgb_aux : rgb0;
		if (rgb == rgb0)
			fprintf(stderr, "filter error! (%i)\n", f);
		break;
	default:rgb = rgb0;
		break;
	}
	size_t r = ot == IMG_tga ? rgb2tga(out, rgb, ri) :
		   ot == IMG_ppm || ot == IMG_pgm ? rgb2ppm(out, rgb, ri) :
		   IF_PNG(ot == IMG_png ? rgb2png(out, rgb, ri) :)
		   IF_JPG(ot == IMG_jpg ? rgb2jpg(out, rgb, ri) :)
		   IF_LL1(ot == IMG_ll1 ? rgb2ll1(out, rgb, ri) :)
		   ot == IMG_raw ? ss_size(ss_cpy(out, rgb)) :
		   ot == IMG_none ? rgb_info(rgb, ri) : 0;
	ss_free(&rgb_aux);
	return r;
}


/*
 * imgtools.c
 *
 * Image processing example using libsrt
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 *
 * Observations:
 * - In order to have PNG and JPEG support: make imgc HAS_JPG=1 HAS_PNG=1
 */

#include "imgtools.h"

enum ImgTypes file_type(const char *file_name)
{
	size_t l = file_name ? strlen(file_name) : 0, sf = l > 3 ? l - 3 : 0;
	return !sf ? IMG_error :
	       !strncmp(file_name + sf, "tga", 3) ? IMG_tga :
	       !strncmp(file_name + sf, "ppm", 3) ? IMG_ppm :
	       !strncmp(file_name + sf, "pgm", 3) ? IMG_pgm :
	       IF_PNG(!strncmp(file_name + sf, "png", 3) ? IMG_png :)
	       IF_JPG(!strncmp(file_name + sf, "jpg", 3) ? IMG_jpg :)
	       IF_JPG(!strncmp(file_name + sf - 1, "jpeg", 4) ? IMG_jpg :)
	       !strncmp(file_name + sf, "raw", 3) ? IMG_raw : IMG_error;
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
	RETURN_IF(!s || !t, S_FALSE);
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
	rgbi_set(ri, S_LD_LE_U16(t + TGA_W), S_LD_LE_U16(t + TGA_H),
		 (size_t)t[TGA_BPP], t[TGA_TYPE] == TGA_RAW_GRAY ? 1 :
							(size_t)t[TGA_BPP]/8);
	RETURN_IF(!rgbi_valid(ri), 0);
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
	RETURN_IF(!rgb || (!rgbi_valid(ri) && (ri->bpp / ri->chn) != 8), 0);
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

static sbool_t rgbi_valid_for_ppm(const struct RGB_Info *ri)
{
	RETURN_IF(!rgbi_valid(ri), S_FALSE);
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
			i = ss_findrcx(ppm, i, nl2, '0', '9'); /* digits */
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
	rgbi_set(ri, f[0], f[1], ri->bpp, ri->chn);
	RETURN_IF(!rgbi_valid(ri), 0);
	ss_cpy_cn(rgb, p + off, ps - off); /* Copy pixel data */
	return ss_size(*rgb);
}

static size_t rgb2ppm(ss_t **ppm, const ss_t *rgb, const struct RGB_Info *ri)
{
	RETURN_IF(!ppm || !rgb || !rgbi_valid_for_ppm(ri), 0);
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

static size_t aux_png_get_chn(const png_structp s, const png_infop pi)
{
	RETURN_IF(!s, S_FALSE);
	int ct = png_get_color_type(s, pi);
	sbool_t alpha = (ct & PNG_COLOR_MASK_ALPHA) != 0 ? S_TRUE : S_FALSE;
	sbool_t gray = (ct & PNG_COLOR_MASK_COLOR) == 0 ? S_TRUE : S_FALSE;
	return (gray ? 1 : 3) + (alpha ? 1 : 0);
}

static size_t aux_png_get_depth(const png_structp s, const png_infop pi)
{
	RETURN_IF(!s, 0);
	return (png_get_color_type(s, pi) & PNG_COLOR_MASK_PALETTE) != 0 ?
		8 : png_get_bit_depth(s, pi);
}

static sbool_t aux_png2rbi(const png_structp s, const png_infop pi,
			   struct RGB_Info *ri)
{
	RETURN_IF(!s || !ri, S_FALSE);
	size_t depth = aux_png_get_depth(s, pi), chn = aux_png_get_chn(s, pi);
	rgbi_set(ri, png_get_image_width(s, pi), png_get_image_height(s, pi),
		 depth * chn, chn);
	return rgbi_valid(ri);
}

static size_t aux_png_row_size(const png_structp s, const png_infop pi)
{
	return aux_png_get_chn(s, pi) * (png_get_image_width(s, pi) *
	       aux_png_get_depth(s, pi)) / 8;
}

static sbool_t aux_png_set_rows(png_bytep *rows, const png_structp s,
				const png_infop pi, const char *rgb)
{
	RETURN_IF(!rows || !pi || !rgb, S_FALSE);
	size_t i = 0, rs = aux_png_row_size(s, pi);
	for (; i < png_get_image_height(s, pi); i++)
		rows[i] = (png_bytep)(rgb + i * rs);
	return S_TRUE;
}

static sbool_t aux_png_read_set_rows(png_bytep *rows, const png_structp s,
				     const png_infop pi, ss_t **rgb)
{
	RETURN_IF(!rows || !pi || !rgb, S_FALSE);
	size_t rs = aux_png_row_size(s, pi),
	       rgb_size = rs * png_get_image_height(s, pi);
	RETURN_IF(ss_reserve(rgb, rgb_size) < rgb_size, S_FALSE);
	ss_set_size(*rgb, rgb_size);
	return aux_png_set_rows(rows, s, pi, ss_get_buffer(*rgb));
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
	if (!aux_png2rbi(s, pi, ri)) {
		png_destroy_info_struct(s, &pi);
		png_destroy_read_struct(&s, NULL, NULL);
		return 0;
	}
	if ((png_get_color_type(s, pi) & PNG_COLOR_MASK_PALETTE) != 0)
		png_set_expand(s); /* pal: expand to RGB */
	png_bytep *rows = (png_bytep *)alloca(sizeof(png_bytep) *
			  png_get_image_height(s, pi));
	if (aux_png_read_set_rows(rows, s, pi, rgb)) {
		size_t i = png_set_interlace_handling(s);
		for (; i > 0; i--)
			png_read_rows(s, rows, NULL,
				      png_get_image_height(s, pi));
		png_read_end(s, pi);
		out_size = ss_size(*rgb);
	}
	png_destroy_info_struct(s, &pi);
	png_destroy_read_struct(&s, NULL, NULL);
	return out_size;
}

static size_t rgb2png(ss_t **png, const ss_t *rgb, const struct RGB_Info *ri)
{
	RETURN_IF(!rgbi_valid(ri) ||!rgb, 0);
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
	png_bytep *rows = (png_bytep *)alloca(sizeof(png_bytep) *
			  png_get_image_height(s, pi));
	if (aux_png_set_rows(rows, s, pi, ss_get_buffer_r(rgb))) {
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

static void jpg_error_exit(j_common_ptr /*jc*/)
{
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
	rgbi_set(ri, jd->output_width, jd->output_height,
		 jd->out_color_components * 8, jd->out_color_components);
	RETURN_IF(!rgbi_valid(ri), S_FALSE);
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
	RETURN_IF(!ja || !rgbi_valid(ri) || !rgb || !jpg_out, S_FALSE);
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

sbool_t rgbi_valid_for_jpg(const struct RGB_Info *ri)
{
	RETURN_IF(!rgbi_valid(ri), S_FALSE);
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
	RETURN_IF(!rgbi_valid_for_jpg(ri) ||!rgb, 0);
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
		const size_t ss = ss_size(rgb);
		size_t cmax = ri->bpp == 32 ? 0xffffffff :
					      0xffffffff &
					      ((1 << ri->bpp) - 1);
		size_t i, uqp = 0;
		const char *p0 = ss_get_buffer_r(rgb);
		const unsigned char *p = (const unsigned char *)p0;
		sb_t *bs = sb_alloc(0);
		sb_eval(&bs, cmax);
		switch (ri->Bpp) {
		case 4: RGB_COUNT_LOOP(i, p, ss, 4, bs, cmax, uqp); break;
		case 3: RGB_COUNT_LOOP(i, p, ss, 3, bs, cmax, uqp); break;
		case 2: RGB_COUNT_LOOP(i, p, ss, 2, bs, cmax, uqp); break;
		case 1: RGB_COUNT_LOOP(i, p, ss, 1, bs, cmax, uqp); break;
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
		if (ri->Bpp == 3 || ri->Bpp == 4) {
			size_t ur, ug, ub, ua, urg, ugb, urb;
			RGBC_COUNT_LOOP(i, p, ss, ri->Bpp, bs, 0, ur);
			RGBC_COUNT_LOOP(i, p, ss, ri->Bpp, bs, 0, ug);
			RGBC_COUNT_LOOP(i, p, ss, ri->Bpp, bs, 0, ub);
			RGBC2_COUNT_LOOP(i, p, ss, ri->Bpp, bs, 0, urg);
			RGBC2_COUNT_LOOP(i, p, ss, ri->Bpp, bs, 1, ugb);
			RGBC2_COUNT_LOOP(i, p, ss, ri->Bpp, bs, 2, urb);
			if (ri->Bpp == 4) {
				RGBC_COUNT_LOOP(i, p, ss, ri->Bpp, bs, 0, ua);
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

size_t any2rgb(ss_t **rgb, struct RGB_Info *ri, const ss_t *in,
	      enum ImgTypes it)
{
	return it == IMG_tga ? tga2rgb(rgb, ri, in) :
	       it == IMG_ppm || it == IMG_pgm ? ppm2rgb(rgb, ri, in) :
	       IF_PNG(it == IMG_png ? png2rgb(rgb, ri, in) :)
	       IF_JPG(it == IMG_jpg ? jpg2rgb(rgb, ri, in) :) 0;
}

size_t rgb2type(ss_t **out, enum ImgTypes ot, const ss_t *rgb0,
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
		   ot == IMG_raw ? ss_size(ss_cpy(out, rgb)) :
		   ot == IMG_none ? rgb_info(rgb, ri) : 0;
	ss_free(&rgb_aux);
	return r;
}

#define CFF2 (char)0xff
#define CFF4 (short)0xffff

int rgbdiff(ss_t **out, const ss_t *rgb0, const struct RGB_Info *ri0,
	    const ss_t *rgb1, const struct RGB_Info *ri1)
{
	RETURN_IF(!out || !rgb0 || !rgb1 || !rgbi_cmp(ri0, ri1), 1);
	ss_reserve(out, ri0->bmp_size);
	ss_set_size(*out, ri0->bmp_size);
	RETURN_IF(ss_size(*out) != ri0->bmp_size, 2);
	int res = 0;
	const char *c0 = ss_get_buffer_r(rgb0), *c1 = ss_get_buffer_r(rgb1);
	char *c2 = ss_get_buffer(*out);
	const short *s0 = (const short *)ss_get_buffer_r(rgb0),
		    *s1 = (const short *)ss_get_buffer_r(rgb1);
	short *s2 = (short *)ss_get_buffer(*out);
	size_t i = 0;
	const size_t mx = ri0->Bpc == 1 ? ri0->bmp_size : ri0->bmp_size / 2;
	switch (ri0->Bpp) {
	case 1:	INLINE_MONO_DIFF_LOOP(c0, c1, c2, CFF2, i, mx, res); break;
	case 2:	INLINE_MONO_DIFF_LOOP(s0, s1, s2, CFF4, i, mx, res); break;
	case 3:	INLINE_RGB_DIFF_LOOP(c0, c1, c2, CFF2, i, mx, res); break;
	case 4:	INLINE_RGBA_DIFF_LOOP(c0, c1, c2, CFF2, i, mx, res); break;
	case 6:	INLINE_RGB_DIFF_LOOP(s0, s1, s2, CFF4, i, mx, res); break;
	case 8:	INLINE_RGBA_DIFF_LOOP(s0, s1, s2, CFF4, i, mx, res); break;
	default: res = 1; break;
	}
	return res;
}


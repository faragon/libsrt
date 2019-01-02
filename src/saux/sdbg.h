#ifndef SDBG_H
#define SDBG_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * sdbg.h
 *
 * Debug helpers (data formatting, etc.).
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 *
 * Observations:
 * - This is intended for debugging.
 */

#include "../shmap.h"
#include "../smap.h"
#include "../sstring.h"
#include "../svector.h"
#include "stree.h"

typedef srt_string *(*ss_cat_stn)(srt_string **s, const srt_tnode *n, const srt_tndx id);

const char *sv_type_to_label(enum eSV_Type t);
void sv_log_obj(srt_string **log, const srt_vector *v);
void st_log_obj(srt_string **log, const srt_tree *t, ss_cat_stn f);
void sm_log_obj(srt_string **log, const srt_map *m);
void shm_log_obj(srt_string **log, const srt_hmap *h);
void s_hex_dump(srt_string **log, const char *label, const char *buf, size_t buf_size);

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif /* SDBG_H */

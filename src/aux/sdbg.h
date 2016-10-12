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
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 *
 * Observations:
 * - This is intended for debugging.
 */

#include "../sstring.h"
#include "../svector.h"
#include "../smap.h"
#include "stree.h"

typedef ss_t *(*ss_cat_stn)(ss_t **s, const stn_t *n, const stndx_t id);

const char *sv_type_to_label(const enum eSV_Type t);
void sv_log_obj(ss_t **log, const sv_t *v);
void st_log_obj(ss_t **log, const st_t *t, ss_cat_stn f);
void sm_log_obj(ss_t **log, const sm_t *m);
void s_hex_dump(ss_t **log, const char *label, const char *buf, const size_t buf_size);

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif  /* SDBG_H */


#ifndef SHASH_H
#define SHASH_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * shash.h
 *
 * Buffer hashing
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 *
 * Features:
 *
 * - Currently only a simple 32-bit checksum is implemented (used in the
 *   routing hash in sdmap.c)
 */

#include "scommon.h"

unsigned sh_csum32(const void *buf, const size_t buf_size);

#ifdef __cplusplus
};      /* extern "C" { */
#endif
#endif	/* SHASH_H */


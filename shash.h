#ifndef SENC_H
#define SENC_H
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

unsigned sh_csum32(const void *buf, const size_t buf_size);

#ifdef __cplusplus
};      /* extern "C" { */
#endif
#endif	/* SENC_H */


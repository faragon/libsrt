#ifndef SBITIO_H
#define SBITIO_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * sbitio.h
 *
 * Bit I/O on memory buffers.
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "scommon.h"

struct SBitIO
{
	unsigned char *bw;
	const unsigned char *br;
	size_t off, acc, accbuf;
};

typedef struct SBitIO sbio_t;

/* #notAPI: |Bit stream byte offset|bit I/O struct|offset (bytes)|O(1)|1;2| */
S_INLINE size_t sbio_off(sbio_t *bio)
{
	return bio->off;
}

/* #notAPI: |Initialize bit I/O for writing|bit I/O struct; write buffer||O(1)|1;2| */
void sbio_write_init(sbio_t *bio, unsigned char *b);

/* #notAPI: |Write code|bit I/O struct; code; code size (bits)||O(n)|1;2| */
void sbio_write(sbio_t *bio, size_t c, size_t cbits);

/* #notAPI: |Finish write|bit I/O struct|written bytes|O(1)|1;2| */
size_t sbio_write_close(sbio_t *bio);

/* #notAPI: |Initialize bit I/O for reading|bit I/O struct; write buffer||O(1)|1;2| */
void sbio_read_init(sbio_t *bio, const unsigned char *b);

/* #notAPI: |Read code|bit I/O struct; code size (bits)|code read|O(n)|1;2| */
size_t sbio_read(sbio_t *bio, size_t code_bits);

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif	/* SBITIO_H */


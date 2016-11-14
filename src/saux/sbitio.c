/*
 * sbitio.c
 *
 * Bit I/O on memory buffers.
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "sbitio.h"

void sbio_write_init(sbio_t *bio, unsigned char *b)
{
	bio->acc = bio->off = 0;
	bio->bw = b;
}

void sbio_write(sbio_t *bio, size_t c, size_t cbits)
{
	unsigned char *b = bio->bw;
	size_t xbits = 8 - bio->acc;
	if (!bio->acc)
		b[bio->off] = (unsigned char)(c << bio->acc);
	else
		b[bio->off] |= (c << bio->acc);
	if (cbits >= xbits) {
		bio->off++;
		c >>= xbits;
		cbits -= xbits;
		size_t copy_size = cbits / 8;
		for (; copy_size > 0;) {
			b[bio->off++] = (unsigned char)c;
			c >>= 8;
			copy_size--;
		}
		if (cbits % 8) {
			b[bio->off] = (unsigned char)c;
			bio->acc = (cbits % 8);
		} else {
			bio->acc = 0;
		}
	} else {
		bio->acc += cbits;
	}
}

size_t sbio_write_close(sbio_t *bio)
{
	if (bio->acc) {
		bio->off++;
		bio->acc = 0;
	}
	return bio->off;
}

void sbio_read_init(sbio_t *bio, const unsigned char *b)
{
	bio->acc = bio->off = bio->accbuf = 0;
	bio->br = b;
}

size_t sbio_read(sbio_t *bio, size_t code_bits)
{
	size_t code = 0;
	const unsigned char *b = bio->br;
	if (bio->acc) {
		code |= bio->accbuf;
		code_bits -= bio->acc;
	}
	for (; code_bits >= 8;) {
		code |= ((size_t)b[bio->off++] << bio->acc);
		code_bits -= 8;
		bio->acc += 8;
	}
	if (code_bits > 0) {
		bio->accbuf = b[bio->off++];
		code |= ((bio->accbuf & S_NBITMASK(code_bits)) << bio->acc);
		bio->accbuf >>= code_bits;
		bio->acc = 8 - code_bits;
	} else {
		bio->acc = 0;
	}
	return code;
}


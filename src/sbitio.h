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
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 *
 */

#include "scommon.h"

S_INLINE void sbitio_write_init(size_t *acc)
{
	*acc = 0;
}

S_INLINE void sbitio_write(unsigned char *b, size_t *i, size_t *acc, size_t c, size_t cbits)
{
	if (*acc) {
		size_t xbits = 8 - *acc;
		b[(*i)++] |= (c << *acc);
		c >>= xbits;
		cbits -= xbits;
	}
	size_t copy_size = cbits / 8;
	switch (copy_size) {
	case 3: b[(*i)++] = (unsigned char)c; c >>= 8;
	case 2: b[(*i)++] = (unsigned char)c; c >>= 8;
	case 1: b[(*i)++] = (unsigned char)c; c >>= 8;
	}
	*acc = cbits % 8;
	if (*acc)
		b[*i] = (unsigned char)c;
}

S_INLINE void sbitio_write_close(unsigned char *b, size_t *i, size_t *acc)
{
	if (*acc)
		b[++*i] = 0;
	*acc = 0;
}

S_INLINE void sbitio_read_init(size_t *acc, size_t *accbuf)
{
	*acc = *accbuf = 0;
}

S_INLINE size_t sbitio_read(const unsigned char *b, size_t *i, size_t *acc, size_t *accbuf, size_t code_bits)
{
	size_t code = 0;
	if (*acc) {
		code |= *accbuf;
		code_bits -= *acc;
	}
	if (code_bits >= 8) {
		code |= ((size_t)b[(*i)++] << *acc);
		code_bits -= 8;
		*acc += 8;
	}
	if (code_bits > 0) {
		*accbuf = b[(*i)++];
		code |= ((*accbuf & S_NBITMASK(code_bits)) << *acc);
		*accbuf >>= code_bits;
		*acc = 8 - code_bits;
	} else {
		*acc = 0;
	}
	return code;
}

#ifdef __cplusplus
};      /* extern "C" { */
#endif
#endif	/* SBITIO_H */


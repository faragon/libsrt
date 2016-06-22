/*
 * shash.c
 *
 * Buffer hashing
 *
 * Copyright (c) 2015-2016 F. Aragon. All rights reserved.
 */

#include "shash.h"

/*
 * Functions
 */

unsigned sh_csum32(const void *buf, const size_t buf_size)
{
	RETURN_IF(!buf, 0);
	const size_t buf_size_m4 = (buf_size / 4) * 4;
	const char *p = (const char *)buf,
		   *p_top = p + buf_size,
		   *pm4_top = p + buf_size_m4;
	unsigned acc = 0;
	for (; p < pm4_top; p += 4)
		acc ^= S_LD_U32(p); /* allow unaligned access */
	if (p < p_top) { /* fill last element with zeros */
		unsigned tail = 0;
		memcpy(&tail, p, p_top - p);
		acc ^= tail;
	}
	return acc;
}

#ifdef STANDALONE_TEST
int main(int argc, const char **argv)
{
	int i = 0;
	size_t lo = 0;
	char buf[128 * 1024];
	int f_in = 0;
	ssize_t l = read(f_in, buf, sizeof(buf));
	unsigned hash = sh_csum32(buf, l);
	printf("%u", hash);
	return 0;
}
#endif


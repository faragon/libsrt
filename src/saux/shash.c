/*
 * shash.c
 *
 * Buffer hashing
 *
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "shash.h"

/*
 * Constants
 */

#define S_CRC32_POLY 0xedb88320
#define S_FNV_PRIME ((uint32_t)0x01000193)
#define MH3_32_C1 0xcc9e2d51
#define MH3_32_C2 0x1b873593

/*
 * CRC-32 implementations
 */

#ifndef S_BUILD_CRC32_TABLES

#if defined(S_MINIMAL) || (defined(S_CRC32_SLC) && S_CRC32_SLC == 0)

/*
 * Compact implementation, without hash tables (one bit per loop)
 */
uint32_t sh_crc32(uint32_t crc, const void *buf, size_t buf_size)
{
	RETURN_IF(!buf, S_CRC32_INIT);
	const uint8_t *p = (const uint8_t *)buf;
	size_t i, j;
	crc = ~crc;
	for (i = 0; i < buf_size; i++) {
		crc ^= p[i];
		for (j = 0; j < 8; j++) {
			int b0 = crc & 1;
			crc >>= 1;
			if (b0)
				crc ^= S_CRC32_POLY;
		}
	}
	return ~crc;
}

#else

#include "scrc32.h"

#define S_U32_BYTE0(a) ((a)&0xff)
#define S_U32_BYTE1(a) (((a) >> 8) & 0xff)
#define S_U32_BYTE2(a) (((a) >> 16) & 0xff)
#define S_U32_BYTE3(a) (((a) >> 24) & 0xff)
#if S_CRC32_SLC == 4
#define SC32A a
#elif S_CRC32_SLC == 8
#define SC32A b
#define SC32B a
#elif S_CRC32_SLC == 12
#define SC32A c
#define SC32B b
#define SC32C a
#else
#define SC32A d
#define SC32B c
#define SC32C b
#define SC32D a
#endif

/*
 * 1, 4, 8, 12, and 16 bytes per loop using 1024 to 16384 bytes table
 */
uint32_t sh_crc32(uint32_t crc, const void *buf, size_t buf_size)
{
#if S_CRC32_SLC >= 4
	size_t bsX;
#endif
	size_t i;
	const uint8_t *p;
	RETURN_IF(!buf, S_CRC32_INIT);
	i = 0;
	p = (const uint8_t *)buf;
	crc = ~crc;
#if (S_CRC32_SLC == 4 || S_CRC32_SLC == 8 || S_CRC32_SLC == 12                 \
     || S_CRC32_SLC == 16)
	bsX = (buf_size / S_CRC32_SLC) * S_CRC32_SLC;
	for (; i < bsX; i += S_CRC32_SLC) {
		uint32_t a = S_LD_LE_U32(p + i) ^ crc
#if S_CRC32_SLC >= 8
			,
			 b = S_LD_LE_U32(p + i + 4)
#endif
#if S_CRC32_SLC >= 12
				 ,
			 c = S_LD_LE_U32(p + i + 8)
#endif
#if S_CRC32_SLC == 16
				 ,
			 d = S_LD_LE_U32(p + i + 12)
#endif
			;
		crc = crc32_tab[0][S_U32_BYTE3(SC32A)]
		      ^ crc32_tab[1][S_U32_BYTE2(SC32A)]
		      ^ crc32_tab[2][S_U32_BYTE1(SC32A)]
		      ^ crc32_tab[3][S_U32_BYTE0(SC32A)]
#if S_CRC32_SLC >= 8
		      ^ crc32_tab[4][S_U32_BYTE3(SC32B)]
		      ^ crc32_tab[5][S_U32_BYTE2(SC32B)]
		      ^ crc32_tab[6][S_U32_BYTE1(SC32B)]
		      ^ crc32_tab[7][S_U32_BYTE0(SC32B)]
#endif
#if S_CRC32_SLC >= 12
		      ^ crc32_tab[8][S_U32_BYTE3(SC32C)]
		      ^ crc32_tab[9][S_U32_BYTE2(SC32C)]
		      ^ crc32_tab[10][S_U32_BYTE1(SC32C)]
		      ^ crc32_tab[11][S_U32_BYTE0(SC32C)]
#endif
#if S_CRC32_SLC == 16
		      ^ crc32_tab[12][S_U32_BYTE3(SC32D)]
		      ^ crc32_tab[13][S_U32_BYTE2(SC32D)]
		      ^ crc32_tab[14][S_U32_BYTE1(SC32D)]
		      ^ crc32_tab[15][S_U32_BYTE0(SC32D)]
#endif
			;
	}
#endif
	for (; i < buf_size; i++)
		crc = crc32_tab[0][(crc ^ p[i]) & 0xff] ^ (crc >> 8);
	return ~crc;
}

#endif /* #ifdef S_MINIMAL */

#define ADLER32_BASE 65521 /* Largest prime below 2^16 */
#define ADLER32_NMAX 5552

uint32_t sh_adler32(uint32_t adler, const void *buf0, size_t buf_size)
{
	uint32_t s1, s2;
	size_t remaining, k;
	const unsigned char *buf;
	RETURN_IF(!buf0, S_ADLER32_INIT);
	remaining = buf_size;
	buf = (const unsigned char *)buf0;
	s1 = (adler & 0xffff);
	s2 = (adler >> 16) & 0xffff;
	for (; remaining > 0; s1 %= ADLER32_BASE, s2 %= ADLER32_BASE) {
		k = remaining < ADLER32_NMAX ? remaining : ADLER32_NMAX;
		remaining -= k;
		for (; k >= 16; buf += 16, k -= 16) {
			s1 += buf[0];
			s2 += s1;
			s1 += buf[1];
			s2 += s1;
			s1 += buf[2];
			s2 += s1;
			s1 += buf[3];
			s2 += s1;
			s1 += buf[4];
			s2 += s1;
			s1 += buf[5];
			s2 += s1;
			s1 += buf[6];
			s2 += s1;
			s1 += buf[7];
			s2 += s1;
			s1 += buf[8];
			s2 += s1;
			s1 += buf[9];
			s2 += s1;
			s1 += buf[10];
			s2 += s1;
			s1 += buf[11];
			s2 += s1;
			s1 += buf[12];
			s2 += s1;
			s1 += buf[13];
			s2 += s1;
			s1 += buf[14];
			s2 += s1;
			s1 += buf[15];
			s2 += s1;
		}
		for (; k > 0; k--, s1 += *buf++, s2 += s1)
			;
	}
	return (s2 << 16) | s1;
}

uint32_t sh_fnv1(uint32_t fnv, const void *buf0, size_t buf_size)
{
	size_t i;
	const uint8_t *buf = (const uint8_t *)buf0;
	for (i = 0; i < buf_size; i++) {
		fnv *= S_FNV_PRIME;
		fnv ^= buf[i];
	}
	return fnv;
}

uint32_t sh_fnv1a(uint32_t fnv, const void *buf0, size_t buf_size)
{
	size_t i;
	const uint8_t *buf = (const uint8_t *)buf0;
	for (i = 0; i < buf_size; i++) {
		fnv ^= buf[i];
		fnv *= S_FNV_PRIME;
	}
	return fnv;
}

S_INLINE uint32_t rotl32(uint32_t x, int r)
{
	return (x << r) | (x >> (32 - r));
}

uint32_t sh_mh3_32(uint32_t acc, const void *buf, size_t buf_size)
{
	uint32_t h = acc, k;
	size_t i, l4 = (buf_size / 4) * 4;
	const uint8_t *data = (const uint8_t *)buf;
	/* body: 4 bytes per loop */
	for (i = 0; i < l4; i += 4) {
		k = S_LD_LE_U32(data + i) * MH3_32_C1;
		k = rotl32(k, 15) * MH3_32_C2;
		h = (rotl32(h ^ k, 13) * 5) + 0xe6546b64;
	}
	/* tail */
	k = 0;
	switch (buf_size & 3) {
	case 3:
		k ^= (uint32_t)(data[i + 2] << 16);
		/* fallthrough */
	case 2:
		k ^= (uint32_t)(data[i + 1] << 8);
		/* fallthrough */
	case 1:
		k ^= data[i];
		k *= MH3_32_C1;
		k = rotl32(k, 15) * MH3_32_C2;
		h ^= k;
		break;
	}
	/* avalanche */
	h ^= buf_size;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

#else

/*
 * gcc shash.c -DS_BUILD_CRC32_TABLES -o a ; ./a >scrc32.h
 */
int main(int argc, const char **argv)
{
	/*
	 * Generation of scrc32.h content
	 *
	 * Learned from:
	 * https://web.archive.org/web/20121011093914/http://www.intel.com
	 *	          /technology/comms/perfnet/download/CRC_generators.pdf
	 * http://create.stephan-brumme.com/crc32/
	 */
	size_t i, j;
	uint32_t c32t[16][256];
	for (i = 0; i < 256; i++) {
		uint32_t crc = i;
		for (j = 0; j < 8; j++) {
			int b0 = crc & 1;
			crc >>= 1;
			if (b0)
				crc ^= S_CRC32_POLY;
		}
		c32t[0][i] = crc;
	}
	for (j = 1; j < 16; j++) {
		for (i = 0; i < 256; i++) {
#define C32L(s)                                                                \
	c32t[s][i] = (c32t[s - 1][i] >> 8) ^ c32t[0][c32t[s - 1][i] & 0xff];
			C32L(1);
			C32L(2);
			C32L(3);
			C32L(4);
			C32L(5);
			C32L(6);
			C32L(7);
			C32L(8);
			C32L(9);
			C32L(10);
			C32L(11);
			C32L(12);
			C32L(13);
			C32L(14);
			C32L(15);
		}
	}
	/*
	 * Flush table to the output
	 */
	printf("/*\n"
	       " * scrc32.h\n"
	       " *\n"
	       " * Precomputed CRC-32 for polynomial 0x%08x\n"
	       " * (gcc shash.c -DS_BUILD_CRC32_TABLES -o a; ./a >scrc32.h)\n"
	       " *\n"
	       " * " LIBSRT_COPYRIGHT
	       "\n"
	       " * " LIBSRT_LICENSE
	       "\n"
	       " */\n\n"
	       "#ifndef SCRC32_H\n"
	       "#define SCRC32_H\n\n"
	       "#ifndef S_CRC32_SLC\n"
	       "#define S_CRC32_SLC 12\n"
	       "#else\n"
	       "#if S_CRC32_SLC != 0 && S_CRC32_SLC != 1 && "
	       "S_CRC32_SLC != 4 && \\\n"
	       "    S_CRC32_SLC != 8 && S_CRC32_SLC != 12 && "
	       "S_CRC32_SLC != 16\n"
	       "#undef S_CRC32_SLC /* if invalid slice size, default to 1 */\n"
	       "#define S_CRC32_SLC 1\n"
	       "#endif\n"
	       "#endif\n\n"
	       "#if S_CRC32_SLC > 0\n\n"
	       "static const uint32_t crc32_tab[S_CRC32_SLC][256] = {\n",
	       S_CRC32_POLY);
	int rows = 6;
	for (i = 0; i < 16; i++) {
		printf("\t{\n\t");
		for (j = 0; j < 256; j++) {
			printf("0x%08x", (unsigned)c32t[i][j]);
			if (j < 255) {
				if ((j % rows) == (rows - 1))
					printf(",\n\t");
				else
					printf(", ");
			}
		}
		printf("\n\t}");
		switch (i) {
		case 0:
			printf("\n#if S_CRC32_SLC >= 4\n\t,");
			break;
		case 3:
			printf("\n#endif /*#if S_CRC32_SLC >= 4*/");
			printf("\n#if S_CRC32_SLC >= 8\n\t,");
			break;
		case 7:
			printf("\n#endif /*#if S_CRC32_SLC >= 8*/");
			printf("\n#if S_CRC32_SLC >= 12\n\t,");
			break;
		case 11:
			printf("\n#endif /*#if S_CRC32_SLC >= 12*/");
			printf("\n#if S_CRC32_SLC >= 16\n\t,");
			break;
		case 15:
			printf("\n#endif /*#if S_CRC32_SLC >= 16*/\n");
			break;
		default:
			printf(",");
		}
		printf("\n");
	}
	printf("};\n\n"
	       "#endif /*#if S_CRC32_SLC > 0*/\n\n"
	       "#endif /* #ifdef SCRC32_H */\n\n");
}

#endif /* #ifndef S_BUILD_CRC32_TABLES */

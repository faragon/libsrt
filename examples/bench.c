/*
 * bench.c
 *
 * Some libsrt string benchmarks (this is quick and weird stuff, it will be
 * changed for "orthodox" benchmaks -or removed completely-)
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 */

#include <libsrt.h>

/*
 * Search benchmark parameters
 */

const char *xa[4] = {
	"111111x1111113111111111111111111111111111111111111111111111111111111111111111111111111111111141111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111112k1",
	"11111111111111111112k1",
	"abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcddcbadcbadcbadcba",
	"45____________________________________________________________________________________________________________________________12345678901234567890123y_123456781234567812345678123456781234567812345678123456781234567812345678_______12345678901234567890123y______12345678901234567890123y45678901234567890123456789012345678901234567890123x456789012k"
	};

const char *xb[4] = {
	"1111111112k1",
	"112k1",
	"dcba",
	"_______________________________________________________________________________________________________________123"
	};

/*
 * Case benchmark parameters
 */

const char *xc[4] = {
	"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
	"\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91" "\xc3\x91",
	"\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac" "\xe2\x82\xac",
	"\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2" "\xf0\xa4\xad\xa2"
	};

#define BENCH_GROUPS		3
#define BENCH_ALGS_PER_GROUP	7

const char *groups[BENCH_GROUPS] = { "search", "case conversion", "misc" };
const char *algs[BENCH_GROUPS][BENCH_ALGS_PER_GROUP] = {
		{ "ss_find", "ss_find_csum_fast", "ss_find_csum_slow",
#ifndef S_MINIMAL_BUILD
		  "ss_find_libc", "ss_find_bf", "ss_find_bmh",
#else
		  "", "", "",
#endif
		  ""
		},
		{ "ss_tolower", "ss_toupper", "towlower", "sc_tolower", "towupper", "sc_toupper", "" },
		{ "ss_len_u", "", "", "", "", "", "" }
	};

int bench_search(const int alg, const int param1, const int count)
{
	int res = 0;
	const char *ba = NULL, *bb = NULL;
	char *ba1 = NULL;
	switch (param1) {
	case 0: case 1: case 2: case 3:
		ba = (char *)xa[param1];
		bb = xb[param1];
		break;
	case 4: {
			const size_t ba_s = 100000;
			ba1 = (char *)malloc(ba_s + 1);
			memset(ba1, '0', ba_s);
			ba1[ba_s] = 0;
			ba1[0] = ba1[ba_s - 2] = '1';
			ba1[ba_s - 1] = '2';
			bb = "12";
			ba = ba1;
		}
		break;
	case 5:
		{
			const size_t max_size = 100 * 1024 * 1024;
			ba1 = (char *)malloc(max_size);
			int f = open("100.txt", O_RDONLY, 0777);
			if (f < 0)
				exit(1);
			int l = read(f, ba1, max_size);
			close(f);
			ba1[l - 1] = 0;
			bb = "the profoundest";
			ba = ba1;
		}
		break;
	}
	ss_t *a = ss_dup_c(ba), *b = ss_dup_c(bb);
	size_t j = 0, ssa = ss_len(a), ssb = ss_len(b);
	switch ((ba && bb) ? alg : -1) {
		case 0:	for (; j < count; ss_find(a, 0, b), j++);
			break;
		case 1:	for (; j < count; ss_find_csum_fast(ba, 0, ssa, bb, ssb), j++);
			break;
		case 2:	for (; j < count; ss_find_csum_slow(ba, 0, ssa, bb, ssb), j++);
			break;
#ifndef S_MINIMAL_BUILD
		case 3:	for (; j < count; ss_find_libc(ba, 0, ssa, bb, ssb), j++);
			break;
		case 4:	for (; j < count; ss_find_bf(ba, 0, ssa, bb, ssb), j++);
			break;
		case 5:	for (; j < count; ss_find_bmh(ba, 0, ssa, bb, ssb), j++);
			break;
#endif
		default:
			res = 1;	/* error */
	}
	ss_free(&a, &b);
	free(ba1);
	return res;
}

int bench_case(const int alg, const int param1, const int count)
{
	int res = 0;
	size_t c = 0, x = 0, y = 0;
	const char *in = param1 < 4 ? xc[param1] : "";
	ss_t *sa = ss_dup_c(in);
	switch (param1 < 4 ? alg : -1) {
	case 0:
		ss_tolower(&sa);
		printf("'%s' -> '%s'\n", in, ss_to_c(sa));
		for (; c < count; c++)
		{
			ss_t *sb = ss_dup_c(in);
			ss_tolower(&sb);
			ss_free(&sb);
		}
		break;
	case 1:
		ss_toupper(&sa);
		printf("'%s' -> '%s'\n", in, ss_to_c(sa));
		for (; c < count; c++)
		{
			ss_t *sb = ss_dup_c(in);
			ss_toupper(&sb);
			ss_free(&sb);
		}
		break;
	case 2:
		for (; y < count; y++)
			for (x = 0; x < 0x1ffff; x++)
				towlower(x);
		break;
	case 3:
		for (; y < count; y++)
			for (x = 0; x < 0x1ffff; x++)
				sc_tolower(x);
		break;
	case 4:
		for (; y < count; y++)
			for (x = 0; x < 0x1ffff; x++)
				towupper(x);
		break;
	case 5:
		for (; y < count; y++)
			for (x = 0; x < 0x1ffff; x++)
				sc_toupper(x);
		break;
	default:
		res = 1;
	}
	ss_free(&sa);
	return res;
}

int bench_misc(const int alg, const int param1, const int count)
{
	int res = 0, c = 0;
	const char *in = param1 < 4 ? xc[param1] : "";
	ss_t *sa = ss_dup_c(in);
	switch (alg) {
	case 0:
		printf("ss_len_u('%s'): %zu\n", in, ss_len_u(sa));
		for (; c < count; ss_len_u(sa), c++);
		break;
	default:
		res = 1;
	}
	ss_free(&sa);
	return res;
}

int main(int argc, char **argv)
{
	if (argc == 5) {
		int grp = atoi(argv[1]), alg = atoi(argv[2]), param = atoi(argv[3]), count0 = atoi(argv[4]);
		int count = count0 > 0 ? count0 : 0;
		if (grp >= 0 && grp < BENCH_GROUPS && alg >= 0 &&
		    alg < BENCH_ALGS_PER_GROUP && algs[grp][alg] &&
		    algs[grp][alg][0]) {
			fprintf(stderr, "%s, count: %i\n", algs[grp][alg], count);
			switch (grp) {
				case 0:	return bench_search(alg, param, count);
				case 1:	return bench_case(alg, param, count);
				case 2: return bench_misc(alg, param, count);
			}
		}
	}
	fprintf(stderr, "Syntax: %s bench_group_id alg_id param_id count\n"
			"(groups: 0..%i, algorithms: 0..%i, count 0..2147483647)\n",
			argv[0], BENCH_GROUPS, BENCH_ALGS_PER_GROUP);
	int i = 0, j;
	for (; i < BENCH_GROUPS; i++) {
		fprintf(stderr, "\tgroup %i: %s\n", i, groups[i]);
		for (j = 0; j < BENCH_ALGS_PER_GROUP; j++) {
			if (!algs[i][j]) /* no more algs */
				break;
			if (!algs[i][j][0]) /* alg with empty name: skip */
				continue;
			fprintf(stderr, "\t\t algorithm %i: %s\n", j, algs[i][j]);
		}
	}
	return 1;
}




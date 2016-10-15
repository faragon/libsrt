#ifndef SCONFIG_H
#define SCONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * sconfig.h
 *
 * Third party interface configuration.
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

/*
 * Heap allocation
 */

#ifndef S_MALLOC
	#define _s_malloc malloc
#endif
#ifndef S_CALLOC
	#define _s_calloc calloc
#endif
#ifndef S_REALLOC
	#define _s_realloc realloc
#endif
#ifndef S_FREE
	#define _s_free free
#endif

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif	/* SCONFIG_H */


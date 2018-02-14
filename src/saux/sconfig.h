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
 * Copyright (c) 2015-2018 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
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

/*
 * For covering the case of errors masked because of the realloc function
 * returning the same base address, a mechanism for detecting those errors
 * is provided (used in the Valgrind test phase, protecting the build)
 */
#if defined(S_FORCE_REALLOC_COPY) && defined(_s_realloc) &&		\
    (defined(__GNUC__) || defined(__clang__) || defined(__TINYC__))
#include <malloc.h>
#include <string.h>
#undef _s_realloc
inline static void *_s_realloc(void *ptr, size_t size)
{
	void *ptr_next;
	size_t size_prev = malloc_usable_size(ptr),
	       size_next = size > size_prev ? size : size_prev;
	ptr_next = malloc(size_next);
	if (!ptr_next)
		return NULL;
	memcpy(ptr_next, ptr, size_prev);
	free(ptr);
	return ptr_next;
}
#endif

#ifdef __cplusplus
}      /* extern "C" { */
#endif
#endif	/* SCONFIG_H */


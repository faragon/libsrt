#ifndef STREE_H
#define STREE_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * stree.h
 *
 * Self-balancing sorted binary tree.
 *
 * Copyright (c) 2015 F. Aragon. All rights reserved.
 *
 * Features:
 * - See documentation for API reference.
 */

#include "sdata.h"

/*
 * Structures and types
 */

#ifdef S_TREE_HUGE /* Up to 2^63 - 1 nodes */
	#define ST_NODE_BITS 63
	#define ST_NIL	((((suint_t)1)<<ST_NODE_BITS) - 1)
	typedef unsigned long long stndx_t;
#else /* Up to 2^31 - 1 nodes */
	#if S_BPWORD >=4 
		#define ST_NODE_BITS 31
		#define ST_NIL	((((suint32_t)1)<<ST_NODE_BITS) - 1)
	#else
		#define ST_NODE_BITS (S_BPWORD * 8 - 1)
		#define ST_NIL	((((unsigned)1)<<ST_NODE_BITS) - 1)
	#endif
	typedef unsigned int stndx_t;
#endif

typedef int (*st_cmp_t)(const void *tree_node, const void *new_node);
typedef void(*stn_callback_t)(void *tree_node);

struct STConf
{
	size_t type;
	size_t node_size;
	st_cmp_t cmp;
	sint_t iaux1, iaux2;
	const void *paux1, *paux2;
};

struct S_Node
{
	struct {
		stndx_t is_red : 1;
		stndx_t l : ST_NODE_BITS;
	};
	struct {
		stndx_t unused : 1; /* TODO: not yet used */
		stndx_t r : ST_NODE_BITS;
	};
};

typedef struct S_Tree st_t;

struct S_Tree
{
	struct SData_Full df;		/* Resize handling */
	stndx_t root;
	struct STConf f;		/* Generic data functions */
};

typedef struct S_Node stn_t;

struct STraverseParams
{
	void *context;
	const st_t *t;
	stndx_t c;
	const stn_t *cn;
	ssize_t level;
	ssize_t max_level;
};

typedef int (*st_traverse)(const struct STraverseParams *p);

/*
* Constants
*/

#define EMPTY_STC { 0, 0, 0 }
#define EMPTY_ST { EMPTY_SData_Full(sizeof(st_t)), 0, EMPTY_STC }
#define EMPTY_STN { { 0, ST_NIL }, { 0, ST_NIL } }

/*
* Macros
*/

#define SDT_HEADER_SIZE sizeof(struct S_Tree)
#define ST_SIZE_TO_ALLOC_SIZE(elems, elem_size) \
	(SDT_HEADER_SIZE + elem_size * elems)

/*
 * Functions
 */

#define st_alloca(f, num_elems)				 		  \
	st_alloc_raw(f, S_TRUE, num_elems,				  \
		     alloca(ST_SIZE_TO_ALLOC_SIZE(num_elems, elem_size)), \
		     ST_SIZE_TO_ALLOC_SIZE(num_elems, elem_size))
st_t *st_alloc_raw(const struct STConf *f, const sbool_t ext_buf,
		   const size_t num_elems, void *buffer,
		   const size_t buffer_size);
st_t *st_alloc(const struct STConf *f, const size_t initial_reserve);

SD_BUILDPROTS(st)

#define st_free(...) st_free_aux(S_NARGS_STPW(__VA_ARGS__), __VA_ARGS__)

/*
 * Operations
 */

st_t *st_dup(const st_t *t);
sbool_t st_insert(st_t **t, const stn_t *n);
sbool_t st_delete(st_t *t, const stn_t *n, stn_callback_t callback);
const stn_t *st_locate(const st_t *t, const stn_t *n);
const stn_t *st_enum(const st_t *t, const stndx_t index);
ssize_t st_traverse_preorder(const st_t *t, st_traverse f, void *context);
ssize_t st_traverse_inorder(const st_t *t, st_traverse f, void *context);
ssize_t st_traverse_postorder(const st_t *t, st_traverse f, void *context);
ssize_t st_traverse_levelorder(const st_t *t, st_traverse f, void *context);
sbool_t st_assert(const st_t *t);

#ifdef __cplusplus
}; /* extern "C" { */
#endif

#endif /* #ifndef STREE_H */


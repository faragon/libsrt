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
	typedef suint_t stndx_t;
#else /* Up to 2^31 - 1 nodes */
	#if S_BPWORD >=4 
		#define ST_NODE_BITS 31
		#define ST_NIL	((((suint32_t)1)<<ST_NODE_BITS) - 1)
	#else
		#define ST_NODE_BITS (S_BPWORD * 8 - 1)
		#define ST_NIL	((((unsigned)1)<<ST_NODE_BITS) - 1)
	#endif
	typedef suint32_t stndx_t;
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
	} x;
	stndx_t r;
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

typedef int (*st_traverse)(struct STraverseParams *p);

/*
* Constants
*/

#define EMPTY_STC { 0, 0, 0, 0, 0, 0, 0 }
#define EMPTY_ST { EMPTY_SData_Full(sizeof(st_t)), 0, EMPTY_STC }
#define EMPTY_STN { { 0, ST_NIL }, ST_NIL }

/*
* Macros
*/

#define SDT_HEADER_SIZE sizeof(struct S_Tree)
#define ST_SIZE_TO_ALLOC_SIZE(elems, elem_size) \
	(SDT_HEADER_SIZE + elem_size * elems)

/*
 * Functions
 */

/*
#API: |Allocate tree (stack)|tree configuration;space preallocated to store n elements|allocated tree|O(1)|
sv_t *st_alloca(const struct STConf *f, const size_t initial_reserve)
*/
#define st_alloca(f, num_elems)				 		  \
	st_alloc_raw(f, S_TRUE,						  \
		     alloca(ST_SIZE_TO_ALLOC_SIZE(num_elems, elem_size)), \
		     ST_SIZE_TO_ALLOC_SIZE(num_elems, elem_size))
st_t *st_alloc_raw(const struct STConf *f, const sbool_t ext_buf,
		   void *buffer, const size_t buffer_size);

/* #API: |Allocate tree (heap)|tree configuration;space preallocated to store n elements|allocated tree|O(1)| */
st_t *st_alloc(const struct STConf *f, const size_t initial_reserve);

SD_BUILDPROTS(st)

/*
#API: |Free one or more trees (heap)|tree;more trees (optional)||O(1)|
void st_free(st_t **c, ...)

#API: |Ensure space for extra elements|tree;number of extra eelements|extra size allocated|O(1)|
size_t st_grow(st_t **c, const size_t extra_elems)

#API: |Ensure space for elements|tree;absolute element reserve|reserved elements|O(1)|
size_t st_reserve(st_t **c, const size_t max_elems)

#API: |Free unused space|tree|same tree (optional usage)|O(1)|
st_t *st_shrink(st_t **c)

#API: |Get tree size|tree|number of tree nodes|O(1)|
size_t st_size(const st_t *c)

#API: |Set tree size (for integer-only trees) |tree;set tree number of elements||O(1)|
void st_set_size(st_t *c, const size_t s)

#API: |Equivalent to st_size|tree|number of tree nodes|O(1)|
size_t st_len(const st_t *c)
*/

#define st_free(...) st_free_aux(S_NARGS_STPW(__VA_ARGS__), __VA_ARGS__)

/*
 * Operations
 */

/* #API: |Duplicate tree|tree|output tree|O(n)| */
st_t *st_dup(const st_t *t);

/* #API: |Insert element into tree|tree; element to insert|S_TRUE: OK, S_FALSE: error (not enough memory)|O(log n)| */
sbool_t st_insert(st_t **t, const stn_t *n);

/* #API: |Delete tree element|tree; element to delete; node delete handling callback (optional if e.g. nodes use no extra dynamic memory references)|S_TRUE: found and deleted; S_FALSE: not found|O(log n)| */
sbool_t st_delete(st_t *t, const stn_t *n, stn_callback_t callback);

/* #API: |Locate node|tree; node|Reference to the located node; NULL if not found|O(log n)| */
const stn_t *st_locate(const st_t *t, const stn_t *n);

/* #API: |Fast unsorted enumeration|tree; element, 0 to n - 1, being n the number of elements|Reference to the located node; NULL if not found|O(1)| */
stn_t *st_enum(st_t *t, const stndx_t index);

/* #API: |Fast unsorted enumeration (read-only)|tree; element, 0 to n - 1, being n the number of elements|Reference to the located node; NULL if not found|O(1)| */
const stn_t *st_enum_r(const st_t *t, const stndx_t index);

/* #API: |Full tree traversal: pre-order|tree; traverse callback; callback context|Number of levels stepped down|O(n)| */
ssize_t st_traverse_preorder(const st_t *t, st_traverse f, void *context);

/* #API: |Full tree traversal: in-order|tree; traverse callback; callback context|Number of levels stepped down|O(n)| */
ssize_t st_traverse_inorder(const st_t *t, st_traverse f, void *context);

/* #API: |Full tree traversal: post-order|tree; traverse callback; callback context|Number of levels stepped down|O(n)| */
ssize_t st_traverse_postorder(const st_t *t, st_traverse f, void *context);

/* #API: |Bread-first tree traversal|tree; traverse callback; callback contest|Number of levels stepped down|O(n); Aux space: n/2 * sizeof(stndx_t)| */
ssize_t st_traverse_levelorder(const st_t *t, st_traverse f, void *context);

/* #API: |Tree check (debug purposes)|tree|S_TREE: OK, S_FALSE: breaks RB tree rules|O(n)| */
sbool_t st_assert(const st_t *t);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* #ifndef STREE_H */


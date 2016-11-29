#ifndef STREE_H
#define STREE_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * stree.h
 *
 * #SHORTDOC self-balancing binary tree
 *
 * #DOC Balanced tree functions. Tree is implemented as Red-Black tree,
 * #DOC with up to 2^31 nodes. Internal representation is intended for
 * #DOC tight memory usage, being implemented as a vector, so pinter
 * #DOC usage is avoided.
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 *
 */

#include "sdata.h"

/*
 * Structures and types
 */

#define ST_NODE_BITS	31
#define ST_NIL		((((uint32_t)1)<<ST_NODE_BITS) - 1)
#define ST_NDX_MAX	(ST_NIL - 1)

typedef uint32_t stndx_t;

typedef int (*st_cmp_t)(const void *tree_node, const void *new_node);
typedef void(*stn_callback_t)(void *tree_node);

struct S_Node
{
	struct {
		stndx_t is_red : 1;
		stndx_t l : ST_NODE_BITS;
	} x;
	stndx_t r;
};

struct S_Tree
{
	struct SDataFull d;
	stndx_t root;
	st_cmp_t cmp_f;
};

typedef struct S_Node stn_t;
typedef struct S_Tree st_t;

struct STraverseParams
{
	void *context;
	const st_t *t;
	stndx_t c;
	ssize_t level;
	ssize_t max_level;
};

typedef int (*st_traverse)(struct STraverseParams *p);
typedef void (*st_rewrite_t)(stn_t *node, const stn_t *new_data, const sbool_t existing);

/*
* Constants
*/

#define EMPTY_STN { { 0, ST_NIL }, ST_NIL }

/*
 * Functions
 */

/*
#NOTAPI: |Allocate tree (stack)|node compare function; node size; space preallocated to store n elements|allocated tree|O(1)|0;2|
sv_t *st_alloca(st_cmp_t cmp_f, const size_t elem_size, const size_t max_size)
*/
#define st_alloca(cmp_f, elem_size, max_size)				\
	st_alloc_raw(cmp_f, S_TRUE,					\
		     alloca(sd_alloc_size_raw(sizeof(st_t), elem_size,	\
					      max_size)),		\
		     elem_size, max_size)

st_t *st_alloc_raw(st_cmp_t cmp_f, const sbool_t ext_buf,
		   void *buffer, const size_t elem_size, const size_t max_size);

/* #NOTAPI: |Allocate tree (heap)|compare function;element size;space preallocated to store n elements|allocated tree|O(1)|1;2| */
st_t *st_alloc(st_cmp_t cmp_f, const size_t elem_size, const size_t init_size);

SD_BUILDFUNCS_FULL(st, 0)

/*
#NOTAPI: |Free one or more trees (heap)|tree;more trees (optional)|-|O(1)|1;2|
void st_free(st_t **t, ...)

#NOTAPI: |Ensure space for extra elements|tree;number of extra eelements|extra size allocated|O(1)|0;2|
size_t st_grow(st_t **t, const size_t extra_elems)

#NOTAPI: |Ensure space for elements|tree;absolute element reserve|reserved elements|O(1)|0;2|
size_t st_reserve(st_t **t, const size_t max_elems)

#NOTAPI: |Free unused space|tree|same tree (optional usage)|O(1)|0;2|
st_t *st_shrink(st_t **t)

#NOTAPI: |Get tree size|tree|number of tree nodes|O(1)|0;2|
size_t st_size(const st_t *t)

#NOTAPI: |Set tree size (for integer-only trees) |tree;set tree number of elements|-|O(1)|0;2|
void st_set_size(st_t *t, const size_t s)

#NOTAPI: |Equivalent to st_size|tree|number of tree nodes|O(1)|1;2|
size_t st_len(const st_t *t)
*/

#define st_free(...) st_free_aux(__VA_ARGS__, S_INVALID_PTR_VARG_TAIL)

/*
 * Operations
 */

/* #NOTAPI: |Duplicate tree|tree|output tree|O(n)|0;2| */
st_t *st_dup(const st_t *t);

/* #NOTAPI: |Insert element into tree|tree; element to insert|S_TRUE: OK, S_FALSE: error (not enough memory)|O(log n)|1;2| */
sbool_t st_insert(st_t **t, const stn_t *n);

/* #NOTAPI: |Insert element into tree, with rewrite function (in case of key already written)|tree; element to insert; rewrite function (if NULL it will behave like st_insert()|S_TRUE: OK, S_FALSE: error (not enough memory)|O(log n)|1;2| */
sbool_t st_insert_rw(st_t **t, const stn_t *n, const st_rewrite_t rw_f);

/* #NOTAPI: |Delete tree element|tree; element to delete; node delete handling callback (optional if e.g. nodes use no extra dynamic memory references)|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
sbool_t st_delete(st_t *t, const stn_t *n, stn_callback_t callback);

/* #NOTAPI: |Locate node|tree; node|Reference to the located node; NULL if not found|O(log n)|1;2| */
const stn_t *st_locate(const st_t *t, const stn_t *n);

/* #NOTAPI: |Full tree traversal: pre-order|tree; traverse callback; callback context|Number of levels stepped down|O(n)|1;2| */
ssize_t st_traverse_preorder(const st_t *t, st_traverse f, void *context);

/* #NOTAPI: |Full tree traversal: in-order|tree; traverse callback; callback context|Number of levels stepped down|O(n)|1;2| */
ssize_t st_traverse_inorder(const st_t *t, st_traverse f, void *context);

/* #NOTAPI: |Full tree traversal: post-order|tree; traverse callback; callback context|Number of levels stepped down|O(n)|1;2| */
ssize_t st_traverse_postorder(const st_t *t, st_traverse f, void *context);

/* #NOTAPI: |Bread-first tree traversal|tree; traverse callback; callback contest|Number of levels stepped down|O(n); Aux space: n/2 * sizeof(stndx_t)|1;2| */
ssize_t st_traverse_levelorder(const st_t *t, st_traverse f, void *context);

/*
 * Other
 */

/* #NOTAPI: |Tree check (debug purposes)|tree|S_TREE: OK, S_FALSE: breaks RB tree rules|O(n)|1;2| */
sbool_t st_assert(const st_t *t);

/*
 * Inlined functions
 */

S_INLINE stn_t *get_node(st_t *t, const stndx_t node_id)
{
	RETURN_IF(node_id == ST_NIL, NULL);
	return (stn_t *)st_elem_addr(t, node_id);
}

S_INLINE const stn_t *get_node_r(const st_t *t, const stndx_t node_id)
{
	RETURN_IF(node_id == ST_NIL, NULL);
	return (const stn_t *)st_elem_addr_r(t, node_id);
}

/* #NOTAPI: |Fast unsorted enumeration|tree; element, 0 to n - 1, being n the number of elements|Reference to the located node; NULL if not found|O(1)|0;2| */
S_INLINE stn_t *st_enum(st_t *t, const stndx_t index)
{
	ASSERT_RETURN_IF(!t, NULL);
	return get_node(t, index);
}

/* #NOTAPI: |Fast unsorted enumeration (read-only)|tree; element, 0 to n - 1, being n the number of elements|Reference to the located node; NULL if not found|O(1)|0;2| */
S_INLINE const stn_t *st_enum_r(const st_t *t, const stndx_t index)
{
	ASSERT_RETURN_IF(!t, NULL);
	return get_node_r(t, index);
}

/*
 * Structure required for tree expansion from
 * other types (e.g. sm_t)
 */

enum STreeScanState {
	STS_ScanStart	= 0,
	STS_ScanLeft	= 1,
	STS_ScanRight	= 2,
	STS_ScanDone	= 3
};

struct STreeScan {
	stndx_t p, c;		/* parent, current */
	enum STreeScanState s;
};

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* #ifndef STREE_H */


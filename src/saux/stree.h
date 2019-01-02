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
 * Copyright (c) 2015-2019 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "sdata.h"

/*
 * Structures and types
 */

#define ST_NODE_BITS 31
#define ST_NIL ((((uint32_t)1) << ST_NODE_BITS) - 1)
#define ST_NDX_MAX (ST_NIL - 1)

typedef uint32_t srt_tndx;

typedef int (*srt_cmp)(const void *tree_node, const void *new_node);
typedef void (*srt_tree_callback)(void *tree_node);

struct S_Node {
	struct {
		srt_tndx is_red : 1;
		srt_tndx l : ST_NODE_BITS;
	} x;
	srt_tndx r;
};

struct S_Tree {
	struct SDataFull d;
	srt_tndx root;
	srt_cmp cmp_f;
};

typedef struct S_Node srt_tnode;
typedef struct S_Tree srt_tree;

struct STraverseParams {
	void *context;
	const srt_tree *t;
	srt_tndx c;
	ssize_t level;
	ssize_t max_level;
};

typedef int (*st_traverse)(struct STraverseParams *p);
typedef void (*srt_tree_rewrite)(srt_tnode *node, const srt_tnode *new_data,
				 srt_bool existing);

/*
 * Constants
 */

#define EMPTY_STN { { 0, ST_NIL }, ST_NIL }

/*
 * Functions
 */

/*
#NOTAPI: |Allocate tree (stack)|node compare function; node size; space preallocated to store n elements|allocated tree|O(1)|0;2|
srt_vector *st_alloca(srt_cmp cmp_f, size_t elem_size, size_t max_size)
*/
#define st_alloca(cmp_f, elem_size, max_size)				\
	st_alloc_raw(cmp_f, S_TRUE,					\
		     s_alloca(sd_alloc_size_raw(sizeof(srt_tree), elem_size,	\
					      max_size)),		\
		     elem_size, max_size)

srt_tree *st_alloc_raw(srt_cmp cmp_f, srt_bool ext_buf,
		   void *buffer, size_t elem_size, size_t max_size);

/* #NOTAPI: |Allocate tree (heap)|compare function;element size;space preallocated to store n elements|allocated tree|O(1)|1;2| */
srt_tree *st_alloc(srt_cmp cmp_f, size_t elem_size, size_t init_size);

SD_BUILDFUNCS_FULL(st, srt_tree, 0)

/*
#NOTAPI: |Free one or more trees (heap)|tree;more trees (optional)|-|O(1)|1;2|
void st_free(srt_tree **t, ...)

#NOTAPI: |Ensure space for extra elements|tree;number of extra eelements|extra size allocated|O(1)|0;2|
size_t st_grow(srt_tree **t, size_t extra_elems)

#NOTAPI: |Ensure space for elements|tree;absolute element reserve|reserved elements|O(1)|0;2|
size_t st_reserve(srt_tree **t, size_t max_elems)

#NOTAPI: |Free unused space|tree|same tree (optional usage)|O(1)|0;2|
srt_tree *st_shrink(srt_tree **t)

#NOTAPI: |Get tree size|tree|number of tree nodes|O(1)|0;2|
size_t st_size(const srt_tree *t)

#NOTAPI: |Set tree size (for integer-only trees) |tree;set tree number of elements|-|O(1)|0;2|
void st_set_size(srt_tree *t, size_t s)

#NOTAPI: |Equivalent to st_size|tree|number of tree nodes|O(1)|1;2|
size_t st_len(const srt_tree *t)
*/

#ifdef S_USE_VA_ARGS
#define st_free(...) st_free_aux(__VA_ARGS__, S_INVALID_PTR_VARG_TAIL)
#else
#define st_free(t) st_free_aux(t, S_INVALID_PTR_VARG_TAIL)
#endif

/*
 * Operations
 */

/* #NOTAPI: |Duplicate tree|tree|output tree|O(n)|0;2| */
srt_tree *st_dup(const srt_tree *t);

/* #NOTAPI: |Insert element into tree|tree; element to insert|S_TRUE: OK, S_FALSE: error (not enough memory)|O(log n)|1;2| */
srt_bool st_insert(srt_tree **t, const srt_tnode *n);

/* #NOTAPI: |Insert element into tree, with rewrite function (in case of key already written)|tree; element to insert; rewrite function (if NULL it will behave like st_insert()|S_TRUE: OK, S_FALSE: error (not enough memory)|O(log n)|1;2| */
srt_bool st_insert_rw(srt_tree **t, const srt_tnode *n, srt_tree_rewrite rw_f);

/* #NOTAPI: |Delete tree element|tree; element to delete; node delete handling callback (optional if e.g. nodes use no extra dynamic memory references)|S_TRUE: found and deleted; S_FALSE: not found|O(log n)|1;2| */
srt_bool st_delete(srt_tree *t, const srt_tnode *n, srt_tree_callback callback);

/* #NOTAPI: |Locate node|tree; node|Reference to the located node; NULL if not found|O(log n)|1;2| */
const srt_tnode *st_locate(const srt_tree *t, const srt_tnode *n);

/* #NOTAPI: |Full tree traversal: pre-order|tree; traverse callback; callback context|Number of levels stepped down|O(n)|1;2| */
ssize_t st_traverse_preorder(const srt_tree *t, st_traverse f, void *context);

/* #NOTAPI: |Full tree traversal: in-order|tree; traverse callback; callback context|Number of levels stepped down|O(n)|1;2| */
ssize_t st_traverse_inorder(const srt_tree *t, st_traverse f, void *context);

/* #NOTAPI: |Full tree traversal: post-order|tree; traverse callback; callback context|Number of levels stepped down|O(n)|1;2| */
ssize_t st_traverse_postorder(const srt_tree *t, st_traverse f, void *context);

/* #NOTAPI: |Bread-first tree traversal|tree; traverse callback; callback contest|Number of levels stepped down|O(n); Aux space: n/2 * sizeof(srt_tndx)|1;2| */
ssize_t st_traverse_levelorder(const srt_tree *t, st_traverse f, void *context);

/*
 * Other
 */

/* #NOTAPI: |Tree check (debug purposes)|tree|S_TREE: OK, S_FALSE: breaks RB tree rules|O(n)|1;2| */
srt_bool st_assert(const srt_tree *t);

/*
 * Inlined functions
 */

S_INLINE srt_tnode *get_node(srt_tree *t, srt_tndx node_id)
{
	RETURN_IF(node_id == ST_NIL, NULL);
	return (srt_tnode *)st_elem_addr(t, node_id);
}

S_INLINE const srt_tnode *get_node_r(const srt_tree *t, srt_tndx node_id)
{
	RETURN_IF(node_id == ST_NIL, NULL);
	return (const srt_tnode *)st_elem_addr_r(t, node_id);
}

/* #NOTAPI: |Fast unsorted enumeration|tree; element, 0 to n - 1, being n the number of elements|Reference to the located node; NULL if not found|O(1)|0;2| */
S_INLINE srt_tnode *st_enum(srt_tree *t, srt_tndx index)
{
	ASSERT_RETURN_IF(!t, NULL);
	return get_node(t, index);
}

/* #NOTAPI: |Fast unsorted enumeration (read-only)|tree; element, 0 to n - 1, being n the number of elements|Reference to the located node; NULL if not found|O(1)|0;2| */
S_INLINE const srt_tnode *st_enum_r(const srt_tree *t, srt_tndx index)
{
	ASSERT_RETURN_IF(!t, NULL);
	return get_node_r(t, index);
}

/*
 * Structure required for tree expansion from
 * other types (e.g. srt_map)
 */

enum STreeScanState {
	STS_ScanStart,
	STS_ScanLeft,
	STS_ScanRight,
	STS_ScanDone
};

struct STreeScan {
	srt_tndx p, c; /* parent, current */
	enum STreeScanState s;
};

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* #ifndef STREE_H */

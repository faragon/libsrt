/*
 * stree.c
 *
 * Self-balancing sorted binary tree.
 *
 * Observations:
 * - Using indexes instead of pointers.
 * - Not using parent references.
 * - Linear space addressing.
 * - The reason of the above points is to reduce memory usage, increase cache
 *   hit ratio, reducing dynamic allocation space and time overhead, and being
 *   suitable for memory-mapped storage when using nodes without references
 *   (e.g. key and value being integers, or constant-size containers).
 *
 * Copyright (c) 2015-2020 F. Aragon. All rights reserved.
 * Released under the BSD 3-Clause License (see the doc/LICENSE)
 */

#include "stree.h"
#include "../svector.h"
#include "scommon.h"

/*
 * Helper macros
 */

#define STN_SET_RBB(t, cn)                                                     \
	{                                                                      \
		cn->x.is_red = S_TRUE;                                         \
		set_red(t, cn->x.l, S_FALSE);                                  \
		set_red(t, cn->r, S_FALSE);                                    \
	}

/*
 * Constants
 */

#define CW_SIZE 4 /* Node context window size (must be 4) */

enum STNDir { ST_Left = 0, ST_Right = 1 };

#define st_void (srt_tree *)sd_void

/*
 * Internal data structures
 */

struct NodeContext {
	srt_tndx x;   /* Index to the memory slot */
	srt_tnode *n; /* Pointer to the node (base_ptr + x * sizeof(node)) */
};

/*
 * Internal functions
 */

S_INLINE void set_lr(srt_tnode *n, enum STNDir d, srt_tndx v)
{
	if (n) {
		if (d == ST_Left)
			n->x.l = v;
		else
			n->r = v;
	}
}

S_INLINE srt_tndx get_lr(const srt_tnode *n, enum STNDir d)
{
	return d == ST_Left ? n->x.l : n->r;
}

S_INLINE srt_tnode *locate_parent(srt_tree *t, const struct NodeContext *son,
				  enum STNDir *d)
{
	srt_tndx lr;
	srt_tnode *cn;
	if (t->root == son->x)
		return son->n;
	cn = get_node(t, t->root);
	for (; cn && cn->x.l != son->x && cn->r != son->x;) {
		lr = get_lr(cn, t->cmp_f(cn, son->n) < 0 ? ST_Right : ST_Left);
		cn = get_node(t, lr);
	}
	*d = cn && cn->x.l == son->x ? ST_Left : ST_Right;
	return cn;
}

S_INLINE void update_node_data(const srt_tree *t, srt_tnode *tgt,
			       const srt_tnode *src)
{
	size_t node_header_size = sizeof(srt_tnode),
	       copy_size = t->d.elem_size - node_header_size;
	char *tgtp = (char *)tgt + node_header_size;
	const char *srcp = (const char *)src + node_header_size;
	memcpy(tgtp, srcp, copy_size);
}

S_INLINE void copy_node(const srt_tree *t, srt_tnode *tgt, const srt_tnode *src)
{
	memcpy(tgt, src, t->d.elem_size);
}

S_INLINE void new_node(const srt_tree *t, srt_tnode *tgt, const srt_tnode *src,
		       srt_bool ir, srt_tree_rewrite rw_f, srt_bool existing)
{
	if (!rw_f)
		update_node_data(t, tgt, src);
	else
		rw_f(tgt, src, existing);
	tgt->x.l = tgt->r = ST_NIL;
	tgt->x.is_red = ir;
}

S_INLINE srt_bool is_red(const srt_tree *t, srt_tndx node_id)
{
	RETURN_IF(node_id == ST_NIL, S_FALSE);
	return get_node_r(t, node_id)->x.is_red;
}

S_INLINE void set_red(srt_tree *t, srt_tndx node_id, srt_bool red)
{
	if (node_id != ST_NIL)
		get_node(t, node_id)->x.is_red = red;
}

/* counter-direction */
S_INLINE enum STNDir cd(enum STNDir d)
{
	return d == ST_Left ? ST_Right : ST_Left;
}

	/*
	 * Node rotation auxiliary functions
	 */

#define F_rotate1X                                                             \
	srt_tndx y = get_lr(xn, xd);                                           \
	srt_tnode *yn = get_node(t, y);                                        \
	set_lr(xn, xd, get_lr(yn, d));                                         \
	set_lr(yn, d, x);                                                      \
	set_red(t, x, S_TRUE);                                                 \
	set_red(t, y, S_FALSE);

S_INLINE srt_tndx rot1x(srt_tree *t, srt_tnode *xn, srt_tndx x, enum STNDir d,
			enum STNDir xd)
{
	F_rotate1X;
	return y;
}

S_INLINE void rot1x_p(srt_tree *t, srt_tnode *xn, srt_tndx x, enum STNDir d,
		      enum STNDir xd, srt_tnode *xpn)
{
	F_rotate1X;
	set_lr(xpn, d, y);
}

S_INLINE srt_tnode *rot1x_y(srt_tree *t, srt_tnode *xn, srt_tndx x,
			    enum STNDir d, enum STNDir xd, srt_tndx *y_out)
{
	F_rotate1X;
	*y_out = y;
	return yn;
}

S_INLINE srt_tndx rot2x(srt_tree *t, srt_tnode *xn, srt_tndx x, enum STNDir d,
			enum STNDir xd)
{
	srt_tndx child = get_lr(xn, xd);
	srt_tnode *child_node = get_node(t, child);
	rot1x_p(t, child_node, child, xd, d, xn);
	return rot1x(t, xn, x, d, xd);
}

/*
 * If current node is the tree root node, change the root index so it targets
 * the new node.
 */
S_INLINE void st_checkfix_root(srt_tree *t, srt_tndx pivot, srt_tndx new_pivot)
{
	if (pivot == t->root) {
		t->root = new_pivot;
		/*set_red(t, t->root, S_FALSE);*/
	}
}

/*
 * Observation: *recursive* function. This is intended for debug-only
 * purposes (for tree validation tests).
 * Returns: 0: error, > 0: tree height from a given node
 */
static size_t st_assert_aux(const srt_tree *t, srt_tndx ndx)
{
	size_t l, r;
	const srt_tnode *n;
	RETURN_IF(!t, 0);
	RETURN_IF(ndx == ST_NIL, 1);
	n = get_node_r(t, ndx);
	if (is_red(t, ndx) && (is_red(t, n->x.l) || is_red(t, n->r))) {
#ifdef DEBUG_stree
		fprintf(stderr, "st_assert: violation: two red nodes\n");
#endif
		return 0;
	}
	if (n->x.l != ST_NIL && t->cmp_f(get_node_r(t, n->x.l), n) >= 0
	    && n->r != ST_NIL && t->cmp_f(get_node_r(t, n->r), n) <= 0) {
#ifdef DEBUG_stree
		fprintf(stderr, "st_assert: tree structure violation\n");
#endif
		return 0;
	}
	l = st_assert_aux(t, n->x.l);
	r = st_assert_aux(t, n->r);
	if (l && r) {
		if (l == r)
			return is_red(t, ndx) ? l : l + 1;
#ifdef DEBUG_stree
		fprintf(stderr, "st_assert: height mismatch l %u r %u\n",
			(unsigned)l, (unsigned)r);
#endif
		return 0;
	}
	return 0;
}

/*
 * Allocation
 */

srt_tree *st_alloc_raw(srt_cmp cmp_f, srt_bool ext_buf, void *buffer,
		       size_t elem_size, size_t max_size)
{
	srt_tree *t;
	RETURN_IF(!cmp_f || !elem_size || !buffer, st_void);
	t = (srt_tree *)buffer;
	sd_reset((srt_data *)t, sizeof(srt_tree), elem_size, max_size, ext_buf,
		 S_FALSE);
	t->cmp_f = cmp_f;
	t->root = 0;
	return t;
}

srt_tree *st_alloc(srt_cmp cmp_f, size_t elem_size, size_t init_size)
{
	size_t alloc_size = sd_alloc_size_raw(sizeof(srt_tree), elem_size,
					      init_size, S_FALSE);
	void *buf = s_malloc(alloc_size);
	srt_tree *t = st_alloc_raw(cmp_f, S_FALSE, buf, elem_size, init_size);
	if (!t || t == st_void)
		s_free(buf);
	return t;
}

/*
 * Operations
 */

srt_tree *st_dup(const srt_tree *t)
{
	srt_tree *t2;
	RETURN_IF(!t, NULL);
	t2 = st_alloc(t->cmp_f, t->d.elem_size, t->d.size);
	RETURN_IF(!t2, NULL);
	memcpy(t2, t, t->d.header_size + t->d.size * t->d.elem_size);
	return t2;
}

srt_bool st_insert(srt_tree **tt, const srt_tnode *n)
{
	return st_insert_rw(tt, n, NULL);
}

srt_bool st_insert_rw(srt_tree **tt, const srt_tnode *n, srt_tree_rewrite rw_f)
{
	srt_tree *t;
	srt_tnode auxn = EMPTY_STN;
	size_t ts, c, cp, cpp, cppp;
	enum STNDir ld = ST_Left; /* last walk direction */
	enum STNDir d = ST_Left;  /* current walk direction */
	struct NodeContext w[CW_SIZE];
	srt_bool done = S_FALSE;
	enum STNDir xld;
	srt_tndx pd, v;
	int64_t cmp;
	/* BEHAVIOR: valid tree, with space for one extra element */
	RETURN_IF(!tt || !*tt || !n || !st_grow(tt, 1), S_FALSE);
	t = *tt;
	ts = st_size(t);
	/* BEHAVIOR: tree reaching capability limit */
	RETURN_IF(ts >= ST_NIL, S_FALSE);
	/*
	 * Trivial case: insert node into empty tree
	 */
	if (!ts) {
		srt_tnode *node = get_node(t, 0);
		new_node(t, node, n, S_FALSE, rw_f, S_FALSE);
		t->root = 0;
		st_set_size(t, 1);
		return S_TRUE;
	}
	/*
	 * Typical case: insert into non-empty tree
	 */
	/*
	 * Prepare a 4-level node tracking window
	 */
	auxn.r = t->root;
	/* c: current node (cn) */
	w[0].x = t->root;
	w[0].n = get_node(t, t->root);
	/* cppp: cn parent parent parent node */
	w[1].x = ST_NIL;
	w[1].n = &auxn;
	/* cpp: cn parent parent node */
	w[2].x = ST_NIL;
	w[2].n = NULL;
	/* cp: cn parent node */
	w[3].x = ST_NIL;
	w[3].n = NULL;
	c = 0;
	/*
	 * Search loop
	 */
	for (;;) {
		cp = (c + 3) % CW_SIZE;
		/* Leaf found? update tree, copy data, update size */
		if (w[c].x == ST_NIL) {
			/* New node: */
			w[c].x = (srt_tndx)ts;
			w[c].n = get_node(t, (srt_tndx)ts);
			new_node(t, w[c].n, n, S_TRUE, rw_f, S_FALSE);
			/* Update parent node: */
			set_lr(w[cp].n, d, w[c].x);
			if (get_lr(w[cp].n, cd(d)) != ST_NIL)
				w[c].n->x.is_red = S_TRUE;
			/* Ensure root is black: */
			set_red(t, t->root, S_FALSE);
			/* Increase tree size: */
			st_set_size(t, ts + 1);
			done = S_TRUE;
		} else {
			/* Two red sons? -> red parent + black sons */
			if (is_red(t, w[c].n->x.l) && is_red(t, w[c].n->r))
				STN_SET_RBB(t, w[c].n);
		}
		/* Check for double red case (current and parent are red) */
		cpp = (c + 2) % CW_SIZE;
		cppp = (c + 1) % CW_SIZE;
		if (w[cpp].n && w[c].n->x.is_red && w[cp].n->x.is_red) {
			xld = cd(ld);
			pd = get_lr(w[cp].n, ld);
			v = w[c].x == pd
				    ? rot1x(t, w[cpp].n, w[cpp].x, xld, ld)
				    : rot2x(t, w[cpp].n, w[cpp].x, xld, ld);
			if (w[cppp].n) {
				enum STNDir d2 = w[cppp].n->r == w[cpp].x
							 ? ST_Right
							 : ST_Left;
				set_lr(w[cppp].n, d2, v);
				st_checkfix_root(t, w[cpp].x, v);
			} else {
				t->root = v;
			}
		}
		if (done)
			break;
		cmp = t->cmp_f(w[c].n, n);
		if (!cmp) {
			if (rw_f)
				rw_f(w[c].n, n, S_TRUE);
			else
				update_node_data(t, w[c].n, n);
			break;
		}
		/* Step down: left or right */
		ld = d;
		d = cmp < 0 ? ST_Right : ST_Left;
		/* Node context window shift */
		w[cppp].x = get_lr(w[c].n, d);
		w[cppp].n = get_node(t, w[cppp].x);
		c = cppp;
	}
	return S_TRUE;
}

srt_bool st_delete(srt_tree *t, const srt_tnode *n, srt_tree_callback callback)
{
	size_t ts0;
	srt_tndx ts;
	srt_tnode auxn = EMPTY_STN;
	srt_tndx c, cp, cpp, cppp;
	struct NodeContext found = {ST_NIL, NULL};
	enum STNDir d0 = ST_Right;
	struct NodeContext w[CW_SIZE];
	int64_t cmp;
	enum STNDir d, ds, dt;
	srt_tndx nd;
	srt_tnode *ndn;
	srt_tndx y;
	enum STNDir xd, xd0, d2;
	srt_tndx s, sz;
	srt_tnode *yn, *sn, *cpp_d2n;
	/* BEHAVIOR: valid request */
	RETURN_IF(!t || !n, S_FALSE);
	/* Check empty tree: */
	ts0 = st_size(t);
	RETURN_IF(ts0 == 0 || ts0 >= ST_NIL, S_FALSE);
	ts = (srt_tndx)ts0;
	/*
	 * Prepare a 4-level node tracking window (in this case a 3-level
	 * would be enough, using 4 in order to avoid the division by 3,
	 * which is more expensive than by 4 in most CPUs).
	 */
	auxn.r = t->root;
	/* c: current node (cn) */
	w[0].x = t->root;
	w[0].n = get_node(t, t->root);
	/* cppp: cn parent parent parent node */
	w[1].x = ST_NIL;
	w[1].n = NULL;
	/* cpp: cn parent parent node */
	w[2].x = ST_NIL;
	w[2].n = NULL;
	/* cp: cn parent node */
	w[3].x = ST_NIL;
	w[3].n = &auxn;
	c = 0;
	cp = 3;
	cpp = 2;
	cppp = 1;
	/* Search loop */
	for (;;) {
		/* Compare node key with given target */
		cmp = t->cmp_f(w[c].n, n);
		d = cmp < 0 ? ST_Right : ST_Left;
		if (!cmp) {
			S_ASSERT(found.n == NULL);
			if (ts == 1) { /* Trivial case: one node */
				if (callback)
					callback((void *)w[c].n);
				st_set_size(t, 0);
				return S_TRUE;
			}
			found = w[c];
		}
		for (;;) {
			/* Push child red node down */
			nd = get_lr(w[c].n, d);
			ndn = get_node(t, nd);
			if (w[c].n->x.is_red || (ndn && ndn->x.is_red))
				break;
			xd = cd(d);
			if (is_red(t, get_lr(w[c].n, xd))) {
				yn = rot1x_y(t, w[c].n, w[c].x, d, xd, &y);
				if (w[cp].n)
					set_lr(w[cp].n, d0, y);
				/* Fix tree root if required: */
				st_checkfix_root(t, w[c].x, y);
				/* Update parent */
				w[cp].x = y;
				w[cp].n = yn;
				break;
			}
			/* s/sn: same-parent brother node */
			xd0 = cd(d0);
			s = get_lr(w[cp].n, xd0);
			if (s == ST_NIL)
				break;
			sn = get_node(t, s);
			if (!is_red(t, get_lr(sn, xd0))
			    && !is_red(t, get_lr(sn, d0))) {
				/* Color flip */
				set_red(t, w[cp].x, S_FALSE);
				set_red(t, s, S_TRUE);
				set_red(t, w[c].x, S_TRUE);
				break;
			}
			if (!w[cpp].n)
				break;
			d2 = w[cpp].n->r == w[cp].x ? ST_Right : ST_Left;
			if (is_red(t, get_lr(sn, d0))) {
				y = rot2x(t, w[cp].n, w[cp].x, d0, xd0);
				set_lr(w[cpp].n, d2, y);
				st_checkfix_root(t, w[cp].x, y);
			} else {
				if (is_red(t, get_lr(sn, xd0))) {
					y = rot1x(t, w[cp].n, w[cp].x, d0, xd0);
					set_lr(w[cpp].n, d2, y);
					st_checkfix_root(t, w[cp].x, y);
				}
			}
			cpp_d2n = get_node(t, get_lr(w[cpp].n, d2));
			/* Fix coloring */
			STN_SET_RBB(t, cpp_d2n);
			w[c].n->x.is_red = cpp_d2n->x.is_red;
			break;
		}
		w[cppp].x = get_lr(w[c].n, d);
		if (w[cppp].x == ST_NIL) /* bottom reached */
			break;
		/* Node context window shift */
		w[cppp].n = get_node(t, w[cppp].x);
		c = cppp;
		cp = (c + 3) % CW_SIZE;
		cpp = (c + 2) % CW_SIZE;
		cppp = (c + 1) % CW_SIZE;
		d0 = d;
	}
	if (found.n) {
		if (callback)
			callback((void *)found.n);
		/*
		 * Node found (the one to be removed) will be used to hold
		 * the last current node found. So shifting the last node to
		 * the location of node to be deleted balancin
		 */
		if (found.n != w[c].n) {
			/*copy_node_data(t, found.n, cn); ?????*/
			update_node_data(t, found.n, w[c].n);
			found.x = w[c].x;
		}
		if (!w[cp].n) { /* Root node deletion (???) */
			t->root =
				w[c].n->x.l != ST_NIL ? w[c].n->x.l : w[c].n->r;
		} else {
			ds = w[c].n->x.l == ST_NIL ? ST_Right : ST_Left;
			dt = w[cp].n->r == w[c].x ? ST_Right : ST_Left;
			set_lr(w[cp].n, dt, get_lr(w[c].n, ds));
		}
		/*
		 * If deleted node is not the last node in the linear space,
		 * in order to avoid fragmentation the last one will be
		 * moved to the deleted location. Despite this, time is
		 * kept into O(log n). Rationale: that's because not using
		 * dynamic memory for individual nodes, but a dynamic memory
		 * for a stack space.
		 */
		S_ASSERT(ts - 1 < ST_NIL);
		sz = ts - 1; /* BEHAVIOR */
		if (w[c].x != sz) {
			srt_tnode *fpn;
			struct NodeContext ct;
			enum STNDir dl = ST_Left;
			ct.x = sz;
			ct.n = get_node(t, sz);
			/* TODO: cache this (!) */
			fpn = locate_parent(t, &ct, &dl);
			if (fpn) {
				copy_node(t, w[c].n, ct.n);
				set_lr(fpn, dl, w[c].x);
				if (t->root == sz)
					t->root = w[c].x;
			} else {
				/* BEHAVIOR: this should never be reached */
				S_ASSERT(S_FALSE);
			}
		}
		st_set_size(t, ts - 1);
	}
	/* Set root node as black */
	set_red(t, t->root, S_FALSE);
	return found.n ? S_TRUE : S_FALSE;
}

const srt_tnode *st_locate(const srt_tree *t, const srt_tnode *n)
{
	int r;
	const srt_tnode *cn = get_node_r(t, t->root);
	for (;;)
		if (!(r = t->cmp_f(cn, n))
		    || !(cn = get_node_r(
				 t, get_lr(cn, r < 0 ? ST_Right : ST_Left))))
			break;
	return cn;
}

/*
 * Depth-first tree traversal
 */

enum eTMode { TR_Preorder, TR_Inorder, TR_Postorder };

/*
 * Iterative tree traverse using auxilliary stack.
 *
 * Time complexity: O(n)
 * Aux space: using the stack -i.e. "free"-, O(2 * log(n))
 */

#define RBT_MAX_DEPTH_LOG2 65

static ssize_t srt_treer_aux(const srt_tree *t, st_traverse f, void *context,
			     enum eTMode m)
{
	size_t ts;
	struct STreeScan p[2 * RBT_MAX_DEPTH_LOG2]; /* 2 * (log2(ts_max) + 1) */
	struct STraverseParams tp = {context, t, ST_NIL, 0, 0};
	int f_pre, f_ino, f_post;
	const srt_tnode *cn_aux;
	RETURN_IF(!t, -1);
	ts = st_size(t);
	RETURN_IF(!ts, S_FALSE);
	if (f)
		f(&tp);
	p[0].p = ST_NIL;
	p[0].c = t->root;
	p[0].s = STS_ScanStart;
	f_pre = f && m == TR_Preorder;
	f_ino = f && m == TR_Inorder;
	f_post = f && m == TR_Postorder;
	for (; tp.level >= 0;) {
		if (tp.level > tp.max_level)
			tp.max_level = tp.level;
		/*
		 * If having more levels than the expected 'rbt_max_depth', it
		 * would mean that there is some bug in the insert/delete
		 * rebalanzing code.
		 */
		S_ASSERT(tp.max_level < (ssize_t)(2 * (slog2(ts) + 1)));
		switch (p[tp.level].s) {
		case STS_ScanStart:
			if (f_pre) {
				tp.c = p[tp.level].c;
				f(&tp);
			}
			cn_aux = get_node_r(t, p[tp.level].c);
			if (cn_aux->x.l != ST_NIL) {
				p[tp.level].s = STS_ScanLeft;
				tp.level++;
				cn_aux = get_node_r(t, p[tp.level - 1].c);
				p[tp.level].c = cn_aux->x.l;
			} else {
				if (f_ino) {
					tp.c = p[tp.level].c;
					f(&tp);
				}
				if (cn_aux->r != ST_NIL) {
					p[tp.level].s = STS_ScanRight;
					tp.level++;
					cn_aux = get_node_r(t,
							    p[tp.level - 1].c);
					p[tp.level].c = cn_aux->r;
				} else {
					if (f_post) {
						tp.c = p[tp.level].c;
						f(&tp);
					}
					p[tp.level].s = STS_ScanDone;
					tp.level--;
					continue;
				}
			}
			p[tp.level].p = p[tp.level - 1].c;
			p[tp.level].s = STS_ScanStart;
			continue;
		case STS_ScanLeft:
			if (f_ino) {
				tp.c = p[tp.level].c;
				f(&tp);
			}
			cn_aux = get_node_r(t, p[tp.level].c);
			if (cn_aux->r != ST_NIL) {
				p[tp.level].s = STS_ScanRight;
				tp.level++;
				p[tp.level].p = p[tp.level - 1].c;
				cn_aux = get_node_r(t, p[tp.level - 1].c);
				p[tp.level].c = cn_aux->r;
				p[tp.level].s = STS_ScanStart;
			} else {
				if (f_post) {
					tp.c = p[tp.level].c;
					f(&tp);
				}
				p[tp.level].s = STS_ScanDone;
				tp.level--;
				continue;
			}
			continue;
		case STS_ScanRight:
		case STS_ScanDone:
			if (p[tp.level].s == STS_ScanRight && f_post) {
				tp.c = p[tp.level].c;
				f(&tp);
			}
			p[tp.level].s = STS_ScanDone;
			tp.level--;
			continue;
		}
	}
	return tp.max_level + 1;
}

ssize_t st_traverse_preorder(const srt_tree *t, st_traverse f, void *context)
{
	return srt_treer_aux(t, f, context, TR_Preorder);
}

ssize_t st_traverse_inorder(const srt_tree *t, st_traverse f, void *context)
{
	return srt_treer_aux(t, f, context, TR_Inorder);
}

ssize_t st_traverse_postorder(const srt_tree *t, st_traverse f, void *context)
{
	return srt_treer_aux(t, f, context, TR_Postorder);
}

ssize_t st_traverse_levelorder(const srt_tree *t, st_traverse f, void *context)
{
	size_t ts, i, le, ne;
	struct STraverseParams tp = {context, t, ST_NIL, 0, 0};
	srt_vector *curr, *next;
	srt_tndx n;
	const srt_tnode *node;
	srt_vector *tmp;
	RETURN_IF(!t, -1); /* BEHAVIOR: invalid parameter */
	ts = st_size(t);
	RETURN_IF(!ts, 0); /* empty */
	curr = sv_alloc_t(SV_U32, ts / 2);
	next = sv_alloc_t(SV_U32, ts / 2);
	sv_push_u32(&curr, t->root);
	for (;; tp.max_level = ++tp.level) {
		if (f) {
			tp.c = ST_NIL; /* report starting new tree level */
			f(&tp);
		}
		le = sv_size(curr);
		for (i = 0; i < le; i++) {
			n = (srt_tndx)sv_at_u32(curr, i);
			node = get_node_r(t, n);
			if (f) {
				tp.c = n;
				f(&tp);
			}
			if (node->x.l != ST_NIL)
				sv_push_u32(&next, node->x.l);
			if (node->r != ST_NIL)
				sv_push_u32(&next, node->r);
		}
		ne = sv_size(next);
		if (ne == 0) /* next level is empty */
			break;
		tmp = curr;
		curr = next;
		next = tmp;
		sv_set_size(next, 0);
	}
#ifdef S_USE_VA_ARGS
	sv_free(&curr, &next);
#else
	sv_free(&curr);
	sv_free(&next);
#endif
	return tp.max_level + 1;
}

srt_bool st_assert(const srt_tree *t)
{
	RETURN_IF(!t, S_FALSE);
	RETURN_IF(t->d.size == 1 && is_red(t, t->root), S_FALSE);
	RETURN_IF(t->d.size == 1, S_TRUE);
	return st_assert_aux(t, t->root) ? S_TRUE : S_FALSE;
}

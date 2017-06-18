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
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */

#include "stree.h"
#include "scommon.h"
#include "../svector.h"

/*
 * Helper macros
 */

#define STN_SET_RBB(t, cn) {			\
		cn->x.is_red = S_TRUE;		\
		set_red(t, cn->x.l, S_FALSE);	\
		set_red(t, cn->r, S_FALSE);	\
	}

/*
 * Constants
 */

#define CW_SIZE 4	/* Node context window size (must be 4) */

enum STNDir
{
	ST_Left = 0,
	ST_Right = 1
};

/*
 * Internal data structures
 */

struct NodeContext
{
	stndx_t x;	/* Index to the memory slot */
	stn_t *n;	/* Pointer to the node (base_ptr + x * sizeof(node)) */
};

/*
 * Internal functions
 */

static void set_lr(stn_t *n, const enum STNDir d, const stndx_t v)
{
	S_ASSERT(n);
	if (n) {
		if (d == ST_Left)
			n->x.l = v;
		else
			n->r = v;
	}
}

static stndx_t get_lr(const stn_t *n, const enum STNDir d)
{
	return d == ST_Left ? n->x.l : n->r;
}

static stn_t *locate_parent(st_t *t, const struct NodeContext *son,
			    enum STNDir *d)
{
	if (t->root == son->x)
		return son->n;
	stn_t *cn = get_node(t, t->root);
	for (; cn && cn->x.l != son->x && cn->r != son->x;)
		cn = get_node(t, get_lr(cn,
					t->cmp_f(cn, son->n) < 0 ?
						ST_Right : ST_Left));
	*d = cn && cn->x.l == son->x ? ST_Left : ST_Right;
	return cn;
}

static void update_node_data(const st_t *t, stn_t *tgt, const stn_t *src)
{
	const size_t node_header_size = sizeof(stn_t);
	const size_t copy_size = t->d.elem_size - node_header_size;
	char *tgtp = (char *)tgt + node_header_size;
	const char *srcp = (const char *)src + node_header_size;
	memcpy(tgtp, srcp, copy_size);
}

static void copy_node(const st_t *t, stn_t *tgt, const stn_t *src)
{
	memcpy(tgt, src, t->d.elem_size);
}

static void new_node(const st_t *t, stn_t *tgt, const stn_t *src, sbool_t ir)
{
	update_node_data(t, tgt, src);
	tgt->x.l = tgt->r = ST_NIL;
	tgt->x.is_red = ir;
}

static sbool_t is_red(const st_t *t, stndx_t node_id)
{
	RETURN_IF(node_id == ST_NIL, S_FALSE);
	return get_node_r(t, node_id)->x.is_red;
}

static void set_red(st_t *t, const stndx_t node_id, const sbool_t red)
{
	if (node_id != ST_NIL)
		get_node(t, node_id)->x.is_red = red;
}

/* counter-direction */
static enum STNDir cd(const enum STNDir d)
{
	return d == ST_Left ? ST_Right : ST_Left;
}

/*
 * Node rotation auxiliary functions
 */

#define F_rotate1X			\
	stndx_t y = get_lr(xn, xd);	\
	stn_t *yn = get_node(t, y);	\
	set_lr(xn, xd, get_lr(yn, d));	\
	set_lr(yn, d, x);		\
	set_red(t, x, S_TRUE);		\
	set_red(t, y, S_FALSE);

static stndx_t rot1x(st_t *t, stn_t *xn, const stndx_t x, const enum STNDir d,
		     const enum STNDir xd)
{
	F_rotate1X;
	return y;
}

static void rot1x_p(st_t *t, stn_t *xn, const stndx_t x, const enum STNDir d,
		    const enum STNDir xd, stn_t *xpn)
{
	F_rotate1X;
	set_lr(xpn, d, y);
}

static stn_t *rot1x_y(st_t *t, stn_t *xn, const stndx_t x, const enum STNDir d,
		      const enum STNDir xd, stndx_t *y_out)
{
	F_rotate1X;
	*y_out = y;
	return yn;
}

static stndx_t rot2x(st_t *t, stn_t *xn, const stndx_t x, const enum STNDir d,
		     const enum STNDir xd)
{
	const stndx_t child = get_lr(xn, xd);
	stn_t *child_node = get_node(t, child);
	rot1x_p(t, child_node, child, xd, d, xn);
	return rot1x(t, xn, x, d, xd);
}

/*
 * If current node is the tree root node, change the root index so it targets
 * the new node.
 */
static void st_checkfix_root(st_t *t, stndx_t pivot, stndx_t new_pivot)
{
	if (pivot == t->root) {
		t->root = new_pivot;
		/*set_red(t, t->root, S_FALSE);*/
	}
}

/*
 * Observation: *recursive* function. Be careful when using it in
 * runtime code if you're in a context with small stack size.
 * Returns: 0: error, > 0: tree height from a given node
 */
static size_t st_assert_aux(const st_t *t, const stndx_t ndx)
{
	ASSERT_RETURN_IF(!t, 0);
	RETURN_IF(ndx == ST_NIL, 1);
	const stn_t *n = get_node_r(t, ndx);
	if (is_red(t, ndx) && (is_red(t, n->x.l) || is_red(t, n->r))) {
#ifdef DEBUG_stree
		fprintf(stderr, "st_assert: violation: two red nodes\n");
#endif
		return 0;
	}
	if (n->x.l != ST_NIL && t->cmp_f(get_node_r(t, n->x.l), n) >= 0 &&
	    n->r != ST_NIL && t->cmp_f(get_node_r(t, n->r), n) <= 0) {
#ifdef DEBUG_stree
		fprintf(stderr, "st_assert: tree structure violation\n");
#endif
		return 0;
	}
	size_t l = st_assert_aux(t, n->x.l), r = st_assert_aux(t, n->r);
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

st_t *st_alloc_raw(st_cmp_t cmp_f, const sbool_t ext_buf, void *buffer,
		   const size_t elem_size, const size_t max_size)
{
	RETURN_IF(!cmp_f || !elem_size || !buffer, (st_t *)sd_void);
	st_t *t = (st_t *)buffer;
	sd_reset((sd_t *)t, sizeof(st_t), elem_size, max_size, ext_buf, S_FALSE);
	t->cmp_f = cmp_f;
	t->root = 0;
	return t;
}

st_t *st_alloc(st_cmp_t cmp_f, const size_t elem_size, const size_t init_size)
{
	size_t alloc_size = sd_alloc_size_raw(sizeof(st_t), elem_size,
					      init_size, S_FALSE);
	return st_alloc_raw(cmp_f, S_FALSE, s_malloc(alloc_size), elem_size,
			    init_size);
}

/*
 * Operations
 */

st_t *st_dup(const st_t *t)
{
	ASSERT_RETURN_IF(!t, NULL);
	st_t *t2 = st_alloc(t->cmp_f, t->d.elem_size, t->d.size);
	ASSERT_RETURN_IF(!t2, NULL); /* FIXME: non-safe; TODO: add void const */
	memcpy(t2, t, t->d.header_size + t->d.size * t->d.elem_size);
	return t2;
}

sbool_t st_insert(st_t **tt, const stn_t *n)
{
	return st_insert_rw(tt, n, NULL);
}

sbool_t st_insert_rw(st_t **tt, const stn_t *n, const st_rewrite_t rw_f)
{
	ASSERT_RETURN_IF(!tt || !*tt || !n || !st_grow(tt, 1), S_FALSE);
	st_t *t = *tt;
	const size_t ts = st_size(t);
	ASSERT_RETURN_IF(ts >= ST_NIL, S_FALSE);
	/*
	 * Trivial case: insert node into empty tree
	 */
	if (!ts) {
		stn_t *node = get_node(t, 0);
		new_node(t, node, n, S_FALSE);
		if (rw_f)
			rw_f(node, n, S_FALSE);
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
	stn_t auxn = EMPTY_STN;
	auxn.r = t->root;
	struct NodeContext w[CW_SIZE] = {
		{ t->root, get_node(t, t->root) }, /* c: current node (cn) */
		{ ST_NIL, &auxn },	/* cppp: cn parent parent parent node */
		{ ST_NIL, NULL },	/* cpp: cn parent parent node */
		{ ST_NIL, NULL } };	/* cp: cn parent node */
	size_t c = 0;
	enum STNDir ld = ST_Left;	/* last walk direction */
	enum STNDir d = ST_Left;	/* current walk direction */
	/*
	 * Search loop
	 */
	sbool_t done = S_FALSE;
	for (;;) {
		const size_t cp = (c + 3) % CW_SIZE;
		/* Leave found? update tree, copy data, update size */
		if (w[c].x == ST_NIL) {
			/* New node: */
			w[c].x = (stndx_t)ts;
			w[c].n = get_node(t, (stndx_t)ts);
			new_node(t, w[c].n, n, S_TRUE);
			if (rw_f)
				rw_f(w[c].n, n, S_FALSE);
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
		const size_t cpp = (c + 2) % CW_SIZE,
			     cppp = (c + 1) % CW_SIZE;
		if (w[cpp].n && w[c].n->x.is_red && w[cp].n->x.is_red) {
			const enum STNDir xld = cd(ld);
			const stndx_t pd = get_lr(w[cp].n, ld);
			const stndx_t v = w[c].x == pd ?
				rot1x(t, w[cpp].n, w[cpp].x, xld, ld) :
				rot2x(t, w[cpp].n, w[cpp].x, xld, ld);
			if (w[cppp].n) {
				enum STNDir d2 = w[cppp].n->r == w[cpp].x ?
						 ST_Right : ST_Left;
				set_lr(w[cppp].n, d2, v);
				st_checkfix_root(t, w[cpp].x, v);
			} else {
				t->root = v;
			}
		}
		if (done)
			break;
		const int64_t cmp = t->cmp_f(w[c].n, n);
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

sbool_t st_delete(st_t *t, const stn_t *n, stn_callback_t callback)
{
	ASSERT_RETURN_IF(!t || !n, S_FALSE);
	/* Check empty tree: */
	const size_t ts0 = st_size(t);
	RETURN_IF(ts0 == 0 || ts0 >= ST_NIL, S_FALSE);
	stndx_t ts = (stndx_t)ts0;
	/*
	 * Prepare a 4-level node tracking window (in this case a 3-level
	 * would be enough, using 4 in order to avoid the division by 3,
	 * which is more expensive than by 4 in many CPUs).
	 */
	stn_t auxn = EMPTY_STN;
	auxn.r = t->root;
	struct NodeContext w[CW_SIZE] = {
		{ t->root, get_node(t, t->root) }, /* c: current node (cn) */
		{ ST_NIL, NULL },    /* cppp: cn parent parent parent node */
		{ ST_NIL, NULL },    /* cpp: cn parent parent node */
		{ ST_NIL, &auxn } }; /* cp: cn parent node */
	stndx_t c = 0, cp = 3, cpp = 2, cppp = 1;
	struct NodeContext found = { ST_NIL, NULL };
	enum STNDir d0 = ST_Right;
	/* Search loop */
	for (;;) {
		/* Compare node key with given target */
		const int64_t cmp = t->cmp_f(w[c].n, n);
		const enum STNDir d = cmp < 0 ? ST_Right : ST_Left;
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
			stndx_t nd = get_lr(w[c].n, d);
			stn_t *ndn = get_node(t, nd);
			if (w[c].n->x.is_red || (ndn && ndn->x.is_red))
				break;
			const enum STNDir xd = cd(d);
			if (is_red(t, get_lr(w[c].n, xd))) {
				stndx_t y;
				stn_t *yn = rot1x_y(t, w[c].n, w[c].x, d,
						    xd, &y);
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
			const enum STNDir xd0 = cd(d0);
			const stndx_t s = get_lr(w[cp].n, xd0);
			if (s == ST_NIL)
				break;
			stn_t *sn = get_node(t, s);
			if (!is_red(t, get_lr(sn, xd0)) &&
			    !is_red(t, get_lr(sn, d0))) {
				/* Color flip */
				set_red(t, w[cp].x, S_FALSE);
				set_red(t, s, S_TRUE);
				set_red(t, w[c].x, S_TRUE);
				break;
			}
			if (!w[cpp].n)
				break;
			const enum STNDir d2 = w[cpp].n->r == w[cp].x ?
						ST_Right : ST_Left;
			if (is_red(t, get_lr(sn, d0))) {
				const stndx_t y = rot2x(t, w[cp].n, w[cp].x,
							d0, xd0);
				set_lr(w[cpp].n, d2, y);
				st_checkfix_root(t, w[cp].x, y);
			} else {
				if (is_red(t, get_lr(sn, xd0))) {
					stndx_t y = rot1x(t, w[cp].n, w[cp].x,
							  d0, xd0);
					set_lr(w[cpp].n, d2, y);
					st_checkfix_root(t, w[cp].x, y);
				}
			}
			stn_t *cpp_d2n = get_node(t, get_lr(w[cpp].n, d2));
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
			t->root = w[c].n->x.l != ST_NIL ? w[c].n->x.l :
							  w[c].n->r;
		} else {
			enum STNDir ds = w[c].n->x.l == ST_NIL ? ST_Right :
								 ST_Left;
			enum STNDir dt = w[cp].n->r == w[c].x ? ST_Right :
								ST_Left;
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
		S_ASSERT(ts - 1 < ST_NIL );
		const stndx_t sz = ts - 1; /* BEHAVIOR */
		if (w[c].x != sz) {
			enum STNDir d = ST_Left;
			const struct NodeContext ct = { sz, get_node(t, sz) };
			/* TODO: cache this (!) */
			stn_t *fpn = locate_parent(t, &ct, &d);
			if (fpn) {
				copy_node(t, w[c].n, ct.n);
				set_lr(fpn, d, w[c].x);
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

const stn_t *st_locate(const st_t *t, const stn_t *n)
{
	const stn_t *cn = get_node_r(t, t->root);
	int r;
	for (;;)
		if (!(r = t->cmp_f(cn, n)) ||
		    !(cn = get_node_r(t, get_lr(cn, r < 0 ? ST_Right :
							    ST_Left))))
			break;
	return cn;
}

/*
 * Depth-first tree traversal
 */

enum eTMode
{
	TR_Preorder	= 1,
	TR_Inorder	= 4,
	TR_Postorder	= 8
};

/*
 * Iterative tree traverse using auxilliary stack.
 *
 * Time complexity: O(n)
 * Aux space: using the stack -i.e. "free"-, O(2 * log(n))
 */

static ssize_t st_tr_aux(const st_t *t, st_traverse f, void *context,
			 const enum eTMode m)
{
	ASSERT_RETURN_IF(!t, -1);
	const size_t ts = st_size(t);
	RETURN_IF(!ts, S_FALSE);
	struct STraverseParams tp = { context, t, ST_NIL, 0, 0 };
	size_t rbt_max_depth = 2 * (slog2(ts) + 1); /* +1: round error */
	/*
	 * DF path length takes twice the logarithm of the number of nodes,
	 * so it will fit always in the stack (e.g. (2^32)-1 nodes would require
	 * allocating less than 1KB of stack space for the path).
	 */
#ifdef _MSC_VER /* supress alloca() warning */
#pragma warning(disable: 6255)
#endif
	struct STreeScan *p = (struct STreeScan *)
					alloca(sizeof(struct STreeScan) *
					       (rbt_max_depth + 3));
	ASSERT_RETURN_IF(!p, -1);
	if (f)
		f(&tp);
	p[0].p = ST_NIL;
	p[0].c = t->root;
	p[0].s = STS_ScanStart;
	const stn_t *cn_aux;
	int f_pre = f && m == TR_Preorder;
	int f_ino = f && m == TR_Inorder;
	int f_post = f && m == TR_Postorder;
	for (; tp.level >= 0;) {
		if (tp.level > tp.max_level)
			tp.max_level = tp.level;
		/*
		 * If having more levels than the expected 'rbt_max_depth', it
		 * would mean that there is some bug in the insert/delete
		 * rebalanzing code.
		 */
		S_ASSERT(tp.max_level < (ssize_t)rbt_max_depth);
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
					cn_aux = get_node_r(t, p[tp.level - 1].c);
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
			if (f_post) {
				tp.c = p[tp.level].c;
				f(&tp);
			}
			/* don't break */
		case STS_ScanDone:
			p[tp.level].s = STS_ScanDone;
			tp.level--;
			continue;
		}
	}
	return tp.max_level + 1;
}

ssize_t st_traverse_preorder(const st_t *t, st_traverse f, void *context)
{
	return st_tr_aux(t, f, context, TR_Preorder);
}

ssize_t st_traverse_inorder(const st_t *t, st_traverse f, void *context)
{
	return st_tr_aux(t, f, context, TR_Inorder);
}

ssize_t st_traverse_postorder(const st_t *t, st_traverse f, void *context)
{
	return st_tr_aux(t, f, context, TR_Postorder);
}

ssize_t st_traverse_levelorder(const st_t *t, st_traverse f, void *context)
{
	ASSERT_RETURN_IF(!t, -1); /* BEHAVIOR: invalid parameter */
	const size_t ts = st_size(t);
	RETURN_IF(!ts, 0);	/* empty */
	struct STraverseParams tp = { context, t, ST_NIL, 0, 0 };
	sv_t *curr = sv_alloc_t(SV_U32, ts/2),
	     *next = sv_alloc_t(SV_U32, ts/2);
	sv_push_u(&curr, t->root);
	for (;; tp.max_level = ++tp.level) {
		if (f) {
			tp.c = ST_NIL; /* report starting new tree level */
			f(&tp);
		}
		size_t i = 0;
		size_t le = sv_size(curr);
		for (i = 0; i < le; i++) {
			stndx_t n = (stndx_t)sv_at_u(curr, i);
			const stn_t *node = get_node_r(t, n);
			if (f) {
				tp.c = n;
				f(&tp);
			}
			if (node->x.l != ST_NIL)
				sv_push_u(&next, node->x.l);
			if (node->r != ST_NIL)
				sv_push_u(&next, node->r);
		}
		size_t ne = sv_size(next);
		if (ne == 0)	/* next level is empty */
			break;
		sv_t *tmp = curr;
		curr = next;
		next = tmp;
		sv_set_size(next, 0);
	}
	sv_free(&curr, &next);
	return tp.max_level + 1;
}

sbool_t st_assert(const st_t *t)
{
	ASSERT_RETURN_IF(!t, S_FALSE);
	ASSERT_RETURN_IF(t->d.size == 1 && is_red(t, t->root), S_FALSE);
	RETURN_IF(t->d.size == 1, S_TRUE);
	return st_assert_aux(t, t->root) ? S_TRUE : S_FALSE;
}


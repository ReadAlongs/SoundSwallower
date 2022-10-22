/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2008 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */

/**
 * @file ps_lattice.h Word graph search
 */

#ifndef __PS_LATTICE_H__
#define __PS_LATTICE_H__

#include <soundswallower/prim_type.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef struct ps_search_s ps_search_t;
typedef struct ps_seg_s ps_seg_t;

/**
 * Word graph structure used in bestpath/nbest search.
 */
typedef struct ps_lattice_s ps_lattice_t;

/**
 * DAG nodes.
 *
 * A node corresponds to a number of hypothesized instances of a word
 * which all share the same starting point.
 */
typedef struct ps_latnode_s ps_latnode_t;

/**
 * Iterator over DAG nodes.
 */
typedef struct ps_latnode_s ps_latnode_iter_t; /* pay no attention to the man behind the curtain */

/**
 * Links between DAG nodes.
 *
 * A link corresponds to a single hypothesized instance of a word with
 * a given start and end point.
 */
typedef struct ps_latlink_s ps_latlink_t;

/**
 * Iterator over DAG links.
 */
typedef struct latlink_list_s ps_latlink_iter_t;

/* Forward declaration needed to avoid circular includes */
struct ps_decoder_s;

/**
 * Retain a lattice.
 *
 * This function retains ownership of a lattice for the caller,
 * preventing it from being freed automatically.  You must call
 * ps_lattice_free() to free it after having called this function.
 *
 * @return pointer to the retained lattice.
 */
ps_lattice_t *ps_lattice_retain(ps_lattice_t *dag);

/**
 * Free a lattice.
 *
 * @return new reference count (0 if dag was freed)
 */
int ps_lattice_free(ps_lattice_t *dag);

/**
 * Get the log-math computation object for this lattice
 *
 * @return The log-math object for this lattice.  The lattice retains
 *         ownership of this pointer, so you should not attempt to
 *         free it manually.  Use logmath_retain() if you wish to
 *         reuse it elsewhere.
 */
logmath_t *ps_lattice_get_logmath(ps_lattice_t *dag);


/**
 * Start iterating over nodes in the lattice.
 *
 * @note No particular order of traversal is guaranteed, and you
 * should not depend on this.
 *
 * @param dag Lattice to iterate over.
 * @return Iterator over lattice nodes.
 */
ps_latnode_iter_t *ps_latnode_iter(ps_lattice_t *dag);

/**
 * Move to next node in iteration.
 * @param itor Node iterator.
 * @return Updated node iterator, or NULL if finished
 */
ps_latnode_iter_t *ps_latnode_iter_next(ps_latnode_iter_t *itor);

/**
 * Stop iterating over nodes.
 * @param itor Node iterator.
 */
void ps_latnode_iter_free(ps_latnode_iter_t *itor);

/**
 * Get node from iterator.
 */
ps_latnode_t *ps_latnode_iter_node(ps_latnode_iter_t *itor);

/**
 * Get start and end time range for a node.
 *
 * @param node Node inquired about.
 * @param out_fef Output: End frame of first exit from this node.
 * @param out_lef Output: End frame of last exit from this node.
 * @return Start frame for all edges exiting this node.
 */
int ps_latnode_times(ps_latnode_t *node, int16 *out_fef, int16 *out_lef);

/**
 * Get word string for this node.
 *
 * @param dag Lattice to which node belongs.
 * @param node Node inquired about.
 * @return Word string for this node (possibly a pronunciation variant).
 */
char const *ps_latnode_word(ps_lattice_t *dag, ps_latnode_t *node);

/**
 * Get base word string for this node.
 *
 * @param dag Lattice to which node belongs.
 * @param node Node inquired about.
 * @return Base word string for this node.
 */
char const *ps_latnode_baseword(ps_lattice_t *dag, ps_latnode_t *node);

/**
 * Iterate over exits from this node.
 *
 * @param node Node inquired about.
 * @return Iterator over exit links from this node.
 */
ps_latlink_iter_t *ps_latnode_exits(ps_latnode_t *node);

/**
 * Iterate over entries to this node.
 *
 * @param node Node inquired about.
 * @return Iterator over entry links to this node.
 */
ps_latlink_iter_t *ps_latnode_entries(ps_latnode_t *node);

/**
 * Get best posterior probability and associated acoustic score from a lattice node.
 *
 * @param dag Lattice to which node belongs.
 * @param node Node inquired about.
 * @param out_link Output: exit link with highest posterior probability
 * @return Posterior probability of the best link exiting this node.
 *         Log is expressed in the log-base used in the decoder.  To
 *         convert to linear floating-point, use
 *         logmath_exp(ps_lattice_get_logmath(), pprob).
 */
int32 ps_latnode_prob(ps_lattice_t *dag, ps_latnode_t *node,
                      ps_latlink_t **out_link);

/**
 * Get next link from a lattice link iterator.
 *
 * @param itor Iterator.
 * @return Updated iterator, or NULL if finished.
 */
ps_latlink_iter_t *ps_latlink_iter_next(ps_latlink_iter_t *itor);

/**
 * Stop iterating over links.
 * @param itor Link iterator.
 */
void ps_latlink_iter_free(ps_latlink_iter_t *itor);

/**
 * Get link from iterator.
 */
ps_latlink_t *ps_latlink_iter_link(ps_latlink_iter_t *itor);

/**
 * Get start and end times from a lattice link.
 *
 * @note these are <strong>inclusive</strong> - i.e. the last frame of
 * this word is ef, not ef-1.
 *
 * @param link Link inquired about.
 * @param out_sf Output: (optional) start frame of this link.
 * @return End frame of this link.
 */
int ps_latlink_times(ps_latlink_t *link, int16 *out_sf);

/**
 * Get destination and source nodes from a lattice link
 *
 * @param link Link inquired about
 * @param out_src Output: (optional) source node.
 * @return destination node
 */
ps_latnode_t *ps_latlink_nodes(ps_latlink_t *link, ps_latnode_t **out_src);

/**
 * Get word string from a lattice link.
 *
 * @param dag Lattice to which node belongs.
 * @param link Link inquired about
 * @return Word string for this link (possibly a pronunciation variant).
 */
char const *ps_latlink_word(ps_lattice_t *dag, ps_latlink_t *link);

/**
 * Get base word string from a lattice link.
 *
 * @param dag Lattice to which node belongs.
 * @param link Link inquired about
 * @return Base word string for this link
 */
char const *ps_latlink_baseword(ps_lattice_t *dag, ps_latlink_t *link);

/**
 * Get predecessor link in best path.
 *
 * @param link Link inquired about
 * @return Best previous link from bestpath search, if any.  Otherwise NULL
 */
ps_latlink_t *ps_latlink_pred(ps_latlink_t *link);

/**
 * Get acoustic score and posterior probability from a lattice link.
 *
 * @param dag Lattice to which node belongs.
 * @param link Link inquired about
 * @param out_ascr Output: (optional) acoustic score.
 * @return Posterior probability for this link.  Log is expressed in
 *         the log-base used in the decoder.  To convert to linear
 *         floating-point, use logmath_exp(ps_lattice_get_logmath(), pprob).
 */
int32 ps_latlink_prob(ps_lattice_t *dag, ps_latlink_t *link, int32 *out_ascr);

/**
 * Create a directed link between "from" and "to" nodes, but if a link already exists,
 * choose one with the best link_scr.
 */
void ps_lattice_link(ps_lattice_t *dag, ps_latnode_t *from, ps_latnode_t *to,
                     int32 score, int32 ef);

/**
 * Start a forward traversal of edges in a word graph.
 *
 * @note A keen eye will notice an inconsistency in this API versus
 * other types of iterators in PocketSphinx.  The reason for this is
 * that the traversal algorithm is much more efficient when it is able
 * to modify the lattice structure.  Therefore, to avoid giving the
 * impression that multiple traversals are possible at once, no
 * separate iterator structure is provided.
 *
 * @param dag Lattice to be traversed.
 * @param start Start node (source) of traversal.
 * @param end End node (goal) of traversal.
 * @return First link in traversal.
 */
ps_latlink_t *ps_lattice_traverse_edges(ps_lattice_t *dag, ps_latnode_t *start, ps_latnode_t *end);

/**
 * Get the next link in forward traversal.
 *
 * @param dag Lattice to be traversed.
 * @param end End node (goal) of traversal.
 * @return Next link in traversal.
 */
ps_latlink_t *ps_lattice_traverse_next(ps_lattice_t *dag, ps_latnode_t *end);

/**
 * Start a reverse traversal of edges in a word graph.
 *
 * @note See ps_lattice_traverse_edges() for why this API is the way it is.
 *
 * @param dag Lattice to be traversed.
 * @param start Start node (goal) of traversal.
 * @param end End node (source) of traversal.
 * @return First link in traversal.
 */
ps_latlink_t *ps_lattice_reverse_edges(ps_lattice_t *dag, ps_latnode_t *start, ps_latnode_t *end);

/**
 * Get the next link in reverse traversal.
 *
 * @param dag Lattice to be traversed.
 * @param start Start node (goal) of traversal.
 * @return Next link in traversal.
 */
ps_latlink_t *ps_lattice_reverse_next(ps_lattice_t *dag, ps_latnode_t *start);

/**
 * Do best-path search on a word graph.
 *
 * This function calculates both the best path as well as the forward
 * probability used in confidence estimation.
 *
 * @return Final link in best path, NULL on error.
 */
ps_latlink_t *ps_lattice_bestpath(ps_lattice_t *dag, void *lmset,
                                  float32 ascale);

/**
 * Calculate link posterior probabilities on a word graph.
 *
 * This function assumes that bestpath search has already been done.
 *
 * @return Posterior probability of the utterance as a whole.
 */
int32 ps_lattice_posterior(ps_lattice_t *dag, void *lmset,
                           float32 ascale);

/**
 * Prune all links (and associated nodes) below a certain posterior probability.
 *
 * This function assumes that ps_lattice_posterior() has already been called.
 *
 * @param beam Minimum posterior probability for links. This is
 *         expressed in the log-base used in the decoder.  To convert
 *         from linear floating-point, use
 *         logmath_log(ps_lattice_get_logmath(), prob).
 * @return number of arcs removed.
 */
int32 ps_lattice_posterior_prune(ps_lattice_t *dag, int32 beam);

/**
 * Get the number of frames in the lattice.
 *
 * @param dag The lattice in question.
 * @return Number of frames in this lattice.
 */
int ps_lattice_n_frames(ps_lattice_t *dag);


/**
 * Linked list of DAG link pointers.
 *
 * Because the same link structure is used for forward and reverse
 * links, as well as for the agenda used in bestpath search, we can't
 * store the list pointer inside latlink_t.  We could use glist_t
 * here, but it wastes 4 bytes per entry on 32-bit machines.
 */
typedef struct latlink_list_s {
    ps_latlink_t *link;
    struct latlink_list_s *next;
} latlink_list_t;

/**
 * Word graph structure used in bestpath/nbest search.
 */
struct ps_lattice_s {
    int refcount;      /**< Reference count. */

    logmath_t *lmath;    /**< Log-math object. */
    ps_search_t *search; /**< Search (if generated by search). */
    dict_t *dict;	 /**< Dictionary for this DAG. */
    int32 silence;       /**< Silence word ID. */
    int32 frate;         /**< Frame rate. */

    ps_latnode_t *nodes;  /**< List of all nodes. */
    ps_latnode_t *start;  /**< Starting node. */
    ps_latnode_t *end;    /**< Ending node. */

    frame_idx_t n_frames;    /**< Number of frames for this utterance. */
    int32 n_nodes;     /**< Number of nodes in this lattice. */
    int32 final_node_ascr; /**< Acoustic score of implicit link exiting final node. */
    int32 norm;        /**< Normalizer for posterior probabilities. */
    char *hyp_str;     /**< Current hypothesis string. */

    listelem_alloc_t *latnode_alloc;     /**< Node allocator for this DAG. */
    listelem_alloc_t *latlink_alloc;     /**< Link allocator for this DAG. */
    listelem_alloc_t *latlink_list_alloc; /**< List element allocator for this DAG. */

    /* This will probably be replaced with a heap. */
    latlink_list_t *q_head; /**< Queue of links for traversal. */
    latlink_list_t *q_tail; /**< Queue of links for traversal. */
};

/**
 * Links between DAG nodes.
 *
 * A link corresponds to a single hypothesized instance of a word with
 * a given start and end point.

 */
struct ps_latlink_s {
    struct ps_latnode_s *from;	/**< From node */
    struct ps_latnode_s *to;	/**< To node */
    struct ps_latlink_s *best_prev;
    int32 ascr;			/**< Score for from->wid (from->sf to this->ef) */
    int32 path_scr;		/**< Best path score from root of DAG */
    frame_idx_t ef;			/**< Ending frame of this word  */
    int32 alpha;                /**< Forward probability of this link P(w,o_1^{ef}) */
    int32 beta;                 /**< Backward probability of this link P(w|o_{ef+1}^T) */
};

/**
 * DAG nodes.
 *
 * A node corresponds to a number of hypothesized instances of a word
 * which all share the same starting point.
 */
struct ps_latnode_s {
    int32 id;			/**< Unique id for this node */
    int32 wid;			/**< Dictionary word id */
    int32 basewid;		/**< Dictionary base word id */
    /* FIXME: These are (ab)used to store backpointer indices, therefore they MUST be 32 bits. */
    int32 fef;			/**< First end frame */
    int32 lef;			/**< Last end frame */
    frame_idx_t sf;			/**< Start frame */
    int16 reachable;		/**< From \verbatim </s> \endverbatim or \verbatim <s> \endverbatim */
    int32 node_id;		/**< Node from fsg model, used to map lattice back to model */
    union {
        glist_t velist;         /**< List of history entries with different lmstate (tst only) */
	int32 fanin;		/**< Number nodes with links to this node */
	int32 rem_score;	/**< Estimated best score from node.sf to end */
	int32 best_exit;	/**< Best exit score (used for final nodes only) */
    } info;
    latlink_list_t *exits;      /**< Links out of this node */
    latlink_list_t *entries;    /**< Links into this node */

    struct ps_latnode_s *alt;   /**< Node with alternate pronunciation for this word */
    struct ps_latnode_s *next;	/**< Next node in DAG (no ordering implied) */
};

/**
 * Partial path structure used in N-best (A*) search.
 *
 * Each partial path (latpath_t) is constructed by extending another
 * partial path--parent--by one node.
 */
typedef struct ps_latpath_s {
    ps_latnode_t *node;            /**< Node ending this path. */
    struct ps_latpath_s *parent;   /**< Previous element in this path. */
    struct ps_latpath_s *next;     /**< Pointer to next path in list of paths. */
    int32 score;                  /**< Exact score from start node up to node->sf. */
} ps_latpath_t;

/**
 * A* search structure.
 */
typedef struct ps_astar_s {
    ps_lattice_t *dag;
    void *lmset;

    frame_idx_t sf;
    frame_idx_t ef;
    int32 w1;
    int32 w2;

    int32 n_hyp_tried;
    int32 n_hyp_insert;
    int32 n_hyp_reject;
    int32 insert_depth;
    int32 n_path;

    ps_latpath_t *path_list;
    ps_latpath_t *path_tail;
    ps_latpath_t *top;

    glist_t hyps;	             /**< List of hypothesis strings. */
    listelem_alloc_t *latpath_alloc; /**< Path allocator for N-best search. */
} ps_astar_t;

/**
 * Construct an empty word graph with reference to a search structure.
 */
ps_lattice_t *ps_lattice_init_search(ps_search_t *search, int n_frame);

/**
 * Insert penalty for fillers
 */
void ps_lattice_penalize_fillers(ps_lattice_t *dag, int32 silpen, int32 fillpen);

/**
 * Remove nodes marked as unreachable.
 */
void ps_lattice_delete_unreachable(ps_lattice_t *dag);

/**
 * Add an edge to the traversal queue.
 */
void ps_lattice_pushq(ps_lattice_t *dag, ps_latlink_t *link);

/**
 * Remove an edge from the traversal queue.
 */
ps_latlink_t *ps_lattice_popq(ps_lattice_t *dag);

/**
 * Clear and reset the traversal queue.
 */
void ps_lattice_delq(ps_lattice_t *dag);

/**
 * Create a new lattice link element.
 */
latlink_list_t *latlink_list_new(ps_lattice_t *dag, ps_latlink_t *link,
                                 latlink_list_t *next);

/**
 * Get hypothesis string after bestpath search.
 */
char const *ps_lattice_hyp(ps_lattice_t *dag, ps_latlink_t *link);

/**
 * Get hypothesis segmentation iterator after bestpath search.
 */
ps_seg_t *ps_lattice_seg_iter(ps_lattice_t *dag, ps_latlink_t *link);

/**
 * Begin N-Gram based A* search on a word graph.
 *
 * @param sf Starting frame for N-best search.
 * @param ef Ending frame for N-best search, or -1 for last frame.
 * @param w1 First context word, or -1 for none.
 * @param w2 Second context word, or -1 for none.
 * @return 0 for success, <0 on error.
 */
ps_astar_t *ps_astar_start(ps_lattice_t *dag,
                           void *lmset,
                           int sf, int ef,
                           int w1, int w2);

/**
 * Find next best hypothesis of A* on a word graph.
 *
 * @return a complete path, or NULL if no more hypotheses exist.
 */
ps_latpath_t *ps_astar_next(ps_astar_t *nbest);

/**
 * Finish N-best search, releasing resources associated with it.
 */
void ps_astar_finish(ps_astar_t *nbest);

/**
 * Get hypothesis string from A* search.
 */
char const *ps_astar_hyp(ps_astar_t *nbest, ps_latpath_t *path);

/**
 * Get hypothesis segmentation from A* search.
 */
ps_seg_t *ps_astar_seg_iter(ps_astar_t *astar, ps_latpath_t *path);

#ifdef __cplusplus
}
#endif

#endif /* __PS_LATTICE_H__ */

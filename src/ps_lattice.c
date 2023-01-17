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
 * @file lattice.c Word graph search.
 */

#include <assert.h>
#include <math.h>
#include <string.h>

#include <soundswallower/ckd_alloc.h>
#include <soundswallower/decoder.h>
#include <soundswallower/dict.h>
#include <soundswallower/err.h>
#include <soundswallower/listelem_alloc.h>
#include <soundswallower/strfuncs.h>

/**
 * Segmentation "iterator" for backpointer table results.
 */
typedef struct dag_seg_s {
    seg_iter_t base; /**< Base structure. */
    latlink_t **links; /**< Array of lattice links. */
    int32 norm; /**< Normalizer for posterior probabilities. */
    int16 n_links; /**< Number of lattice links. */
    int16 cur; /**< Current position in bpidx. */
} dag_seg_t;

/**
 * Segmentation "iterator" for A* search results.
 */
typedef struct astar_seg_s {
    seg_iter_t base;
    latnode_t **nodes;
    int n_nodes;
    int cur;
} astar_seg_t;

/*
 * Create a directed link between "from" and "to" nodes, but if a link already exists,
 * choose one with the best ascr.
 */
void
lattice_link(lattice_t *dag, latnode_t *from, latnode_t *to,
             int32 score, int32 ef)
{
    latlink_list_t *fwdlink;

    /* Look for an existing link between "from" and "to" nodes */
    for (fwdlink = from->exits; fwdlink; fwdlink = fwdlink->next)
        if (fwdlink->link->to == to)
            break;

    if (fwdlink == NULL) {
        latlink_list_t *revlink;
        latlink_t *link;

        /* No link between the two nodes; create a new one */
        link = listelem_malloc(dag->latlink_alloc);
        fwdlink = listelem_malloc(dag->latlink_list_alloc);
        revlink = listelem_malloc(dag->latlink_list_alloc);

        link->from = from;
        link->to = to;
        link->ascr = score;
        link->ef = ef;
        link->best_prev = NULL;

        fwdlink->link = revlink->link = link;
        fwdlink->next = from->exits;
        from->exits = fwdlink;
        revlink->next = to->entries;
        to->entries = revlink;
    } else {
        /* Link already exists; just retain the best ascr */
        if (score BETTER_THAN fwdlink->link->ascr) {
            fwdlink->link->ascr = score;
            fwdlink->link->ef = ef;
        }
    }
}

void
lattice_penalize_fillers(lattice_t *dag, int32 silpen, int32 fillpen)
{
    latnode_t *node;

    for (node = dag->nodes; node; node = node->next) {
        latlink_list_t *linklist;
        if (node != dag->start && node != dag->end && dict_filler_word(dag->dict, node->basewid)) {
            for (linklist = node->entries; linklist; linklist = linklist->next)
                linklist->link->ascr += (node->basewid == dag->silence) ? silpen : fillpen;
        }
    }
}

static void
delete_node(lattice_t *dag, latnode_t *node)
{
    latlink_list_t *x, *next_x;

    for (x = node->exits; x; x = next_x) {
        next_x = x->next;
        x->link->from = NULL;
        listelem_free(dag->latlink_list_alloc, x);
    }
    for (x = node->entries; x; x = next_x) {
        next_x = x->next;
        x->link->to = NULL;
        listelem_free(dag->latlink_list_alloc, x);
    }
    listelem_free(dag->latnode_alloc, node);
}

static void
remove_dangling_links(lattice_t *dag, latnode_t *node)
{
    latlink_list_t *x, *prev_x, *next_x;

    prev_x = NULL;
    for (x = node->exits; x; x = next_x) {
        next_x = x->next;
        if (x->link->to == NULL) {
            if (prev_x)
                prev_x->next = next_x;
            else
                node->exits = next_x;
            listelem_free(dag->latlink_alloc, x->link);
            listelem_free(dag->latlink_list_alloc, x);
        } else
            prev_x = x;
    }
    prev_x = NULL;
    for (x = node->entries; x; x = next_x) {
        next_x = x->next;
        if (x->link->from == NULL) {
            if (prev_x)
                prev_x->next = next_x;
            else
                node->entries = next_x;
            listelem_free(dag->latlink_alloc, x->link);
            listelem_free(dag->latlink_list_alloc, x);
        } else
            prev_x = x;
    }
}

void
lattice_delete_unreachable(lattice_t *dag)
{
    latnode_t *node, *prev_node, *next_node;
    int i;

    /* Remove unreachable nodes from the list of nodes. */
    prev_node = NULL;
    for (node = dag->nodes; node; node = next_node) {
        next_node = node->next;
        if (!node->reachable) {
            if (prev_node)
                prev_node->next = next_node;
            else
                dag->nodes = next_node;
            /* Delete this node and NULLify links to it. */
            delete_node(dag, node);
        } else
            prev_node = node;
    }

    /* Remove all links to and from unreachable nodes. */
    i = 0;
    for (node = dag->nodes; node; node = node->next) {
        /* Assign sequence numbers. */
        node->id = i++;

        /* We should obviously not encounter unreachable nodes here! */
        assert(node->reachable);

        /* Remove all links that go nowhere. */
        remove_dangling_links(dag, node);
    }
}

int
lattice_n_frames(lattice_t *dag)
{
    return dag->n_frames;
}

lattice_t *
lattice_init_search(search_module_t *search, int n_frame)
{
    lattice_t *dag;

    dag = ckd_calloc(1, sizeof(*dag));
    dag->search = search;
    dag->dict = dict_retain(search->dict);
    dag->lmath = logmath_retain(search->acmod->lmath);
    dag->frate = config_int(dag->search->config, "frate");
    dag->silence = dict_silwid(dag->dict);
    dag->n_frames = n_frame;
    dag->latnode_alloc = listelem_alloc_init(sizeof(latnode_t));
    dag->latlink_alloc = listelem_alloc_init(sizeof(latlink_t));
    dag->latlink_list_alloc = listelem_alloc_init(sizeof(latlink_list_t));
    dag->refcount = 1;
    return dag;
}

lattice_t *
lattice_retain(lattice_t *dag)
{
    if (dag == NULL)
        return NULL;
    ++dag->refcount;
    return dag;
}

int
lattice_free(lattice_t *dag)
{
    if (dag == NULL)
        return 0;
    if (--dag->refcount > 0)
        return dag->refcount;
    logmath_free(dag->lmath);
    dict_free(dag->dict);
    listelem_alloc_free(dag->latnode_alloc);
    listelem_alloc_free(dag->latlink_alloc);
    listelem_alloc_free(dag->latlink_list_alloc);
    ckd_free(dag->hyp_str);
    ckd_free(dag);
    return 0;
}

logmath_t *
lattice_get_logmath(lattice_t *dag)
{
    return dag->lmath;
}

latnode_iter_t *
ps_latnode_iter(lattice_t *dag)
{
    return dag->nodes;
}

latnode_iter_t *
ps_latnode_iter_next(latnode_iter_t *itor)
{
    return itor->next;
}

void
ps_latnode_iter_free(latnode_iter_t *itor)
{
    /* Do absolutely nothing. */
    (void)itor;
}

latnode_t *
ps_latnode_iter_node(latnode_iter_t *itor)
{
    return itor;
}

int
latnode_times(latnode_t *node, int16 *out_fef, int16 *out_lef)
{
    if (out_fef)
        *out_fef = (int16)node->fef;
    if (out_lef)
        *out_lef = (int16)node->lef;
    return node->sf;
}

const char *
ps_latnode_word(lattice_t *dag, latnode_t *node)
{
    return dict_wordstr(dag->dict, node->wid);
}

const char *
ps_latnode_baseword(lattice_t *dag, latnode_t *node)
{
    return dict_wordstr(dag->dict, node->basewid);
}

int32
ps_latnode_prob(lattice_t *dag, latnode_t *node,
                latlink_t **out_link)
{
    latlink_list_t *links;
    int32 bestpost = logmath_get_zero(dag->lmath);

    for (links = node->exits; links; links = links->next) {
        int32 post = links->link->alpha + links->link->beta - dag->norm;
        if (post > bestpost) {
            if (out_link)
                *out_link = links->link;
            bestpost = post;
        }
    }
    return bestpost;
}

latlink_iter_t *
ps_latnode_exits(latnode_t *node)
{
    return node->exits;
}

latlink_iter_t *
ps_latnode_entries(latnode_t *node)
{
    return node->entries;
}

latlink_iter_t *
ps_latlink_iter_next(latlink_iter_t *itor)
{
    return itor->next;
}

void
ps_latlink_iter_free(latlink_iter_t *itor)
{
    /* Do absolutely nothing. */
    (void)itor;
}

latlink_t *
ps_latlink_iter_link(latlink_iter_t *itor)
{
    return itor->link;
}

int
latlink_times(latlink_t *link, int16 *out_sf)
{
    if (out_sf) {
        if (link->from) {
            *out_sf = link->from->sf;
        } else {
            *out_sf = 0;
        }
    }
    return link->ef;
}

latnode_t *
ps_latlink_nodes(latlink_t *link, latnode_t **out_src)
{
    if (out_src)
        *out_src = link->from;
    return link->to;
}

const char *
ps_latlink_word(lattice_t *dag, latlink_t *link)
{
    if (link->from == NULL)
        return NULL;
    return dict_wordstr(dag->dict, link->from->wid);
}

const char *
ps_latlink_baseword(lattice_t *dag, latlink_t *link)
{
    if (link->from == NULL)
        return NULL;
    return dict_wordstr(dag->dict, link->from->basewid);
}

latlink_t *
ps_latlink_pred(latlink_t *link)
{
    return link->best_prev;
}

int32
ps_latlink_prob(lattice_t *dag, latlink_t *link, int32 *out_ascr)
{
    int32 post = link->alpha + link->beta - dag->norm;
    if (out_ascr)
        *out_ascr = link->ascr << SENSCR_SHIFT;
    return post;
}

const char *
lattice_hyp(lattice_t *dag, latlink_t *link)
{
    latlink_t *l;
    size_t len;
    char *c;

    /* Backtrace once to get hypothesis length. */
    len = 0;
    /* FIXME: There may not be a search, but actually there should be a dict. */
    if (dict_real_word(dag->dict, link->to->basewid)) {
        char *wstr = dict_wordstr(dag->dict, link->to->basewid);
        if (wstr != NULL)
            len += strlen(wstr) + 1;
    }
    for (l = link; l; l = l->best_prev) {
        if (dict_real_word(dag->dict, l->from->basewid)) {
            char *wstr = dict_wordstr(dag->dict, l->from->basewid);
            if (wstr != NULL)
                len += strlen(wstr) + 1;
        }
    }

    /* Backtrace again to construct hypothesis string. */
    ckd_free(dag->hyp_str);
    dag->hyp_str = ckd_calloc(1, len + 1); /* extra one incase the hyp is empty */
    c = dag->hyp_str + len - 1;
    if (dict_real_word(dag->dict, link->to->basewid)) {
        char *wstr = dict_wordstr(dag->dict, link->to->basewid);
        if (wstr != NULL) {
            len = strlen(wstr);
            c -= len;
            memcpy(c, wstr, len);
            if (c > dag->hyp_str) {
                --c;
                *c = ' ';
            }
        }
    }
    for (l = link; l; l = l->best_prev) {
        if (dict_real_word(dag->dict, l->from->basewid)) {
            char *wstr = dict_wordstr(dag->dict, l->from->basewid);
            if (wstr != NULL) {
                len = strlen(wstr);
                c -= len;
                memcpy(c, wstr, len);
                if (c > dag->hyp_str) {
                    --c;
                    *c = ' ';
                }
            }
        }
    }

    return dag->hyp_str;
}

static void
lattice_link2itor(seg_iter_t *seg, latlink_t *link, int to)
{
    dag_seg_t *itor = (dag_seg_t *)seg;
    latnode_t *node;

    if (to) {
        node = link->to;
        seg->ef = node->lef;
        seg->prob = 0; /* norm + beta - norm */
    } else {
        latlink_list_t *x;
        latnode_t *n;
        logmath_t *lmath = search_module_acmod(seg->search)->lmath;

        node = link->from;
        seg->ef = link->ef;
        seg->prob = link->alpha + link->beta - itor->norm;
        /* Sum over all exits for this word and any alternate
           pronunciations at the same frame. */
        for (n = node; n; n = n->alt) {
            for (x = n->exits; x; x = x->next) {
                if (x->link == link)
                    continue;
                seg->prob = logmath_add(lmath, seg->prob,
                                        x->link->alpha + x->link->beta - itor->norm);
            }
        }
    }
    seg->word = dict_wordstr(search_module_dict(seg->search), node->wid);
    seg->sf = node->sf;
    seg->ascr = link->ascr << SENSCR_SHIFT;
}

static void
lattice_seg_free(seg_iter_t *seg)
{
    dag_seg_t *itor = (dag_seg_t *)seg;

    ckd_free(itor->links);
    ckd_free(itor);
}

static seg_iter_t *
lattice_seg_next(seg_iter_t *seg)
{
    dag_seg_t *itor = (dag_seg_t *)seg;

    ++itor->cur;
    if (itor->cur == itor->n_links + 1) {
        lattice_seg_free(seg);
        return NULL;
    } else if (itor->cur == itor->n_links) {
        /* Re-use the last link but with the "to" node. */
        lattice_link2itor(seg, itor->links[itor->cur - 1], TRUE);
    } else {
        lattice_link2itor(seg, itor->links[itor->cur], FALSE);
    }

    return seg;
}

static ps_segfuncs_t lattice_segfuncs = {
    /* seg_next */ lattice_seg_next,
    /* seg_free */ lattice_seg_free
};

seg_iter_t *
lattice_seg_iter(lattice_t *dag, latlink_t *link)
{
    dag_seg_t *itor;
    latlink_t *l;
    int cur;

    /* Calling this an "iterator" is a bit of a misnomer since we have
     * to get the entire backtrace in order to produce it.
     */
    itor = ckd_calloc(1, sizeof(*itor));
    itor->base.vt = &lattice_segfuncs;
    itor->base.search = dag->search;
    itor->n_links = 0;
    itor->norm = dag->norm;

    for (l = link; l; l = l->best_prev) {
        ++itor->n_links;
    }
    if (itor->n_links == 0) {
        ckd_free(itor);
        return NULL;
    }

    itor->links = ckd_calloc(itor->n_links, sizeof(*itor->links));
    cur = itor->n_links - 1;
    for (l = link; l; l = l->best_prev) {
        itor->links[cur] = l;
        --cur;
    }

    lattice_link2itor((seg_iter_t *)itor, itor->links[0], FALSE);
    return (seg_iter_t *)itor;
}

latlink_list_t *
latlink_list_new(lattice_t *dag, latlink_t *link, latlink_list_t *next)
{
    latlink_list_t *ll;

    ll = listelem_malloc(dag->latlink_list_alloc);
    ll->link = link;
    ll->next = next;

    return ll;
}

void
lattice_pushq(lattice_t *dag, latlink_t *link)
{
    if (dag->q_head == NULL)
        dag->q_head = dag->q_tail = latlink_list_new(dag, link, NULL);
    else {
        dag->q_tail->next = latlink_list_new(dag, link, NULL);
        dag->q_tail = dag->q_tail->next;
    }
}

latlink_t *
lattice_popq(lattice_t *dag)
{
    latlink_list_t *x;
    latlink_t *link;

    if (dag->q_head == NULL)
        return NULL;
    link = dag->q_head->link;
    x = dag->q_head->next;
    listelem_free(dag->latlink_list_alloc, dag->q_head);
    dag->q_head = x;
    if (dag->q_head == NULL)
        dag->q_tail = NULL;
    return link;
}

void
lattice_delq(lattice_t *dag)
{
    while (lattice_popq(dag)) {
        /* Do nothing. */
    }
}

latlink_t *
lattice_traverse_edges(lattice_t *dag, latnode_t *start, latnode_t *end)
{
    latnode_t *node;
    latlink_list_t *x;

    /* Cancel any unfinished traversal. */
    lattice_delq(dag);

    /* Initialize node fanin counts and path scores. */
    for (node = dag->nodes; node; node = node->next)
        node->info.fanin = 0;
    for (node = dag->nodes; node; node = node->next) {
        for (x = node->exits; x; x = x->next)
            (x->link->to->info.fanin)++;
    }

    /* Initialize agenda with all exits from start. */
    if (start == NULL)
        start = dag->start;
    for (x = start->exits; x; x = x->next)
        lattice_pushq(dag, x->link);

    /* Pull the first edge off the queue. */
    return lattice_traverse_next(dag, end);
}

latlink_t *
lattice_traverse_next(lattice_t *dag, latnode_t *end)
{
    latlink_t *next;

    next = lattice_popq(dag);
    if (next == NULL)
        return NULL;

    /* Decrease fanin count for destination node and expand outgoing
     * edges if all incoming edges have been seen. */
    --next->to->info.fanin;
    if (next->to->info.fanin == 0) {
        latlink_list_t *x;

        if (end == NULL)
            end = dag->end;
        if (next->to == end) {
            /* If we have traversed all links entering the end node,
             * clear the queue, causing future calls to this function
             * to return NULL. */
            lattice_delq(dag);
            return next;
        }

        /* Extend all outgoing edges. */
        for (x = next->to->exits; x; x = x->next)
            lattice_pushq(dag, x->link);
    }
    return next;
}

latlink_t *
lattice_reverse_edges(lattice_t *dag, latnode_t *start, latnode_t *end)
{
    latnode_t *node;
    latlink_list_t *x;

    /* Cancel any unfinished traversal. */
    lattice_delq(dag);

    /* Initialize node fanout counts and path scores. */
    for (node = dag->nodes; node; node = node->next) {
        node->info.fanin = 0;
        for (x = node->exits; x; x = x->next)
            ++node->info.fanin;
    }

    /* Initialize agenda with all entries from end. */
    if (end == NULL)
        end = dag->end;
    for (x = end->entries; x; x = x->next)
        lattice_pushq(dag, x->link);

    /* Pull the first edge off the queue. */
    return lattice_reverse_next(dag, start);
}

latlink_t *
lattice_reverse_next(lattice_t *dag, latnode_t *start)
{
    latlink_t *next;

    next = lattice_popq(dag);
    if (next == NULL)
        return NULL;

    /* Decrease fanout count for source node and expand incoming
     * edges if all incoming edges have been seen. */
    --next->from->info.fanin;
    if (next->from->info.fanin == 0) {
        latlink_list_t *x;

        if (start == NULL)
            start = dag->start;
        if (next->from == start) {
            /* If we have traversed all links entering the start node,
             * clear the queue, causing future calls to this function
             * to return NULL. */
            lattice_delq(dag);
            return next;
        }

        /* Extend all outgoing edges. */
        for (x = next->from->entries; x; x = x->next)
            lattice_pushq(dag, x->link);
    }
    return next;
}

/*
 * Find the best score from dag->start to end point of any link and
 * use it to update links further down the path.  This is like
 * single-source shortest path search, except that it is done over
 * edges rather than nodes, which allows us to do exact trigram scoring.
 *
 * Helpfully enough, we get half of the posterior probability
 * calculation for free that way too.  (interesting research topic: is
 * there a reliable Viterbi analogue to word-level Forward-Backward
 * like there is for state-level?  Or, is it just lattice density?)
 */
latlink_t *
lattice_bestpath(lattice_t *dag, float32 ascale)
{
    search_module_t *search;
    latnode_t *node;
    latlink_t *link;
    latlink_t *bestend;
    latlink_list_t *x;
    logmath_t *lmath;
    int32 bestescr;

    search = dag->search;
    lmath = dag->lmath;

    /* Initialize path scores for all links exiting dag->start, and
     * set all other scores to the minimum.  Also initialize alphas to
     * log-zero. */
    for (node = dag->nodes; node; node = node->next) {
        for (x = node->exits; x; x = x->next) {
            x->link->path_scr = MAX_NEG_INT32;
            x->link->alpha = logmath_get_zero(lmath);
        }
    }
    for (x = dag->start->exits; x; x = x->next) {
        /* Best path points to dag->start, obviously. */
        x->link->path_scr = x->link->ascr;
        x->link->best_prev = NULL;
        /* No predecessors for start links. */
        x->link->alpha = 0;
    }

    /* Traverse the edges in the graph, updating path scores. */
    for (link = lattice_traverse_edges(dag, NULL, NULL);
         link; link = lattice_traverse_next(dag, NULL)) {
        int32 bprob;
        int32 w3_wid, w2_wid;
        int16 w3_is_fil, w2_is_fil;
        latlink_t *prev_link;

        /* Sanity check, we should not be traversing edges that
         * weren't previously updated, otherwise nasty overflows will result. */
        assert(link->path_scr != MAX_NEG_INT32);

        /* Find word predecessor if from-word is filler */
        w3_wid = link->from->basewid;
        w2_wid = link->to->basewid;
        w3_is_fil = dict_filler_word(search_module_dict(search), link->from->basewid) && link->from != dag->start;
        w2_is_fil = dict_filler_word(search_module_dict(search), w2_wid) && link->to != dag->end;
        prev_link = link;

        if (w3_is_fil) {
            while (prev_link->best_prev != NULL) {
                prev_link = prev_link->best_prev;
                w3_wid = prev_link->from->basewid;
                if (!dict_filler_word(search_module_dict(search), w3_wid) || prev_link->from == dag->start) {
                    w3_is_fil = FALSE;
                    break;
                }
            }
        }

        /* Calculate common bigram probability for all alphas. */
        bprob = 0;
        /* Add in this link's acoustic score, which was a constant
           factor in previous computations (if any). */
        link->alpha += (int32)((link->ascr << SENSCR_SHIFT) * ascale);

        if (w2_is_fil) {
            w2_is_fil = w3_is_fil;
            w3_is_fil = TRUE;
            w2_wid = w3_wid;
            while (prev_link->best_prev != NULL) {
                prev_link = prev_link->best_prev;
                w3_wid = prev_link->from->basewid;
                if (!dict_filler_word(search_module_dict(search), w3_wid) || prev_link->from == dag->start) {
                    w3_is_fil = FALSE;
                    break;
                }
            }
        }

        /* Update scores for all paths exiting link->to. */
        for (x = link->to->exits; x; x = x->next) {
            int32 score;

            /* Update alpha with sum of previous alphas. */
            x->link->alpha = logmath_add(lmath, x->link->alpha, link->alpha + bprob);

            /* Update link score with maximum link score. */
            score = link->path_scr + x->link->ascr;

            if (score BETTER_THAN x->link->path_scr) {
                x->link->path_scr = score;
                x->link->best_prev = link;
            }
        }
    }

    /* Find best link entering final node, and calculate normalizer
     * for posterior probabilities. */
    bestend = NULL;
    bestescr = MAX_NEG_INT32;

    /* Normalizer is the alpha for the imaginary link exiting the
       final node. */
    dag->norm = logmath_get_zero(lmath);
    for (x = dag->end->entries; x; x = x->next) {
        int32 bprob;
        int32 from_wid;
        int16 from_is_fil;

        from_wid = x->link->from->basewid;
        from_is_fil = dict_filler_word(search_module_dict(search), from_wid) && x->link->from != dag->start;
        if (from_is_fil) {
            latlink_t *prev_link = x->link;
            while (prev_link->best_prev != NULL) {
                prev_link = prev_link->best_prev;
                from_wid = prev_link->from->basewid;
                if (!dict_filler_word(search_module_dict(search), from_wid) || prev_link->from == dag->start) {
                    from_is_fil = FALSE;
                    break;
                }
            }
        }

        bprob = 0;
        dag->norm = logmath_add(lmath, dag->norm, x->link->alpha + bprob);
        if (x->link->path_scr BETTER_THAN bestescr) {
            bestescr = x->link->path_scr;
            bestend = x->link;
        }
    }
    /* FIXME: floating point... */
    /* FIXME: possible under/overflow!!! */
    dag->norm += (int32)(dag->final_node_ascr << SENSCR_SHIFT) * ascale;

    E_INFO("Bestpath score: %d\n", bestescr);
    E_INFO("Normalizer P(O) = alpha(%s:%d:%d) = %d\n",
           dict_wordstr(dag->search->dict, dag->end->wid),
           dag->end->sf, dag->end->lef,
           dag->norm);
    return bestend;
}

static int32
lattice_joint(lattice_t *dag, latlink_t *link, float32 ascale)
{
    int32 jprob;

    jprob = (int32)((dag->final_node_ascr << SENSCR_SHIFT) * ascale);
    while (link) {
        /* If there is no language model, we assume that the language
           model probability (such as it is) has been included in the
           link score. */
        jprob += (int32)((link->ascr << SENSCR_SHIFT) * ascale);
        link = link->best_prev;
    }

    E_INFO("Joint P(O,S) = %d P(S|O) = %d\n", jprob, jprob - dag->norm);
    return jprob;
}

int32
lattice_posterior(lattice_t *dag, float32 ascale)
{
    logmath_t *lmath;
    latnode_t *node;
    latlink_t *link;
    latlink_list_t *x;
    latlink_t *bestend;
    int32 bestescr;

    lmath = dag->lmath;

    /* Reset all betas to zero. */
    for (node = dag->nodes; node; node = node->next) {
        for (x = node->exits; x; x = x->next) {
            x->link->beta = logmath_get_zero(lmath);
        }
    }

    bestend = NULL;
    bestescr = MAX_NEG_INT32;
    /* Accumulate backward probabilities for all links. */
    for (link = lattice_reverse_edges(dag, NULL, NULL);
         link; link = lattice_reverse_next(dag, NULL)) {
        int32 bprob;
        int32 from_wid, to_wid;
        int16 from_is_fil, to_is_fil;

        from_wid = link->from->basewid;
        to_wid = link->to->basewid;
        from_is_fil = dict_filler_word(dag->dict, from_wid) && link->from != dag->start;
        to_is_fil = dict_filler_word(dag->dict, to_wid) && link->to != dag->end;

        /* Find word predecessor if from-word is filler */
        if (!to_is_fil && from_is_fil) {
            latlink_t *prev_link = link;
            while (prev_link->best_prev != NULL) {
                prev_link = prev_link->best_prev;
                from_wid = prev_link->from->basewid;
                if (!dict_filler_word(dag->dict, from_wid) || prev_link->from == dag->start) {
                    from_is_fil = FALSE;
                    break;
                }
            }
        }

        /* Calculate LM probability. */
        bprob = 0;

        if (link->to == dag->end) {
            /* Track the best path - we will backtrace in order to
               calculate the unscaled joint probability for sentence
               posterior. */
            if (link->path_scr BETTER_THAN bestescr) {
                bestescr = link->path_scr;
                bestend = link;
            }
            /* Imaginary exit link from final node has beta = 1.0 */
            link->beta = bprob + (int32)((dag->final_node_ascr << SENSCR_SHIFT) * ascale);
        } else {
            /* Update beta from all outgoing betas. */
            for (x = link->to->exits; x; x = x->next) {
                link->beta = logmath_add(lmath, link->beta,
                                         x->link->beta + bprob
                                             + (int)((x->link->ascr << SENSCR_SHIFT) * ascale));
            }
        }
    }

    /* Return P(S|O) = P(O,S)/P(O) */
    return lattice_joint(dag, bestend, ascale) - dag->norm;
}

/* Mark every node that has a path to the argument dagnode as "reachable". */
static void
dag_mark_reachable(latnode_t *d)
{
    latlink_list_t *l;

    d->reachable = 1;
    for (l = d->entries; l; l = l->next)
        if (l->link->from && !l->link->from->reachable)
            dag_mark_reachable(l->link->from);
}

int32
lattice_posterior_prune(lattice_t *dag, int32 beam)
{
    latlink_t *link;
    int npruned = 0;

    for (link = lattice_traverse_edges(dag, dag->start, dag->end);
         link; link = lattice_traverse_next(dag, dag->end)) {
        link->from->reachable = FALSE;
        if (link->alpha + link->beta - dag->norm < beam) {
            latlink_list_t *x, *tmp, *next;
            tmp = NULL;
            for (x = link->from->exits; x; x = next) {
                next = x->next;
                if (x->link == link) {
                    listelem_free(dag->latlink_list_alloc, x);
                } else {
                    x->next = tmp;
                    tmp = x;
                }
            }
            link->from->exits = tmp;
            tmp = NULL;
            for (x = link->to->entries; x; x = next) {
                next = x->next;
                if (x->link == link) {
                    listelem_free(dag->latlink_list_alloc, x);
                } else {
                    x->next = tmp;
                    tmp = x;
                }
            }
            link->to->entries = tmp;
            listelem_free(dag->latlink_alloc, link);
            ++npruned;
        }
    }
    dag_mark_reachable(dag->end);
    lattice_delete_unreachable(dag);
    return npruned;
}

/* Parameters to prune n-best alternatives search */
#define MAX_PATHS 500 /* Max allowed active paths at any time */
#define MAX_HYP_TRIES 10000

/*
 * For each node in any path between from and end of utt, find the
 * best score from "from".sf to end of utt.  (NOTE: Uses bigram probs;
 * this is an estimate of the best score from "from".)  (NOTE #2: yes,
 * this is the "heuristic score" used in A* search)
 */
static int32
best_rem_score(astar_search_t *nbest, latnode_t *from)
{
    latlink_list_t *x;
    int32 bestscore, score;

    if (from->info.rem_score <= 0)
        return (from->info.rem_score);

    /* Best score from "from" to end of utt not known; compute from successors */
    bestscore = WORST_SCORE;
    for (x = from->exits; x; x = x->next) {
        score = best_rem_score(nbest, x->link->to);
        score += x->link->ascr;
        if (score BETTER_THAN bestscore)
            bestscore = score;
    }
    from->info.rem_score = bestscore;

    return bestscore;
}

/*
 * Insert newpath in sorted (by path score) list of paths.  But if newpath is
 * too far down the list, drop it (FIXME: necessary?)
 * total_score = path score (newpath) + rem_score to end of utt.
 */
static void
path_insert(astar_search_t *nbest, latpath_t *newpath, int32 total_score)
{
    latpath_t *prev, *p;
    int32 i;

    prev = NULL;
    for (i = 0, p = nbest->path_list; (i < MAX_PATHS) && p; p = p->next, i++) {
        if ((p->score + p->node->info.rem_score) < total_score)
            break;
        prev = p;
    }

    /* newpath should be inserted between prev and p */
    if (i < MAX_PATHS) {
        /* Insert new partial hyp */
        newpath->next = p;
        if (!prev)
            nbest->path_list = newpath;
        else
            prev->next = newpath;
        if (!p)
            nbest->path_tail = newpath;

        nbest->n_path++;
        nbest->n_hyp_insert++;
        nbest->insert_depth += i;
    } else {
        /* newpath score too low; reject it and also prune paths beyond MAX_PATHS */
        nbest->path_tail = prev;
        prev->next = NULL;
        nbest->n_path = MAX_PATHS;
        listelem_free(nbest->latpath_alloc, newpath);

        nbest->n_hyp_reject++;
        for (; p; p = newpath) {
            newpath = p->next;
            listelem_free(nbest->latpath_alloc, p);
            nbest->n_hyp_reject++;
        }
    }
}

/* Find all possible extensions to given partial path */
static void
path_extend(astar_search_t *nbest, latpath_t *path)
{
    latlink_list_t *x;
    latpath_t *newpath;
    int32 total_score, tail_score;

    /* Consider all successors of path->node */
    for (x = path->node->exits; x; x = x->next) {
        /* Skip successor if no path from it reaches the final node */
        if (x->link->to->info.rem_score <= WORST_SCORE)
            continue;

        /* Create path extension and compute exact score for this extension */
        newpath = listelem_malloc(nbest->latpath_alloc);
        newpath->node = x->link->to;
        newpath->parent = path;
        newpath->score = path->score + x->link->ascr;

        /* Insert new partial path hypothesis into sorted path_list */
        nbest->n_hyp_tried++;
        total_score = newpath->score + newpath->node->info.rem_score;

        /* First see if hyp would be worse than the worst */
        if (nbest->n_path >= MAX_PATHS) {
            tail_score = nbest->path_tail->score
                + nbest->path_tail->node->info.rem_score;
            if (total_score < tail_score) {
                listelem_free(nbest->latpath_alloc, newpath);
                nbest->n_hyp_reject++;
                continue;
            }
        }

        path_insert(nbest, newpath, total_score);
    }
}

astar_search_t *
astar_search_start(lattice_t *dag,
                   int sf, int ef,
                   int w1, int w2)
{
    astar_search_t *nbest;
    latnode_t *node;

    nbest = ckd_calloc(1, sizeof(*nbest));
    nbest->dag = dag;
    nbest->sf = sf;
    if (ef < 0)
        nbest->ef = dag->n_frames + 1;
    else
        nbest->ef = ef;
    nbest->w1 = w1;
    nbest->w2 = w2;
    nbest->latpath_alloc = listelem_alloc_init(sizeof(latpath_t));

    /* Initialize rem_score (A* heuristic) to default values */
    for (node = dag->nodes; node; node = node->next) {
        if (node == dag->end)
            node->info.rem_score = 0;
        else if (node->exits == NULL)
            node->info.rem_score = WORST_SCORE;
        else
            node->info.rem_score = 1; /* +ve => unknown value */
    }

    /* Create initial partial hypotheses list consisting of nodes starting at sf */
    nbest->path_list = nbest->path_tail = NULL;
    for (node = dag->nodes; node; node = node->next) {
        if (node->sf == sf) {
            latpath_t *path;

            best_rem_score(nbest, node);
            path = listelem_malloc(nbest->latpath_alloc);
            path->node = node;
            path->parent = NULL;
            path->score = 0;
            path->score >>= SENSCR_SHIFT;
            path_insert(nbest, path, path->score + node->info.rem_score);
        }
    }

    return nbest;
}

latpath_t *
astar_next(astar_search_t *nbest)
{
    lattice_t *dag;

    dag = nbest->dag;

    /* Pop the top (best) partial hypothesis */
    while ((nbest->top = nbest->path_list) != NULL) {
        nbest->path_list = nbest->path_list->next;
        if (nbest->top == nbest->path_tail)
            nbest->path_tail = NULL;
        nbest->n_path--;

        /* Complete hypothesis? */
        if ((nbest->top->node->sf >= nbest->ef)
            || ((nbest->top->node == dag->end) && (nbest->ef > dag->end->sf))) {
            /* FIXME: Verify that it is non-empty.  Also we may want
             * to verify that it is actually distinct from other
             * paths, since often this is not the case*/
            return nbest->top;
        } else {
            if (nbest->top->node->fef < nbest->ef)
                path_extend(nbest, nbest->top);
        }
    }

    /* Did not find any more paths to extend. */
    return NULL;
}

const char *
astar_hyp(astar_search_t *nbest, latpath_t *path)
{
    search_module_t *search;
    latpath_t *p;
    size_t len;
    char *c;
    char *hyp;

    search = nbest->dag->search;

    /* Backtrace once to get hypothesis length. */
    len = 0;
    for (p = path; p; p = p->parent) {
        if (dict_real_word(search_module_dict(search), p->node->basewid)) {
            char *wstr = dict_wordstr(search_module_dict(search), p->node->basewid);
            if (wstr != NULL)
                len += strlen(wstr) + 1;
        }
    }

    if (len == 0) {
        return NULL;
    }

    /* Backtrace again to construct hypothesis string. */
    hyp = ckd_calloc(1, len);
    c = hyp + len - 1;
    for (p = path; p; p = p->parent) {
        if (dict_real_word(search_module_dict(search), p->node->basewid)) {
            char *wstr = dict_wordstr(search_module_dict(search), p->node->basewid);
            if (wstr != NULL) {
                len = strlen(wstr);
                c -= len;
                memcpy(c, wstr, len);
                if (c > hyp) {
                    --c;
                    *c = ' ';
                }
            }
        }
    }

    nbest->hyps = glist_add_ptr(nbest->hyps, hyp);
    return hyp;
}

static void
astar_node2itor(astar_seg_t *itor)
{
    seg_iter_t *seg = (seg_iter_t *)itor;
    latnode_t *node;

    assert(itor->cur < itor->n_nodes);
    node = itor->nodes[itor->cur];
    if (itor->cur == itor->n_nodes - 1)
        seg->ef = node->lef;
    else
        seg->ef = itor->nodes[itor->cur + 1]->sf - 1;
    seg->word = dict_wordstr(search_module_dict(seg->search), node->wid);
    seg->sf = node->sf;
    seg->prob = 0; /* FIXME: implement forward-backward */
}

static void
astar_search_seg_free(seg_iter_t *seg)
{
    astar_seg_t *itor = (astar_seg_t *)seg;
    ckd_free(itor->nodes);
    ckd_free(itor);
}

static seg_iter_t *
astar_search_seg_next(seg_iter_t *seg)
{
    astar_seg_t *itor = (astar_seg_t *)seg;

    ++itor->cur;
    if (itor->cur == itor->n_nodes) {
        astar_search_seg_free(seg);
        return NULL;
    } else {
        astar_node2itor(itor);
    }

    return seg;
}

static ps_segfuncs_t astar_search_segfuncs = {
    /* seg_next */ astar_search_seg_next,
    /* seg_free */ astar_search_seg_free
};

seg_iter_t *
astar_search_seg_iter(astar_search_t *astar, latpath_t *path)
{
    astar_seg_t *itor;
    latpath_t *p;
    int cur;

    /* Backtrace and make an iterator, this should look familiar by now. */
    itor = ckd_calloc(1, sizeof(*itor));
    itor->base.vt = &astar_search_segfuncs;
    itor->base.search = astar->dag->search;
    itor->n_nodes = itor->cur = 0;
    for (p = path; p; p = p->parent) {
        ++itor->n_nodes;
    }
    itor->nodes = ckd_calloc(itor->n_nodes, sizeof(*itor->nodes));
    cur = itor->n_nodes - 1;
    for (p = path; p; p = p->parent) {
        itor->nodes[cur] = p->node;
        --cur;
    }

    astar_node2itor(itor);
    return (seg_iter_t *)itor;
}

void
astar_finish(astar_search_t *nbest)
{
    gnode_t *gn;

    /* Free all hyps. */
    for (gn = nbest->hyps; gn; gn = gnode_next(gn)) {
        ckd_free(gnode_ptr(gn));
    }
    glist_free(nbest->hyps);
    /* Free all paths. */
    listelem_alloc_free(nbest->latpath_alloc);
    /* Free the Henge. */
    ckd_free(nbest);
}

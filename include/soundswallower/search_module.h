/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2008 Carnegie Mellon University.  All rights
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
 * @file search_module.h Internal search module API for SoundSwallower
 */

#ifndef __SEARCH_MODULE_H__
#define __SEARCH_MODULE_H__

#include <soundswallower/prim_type.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/* Forward declare everything (we cannot and don't need to include
 * these headers) */
typedef struct acmod_s acmod_t;
typedef struct config_s config_t;
typedef struct fe_s fe_t;
typedef struct lattice_s lattice_t;
typedef struct latlink_s latlink_t;
typedef struct dict_s dict_t;
typedef struct dict2pid_s dict2pid_t;
typedef struct seg_iter_s seg_iter_t;
typedef struct search_module_s search_module_t;

/* Search names (FIXME: not used, must be destroyed) */
#define PS_DEFAULT_SEARCH "_default"

/* Search types */
#define PS_SEARCH_TYPE_FSG "fsg"
#define PS_SEARCH_TYPE_STATE_ALIGN "state_align"

/**
 * V-table for search algorithm.
 */
typedef struct searchfuncs_s {
    int (*start)(search_module_t *search);
    int (*step)(search_module_t *search, int frame_idx);
    int (*finish)(search_module_t *search);
    int (*reinit)(search_module_t *search, dict_t *dict, dict2pid_t *d2p);
    void (*free)(search_module_t *search);

    lattice_t *(*lattice)(search_module_t *search);
    const char *(*hyp)(search_module_t *search, int32 *out_score);
    int32 (*prob)(search_module_t *search);
    seg_iter_t *(*seg_iter)(search_module_t *search);
} searchfuncs_t;

/**
 * Base structure for search module.
 */
struct search_module_s {
    searchfuncs_t *vt; /**< V-table of search methods. */

    char *type;
    char *name;

    config_t *config; /**< Configuration. */
    acmod_t *acmod; /**< Acoustic model. */
    dict_t *dict; /**< Pronunciation dictionary. */
    dict2pid_t *d2p; /**< Dictionary to senone mappings. */
    char *hyp_str; /**< Current hypothesis string. */
    lattice_t *dag; /**< Current hypothesis word graph. */
    latlink_t *last_link; /**< Final link in best path. */
    int32 post; /**< Utterance posterior probability. */
    int32 n_words; /**< Number of words known to search (may
                      be less than in the dictionary) */

    /* Magical word IDs that must exist in the dictionary: */
    int32 start_wid; /**< Start word ID. */
    int32 silence_wid; /**< Silence word ID. */
    int32 finish_wid; /**< Finish word ID. */
};

#define search_module_base(s) ((search_module_t *)s)
#define search_module_config(s) search_module_base(s)->config
#define search_module_acmod(s) search_module_base(s)->acmod
#define search_module_dict(s) search_module_base(s)->dict
#define search_module_dict2pid(s) search_module_base(s)->d2p
#define search_module_dag(s) search_module_base(s)->dag
#define search_module_last_link(s) search_module_base(s)->last_link
#define search_module_post(s) search_module_base(s)->post
#define search_module_n_words(s) search_module_base(s)->n_words

#define search_module_type(s) search_module_base(s)->type
#define search_module_name(s) search_module_base(s)->name
#define search_module_start(s) (*(search_module_base(s)->vt->start))(s)
#define search_module_step(s, i) (*(search_module_base(s)->vt->step))(s, i)
#define search_module_finish(s) (*(search_module_base(s)->vt->finish))(s)
#define search_module_reinit(s, d, d2p) (*(search_module_base(s)->vt->reinit))(s, d, d2p)
#define search_module_free(s) (*(search_module_base(s)->vt->free))(s)
#define search_module_lattice(s) (*(search_module_base(s)->vt->lattice))(s)
#define search_module_hyp(s, sc) (*(search_module_base(s)->vt->hyp))(s, sc)
#define search_module_prob(s) (*(search_module_base(s)->vt->prob))(s)
#define search_module_seg_iter(s) (*(search_module_base(s)->vt->seg_iter))(s)

/* For convenience... */
#define search_module_silence_wid(s) search_module_base(s)->silence_wid
#define search_module_start_wid(s) search_module_base(s)->start_wid
#define search_module_finish_wid(s) search_module_base(s)->finish_wid

/**
 * Initialize base structure.
 */
void search_module_init(search_module_t *search, searchfuncs_t *vt,
                        const char *type, const char *name,
                        config_t *config, acmod_t *acmod, dict_t *dict,
                        dict2pid_t *d2p);

/**
 * Free search
 */
void search_module_base_free(search_module_t *search);

/**
 * Re-initialize base structure with new dictionary.
 */
void search_module_base_reinit(search_module_t *search, dict_t *dict,
                               dict2pid_t *d2p);

typedef struct ps_segfuncs_s {
    seg_iter_t *(*seg_next)(seg_iter_t *seg);
    void (*seg_free)(seg_iter_t *seg);
} ps_segfuncs_t;

/**
 * Base structure for hypothesis segmentation iterator.
 */
struct seg_iter_s {
    ps_segfuncs_t *vt; /**< V-table of seg methods */
    search_module_t *search; /**< Search object from whence this came */
    const char *word; /**< Word string (pointer into dictionary hash) */
    frame_idx_t sf; /**< Start frame. */
    frame_idx_t ef; /**< End frame. */
    int32 ascr; /**< Acoustic score. */
    int32 lscr; /**< Language model score. */
    int32 prob; /**< Log posterior probability. */
};

#define search_module_seg_next(seg) (*(seg->vt->seg_next))(seg)
#define search_module_seg_free(s) (*(seg->vt->seg_free))(seg)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __SEARCH_MODULE_H__ */

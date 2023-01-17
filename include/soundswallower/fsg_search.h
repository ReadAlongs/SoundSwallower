/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
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

/*
 * fsg_search_internal.h -- Search structures for FSG decoding.
 */

#ifndef __S2_FSG_SEARCH_H__
#define __S2_FSG_SEARCH_H__

#include <soundswallower/configuration.h>
#include <soundswallower/decoder.h>
#include <soundswallower/fsg_history.h>
#include <soundswallower/fsg_lextree.h>
#include <soundswallower/fsg_model.h>
#include <soundswallower/glist.h>
#include <soundswallower/hmm.h>
#include <soundswallower/search_module.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * Segmentation "iterator" for FSG history.
 */
typedef struct fsg_seg_s {
    seg_iter_t base; /**< Base structure. */
    fsg_hist_entry_t **hist; /**< Sequence of history entries. */
    int16 n_hist; /**< Number of history entries. */
    int16 cur; /**< Current position in hist. */
} fsg_seg_t;

/**
 * Implementation of FSG search (and "FSG set") structure.
 */
typedef struct fsg_search_s {
    search_module_t base;

    hmm_context_t *hmmctx; /**< HMM context. */

    fsg_model_t *fsg; /**< FSG model */
    struct fsg_lextree_s *lextree; /**< Lextree structure for the currently
                                    active FSG */
    struct fsg_history_s *history; /**< For storing the Viterbi search history */

    glist_t pnode_active; /**< Those active in this frame */
    glist_t pnode_active_next; /**< Those activated for the next frame */

    int32 beam_orig; /**< Global pruning threshold */
    int32 pbeam_orig; /**< Pruning threshold for phone transition */
    int32 wbeam_orig; /**< Pruning threshold for word exit */
    float32 beam_factor; /**< Dynamic/adaptive factor (<=1) applied to above
                              beams to determine actual effective beams.
                              For implementing absolute pruning. */
    int32 beam, pbeam, wbeam; /**< Effective beams after applying beam_factor */
    float32 lw; /**< Language weight */
    int32 pip, wip; /**< Log insertion penalties */

    frame_idx_t frame; /**< Current frame. */
    uint8 final; /**< Decoding is finished for this utterance. */
    uint8 bestpath; /**< Whether to run bestpath search
                       and confidence annotation at end. */
    float32 ascale; /**< Acoustic score scale for posterior probabilities. */

    int32 bestscore; /**< For beam pruning */
    int32 bpidx_start; /**< First history entry index this frame */

    int32 ascr, lscr; /**< Total acoustic and lm score for utt */

    int32 n_hmm_eval; /**< Total HMMs evaluated this utt */
    int32 n_sen_eval; /**< Total senones evaluated this utt */

    ptmr_t perf; /**< Performance counter */
    int32 n_tot_frame;

} fsg_search_t;

/* Access macros */
#define fsg_search_frame(s) ((s)->frame)

/**
 * Create, initialize and return a search module.
 */
search_module_t *fsg_search_init(const char *name,
                                 fsg_model_t *fsg,
                                 config_t *config,
                                 acmod_t *acmod,
                                 dict_t *dict,
                                 dict2pid_t *d2p);

/**
 * Deallocate search structure.
 */
void fsg_search_free(search_module_t *search);

/**
 * Update FSG search module for new or updated FSGs.
 */
int fsg_search_reinit(search_module_t *fsgs, dict_t *dict, dict2pid_t *d2p);

/**
 * Prepare the FSG search structure for beginning decoding of the next
 * utterance.
 */
int fsg_search_start(search_module_t *search);

/**
 * Step one frame forward through the Viterbi search.
 */
int fsg_search_step(search_module_t *search, int frame_idx);

/**
 * Windup and clean the FSG search structure after utterance.
 */
int fsg_search_finish(search_module_t *search);

/**
 * Get hypothesis string from the FSG search.
 */
const char *fsg_search_hyp(search_module_t *search, int32 *out_score);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

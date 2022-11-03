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
 * @file acmod.h Acoustic model structures for SoundSwallower
 * @author David Huggins-Daines <dhdaines@gmail.com>
 */

#ifndef __ACMOD_H__
#define __ACMOD_H__

/* System headers. */
#include <stdio.h>

#include <soundswallower/configuration.h>
#include <soundswallower/logmath.h>
#include <soundswallower/fe.h>
#include <soundswallower/feat.h>
#include <soundswallower/bitvec.h>
#include <soundswallower/err.h>
#include <soundswallower/prim_type.h>
#include <soundswallower/mllr.h>
#include <soundswallower/bin_mdef.h>
#include <soundswallower/tmat.h>
#include <soundswallower/hmm.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * States in utterance processing.
 */
typedef enum acmod_state_e {
    ACMOD_IDLE,		/**< Not in an utterance. */
    ACMOD_STARTED,      /**< Utterance started, no data yet. */
    ACMOD_PROCESSING,   /**< Utterance in progress. */
    ACMOD_ENDED         /**< Utterance ended, still buffering. */
} acmod_state_t;

/**
 * Is acmod growable by default? (yes, for minimal surprise)
 */
#define ACMOD_GROW_DEFAULT TRUE

/**
 * Dummy senone score value for unintentionally active states.
 */
#define SENSCR_DUMMY 0x7fff


/**
 * Acoustic model parameter structure. 
 */
typedef struct mgau_s mgau_t;

typedef struct mgaufuncs_s {
    const char *name;

    int (*frame_eval)(mgau_t *mgau,
                      int16 *senscr,
                      uint8 *senone_active,
                      int32 n_senone_active,
                      mfcc_t ** feat,
                      int32 frame,
                      int32 compallsen);
    int (*transform)(mgau_t *mgau,
                     mllr_t *mllr);
    void (*free)(mgau_t *mgau);
} mgaufuncs_t;    

struct mgau_s {
    mgaufuncs_t *vt;  /**< vtable of mgau functions. */
    int frame_idx;       /**< frame counter. */
};

#define ps_mgau_base(mg) ((mgau_t *)(mg))
#define ps_mgau_frame_eval(mg,senscr,senone_active,n_senone_active,feat,frame,compallsen) \
    (*ps_mgau_base(mg)->vt->frame_eval)                                 \
    (mg, senscr, senone_active, n_senone_active, feat, frame, compallsen)
#define mgau_transform(mg, mllr)                                  \
    (*ps_mgau_base(mg)->vt->transform)(mg, mllr)
#define ps_mgau_free(mg)                                  \
    (*ps_mgau_base(mg)->vt->free)(mg)

/**
 * Acoustic model structure.
 *
 * This object encapsulates all stages of acoustic processing, from
 * raw audio input to acoustic score output.  The reason for grouping
 * all of these modules together is that they all have to "agree" in
 * their parameterizations, and the configuration of the acoustic and
 * dynamic feature computation is completely dependent on the
 * parameters used to build the original acoustic model (which should
 * by now always be specified in a feat.params file).
 *
 * Because there is not a one-to-one correspondence from blocks of
 * input audio or frames of input features to frames of acoustic
 * scores (due to dynamic feature calculation), results may not be
 * immediately available after input, and the output results will not
 * correspond to the last piece of data input.
 */
struct acmod_s {
    /* Global objects, not retained. */
    config_t *config;          /**< Configuration. */
    logmath_t *lmath;          /**< Log-math computation. */
    glist_t strings;           /**< Temporary acoustic model filenames. */

    /* Feature computation: */
    fe_t *fe;                  /**< Acoustic feature computation. */
    feat_t *fcb;               /**< Dynamic feature computation. */

    /* Model parameters: */
    bin_mdef_t *mdef;          /**< Model definition. */
    tmat_t *tmat;              /**< Transition matrices. */
    mgau_t *mgau;           /**< Model parameters. */
    mllr_t *mllr;           /**< Speaker transformation. */

    /* Senone scoring: */
    int16 *senone_scores;      /**< GMM scores for current frame. */
    bitvec_t *senone_active_vec; /**< Active GMMs in current frame. */
    uint8 *senone_active;      /**< Array of deltas to active GMMs. */
    int senscr_frame;          /**< Frame index for senone_scores. */
    int n_senone_active;       /**< Number of active GMMs. */
    int log_zero;              /**< Zero log-probability value. */

    /* Utterance processing: */
    mfcc_t **mfc_buf;   /**< Temporary buffer of acoustic features. */
    mfcc_t ***feat_buf; /**< Temporary buffer of dynamic features. */
    long *framepos;     /**< File positions of recent frames in senone file. */

    /* A whole bunch of flags and counters: */
    uint8 state;        /**< State of utterance processing. */
    uint8 compallsen;   /**< Compute all senones? */
    uint8 grow_feat;    /**< Whether to grow feat_buf. */
    uint8 insen_swap;   /**< Whether to swap input senone score. */

    frame_idx_t output_frame; /**< Index of next frame of dynamic features. */
    frame_idx_t n_mfc_alloc;  /**< Number of frames allocated in mfc_buf */
    frame_idx_t n_mfc_frame;  /**< Number of frames active in mfc_buf */
    frame_idx_t mfc_outidx;   /**< Start of active frames in mfc_buf */
    frame_idx_t n_feat_alloc; /**< Number of frames allocated in feat_buf */
    frame_idx_t n_feat_frame; /**< Number of frames active in feat_buf */
    frame_idx_t feat_outidx;  /**< Start of active frames in feat_buf */
};
typedef struct acmod_s acmod_t;

/**
 * Initialize an acoustic model.
 *
 * @param config a command-line object containing parameters.
 *               Ownership of this pointer is retained by this object,
 *               so you may free it if you no longer need it.
 * @param lmath global log-math parameters.
 * @param fe a previously-initialized acoustic feature module to use.
 *           If this is supplied and its parameters do not match those
 *           in the acoustic model, this function will fail.  This
 *           pointer is retained.
 * @param fcb a previously-initialized dynamic feature module to use.
 *           If this is supplied and its parameters do not match those
 *           in the acoustic model, this function will fail.  This
 *           pointer is retained.
 * @return a newly initialized acmod_t, or NULL on failure.
 */
acmod_t *acmod_init(config_t *config, logmath_t *lmath, fe_t *fe, feat_t *fcb);

/**
 * Create the acmod without loading any files.
 */
acmod_t *acmod_create(config_t *config, logmath_t *lmath, fe_t *fe, feat_t *fcb);

/**
 * Reinitialize acmod with new feature computation modules.
 */
int acmod_reinit_feat(acmod_t *acmod, fe_t *fe, feat_t *fcb);

/**
 * Load acoustic model files.
 */
int acmod_load_am(acmod_t *acmod);

/**
 * Initialize senone scoring (after loading acoustic model files).
 */
int acmod_init_senscr(acmod_t *acmod);

/**
 * Verify that feature extraction parameters are compatible with
 * acoustic model.
 *
  * @param fe acoustic feature extraction module to verify.
 * @return TRUE if compatible, FALSE otherwise
 */
int acmod_fe_mismatch(acmod_t *acmod, fe_t *fe);

/**
 * Verify that dynamic feature computation parameters are compatible
 * with acoustic model.
 *
 * @param fcb dynamic feature computation module to verify.
 * @return TRUE if compatible, FALSE otherwise
 */
int acmod_feat_mismatch(acmod_t *acmod, feat_t *fcb);

/**
 * Adapt acoustic model using a linear transform.
 *
 * @param mllr The new transform to use, or NULL to update the
 *              existing transform.  The decoder retains ownership of
 *              this pointer, so you may free it if you no longer need
 *              it.
 * @return The updated transform object for this decoder, or
 *         NULL on failure.
 */
mllr_t *acmod_update_mllr(acmod_t *acmod, mllr_t *mllr);

/**
 * Finalize an acoustic model.
 */
void acmod_free(acmod_t *acmod);

/**
 * Mark the start of an utterance.
 */
int acmod_start_utt(acmod_t *acmod);

/**
 * Mark the end of an utterance.
 */
int acmod_end_utt(acmod_t *acmod);

/**
 * Rewind the current utterance, allowing it to be rescored.
 *
 * After calling this function, the internal frame index is reset, and
 * acmod_score() will return scores starting at the first frame of the
 * current utterance.  The acmod must be growable (which it probably
 * is unless you disabled this with acmod_set_grow() - see
 * ACMOD_GROW_DEFAULT).
 *
 * @return 0 for success, <0 for failure (if the utterance can't be
 *         rewound due to no feature or score data available)
 */
int acmod_rewind(acmod_t *acmod);

/**
 * Advance the frame index.
 *
 * This function moves to the next frame of input data.  Subsequent
 * calls to acmod_score() will return scores for that frame, until the
 * next call to acmod_advance().
 *
 * @return New frame index.
 */
int acmod_advance(acmod_t *acmod);

/**
 * Set memory allocation policy for utterance processing.
 *
 * @param grow_feat If non-zero, the internal dynamic feature buffer
 * will expand as necessary to encompass any amount of data fed to the
 * model.
 * @return previous allocation policy.
 */
int acmod_set_grow(acmod_t *acmod, int grow_feat);

/**
 * TODO: Set queue length for utterance processing.
 *
 * This function allows multiple concurrent passes of search to
 * operate on different parts of the utterance.
 */

/**
 * Feed raw audio data to the acoustic model for scoring.
 *
 * @param inout_raw In: Pointer to buffer of raw samples
 *                  Out: Pointer to next sample to be read
 * @param inout_n_samps In: Number of samples available
 *                      Out: Number of samples remaining
 * @param full_utt If non-zero, this block represents a full
 *                 utterance and should be processed as such.
 * @return Number of frames of data processed.
 */
int acmod_process_raw(acmod_t *acmod,
                      int16 **inout_raw,
                      size_t *inout_n_samps,
                      int full_utt);

/**
 * Feed raw audio data in float32 format (in range [-1.0,1.0]) to the
 * acoustic model for scoring.
 *
 * @param inout_raw In: Pointer to buffer of raw samples
 *                  Out: Pointer to next sample to be read
 * @param inout_n_samps In: Number of samples available
 *                      Out: Number of samples remaining
 * @param full_utt If non-zero, this block represents a full
 *                 utterance and should be processed as such.
 * @return Number of frames of data processed.
 */
int
acmod_process_float32(acmod_t *acmod,
                      float32 **inout_raw,
                      size_t *inout_n_samps,
                      int full_utt);

/**
 * Feed acoustic feature data into the acoustic model for scoring.
 *
 * @param inout_cep In: Pointer to buffer of features
 *                  Out: Pointer to next frame to be read
 * @param inout_n_frames In: Number of frames available
 *                      Out: Number of frames remaining
 * @param full_utt If non-zero, this block represents a full
 *                 utterance and should be processed as such.
 * @return Number of frames of data processed.
 */
int acmod_process_cep(acmod_t *acmod,
                      mfcc_t ***inout_cep,
                      int *inout_n_frames,
                      int full_utt);

/**
 * Feed dynamic feature data into the acoustic model for scoring.
 *
 * Unlike acmod_process_raw() and acmod_process_cep(), this function
 * accepts a single frame at a time.  This is because there is no need
 * to do buffering when using dynamic features as input.  However, if
 * the dynamic feature buffer is full, this function will fail, so you
 * should either always check the return value, or always pair a call
 * to it with a call to acmod_score().
 *
 * @param feat Pointer to one frame of dynamic features.
 * @return Number of frames processed (either 0 or 1).
 */
int acmod_process_feat(acmod_t *acmod,
                       mfcc_t **feat);

/**
 * Get a frame of dynamic feature data.
 *
 * @param inout_frame_idx Input: frame index to get, or NULL
 *                        to obtain features for the most recent frame.
 *                        Output: frame index corresponding to this
 *                        set of features.
 * @return Feature array, or NULL if requested frame is not available.
 */
mfcc_t **acmod_get_frame(acmod_t *acmod, int *inout_frame_idx);

/**
 * Score one frame of data.
 *
 * @param inout_frame_idx Input: frame index to score, or NULL
 *                        to obtain scores for the most recent frame.
 *                        Output: frame index corresponding to this
 *                        set of scores.
 * @return Array of senone scores for this frame, or NULL if no frame
 *         is available for scoring (such as if a frame index is
 *         requested that is not yet or no longer available).  The
 *         data pointed to persists only until the next call to
 *         acmod_score() or acmod_advance().
 */
int16 const *acmod_score(acmod_t *acmod,
                         int *inout_frame_idx);

/**
 * Get best score and senone index for current frame.
 */
int acmod_best_score(acmod_t *acmod, int *out_best_senid);

/**
 * Clear set of active senones.
 */
void acmod_clear_active(acmod_t *acmod);

/**
 * Activate senones associated with an HMM.
 */
void acmod_activate_hmm(acmod_t *acmod, hmm_t *hmm);

/**
 * Activate a single senone.
 */
#define acmod_activate_sen(acmod, sen) bitvec_set((acmod)->senone_active_vec, sen)

/**
 * Build active list.
 */
int32 acmod_flags2list(acmod_t *acmod);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __ACMOD_H__ */

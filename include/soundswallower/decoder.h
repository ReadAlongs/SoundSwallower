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
 * @file decoder.h Decoder API for SoundSwallower
 */

#ifndef __DECODER_H__
#define __DECODER_H__

#include <stdio.h>

#include <soundswallower/configuration.h>
#include <soundswallower/logmath.h>
#include <soundswallower/fe.h>
#include <soundswallower/feat.h>
#include <soundswallower/acmod.h>
#include <soundswallower/dict.h>
#include <soundswallower/dict2pid.h>
#include <soundswallower/alignment.h>
#include <soundswallower/lattice.h>
#include <soundswallower/mllr.h>
#include <soundswallower/fsg_model.h>
#include <soundswallower/profile.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * Speech recognizer object.
 */
typedef struct ps_decoder_s ps_decoder_t;


/**
 * N-best hypothesis iterator object.
 */
typedef struct ps_astar_s ps_nbest_t;

/**
 * Segmentation iterator object.
 */
typedef struct ps_seg_s ps_seg_t;

/**
 * Initialize the decoder from a configuration object.
 *
 * @note The decoder retains ownership of the pointer
 * <code>config</code>, so if you are not going to use it
 * elsewhere, you can free it.
 *
 * @param config a command-line structure, as created by
 * config_parse() or cmd_ln_parse_file_r().  If NULL, the
 * decoder will be allocated but not initialized.  You can
 * proceed to initialize it with ps_reinit().
 */
ps_decoder_t *ps_init(config_t *config);

/**
 * Reinitialize the decoder with updated configuration.
 *
 * This function allows you to switch the acoustic model, dictionary,
 * or other configuration without creating an entirely new decoding
 * object.
 *
 * @note The decoder retains ownership of the pointer
 * <code>config</code>, so you should free it when no longer used.
 *
 * @param ps Decoder.
 * @param config An optional new configuration to use.  If this is
 *               NULL, the previous configuration will be reloaded,
 *               with any changes applied.
 * @return 0 for success, <0 for failure.
 */
int ps_reinit(ps_decoder_t *ps, config_t *config);

/**
 * Reinitialize only the feature extractor with updated configuration.
 *
 * This function allows you to switch the feature extraction
 * parameters without otherwise affecting the decoder configuration.
 * For example, if you change the sample rate or the frame rate and do
 * not need to reconfigure the rest of the decoder.
 *
 * @note The decoder retains ownership of the pointer
 * <code>config</code>, so you should free it when no longer used.
 *
 * @param ps Decoder.
 * @param config An optional new configuration to use.  If this is
 *               NULL, the previous configuration will be reloaded,
 *               with any changes to feature extraction applied.
 * @return pointer to new feature extractor. The decoder owns this
 *         pointer, so you should not attempt to free it manually.
 *         Use fe_retain() if you wish to reuse it elsewhere.
 */
fe_t * ps_reinit_fe(ps_decoder_t *ps, config_t *config);

/**
 * Returns the argument definitions used in ps_init().
 *
 * This is here to avoid exporting global data, which is problematic
 * on Win32 and Symbian (and possibly other platforms).
 */
config_param_t const *ps_args(void);

/**
 * Retain a pointer to the decoder.
 *
 * This increments the reference count on the decoder, allowing it to
 * be shared between multiple parent objects.  In general you will not
 * need to use this function, ever.  It is mainly here for the
 * convenience of scripting language bindings.
 *
 * @return pointer to retained decoder.
 */
ps_decoder_t *ps_retain(ps_decoder_t *ps);

/**
 * Finalize the decoder.
 *
 * This releases all resources associated with the decoder.
 *
 * @param ps Decoder to be freed.
 * @return New reference count (0 if freed).
 */
int ps_free(ps_decoder_t *ps);

/**
 * Get the configuration object for this decoder.
 *
 * @return The configuration object for this decoder.  The decoder
 *         owns this pointer, so you should not attempt to free it
 *         manually.  Use cmd_ln_retain() if you wish to reuse it
 *         elsewhere.
 */
config_t *ps_get_config(ps_decoder_t *ps);

/**
 * Get the log-math computation object for this decoder.
 *
 * @return The log-math object for this decoder.  The decoder owns
 *         this pointer, so you should not attempt to free it
 *         manually.  Use logmath_retain() if you wish to reuse it
 *         elsewhere.
 */
logmath_t *ps_get_logmath(ps_decoder_t *ps);

/**
 * Get the feature extraction object for this decoder.
 *
 * @return The feature extraction object for this decoder.  The
 *         decoder owns this pointer, so you should not attempt to
 *         free it manually.  Use fe_retain() if you wish to reuse it
 *         elsewhere.
 */
fe_t *ps_get_fe(ps_decoder_t *ps);

/**
 * Get the dynamic feature computation object for this decoder.
 *
 * @return The dynamic feature computation object for this decoder.
 *         The decoder owns this pointer, so you should not attempt to
 *         free it manually.  Use feat_retain() if you wish to reuse
 *         it elsewhere.
 */
feat_t *ps_get_feat(ps_decoder_t *ps);

/**
 * Load new finite state grammar.
 *
 * @note The decoder retains ownership of the pointer
 * <code>fsg</code>, so you should free it when no longer used.
 */
int ps_set_fsg(ps_decoder_t *ps, const char *name, fsg_model_t *fsg);

/**
 * Load new finite state grammar from JSGF file.
 */
int ps_set_jsgf_file(ps_decoder_t *ps, const char *name, const char *path);

/**
 * Load new finite state grammar parsing JSGF from string.
 */
int ps_set_jsgf_string(ps_decoder_t *ps, const char *name, const char *jsgf_string);

/**
 * Adapt current acoustic model using a linear transform.
 *
 * @param mllr The new transform to use, or NULL to update the existing
 *              transform.  The decoder retains ownership of this pointer,
 *              so you should free it if you no longer need it.
 * @return The updated transform object for this decoder, or
 *         NULL on failure.
 */
ps_mllr_t *ps_update_mllr(ps_decoder_t *ps, ps_mllr_t *mllr);

/**
 * Add a word to the pronunciation dictionary.
 *
 * @param word Word string to add.
 * @param phones Whitespace-separated list of phoneme strings
 *               describing pronunciation of <code>word</code>.
 * @param update If TRUE, update the search module (whichever one is
 *               currently active) to recognize the newly added word.
 *               If adding multiple words, it is more efficient to
 *               pass FALSE here in all but the last word.
 * @return The internal ID (>= 0) of the newly added word, or <0 on
 *         failure.
 */
int ps_add_word(ps_decoder_t *ps,
                char const *word,
                char const *phones,
                int update);

/** 
 * Lookup for the word in the dictionary and return phone transcription
 * for it.
 *
 * @param ps decoder
 * @param word Word to look for
 *
 * @return Whitespace-spearated phone string describing the pronunciation of the <code>word</code>
 *         or NULL if word is not present in the dictionary. The string is
 *         allocated and must be freed by the user.
 */
char *ps_lookup_word(ps_decoder_t *ps, 
	             const char *word);

/**
 * Start utterance processing.
 *
 * This function should be called before any utterance data is passed
 * to the decoder.  It marks the start of a new utterance and
 * reinitializes internal data structures.
 *
 * @param ps Decoder to be started.
 * @return 0 for success, <0 on error.
 */
int ps_start_utt(ps_decoder_t *ps);

/**
 * Decode raw audio data.
 *
 * @param ps Decoder.
 * @param no_search If non-zero, perform feature extraction but don't
 *                  do any recognition yet.  This may be necessary if
 *                  your processor has trouble doing recognition in
 *                  real-time.
 * @param full_utt If non-zero, this block of data is a full utterance
 *                 worth of data.  This may allow the recognizer to
 *                 produce more accurate results.
 * @return Number of frames of data searched, or <0 for error.
 */
int ps_process_raw(ps_decoder_t *ps,
                   int16 const *data,
                   size_t n_samples,
                   int no_search,
                   int full_utt);

/**
 * Decode acoustic feature data.
 *
 * @param ps Decoder.
 * @param no_search If non-zero, perform feature extraction but don't
 *                  do any recognition yet.  This may be necessary if
 *                  your processor has trouble doing recognition in
 *                  real-time.
 * @param full_utt If non-zero, this block of data is a full utterance
 *                 worth of data.  This may allow the recognizer to
 *                 produce more accurate results.
 * @return Number of frames of data searched, or <0 for error.
 */
int ps_process_cep(ps_decoder_t *ps,
                   mfcc_t **data,
                   int n_frames,
                   int no_search,
                   int full_utt);

/**
 * Get the number of frames of data searched.
 *
 * Note that there is a delay between this and the number of frames of
 * audio which have been input to the system.  This is due to the fact
 * that acoustic features are computed using a sliding window of
 * audio, and dynamic features are computed over a sliding window of
 * acoustic features.
 *
 * @param ps Decoder.
 * @return Number of frames of speech data which have been recognized
 * so far.
 */
int ps_get_n_frames(ps_decoder_t *ps);

/**
 * End utterance processing.
 *
 * @param ps Decoder.
 * @return 0 for success, <0 on error
 */
int ps_end_utt(ps_decoder_t *ps);

/**
 * Get hypothesis string and path score.
 *
 * @param ps Decoder.
 * @param out_best_score Output: path score corresponding to returned string.
 * @return String containing best hypothesis at this point in
 *         decoding.  NULL if no hypothesis is available.  This string is owned
 *         by the decoder, so you should copy it if you need to hold onto it.
 */
char const *ps_get_hyp(ps_decoder_t *ps, int32 *out_best_score);

/**
 * Get posterior probability.
 *
 * @note Unless the -bestpath option is enabled, this function will
 * always return zero (corresponding to a posterior probability of
 * 1.0).  Even if -bestpath is enabled, it will also return zero when
 * called on a partial result.  Ongoing research into effective
 * confidence annotation for partial hypotheses may result in these
 * restrictions being lifted in future versions.
 *
 * @param ps Decoder.
 * @return Posterior probability of the best hypothesis.
 */
int32 ps_get_prob(ps_decoder_t *ps);

/**
 * Get word lattice.
 *
 * There isn't much you can do with this so far, a public API will
 * appear in the future.
 *
 * @param ps Decoder.
 * @return Word lattice object containing all hypotheses so far.  NULL
 *         if no hypotheses are available.  This pointer is owned by
 *         the decoder and you should not attempt to free it manually.
 *         It is only valid until the next utterance, unless you use
 *         ps_lattice_retain() to retain it.
 */
ps_lattice_t *ps_get_lattice(ps_decoder_t *ps);

/**
 * Get an iterator over the word segmentation for the best hypothesis.
 *
 * @param ps Decoder.
 * @return Iterator over the best hypothesis at this point in
 *         decoding.  NULL if no hypothesis is available.
 */
ps_seg_t *ps_seg_iter(ps_decoder_t *ps);

/**
 * Get the next segment in a word segmentation.
 *
 * @param seg Segment iterator.
 * @return Updated iterator with the next segment.  NULL at end of
 *         utterance (the iterator will be freed in this case).
 */
ps_seg_t *ps_seg_next(ps_seg_t *seg);

/**
 * Get word string from a segmentation iterator.
 *
 * @param seg Segment iterator.
 * @return Read-only string giving string name of this segment.  This
 * is only valid until the next call to ps_seg_next().
 */
char const *ps_seg_word(ps_seg_t *seg);

/**
 * Get inclusive start and end frames from a segmentation iterator.
 *
 * @note These frame numbers are inclusive, i.e. the end frame refers
 * to the last frame in which the given word or other segment was
 * active.  Therefore, the actual duration is *out_ef - *out_sf + 1.
 *
 * @param seg Segment iterator.
 * @param out_sf Output: First frame index in segment.
 * @param out_ef Output: Last frame index in segment.
 */
void ps_seg_frames(ps_seg_t *seg, int *out_sf, int *out_ef);

/**
 * Get language, acoustic, and posterior probabilities from a
 * segmentation iterator.
 *
 * @note Unless the -bestpath option is enabled, this function will
 * always return zero (corresponding to a posterior probability of
 * 1.0).  Even if -bestpath is enabled, it will also return zero when
 * called on a partial result.  Ongoing research into effective
 * confidence annotation for partial hypotheses may result in these
 * restrictions being lifted in future versions.
 *
 * @param out_ascr Output: acoustic model score for this segment.
 * @param out_lscr Output: language model score for this segment.
 * @return Log posterior probability of current segment.  Log is
 *         expressed in the log-base used in the decoder.  To convert
 *         to linear floating-point, use logmath_exp(ps_get_logmath(),
 *         pprob).
 */
int32 ps_seg_prob(ps_seg_t *seg, int32 *out_ascr, int32 *out_lscr);

/**
 * Finish iterating over a word segmentation early, freeing resources.
 */
void ps_seg_free(ps_seg_t *seg);

/**
 * Get an iterator over the best hypotheses. The function may also
 * return a NULL which means that there is no hypothesis available for this
 * utterance.
 *
 * @param ps Decoder.
 * @return Iterator over N-best hypotheses or NULL if no hypothesis is available
 */
ps_nbest_t *ps_nbest(ps_decoder_t *ps);

/**
 * Move an N-best list iterator forward.
 *
 * @param nbest N-best iterator.
 * @return Updated N-best iterator, or NULL if no more hypotheses are
 *         available (iterator is freed ni this case).
 */
ps_nbest_t *ps_nbest_next(ps_nbest_t *nbest);

/**
 * Get the hypothesis string from an N-best list iterator.
 *
 * @param nbest N-best iterator.
 * @param out_score Output: Path score for this hypothesis.
 * @return String containing next best hypothesis.
 */
char const *ps_nbest_hyp(ps_nbest_t *nbest, int32 *out_score);

/**
 * Get the word segmentation from an N-best list iterator.
 *
 * @param nbest N-best iterator.
 * @return Iterator over the next best hypothesis.
 */
ps_seg_t *ps_nbest_seg(ps_nbest_t *nbest);

/**
 * Finish N-best search early, releasing resources.
 *
 * @param nbest N-best iterator.
 */
void ps_nbest_free(ps_nbest_t *nbest);

/**
 * Get performance information for the current utterance.
 *
 * @param ps Decoder.
 * @param out_nspeech Output: Number of seconds of speech.
 * @param out_ncpu    Output: Number of seconds of CPU time used.
 * @param out_nwall   Output: Number of seconds of wall time used.
 */
void ps_get_utt_time(ps_decoder_t *ps, double *out_nspeech,
                     double *out_ncpu, double *out_nwall);

/**
 * Get overall performance information.
 *
 * @param ps Decoder.
 * @param out_nspeech Output: Number of seconds of speech.
 * @param out_ncpu    Output: Number of seconds of CPU time used.
 * @param out_nwall   Output: Number of seconds of wall time used.
 */
void ps_get_all_time(ps_decoder_t *ps, double *out_nspeech,
                     double *out_ncpu, double *out_nwall);

/**
 * Set logging to go to a file.
 *
 * @param ps Decoder.
 * @param logfn Filename to log to, or NULL to log to standard output.
 * @return 0 for success or -1 for failure.
 */
int ps_set_logfile(ps_decoder_t *ps, const char *logfn);

/**
 * Search algorithm structure.
 */
typedef struct ps_search_s ps_search_t;


/* Search names*/
#define PS_DEFAULT_SEARCH  "_default"
#define PS_DEFAULT_PL_SEARCH  "_default_pl"

/* Search types */
#define PS_SEARCH_TYPE_FSG    "fsg"
#define PS_SEARCH_TYPE_STATE_ALIGN  "state_align"
#define PS_SEARCH_TYPE_PHONE_LOOP  "phone_loop"

/**
 * V-table for search algorithm.
 */
typedef struct ps_searchfuncs_s {
    int (*start)(ps_search_t *search);
    int (*step)(ps_search_t *search, int frame_idx);
    int (*finish)(ps_search_t *search);
    int (*reinit)(ps_search_t *search, dict_t *dict, dict2pid_t *d2p);
    void (*free)(ps_search_t *search);

    ps_lattice_t *(*lattice)(ps_search_t *search);
    char const *(*hyp)(ps_search_t *search, int32 *out_score);
    int32 (*prob)(ps_search_t *search);
    ps_seg_t *(*seg_iter)(ps_search_t *search);
} ps_searchfuncs_t;

/**
 * Base structure for search module.
 */
struct ps_search_s {
    ps_searchfuncs_t *vt;  /**< V-table of search methods. */
    
    char *type;
    char *name;
    
    config_t *config;      /**< Configuration. */
    acmod_t *acmod;        /**< Acoustic model. */
    dict_t *dict;        /**< Pronunciation dictionary. */
    dict2pid_t *d2p;       /**< Dictionary to senone mappings. */
    char *hyp_str;         /**< Current hypothesis string. */
    ps_lattice_t *dag;	   /**< Current hypothesis word graph. */
    ps_latlink_t *last_link; /**< Final link in best path. */
    int32 post;            /**< Utterance posterior probability. */
    int32 n_words;         /**< Number of words known to search (may
                              be less than in the dictionary) */

    /* Magical word IDs that must exist in the dictionary: */
    int32 start_wid;       /**< Start word ID. */
    int32 silence_wid;     /**< Silence word ID. */
    int32 finish_wid;      /**< Finish word ID. */
};

#define ps_search_base(s) ((ps_search_t *)s)
#define ps_search_config(s) ps_search_base(s)->config
#define ps_search_acmod(s) ps_search_base(s)->acmod
#define ps_search_dict(s) ps_search_base(s)->dict
#define ps_search_dict2pid(s) ps_search_base(s)->d2p
#define ps_search_dag(s) ps_search_base(s)->dag
#define ps_search_last_link(s) ps_search_base(s)->last_link
#define ps_search_post(s) ps_search_base(s)->post
#define ps_search_n_words(s) ps_search_base(s)->n_words

#define ps_search_type(s) ps_search_base(s)->type
#define ps_search_name(s) ps_search_base(s)->name
#define ps_search_start(s) (*(ps_search_base(s)->vt->start))(s)
#define ps_search_step(s,i) (*(ps_search_base(s)->vt->step))(s,i)
#define ps_search_finish(s) (*(ps_search_base(s)->vt->finish))(s)
#define ps_search_reinit(s,d,d2p) (*(ps_search_base(s)->vt->reinit))(s,d,d2p)
#define ps_search_free(s) (*(ps_search_base(s)->vt->free))(s)
#define ps_search_lattice(s) (*(ps_search_base(s)->vt->lattice))(s)
#define ps_search_hyp(s,sc) (*(ps_search_base(s)->vt->hyp))(s,sc)
#define ps_search_prob(s) (*(ps_search_base(s)->vt->prob))(s)
#define ps_search_seg_iter(s) (*(ps_search_base(s)->vt->seg_iter))(s)

/* For convenience... */
#define ps_search_silence_wid(s) ps_search_base(s)->silence_wid
#define ps_search_start_wid(s) ps_search_base(s)->start_wid
#define ps_search_finish_wid(s) ps_search_base(s)->finish_wid

/**
 * Initialize base structure.
 */
void ps_search_init(ps_search_t *search, ps_searchfuncs_t *vt,
		    const char *type, const char *name,
                    config_t *config, acmod_t *acmod, dict_t *dict,
                    dict2pid_t *d2p);


/**
 * Free search
 */
void ps_search_base_free(ps_search_t *search);

/**
 * Re-initialize base structure with new dictionary.
 */
void ps_search_base_reinit(ps_search_t *search, dict_t *dict,
                           dict2pid_t *d2p);

typedef struct ps_segfuncs_s {
    ps_seg_t *(*seg_next)(ps_seg_t *seg);
    void (*seg_free)(ps_seg_t *seg);
} ps_segfuncs_t;

/**
 * Base structure for hypothesis segmentation iterator.
 */
struct ps_seg_s {
    ps_segfuncs_t *vt;     /**< V-table of seg methods */
    ps_search_t *search;   /**< Search object from whence this came */
    char const *word;      /**< Word string (pointer into dictionary hash) */
    frame_idx_t sf;        /**< Start frame. */
    frame_idx_t ef;        /**< End frame. */
    int32 ascr;            /**< Acoustic score. */
    int32 lscr;            /**< Language model score. */
    int32 prob;            /**< Log posterior probability. */
};

#define ps_search_seg_next(seg) (*(seg->vt->seg_next))(seg)
#define ps_search_seg_free(s) (*(seg->vt->seg_free))(seg)

/**
 * Decoder object.
 */
struct ps_decoder_s {
    /* Model parameters and such. */
    config_t *config;  /**< Configuration. */
    int refcount;      /**< Reference count. */

    /* Basic units of computation. */
    fe_t *fe;          /**< Acoustic feature computation. */
    feat_t *fcb;       /**< Dynamic feature computation. */
    acmod_t *acmod;    /**< Acoustic model. */
    dict_t *dict;      /**< Pronunciation dictionary. */
    dict2pid_t *d2p;   /**< Dictionary to senone mapping. */
    logmath_t *lmath;  /**< Log math computation. */
    ps_search_t *search;     /**< Main search object. */

    /* Utterance-processing related stuff. */
    uint32 uttno;       /**< Utterance counter. */
    ptmr_t perf;        /**< Performance counter for all of decoding. */
    uint32 n_frame;     /**< Total number of frames processed. */

#ifndef EMSCRIPTEN
    /* Logging. */
    FILE *logfh;
#endif
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __POCKETSPHINX_H__ */

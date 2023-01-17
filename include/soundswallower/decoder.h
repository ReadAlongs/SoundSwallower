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

#include <soundswallower/acmod.h>
#include <soundswallower/alignment.h>
#include <soundswallower/configuration.h>
#include <soundswallower/dict.h>
#include <soundswallower/dict2pid.h>
#include <soundswallower/fe.h>
#include <soundswallower/feat.h>
#include <soundswallower/fsg_model.h>
#include <soundswallower/lattice.h>
#include <soundswallower/logmath.h>
#include <soundswallower/mllr.h>
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
typedef struct decoder_s decoder_t;

/**
 * N-best hypothesis iterator object.
 */
typedef struct astar_search_s hyp_iter_t;

/**
 * Segmentation iterator object.
 */
typedef struct seg_iter_s seg_iter_t;

/**
 * Create and configure the decoder without initializing it.
 *
 * The decoder will be allocated and configured but not initialized.
 * You can proceed to initialize it with decoder_reinit().  Note that
 * in particular, model parameters will be created, including the
 * default dictionary.  This gives you the chance to remove them
 * (using config_unset()) if you disagree.
 *
 * @note The decoder consumes the pointer <code>config</code>.  If you
 * wish to reuse it, you must call config_retain() on it.
 *
 * @param config a command-line structure, as created by
 * config_init().
 */
decoder_t *decoder_create(config_t *config);

/**
 * Initialize the decoder from a configuration object.
 *
 * @note The decoder consumes the pointer <code>config</code>.  If you
 * wish to reuse it, you must call config_retain() on it.
 *
 * @param config a command-line structure, as created by
 * config_init().
 */
decoder_t *decoder_init(config_t *config);

/**
 * Reinitialize the decoder with updated configuration.
 *
 * This function allows you to switch the acoustic model, dictionary,
 * or other configuration without creating an entirely new decoding
 * object.
 *
 * @note The decoder consumes the pointer <code>config</code>.  If you
 * wish to reuse it, you must call config_retain() on it.
 *
 * @param ps Decoder.
 * @param config An optional new configuration to use.  If this is
 *               NULL, the previous configuration will be reloaded,
 *               with any changes applied.
 * @return 0 for success, <0 for failure.
 */
int decoder_reinit(decoder_t *d, config_t *config);

/**
 * Reinitialize only the feature computation with updated configuration.
 *
 * This function allows you to switch the feature extraction and
 * computatoin parameters without otherwise affecting the decoder
 * configuration.  For example, if you change the sample rate or the
 * frame rate and do not need to reconfigure the rest of the decoder.
 *
 * @note The decoder consumes the pointer <code>config</code>.  If you
 * wish to reuse it, you must call config_retain() on it.
 *
 * @param ps Decoder.
 * @param config An optional new configuration to use.  If this is
 *               NULL, the previous configuration will be reloaded,
 *               with any changes to feature extraction applied.
 * @return 0 for success or -1 for error.
 */
int decoder_reinit_feat(decoder_t *d, config_t *config);

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
decoder_t *decoder_retain(decoder_t *d);

/**
 * Finalize the decoder.
 *
 * This releases all resources associated with the decoder.
 *
 * @param ps Decoder to be freed.
 * @return New reference count (0 if freed).
 */
int decoder_free(decoder_t *d);

/**
 * Access the configuration object for this decoder.
 *
 * @return The configuration object for this decoder.  The decoder
 *         owns this pointer, so you should not attempt to free it
 *         manually.  decoder_retain() if you wish to reuse it
 *         elsewhere.
 */
config_t *decoder_config(decoder_t *d);

/**
 * Access the log-math computation object for this decoder.
 *
 * @return The log-math object for this decoder.  The decoder owns
 *         this pointer, so you should not attempt to free it
 *         manually.  Use logmath_retain() if you wish to reuse it
 *         elsewhere.
 */
logmath_t *decoder_logmath(decoder_t *d);

/**
 * Access the feature extraction object for this decoder.
 *
 * @return The feature extraction object for this decoder.  The
 *         decoder owns this pointer, so you should not attempt to
 *         free it manually.  Use fe_retain() if you wish to reuse it
 *         elsewhere.
 */
fe_t *decoder_fe(decoder_t *d);

/**
 * Access the dynamic feature computation object for this decoder.
 *
 * @return The dynamic feature computation object for this decoder.
 *         The decoder owns this pointer, so you should not attempt to
 *         free it manually.  Use feat_retain() if you wish to reuse
 *         it elsewhere.
 */
feat_t *decoder_feat(decoder_t *d);

/**
 * Get the current cepstral mean as a string.
 *
 * This is the string representation of the current cepstral mean,
 * which represents the acoustic channel conditions in live
 * recognition.  This can be used to initialize the decoder with the
 * `cmninit` option, e.g.:
 *
 *     config = config_parse_json(NULL, "cmninit: 42,-1,0");
 *
 * @memberof decoder_t
 * @param ps Decoder
 * @param update Update the cepstral mean using data processed so far.
 * @return String representation of cepstral mean, as
 *         `config_get_int(config, "ceplen")` comma-separated
 *         numbers.  This pointer is owned by the decoder and only
 *         valid until the next call to decoder_get_cmn(), decoder_set_cmn() or
 *         decoder_end_utt().
 */
const char *decoder_get_cmn(decoder_t *ps, int update);

/**
 * Set the current cepstral mean from a string.
 *
 * This does the same thing as setting `cmninit` with
 * config_set_string() and running `decoder_reinit_feat()` but is more
 * efficient, and can also be done in the middle of an utterance if
 * you like.
 *
 * @memberof decoder_t
 * @param ps Decoder
 * @param cmn String representation of cepstral mean, as up to
 *            `config_get_int(config, "ceplen")` -separated numbers
 *            (any missing values will be zero-filled).  @return 0 for
 *            success of -1 for invalid input.
 */
int decoder_set_cmn(decoder_t *ps, const char *cmn);

/**
 * Load new finite state grammar.
 *
 * @note The decoder consumes the pointer <code>fsg</code>, so you
 * should call fsg_model_retain() on it if you wish to use it
 * elsewhere.
 */
int decoder_set_fsg(decoder_t *d, fsg_model_t *fsg);

/**
 * Load new finite state grammar from JSGF file.
 */
int decoder_set_jsgf_file(decoder_t *d, const char *path);

/**
 * Load new finite state grammar parsing JSGF from string.
 */
int decoder_set_jsgf_string(decoder_t *d, const char *jsgf_string);

/**
 * Set a word sequence for force-alignment.
 *
 * Decoding proceeds as normal, though only this word sequence will be
 * recognized, with silences and alternate pronunciations inserted.
 * Word alignments are available with decoder_seg_iter().  To obtain
 * phoneme or state segmentations, use decoder_alignment().
 *
 * @memberof ps_decoder_t
 * @param ps Decoder
 * @param words String containing whitespace-separated words for alignment.
 *              These words are assumed to exist in the current dictionary.
 */
int decoder_set_align_text(decoder_t *d, const char *text);

/**
 * Adapt current acoustic model using a linear transform.
 *
 * @param mllr The new transform to use, or NULL to update the
 *              existing transform.  The decoder consumes this
 *              pointer, so you should call mllr_retain() on it if
 *              you wish to reuse it elsewhere
 * @return The updated transform object for this decoder, or
 *         NULL on failure.
 */
mllr_t *decoder_apply_mllr(decoder_t *d, mllr_t *mllr);

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
int decoder_add_word(decoder_t *d,
                     const char *word,
                     const char *phones,
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
char *decoder_lookup_word(decoder_t *d,
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
int decoder_start_utt(decoder_t *d);

/**
 * Decode integer audio data.
 *
 * @param ps Decoder.
 * @param no_search If non-zero, perform feature extraction but don't
 *                  do any recognition yet (features will be buffered).
 * @param full_utt If non-zero, this block of data is a full utterance
 *                 worth of data.  This may allow the recognizer to
 *                 produce more accurate results.
 * @return Number of frames of data searched, or <0 for error.
 */
int decoder_process_int16(decoder_t *d,
                          int16 *data,
                          size_t n_samples,
                          int no_search,
                          int full_utt);

/**
 * Decode floating-point audio data.
 *
 * @param ps Decoder.
 * @param no_search If non-zero, perform feature extraction but don't
 *                  do any recognition yet (features will be buffered).
 * @param full_utt If non-zero, this block of data is a full utterance
 *                 worth of data.  This may allow the recognizer to
 *                 produce more accurate results.
 * @return Number of frames of data searched, or <0 for error.
 */
int
decoder_process_float32(decoder_t *d,
                        float32 *data,
                        size_t n_samples,
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
int decoder_n_frames(decoder_t *d);

/**
 * End utterance processing.
 *
 * @param ps Decoder.
 * @return 0 for success, <0 on error
 */
int decoder_end_utt(decoder_t *d);

/**
 * Get hypothesis string and path score.
 *
 * @param ps Decoder.
 * @param out_best_score Output: path score corresponding to returned string.
 * @return String containing best hypothesis at this point in
 *         decoding.  NULL if no hypothesis is available.  This string is owned
 *         by the decoder, so you should copy it if you need to hold onto it.
 */
const char *decoder_hyp(decoder_t *d, int32 *out_best_score);

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
int32 decoder_prob(decoder_t *d);

/**
 * Get word lattice.
 *
 * @param ps Decoder.
 * @return Word lattice object containing all hypotheses so far.  NULL
 *         if no hypotheses are available.  This pointer is owned by
 *         the decoder and you should not attempt to free it manually.
 *         It is only valid until the next utterance, unless you use
 *         lattice_retain() to retain it.
 */
lattice_t *decoder_lattice(decoder_t *d);

/**
 * Get an iterator over the word segmentation for the best hypothesis.
 *
 * @param ps Decoder.
 * @return Iterator over the best hypothesis at this point in
 *         decoding.  NULL if no hypothesis is available.
 */
seg_iter_t *decoder_seg_iter(decoder_t *d);

/**
 * Get the next segment in a word segmentation.
 *
 * @param seg Segment iterator.
 * @return Updated iterator with the next segment.  NULL at end of
 *         utterance (the iterator will be freed in this case).
 */
seg_iter_t *seg_iter_next(seg_iter_t *seg);

/**
 * Get word string from a segmentation iterator.
 *
 * @param seg Segment iterator.
 * @return Read-only string giving string name of this segment.  This
 * is only valid until the next call to seg_iter_next().
 */
const char *seg_iter_word(seg_iter_t *seg);

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
void seg_iter_frames(seg_iter_t *seg, int *out_sf, int *out_ef);

/**
 * Get language, acoustic, and posterior probabilities from a
 * segmentation iterator.
 *
 * @param out_ascr Output: acoustic model score for this segment.
 * @param out_lscr Output: language model score for this segment.
 * @return Log posterior probability of current segment.  Log is
 *         expressed in the log-base used in the decoder.  To convert
 *         to linear floating-point, use logmath_exp(decoder_logmath(),
 *         pprob).
 */
int32 seg_iter_prob(seg_iter_t *seg, int32 *out_ascr, int32 *out_lscr);

/**
 * Finish iterating over a word segmentation early, freeing resources.
 */
void seg_iter_free(seg_iter_t *seg);

/**
 * Get an iterator over the best hypotheses. The function may also
 * return a NULL which means that there is no hypothesis available for this
 * utterance.
 *
 * @param ps Decoder.
 * @return Iterator over N-best hypotheses or NULL if no hypothesis is available
 */
hyp_iter_t *decoder_nbest(decoder_t *d);

/**
 * Move an N-best list iterator forward.
 *
 * @param nbest N-best iterator.
 * @return Updated N-best iterator, or NULL if no more hypotheses are
 *         available (iterator is freed ni this case).
 */
hyp_iter_t *hyp_iter_next(hyp_iter_t *nbest);

/**
 * Get the hypothesis string from an N-best list iterator.
 *
 * @param nbest N-best iterator.
 * @param out_score Output: Path score for this hypothesis.
 * @return String containing next best hypothesis.
 */
const char *hyp_iter_hyp(hyp_iter_t *nbest, int32 *out_score);

/**
 * Get the word segmentation from an N-best list iterator.
 *
 * @param nbest N-best iterator.
 * @return Iterator over the next best hypothesis.
 */
seg_iter_t *hyp_iter_seg(hyp_iter_t *nbest);

/**
 * Finish N-best search early, releasing resources.
 *
 * @param nbest N-best iterator.
 */
void hyp_iter_free(hyp_iter_t *nbest);

/**
 * Get the phone and state-level alignment for the current utterance.
 *
 * @note The returned alignment is owned by the decoder and is only
 * valid until the next call to decoder_alignment() or
 * decoder_reinit().  If you wish to keep it, call alignment_retain()
 * on it.
 */
alignment_t *decoder_alignment(decoder_t *d);

/**
 * Get the decoding result as a null-terminated JSON line.
 *
 * The returned JSON is guaranteed to end with a newline, so
 * you can safely concatenate it with other results to produce
 * line-oriented JSON.
 *
 * @note The returned string is owned by the decoder and is only valid
 * until the next call to decoder_result_json(), decoder_start_utt(),
 * or decoder_reinit().  If you wish to keep it, copy it with strdup().
 *
 * @param start Start time to add to reported segment times.
 * @param align_level 1 to output phone alignments, 2 to output phone
 *                    and state alignments.
 */
const char *decoder_result_json(decoder_t *decoder, double start,
                                int align_level);

/**
 * Get performance information for the current utterance.
 *
 * @param ps Decoder.
 * @param out_nspeech Output: Number of seconds of speech.
 * @param out_ncpu    Output: Number of seconds of CPU time used.
 * @param out_nwall   Output: Number of seconds of wall time used.
 */
void decoder_utt_time(decoder_t *d, double *out_nspeech,
                      double *out_ncpu, double *out_nwall);

/**
 * Get overall performance information.
 *
 * @param ps Decoder.
 * @param out_nspeech Output: Number of seconds of speech.
 * @param out_ncpu    Output: Number of seconds of CPU time used.
 * @param out_nwall   Output: Number of seconds of wall time used.
 */
void decoder_all_time(decoder_t *d, double *out_nspeech,
                      double *out_ncpu, double *out_nwall);

/**
 * Set logging to go to a file.
 *
 * @param ps Decoder.
 * @param logfn Filename to log to, or NULL to log to standard output.
 * @return 0 for success or -1 for failure.
 */
int decoder_set_logfile(decoder_t *d, const char *logfn);

/**
 * Search algorithm structure.
 */
typedef struct search_module_s search_module_t;

/**
 * Decoder object.
 */
struct decoder_s {
    /* Model parameters and such. */
    config_t *config; /**< Configuration. */
    int refcount; /**< Reference count. */

    /* Basic units of computation. */
    fe_t *fe; /**< Acoustic feature computation. */
    feat_t *fcb; /**< Dynamic feature computation. */
    acmod_t *acmod; /**< Acoustic model. */
    dict_t *dict; /**< Pronunciation dictionary. */
    dict2pid_t *d2p; /**< Dictionary to senone mapping. */
    logmath_t *lmath; /**< Log math computation. */
    search_module_t *search; /**< Main search module. */
    search_module_t *align; /**< State alignment module. */
    char *json_result; /**< Decoding result as JSON. */

    /* Utterance-processing related stuff. */
    uint32 uttno; /**< Utterance counter. */
    ptmr_t perf; /**< Performance counter for all of decoding. */
    uint32 n_frame; /**< Total number of frames processed. */

#ifndef EMSCRIPTEN
    /* Logging. */
    FILE *logfh;
#endif
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __DECODER_H__ */

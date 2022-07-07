/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1996-2004 Carnegie Mellon University.  All rights 
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

/*
 * fe.h
 * 
 * $Log: fe.h,v $
 * Revision 1.11  2005/02/05 02:15:02  egouvea
 * Removed fe_process(), never used
 *
 * Revision 1.10  2004/12/10 16:48:55  rkm
 * Added continuous density acoustic model handling
 *
 *
 */

#if defined(_WIN32) && !defined(GNUWINCE)
#define srand48(x) srand(x)
#define lrand48() rand()
#endif

#ifndef _NEW_FE_H_
#define _NEW_FE_H_

#include <soundswallower/cmd_ln.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

#if WORDS_BIGENDIAN
#define NATIVE_ENDIAN "big"
#else
#define NATIVE_ENDIAN "little"
#endif

/** Default number of samples per second. */
#ifdef __EMSCRIPTEN__
#define DEFAULT_SAMPLING_RATE 44100
#else
#define DEFAULT_SAMPLING_RATE 16000
#endif
/** Default number of frames per second. */
#define DEFAULT_FRAME_RATE 100
/** Default spacing between frame starts (equal to
 * DEFAULT_SAMPLING_RATE/DEFAULT_FRAME_RATE) */
#define DEFAULT_FRAME_SHIFT 160
/** Default size of each frame (410 samples @ 16000Hz). */
#define DEFAULT_WINDOW_LENGTH 0.025625 
/** Default number of FFT points. */
#ifdef __EMSCRIPTEN__
#define DEFAULT_FFT_SIZE 2048
#else
#define DEFAULT_FFT_SIZE 512
#endif
/** Default number of MFCC coefficients in output. */
#define DEFAULT_NUM_CEPSTRA 13
/** Default number of filter bands used to generate MFCCs. */
#define DEFAULT_NUM_FILTERS 40
/** Default lower edge of mel filter bank. */
#define DEFAULT_LOWER_FILT_FREQ 133.33334
/** Default upper edge of mel filter bank. */
#define DEFAULT_UPPER_FILT_FREQ 6855.4976
/** Default pre-emphasis filter coefficient. */
#define DEFAULT_PRE_EMPHASIS_ALPHA 0.97
/** Default type of frequency warping to use for VTLN. */
#define DEFAULT_WARP_TYPE "inverse_linear"
/** Default random number seed to use for dithering. */
#define SEED  -1

#define waveform_to_cepstral_command_line_macro() \
  { "-logspec", \
    ARG_BOOLEAN, \
    "no", \
    "Write out logspectral files instead of cepstra" }, \
   \
  { "-smoothspec", \
    ARG_BOOLEAN, \
    "no", \
    "Write out cepstral-smoothed logspectral files" }, \
   \
  { "-transform", \
    ARG_STRING, \
    "legacy", \
    "Which type of transform to use to calculate cepstra (legacy, dct, or htk)" }, \
   \
  { "-alpha", \
    ARG_FLOATING, \
    ARG_STRINGIFY(DEFAULT_PRE_EMPHASIS_ALPHA), \
    "Preemphasis parameter" }, \
   \
  { "-samprate", \
    ARG_FLOATING, \
    ARG_STRINGIFY(DEFAULT_SAMPLING_RATE), \
    "Sampling rate" }, \
   \
  { "-frate", \
    ARG_INTEGER, \
    ARG_STRINGIFY(DEFAULT_FRAME_RATE), \
    "Frame rate" }, \
   \
  { "-wlen", \
    ARG_FLOATING, \
    ARG_STRINGIFY(DEFAULT_WINDOW_LENGTH), \
    "Hamming window length" }, \
   \
  { "-nfft", \
    ARG_INTEGER, \
    "0", \
    "Size of FFT, or 0 to set automatically (recommended)" }, \
   \
  { "-nfilt", \
    ARG_INTEGER, \
    ARG_STRINGIFY(DEFAULT_NUM_FILTERS), \
    "Number of filter banks" }, \
   \
  { "-lowerf", \
    ARG_FLOATING, \
    ARG_STRINGIFY(DEFAULT_LOWER_FILT_FREQ), \
    "Lower edge of filters" }, \
   \
  { "-upperf", \
    ARG_FLOATING, \
    ARG_STRINGIFY(DEFAULT_UPPER_FILT_FREQ), \
    "Upper edge of filters" }, \
   \
  { "-unit_area", \
    ARG_BOOLEAN, \
    "yes", \
    "Normalize mel filters to unit area" }, \
   \
  { "-round_filters", \
    ARG_BOOLEAN, \
    "yes", \
    "Round mel filter frequencies to DFT points" }, \
   \
  { "-ncep", \
    ARG_INTEGER, \
    ARG_STRINGIFY(DEFAULT_NUM_CEPSTRA), \
    "Number of cep coefficients" }, \
   \
  { "-doublebw", \
    ARG_BOOLEAN, \
    "no", \
    "Use double bandwidth filters (same center freq)" }, \
   \
  { "-lifter", \
    ARG_INTEGER, \
    "0", \
    "Length of sin-curve for liftering, or 0 for no liftering." }, \
   \
  { "-input_endian", \
    ARG_STRING, \
    NATIVE_ENDIAN, \
    "Endianness of input data, big or little, ignored if NIST or MS Wav" }, \
   \
  { "-warp_type", \
    ARG_STRING, \
    DEFAULT_WARP_TYPE, \
    "Warping function type (or shape)" }, \
   \
  { "-warp_params", \
    ARG_STRING, \
    NULL, \
    "Parameters defining the warping function" }, \
   \
  { "-dither", \
    ARG_BOOLEAN, \
    "no", \
    "Add 1/2-bit noise" }, \
   \
  { "-seed", \
    ARG_INTEGER, \
    ARG_STRINGIFY(SEED), \
    "Seed for random number generator; if less than zero, pick our own" }, \
   \
  { "-remove_dc", \
    ARG_BOOLEAN, \
    "no", \
    "Remove DC offset from each frame" }, \
  { "-remove_noise", \
    ARG_BOOLEAN,                          \
    "no",                                        \
    "Remove noise using spectral subtraction" }, \
  { "-verbose",                                 \
          ARG_BOOLEAN,                          \
          "no",                                 \
          "Show input filenames" }

  
/** MFCC computation type. */
typedef float32 mfcc_t;
/** Convert a floating-point value to mfcc_t. */
#define FLOAT2MFCC(x) ((mfcc_t)(x))
/** Convert a mfcc_t value to floating-point. */
#define MFCC2FLOAT(x) (x)
/** Multiply two mfcc_t values. */
#define MFCCMUL(a,b) ((a)*(b))
#define MFCCLN(x,in,out) log(x)

/**
 * Structure for the front-end computation.
 */
typedef struct fe_s fe_t;

/**
 * Error codes returned by stuff.
 */
enum fe_error_e {
	FE_SUCCESS = 0,
	FE_OUTPUT_FILE_SUCCESS  = 0,
	FE_CONTROL_FILE_ERROR = -1,
	FE_START_ERROR = -2,
	FE_UNKNOWN_SINGLE_OR_BATCH = -3,
	FE_INPUT_FILE_OPEN_ERROR = -4,
	FE_INPUT_FILE_READ_ERROR = -5,
	FE_MEM_ALLOC_ERROR = -6,
	FE_OUTPUT_FILE_WRITE_ERROR = -7,
	FE_OUTPUT_FILE_OPEN_ERROR = -8,
	FE_ZERO_ENERGY_ERROR = -9,
	FE_INVALID_PARAM_ERROR =  -10
};

/**
 * Initialize a front-end object from a command-line parse.
 *
 * @param config Command-line object, as returned by cmd_ln_parse_r()
 *               or cmd_ln_parse_file().  Ownership is retained by the
 *               fe_t, so you may free this if you no longer need it.
 * @return Newly created front-end object.
 */
fe_t *fe_init(cmd_ln_t *config);

/**
 * Retain ownership of a front end object.
 *
 * @return pointer to the retained front end.
 */
fe_t *fe_retain(fe_t *fe);

/**
 * Free the front end. 
 *
 * Releases resources associated with the front-end object.
 *
 * @return new reference count (0 if freed completely)
 */
int fe_free(fe_t *fe);

/**
 * Retrieve the command-line object used to initialize this front-end.
 *
 * @return command-line object for this front-end.  This pointer is
 *         owned by the fe_t, so you should not attempt to free it
 *         manually.
 */
cmd_ln_t *fe_get_config(fe_t *fe);

/**
 * Get the dimensionality of the output of this front-end object.
 *
 * This is guaranteed to be the number of values in one frame of
 * output from fe_end(), fe_process_frame(), and
 * fe_process_frames().  It is usually the number of MFCC
 * coefficients, but it might be the number of log-spectrum bins, if
 * the <tt>-logspec</tt> or <tt>-smoothspec</tt> options to
 * fe_init_auto_r() were true.
 *
 * @param fe Front-end object
 * @return Dimensionality of front-end output.
 */
int fe_get_output_size(fe_t *fe);

/**
 * Get the dimensionality of the input to this front-end object.
 *
 * This function retrieves the number of input samples consumed by one
 * frame of processing.  To obtain one frame of output, you must have
 * at least <code>*out_frame_size</code> samples.  To obtain <i>N</i>
 * frames of output, you must have at least <code>(N-1) *
 * *out_frame_shift + *out_frame_size</code> input samples.
 *
 * @param fe Front-end object
 * @param out_frame_shift Output: Number of samples between each frame start.
 * @param out_frame_size Output: Number of samples in each frame.
 */
void fe_get_input_size(fe_t *fe, int *out_frame_shift,
                       int *out_frame_size);

/**
 * Start processing data.
 * @return 0 for success, <0 for error (see enum fe_error_e)
 */
int fe_start(fe_t *fe);

/** 
 * Process a block of samples.
 *
 * This function generates up to <code>nframes</code> frames of
 * features, or as many as can be generated from
 * <code>*inout_nsamps</code> samples.
 *
 * On exit, the <code>inout_spch</code> and <code>inout_nsamps</code>,
 * parameters are updated to point to the remaining sample data and
 * the number of remaining samples.  This allows you to call this
 * repeatedly to process a large block of audio in small (say,
 * 5-frame) chunks:
 *
 *     int16 *bigbuf, *p;
 *     mfcc_t **mfcc;
 *     size_t nsamps;
 *     int nfr;
 *
 *     mfcc = (mfcc_t **)
 *         ckd_calloc_2d(5, fe_get_output_size(fe),
 *                       sizeof(**mfcc));
 *     p = bigbuf;
 *     fe_start(fe);
 *     while (nsamps) {
 *         nfr = fe_process(fe, &p, &nsamps, mfcc, 5);
 *         if (nfr > 0)
 *             do_some_stuff(mfcc, nfr);
 *     }
 *     nfr = fe_end(fe, mfcc, 5);
 *     if (nfr > 0)
 *         do_some_stuff(mfcc, nfr);
 *
 * @param inout_spch Input: Pointer to pointer to speech samples
 *                   (signed 16-bit linear PCM).
 *                   Output: Pointer to remaining samples.
 * @param inout_nsamps Input: Pointer to maximum number of samples to
 *                     process.
 *                     Output: Number of samples remaining in input buffer.
 * @param buf_cep Two-dimensional buffer (allocated with
 *                ckd_calloc_2d()) which will receive frames of output
 *                data.  If NULL, no actual processing will be done,
 *                and the maximum number of output frames which would
 *                be generated (including the trailing frame from
 *                fe_end()) is returned.
 * @param nframes Maximum number of frames to generate.
 * @return number of frames written, or the if buf_cep is NULL, the
 * number of frames that could be generated from *inout_nsamps, including
 * the trailing frame written by fe_end().
 */
int fe_process_int16(fe_t *fe,
               int16 const **inout_spch,
               size_t *inout_nsamps,
               mfcc_t **buf_cep,
               int nframes);

/** 
 * Process a block of floating-point samples.
 *
 * See fe_process_int16(), except that the input is expected to be
 * 32-bit floating point in the range of [-1.0, 1.0].
 */
int fe_process_float32(fe_t *fe,
                       float32 const **inout_spch,
                       size_t *inout_nsamps,
                       mfcc_t **buf_cep,
                       int nframes);

/**
 * Finish processing.
 *
 * This function collects any remaining samples and calculates a final
 * cepstral vector, if present.  If there are overflow samples
 * remaining, it will pad with zeros to make a complete frame.
 *
 * @param fe Front-end object.
 * @param buf_cep Cepstral buffer as passed to fe_process().
 * @param nframes Number of frames available in buf_cep.
 * @return number of frames written (always 0 or 1), <0 for error (see
 *         enum fe_error_e)
 */
int fe_end(fe_t *fe, mfcc_t **buf_cep, int nframes);


/**
 * Process one frame of log spectra into MFCC using discrete cosine
 * transform.
 *
 * This uses a variant of the DCT-II where the first frequency bin is
 * scaled by 0.5.  Unless somebody misunderstood the DCT-III equations
 * and thought that's what they were implementing here, this is
 * ostensibly done to account for the symmetry properties of the
 * DCT-II versus the DFT - the first coefficient of the input is
 * assumed to be repeated in the negative frequencies, which is not
 * the case for the DFT.  (This begs the question, why not just use
 * the DCT-I, since it has the appropriate symmetry properties...)
 * Moreover, this is bogus since the mel-frequency bins on which we
 * are doing the DCT don't extend to the edge of the DFT anyway.
 *
 * This also means that the matrix used in computing this DCT can not
 * be made orthogonal, and thus inverting the transform is difficult.
 * Therefore if you want to do cepstral smoothing or have some other
 * reason to invert your MFCCs, use fe_logspec_dct2() and its inverse
 * fe_logspec_dct3() instead.
 *
 * Also, it normalizes by 1/nfilt rather than 2/nfilt, for some reason.
 **/
int fe_logspec_to_mfcc(fe_t *fe,  /**< A fe structure */
                       const mfcc_t *fr_spec, /**< One frame of spectrum */
                       mfcc_t *fr_cep /**< One frame of cepstrum */
        );

/**
 * Convert log spectra to MFCC using DCT-II.
 *
 * This uses the "unitary" form of the DCT-II, i.e. with a scaling
 * factor of sqrt(2/N) and a "beta" factor of sqrt(1/2) applied to the
 * cos(0) basis vector (i.e. the one corresponding to the DC
 * coefficient in the output).
 **/
int fe_logspec_dct2(fe_t *fe,  /**< A fe structure */
                    const mfcc_t *fr_spec, /**< One frame of spectrum */
                    mfcc_t *fr_cep /**< One frame of cepstrum */
        );

/**
 * Convert MFCC to log spectra using DCT-III.
 *
 * This uses the "unitary" form of the DCT-III, i.e. with a scaling
 * factor of sqrt(2/N) and a "beta" factor of sqrt(1/2) applied to the
 * cos(0) basis vector (i.e. the one corresponding to the DC
 * coefficient in the input).
 **/
int fe_mfcc_dct3(fe_t *fe,  /**< A fe structure */
                 const mfcc_t *fr_cep, /**< One frame of cepstrum */
                 mfcc_t *fr_spec /**< One frame of spectrum */
        );

#ifdef __cplusplus
}
#endif


#endif

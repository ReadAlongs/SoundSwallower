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

#include "config.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _MSC_VER
#pragma warning (disable: 4244)
#endif

/**
 * Windows math.h does not contain M_PI
 */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <soundswallower/prim_type.h>
#include <soundswallower/ckd_alloc.h>
#include <soundswallower/byteorder.h>
#include <soundswallower/fe.h>
#include <soundswallower/genrand.h>
#include <soundswallower/err.h>
#include <soundswallower/fe_internal.h>
#include <soundswallower/fe_warp.h>

/* Use extra precision for cosines, Hamming window, pre-emphasis
 * coefficient, twiddle factors. */
#define FLOAT2COS(x) (x)
#define COSMUL(x,y) ((x)*(y))

static float32
fe_mel(melfb_t * mel, float32 x)
{
    float32 warped = fe_warp_unwarped_to_warped(mel, x);

    return (float32) (2595.0 * log10(1.0 + warped / 700.0));
}

static float32
fe_melinv(melfb_t * mel, float32 x)
{
    float32 warped = (float32) (700.0 * (pow(10.0, x / 2595.0) - 1.0));
    return fe_warp_warped_to_unwarped(mel, warped);
}

int32
fe_build_melfilters(melfb_t * mel_fb)
{
    float32 melmin, melmax, melbw, fftfreq;
    int n_coeffs, i, j;


    /* Filter coefficient matrix, in flattened form. */
    mel_fb->spec_start =
        ckd_calloc(mel_fb->num_filters, sizeof(*mel_fb->spec_start));
    mel_fb->filt_start =
        ckd_calloc(mel_fb->num_filters, sizeof(*mel_fb->filt_start));
    mel_fb->filt_width =
        ckd_calloc(mel_fb->num_filters, sizeof(*mel_fb->filt_width));

    /* First calculate the widths of each filter. */
    /* Minimum and maximum frequencies in mel scale. */
    melmin = fe_mel(mel_fb, mel_fb->lower_filt_freq);
    melmax = fe_mel(mel_fb, mel_fb->upper_filt_freq);

    /* Width of filters in mel scale */
    melbw = (melmax - melmin) / (mel_fb->num_filters + 1);
    if (mel_fb->doublewide) {
        melmin -= melbw;
        melmax += melbw;
        if ((fe_melinv(mel_fb, melmin) < 0) ||
            (fe_melinv(mel_fb, melmax) > mel_fb->sampling_rate / 2)) {
            E_WARN
                ("Out of Range: low  filter edge = %f (%f)\n",
                 fe_melinv(mel_fb, melmin), 0.0);
            E_WARN
                ("              high filter edge = %f (%f)\n",
                 fe_melinv(mel_fb, melmax), mel_fb->sampling_rate / 2);
            return FE_INVALID_PARAM_ERROR;
        }
    }

    /* DFT point spacing */
    fftfreq = mel_fb->sampling_rate / (float32) mel_fb->fft_size;

    /* Count and place filter coefficients. */
    n_coeffs = 0;
    for (i = 0; i < mel_fb->num_filters; ++i) {
        float32 freqs[3];

        /* Left, center, right frequencies in Hertz */
        for (j = 0; j < 3; ++j) {
            if (mel_fb->doublewide)
                freqs[j] = fe_melinv(mel_fb, (i + j * 2) * melbw + melmin);
            else
                freqs[j] = fe_melinv(mel_fb, (i + j) * melbw + melmin);
            /* Round them to DFT points if requested */
            if (mel_fb->round_filters)
                freqs[j] = ((int) (freqs[j] / fftfreq + 0.5)) * fftfreq;
        }

        /* spec_start is the start of this filter in the power spectrum. */
        mel_fb->spec_start[i] = -1;
        /* There must be a better way... */
        for (j = 0; j < mel_fb->fft_size / 2 + 1; ++j) {
            float32 hz = j * fftfreq;
            if (hz < freqs[0])
                continue;
            else if (hz > freqs[2] || j == mel_fb->fft_size / 2) {
                /* filt_width is the width in DFT points of this filter. */
                mel_fb->filt_width[i] = j - mel_fb->spec_start[i];
                /* filt_start is the start of this filter in the filt_coeffs array. */
                mel_fb->filt_start[i] = n_coeffs;
                n_coeffs += mel_fb->filt_width[i];
                break;
            }
            if (mel_fb->spec_start[i] == -1)
                mel_fb->spec_start[i] = j;
        }
    }

    /* Now go back and allocate the coefficient array. */
    mel_fb->filt_coeffs =
        ckd_malloc(n_coeffs * sizeof(*mel_fb->filt_coeffs));

    /* And now generate the coefficients. */
    n_coeffs = 0;
    for (i = 0; i < mel_fb->num_filters; ++i) {
        float32 freqs[3];

        /* Left, center, right frequencies in Hertz */
        for (j = 0; j < 3; ++j) {
            if (mel_fb->doublewide)
                freqs[j] = fe_melinv(mel_fb, (i + j * 2) * melbw + melmin);
            else
                freqs[j] = fe_melinv(mel_fb, (i + j) * melbw + melmin);
            /* Round them to DFT points if requested */
            if (mel_fb->round_filters)
                freqs[j] = ((int) (freqs[j] / fftfreq + 0.5)) * fftfreq;
        }

        for (j = 0; j < mel_fb->filt_width[i]; ++j) {
            float32 hz, loslope, hislope;

            hz = (mel_fb->spec_start[i] + j) * fftfreq;
            if (hz < freqs[0] || hz > freqs[2]) {
                E_FATAL
                    ("Failed to create filterbank, frequency range does not match. "
                     "Sample rate %f, FFT size %d, lowerf %f < freq %f > upperf %f.\n",
                     mel_fb->sampling_rate, mel_fb->fft_size, freqs[0], hz,
                     freqs[2]);
            }
            loslope = (hz - freqs[0]) / (freqs[1] - freqs[0]);
            hislope = (freqs[2] - hz) / (freqs[2] - freqs[1]);
            if (mel_fb->unit_area) {
                loslope *= 2 / (freqs[2] - freqs[0]);
                hislope *= 2 / (freqs[2] - freqs[0]);
            }
            if (loslope < hislope) {
                mel_fb->filt_coeffs[n_coeffs] = loslope;
            }
            else {
                mel_fb->filt_coeffs[n_coeffs] = hislope;
            }
            ++n_coeffs;
        }
    }

    return FE_SUCCESS;
}

int32
fe_compute_melcosine(melfb_t * mel_fb)
{

    float64 freqstep;
    int32 i, j;

    mel_fb->mel_cosine =
        (mfcc_t **) ckd_calloc_2d(mel_fb->num_cepstra,
                                  mel_fb->num_filters, sizeof(mfcc_t));

    freqstep = M_PI / mel_fb->num_filters;
    /* NOTE: The first row vector is actually unnecessary but we leave
     * it in to avoid confusion. */
    for (i = 0; i < mel_fb->num_cepstra; i++) {
        for (j = 0; j < mel_fb->num_filters; j++) {
            float64 cosine;

            cosine = cos(freqstep * i * (j + 0.5));
            mel_fb->mel_cosine[i][j] = FLOAT2COS(cosine);
        }
    }

    /* Also precompute normalization constants for unitary DCT. */
    mel_fb->sqrt_inv_n = FLOAT2COS(sqrt(1.0 / mel_fb->num_filters));
    mel_fb->sqrt_inv_2n = FLOAT2COS(sqrt(2.0 / mel_fb->num_filters));

    /* And liftering weights */
    if (mel_fb->lifter_val) {
        mel_fb->lifter =
            calloc(mel_fb->num_cepstra, sizeof(*mel_fb->lifter));
        for (i = 0; i < mel_fb->num_cepstra; ++i) {
            mel_fb->lifter[i] = FLOAT2MFCC(1 + mel_fb->lifter_val / 2
                                           * sin(i * M_PI /
                                                 mel_fb->lifter_val));
        }
    }

    return (0);
}

static void
fe_pre_emphasis_int16(int16 const *in, frame_t * out, int32 len,
                      float32 factor, int16 prior)
{
    int i;

    out[0] = (frame_t) in[0] - (frame_t) prior *factor;
    for (i = 1; i < len; i++)
        out[i] = (frame_t) in[i] - (frame_t) in[i - 1] * factor;
}

static void
fe_copy_to_frame_int16(int16 const *in, frame_t * out, int32 len)
{
    int i;

    for (i = 0; i < len; i++)
        out[i] = (frame_t) in[i];
}

static void
fe_pre_emphasis_float32(float32 const *in, frame_t * out, int32 len,
                        float32 factor, float32 prior)
{
    int i;

    out[0] = (frame_t) in[0] - (frame_t) prior *factor;
    for (i = 1; i < len; i++)
        out[i] = (frame_t) in[i] - (frame_t) in[i - 1] * factor;
}

static void
fe_copy_to_frame_float32(float32 const *in, frame_t * out, int32 len)
{
    int i;

    for (i = 0; i < len; i++)
        out[i] = (frame_t) in[i];
}

void
fe_create_hamming(window_t * in, int32 in_len)
{
    int i;

    /* Symmetric, so we only create the first half of it. */
    for (i = 0; i < in_len / 2; i++) {
        float64 hamm;
        hamm = (0.54 - 0.46 * cos(2 * M_PI * i /
                                  ((float64) in_len - 1.0)));
        in[i] = FLOAT2COS(hamm);
    }
}

static void
fe_hamming_window(frame_t * in, window_t * window, int32 in_len,
                  int32 remove_dc)
{
    int i;

    if (remove_dc) {
        frame_t mean = 0;

        for (i = 0; i < in_len; i++)
            mean += in[i];
        mean /= in_len;
        for (i = 0; i < in_len; i++)
            in[i] -= (frame_t) mean;
    }

    for (i = 0; i < in_len / 2; i++) {
        in[i] = COSMUL(in[i], window[i]);
        in[in_len - 1 - i] = COSMUL(in[in_len - 1 - i], window[i]);
    }
}

static int
fe_spch_to_frame(fe_t * fe, int len)
{
    /* Copy to the frame buffer. */
    if (fe->is_float32) {
        if (fe->pre_emphasis_alpha != 0.0) {
            fe_pre_emphasis_float32(fe->spch._float32, fe->frame, len,
                                    fe->pre_emphasis_alpha,
                                    fe->pre_emphasis_prior._float32);
            if (len >= fe->frame_shift)
                fe->pre_emphasis_prior._float32 = fe->spch._float32[fe->frame_shift - 1];
            else
                fe->pre_emphasis_prior._float32 = fe->spch._float32[len - 1];
        }
        else
            fe_copy_to_frame_float32(fe->spch._float32, fe->frame, len);
    }
    else {
        if (fe->pre_emphasis_alpha != 0.0) {
            fe_pre_emphasis_int16(fe->spch._int16, fe->frame, len,
                                  fe->pre_emphasis_alpha,
                                  fe->pre_emphasis_prior._int16);
            if (len >= fe->frame_shift)
                fe->pre_emphasis_prior._int16 = fe->spch._int16[fe->frame_shift - 1];
            else
                fe->pre_emphasis_prior._int16 = fe->spch._int16[len - 1];
        }
        else
            fe_copy_to_frame_int16(fe->spch._int16, fe->frame, len);
    }

    /* Zero pad up to FFT size. */
    memset(fe->frame + len, 0, (fe->fft_size - len) * sizeof(*fe->frame));

    /* Window. */
    fe_hamming_window(fe->frame, fe->hamming_window, fe->frame_size,
                      fe->remove_dc);

    return len;
}

int
fe_read_frame_int16(fe_t * fe, int16 const *in, int32 len)
{
    int i;

    if (fe->is_float32) {
        E_ERROR("Called fe_read_frame_int16 when -input_float32 is true\n");
        return -1;
    }

    if (len > fe->frame_size)
        len = fe->frame_size;

    /* Read it into the raw speech buffer. */
    memcpy(fe->spch._int16, in, len * sizeof(*in));
    /* Swap and dither if necessary. */
    if (fe->swap)
        for (i = 0; i < len; ++i)
            SWAP_INT16(&fe->spch._int16[i]);
    if (fe->dither)
        for (i = 0; i < len; ++i)
            fe->spch._int16[i] += (int16) ((!(s3_rand_int31() % 4)) ? 1 : 0);

    return fe_spch_to_frame(fe, len);
}

int
fe_read_frame(fe_t * fe, int16 const *in, int32 len)
{
    return fe_read_frame_int16(fe, in, len);
}

#define FLOAT32_SCALE 32768.0
#define FLOAT32_DITHER 1.0

int
fe_read_frame_float32(fe_t * fe, float32 const *in, int32 len)
{
    int i;

    if (!fe->is_float32) {
        E_ERROR("Called fe_read_frame_float32 when -input_float32 is false\n");
        return -1;
    }

    if (len > fe->frame_size)
        len = fe->frame_size;

    /* Scale and dither if necessary. */
    if (fe->dither)
        for (i = 0; i < len; ++i)
            fe->spch._float32[i] =
                (in[i] * FLOAT32_SCALE
                 + ((!(s3_rand_int31() % 4)) ? FLOAT32_DITHER : 0.0));
    else
        for (i = 0; i < len; ++i)
            fe->spch._float32[i] = in[i] * FLOAT32_SCALE;

    return fe_spch_to_frame(fe, len);
}

int
fe_shift_frame_int16(fe_t * fe, int16 const *in, int32 len)
{
    int offset, i;

    if (fe->is_float32) {
        E_ERROR("Called fe_shift_frame_int16 when -input_float32 is true\n");
        return -1;
    }


    if (len > fe->frame_shift)
        len = fe->frame_shift;
    offset = fe->frame_size - fe->frame_shift;

    /* Shift data into the raw speech buffer. */
    memmove(fe->spch._int16, fe->spch._int16 + fe->frame_shift,
            offset * sizeof(*fe->spch._int16));
    memcpy(fe->spch._int16 + offset, in, len * sizeof(*fe->spch._int16));
    /* Swap and dither if necessary. */
    if (fe->swap)
        for (i = 0; i < len; ++i)
            SWAP_INT16(&fe->spch._int16[offset + i]);
    if (fe->dither)
        for (i = 0; i < len; ++i)
            fe->spch._int16[offset + i]
                += (int16) ((!(s3_rand_int31() % 4)) ? 1 : 0);

    return fe_spch_to_frame(fe, offset + len);
}

int
fe_shift_frame(fe_t * fe, int16 const *in, int32 len)
{
    return fe_shift_frame_int16(fe, in, len);
}

int
fe_shift_frame_float32(fe_t * fe, float32 const *in, int32 len)
{
    int offset, i;

    if (!fe->is_float32) {
        E_ERROR("Called fe_read_frame_float32 when -input_float32 is false\n");
        return -1;
    }

    if (len > fe->frame_shift)
        len = fe->frame_shift;
    offset = fe->frame_size - fe->frame_shift;

    /* Shift data into the raw speech buffer. */
    memmove(fe->spch._float32, fe->spch._float32 + fe->frame_shift,
            offset * sizeof(*fe->spch._float32));
    /* Scale and dither if necessary. */
    if (fe->dither)
        for (i = 0; i < len; ++i)
            fe->spch._float32[i + offset] =
                (in[i] * FLOAT32_SCALE
                 + ((!(s3_rand_int31() % 4)) ? FLOAT32_DITHER : 0.0));
    else
        for (i = 0; i < len; ++i)
            fe->spch._float32[i + offset] = in[i] * FLOAT32_SCALE;

    return fe_spch_to_frame(fe, offset + len);
}

/**
 * Create arrays of twiddle factors.
 */
void
fe_create_twiddle(fe_t * fe)
{
    int i;

    for (i = 0; i < fe->fft_size / 4; ++i) {
        float64 a = 2 * M_PI * i / fe->fft_size;
        fe->ccc[i] = cos(a);
        fe->sss[i] = sin(a);
    }
}


static int
fe_fft_real(fe_t * fe)
{
    int i, j, k, m, n;
    frame_t *x, xt;

    x = fe->frame;
    m = fe->fft_order;
    n = fe->fft_size;

    /* Bit-reverse the input. */
    j = 0;
    for (i = 0; i < n - 1; ++i) {
        if (i < j) {
            xt = x[j];
            x[j] = x[i];
            x[i] = xt;
        }
        k = n / 2;
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }

    /* Basic butterflies (2-point FFT, real twiddle factors):
     * x[i]   = x[i] +  1 * x[i+1]
     * x[i+1] = x[i] + -1 * x[i+1]
     */
    for (i = 0; i < n; i += 2) {
        xt = x[i];
        x[i] = (xt + x[i + 1]);
        x[i + 1] = (xt - x[i + 1]);
    }

    /* The rest of the butterflies, in stages from 1..m */
    for (k = 1; k < m; ++k) {
        int n1, n2, n4;

        n4 = k - 1;
        n2 = k;
        n1 = k + 1;
        /* Stride over each (1 << (k+1)) points */
        for (i = 0; i < n; i += (1 << n1)) {
            /* Basic butterfly with real twiddle factors:
             * x[i]          = x[i] +  1 * x[i + (1<<k)]
             * x[i + (1<<k)] = x[i] + -1 * x[i + (1<<k)]
             */
            xt = x[i];
            x[i] = (xt + x[i + (1 << n2)]);
            x[i + (1 << n2)] = (xt - x[i + (1 << n2)]);

            /* The other ones with real twiddle factors:
             * x[i + (1<<k) + (1<<(k-1))]
             *   = 0 * x[i + (1<<k-1)] + -1 * x[i + (1<<k) + (1<<k-1)]
             * x[i + (1<<(k-1))]
             *   = 1 * x[i + (1<<k-1)] +  0 * x[i + (1<<k) + (1<<k-1)]
             */
            x[i + (1 << n2) + (1 << n4)] = -x[i + (1 << n2) + (1 << n4)];
            x[i + (1 << n4)] = x[i + (1 << n4)];

            /* Butterflies with complex twiddle factors.
             * There are (1<<k-1) of them.
             */
            for (j = 1; j < (1 << n4); ++j) {
                frame_t cc, ss, t1, t2;
                int i1, i2, i3, i4;

                i1 = i + j;
                i2 = i + (1 << n2) - j;
                i3 = i + (1 << n2) + j;
                i4 = i + (1 << n2) + (1 << n2) - j;

                /*
                 * cc = real(W[j * n / (1<<(k+1))])
                 * ss = imag(W[j * n / (1<<(k+1))])
                 */
                cc = fe->ccc[j << (m - n1)];
                ss = fe->sss[j << (m - n1)];

                /* There are some symmetry properties which allow us
                 * to get away with only four multiplications here. */
                t1 = COSMUL(x[i3], cc) + COSMUL(x[i4], ss);
                t2 = COSMUL(x[i3], ss) - COSMUL(x[i4], cc);

                x[i4] = (x[i2] - t2);
                x[i3] = (-x[i2] - t2);
                x[i2] = (x[i1] - t1);
                x[i1] = (x[i1] + t1);
            }
        }
    }

    /* This isn't used, but return it for completeness. */
    return m;
}

static void
fe_spec_magnitude(fe_t * fe)
{
    frame_t *fft;
    powspec_t *spec;
    int32 j, scale, fftsize;

    /* Do FFT and get the scaling factor back (only actually used in
     * fixed-point).  Note the scaling factor is expressed in bits. */
    scale = fe_fft_real(fe);

    /* Convenience pointers to make things less awkward below. */
    fft = fe->frame;
    spec = fe->spec;
    fftsize = fe->fft_size;

    /* We need to scale things up the rest of the way to N. */
    scale = fe->fft_order - scale;

    /* The first point (DC coefficient) has no imaginary part */
    {
        spec[0] = fft[0] * fft[0];
    }

    for (j = 1; j <= fftsize / 2; j++) {
        spec[j] = fft[j] * fft[j] + fft[fftsize - j] * fft[fftsize - j];
    }
}

static void
fe_mel_spec(fe_t * fe)
{
    int whichfilt;
    powspec_t *spec, *mfspec;

    /* Convenience poitners. */
    spec = fe->spec;
    mfspec = fe->mfspec;
    for (whichfilt = 0; whichfilt < fe->mel_fb->num_filters; whichfilt++) {
        int spec_start, filt_start, i;

        spec_start = fe->mel_fb->spec_start[whichfilt];
        filt_start = fe->mel_fb->filt_start[whichfilt];

        mfspec[whichfilt] = 0;
        for (i = 0; i < fe->mel_fb->filt_width[whichfilt]; i++)
            mfspec[whichfilt] +=
                spec[spec_start + i] * fe->mel_fb->filt_coeffs[filt_start +
                                                               i];
    }

}

#define LOG_FLOOR 1e-4

static void
fe_mel_cep(fe_t * fe, mfcc_t * mfcep)
{
    int32 i;
    powspec_t *mfspec;

    /* Convenience pointer. */
    mfspec = fe->mfspec;

    for (i = 0; i < fe->mel_fb->num_filters; ++i) {
        mfspec[i] = log(mfspec[i] + LOG_FLOOR);
    }

    /* If we are doing LOG_SPEC, then do nothing. */
    if (fe->log_spec == RAW_LOG_SPEC) {
        for (i = 0; i < fe->feature_dimension; i++) {
            mfcep[i] = (mfcc_t) mfspec[i];
        }
    }
    /* For smoothed spectrum, do DCT-II followed by (its inverse) DCT-III */
    else if (fe->log_spec == SMOOTH_LOG_SPEC) {
        /* FIXME: This is probably broken for fixed-point. */
        fe_dct2(fe, mfspec, mfcep, 0);
        fe_dct3(fe, mfcep, mfspec);
        for (i = 0; i < fe->feature_dimension; i++) {
            mfcep[i] = (mfcc_t) mfspec[i];
        }
    }
    else if (fe->transform == DCT_II)
        fe_dct2(fe, mfspec, mfcep, FALSE);
    else if (fe->transform == DCT_HTK)
        fe_dct2(fe, mfspec, mfcep, TRUE);
    else
        fe_spec2cep(fe, mfspec, mfcep);

    return;
}

void
fe_spec2cep(fe_t * fe, const powspec_t * mflogspec, mfcc_t * mfcep)
{
    int32 i, j, beta;

    /* Compute C0 separately (its basis vector is 1) to avoid
     * costly multiplications. */
    mfcep[0] = mflogspec[0] / 2;        /* beta = 0.5 */
    for (j = 1; j < fe->mel_fb->num_filters; j++)
        mfcep[0] += mflogspec[j];       /* beta = 1.0 */
    mfcep[0] /= (frame_t) fe->mel_fb->num_filters;

    for (i = 1; i < fe->num_cepstra; ++i) {
        mfcep[i] = 0;
        for (j = 0; j < fe->mel_fb->num_filters; j++) {
            if (j == 0)
                beta = 1;       /* 0.5 */
            else
                beta = 2;       /* 1.0 */
            mfcep[i] += COSMUL(mflogspec[j],
                               fe->mel_fb->mel_cosine[i][j]) * beta;
        }
        /* Note that this actually normalizes by num_filters, like the
         * original Sphinx front-end, due to the doubled 'beta' factor
         * above.  */
        mfcep[i] /= (frame_t) fe->mel_fb->num_filters * 2;
    }
}

void
fe_dct2(fe_t * fe, const powspec_t * mflogspec, mfcc_t * mfcep, int htk)
{
    int32 i, j;

    /* Compute C0 separately (its basis vector is 1) to avoid
     * costly multiplications. */
    mfcep[0] = mflogspec[0];
    for (j = 1; j < fe->mel_fb->num_filters; j++)
        mfcep[0] += mflogspec[j];
    if (htk)
        mfcep[0] = COSMUL(mfcep[0], fe->mel_fb->sqrt_inv_2n);
    else                        /* sqrt(1/N) = sqrt(2/N) * 1/sqrt(2) */
        mfcep[0] = COSMUL(mfcep[0], fe->mel_fb->sqrt_inv_n);

    for (i = 1; i < fe->num_cepstra; ++i) {
        mfcep[i] = 0;
        for (j = 0; j < fe->mel_fb->num_filters; j++) {
            mfcep[i] += COSMUL(mflogspec[j], fe->mel_fb->mel_cosine[i][j]);
        }
        mfcep[i] = COSMUL(mfcep[i], fe->mel_fb->sqrt_inv_2n);
    }
}

void
fe_lifter(fe_t * fe, mfcc_t * mfcep)
{
    int32 i;

    if (fe->mel_fb->lifter_val == 0)
        return;

    for (i = 0; i < fe->num_cepstra; ++i) {
        mfcep[i] = MFCCMUL(mfcep[i], fe->mel_fb->lifter[i]);
    }
}

void
fe_dct3(fe_t * fe, const mfcc_t * mfcep, powspec_t * mflogspec)
{
    int32 i, j;

    for (i = 0; i < fe->mel_fb->num_filters; ++i) {
        mflogspec[i] = COSMUL(mfcep[0], SQRT_HALF);
        for (j = 1; j < fe->num_cepstra; j++) {
            mflogspec[i] += COSMUL(mfcep[j], fe->mel_fb->mel_cosine[j][i]);
        }
        mflogspec[i] = COSMUL(mflogspec[i], fe->mel_fb->sqrt_inv_2n);
    }
}

void
fe_write_frame(fe_t * fe, mfcc_t * feat)
{
    fe_spec_magnitude(fe);
    fe_mel_spec(fe);
    fe_mel_cep(fe, feat);
    fe_lifter(fe, feat);
}


void *
fe_create_2d(int32 d1, int32 d2, int32 elem_size)
{
    return (void *) ckd_calloc_2d(d1, d2, elem_size);
}

void
fe_free_2d(void *arr)
{
    ckd_free_2d((void **) arr);
}

/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
 * Interface for "semi-continuous vector quantization", a.k.a. Sphinx2
 * fast GMM computation.
 */

#ifndef __S2_SEMI_MGAU_H__
#define __S2_SEMI_MGAU_H__

#include <soundswallower/acmod.h>
#include <soundswallower/bin_mdef.h>
#include <soundswallower/fe.h>
#include <soundswallower/hmm.h>
#include <soundswallower/logmath.h>
#include <soundswallower/ms_gauden.h>
#include <soundswallower/s3file.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef struct vqFeature_s vqFeature_t;

typedef struct s2_semi_mgau_s s2_semi_mgau_t;
struct s2_semi_mgau_s {
    mgau_t base; /**< base structure. */
    config_t *config; /* configuration parameters */

    gauden_t *g; /* Set of Gaussians (pointers below point in here and will go away soon) */

    uint8 ***mixw; /* mixture weight distributions */
    s3file_t *sendump_mmap; /* memory map for mixw (or NULL if not mmap) */

    uint8 *mixw_cb; /* mixture weight codebook, if any (assume it contains 16 values) */
    int32 n_sen; /* Number of senones */
    uint8 *topn_beam; /* Beam for determining per-frame top-N densities */
    int16 max_topn;
    int16 ds_ratio;

    vqFeature_t ***topn_hist; /**< Top-N scores and codewords for past frames. */
    uint8 **topn_hist_n; /**< Variable top-N for past frames. */
    vqFeature_t **f; /**< Topn-N for currently scoring frame. */
    int n_topn_hist; /**< Number of past frames tracked. */

    /* Log-add table for compressed values. */
    logmath_t *lmath_8b;
    /* Log-add object for reloading means/variances. */
    logmath_t *lmath;
};

mgau_t *s2_semi_mgau_init(acmod_t *acmod);
mgau_t *s2_semi_mgau_init_s3file(acmod_t *acmod, s3file_t *means, s3file_t *vars,
                                 s3file_t *mixw, s3file_t *sendump);
void s2_semi_mgau_free(mgau_t *s);
int s2_semi_mgau_frame_eval(mgau_t *s,
                            int16 *senone_scores,
                            uint8 *senone_active,
                            int32 n_senone_active,
                            mfcc_t **featbuf,
                            int32 frame,
                            int32 compallsen);
int s2_semi_mgau_mllr_transform(mgau_t *s,
                                mllr_t *mllr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*  __S2_SEMI_MGAU_H__ */

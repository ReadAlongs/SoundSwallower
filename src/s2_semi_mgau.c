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

#include "config.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <soundswallower/ckd_alloc.h>
#include <soundswallower/configuration.h>
#include <soundswallower/err.h>
#include <soundswallower/prim_type.h>
#include <soundswallower/s2_semi_mgau.h>
#include <soundswallower/tied_mgau_common.h>

static mgaufuncs_t s2_semi_mgau_funcs = {
    "s2_semi",
    s2_semi_mgau_frame_eval, /* frame_eval */
    s2_semi_mgau_mllr_transform, /* transform */
    s2_semi_mgau_free /* free */
};

struct vqFeature_s {
    int32 score; /* score or distance */
    int32 codeword; /* codeword (vector index) */
};

static void
eval_topn(s2_semi_mgau_t *s, int32 feat, mfcc_t *z)
{
    int i, ceplen;
    vqFeature_t *topn;

    topn = s->f[feat];
    ceplen = s->g->featlen[feat];

    for (i = 0; i < s->max_topn; i++) {
        mfcc_t *mean, diff, sqdiff, compl ; /* diff, diff^2, component likelihood */
        vqFeature_t vtmp;
        mfcc_t *var, d;
        mfcc_t *obs;
        int32 cw, j;

        cw = topn[i].codeword;
        mean = s->g->mean[0][feat][0] + cw * ceplen;
        var = s->g->var[0][feat][0] + cw * ceplen;
        d = s->g->det[0][feat][cw];
        obs = z;
        for (j = 0; j < ceplen; j++) {
            diff = *obs++ - *mean++;
            sqdiff = MFCCMUL(diff, diff);
            compl = MFCCMUL(sqdiff, *var);
            d = GMMSUB(d, compl );
            ++var;
        }
        if (d < (mfcc_t)MAX_NEG_INT32) /* Redundant if FIXED_POINT */
            topn[i].score = MAX_NEG_INT32;
        else
            topn[i].score = (int32)d;
        if (i == 0)
            continue;
        vtmp = topn[i];
        for (j = i - 1; j >= 0 && vtmp.score > topn[j].score; j--) {
            topn[j + 1] = topn[j];
        }
        topn[j + 1] = vtmp;
    }
}

static void
eval_cb(s2_semi_mgau_t *s, int32 feat, mfcc_t *z)
{
    vqFeature_t *worst, *best, *topn;
    mfcc_t *mean;
    mfcc_t *var, *det, *detP, *detE;
    int32 i, ceplen;

    best = topn = s->f[feat];
    worst = topn + (s->max_topn - 1);
    mean = s->g->mean[0][feat][0];
    var = s->g->var[0][feat][0];
    det = s->g->det[0][feat];
    detE = det + s->g->n_density;
    ceplen = s->g->featlen[feat];

    for (detP = det; detP < detE; ++detP) {
        mfcc_t diff, sqdiff, compl ; /* diff, diff^2, component likelihood */
        mfcc_t d;
        mfcc_t *obs;
        vqFeature_t *cur;
        int32 cw, j, d_int;

        d = *detP;
        obs = z;
        cw = (int)(detP - det);
        for (j = 0; (j < ceplen) && (d >= worst->score); ++j) {
            diff = *obs++ - *mean++;
            sqdiff = MFCCMUL(diff, diff);
            compl = MFCCMUL(sqdiff, *var);
            d = GMMSUB(d, compl );
            ++var;
        }
        if (j < ceplen) {
            /* terminated early, so not in topn */
            mean += (ceplen - j);
            var += (ceplen - j);
            continue;
        }
        if (d < (mfcc_t)MAX_NEG_INT32)
            d_int = MAX_NEG_INT32;
        else
            d_int = (int32)d;
        if (d_int < worst->score)
            continue;
        for (i = 0; i < s->max_topn; i++) {
            /* already there, so don't need to insert */
            if (topn[i].codeword == cw)
                break;
        }
        if (i < s->max_topn)
            continue; /* already there.  Don't insert */
        /* remaining code inserts codeword and dist in correct spot */
        for (cur = worst - 1; cur >= best && d_int >= cur->score; --cur)
            memcpy(cur + 1, cur, sizeof(vqFeature_t));
        ++cur;
        cur->codeword = cw;
        cur->score = d_int;
    }
}

static void
mgau_dist(s2_semi_mgau_t *s, int32 frame, int32 feat, mfcc_t *z)
{
    eval_topn(s, feat, z);

    /* If this frame is skipped, do nothing else. */
    if (frame % s->ds_ratio)
        return;

    /* Evaluate the rest of the codebook (or subset thereof). */
    eval_cb(s, feat, z);
}

static int
mgau_norm(s2_semi_mgau_t *s, int feat)
{
    int32 norm;
    int j;

    /* Compute quantized normalizing constant. */
    norm = s->f[feat][0].score >> SENSCR_SHIFT;

    /* Normalize the scores, negate them, and clamp their dynamic range. */
    for (j = 0; j < s->max_topn; ++j) {
        s->f[feat][j].score = -((s->f[feat][j].score >> SENSCR_SHIFT) - norm);
        if (s->f[feat][j].score > MAX_NEG_ASCR)
            s->f[feat][j].score = MAX_NEG_ASCR;
        if (s->topn_beam[feat] && s->f[feat][j].score > s->topn_beam[feat])
            break;
    }
    return j;
}

static int32
get_scores_8b_feat_6(s2_semi_mgau_t *s, int i,
                     int16 *senone_scores, uint8 *senone_active,
                     int32 n_senone_active)
{
    int32 j, l;
    uint8 *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3, *pid_cw4, *pid_cw5;

    pid_cw0 = s->mixw[i][s->f[i][0].codeword];
    pid_cw1 = s->mixw[i][s->f[i][1].codeword];
    pid_cw2 = s->mixw[i][s->f[i][2].codeword];
    pid_cw3 = s->mixw[i][s->f[i][3].codeword];
    pid_cw4 = s->mixw[i][s->f[i][4].codeword];
    pid_cw5 = s->mixw[i][s->f[i][5].codeword];

    for (l = j = 0; j < n_senone_active; j++) {
        int sen = senone_active[j] + l;
        int32 tmp = pid_cw0[sen] + s->f[i][0].score;

        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw1[sen] + s->f[i][1].score);
        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw2[sen] + s->f[i][2].score);
        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw3[sen] + s->f[i][3].score);
        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw4[sen] + s->f[i][4].score);
        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw5[sen] + s->f[i][5].score);

        senone_scores[sen] += tmp;
        l = sen;
    }
    return 0;
}

static int32
get_scores_8b_feat_5(s2_semi_mgau_t *s, int i,
                     int16 *senone_scores, uint8 *senone_active,
                     int32 n_senone_active)
{
    int32 j, l;
    uint8 *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3, *pid_cw4;

    pid_cw0 = s->mixw[i][s->f[i][0].codeword];
    pid_cw1 = s->mixw[i][s->f[i][1].codeword];
    pid_cw2 = s->mixw[i][s->f[i][2].codeword];
    pid_cw3 = s->mixw[i][s->f[i][3].codeword];
    pid_cw4 = s->mixw[i][s->f[i][4].codeword];

    for (l = j = 0; j < n_senone_active; j++) {
        int sen = senone_active[j] + l;
        int32 tmp = pid_cw0[sen] + s->f[i][0].score;

        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw1[sen] + s->f[i][1].score);
        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw2[sen] + s->f[i][2].score);
        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw3[sen] + s->f[i][3].score);
        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw4[sen] + s->f[i][4].score);

        senone_scores[sen] += tmp;
        l = sen;
    }
    return 0;
}

static int32
get_scores_8b_feat_4(s2_semi_mgau_t *s, int i,
                     int16 *senone_scores, uint8 *senone_active,
                     int32 n_senone_active)
{
    int32 j, l;
    uint8 *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3;

    pid_cw0 = s->mixw[i][s->f[i][0].codeword];
    pid_cw1 = s->mixw[i][s->f[i][1].codeword];
    pid_cw2 = s->mixw[i][s->f[i][2].codeword];
    pid_cw3 = s->mixw[i][s->f[i][3].codeword];

    for (l = j = 0; j < n_senone_active; j++) {
        int sen = senone_active[j] + l;
        int32 tmp = pid_cw0[sen] + s->f[i][0].score;

        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw1[sen] + s->f[i][1].score);
        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw2[sen] + s->f[i][2].score);
        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw3[sen] + s->f[i][3].score);

        senone_scores[sen] += tmp;
        l = sen;
    }
    return 0;
}

static int32
get_scores_8b_feat_3(s2_semi_mgau_t *s, int i,
                     int16 *senone_scores, uint8 *senone_active,
                     int32 n_senone_active)
{
    int32 j, l;
    uint8 *pid_cw0, *pid_cw1, *pid_cw2;

    pid_cw0 = s->mixw[i][s->f[i][0].codeword];
    pid_cw1 = s->mixw[i][s->f[i][1].codeword];
    pid_cw2 = s->mixw[i][s->f[i][2].codeword];

    for (l = j = 0; j < n_senone_active; j++) {
        int sen = senone_active[j] + l;
        int32 tmp = pid_cw0[sen] + s->f[i][0].score;

        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw1[sen] + s->f[i][1].score);
        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw2[sen] + s->f[i][2].score);

        senone_scores[sen] += tmp;
        l = sen;
    }
    return 0;
}

static int32
get_scores_8b_feat_2(s2_semi_mgau_t *s, int i,
                     int16 *senone_scores, uint8 *senone_active,
                     int32 n_senone_active)
{
    int32 j, l;
    uint8 *pid_cw0, *pid_cw1;

    pid_cw0 = s->mixw[i][s->f[i][0].codeword];
    pid_cw1 = s->mixw[i][s->f[i][1].codeword];

    for (l = j = 0; j < n_senone_active; j++) {
        int sen = senone_active[j] + l;
        int32 tmp = pid_cw0[sen] + s->f[i][0].score;

        tmp = fast_logmath_add(s->lmath_8b, tmp,
                               pid_cw1[sen] + s->f[i][1].score);

        senone_scores[sen] += tmp;
        l = sen;
    }
    return 0;
}

static int32
get_scores_8b_feat_1(s2_semi_mgau_t *s, int i,
                     int16 *senone_scores, uint8 *senone_active,
                     int32 n_senone_active)
{
    int32 j, l;
    uint8 *pid_cw0;

    pid_cw0 = s->mixw[i][s->f[i][0].codeword];
    for (l = j = 0; j < n_senone_active; j++) {
        int sen = senone_active[j] + l;
        int32 tmp = pid_cw0[sen] + s->f[i][0].score;
        senone_scores[sen] += tmp;
        l = sen;
    }
    return 0;
}

static int32
get_scores_8b_feat_any(s2_semi_mgau_t *s, int i, int topn,
                       int16 *senone_scores, uint8 *senone_active,
                       int32 n_senone_active)
{
    int32 j, k, l;

    for (l = j = 0; j < n_senone_active; j++) {
        int sen = senone_active[j] + l;
        uint8 *pid_cw;
        int32 tmp;
        pid_cw = s->mixw[i][s->f[i][0].codeword];
        tmp = pid_cw[sen] + s->f[i][0].score;
        for (k = 1; k < topn; ++k) {
            pid_cw = s->mixw[i][s->f[i][k].codeword];
            tmp = fast_logmath_add(s->lmath_8b, tmp,
                                   pid_cw[sen] + s->f[i][k].score);
        }
        senone_scores[sen] += tmp;
        l = sen;
    }
    return 0;
}

static int32
get_scores_8b_feat(s2_semi_mgau_t *s, int i, int topn,
                   int16 *senone_scores, uint8 *senone_active, int32 n_senone_active)
{
    switch (topn) {
    case 6:
        return get_scores_8b_feat_6(s, i, senone_scores,
                                    senone_active, n_senone_active);
    case 5:
        return get_scores_8b_feat_5(s, i, senone_scores,
                                    senone_active, n_senone_active);
    case 4:
        return get_scores_8b_feat_4(s, i, senone_scores,
                                    senone_active, n_senone_active);
    case 3:
        return get_scores_8b_feat_3(s, i, senone_scores,
                                    senone_active, n_senone_active);
    case 2:
        return get_scores_8b_feat_2(s, i, senone_scores,
                                    senone_active, n_senone_active);
    case 1:
        return get_scores_8b_feat_1(s, i, senone_scores,
                                    senone_active, n_senone_active);
    default:
        return get_scores_8b_feat_any(s, i, topn, senone_scores,
                                      senone_active, n_senone_active);
    }
}

static int32
get_scores_8b_feat_all(s2_semi_mgau_t *s, int i, int topn, int16 *senone_scores)
{
    int32 j, k;

    for (j = 0; j < s->n_sen; j++) {
        uint8 *pid_cw;
        int32 tmp;
        pid_cw = s->mixw[i][s->f[i][0].codeword];
        tmp = pid_cw[j] + s->f[i][0].score;
        for (k = 1; k < topn; ++k) {
            pid_cw = s->mixw[i][s->f[i][k].codeword];
            tmp = fast_logmath_add(s->lmath_8b, tmp,
                                   pid_cw[j] + s->f[i][k].score);
        }
        senone_scores[j] += tmp;
    }
    return 0;
}

static int32
get_scores_4b_feat_6(s2_semi_mgau_t *s, int i,
                     int16 *senone_scores, uint8 *senone_active,
                     int32 n_senone_active)
{
    int32 j, l;
    uint8 *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3, *pid_cw4, *pid_cw5;
    uint8 w_den[6][16];

    /* Precompute scaled densities. */
    for (j = 0; j < 16; ++j) {
        w_den[0][j] = s->mixw_cb[j] + s->f[i][0].score;
        w_den[1][j] = s->mixw_cb[j] + s->f[i][1].score;
        w_den[2][j] = s->mixw_cb[j] + s->f[i][2].score;
        w_den[3][j] = s->mixw_cb[j] + s->f[i][3].score;
        w_den[4][j] = s->mixw_cb[j] + s->f[i][4].score;
        w_den[5][j] = s->mixw_cb[j] + s->f[i][5].score;
    }

    pid_cw0 = s->mixw[i][s->f[i][0].codeword];
    pid_cw1 = s->mixw[i][s->f[i][1].codeword];
    pid_cw2 = s->mixw[i][s->f[i][2].codeword];
    pid_cw3 = s->mixw[i][s->f[i][3].codeword];
    pid_cw4 = s->mixw[i][s->f[i][4].codeword];
    pid_cw5 = s->mixw[i][s->f[i][5].codeword];

    for (l = j = 0; j < n_senone_active; j++) {
        int n = senone_active[j] + l;
        int tmp, cw;

        if (n & 1) {
            cw = pid_cw0[n / 2] >> 4;
            tmp = w_den[0][cw];
            cw = pid_cw1[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[1][cw]);
            cw = pid_cw2[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[2][cw]);
            cw = pid_cw3[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[3][cw]);
            cw = pid_cw4[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[4][cw]);
            cw = pid_cw5[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[5][cw]);
        } else {
            cw = pid_cw0[n / 2] & 0x0f;
            tmp = w_den[0][cw];
            cw = pid_cw1[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[1][cw]);
            cw = pid_cw2[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[2][cw]);
            cw = pid_cw3[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[3][cw]);
            cw = pid_cw4[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[4][cw]);
            cw = pid_cw5[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[5][cw]);
        }
        senone_scores[n] += tmp;
        l = n;
    }
    return 0;
}

static int32
get_scores_4b_feat_5(s2_semi_mgau_t *s, int i,
                     int16 *senone_scores, uint8 *senone_active,
                     int32 n_senone_active)
{
    int32 j, l;
    uint8 *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3, *pid_cw4;
    uint8 w_den[5][16];

    /* Precompute scaled densities. */
    for (j = 0; j < 16; ++j) {
        w_den[0][j] = s->mixw_cb[j] + s->f[i][0].score;
        w_den[1][j] = s->mixw_cb[j] + s->f[i][1].score;
        w_den[2][j] = s->mixw_cb[j] + s->f[i][2].score;
        w_den[3][j] = s->mixw_cb[j] + s->f[i][3].score;
        w_den[4][j] = s->mixw_cb[j] + s->f[i][4].score;
    }

    pid_cw0 = s->mixw[i][s->f[i][0].codeword];
    pid_cw1 = s->mixw[i][s->f[i][1].codeword];
    pid_cw2 = s->mixw[i][s->f[i][2].codeword];
    pid_cw3 = s->mixw[i][s->f[i][3].codeword];
    pid_cw4 = s->mixw[i][s->f[i][4].codeword];

    for (l = j = 0; j < n_senone_active; j++) {
        int n = senone_active[j] + l;
        int tmp, cw;

        if (n & 1) {
            cw = pid_cw0[n / 2] >> 4;
            tmp = w_den[0][cw];
            cw = pid_cw1[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[1][cw]);
            cw = pid_cw2[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[2][cw]);
            cw = pid_cw3[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[3][cw]);
            cw = pid_cw4[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[4][cw]);
        } else {
            cw = pid_cw0[n / 2] & 0x0f;
            tmp = w_den[0][cw];
            cw = pid_cw1[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[1][cw]);
            cw = pid_cw2[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[2][cw]);
            cw = pid_cw3[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[3][cw]);
            cw = pid_cw4[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[4][cw]);
        }
        senone_scores[n] += tmp;
        l = n;
    }
    return 0;
}

static int32
get_scores_4b_feat_4(s2_semi_mgau_t *s, int i,
                     int16 *senone_scores, uint8 *senone_active,
                     int32 n_senone_active)
{
    int32 j, l;
    uint8 *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3;
    uint8 w_den[4][16];

    /* Precompute scaled densities. */
    for (j = 0; j < 16; ++j) {
        w_den[0][j] = s->mixw_cb[j] + s->f[i][0].score;
        w_den[1][j] = s->mixw_cb[j] + s->f[i][1].score;
        w_den[2][j] = s->mixw_cb[j] + s->f[i][2].score;
        w_den[3][j] = s->mixw_cb[j] + s->f[i][3].score;
    }

    pid_cw0 = s->mixw[i][s->f[i][0].codeword];
    pid_cw1 = s->mixw[i][s->f[i][1].codeword];
    pid_cw2 = s->mixw[i][s->f[i][2].codeword];
    pid_cw3 = s->mixw[i][s->f[i][3].codeword];

    for (l = j = 0; j < n_senone_active; j++) {
        int n = senone_active[j] + l;
        int tmp, cw;

        if (n & 1) {
            cw = pid_cw0[n / 2] >> 4;
            tmp = w_den[0][cw];
            cw = pid_cw1[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[1][cw]);
            cw = pid_cw2[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[2][cw]);
            cw = pid_cw3[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[3][cw]);
        } else {
            cw = pid_cw0[n / 2] & 0x0f;
            tmp = w_den[0][cw];
            cw = pid_cw1[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[1][cw]);
            cw = pid_cw2[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[2][cw]);
            cw = pid_cw3[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[3][cw]);
        }
        senone_scores[n] += tmp;
        l = n;
    }
    return 0;
}

static int32
get_scores_4b_feat_3(s2_semi_mgau_t *s, int i,
                     int16 *senone_scores, uint8 *senone_active,
                     int32 n_senone_active)
{
    int32 j, l;
    uint8 *pid_cw0, *pid_cw1, *pid_cw2;
    uint8 w_den[3][16];

    /* Precompute scaled densities. */
    for (j = 0; j < 16; ++j) {
        w_den[0][j] = s->mixw_cb[j] + s->f[i][0].score;
        w_den[1][j] = s->mixw_cb[j] + s->f[i][1].score;
        w_den[2][j] = s->mixw_cb[j] + s->f[i][2].score;
    }

    pid_cw0 = s->mixw[i][s->f[i][0].codeword];
    pid_cw1 = s->mixw[i][s->f[i][1].codeword];
    pid_cw2 = s->mixw[i][s->f[i][2].codeword];

    for (l = j = 0; j < n_senone_active; j++) {
        int n = senone_active[j] + l;
        int tmp, cw;

        if (n & 1) {
            cw = pid_cw0[n / 2] >> 4;
            tmp = w_den[0][cw];
            cw = pid_cw1[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[1][cw]);
            cw = pid_cw2[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[2][cw]);
        } else {
            cw = pid_cw0[n / 2] & 0x0f;
            tmp = w_den[0][cw];
            cw = pid_cw1[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[1][cw]);
            cw = pid_cw2[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[2][cw]);
        }
        senone_scores[n] += tmp;
        l = n;
    }
    return 0;
}

static int32
get_scores_4b_feat_2(s2_semi_mgau_t *s, int i,
                     int16 *senone_scores, uint8 *senone_active,
                     int32 n_senone_active)
{
    int32 j, l;
    uint8 *pid_cw0, *pid_cw1;
    uint8 w_den[2][16];

    /* Precompute scaled densities. */
    for (j = 0; j < 16; ++j) {
        w_den[0][j] = s->mixw_cb[j] + s->f[i][0].score;
        w_den[1][j] = s->mixw_cb[j] + s->f[i][1].score;
    }

    pid_cw0 = s->mixw[i][s->f[i][0].codeword];
    pid_cw1 = s->mixw[i][s->f[i][1].codeword];

    for (l = j = 0; j < n_senone_active; j++) {
        int n = senone_active[j] + l;
        int tmp, cw;

        if (n & 1) {
            cw = pid_cw0[n / 2] >> 4;
            tmp = w_den[0][cw];
            cw = pid_cw1[n / 2] >> 4;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[1][cw]);
        } else {
            cw = pid_cw0[n / 2] & 0x0f;
            tmp = w_den[0][cw];
            cw = pid_cw1[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp, w_den[1][cw]);
        }
        senone_scores[n] += tmp;
        l = n;
    }
    return 0;
}

static int32
get_scores_4b_feat_1(s2_semi_mgau_t *s, int i,
                     int16 *senone_scores, uint8 *senone_active,
                     int32 n_senone_active)
{
    int32 j, l;
    uint8 *pid_cw0;
    uint8 w_den[16];

    /* Precompute scaled densities. */
    for (j = 0; j < 16; ++j) {
        w_den[j] = s->mixw_cb[j] + s->f[i][0].score;
    }

    pid_cw0 = s->mixw[i][s->f[i][0].codeword];

    for (l = j = 0; j < n_senone_active; j++) {
        int n = senone_active[j] + l;
        int tmp, cw;

        if (n & 1) {
            cw = pid_cw0[n / 2] >> 4;
            tmp = w_den[cw];
        } else {
            cw = pid_cw0[n / 2] & 0x0f;
            tmp = w_den[cw];
        }
        senone_scores[n] += tmp;
        l = n;
    }
    return 0;
}

static int32
get_scores_4b_feat_any(s2_semi_mgau_t *s, int i, int topn,
                       int16 *senone_scores, uint8 *senone_active,
                       int32 n_senone_active)
{
    int32 j, k, l;

    for (l = j = 0; j < n_senone_active; j++) {
        int n = senone_active[j] + l;
        int tmp, cw;
        uint8 *pid_cw;

        pid_cw = s->mixw[i][s->f[i][0].codeword];
        if (n & 1)
            cw = pid_cw[n / 2] >> 4;
        else
            cw = pid_cw[n / 2] & 0x0f;
        tmp = s->mixw_cb[cw] + s->f[i][0].score;
        for (k = 1; k < topn; ++k) {
            pid_cw = s->mixw[i][s->f[i][k].codeword];
            if (n & 1)
                cw = pid_cw[n / 2] >> 4;
            else
                cw = pid_cw[n / 2] & 0x0f;
            tmp = fast_logmath_add(s->lmath_8b, tmp,
                                   s->mixw_cb[cw] + s->f[i][k].score);
        }
        senone_scores[n] += tmp;
        l = n;
    }
    return 0;
}

static int32
get_scores_4b_feat(s2_semi_mgau_t *s, int i, int topn,
                   int16 *senone_scores, uint8 *senone_active, int32 n_senone_active)
{
    switch (topn) {
    case 6:
        return get_scores_4b_feat_6(s, i, senone_scores,
                                    senone_active, n_senone_active);
    case 5:
        return get_scores_4b_feat_5(s, i, senone_scores,
                                    senone_active, n_senone_active);
    case 4:
        return get_scores_4b_feat_4(s, i, senone_scores,
                                    senone_active, n_senone_active);
    case 3:
        return get_scores_4b_feat_3(s, i, senone_scores,
                                    senone_active, n_senone_active);
    case 2:
        return get_scores_4b_feat_2(s, i, senone_scores,
                                    senone_active, n_senone_active);
    case 1:
        return get_scores_4b_feat_1(s, i, senone_scores,
                                    senone_active, n_senone_active);
    default:
        return get_scores_4b_feat_any(s, i, topn, senone_scores,
                                      senone_active, n_senone_active);
    }
}

static int32
get_scores_4b_feat_all(s2_semi_mgau_t *s, int i, int topn, int16 *senone_scores)
{
    int j, last_sen;

    j = 0;
    /* Number of senones is always even, but don't overrun if it isn't. */
    last_sen = s->n_sen & ~1;
    while (j < last_sen) {
        uint8 *pid_cw;
        int32 tmp0, tmp1;
        int k;

        pid_cw = s->mixw[i][s->f[i][0].codeword];
        tmp0 = s->mixw_cb[pid_cw[j / 2] & 0x0f] + s->f[i][0].score;
        tmp1 = s->mixw_cb[pid_cw[j / 2] >> 4] + s->f[i][0].score;
        for (k = 1; k < topn; ++k) {
            int32 w_den0, w_den1;

            pid_cw = s->mixw[i][s->f[i][k].codeword];
            w_den0 = s->mixw_cb[pid_cw[j / 2] & 0x0f] + s->f[i][k].score;
            w_den1 = s->mixw_cb[pid_cw[j / 2] >> 4] + s->f[i][k].score;
            tmp0 = fast_logmath_add(s->lmath_8b, tmp0, w_den0);
            tmp1 = fast_logmath_add(s->lmath_8b, tmp1, w_den1);
        }
        senone_scores[j++] += tmp0;
        senone_scores[j++] += tmp1;
    }
    return 0;
}

/*
 * Compute senone scores for the active senones.
 */
int32
s2_semi_mgau_frame_eval(mgau_t *ps,
                        int16 *senone_scores,
                        uint8 *senone_active,
                        int32 n_senone_active,
                        mfcc_t **featbuf, int32 frame,
                        int32 compallsen)
{
    s2_semi_mgau_t *s = (s2_semi_mgau_t *)ps;
    int i, topn_idx;
    int n_feat = s->g->n_feat;

    memset(senone_scores, 0, s->n_sen * sizeof(*senone_scores));
    /* No bounds checking is done here, which just means you'll get
     * semi-random crap if you request a frame in the future or one
     * that's too far in the past. */
    topn_idx = frame % s->n_topn_hist;
    s->f = s->topn_hist[topn_idx];
    for (i = 0; i < n_feat; ++i) {
        /* For past frames this will already be computed. */
        if (frame >= ps_mgau_base(ps)->frame_idx) {
            vqFeature_t **lastf;
            if (topn_idx == 0)
                lastf = s->topn_hist[s->n_topn_hist - 1];
            else
                lastf = s->topn_hist[topn_idx - 1];
            memcpy(s->f[i], lastf[i], sizeof(vqFeature_t) * s->max_topn);
            mgau_dist(s, frame, i, featbuf[i]);
            s->topn_hist_n[topn_idx][i] = mgau_norm(s, i);
        }
        if (s->mixw_cb) {
            if (compallsen)
                get_scores_4b_feat_all(s, i, s->topn_hist_n[topn_idx][i], senone_scores);
            else
                get_scores_4b_feat(s, i, s->topn_hist_n[topn_idx][i], senone_scores,
                                   senone_active, n_senone_active);
        } else {
            if (compallsen)
                get_scores_8b_feat_all(s, i, s->topn_hist_n[topn_idx][i], senone_scores);
            else
                get_scores_8b_feat(s, i, s->topn_hist_n[topn_idx][i], senone_scores,
                                   senone_active, n_senone_active);
        }
    }

    return 0;
}

static int
split_topn(const char *str, uint8 *out, int nfeat)
{
    char *topn_list = ckd_salloc(str);
    char *c, *cc;
    int i, maxn;

    c = topn_list;
    i = 0;
    maxn = 0;
    while (i < nfeat && (cc = strchr(c, ',')) != NULL) {
        *cc = '\0';
        out[i] = atoi(c);
        if (out[i] > maxn)
            maxn = out[i];
        c = cc + 1;
        ++i;
    }
    if (i < nfeat && *c != '\0') {
        out[i] = atoi(c);
        if (out[i] > maxn)
            maxn = out[i];
        ++i;
    }
    while (i < nfeat)
        out[i++] = maxn;

    ckd_free(topn_list);
    return maxn;
}

mgau_t *
s2_semi_mgau_init_s3file(acmod_t *acmod, s3file_t *means, s3file_t *vars,
                         s3file_t *mixw, s3file_t *sendump)
{
    s2_semi_mgau_t *s;
    mgau_t *ps;
    int i;
    int n_feat;

    s = ckd_calloc(1, sizeof(*s));
    s->config = acmod->config;

    s->lmath = logmath_retain(acmod->lmath);
    /* Log-add table. */
    s->lmath_8b = logmath_init(logmath_get_base(acmod->lmath), SENSCR_SHIFT, TRUE);
    if (s->lmath_8b == NULL)
        goto error_out;
    /* Ensure that it is only 8 bits wide so that fast_logmath_add() works. */
    if (logmath_get_width(s->lmath_8b) != 1) {
        E_ERROR("Log base %f is too small to represent add table in 8 bits\n",
                logmath_get_base(s->lmath_8b));
        goto error_out;
    }

    /* Read means and variances. */
    if ((s->g = gauden_init_s3file(means, vars,
                                   config_float(s->config, "varfloor"),
                                   s->lmath))
        == NULL) {
        E_ERROR("Failed to read means and variances\n");
        goto error_out;
    }

    /* Currently only a single codebook is supported. */
    if (s->g->n_mgau != 1)
        goto error_out;

    n_feat = s->g->n_feat;

    /* Verify n_feat and veclen, against acmod. */
    if (n_feat != feat_dimension1(acmod->fcb)) {
        E_ERROR("Number of streams does not match: %d != %d\n",
                n_feat, feat_dimension1(acmod->fcb));
        goto error_out;
    }
    for (i = 0; i < n_feat; ++i) {
        if ((uint32)s->g->featlen[i] != feat_dimension2(acmod->fcb, i)) {
            E_ERROR("Dimension of stream %d does not match: %d != %d\n",
                    i, s->g->featlen[i], feat_dimension2(acmod->fcb, i));
            goto error_out;
        }
    }
    /* Read mixture weights */
    if (sendump) {
        s->n_sen = bin_mdef_n_sen(acmod->mdef);
        if (read_sendump(sendump, s->g, s->n_sen,
                         &s->mixw_cb, &s->mixw)
            < 0)
            goto error_out;
        s->sendump_mmap = s3file_retain(sendump);
    } else {
        float32 mixw_floor = config_float(s->config, "mixwfloor");
        if (read_mixw(mixw, s->g, s->lmath_8b, &s->n_sen, &s->mixw, mixw_floor) < 0)
            goto error_out;
    }
    s->ds_ratio = config_int(s->config, "ds");

    /* Determine top-N for each feature */
    s->topn_beam = ckd_calloc(n_feat, sizeof(*s->topn_beam));
    s->max_topn = config_int(s->config, "topn");
    split_topn(config_str(s->config, "topn_beam"), s->topn_beam, n_feat);
    E_INFO("Maximum top-N: %d ", s->max_topn);
    E_INFOCONT("Top-N beams:");
    for (i = 0; i < n_feat; ++i) {
        E_INFOCONT(" %d", s->topn_beam[i]);
    }
    E_INFOCONT("\n");

    /* Top-N scores from recent frames */
    s->n_topn_hist = 2;
    s->topn_hist = (vqFeature_t ***)
        ckd_calloc_3d(s->n_topn_hist, n_feat, s->max_topn,
                      sizeof(***s->topn_hist));
    s->topn_hist_n = ckd_calloc_2d(s->n_topn_hist, n_feat,
                                   sizeof(**s->topn_hist_n));
    for (i = 0; i < s->n_topn_hist; ++i) {
        int j;
        for (j = 0; j < n_feat; ++j) {
            int k;
            for (k = 0; k < s->max_topn; ++k) {
                s->topn_hist[i][j][k].score = WORST_DIST;
                s->topn_hist[i][j][k].codeword = k;
            }
        }
    }

    ps = (mgau_t *)s;
    ps->vt = &s2_semi_mgau_funcs;
    return ps;
error_out:
    s2_semi_mgau_free(ps_mgau_base(s));
    return NULL;
}

/* FIXME: This is identical to ptm_mgau_init */
mgau_t *
s2_semi_mgau_init(acmod_t *acmod)
{
    s3file_t *means = NULL;
    s3file_t *vars = NULL;
    s3file_t *mixw = NULL;
    s3file_t *sendump = NULL;
    const char *path;
    mgau_t *ps = NULL;

    path = config_str(acmod->config, "mean");
    E_INFO("Reading mixture gaussian parameter: %s\n", path);
    if ((means = s3file_map_file(path)) == NULL) {
        E_ERROR_SYSTEM("Failed to open mean file '%s' for reading", path);
        goto error_out;
    }
    path = config_str(acmod->config, "var");
    E_INFO("Reading mixture gaussian parameter: %s\n", path);
    if ((vars = s3file_map_file(path)) == NULL) {
        E_ERROR_SYSTEM("Failed to open variance file '%s' for reading", path);
        goto error_out;
    }
    if ((path = config_str(acmod->config, "sendump"))) {
        E_INFO("Loading senones from dump file %s\n", path);
        if ((sendump = s3file_map_file(path)) == NULL) {
            E_ERROR_SYSTEM("Failed to open sendump '%s' for reading", path);
            goto error_out;
        }
    } else {
        path = config_str(acmod->config, "mixw");
        E_INFO("Reading senone mixture weights: %s\n", path);
        if ((mixw = s3file_map_file(path)) == NULL) {
            E_ERROR_SYSTEM("Failed to open mixture weights '%s' for reading",
                           path);
            goto error_out;
        }
    }

    ps = s2_semi_mgau_init_s3file(acmod, means, vars, mixw, sendump);
error_out:
    s3file_free(means);
    s3file_free(vars);
    s3file_free(mixw);
    s3file_free(sendump);
    return ps;
}

int
s2_semi_mgau_mllr_transform(mgau_t *ps,
                            mllr_t *mllr)
{
    s2_semi_mgau_t *s = (s2_semi_mgau_t *)ps;
    return gauden_mllr_transform(s->g, mllr, s->config);
}

void
s2_semi_mgau_free(mgau_t *ps)
{
    s2_semi_mgau_t *s = (s2_semi_mgau_t *)ps;

    logmath_free(s->lmath);
    logmath_free(s->lmath_8b);
    if (s->sendump_mmap) {
        ckd_free_2d(s->mixw);
        s3file_free(s->sendump_mmap);
    } else {
        ckd_free_3d(s->mixw);
        if (s->mixw_cb)
            ckd_free(s->mixw_cb);
    }
    gauden_free(s->g);
    ckd_free(s->topn_beam);
    ckd_free_2d(s->topn_hist_n);
    ckd_free_3d((void **)s->topn_hist);
    ckd_free(s);
}

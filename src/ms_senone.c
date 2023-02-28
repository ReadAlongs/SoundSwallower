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

/* System headers. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <soundswallower/ms_senone.h>

#define MIXW_PARAM_VERSION "1.0"
#define SPDEF_PARAM_VERSION "1.2"

static int
senone_mgau_map_read(senone_t *s, s3file_t *s3f)
{
    int n_gauden_present = 0;
    int32 i;
    void *ptr;

    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (s3file_parse_header(s3f, SPDEF_PARAM_VERSION) < 0) {
        E_ERROR("Failed to read senmgau header\n");
        return -1;
    }
    /* set n_gauden_present based on version. */
    for (i = 0; (size_t)i < s3f->nhdr; i++) {
        if (s3file_header_name_is(s3f, i, "version")) {
            char *version = s3file_copy_header_value(s3f, i);
            float v = (float)atof(version);
            n_gauden_present = (v > 1.1) ? 1 : 0;
            ckd_free(version);
        }
    }

    /* Read #gauden (if version matches) */
    if (n_gauden_present) {
        E_INFO("Reading number of codebooks\n");
        if (s3file_get(&(s->n_gauden), sizeof(int32), 1, s3f) != 1)
            E_ERROR("read (#gauden) failed\n");
        return -1;
    }

    /* Read 1d array data */
    if (s3file_get_1d(&ptr, sizeof(uint32), &(s->n_sen), s3f) < 0) {
        E_ERROR("s3file_get_1d failed\n");
        return -1;
    }
    s->mgau = ptr;
    E_INFO("Mapping %d senones to %d codebooks\n", s->n_sen, s->n_gauden);

    /* Infer n_gauden if not present in this version */
    if (!n_gauden_present) {
        s->n_gauden = 1;
        for (i = 0; (uint32)i < s->n_sen; i++)
            if (s->mgau[i] >= s->n_gauden)
                s->n_gauden = s->mgau[i] + 1;
    }

    if (s3file_verify_chksum(s3f) < 0)
        return -1;

    E_INFO("Read %d->%d senone-codebook mappings\n", s->n_sen,
           s->n_gauden);

    return 0;
}

static int
senone_mixw_read(senone_t *s, s3file_t *s3f, logmath_t *lmath)
{
    float32 *pdf = NULL;
    int32 i, f, c, p, n_err;

    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (s3file_parse_header(s3f, MIXW_PARAM_VERSION) < 0)
        return -1;

    /* Read #senones, #features, #codewords, arraysize */
    if ((s3file_get(&(s->n_sen), sizeof(int32), 1, s3f) != 1)
        || (s3file_get(&(s->n_feat), sizeof(int32), 1, s3f)
            != 1)
        || (s3file_get(&(s->n_cw), sizeof(int32), 1, s3f)
            != 1)
        || (s3file_get(&i, sizeof(int32), 1, s3f) != 1)) {
        E_ERROR("s3file_get (arraysize) failed\n");
        return -1;
    }
    if ((uint32)i != s->n_sen * s->n_feat * s->n_cw) {
        E_ERROR("#float32s(%d) doesn't match dimensions: %d x %d x %d\n",
                i, s->n_sen, s->n_feat, s->n_cw);
        return -1;
    }

    /*
     * Compute #LSB bits to be dropped to represent mixwfloor with 8 bits.
     * All PDF values will be truncated (in the LSB positions) by these many bits.
     */
    if ((s->mixwfloor <= 0.0) || (s->mixwfloor >= 1.0)) {
        E_ERROR("mixwfloor (%e) not in range (0, 1)\n", s->mixwfloor);
        return -1;
    }

    /* Use a fixed shift for compatibility with everything else. */
    E_INFO("Truncating senone logs3(pdf) values by %d bits\n", SENSCR_SHIFT);

    /*
     * Allocate memory for senone PDF data.  Organize normally or transposed depending on
     * s->n_gauden.
     */
    if (s->n_gauden > 1) {
        E_INFO("Not transposing mixture weights in memory\n");
        s->pdf = (senprob_t ***)ckd_calloc_3d(s->n_sen, s->n_feat, s->n_cw,
                                              sizeof(senprob_t));
    } else {
        E_INFO("Transposing mixture weights in memory\n");
        s->pdf = (senprob_t ***)ckd_calloc_3d(s->n_feat, s->n_cw, s->n_sen,
                                              sizeof(senprob_t));
    }

    /* Temporary structure to read in floats */
    pdf = (float32 *)ckd_calloc(s->n_cw, sizeof(float32));

    /* Read senone probs data, normalize, floor, convert to logs3, truncate to 8 bits */
    n_err = 0;
    for (i = 0; (uint32)i < s->n_sen; i++) {
        for (f = 0; (uint32)f < s->n_feat; f++) {
            if (s3file_get((void *)pdf, sizeof(float32), s->n_cw, s3f)
                != (size_t)s->n_cw) {
                E_ERROR("s3file_get (arraydata) failed\n");
                return -1;
            }

            /* Normalize and floor */
            if (vector_sum_norm(pdf, s->n_cw) <= 0.0)
                n_err++;
            vector_floor(pdf, s->n_cw, s->mixwfloor);
            vector_sum_norm(pdf, s->n_cw);

            /* Convert to logs3, truncate to 8 bits, and store in s->pdf */
            for (c = 0; (uint32)c < s->n_cw; c++) {
                p = -(logmath_log(lmath, pdf[c]));
                p += (1 << (SENSCR_SHIFT - 1)) - 1; /* Rounding before truncation */

                if (s->n_gauden > 1)
                    s->pdf[i][f][c] = (p < (255 << SENSCR_SHIFT)) ? (p >> SENSCR_SHIFT) : 255;
                else
                    s->pdf[f][c][i] = (p < (255 << SENSCR_SHIFT)) ? (p >> SENSCR_SHIFT) : 255;
            }
        }
    }
    if (n_err > 0)
        E_WARN("Weight normalization failed for %d mixture weights components\n", n_err);
    if (s3file_verify_chksum(s3f) < 0)
        goto error_out;
    E_INFO("Read mixture weights for %d senones: %d features x %d codewords\n",
           s->n_sen, s->n_feat, s->n_cw);
    return 0;

error_out:
    if (pdf)
        ckd_free(pdf);
    return -1;
}

senone_t *
senone_init_s3file(gauden_t *g,
                   s3file_t *mixwfile,
                   s3file_t *mgau_mapfile,
                   float32 mixwfloor,
                   logmath_t *lmath,
                   bin_mdef_t *mdef)
{
    senone_t *s;
    int32 i;

    s = (senone_t *)ckd_calloc(1, sizeof(senone_t));
    s->lmath = logmath_init(logmath_get_base(lmath), SENSCR_SHIFT, TRUE);
    s->mixwfloor = mixwfloor;

    s->n_gauden = g->n_mgau;
    if (mgau_mapfile) {
        if (senone_mgau_map_read(s, mgau_mapfile) < 0) {
            goto error_out;
        }
    }

    if (senone_mixw_read(s, mixwfile, lmath) < 0)
        goto error_out;

    if (!mgau_mapfile) {
        if (s->n_gauden == 1) {
            /* sen2mgau_map_file = ".semi."; */
            /* All-to-1 senones-codebook mapping */
            E_INFO("Mapping all senones to one codebook\n");
            s->mgau = (uint32 *)ckd_calloc(s->n_sen, sizeof(*s->mgau));
        } else if (s->n_gauden == (uint32)bin_mdef_n_ciphone(mdef)) {
            /* sen2mgau_map_file = ".ptm."; */
            /* All-to-ciphone-id senones-codebook mapping */
            E_INFO("Mapping senones to context-independent phone codebooks\n");
            s->mgau = (uint32 *)ckd_calloc(s->n_sen, sizeof(*s->mgau));
            for (i = 0; (uint32)i < s->n_sen; i++)
                s->mgau[i] = bin_mdef_sen2cimap(mdef, i);
        } else {
            /* sen2mgau_map_file = ".cont."; */
            /* 1-to-1 senone-codebook mapping */
            E_INFO("Mapping senones to individual codebooks\n");
            if (s->n_sen <= 1)
                E_FATAL("#senone=%d; must be >1\n", s->n_sen);

            s->mgau = (uint32 *)ckd_calloc(s->n_sen, sizeof(*s->mgau));
            for (i = 0; (uint32)i < s->n_sen; i++)
                s->mgau[i] = i;
            /* Not sure why this is here, it probably does nothing. */
            s->n_gauden = s->n_sen;
        }
    }

    s->featscr = NULL;
    return s;

error_out:
    senone_free(s);
    return NULL;
}

senone_t *
senone_init(gauden_t *g, const char *mixwfile, const char *sen2mgau_map_file,
            float32 mixwfloor, logmath_t *lmath, bin_mdef_t *mdef)
{
    s3file_t *senmgau = NULL;
    s3file_t *mixw = NULL;
    senone_t *s = NULL;

    if (sen2mgau_map_file) {
        if (!(strcmp(sen2mgau_map_file, ".semi.") == 0
              || strcmp(sen2mgau_map_file, ".ptm.") == 0
              || strcmp(sen2mgau_map_file, ".cont.") == 0)) {
            E_INFO("Reading senone to gmm mapping: %s\n", sen2mgau_map_file);
            if ((senmgau = s3file_map_file(sen2mgau_map_file)) == NULL) {
                E_ERROR_SYSTEM("Failed to open senmgau '%s' for reading",
                               sen2mgau_map_file);
                goto error_out;
            }
        }
    }
    E_INFO("Reading senone mixture weights: %s\n", mixwfile);
    if ((mixw = s3file_map_file(mixwfile)) == NULL) {
        E_ERROR_SYSTEM("Failed to open mixture weights file '%s' for reading",
                       mixwfile);
        goto error_out;
    }
    s = senone_init_s3file(g, mixw, senmgau, mixwfloor, lmath, mdef);
error_out:
    s3file_free(senmgau);
    s3file_free(mixw);
    return s;
}

void
senone_free(senone_t *s)
{
    if (s == NULL)
        return;
    if (s->pdf)
        ckd_free_3d((void *)s->pdf);
    if (s->mgau)
        ckd_free(s->mgau);
    if (s->featscr)
        ckd_free(s->featscr);
    logmath_free(s->lmath);
    ckd_free(s);
}

/*
 * Compute senone score for one senone.
 * NOTE:  Remember that senone PDF tables contain SCALED, NEGATED logs3 values.
 * NOTE:  Remember also that PDF data may be transposed or not depending on s->n_gauden.
 */
int32
senone_eval(senone_t *s, int id, gauden_dist_t **dist, int32 n_top)
{
    int32 scr; /* total senone score */
    int32 fden; /* Gaussian density */
    int32 fscr; /* senone score for one feature */
    int32 fwscr; /* senone score for one feature, one codeword */
    int32 f, t;
    gauden_dist_t *fdist;

    assert((id >= 0) && ((uint32)id < s->n_sen));
    assert((n_top > 0) && ((uint32)n_top <= s->n_cw));

    scr = 0;

    for (f = 0; (uint32)f < s->n_feat; f++) {
        fdist = dist[f];

        if (fdist[0].dist < (mfcc_t)MAX_NEG_INT32)
            fden = MAX_NEG_INT32 >> SENSCR_SHIFT;
        else
            fden = ((int32)fdist[0].dist + ((1 << SENSCR_SHIFT) - 1)) >> SENSCR_SHIFT;
        fscr = (s->n_gauden > 1)
            ? (fden + -s->pdf[id][f][fdist[0].id]) /* untransposed */
            : (fden + -s->pdf[f][fdist[0].id][id]); /* transposed */
        /* Remaining of n_top codewords for feature f */
        for (t = 1; t < n_top; t++) {
            if (fdist[t].dist < (mfcc_t)MAX_NEG_INT32)
                fden = MAX_NEG_INT32 >> SENSCR_SHIFT;
            else
                fden = ((int32)fdist[t].dist + ((1 << SENSCR_SHIFT) - 1)) >> SENSCR_SHIFT;

            fwscr = (s->n_gauden > 1) ? (fden + -s->pdf[id][f][fdist[t].id]) : (fden + -s->pdf[f][fdist[t].id][id]);
            fscr = logmath_add(s->lmath, fscr, fwscr);
        }
        /* Senone scores are also scaled, negated logs3 values.  Hence
         * we have to negate the stuff we calculated above. */
        scr -= fscr;
    }
    /* Downscale scores. */
    scr /= s->aw;

    /* Avoid overflowing int16 */
    if (scr > 32767)
        scr = 32767;
    if (scr < -32768)
        scr = -32768;
    return scr;
}

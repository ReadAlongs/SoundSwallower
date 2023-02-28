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
 * @file mllr.h Model-space linear transforms for speaker adaptation
 */

#ifndef __PS_MLLR_H__
#define __PS_MLLR_H__

#include <soundswallower/prim_type.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * Feature space linear transform object.
 */
typedef struct mllr_s mllr_t;
struct mllr_s {
    int refcnt; /**< Reference count. */
    int n_class; /**< Number of MLLR classes. */
    int n_feat; /**< Number of feature streams. */
    int *veclen; /**< Length of input vectors for each stream. */
    float32 ****A; /**< Rotation part of mean transformations. */
    float32 ***b; /**< Bias part of mean transformations. */
    float32 ***h; /**< Diagonal transformation of variances. */
    int32 *cb2mllr; /**< Mapping from codebooks to transformations. */
};

/**
 * Read a speaker-adaptive linear transform from a file.
 */
mllr_t *mllr_read(const char *file);

/**
 * Retain a pointer to a linear transform.
 */
mllr_t *mllr_retain(mllr_t *mllr);

/**
 * Release a pointer to a linear transform.
 */
int mllr_free(mllr_t *mllr);

#ifdef __cplusplus
}
#endif

#endif /* __PS_MLLR_H__ */

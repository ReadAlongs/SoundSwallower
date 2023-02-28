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

#ifndef _S3_TMAT_H_
#define _S3_TMAT_H_

#include <soundswallower/logmath.h>
#include <soundswallower/s3file.h>
#include <stdio.h>

/** \file tmat.h
 *  \brief Transition matrix data structure.
 */
#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * \struct tmat_t
 * \brief Transition matrix data structure.  All phone HMMs are assumed to have the same
 * topology.
 */
typedef struct tmat_s {
    uint8 ***tp; /**< The transition matrices; kept in the same scale as acoustic scores;
                    tp[tmatid][from-state][to-state] */
    int16 n_tmat; /**< Number matrices */
    int16 n_state; /**< Number source states in matrix (only the emitting states);
                      Number destination states = n_state+1, it includes the exit state */
} tmat_t;

/** Initialize transition matrix */

tmat_t *tmat_init(const char *tmatfile, /**< In: input file */
                  logmath_t *lmath, /**< In: log math parameters */
                  float64 tpfloor /**< In: floor value for each non-zero transition probability */
);

/**
 * Initialize transition matrix from existing s3file_t.
 */
tmat_t *tmat_init_s3file(s3file_t *s, logmath_t *lmath, float64 tpfloor);

/**
 * RAH, add code to remove memory allocated by tmat_init
 */

void tmat_free(tmat_t *t /**< In: transition matrix */
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2007 Carnegie Mellon University.  All rights
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

#include <assert.h>
#include <math.h>
#include <string.h>

#include <soundswallower/ckd_alloc.h>
#include <soundswallower/err.h>
#include <soundswallower/logmath.h>
#include <soundswallower/mmio.h>
#include <soundswallower/strfuncs.h>

struct logmath_s {
    logadd_t t;
    int refcount;
    mmio_file_t *filemap;
    float64 base;
    float64 log_of_base;
    float64 log10_of_base;
    float64 inv_log_of_base;
    float64 inv_log10_of_base;
    int32 zero;
};

logmath_t *
logmath_init(float64 base, int shift, int use_table)
{
    logmath_t *lmath;
    uint32 maxyx, i;
    float64 byx;
    int width;

    /* Check that the base is correct. */
    if (base <= 1.0) {
        E_ERROR("Base must be greater than 1.0\n");
        return NULL;
    }

    /* Set up various necessary constants. */
    lmath = ckd_calloc(1, sizeof(*lmath));
    lmath->refcount = 1;
    lmath->base = base;
    lmath->log_of_base = log(base);
    lmath->log10_of_base = log10(base);
    lmath->inv_log_of_base = 1.0 / lmath->log_of_base;
    lmath->inv_log10_of_base = 1.0 / lmath->log10_of_base;
    lmath->t.shift = shift;
    /* Shift this sufficiently that overflows can be avoided. */
    lmath->zero = MAX_NEG_INT32 >> (shift + 2);

    if (!use_table)
        return lmath;

    /* Create a logadd table with the appropriate width */
    maxyx = (uint32)(log(2.0) / log(base) + 0.5) >> shift;
    /* Poor man's log2 */
    if (maxyx < 256)
        width = 1;
    else if (maxyx < 65536)
        width = 2;
    else
        width = 4;

    lmath->t.width = width;
    /* Figure out size of add table required. */
    byx = 1.0; /* Maximum possible base^{y-x} value - note that this implies that y-x == 0 */
    for (i = 0;; ++i) {
        float64 lobyx = log(1.0 + byx) * lmath->inv_log_of_base; /* log_{base}(1 + base^{y-x}); */
        int32 k = (int32)(lobyx + 0.5 * (1 << shift)) >> shift; /* Round to shift */

        /* base^{y-x} has reached the smallest representable value. */
        if (k <= 0)
            break;

        /* This table is indexed by -(y-x), so we multiply byx by
         * base^{-1} here which is equivalent to subtracting one from
         * (y-x). */
        byx /= base;
    }
    i >>= shift;

    /* Never produce a table smaller than 256 entries. */
    if (i < 255)
        i = 255;

    lmath->t.table = ckd_calloc(i + 1, width);
    lmath->t.table_size = i + 1;
    /* Create the add table (see above). */
    byx = 1.0;
    for (i = 0;; ++i) {
        float64 lobyx = log(1.0 + byx) * lmath->inv_log_of_base;
        int32 k = (int32)(lobyx + 0.5 * (1 << shift)) >> shift; /* Round to shift */
        uint32 prev = 0;

        /* Check any previous value - if there is a shift, we want to
         * only store the highest one. */
        switch (width) {
        case 1:
            prev = ((uint8 *)lmath->t.table)[i >> shift];
            break;
        case 2:
            prev = ((uint16 *)lmath->t.table)[i >> shift];
            break;
        case 4:
            prev = ((uint32 *)lmath->t.table)[i >> shift];
            break;
        }
        if (prev == 0) {
            switch (width) {
            case 1:
                ((uint8 *)lmath->t.table)[i >> shift] = (uint8)k;
                break;
            case 2:
                ((uint16 *)lmath->t.table)[i >> shift] = (uint16)k;
                break;
            case 4:
                ((uint32 *)lmath->t.table)[i >> shift] = (uint32)k;
                break;
            }
        }
        if (k <= 0)
            break;

        /* Decay base^{y-x} exponentially according to base. */
        byx /= base;
    }

    return lmath;
}

logmath_t *
logmath_retain(logmath_t *lmath)
{
    if (lmath == NULL)
        return NULL;
    ++lmath->refcount;
    return lmath;
}

int
logmath_free(logmath_t *lmath)
{
    if (lmath == NULL)
        return 0;
    if (--lmath->refcount > 0)
        return lmath->refcount;
    if (lmath->filemap)
        mmio_file_unmap(lmath->filemap);
    else
        ckd_free(lmath->t.table);
    ckd_free(lmath);
    return 0;
}

int32
logmath_get_table_shape(logmath_t *lmath, uint32 *out_size,
                        uint32 *out_width, uint32 *out_shift)
{
    if (out_size)
        *out_size = lmath->t.table_size;
    if (out_width)
        *out_width = lmath->t.width;
    if (out_shift)
        *out_shift = lmath->t.shift;

    return lmath->t.table_size * lmath->t.width;
}

float64
logmath_get_base(logmath_t *lmath)
{
    return lmath->base;
}

int
logmath_get_zero(logmath_t *lmath)
{
    return lmath->zero;
}

int
logmath_get_width(logmath_t *lmath)
{
    return lmath->t.width;
}

int
logmath_get_shift(logmath_t *lmath)
{
    return lmath->t.shift;
}

int
logmath_add(logmath_t *lmath, int logb_x, int logb_y)
{
    logadd_t *t = LOGMATH_TABLE(lmath);
    int d, r;

    /* handle 0 + x = x case. */
    if (logb_x <= lmath->zero)
        return logb_y;
    if (logb_y <= lmath->zero)
        return logb_x;

    if (t->table == NULL)
        return logmath_add_exact(lmath, logb_x, logb_y);

    /* d must be positive, obviously. */
    if (logb_x > logb_y) {
        d = (logb_x - logb_y);
        r = logb_x;
    } else {
        d = (logb_y - logb_x);
        r = logb_y;
    }

    if (d < 0) {
        /* Some kind of overflow has occurred, fail gracefully. */
        return r;
    }
    if ((size_t)d >= t->table_size) {
        /* If this happens, it's not actually an error, because the
         * last entry in the logadd table is guaranteed to be zero.
         * Therefore we just return the larger of the two values. */
        return r;
    }

    switch (t->width) {
    case 1:
        return r + (((uint8 *)t->table)[d]);
    case 2:
        return r + (((uint16 *)t->table)[d]);
    case 4:
        return r + (((uint32 *)t->table)[d]);
    }
    return r;
}

int
logmath_add_exact(logmath_t *lmath, int logb_p, int logb_q)
{
    return logmath_log(lmath,
                       logmath_exp(lmath, logb_p)
                           + logmath_exp(lmath, logb_q));
}

int
logmath_log(logmath_t *lmath, float64 p)
{
    if (p <= 0) {
        return lmath->zero;
    }
    return (int)(log(p) * lmath->inv_log_of_base) >> lmath->t.shift;
}

float64
logmath_exp(logmath_t *lmath, int logb_p)
{
    return pow(lmath->base, (float64)(logb_p << lmath->t.shift));
}

int
logmath_ln_to_log(logmath_t *lmath, float64 log_p)
{
    return (int)(log_p * lmath->inv_log_of_base) >> lmath->t.shift;
}

float64
logmath_log_to_ln(logmath_t *lmath, int logb_p)
{
    return (float64)(logb_p << lmath->t.shift) * lmath->log_of_base;
}

int
logmath_log10_to_log(logmath_t *lmath, float64 log_p)
{
    return (int)(log_p * lmath->inv_log10_of_base) >> lmath->t.shift;
}

float
logmath_log10_to_log_float(logmath_t *lmath, float64 log_p)
{
    int i;
    float res = (float)(log_p * lmath->inv_log10_of_base);
    for (i = 0; i < lmath->t.shift; i++)
        res /= 2.0f;
    return res;
}

float64
logmath_log_to_log10(logmath_t *lmath, int logb_p)
{
    return (float64)(logb_p << lmath->t.shift) * lmath->log10_of_base;
}

float64
logmath_log_float_to_log10(logmath_t *lmath, float log_p)
{
    int i;
    for (i = 0; i < lmath->t.shift; i++) {
        log_p *= 2;
    }
    return log_p * lmath->log10_of_base;
}

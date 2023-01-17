/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2022 Carnegie Mellon University.  All rights
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
 * s3file.h -- Sphinx-3 binary file parsing in memory
 *
 * loosely based on bio.h:
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 */

#ifndef _S3FILE_H_
#define _S3FILE_H_

#include <soundswallower/byteorder.h>
#include <soundswallower/mmio.h>
#include <soundswallower/prim_type.h>

#include <stddef.h>

/** \file s3file.h
 * \brief Functions to read sphinx3 files from memory.

 */

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

typedef struct s3hdr_s {
    struct {
        const char *buf;
        size_t len;
    } name, value;
} s3hdr_t;

typedef struct s3file_s {
    int refcount;
    mmio_file_t *mf;
    const void *buf;
    const char *ptr, *end;
    s3hdr_t *headers;
    size_t nhdr;
    int do_swap;
    int do_chksum;
    uint32 chksum;
} s3file_t;

/**
 * Initialize an s3file_t from memory
 */
s3file_t *s3file_init(const void *buf, size_t len);

/**
 * Initialize an s3file_t by memory-mapping (or reading) a file.
 */
s3file_t *s3file_map_file(const char *filename);

/**
 * Retain s3file_t.
 */
s3file_t *s3file_retain(s3file_t *s);

/**
 * Release s3file_t, freeing and unmapping if refcnt=0.
 */
int s3file_free(s3file_t *s);

/**
 * Rewind and reset an s3file_t to its initial state.
 */
void s3file_rewind(s3file_t *s);

/**
 * Read binary file format header: has the following format
 * <pre>
 *     s3
 *     <argument-name> <argument-value>
 *     <argument-name> <argument-value>
 *     ...
 *     endhdr
 *     4-byte byte-order word used to find file byte ordering relative to host machine.
 * </pre>
 * Lines beginning with # are ignored.
 * Checksum enabled if "chksum0" header present.
 * Version checked with warning (not fatal) if mismatch.
 * @param version Optional version to check against.
 * @return 0 if successful, -1 otherwise.
 */
int s3file_parse_header(s3file_t *s, const char *version);

/**
 * Compare argument name in place.
 */
int s3file_header_name_is(s3file_t *s, size_t idx, const char *name);

/**
 * Compare argument value in place.
 */
int s3file_header_value_is(s3file_t *s, size_t idx, const char *value);

/**
 * Get copy of argument name.
 * @return pointer to copy, must be freed by user.
 */
char *s3file_copy_header_name(s3file_t *s, size_t idx);

/**
 * Get copy of argument value.
 * @return pointer to copy, must be freed by user.
 */
char *s3file_copy_header_value(s3file_t *s, size_t idx);

/**
 * Advance one line in file.
 * @return start of current line (end is in s->ptr) or NULL if at end-of-file.
 */
const char *s3file_nextline(s3file_t *s);

/**
 * Advance one whitespace-separated "word" in file.
 *
 * @param ptr Optional pointer within line as returned by
 * s3file_nextline.  If not NULL then it will be advanced on output,
 * up to s->ptr, assumed to be end-of-line, at which point the return
 * value will be NULL.  Otherwise, words will be scanned until s->end
 * (end-of-file)
 *
 * @return start of current word (end is in *ptr or s->ptr) or NULL if
 * at end-of-line or end-of-file.
 */
const char *s3file_nextword(s3file_t *s, const char **ptr);

/**
 * Convenience function to extract and copy the next word.
 * @return pointer to copy, must be freed by user.
 */
char *s3file_copy_nextword(s3file_t *s, const char **ptr);

/**
 * Extract values with byteswapping and checksum.
 */
size_t s3file_get(void *buf, /**< In: adddress to write values to. */
                  size_t el_sz, /**< In: element size */
                  size_t n_el, /**< In: number of elements */
                  s3file_t *s);

/**
 * Read a 1-d array (fashioned after fread):
 *
 *  - 4-byte array size (returned in n_el)
 *  - memory allocated for the array and read (returned in buf)
 *
 * Byteswapping and checksum accumulation performed as necessary.
 * @return number of array elements allocated and read; -1 if error.
 */
long s3file_get_1d(void **buf, /**< Out: contains array data; allocated by this
                                  function; can be freed using ckd_free */
                   size_t el_sz, /**< In: Array element size */
                   uint32 *n_el, /**< Out: Number of array elements allocated/read */
                   s3file_t *s);

/**
 * Read a 2-d matrix:
 *
 * - 4-byte # rows, # columns (returned in d1, d2, d3)
 * - memory allocated for the array and read (returned in buf)
 *
 * Byteswapping and checksum accumulation performed as necessary.
 * @return number of array elements allocated and read
 */
long s3file_get_2d(void ***arr,
                   size_t e_sz,
                   uint32 *d1,
                   uint32 *d2,
                   s3file_t *s);

/**
 * Read a 3-d array (set of matrices)
 *
 * - 4-byte # matrices, # rows, # columns (returned in d1, d2, d3)
 * - memory allocated for the array and read (returned in buf)
 *
 * Byteswapping and checksum accumulation performed as necessary.
 * @return number of array elements allocated and read
 */
long s3file_get_3d(void ****arr,
                   size_t e_sz,
                   uint32 *d1,
                   uint32 *d2,
                   uint32 *d3,
                   s3file_t *s);

/**
 * Read and verify checksum at the end of binary file.  Returns 0 for
 * success or -1 on a mismatch.
 */
int s3file_verify_chksum(s3file_t *s);

#ifdef __cplusplus
}
#endif

#endif

/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1995-2004 Carnegie Mellon University.  All rights
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
 * @file strfuncs.h
 * @brief Miscellaneous useful string functions
 */

#ifndef __SB_STRFUNCS_H__
#define __SB_STRFUNCS_H__

#include <stdarg.h>

#include <soundswallower/prim_type.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

/**
 * Concatenate a NULL-terminated argument list of strings, returning a
 * newly allocated string.
 **/
char *string_join(const char *base, ...);

/**
 * Locale-independent isspace to avoid incompatibilities
 */
int isspace_c(char ch);

/**
 * Which end of a string to operate on for string_trim().
 */
enum string_edge_e {
    STRING_START, /**< Beginning of string. */
    STRING_END, /**< End of string. */
    STRING_BOTH /**< Both ends of string. */
};

/**
 * Remove whitespace from a string, modifying it in-place.
 *
 * @param string string to trim, contents will be modified.
 * @param which one of STRING_START, STRING_END, or STRING_BOTH.
 */
char *string_trim(char *string, enum string_edge_e which);

/**
 * Crappy string-splitting function.
 */
int32 nextword(char *line, const char *delim, char **word, char *delimfound);

#ifdef __cplusplus
}
#endif

#endif /* __SB_STRFUNCS_H__ */

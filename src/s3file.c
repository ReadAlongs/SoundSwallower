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
 * s3file.c -- Sphinx-3 binary file parsing in memory.
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 */

#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <soundswallower/s3file.h>
#include <soundswallower/err.h>
#include <soundswallower/ckd_alloc.h>


#define BYTE_ORDER_MAGIC (0x11223344)
#define BIO_HDRARG_MAX	32
#define END_COMMENT	"*end_comment*\n"

s3file_t *
s3file_init(const void *buf, size_t len)
{
    s3file_t *s = ckd_calloc(1, sizeof(*s));
    s->refcnt = 1;
    s->buf = buf;
    s->ptr = buf;
    s->end = buf + len;
    return s;
}

s3file_t *
s3file_retain(s3file_t *s)
{
    if (s == NULL)
        return NULL;
    ++s->refcnt;
    return s;
}

int
s3file_free(s3file_t *s)
{
    int rv = --s->refcnt;
    assert(rv >= 0);
    if (rv == 0) {
        ckd_free(s);
    }
    return rv;
}

static int32
swap_check(s3file_t *s)
{
    uint32 magic;

    if (s3file_get(&magic, sizeof(uint32), 1, s) != 1) {
        E_ERROR("Cannot read BYTEORDER MAGIC NO.\n");
        return -1;
    }

    if (magic != BYTE_ORDER_MAGIC) {
        /* either need to swap or got bogus magic number */
        SWAP_INT32(&magic);

        if (magic == BYTE_ORDER_MAGIC)
            return 1;

        SWAP_INT32(&magic);
        E_ERROR("Bad BYTEORDER MAGIC NO: %08x, expecting %08x\n",
                magic, BYTE_ORDER_MAGIC);
        return -1;
    }

    return 0;
}

const char *
s3file_readline(s3file_t *s)
{
    const char *line;

    assert(s->ptr <= s->end);
    if (s->ptr == s->end)
        return NULL;
    for (line = s->ptr; s->ptr < s->end && *s->ptr != '\n'; ++s->ptr)
        ;
    if (s->ptr != s->end)
        ++s->ptr;
    return line;
}

int
s3file_parse_header(s3file_t *s)
{
    int lineno;
    const char *line;

    lineno = 0;
    if ((line = s3file_readline(s)) == NULL) {
        E_ERROR("Premature EOF, line %d\n", lineno);
        return -1;
    }
    lineno++;
    if (strncmp(line, "s3\n", 3) == 0) {
        /* New format (post Dec-1996, including checksums); read argument-value pairs */
        const char *start = s->ptr;
        int start_line;
        size_t i;

        start_line = lineno;
        while (1) {
            if ((line = s3file_readline(s)) == NULL) {
                E_ERROR("Premature EOF, line %d\n", lineno);
                return -1;
            }
            lineno++;
            while (isspace(*line) && line < s->ptr)
                line++;
            if (line == s->ptr) {
                E_ERROR("Missing header in line %d\n", lineno);
                return -1;
            }
            if (*line == '#')
                continue;
            if (strncmp(line, "endhdr", 6) == 0)
                break;
            s->nhdr++;
        }
        lineno = start_line;
        s->ptr = start;
        s->headers = ckd_calloc(s->nhdr, sizeof(*s->headers));
        i = 0;
        while (1) {
            if ((line = s3file_readline(s)) == NULL) {
                E_ERROR("Premature EOF, line %d\n", lineno);
                return -1;
            }
            lineno++;
            while (isspace(*line) && line < s->ptr)
                line++;
            if (line == s->ptr) {
                E_ERROR("Missing name in header line %d\n", lineno);
                return -1;
            }
            if (*line == '#')
                continue;
            if (strncmp(line, "endhdr", 6) == 0)
                break;

            s->headers[i].name.buf = line;
            while (!isspace(*line) && line < s->ptr)
                line++;
            s->headers[i].name.len = line - s->headers[i].name.buf;
            while (isspace(*line) && line < s->ptr)
                line++;
            if (line == s->ptr) {
                E_ERROR("Missing value in header line %d\n", lineno);
                return -1;
            }
            s->headers[i].value.buf = line;
            while (!isspace(*line) && line < s->ptr)
                line++;
            s->headers[i].value.len = line - s->headers[i].value.buf;
            E_INFO("header line %d: %.*s=%.*s\n", lineno,
                   s->headers[i].name.len,
                   s->headers[i].name.buf,
                   s->headers[i].value.len,
                   s->headers[i].value.buf);
            i++;
        }
    }
    else {
        static const char version_string[] = "version";
        /* Old format (without checksums); the first entry must be the version# */
        s->headers = ckd_calloc(1, sizeof(*s->headers));
        s->nhdr = 1;
        s->headers[0].name.buf = version_string;
        s->headers[0].name.len = strlen(version_string);
        s->headers[0].value.buf = line;
        s->headers[0].value.len = s->ptr - line;
        /* Just ignore everything until *end_comment* */
        while (1) {
            if ((line = s3file_readline(s)) == NULL) {
                E_ERROR("Premature EOF, line %d\n", lineno);
                return -1;
            }
            if (strncmp(line, END_COMMENT, s->ptr - line) == 0)
                break;
            lineno++;
        }
    }

    if ((s->swap = swap_check(s)) < 0) {
        E_ERROR("swap_check failed\n");
        return -1;
    }

    return 0;
}

int
s3file_header_name_is(s3file_t *s, size_t idx, const char *name)
{
    size_t len = strlen(name);
    if (len != s->headers[idx].name.len)
        return FALSE;
    return 0 == strncmp(name, s->headers[idx].name.buf, len);
}

int
s3file_header_value_is(s3file_t *s, size_t idx, const char *value)
{
    size_t len = strlen(value);
    if (len != s->headers[idx].value.len)
        return FALSE;
    return 0 == strncmp(value, s->headers[idx].value.buf, len);
}

char *
s3file_header_name(s3file_t *s, size_t idx)
{
    char *str = ckd_malloc(s->headers[idx].name.len + 1);
    memcpy(str, s->headers[idx].name.buf,  s->headers[idx].name.len);
    str[s->headers[idx].name.len] = '\0';
    return str;
}

char *
s3file_header_value(s3file_t *s, size_t idx)
{
    char *str = ckd_malloc(s->headers[idx].value.len + 1);
    memcpy(str, s->headers[idx].value.buf,  s->headers[idx].value.len);
    str[s->headers[idx].value.len] = '\0';
    return str;
}

size_t
s3file_get(void *buf,
           size_t el_sz,
           size_t n_el,
           s3file_t *s)
{
    size_t available = s->end - s->ptr;

    if (available < el_sz * n_el)
        n_el = available / el_sz;
    if (n_el == 0)
        return 0;
        
    memcpy(buf, s->ptr, el_sz * n_el);
    s->ptr += el_sz * n_el;
    /* special case for presumed numeric types... */
    if (s->swap) {
        size_t i;
        if (el_sz == 2) {
            for (i = 0; i < n_el; ++i)
                SWAP_INT16((uint16 *)buf + i);
        }
        else if (el_sz == 4) {
            for (i = 0; i < n_el; ++i)
                SWAP_INT32((uint32 *)buf + i);
        }
        else if (el_sz == 8) {
            for (i = 0; i < n_el; ++i)
                SWAP_FLOAT64((uint64 *)buf + i);
        }
    }
    return n_el;
}

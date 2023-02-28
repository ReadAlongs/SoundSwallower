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

#include <assert.h>
#include <ctype.h>
#include <string.h>

#include <soundswallower/ckd_alloc.h>
#include <soundswallower/err.h>
#include <soundswallower/s3file.h>
#include <soundswallower/strfuncs.h>

#define BYTE_ORDER_MAGIC (0x11223344)
#define BIO_HDRARG_MAX 32
#define END_COMMENT "*end_comment*\n"

s3file_t *
s3file_init(const void *buf, size_t len)
{
    s3file_t *s = ckd_calloc(1, sizeof(*s));
    s->refcount = 1;
    s->buf = buf;
    s->ptr = buf;
    s->end = (const char *)buf + len;

    return s;
}

s3file_t *
s3file_map_file(const char *filename)
{
    mmio_file_t *mf;

    if ((mf = mmio_file_read(filename)) == NULL)
        return NULL;
    s3file_t *s = ckd_calloc(1, sizeof(*s));
    s->refcount = 1;
    s->mf = mf;
    s->buf = mmio_file_ptr(mf);
    s->ptr = s->buf;
    s->end = (const char *)s->buf + mmio_file_size(mf);

    return s;
}

s3file_t *
s3file_retain(s3file_t *s)
{
    if (s == NULL)
        return NULL;
    ++s->refcount;
    return s;
}

int
s3file_free(s3file_t *s)
{
    if (s == NULL)
        return 0;
    if (--s->refcount > 0)
        return s->refcount;
    if (s->mf)
        mmio_file_unmap(s->mf);
    ckd_free(s->headers);
    ckd_free(s);
    return 0;
}

void
s3file_rewind(s3file_t *s)
{
    if (s == NULL)
        return;
    s->ptr = s->buf;
    ckd_free(s->headers);
    s->headers = NULL;
    s->nhdr = 0;
    s->do_swap = s->do_chksum = 0;
    s->chksum = 0;
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
s3file_nextline(s3file_t *s)
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

const char *
s3file_nextword(s3file_t *s, const char **ptr)
{
    const char *word, *end, *endspace;

    if (ptr == NULL) {
        ptr = &s->ptr;
        end = s->end;
    } else
        end = s->ptr;
    assert(*ptr <= end);
    if (*ptr == end)
        return NULL;
    while (*ptr < end && isspace_c(**ptr))
        ++*ptr;
    if (*ptr == end)
        return NULL;
    for (word = *ptr; *ptr < end && !isspace_c(**ptr); ++*ptr)
        /* Do nothing */;
    for (endspace = *ptr; endspace < end && isspace_c(*endspace); ++endspace)
        /* Do nothing */;
    return word;
}

char *
s3file_copy_nextword(s3file_t *s, const char **ptr)
{
    const char *word;
    size_t len;
    char *copy;
    if (ptr == NULL)
        ptr = &s->ptr;
    word = s3file_nextword(s, ptr);
    if (word == NULL)
        return NULL;
    len = *ptr - word;
    copy = ckd_malloc(len + 1);
    memcpy(copy, word, len);
    copy[len] = '\0';
    return copy;
}

int
s3file_parse_header(s3file_t *s, const char *version)
{
    int lineno, do_chksum = FALSE;
    const char *line;

    lineno = 0;
    if ((line = s3file_nextline(s)) == NULL) {
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
            const char *ptr, *word;
            if ((ptr = line = s3file_nextline(s)) == NULL) {
                E_ERROR("Premature EOF, line %d\n", lineno);
                return -1;
            }
            lineno++;

            word = s3file_nextword(s, &ptr);
            E_DEBUG("|%.*s|\n", ptr - word, word);
            if (word == NULL) {
                E_ERROR("Missing header in line %d\n", lineno);
                return -1;
            }
            if (*word == '#')
                continue;
            if (strncmp(word, "endhdr", ptr - word) == 0)
                break;
            s->nhdr++;
        }
        lineno = start_line;
        s->ptr = start;
        s->headers = ckd_calloc(s->nhdr, sizeof(*s->headers));
        i = 0;
        while (1) {
            const char *ptr, *word;
            if ((ptr = line = s3file_nextline(s)) == NULL) {
                E_ERROR("Premature EOF, line %d\n", lineno);
                return -1;
            }
            lineno++;
            word = s3file_nextword(s, &ptr);
            if (word == NULL) {
                E_ERROR("Missing name in header line %d\n", lineno);
                return -1;
            }
            if (*word == '#')
                continue;
            if (strncmp(word, "endhdr", ptr - word) == 0)
                break;

            s->headers[i].name.buf = word;
            s->headers[i].name.len = ptr - word;

            word = s3file_nextword(s, &ptr);
            if (word == NULL) {
                E_ERROR("Missing value in header line %d\n", lineno);
                return -1;
            }
            s->headers[i].value.buf = word;
            s->headers[i].value.len = ptr - word;

            if (version) {
                if (s3file_header_name_is(s, i, "version")) {
                    if (!s3file_header_value_is(s, i, version))
                        E_WARN("Version mismatch: %.*s, expecting %s\n",
                               s->headers[i].value.len,
                               s->headers[i].value.buf,
                               version);
                }
            }
            if (s3file_header_name_is(s, i, "chksum0"))
                do_chksum = TRUE;
            E_DEBUG("header line %d: %.*s=%.*s\n", lineno,
                    s->headers[i].name.len,
                    s->headers[i].name.buf,
                    s->headers[i].value.len,
                    s->headers[i].value.buf);
            i++;
        }
    } else {
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
            if ((line = s3file_nextline(s)) == NULL) {
                E_ERROR("Premature EOF, line %d\n", lineno);
                return -1;
            }
            if (strncmp(line, END_COMMENT, s->ptr - line) == 0)
                break;
            lineno++;
        }
    }

    if ((s->do_swap = swap_check(s)) < 0) {
        E_ERROR("swap_check failed\n");
        return -1;
    }
    /* Don't set this until swap check complete! */
    s->do_chksum = do_chksum;

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
s3file_copy_header_name(s3file_t *s, size_t idx)
{
    char *str = ckd_malloc(s->headers[idx].name.len + 1);
    memcpy(str, s->headers[idx].name.buf, s->headers[idx].name.len);
    str[s->headers[idx].name.len] = '\0';
    return str;
}

char *
s3file_copy_header_value(s3file_t *s, size_t idx)
{
    char *str = ckd_malloc(s->headers[idx].value.len + 1);
    memcpy(str, s->headers[idx].value.buf, s->headers[idx].value.len);
    str[s->headers[idx].value.len] = '\0';
    return str;
}

static uint32
chksum_accum(const void *buf, size_t el_sz, size_t n_el, uint32 sum)
{
    size_t i;
    uint8 *i8;
    uint16 *i16;
    uint32 *i32;

    switch (el_sz) {
    case 1:
        i8 = (uint8 *)buf;
        for (i = 0; i < n_el; i++)
            sum = (sum << 5 | sum >> 27) + i8[i];
        break;
    case 2:
        i16 = (uint16 *)buf;
        for (i = 0; i < n_el; i++)
            sum = (sum << 10 | sum >> 22) + i16[i];
        break;
    case 4:
    case 8:
        i32 = (uint32 *)buf;
        for (i = 0; i < n_el; i++)
            sum = (sum << 20 | sum >> 12) + i32[i];
        break;
    default:
        E_FATAL("Unsupported elemsize for checksum: %d\n", el_sz);
        break;
    }
    return sum;
}

static void
swap_buf(void *buf, size_t el_sz, size_t n_el)
{
    size_t i;

    switch (el_sz) {
    case 1:
        break;
    case 2:
        for (i = 0; i < n_el; i++)
            SWAP_INT16((uint16 *)buf + i);
        break;
    case 4:
        for (i = 0; i < n_el; i++)
            SWAP_INT32((uint32 *)buf + i);
        break;
    case 8:
        for (i = 0; i < n_el; i++)
            SWAP_FLOAT64((uint64 *)buf + i);
        break;
    default:
        E_FATAL("Unsupported elemsize for byteswapping: %d\n", el_sz);
        break;
    }
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
    if (s->do_swap)
        swap_buf(buf, el_sz, n_el);
    if (s->do_chksum)
        s->chksum = chksum_accum(buf, el_sz, n_el, s->chksum);

    return n_el;
}

long
s3file_get_1d(void **buf, size_t el_sz, uint32 *n_el, s3file_t *s)
{
    /* Read 1-d array size */
    if (s3file_get(n_el, sizeof(int32), 1, s) != 1) {
        E_ERROR("get(arraysize) failed\n");
        return -1;
    }
    if (*n_el <= 0)
        E_FATAL("Bad arraysize: %d\n", *n_el);

    /* Allocate memory for array data */
    *buf = (void *)ckd_calloc(*n_el, el_sz);

    /* Read array data */
    if (s3file_get(*buf, el_sz, *n_el, s) != *n_el) {
        E_ERROR("get(arraydata) failed\n");
        return -1;
    }

    return *n_el;
}

long
s3file_get_2d(void ***arr,
              size_t e_sz,
              uint32 *d1,
              uint32 *d2,
              s3file_t *s)
{
    uint32 l_d1, l_d2;
    uint32 n;
    size_t ret;
    void *raw;

    ret = s3file_get(&l_d1, sizeof(uint32), 1, s);
    if (ret != 1) {
        E_ERROR("get(dimension1) failed");
        return -1;
    }
    ret = s3file_get(&l_d2, sizeof(uint32), 1, s);
    if (ret != 1) {
        E_ERROR("get(dimension1) failed");
        return -1;
    }
    if (s3file_get_1d(&raw, e_sz, &n, s) != (int32)n) {
        E_ERROR("get(arraydata) failed");
        return -1;
    }

    assert(n == l_d1 * l_d2);

    *d1 = l_d1;
    *d2 = l_d2;
    *arr = ckd_alloc_2d_ptr(l_d1, l_d2, raw, e_sz);

    return n;
}

long
s3file_get_3d(void ****arr,
              size_t e_sz,
              uint32 *d1,
              uint32 *d2,
              uint32 *d3,
              s3file_t *s)
{
    uint32 l_d1;
    uint32 l_d2;
    uint32 l_d3;
    uint32 n;
    void *raw;
    size_t ret;

    ret = s3file_get(&l_d1, sizeof(uint32), 1, s);
    if (ret != 1) {
        E_ERROR("get(dimension1) failed");
        return -1;
    }
    ret = s3file_get(&l_d2, sizeof(uint32), 1, s);
    if (ret != 1) {
        E_ERROR("get(dimension2) failed");
        return -1;
    }
    ret = s3file_get(&l_d3, sizeof(uint32), 1, s);
    if (ret != 1) {
        E_ERROR("get(dimension3) failed");
        return -1;
    }
    if (s3file_get_1d(&raw, e_sz, &n, s) != (int32)n) {
        E_ERROR("get(arraydata) failed");
        return -1;
    }

    assert(n == l_d1 * l_d2 * l_d3);

    *arr = ckd_alloc_3d_ptr(l_d1, l_d2, l_d3, raw, e_sz);
    *d1 = l_d1;
    *d2 = l_d2;
    *d3 = l_d3;

    return n;
}

int
s3file_verify_chksum(s3file_t *s)
{
    uint32 file_chksum;

    if (!s->do_chksum)
        return 0;

    /* No more checksumming to do! */
    s->do_chksum = FALSE;
    if (s3file_get(&file_chksum, sizeof(uint32), 1, s) != 1) {
        E_ERROR("get(chksum) failed\n");
        return -1;
    }
    if (file_chksum != s->chksum) {
        E_ERROR("Checksum error; file-checksum %08x, computed %08x\n",
                file_chksum, s->chksum);
        return -1;
    }
    return 0;
}

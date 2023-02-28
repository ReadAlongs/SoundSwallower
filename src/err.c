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
/**
 * @file err.c
 * @brief Somewhat antiquated logging and error interface.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <soundswallower/ckd_alloc.h>
#include <soundswallower/err.h>
#include <soundswallower/prim_type.h>

static err_cb_f err_cb = err_stderr_cb;
static void *err_user_data;
static err_lvl_t min_loglevel = ERR_WARN;
static const char *err_level[ERR_MAX] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char *
path2basename(const char *path)
{
    const char *result;

#if defined(_WIN32) || defined(__CYGWIN__)
    result = strrchr(path, '\\');
#else
    result = strrchr(path, '/');
#endif

    return (result == NULL ? path : result + 1);
}

int
err_set_loglevel(err_lvl_t lvl)
{
    int rv = min_loglevel;
    min_loglevel = lvl;
    return rv;
}

const char *
err_set_loglevel_str(const char *lvl)
{
    const char *rv = err_level[min_loglevel];
    int i;

    if (lvl == NULL)
        return NULL;
    if (!strncmp(lvl, "ERR_", 4))
        lvl += 4;
    for (i = 0; i < ERR_MAX; ++i) {
        if (!strcmp(lvl, err_level[i])) {
            min_loglevel = i;
            return rv;
        }
    }
    return NULL;
}

/* This is very annoying, it would be easier to use vasprintf()... */
static int
vfmt_msg_size(const char *fmt, va_list args)
{
    int size = 0;
    char *bogus = NULL;

    /* Find size of message. */
    size = vsnprintf(bogus, size, fmt, args);
    if (size < 0) {
        err_cb(err_user_data, ERR_FATAL, "vsnprintf() failed in logging\n");
        return size;
    }
    size++; /* terminating \0 */
    return size;
}

static char *
vfmt_msg(const char *fmt, int size, va_list args)
{
    char *msg;
    if ((msg = ckd_malloc(size)) == NULL) {
        err_cb(err_user_data, ERR_FATAL, "malloc() failed in logging\n");
        return NULL;
    }
    size = vsnprintf(msg, size, fmt, args);
    if (size < 0) {
        ckd_free(msg);
        err_cb(err_user_data, ERR_FATAL, "vsnprintf() failed in logging\n");
        return NULL;
    }
    return msg;
}

static char *
fmt_msg(const char *fmt, ...)
{
    va_list ap;
    char *msg;
    int size;

    va_start(ap, fmt);
    size = vfmt_msg_size(fmt, ap);
    va_end(ap);
    if (size < 0)
        return NULL;
    va_start(ap, fmt);
    msg = vfmt_msg(fmt, size, ap);
    va_end(ap);
    return msg;
}

static char *
add_level_and_lineno(err_lvl_t lvl, const char *fname, long ln, char *msg)
{
    if (lvl == ERR_INFO)
        return fmt_msg("%s: %s(%ld): %s", err_level[lvl], fname, ln, msg);
    else
        return fmt_msg("%s: \"%s\", line %ld: %s", err_level[lvl], fname, ln, msg);
}

void
err_msg(err_lvl_t lvl, const char *path, long ln, const char *fmt, ...)
{

    char *msg;
    va_list args;
    int size;

    if (!err_cb)
        return;
    if (lvl < min_loglevel)
        return;
    va_start(args, fmt);
    size = vfmt_msg_size(fmt, args);
    va_end(args);
    if (size < 0)
        return;
    va_start(args, fmt);
    msg = vfmt_msg(fmt, size, args);
    va_end(args);
    if (msg == NULL)
        return;

    if (path) {
        const char *fname = path2basename(path);
        char *longmsg = add_level_and_lineno(lvl, fname, ln, msg);
        if (longmsg == NULL)
            ckd_free(msg);
        else
            err_cb(err_user_data, lvl, longmsg);
        ckd_free(longmsg);
    } else {
        err_cb(err_user_data, lvl, msg);
    }
    ckd_free(msg);
}

void
err_msg_system(err_lvl_t lvl, const char *path, long ln, const char *fmt, ...)
{
    int local_errno = errno;

    char *msg, *msgsys;
    va_list args;
    int size;

    if (!err_cb)
        return;
    if (lvl < min_loglevel)
        return;
    va_start(args, fmt);
    size = vfmt_msg_size(fmt, args);
    va_end(args);
    if (size < 0)
        return;
    va_start(args, fmt);
    msg = vfmt_msg(fmt, size, args);
    va_end(args);
    if (msg == NULL)
        return;
    msgsys = fmt_msg("%s: %s\n", msg, strerror(local_errno));
    ckd_free(msg);
    if (msgsys == NULL)
        return;
    if (path) {
        const char *fname = path2basename(path);
        char *longmsg = add_level_and_lineno(lvl, fname, ln, msgsys);
        if (longmsg == NULL)
            ckd_free(msg);
        else
            err_cb(err_user_data, lvl, longmsg);
        ckd_free(longmsg);
    } else {
        err_cb(err_user_data, lvl, msgsys);
    }
    ckd_free(msgsys);
}

void
err_stderr_cb(void *user_data, err_lvl_t lvl, const char *msg)
{
    (void)user_data;
    (void)lvl;

    fwrite(msg, 1, strlen(msg), stderr);
    fflush(stderr);
}

void
err_set_callback(err_cb_f cb, void *user_data)
{
    err_cb = cb;
    err_user_data = user_data;
}

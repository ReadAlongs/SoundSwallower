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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <soundswallower/err.h>
#include <soundswallower/prim_type.h>
#include <soundswallower/filename.h>
#include <soundswallower/ckd_alloc.h>

static FILE*  logfp = NULL;
static int    logfp_disabled = FALSE;

static err_cb_f err_cb = err_logfp_cb;
static void* err_user_data;

void
err_msg(err_lvl_t lvl, const char *path, long ln, const char *fmt, ...)
{
    static const char *err_prefix[ERR_MAX] = {
        "DEBUG", "INFO", "INFOCONT", "WARN", "ERROR", "FATAL"
    };

    char msg[1024];
    va_list ap;

    if (!err_cb)
        return;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    if (path) {
        const char *fname = path2basename(path);
        if (lvl == ERR_INFOCONT)
    	    err_cb(err_user_data, lvl, "%s(%ld): %s", fname, ln, msg);
        else if (lvl == ERR_INFO)
            err_cb(err_user_data, lvl, "%s: %s(%ld): %s", err_prefix[lvl], fname, ln, msg);
        else
    	    err_cb(err_user_data, lvl, "%s: \"%s\", line %ld: %s", err_prefix[lvl], fname, ln, msg);
    } else {
        err_cb(err_user_data, lvl, "%s", msg);
    }
}

void
err_msg_system(err_lvl_t lvl, const char *path, long ln, const char *fmt, ...)
{
    int local_errno = errno;
    
    static const char *err_prefix[ERR_MAX] = {
        "DEBUG", "INFO", "INFOCONT", "WARN", "ERROR", "FATAL"
    };

    char msg[1024];
    va_list ap;

    if (!err_cb)
        return;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    if (path) {
        const char *fname = path2basename(path);
        if (lvl == ERR_INFOCONT)
    	    err_cb(err_user_data, lvl, "%s(%ld): %s: %s\n", fname, ln, msg, strerror(local_errno));
        else if (lvl == ERR_INFO)
            err_cb(err_user_data, lvl, "%s: %s(%ld): %s: %s\n", err_prefix[lvl], fname, ln, msg, strerror(local_errno));
        else
    	    err_cb(err_user_data, lvl, "%s: \"%s\", line %ld: %s: %s\n", err_prefix[lvl], fname, ln, msg, strerror(local_errno));
    } else {
        err_cb(err_user_data, lvl, "%s: %s\n", msg, strerror(local_errno));
    }
}

void
err_logfp_cb(void *user_data, err_lvl_t lvl, const char *fmt, ...)
{
    va_list ap;
    FILE *fp = err_get_logfp();

    (void)user_data;
    (void)lvl; /* FIXME?!?! */
    
    if (!fp)
        return;
    
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);
    fflush(fp);
}

int
err_set_logfile(const char *path)
{
    FILE *newfp, *oldfp;

    if ((newfp = fopen(path, "a")) == NULL)
        return -1;
    oldfp = err_get_logfp();
    err_set_logfp(newfp);
    return 0;
}

void
err_set_logfp(FILE *stream)
{
    if (logfp != NULL && logfp != stdout && logfp != stderr)
        fclose(logfp);
    if (stream == NULL) {
	logfp_disabled = TRUE;
	logfp = NULL;
	return;
    }    
    logfp_disabled = FALSE;
    logfp = stream;
    return;
}

FILE *
err_get_logfp(void)
{
    if (logfp_disabled)
	return NULL;
    if (logfp == NULL)
	return stderr;

    return logfp;
}

void
err_set_callback(err_cb_f cb, void* user_data)
{
    err_cb = cb;
    err_user_data= user_data;
}

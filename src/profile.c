/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2001 Carnegie Mellon University.  All rights
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
 * profile.c -- For timing and event counting.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 *
 * HISTORY
 * $Log: profile.c,v $
 * Revision 1.7  2005/06/22 03:10:59  arthchan2003
 * 1, Fixed doxygen documentation, 2, Added  keyword.
 *
 * Revision 1.3  2005/03/30 01:22:48  archan
 * Fixed mistakes in last updates. Add
 *
 *
 * 11-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added ptmr_init().
 *
 * 19-Jun-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <time.h>
#include <windows.h>
#elif defined(HAVE_UNISTD_H) /* I know this, this is Unix... */
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include <soundswallower/ckd_alloc.h>
#include <soundswallower/err.h>
#include <soundswallower/profile.h>

pctr_t *
pctr_new(char *nm)
{
    pctr_t *pc;

    pc = ckd_calloc(1, sizeof(pctr_t));
    pc->name = ckd_salloc(nm);
    pc->count = 0;

    return pc;
}

void
pctr_reset(pctr_t *ctr)
{
    ctr->count = 0;
}

void
pctr_increment(pctr_t *ctr, int32 inc)
{
    ctr->count += inc;
    /*   E_INFO("Name %s, Count %d, inc %d\n",ctr->name, ctr->count, inc); */
}

void
pctr_free(pctr_t *pc)
{
    if (pc) {
        if (pc->name)
            ckd_free(pc->name);
    }
    ckd_free(pc);
}

#if defined(_WIN32)

#define TM_LOWSCALE 1e-7
#define TM_HIGHSCALE (4294967296.0 * TM_LOWSCALE);

static float64
make_sec(FILETIME *tm)
{
    float64 dt;

    dt = tm->dwLowDateTime * TM_LOWSCALE;
    dt += tm->dwHighDateTime * TM_HIGHSCALE;

    return (dt);
}

#else /* NOT WINDOWS */

static float64
make_sec(struct timeval *s)
{
    return (s->tv_sec + s->tv_usec * 0.000001);
}

#endif

void
ptmr_start(ptmr_t *tm)
{
#if !defined(_WIN32)
    struct timeval e_start; /* Elapsed time */
    gettimeofday(&e_start, 0);
    tm->start_elapsed = make_sec(&e_start);
#else
    HANDLE pid;
    FILETIME t_create, t_exit, kst, ust;

    /* PC */
    pid = GetCurrentProcess();
    GetProcessTimes(pid, &t_create, &t_exit, &kst, &ust);
    tm->start_cpu = make_sec(&ust) + make_sec(&kst);

    tm->start_elapsed = (float64)clock() / CLOCKS_PER_SEC;
#endif
}

void
ptmr_stop(ptmr_t *tm)
{
    float64 dt_cpu, dt_elapsed;

#if !defined(_WIN32)
    /* Unix */
    struct timeval e_stop; /* Elapsed time */
#if defined(HAVE_GETRUSAGE) && !defined(__EMSCRIPTEN__) /* Which LIES */
    struct rusage stop; /* CPU time */
    getrusage(RUSAGE_SELF, &stop);
    dt_cpu = make_sec(&stop.ru_utime) + make_sec(&stop.ru_stime) - tm->start_cpu;
#else
    dt_cpu = 0.0;
#endif
    gettimeofday(&e_stop, 0);
    dt_elapsed = (make_sec(&e_stop) - tm->start_elapsed);
#else
    /* Windows */
    HANDLE pid;
    FILETIME t_create, t_exit, kst, ust;

    /* PC */
    pid = GetCurrentProcess();
    GetProcessTimes(pid, &t_create, &t_exit, &kst, &ust);
    dt_cpu = make_sec(&ust) + make_sec(&kst) - tm->start_cpu;
    dt_elapsed = ((float64)clock() / CLOCKS_PER_SEC) - tm->start_elapsed;
#endif

    tm->t_cpu += dt_cpu;
    tm->t_elapsed += dt_elapsed;

    tm->t_tot_cpu += dt_cpu;
    tm->t_tot_elapsed += dt_elapsed;
}

void
ptmr_reset(ptmr_t *tm)
{
    tm->t_cpu = 0.0;
    tm->t_elapsed = 0.0;
}

void
ptmr_init(ptmr_t *tm)
{
    tm->t_cpu = 0.0;
    tm->t_elapsed = 0.0;
    tm->t_tot_cpu = 0.0;
    tm->t_tot_elapsed = 0.0;
}

void
ptmr_reset_all(ptmr_t *tm)
{
    for (; tm->name; tm++)
        ptmr_reset(tm);
}

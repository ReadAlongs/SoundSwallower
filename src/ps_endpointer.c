/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2022 David Huggins-Daines.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 */

#include <string.h>
#include <assert.h>

#include <soundswallower/endpointer.h>
#include <soundswallower/ckd_alloc.h>
#include <soundswallower/err.h>

struct endpointer_s {
    vad_t *vad;
    int refcount;
    int start_frames, end_frames;
    double frame_length;
    int in_speech;
    int frame_size;
    int maxlen;
    int16 *buf;
    int8 *is_speech;
    int pos, n;
    double qstart_time, timestamp;
    double speech_start, speech_end;
};

endpointer_t *
endpointer_init(double window,
                double ratio,
                vad_mode_t mode,
                int sample_rate, double frame_length)
{
    endpointer_t *ep = ckd_calloc(1, sizeof(*ep));
    ep->refcount = 1;
    if ((ep->vad = vad_init(mode, sample_rate, frame_length)) == NULL)
        goto error_out;
    if (window == 0.0)
        window = ENDPOINTER_DEFAULT_WINDOW;
    if (ratio == 0.0)
        ratio = ENDPOINTER_DEFAULT_RATIO;
    ep->frame_length = vad_frame_length(ep->vad);
    ep->maxlen = (int)(window / ep->frame_length + 0.5);
    ep->start_frames = (int)(ratio * ep->maxlen);
    ep->end_frames = (int)((1.0 - ratio) * ep->maxlen + 0.5);
    /* Make sure endpointing is possible ;-) */
    if (ep->start_frames <= 0 || ep->start_frames >= ep->maxlen) {
        E_ERROR("Ratio %.2f makes start-pointing stupid or impossible (%d frames of %d)\n",
                ratio, ep->start_frames, ep->maxlen);
        goto error_out;
    }
    if (ep->end_frames <= 0 || ep->end_frames >= ep->maxlen) {
        E_ERROR("Ratio %.2f makes end-pointing stupid or impossible (%d frames of %d)\n",
                ratio, ep->end_frames, ep->maxlen);
        goto error_out;
    }
    E_INFO("Threshold %d%% of %.3fs window (>%d frames <%d frames of %d)\n",
           (int)(ratio * 100.0 + 0.5),
           ep->maxlen * ep->frame_length, ep->start_frames, ep->end_frames, ep->maxlen);
    ep->frame_size = endpointer_frame_size(ep);
    ep->buf = ckd_calloc(sizeof(*ep->buf),
                         ep->maxlen * ep->frame_size);
    ep->is_speech = ckd_calloc(1, ep->maxlen);
    ep->pos = ep->n = 0;
    return ep;
error_out:
    endpointer_free(ep);
    return NULL;
}

endpointer_t *
endpointer_retain(endpointer_t *ep)
{
    if (ep == NULL)
        return NULL;
    ++ep->refcount;
    return ep;
}

int
endpointer_free(endpointer_t *ep)
{
    if (ep == NULL)
        return 0;
    if (--ep->refcount > 0)
        return ep->refcount;
    vad_free(ep->vad);
    if (ep->buf)
        ckd_free(ep->buf);
    if (ep->is_speech)
        ckd_free(ep->is_speech);
    ckd_free(ep);
    return 0;
}

vad_t *
endpointer_vad(endpointer_t *ep)
{
    if (ep == NULL)
        return NULL;
    return ep->vad;
}

static int
ep_empty(endpointer_t *ep)
{
    return ep->n == 0;
}

static int
ep_full(endpointer_t *ep)
{
    return ep->n == ep->maxlen;
}

static void
ep_clear(endpointer_t *ep)
{
    ep->n = 0;
}

static int
ep_speech_count(endpointer_t *ep)
{
    int count = 0;
    if (ep_empty(ep))
        ;
    else if (ep_full(ep)) {
        int i;
        for (i = 0; i < ep->maxlen; ++i)
            count += ep->is_speech[i];
    }
    else {
        int i = ep->pos, end = (ep->pos + ep->n) % ep->maxlen;
        count = ep->is_speech[i++];
        while (i != end) {
            count += ep->is_speech[i++];
            i = i % ep->maxlen;
        }
    }
    return count;
}

static int
ep_push(endpointer_t *ep, int is_speech, const int16 *frame)
{
    int i = (ep->pos + ep->n) % ep->maxlen;
    int16 *dest = ep->buf + (i * ep->frame_size);
    memcpy(dest, frame, sizeof(*ep->buf) * ep->frame_size);
    ep->is_speech[i] = is_speech;
    if (ep_full(ep)) {
        ep->qstart_time += ep->frame_length;
        ep->pos = (ep->pos + 1) % ep->maxlen;
    }
    else
        ep->n++;
    return ep->n;
}

static int16 *
ep_pop(endpointer_t *ep, int *out_is_speech)
{
    int16 *pcm;
    if (ep_empty(ep))
        return NULL;
    ep->qstart_time += ep->frame_length;
    if (out_is_speech)
        *out_is_speech = ep->is_speech[ep->pos];
    pcm = ep->buf + (ep->pos * ep->frame_size);
    ep->pos = (ep->pos + 1) % ep->maxlen;
    ep->n--;
    return pcm;
}

static void
ep_linearize(endpointer_t *ep)
{
    int16 *tmp_pcm;
    uint8 *tmp_is_speech;

    if (ep->pos == 0)
        return;
    /* Second part of data: | **** ^ .. | */
    tmp_pcm = ckd_calloc(sizeof(*ep->buf),
                         ep->pos * ep->frame_size);
    tmp_is_speech = ckd_calloc(sizeof(*ep->is_speech), ep->pos);
    memcpy(tmp_pcm, ep->buf,
           sizeof(*ep->buf) * ep->pos * ep->frame_size);
    memcpy(tmp_is_speech, ep->is_speech,
           sizeof(*ep->is_speech) * ep->pos);

    /* First part of data: | .... ^ ** | -> | ** ---- |  */
    memmove(ep->buf, ep->buf + ep->pos * ep->frame_size,
            sizeof(*ep->buf) * (ep->maxlen - ep->pos) * ep->frame_size);
    memmove(ep->is_speech, ep->is_speech + ep->pos,
            sizeof(*ep->is_speech) * (ep->maxlen - ep->pos));

    /* Second part of data: | .. **** | */
    memcpy(ep->buf + (ep->maxlen - ep->pos) * ep->frame_size, tmp_pcm,
           sizeof(*ep->buf) * ep->pos * ep->frame_size);
    memcpy(ep->is_speech + (ep->maxlen - ep->pos), tmp_is_speech,
           sizeof(*ep->is_speech) * ep->pos);

    /* Update pointer */
    ep->pos = 0;
    ckd_free(tmp_pcm);
    ckd_free(tmp_is_speech);
}

const int16 *
endpointer_end_stream(endpointer_t *ep,
                      const int16 *frame,
                      size_t nsamp,
                      size_t *out_nsamp)
{
    if (nsamp > (size_t)ep->frame_size) {
        E_ERROR("Final frame must be %d samples or less\n",
                ep->frame_size);
        return NULL;
    }
    
    if (out_nsamp)
        *out_nsamp = 0;
    if (!ep->in_speech)
        return NULL;
    ep->in_speech = FALSE;
    ep->speech_end = ep->qstart_time;

    /* Rotate the buffer so we can return data in a single call. */
    ep_linearize(ep);
    assert(ep->pos == 0);
    while (!ep_empty(ep)) {
        int is_speech;
        ep_pop(ep, &is_speech);
        if (is_speech) {
            if (out_nsamp)
                *out_nsamp += ep->frame_size;
            ep->speech_end = ep->qstart_time;
        }
        else
            break;
    }
    /* If we used all the VAD queue, add the trailing samples. */
    if (ep_empty(ep) && ep->speech_end == ep->qstart_time) {
        if (ep->pos == ep->maxlen) {
            E_ERROR("VAD queue overflow (should not happen)");
            /* Not fatal, we just lose data. */
        }
        else {
            ep->timestamp +=
                (double)nsamp / endpointer_sample_rate(ep);
            if (out_nsamp)
                *out_nsamp += nsamp;
            memcpy(ep->buf + ep->pos * ep->frame_size,
                   frame, nsamp * sizeof(*ep->buf));
            ep->speech_end = ep->timestamp;
        }
    }
    ep_clear(ep);
    return ep->buf;
}

const int16 *
endpointer_process(endpointer_t *ep,
                   const int16 *frame)
{
    int is_speech, speech_count;
    if (ep == NULL || ep->vad == NULL)
        return NULL;
    if (ep->in_speech && ep_full(ep)) {
        E_ERROR("VAD queue overflow (should not happen)");
        /* Not fatal, we just lose data. */
    }
    is_speech = vad_classify(ep->vad, frame);
    ep_push(ep, is_speech, frame);
    ep->timestamp += ep->frame_length;
    speech_count = ep_speech_count(ep);
    E_DEBUG("%.2f %d %d %d\n", ep->timestamp, speech_count,
            ep->start_frames, ep->end_frames);
    if (ep->in_speech) {
        if (speech_count < ep->end_frames) {
            /* Return only the first frame.  Either way it's sort of
               arbitrary, but this avoids having to drain the queue to
               prevent overlapping segments.  It's also closer to what
               human annotators will do. */
            int16 *pcm = ep_pop(ep, NULL);
            ep->speech_end = ep->qstart_time;
            ep->in_speech = FALSE;
            return pcm;
        }
    }
    else {
        if (speech_count > ep->start_frames) {
            ep->speech_start = ep->qstart_time;
            ep->speech_end = 0;
            ep->in_speech = TRUE;
        }
    }
    if (ep->in_speech)
        return ep_pop(ep, NULL);
    else
        return NULL;
}

int
endpointer_in_speech(endpointer_t *ep)
{
    return ep->in_speech;
}

double
endpointer_speech_start(endpointer_t *ep)
{
    return ep->speech_start;
}

double
endpointer_speech_end(endpointer_t *ep)
{
    return ep->speech_end;
}

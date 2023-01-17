/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2008 Carnegie Mellon University.  All rights
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

#include "config.h"

#include <assert.h>
#include <stdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <soundswallower/acmod.h>
#include <soundswallower/config_defs.h>
#include <soundswallower/decoder.h>
#include <soundswallower/err.h>
#include <soundswallower/fsg_search.h>
#include <soundswallower/hash_table.h>
#include <soundswallower/jsgf.h>
#include <soundswallower/search_module.h>
#include <soundswallower/state_align_search.h>
#include <soundswallower/strfuncs.h>

/* Do this unconditionally if we have no filesystem */
/* FIXME: Actually just the web environment */
#ifdef __EMSCRIPTEN__
static void
expand_file_config(config_t *config, const char *arg,
                   const char *hmmdir, const char *file)
{
    const char *val;
    if ((val = config_str(config, arg)) == NULL) {
        char *tmp = string_join(hmmdir, "/", file, NULL);
        config_set_str(config, arg, tmp);
        ckd_free(tmp);
    }
}
#else
#ifdef WIN32
#define PATHSEP "\\"
#else
#define PATHSEP "/"
#endif
static int
file_exists(const char *path)
{
    FILE *tmp;

    tmp = fopen(path, "rb");
    if (tmp)
        fclose(tmp);
    return (tmp != NULL);
}

static void
expand_file_config(config_t *config, const char *arg,
                   const char *hmmdir, const char *file)
{
    const char *val;
    if ((val = config_str(config, arg)) == NULL) {
        char *tmp = string_join(hmmdir, PATHSEP, file, NULL);
        if (file_exists(tmp))
            config_set_str(config, arg, tmp);
        else
            config_set_str(config, arg, NULL);
        ckd_free(tmp);
    }
}
#endif /* __EMSCRIPTEN__ */

void
config_expand(config_t *config)
{
    const char *hmmdir, *featparams;

    /* Get acoustic model filenames and add them to the command-line */
    hmmdir = config_str(config, "hmm");
    if (hmmdir) {
        expand_file_config(config, "mdef", hmmdir, "mdef");
        expand_file_config(config, "mean", hmmdir, "means");
        expand_file_config(config, "var", hmmdir, "variances");
        expand_file_config(config, "tmat", hmmdir, "transition_matrices");
        expand_file_config(config, "mixw", hmmdir, "mixture_weights");
        expand_file_config(config, "sendump", hmmdir, "sendump");
        expand_file_config(config, "lda", hmmdir, "feature_transform");
        expand_file_config(config, "featparams", hmmdir, "feat_params.json");
        expand_file_config(config, "senmgau", hmmdir, "senmgau");
        expand_file_config(config, "dict", hmmdir, "dict.txt");
        expand_file_config(config, "fdict", hmmdir, "noisedict.txt");
    }

    /* And again ... without stdio, can't do this. */
#ifdef __EMSCRIPTEN__
    (void)featparams;
#else
    /* Look for feat_params.json in acoustic model dir. */
    if ((featparams = config_str(config, "featparams")) != NULL) {
        /* Open in binary mode so fread() matches ftell() on Windows */
        FILE *json = fopen(featparams, "rb");
        char *jsontxt;
        size_t len, rv;

        if (json == NULL)
            return;
        fseek(json, 0, SEEK_END);
        len = ftell(json);
        if (fseek(json, 0, SEEK_SET) < 0) {
            E_ERROR_SYSTEM("Failed to rewind %s", featparams);
            return;
        }
        jsontxt = malloc(len + 1);
        jsontxt[len] = '\0';
        if ((rv = fread(jsontxt, 1, len, json)) != len) {
            E_ERROR_SYSTEM("Failed to read %s (got %zu not %zu)",
                           featparams, rv, len);
            ckd_free(jsontxt);
            return;
        }
        if (config_parse_json(config, jsontxt))
            E_INFO("Parsed model-specific feature parameters from %s\n",
                   featparams);
        ckd_free(jsontxt);
        fclose(json);
    }
#endif
}

static void
decoder_free_searches(decoder_t *d)
{
    if (d->search) {
        search_module_free(d->search);
        d->search = NULL;
    }
    if (d->align) {
        search_module_free(d->align);
        d->align = NULL;
    }
}

static int
set_loglevel(config_t *config)
{
    const char *loglevel;
    loglevel = config_str(config, "loglevel");
    if (loglevel) {
        if (err_set_loglevel_str(loglevel) == NULL) {
            E_ERROR("Invalid log level: %s\n", loglevel);
            return -1;
        }
    }
    return 0;
}

#ifndef __EMSCRIPTEN__
static void
log_callback(void *user_data, err_lvl_t lvl, const char *msg)
{
    decoder_t *d = (decoder_t *)user_data;
    (void)lvl;
    assert(d->logfh != NULL);
    fwrite(msg, 1, strlen(msg), d->logfh);
    fflush(d->logfh);
}
#endif

int
decoder_set_logfile(decoder_t *d, const char *logfn)
{
#ifdef __EMSCRIPTEN__
    (void)d;
    (void)logfn;
    E_WARN("decoder_set_logfile() does nothing in JavaScript");
#else
    FILE *new_logfh;
    if (logfn == NULL)
        new_logfh = NULL;
    else {
        new_logfh = fopen(logfn, "a");
        if (new_logfh == NULL) {
            E_ERROR_SYSTEM("Failed to open log file %s", logfn);
            return -1;
        }
    }
    if (d->logfh)
        fclose(d->logfh);
    d->logfh = new_logfh;
    if (new_logfh == NULL)
        err_set_callback(err_stderr_cb, NULL);
    else
        err_set_callback(log_callback, d);
#endif
    return 0;
}

static void
set_logfile(decoder_t *d, config_t *config)
{
#ifdef __EMSCRIPTEN__
    (void)d;
    (void)config;
#else
    const char *logfn;
    logfn = config_str(config, "logfn");
    if (logfn)
        decoder_set_logfile(d, logfn);
#endif
}

int
decoder_init_config(decoder_t *d, config_t *config)
{
    /* Return immediately on invalid loglevel */
    if (set_loglevel(config ? config : d->config) < 0)
        return -1;
    /* Switch the config if it is new and different */
    if (config && config != d->config) {
        config_free(d->config);
        /* Note! Consuming semantics. */
        d->config = config;
        set_logfile(d, d->config);
    }
    /* Expand model parameters. */
    config_expand(d->config);
    /* Print out the config for logging. */
    config_log_values(d->config);

    /* Logmath computation (used in acmod and search) */
    if (d->lmath == NULL
        || (logmath_get_base(d->lmath) != (float64)config_float(d->config, "logbase"))) {
        if (d->lmath)
            logmath_free(d->lmath);
        d->lmath = logmath_init((float64)config_float(d->config, "logbase"), 0, TRUE);
    }

    /* Initialize performance timer. */
    d->perf.name = "decode";
    ptmr_init(&d->perf);

    return 0;
}

int
decoder_init_cleanup(decoder_t *d)
{
    /* Free old searches (do this before other reinit) */
    decoder_free_searches(d);
    ckd_free(d->json_result);
    d->json_result = NULL;

    return 0;
}

fe_t *
decoder_init_fe(decoder_t *d)
{
    if (d->config == NULL)
        return NULL;
    fe_free(d->fe);
    d->fe = fe_init(d->config);
    return d->fe;
}

feat_t *
decoder_init_feat(decoder_t *d)
{
    if (d->config == NULL)
        return NULL;
    feat_free(d->fcb);
    d->fcb = feat_init(d->config);
    return d->fcb;
}

feat_t *
decoder_init_feat_s3file(decoder_t *d, s3file_t *lda)
{
    if (d->config == NULL)
        return NULL;
    feat_free(d->fcb);
    d->fcb = feat_init_s3file(d->config, lda);
    return d->fcb;
}

acmod_t *
decoder_init_acmod_pre(decoder_t *d)
{
    if (d->config == NULL)
        return NULL;
    if (d->lmath == NULL)
        return NULL;
    if (d->fe == NULL)
        return NULL;
    if (d->fcb == NULL)
        return NULL;
    acmod_free(d->acmod);
    d->acmod = acmod_create(d->config, d->lmath, d->fe, d->fcb);
    return d->acmod;
}

int
decoder_init_acmod_post(decoder_t *d)
{
    if (d->acmod == NULL)
        return -1;
    if (acmod_init_senscr(d->acmod) < 0)
        return -1;
    return 0;
}

acmod_t *
decoder_init_acmod(decoder_t *d)
{
    if (d->config == NULL)
        return NULL;
    if (d->lmath == NULL)
        return NULL;
    if (d->fe == NULL)
        return NULL;
    if (d->fcb == NULL)
        return NULL;
    acmod_free(d->acmod);
    d->acmod = acmod_init(d->config, d->lmath, d->fe, d->fcb);
    return d->acmod;
}

dict_t *
decoder_init_dict(decoder_t *d)
{
    if (d->config == NULL)
        return NULL;
    if (d->acmod == NULL)
        return NULL;
    /* Free old dictionary */
    dict_free(d->dict);
    /* Free d2p */
    dict2pid_free(d->d2p);
    /* Dictionary and triphone mappings (depends on acmod). */
    /* FIXME: pass config, change arguments, implement LTS, etc. */
    if ((d->dict = dict_init(d->config, d->acmod->mdef)) == NULL)
        return NULL;
    if ((d->d2p = dict2pid_build(d->acmod->mdef, d->dict)) == NULL)
        return NULL;
    return d->dict;
}

dict_t *
decoder_init_dict_s3file(decoder_t *d, s3file_t *dict, s3file_t *fdict)
{
    if (d->config == NULL)
        return NULL;
    if (d->acmod == NULL)
        return NULL;
    /* Free old dictionary */
    dict_free(d->dict);
    /* Free d2p */
    dict2pid_free(d->d2p);
    /* Dictionary and triphone mappings (depends on acmod). */
    /* FIXME: pass config, change arguments, implement LTS, etc. */
    if ((d->dict = dict_init_s3file(d->config, d->acmod->mdef, dict, fdict)) == NULL)
        return NULL;
    if ((d->d2p = dict2pid_build(d->acmod->mdef, d->dict)) == NULL)
        return NULL;
    return d->dict;
}

int
decoder_init_grammar(decoder_t *d)
{
    const char *path;
    float32 lw;

    lw = config_float(d->config, "lw");

    if ((path = config_str(d->config, "jsgf"))) {
        if (decoder_set_jsgf_file(d, path) != 0)
            return -1;
    } else if ((path = config_str(d->config, "fsg"))) {
        fsg_model_t *fsg = fsg_model_readfile(path, d->lmath, lw);
        if (!fsg)
            return -1;
        if (decoder_set_fsg(d, fsg) != 0) {
            fsg_model_free(fsg);
            return -1;
        }
    }
    return 0;
}

int
decoder_init_grammar_s3file(decoder_t *d, s3file_t *fsg_file, s3file_t *jsgf_file)
{
    float32 lw;

    lw = config_float(d->config, "lw");

    /* JSGF takes precedence */
    if (jsgf_file) {
        /* FIXME: This depends on jsgf->buf having 0 at the end, which
           it will when created by JavaScript, but that is not
           guaranteed when memory-mapped. */
        if (decoder_set_jsgf_string(d, jsgf_file->ptr) != 0)
            return -1;
    }
    if (fsg_file) {
        fsg_model_t *fsg = fsg_model_read_s3file(fsg_file, d->lmath, lw);
        if (!fsg)
            return -1;
        if (decoder_set_fsg(d, fsg) != 0) {
            fsg_model_free(fsg);
            return -1;
        }
    }

    return 0;
}

int
decoder_reinit_feat(decoder_t *d, config_t *config)
{
    if (config && config != d->config) {
        config_free(d->config);
        /* NOTE! Consuming semantics */
        d->config = config;
    }
    if (decoder_init_fe(d) == NULL)
        return -1;
    if (decoder_init_feat(d) == NULL)
        return -1;
    return acmod_reinit_feat(d->acmod, d->fe, d->fcb);
}

int
decoder_reinit(decoder_t *d, config_t *config)
{
    if (config)
        if (decoder_init_config(d, config) < 0)
            return -1;
    if (decoder_init_cleanup(d) < 0)
        return -1;
    if (decoder_init_fe(d) == NULL)
        return -1;
    if (decoder_init_feat(d) == NULL)
        return -1;
    if (decoder_init_acmod(d) == NULL)
        return -1;
    if (decoder_init_dict(d) == NULL)
        return -1;
    if (decoder_init_grammar(d) < 0)
        return -1;

    return 0;
}

const char *
decoder_get_cmn(decoder_t *ps, int update)
{
    if (update)
        cmn_live_update(ps->acmod->fcb->cmn_struct);
    return cmn_repr(ps->acmod->fcb->cmn_struct);
}

int
decoder_set_cmn(decoder_t *ps, const char *cmn)
{
    return cmn_set_repr(ps->acmod->fcb->cmn_struct, cmn);
}

decoder_t *
decoder_create(config_t *config)
{
    decoder_t *d;

    d = ckd_calloc(1, sizeof(*d));
    d->refcount = 1;
    if (config) {
        if (decoder_init_config(d, config) < 0) {
            decoder_free(d);
            return NULL;
        }
    }
    return d;
}

decoder_t *
decoder_init(config_t *config)
{
    decoder_t *d = decoder_create(config);
    if (d == NULL)
        return NULL;
    if (decoder_reinit(d, NULL) < 0) {
        decoder_free(d);
        return NULL;
    }
    return d;
}

decoder_t *
decoder_retain(decoder_t *d)
{
    if (d == NULL)
        return NULL;
    ++d->refcount;
    return d;
}

int
decoder_free(decoder_t *d)
{
    if (d == NULL)
        return 0;
    if (--d->refcount > 0)
        return d->refcount;
    decoder_free_searches(d);
    dict_free(d->dict);
    dict2pid_free(d->d2p);
    feat_free(d->fcb);
    fe_free(d->fe);
    acmod_free(d->acmod);
    logmath_free(d->lmath);
    config_free(d->config);
    ckd_free(d->json_result);
#ifndef __EMSCRIPTEN__
    if (d->logfh) {
        fclose(d->logfh);
        err_set_callback(err_stderr_cb, NULL);
    }
#endif
    ckd_free(d);
    return 0;
}

config_t *
decoder_config(decoder_t *d)
{
    return d->config;
}

logmath_t *
decoder_logmath(decoder_t *d)
{
    return d->lmath;
}

fe_t *
decoder_fe(decoder_t *d)
{
    return d->fe;
}

feat_t *
decoder_feat(decoder_t *d)
{
    return d->fcb;
}

mllr_t *
decoder_apply_mllr(decoder_t *d, mllr_t *mllr)
{
    return acmod_update_mllr(d->acmod, mllr);
}

int
decoder_set_fsg(decoder_t *d, fsg_model_t *fsg)
{
    search_module_t *search;
    search = fsg_search_init(fsg->name, fsg, d->config, d->acmod, d->dict, d->d2p);
    if (search == NULL)
        return -1;
    if (d->search)
        search_module_free(d->search);
    d->search = search;
    return 0;
}

int
decoder_set_jsgf_file(decoder_t *d, const char *path)
{
    fsg_model_t *fsg;
    jsgf_rule_t *rule;
    const char *toprule;
    jsgf_t *jsgf = jsgf_parse_file(path, NULL);
    float lw;
    int result;

    if (!jsgf)
        return -1;

    rule = NULL;
    /* Take the -toprule if specified. */
    if ((toprule = config_str(d->config, "toprule"))) {
        rule = jsgf_get_rule(jsgf, toprule);
        if (rule == NULL) {
            E_ERROR("Start rule %s not found\n", toprule);
            jsgf_grammar_free(jsgf);
            return -1;
        }
    } else {
        rule = jsgf_get_public_rule(jsgf);
        if (rule == NULL) {
            E_ERROR("No public rules found in %s\n", path);
            jsgf_grammar_free(jsgf);
            return -1;
        }
    }

    lw = config_float(d->config, "lw");
    fsg = jsgf_build_fsg(jsgf, rule, d->lmath, lw);
    result = decoder_set_fsg(d, fsg);
    jsgf_grammar_free(jsgf);
    return result;
}

int
decoder_set_jsgf_string(decoder_t *d, const char *jsgf_string)
{
    fsg_model_t *fsg;
    jsgf_rule_t *rule;
    const char *toprule;
    jsgf_t *jsgf = jsgf_parse_string(jsgf_string, NULL);
    float lw;
    int result;

    if (!jsgf)
        return -1;

    rule = NULL;
    /* Take the -toprule if specified. */
    if ((toprule = config_str(d->config, "toprule"))) {
        rule = jsgf_get_rule(jsgf, toprule);
        if (rule == NULL) {
            E_ERROR("Start rule %s not found\n", toprule);
            jsgf_grammar_free(jsgf);
            return -1;
        }
    } else {
        rule = jsgf_get_public_rule(jsgf);
        if (rule == NULL) {
            E_ERROR("No public rules found in input string\n");
            jsgf_grammar_free(jsgf);
            return -1;
        }
    }

    lw = config_float(d->config, "lw");
    fsg = jsgf_build_fsg(jsgf, rule, d->lmath, lw);
    result = decoder_set_fsg(d, fsg);
    jsgf_grammar_free(jsgf);
    return result;
}

int
decoder_set_align_text(decoder_t *d, const char *text)
{
    fsg_model_t *fsg;
    char *textbuf = ckd_salloc(text);
    char *ptr, *word, delimfound;
    int n, nwords;

    textbuf = string_trim(textbuf, STRING_BOTH);
    /* First pass: count and verify words */
    nwords = 0;
    ptr = textbuf;
    while ((n = nextword(ptr, " \t\n\r", &word, &delimfound)) >= 0) {
        int wid;
        if ((wid = dict_wordid(d->dict, word)) == BAD_S3WID) {
            E_ERROR("Unknown word %s\n", word);
            ckd_free(textbuf);
            return -1;
        }
        ptr = word + n;
        *ptr = delimfound;
        ++nwords;
    }
    /* Second pass: make fsg */
    fsg = fsg_model_init(text, d->lmath,
                         config_float(d->config, "lw"),
                         nwords + 1);
    nwords = 0;
    ptr = textbuf;
    while ((n = nextword(ptr, " \t\n\r", &word, &delimfound)) >= 0) {
        int wid;
        if ((wid = dict_wordid(d->dict, word)) == BAD_S3WID) {
            E_ERROR("Unknown word %s\n", word);
            ckd_free(textbuf);
            return -1;
        }
        wid = fsg_model_word_add(fsg, word);
        fsg_model_trans_add(fsg, nwords, nwords + 1, 0, wid);
        ptr = word + n;
        *ptr = delimfound;
        ++nwords;
    }
    ckd_free(textbuf);
    fsg->start_state = 0;
    fsg->final_state = nwords;
    if (decoder_set_fsg(d, fsg) < 0) {
        fsg_model_free(fsg);
        return -1;
    }
    return 0;
}

alignment_t *
decoder_alignment(decoder_t *d)
{
    seg_iter_t *seg;
    alignment_t *al;
    frame_idx_t output_frame;
    int prev_ef;

    /* Reuse the existing alignment if nothing has changed. */
    if (d->align) {
        state_align_search_t *align = (state_align_search_t *)d->align;
        if (align->frame == d->acmod->output_frame) {
            E_INFO("Reusing existing alignment at frame %d\n", align->frame);
            return align->al;
        }
    }
    seg = decoder_seg_iter(d);
    if (seg == NULL)
        return NULL;
    al = alignment_init(d->d2p);
    prev_ef = -1;
    while (seg) {
        int32 wid = dict_wordid(d->dict, seg->word);
        /* This is actually possible, because of (null) transitions. */
        if (wid != BAD_S3WID) {
            /* The important thing is that they be continguous. */
            assert(seg->sf == prev_ef + 1);
            prev_ef = seg->ef;
            alignment_add_word(al, wid, seg->sf, seg->ef - seg->sf + 1);
        }
        seg = seg_iter_next(seg);
    }
    if (alignment_populate(al) < 0) {
        /* Not yet owned by the search module so we must free it */
        alignment_free(al);
        return NULL;
    }

    if (d->align)
        search_module_free(d->align);
    d->align = state_align_search_init("_state_align", d->config, d->acmod, al);
    if (d->align == NULL) {
        alignment_free(al);
        return NULL;
    }

    /* Search only up to current output frame (in case this is called for a partial result) */
    output_frame = d->acmod->output_frame;
    if (acmod_rewind(d->acmod) < 0)
        return NULL;
    if (search_module_start(d->align) < 0)
        return NULL;
    while (d->acmod->output_frame < output_frame) {
        if (search_module_step(d->align, d->acmod->output_frame) < 0)
            return NULL;
        acmod_advance(d->acmod);
    }
    if (search_module_finish(d->align) < 0)
        return NULL;

    return al;
}

int
decoder_add_word(decoder_t *d,
                 const char *word,
                 const char *phones,
                 int update)
{
    int32 wid;
    s3cipid_t *pron;
    int np;
    char *phonestr, *ptr;

    assert(word != NULL);
    assert(phones != NULL);
    /* Cannot have more phones than chars... */
    pron = ckd_calloc(1, strlen(phones));
    /* Parse phones into an array of phone IDs. */
    ptr = phonestr = ckd_salloc(phones);
    np = 0;
    while (*ptr) {
        char *phone;
        int final;
        /* Leading whitespace if any */
        while (*ptr && isspace_c(*ptr))
            ++ptr;
        if (*ptr == '\0')
            break;
        phone = ptr;
        while (*ptr && !isspace_c(*ptr))
            ++ptr;
        final = (*ptr == '\0');
        *ptr = '\0';
        pron[np] = bin_mdef_ciphone_id(d->acmod->mdef, phone);
        if (pron[np] == -1) {
            E_ERROR("Unknown phone %s in phone string %s\n",
                    phone, phones);
            ckd_free(phonestr);
            ckd_free(pron);
            return -1;
        }
        ++np;
        if (final)
            break;
        ++ptr;
    }
    ckd_free(phonestr);

    /* Add it to the dictionary. */
    if ((wid = dict_add_word(d->dict, word, pron, np)) == -1) {
        ckd_free(pron);
        return -1;
    }
    ckd_free(pron);

    /* Now we also have to add it to dict2pid. */
    dict2pid_add_word(d->d2p, wid);

    /* Reconfigure the search object, if any. */
    if (d->search && update) {
        /* Note, this is not an error if there is no d->search, we
         * will have updated the dictionary anyway. */
        search_module_reinit(d->search, d->dict, d->d2p);
    }

    /* Rebuild the widmap and search tree if requested. */
    return wid;
}

char *
decoder_lookup_word(decoder_t *d, const char *word)
{
    s3wid_t wid;
    int phlen, j;
    char *phones;
    dict_t *dict = d->dict;

    wid = dict_wordid(dict, word);
    if (wid == BAD_S3WID)
        return NULL;

    for (phlen = j = 0; j < dict_pronlen(dict, wid); ++j)
        phlen += (int)strlen(dict_ciphone_str(dict, wid, j)) + 1;
    phones = ckd_calloc(1, phlen);
    for (j = 0; j < dict_pronlen(dict, wid); ++j) {
        strcat(phones, dict_ciphone_str(dict, wid, j));
        if (j != dict_pronlen(dict, wid) - 1)
            strcat(phones, " ");
    }
    return phones;
}

int
decoder_start_utt(decoder_t *d)
{
    int rv;
    char uttid[16];

    if (d->acmod->state == ACMOD_STARTED || d->acmod->state == ACMOD_PROCESSING) {
        E_ERROR("Utterance already started\n");
        return -1;
    }

    if (d->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }

    ptmr_reset(&d->perf);
    ptmr_start(&d->perf);

    sprintf(uttid, "%09u", d->uttno);
    ++d->uttno;

    /* Remove any residual word lattice and hypothesis. */
    lattice_free(d->search->dag);
    d->search->dag = NULL;
    d->search->last_link = NULL;
    d->search->post = 0;
    ckd_free(d->search->hyp_str);
    d->search->hyp_str = NULL;
    ckd_free(d->json_result);
    d->json_result = NULL;

    /* Remove any state aligner. */
    if (d->align) {
        search_module_free(d->align);
        d->align = NULL;
    }

    if ((rv = acmod_start_utt(d->acmod)) < 0)
        return rv;

    return search_module_start(d->search);
}

static int
search_module_forward(decoder_t *d)
{
    int nfr;

    if (d->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }
    nfr = 0;
    while (d->acmod->n_feat_frame > 0) {
        int k;
        if ((k = search_module_step(d->search,
                                    d->acmod->output_frame))
            < 0)
            return k;
        acmod_advance(d->acmod);
        ++d->n_frame;
        ++nfr;
    }
    return nfr;
}

int
decoder_process_float32(decoder_t *d,
                        float32 *data,
                        size_t n_samples,
                        int no_search,
                        int full_utt)
{
    int n_searchfr = 0;

    if (d->acmod->state == ACMOD_IDLE) {
        E_ERROR("Failed to process data, utterance is not started. Use start_utt to start it\n");
        return 0;
    }

    if (no_search)
        acmod_set_grow(d->acmod, TRUE);

    while (n_samples) {
        int nfr;

        /* Process some data into features. */
        if ((nfr = acmod_process_float32(d->acmod, &data,
                                         &n_samples, full_utt))
            < 0)
            return nfr;

        /* Score and search as much data as possible */
        if (no_search)
            continue;
        if ((nfr = search_module_forward(d)) < 0)
            return nfr;
        n_searchfr += nfr;
    }

    return n_searchfr;
}

int
decoder_process_int16(decoder_t *d,
                      int16 *data,
                      size_t n_samples,
                      int no_search,
                      int full_utt)
{
    int n_searchfr = 0;

    if (d->acmod->state == ACMOD_IDLE) {
        E_ERROR("Failed to process data, utterance is not started. Use start_utt to start it\n");
        return 0;
    }

    if (no_search)
        acmod_set_grow(d->acmod, TRUE);

    while (n_samples) {
        int nfr;

        /* Process some data into features. */
        if ((nfr = acmod_process_raw(d->acmod, &data,
                                     &n_samples, full_utt))
            < 0)
            return nfr;

        /* Score and search as much data as possible */
        if (no_search)
            continue;
        if ((nfr = search_module_forward(d)) < 0)
            return nfr;
        n_searchfr += nfr;
    }

    return n_searchfr;
}

int
decoder_end_utt(decoder_t *d)
{
    int rv = 0;

    if (d->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }
    if (d->acmod->state == ACMOD_ENDED || d->acmod->state == ACMOD_IDLE) {
        E_ERROR("Utterance is not started\n");
        return -1;
    }
    acmod_end_utt(d->acmod);

    /* Search any remaining frames. */
    if ((rv = search_module_forward(d)) < 0) {
        ptmr_stop(&d->perf);
        return rv;
    }
    /* Finish main search. */
    if ((rv = search_module_finish(d->search)) < 0) {
        ptmr_stop(&d->perf);
        return rv;
    }
    ptmr_stop(&d->perf);
    /* Log a backtrace if requested. */
    if (config_bool(d->config, "backtrace")) {
        const char *hyp;
        seg_iter_t *seg;
        int32 score;

        hyp = decoder_hyp(d, &score);

        if (hyp != NULL) {
            E_INFO("%s (%d)\n", hyp, score);
            E_INFO_NOFN("%-20s %-5s %-5s %-5s %-10s %-10s\n",
                        "word", "start", "end", "pprob", "ascr", "lscr");
            for (seg = decoder_seg_iter(d); seg;
                 seg = seg_iter_next(seg)) {
                const char *word;
                int sf, ef;
                int32 post, lscr, ascr;

                word = seg_iter_word(seg);
                seg_iter_frames(seg, &sf, &ef);
                post = seg_iter_prob(seg, &ascr, &lscr);
                E_INFO_NOFN("%-20s %-5d %-5d %-1.3f %-10d %-10d\n",
                            word, sf, ef, logmath_exp(decoder_logmath(d), post),
                            ascr, lscr);
            }
        }
    }
    return rv;
}

const char *
decoder_hyp(decoder_t *d, int32 *out_best_score)
{
    const char *hyp;

    if (d->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return NULL;
    }
    ptmr_start(&d->perf);
    hyp = search_module_hyp(d->search, out_best_score);
    ptmr_stop(&d->perf);
    return hyp;
}

int32
decoder_prob(decoder_t *d)
{
    int32 prob;

    if (d->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }
    ptmr_start(&d->perf);
    prob = search_module_prob(d->search);
    ptmr_stop(&d->perf);
    return prob;
}

seg_iter_t *
decoder_seg_iter(decoder_t *d)
{
    seg_iter_t *itor;

    if (d->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return NULL;
    }
    ptmr_start(&d->perf);
    itor = search_module_seg_iter(d->search);
    ptmr_stop(&d->perf);
    return itor;
}

seg_iter_t *
seg_iter_next(seg_iter_t *seg)
{
    return search_module_seg_next(seg);
}

const char *
seg_iter_word(seg_iter_t *seg)
{
    return seg->word;
}

void
seg_iter_frames(seg_iter_t *seg, int *out_sf, int *out_ef)
{
    if (out_sf)
        *out_sf = seg->sf;
    if (out_ef)
        *out_ef = seg->ef;
}

int32
seg_iter_prob(seg_iter_t *seg, int32 *out_ascr, int32 *out_lscr)
{
    if (out_ascr)
        *out_ascr = seg->ascr;
    if (out_lscr)
        *out_lscr = seg->lscr;
    return seg->prob;
}

void
seg_iter_free(seg_iter_t *seg)
{
    search_module_seg_free(seg);
}

lattice_t *
decoder_lattice(decoder_t *d)
{
    if (d->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return NULL;
    }
    return search_module_lattice(d->search);
}

hyp_iter_t *
decoder_nbest(decoder_t *d)
{
    lattice_t *dag;
    astar_search_t *nbest;

    if (d->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return NULL;
    }
    if ((dag = decoder_lattice(d)) == NULL)
        return NULL;

    nbest = astar_search_start(dag, 0, -1, -1, -1);
    nbest = hyp_iter_next(nbest);

    return (hyp_iter_t *)nbest;
}

void
hyp_iter_free(hyp_iter_t *nbest)
{
    astar_finish(nbest);
}

hyp_iter_t *
hyp_iter_next(hyp_iter_t *nbest)
{
    latpath_t *next;

    next = astar_next(nbest);
    if (next == NULL) {
        hyp_iter_free(nbest);
        return NULL;
    }
    return nbest;
}

const char *
hyp_iter_hyp(hyp_iter_t *nbest, int32 *out_score)
{
    assert(nbest != NULL);

    if (nbest->top == NULL)
        return NULL;
    if (out_score)
        *out_score = nbest->top->score;
    return astar_hyp(nbest, nbest->top);
}

seg_iter_t *
hyp_iter_seg(hyp_iter_t *nbest)
{
    if (nbest->top == NULL)
        return NULL;

    return astar_search_seg_iter(nbest, nbest->top);
}

int
decoder_n_frames(decoder_t *d)
{
    return d->acmod->output_frame + 1;
}

void
decoder_utt_time(decoder_t *d, double *out_nspeech,
                 double *out_ncpu, double *out_nwall)
{
    int32 frate;

    frate = config_int(d->config, "frate");
    *out_nspeech = (double)d->acmod->output_frame / frate;
    *out_ncpu = d->perf.t_cpu;
    *out_nwall = d->perf.t_elapsed;
}

void
decoder_all_time(decoder_t *d, double *out_nspeech,
                 double *out_ncpu, double *out_nwall)
{
    int32 frate;

    frate = config_int(d->config, "frate");
    *out_nspeech = (double)d->n_frame / frate;
    *out_ncpu = d->perf.t_tot_cpu;
    *out_nwall = d->perf.t_tot_elapsed;
}

void
search_module_init(search_module_t *search, searchfuncs_t *vt,
                   const char *type,
                   const char *name,
                   config_t *config, acmod_t *acmod, dict_t *dict,
                   dict2pid_t *d2p)
{
    search->vt = vt;
    search->name = ckd_salloc(name);
    search->type = ckd_salloc(type);

    /* Search modules are (hopefully) invisible to the user and won't
     * live longer than the decoder, so no need to retain anything
     * here. */
    search->config = config;
    search->acmod = acmod;
    if (d2p)
        search->d2p = d2p;
    else
        search->d2p = NULL;
    if (dict) {
        search->dict = dict;
        search->start_wid = dict_startwid(dict);
        search->finish_wid = dict_finishwid(dict);
        search->silence_wid = dict_silwid(dict);
        search->n_words = dict_size(dict);
    } else {
        search->dict = NULL;
        search->start_wid = search->finish_wid = search->silence_wid = -1;
        search->n_words = 0;
    }
}

void
search_module_base_free(search_module_t *search)
{
    ckd_free(search->name);
    ckd_free(search->type);
    ckd_free(search->hyp_str);
    lattice_free(search->dag);
}

void
search_module_base_reinit(search_module_t *search, dict_t *dict,
                          dict2pid_t *d2p)
{
    if (dict) {
        search->dict = dict;
        search->start_wid = dict_startwid(dict);
        search->finish_wid = dict_finishwid(dict);
        search->silence_wid = dict_silwid(dict);
        search->n_words = dict_size(dict);
    } else {
        search->dict = NULL;
        search->start_wid = search->finish_wid = search->silence_wid = -1;
        search->n_words = 0;
    }
    if (d2p)
        search->d2p = d2p;
    else
        search->d2p = NULL;
}

#define HYP_FORMAT "{\"b\":%.3f,\"d\":%.3f,\"p\":%.3f,\"t\":\"%s\""
static int
format_hyp(char *outptr, int len, decoder_t *decoder, double start, double duration)
{
    logmath_t *lmath;
    double prob;
    const char *hyp;

    lmath = decoder_logmath(decoder);
    prob = logmath_exp(lmath, decoder_prob(decoder));
    hyp = decoder_hyp(decoder, NULL);
    if (hyp == NULL)
        hyp = "";
    return snprintf(outptr, len, HYP_FORMAT, start, duration, prob, hyp);
}

static int
format_seg(char *outptr, int len, seg_iter_t *seg,
           double utt_start, int frate,
           logmath_t *lmath)
{
    double prob, st, dur;
    int sf, ef;
    const char *word;

    seg_iter_frames(seg, &sf, &ef);
    st = utt_start + (double)sf / frate;
    dur = (double)(ef + 1 - sf) / frate;
    word = seg_iter_word(seg);
    if (word == NULL)
        word = "";
    prob = logmath_exp(lmath, seg_iter_prob(seg, NULL, NULL));
    len = snprintf(outptr, len, HYP_FORMAT, st, dur, prob, word);
    if (outptr) {
        outptr += len;
        *outptr++ = '}';
        *outptr = '\0';
    }
    len++;
    return len;
}

static int
format_align_iter(char *outptr, int maxlen,
                  alignment_iter_t *itor, double utt_start, int frate, logmath_t *lmath)
{
    int start, duration, score;
    double prob, st, dur;
    const char *word;

    score = alignment_iter_seg(itor, &start, &duration);
    st = utt_start + (double)start / frate;
    dur = (double)duration / frate;
    prob = logmath_exp(lmath, score);
    word = alignment_iter_name(itor);
    if (word == NULL)
        word = "";

    return snprintf(outptr, maxlen, HYP_FORMAT, st, dur, prob, word);
}

static int
format_seg_align(char *outptr, int maxlen,
                 alignment_iter_t *itor,
                 double utt_start, int frate,
                 logmath_t *lmath, int state_align)
{
    alignment_iter_t *pitor;
    int len = 0, hyplen;

    hyplen = format_align_iter(outptr, maxlen,
                               itor, utt_start, frate, lmath);
    len += hyplen;
    if (outptr)
        outptr += hyplen;
    if (maxlen)
        maxlen -= hyplen;

    len += 6; /* "w":,[ */
    if (outptr) {
        memcpy(outptr, ",\"w\":[", 6);
        outptr += 6;
    }
    if (maxlen)
        maxlen -= 6;

    pitor = alignment_iter_children(itor);
    while (pitor != NULL) {
        hyplen = format_align_iter(outptr, maxlen,
                                   pitor, utt_start, frate, lmath);
        len += hyplen;
        if (outptr)
            outptr += hyplen;
        if (maxlen)
            maxlen -= hyplen;

        /* FIXME: refactor with recursion, someday */
        if (state_align) {
            alignment_iter_t *sitor = alignment_iter_children(pitor);
            len += 6; /* "w":,[ */
            if (outptr) {
                memcpy(outptr, ",\"w\":[", 6);
                outptr += 6;
            }
            if (maxlen)
                maxlen -= 6;
            while (sitor != NULL) {
                hyplen = format_align_iter(outptr, maxlen,
                                           sitor, utt_start, frate, lmath);
                len += hyplen;
                if (outptr)
                    outptr += hyplen;
                if (maxlen)
                    maxlen -= hyplen;

                len++; /* } */
                if (outptr)
                    *outptr++ = '}';
                if (maxlen)
                    maxlen--;
                sitor = alignment_iter_next(sitor);
                if (sitor != NULL) {
                    len++;
                    if (outptr)
                        *outptr++ = ',';
                    if (maxlen)
                        maxlen--;
                }
            }
            len++;
            if (outptr)
                *outptr++ = ']';
            if (maxlen)
                maxlen--;
        }

        len++; /* } */
        if (outptr)
            *outptr++ = '}';
        if (maxlen)
            maxlen--;
        pitor = alignment_iter_next(pitor);
        if (pitor != NULL) {
            len++;
            if (outptr)
                *outptr++ = ',';
            if (maxlen)
                maxlen--;
        }
    }

    len += 2;
    if (outptr) {
        *outptr++ = ']';
        *outptr++ = '}';
        *outptr = '\0';
    }
    if (maxlen)
        maxlen--;

    return len;
}

const char *
decoder_result_json(decoder_t *d, double start, int align_level)
{
    logmath_t *lmath = decoder_logmath(d);
    int state_align = (align_level > 1);
    alignment_t *alignment = NULL;
    char *ptr;
    int frate;
    int maxlen, len;
    double duration;

    if (align_level) {
        alignment = decoder_alignment(d);
        if (alignment == NULL)
            return NULL;
    }
    frate = config_int(decoder_config(d), "frate");
    duration = (double)decoder_n_frames(d) / frate;
    maxlen = format_hyp(NULL, 0, d, start, duration);
    maxlen += 6; /* "w":,[ */
    if (alignment) {
        alignment_iter_t *itor = alignment_words(alignment);
        if (itor == NULL)
            maxlen++; /* ] at end */
        for (; itor; itor = alignment_iter_next(itor)) {
            maxlen += format_seg_align(NULL, 0, itor, start, frate,
                                       lmath, state_align);
            maxlen++; /* , or ] at end */
        }
    } else {
        seg_iter_t *itor = decoder_seg_iter(d);
        if (itor == NULL)
            maxlen++; /* ] at end */
        for (; itor; itor = seg_iter_next(itor)) {
            maxlen += format_seg(NULL, 0, itor, start, frate, lmath);
            maxlen++; /* , or ] at end */
        }
    }
    maxlen++; /* final } */
    maxlen++; /* trailing \n */
    maxlen++; /* trailing \0 */

    ckd_free(d->json_result);
    ptr = d->json_result = ckd_calloc(maxlen, 1);
    len = maxlen;
    len = format_hyp(d->json_result, len, d, start, duration);
    ptr += len;
    maxlen -= len;

    assert(maxlen > 6);
    memcpy(ptr, ",\"w\":[", 6);
    ptr += 6;
    maxlen -= 6;

    if (alignment) {
        alignment_iter_t *itor;
        for (itor = alignment_words(alignment); itor;
             itor = alignment_iter_next(itor)) {
            assert(maxlen > 0);
            len = format_seg_align(ptr, maxlen, itor, start, frate, lmath,
                                   state_align);
            ptr += len;
            maxlen -= len;
            *ptr++ = ',';
            maxlen--;
        }
    } else {
        seg_iter_t *itor = decoder_seg_iter(d);
        if (itor == NULL) {
            *ptr++ = ']'; /* Gets overwritten below... */
            maxlen--;
        }
        for (; itor; itor = seg_iter_next(itor)) {
            assert(maxlen > 0);
            len = format_seg(ptr, maxlen, itor, start, frate, lmath);
            ptr += len;
            maxlen -= len;
            *ptr++ = ',';
            maxlen--;
        }
    }
    --ptr;
    *ptr++ = ']';
    assert(maxlen == 3);
    *ptr++ = '}';
    --maxlen;
    *ptr++ = '\n';
    --maxlen;
    *ptr = '\0';

    return d->json_result;
}

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

#include <stdio.h>
#include <assert.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <soundswallower/err.h>
#include <soundswallower/strfuncs.h>
#include <soundswallower/filename.h>
#include <soundswallower/acmod.h>
#include <soundswallower/jsgf.h>
#include <soundswallower/hash_table.h>
#include <soundswallower/config_defs.h>
#include <soundswallower/decoder.h>
#include <soundswallower/fsg_search.h>
#include <soundswallower/search_module.h>

static void
decoder_free_searches(decoder_t *d)
{
    if (d->search) {
        search_module_free(d->search);
        d->search = NULL;
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
    (void) lvl;
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
    /* Set up logging. We do this immediately because we want to dump
       the information to the configured log, not to the stderr. */
    if (config && config != d->config) {
        if (set_loglevel(config) < 0)
            return -1;
        set_logfile(d, config);
        config_free(d->config);
        /* Note! Consuming semantics. */
        d->config = config;
    }
    
    /* Print out the config for logging. */
    config_log_values(d->config);
    
    /* Logmath computation (used in acmod and search) */
    if (d->lmath == NULL
        || (logmath_get_base(d->lmath) !=
            (float64)config_float(d->config, "logbase"))) {
        if (d->lmath)
            logmath_free(d->lmath);
        d->lmath = logmath_init
            ((float64)config_float(d->config, "logbase"), 0, TRUE);
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
    }
    else if ((path = config_str(d->config, "fsg"))) {
        fsg_model_t *fsg = fsg_model_readfile(path, d->lmath, lw);
        if (!fsg)
            return -1;
        if (decoder_set_fsg(d, fsg) != 0) {
            fsg_model_free(fsg);
            return -1;
        }
    }
    return  0;
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
    
    return  0;
}

fe_t *
decoder_reinit_fe(decoder_t *d, config_t *config)
{
    fe_t *new_fe;
    
    if (config && config != d->config) {
        config_free(d->config);
        /* NOTE! Consuming semantics */
        d->config = config;
    }
    if ((new_fe = fe_init(d->config)) == NULL)
        return NULL;
    if (acmod_fe_mismatch(d->acmod, new_fe)) {
        fe_free(new_fe);
        return NULL;
    }
    fe_free(d->fe);
    d->fe = new_fe;
    /* FIXME: should be in an acmod_set_fe function */
    fe_free(d->acmod->fe);
    d->acmod->fe = fe_retain(d->fe);

    return d->fe;
}

int
decoder_reinit(decoder_t *d, config_t *config)
{
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

decoder_t *
decoder_init(config_t *config)
{
    decoder_t *d;
    
    d = ckd_calloc(1, sizeof(*d));
    d->refcount = 1;
    if (config) {
#ifdef __EMSCRIPTEN__
        E_WARN("decoder_init(config) does nothing in JavaScript\n");
#else
        if (decoder_reinit(d, config) < 0) {
            decoder_free(d);
            return NULL;
        }
#endif
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
    char const *toprule;
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
    char const *toprule;
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
decoder_add_word(decoder_t *d,
                 char const *word,
                 char const *phones,
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
                                    d->acmod->output_frame)) < 0)
            return k;
        acmod_advance(d->acmod);
        ++d->n_frame;
        ++nfr;
    }
    return nfr;
}

int
decoder_process_float32(decoder_t *d,
                        float32 const *data,
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
                                         &n_samples, full_utt)) < 0)
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
decoder_process_raw(decoder_t *d,
                    int16 const *data,
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
                                     &n_samples, full_utt)) < 0)
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
decoder_process_cep(decoder_t *d,
                    mfcc_t **data,
                    int32 n_frames,
                    int no_search,
                    int full_utt)
{
    int n_searchfr = 0;

    if (no_search)
        acmod_set_grow(d->acmod, TRUE);

    while (n_frames) {
        int nfr;

        /* Process some data into features. */
        if ((nfr = acmod_process_cep(d->acmod, &data,
                                     &n_frames, full_utt)) < 0)
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
        const char* hyp;
        seg_iter_t *seg;
        int32 score;

        hyp = decoder_hyp(d, &score);
        
        if (hyp != NULL) {
    	    E_INFO("%s (%d)\n", hyp, score);
    	    E_INFO_NOFN("%-20s %-5s %-5s %-5s %-10s %-10s\n",
                        "word", "start", "end", "pprob", "ascr", "lscr");
    	    for (seg = decoder_seg_iter(d); seg;
        	 seg = seg_iter_next(seg)) {
    	        char const *word;
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

char const *
decoder_hyp(decoder_t *d, int32 *out_best_score)
{
    char const *hyp;

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

char const *
seg_iter_word(seg_iter_t *seg)
{
    return seg->word;
}

void
seg_iter_frames(seg_iter_t *seg, int *out_sf, int *out_ef)
{
    if (out_sf) *out_sf = seg->sf;
    if (out_ef) *out_ef = seg->ef;
}

int32
seg_iter_prob(seg_iter_t *seg, int32 *out_ascr, int32 *out_lscr)
{
    if (out_ascr) *out_ascr = seg->ascr;
    if (out_lscr) *out_lscr = seg->lscr;
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

char const *
hyp_iter_hyp(hyp_iter_t *nbest, int32 *out_score)
{
    assert(nbest != NULL);

    if (nbest->top == NULL)
        return NULL;
    if (out_score) *out_score = nbest->top->score;
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
    }
    else {
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
    }
    else {
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
int
format_hyp(char *outptr, int len, decoder_t *decoder, double start, double duration)
{
    logmath_t *lmath;
    double prob;
    const char *hyp;

    lmath = decoder_logmath(decoder);
    prob = logmath_exp(lmath, decoder_prob(decoder));
    if (duration == 0.0) {
        start = 0.0;
        duration = (double)decoder_n_frames(decoder)
            / config_int(decoder_config(decoder), "frate");
    }
    hyp = decoder_hyp(decoder, NULL);
    if (hyp == NULL)
        hyp = "";
    return snprintf(outptr, len, HYP_FORMAT, start, duration, prob, hyp);
}

int
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

int
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

int
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

void
output_hyp(decoder_t *decoder, alignment_t *alignment, double start, double duration)
{
    logmath_t *lmath;
    char *hyp_json, *ptr;
    int frate;
    int maxlen, len;
    int state_align = config_bool(decoder->config, "state_align");

    maxlen = format_hyp(NULL, 0, decoder, start, duration);
    maxlen += 6; /* "w":,[ */
    lmath = decoder_logmath(decoder);
    frate = config_int(decoder_config(decoder), "frate");
    if (alignment) {
        alignment_iter_t *itor = alignment_words(alignment);
        if (itor == NULL)
            maxlen++; /* ] at end */
        for (; itor; itor = alignment_iter_next(itor)) {
            maxlen += format_seg_align(NULL, 0, itor, start, frate,
                                       lmath, state_align);
            maxlen++; /* , or ] at end */
        }
    }
    else {
        seg_iter_t *itor = decoder_seg_iter(decoder);
        if (itor == NULL)
            maxlen++; /* ] at end */
        for (; itor; itor = seg_iter_next(itor)) {
            maxlen += format_seg(NULL, 0, itor, start, frate, lmath);
            maxlen++; /* , or ] at end */
        }
    }
    maxlen++; /* final } */
    maxlen++; /* trailing \0 */

    ptr = hyp_json = ckd_calloc(maxlen, 1);
    len = maxlen;
    len = format_hyp(hyp_json, len, decoder, start, duration);
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
    }
    else {
        seg_iter_t *itor = decoder_seg_iter(decoder);
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
    assert(maxlen == 2);
    *ptr++ = '}';
    --maxlen;
    *ptr = '\0';
    puts(hyp_json);
    ckd_free(hyp_json);
}

static int sample_rates[] = {
    8000,
    11025,
    16000,
    22050,
    32000,
    44100,
    48000
};
static const int n_sample_rates = sizeof(sample_rates)/sizeof(sample_rates[0]);

int
minimum_samprate(config_t *config)
{
    double upperf = config_float(config, "upperf");
    int nyquist = (int)(upperf * 2);
    int i;
    for (i = 0; i < n_sample_rates; ++i)
        if (sample_rates[i] >= nyquist)
            break;
    if (i == n_sample_rates)
        E_FATAL("Unable to find sampling rate for -upperf %f\n", upperf);
    return sample_rates[i];
}

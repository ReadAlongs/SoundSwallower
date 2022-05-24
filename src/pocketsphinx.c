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

/* SphinxBase headers. */
#include <soundswallower/export.h>
#include <soundswallower/err.h>
#include <soundswallower/strfuncs.h>
#include <soundswallower/filename.h>
#include <soundswallower/acmod.h>
#include <soundswallower/pio.h>
#include <soundswallower/jsgf.h>
#include <soundswallower/hash_table.h>
#include <soundswallower/cmdln_macro.h>
#include <soundswallower/pocketsphinx.h>

/* Local headers. */
#include <soundswallower/pocketsphinx_internal.h>
#include <soundswallower/ps_lattice_internal.h>
#include <soundswallower/fsg_search_internal.h>

static const arg_t ps_args_def[] = {
    POCKETSPHINX_OPTIONS,
    CMDLN_EMPTY_OPTION
};

/* I'm not sure what the portable way to do this is. */
static int
file_exists(const char *path)
{
    FILE *tmp;

    tmp = fopen(path, "rb");
    if (tmp) fclose(tmp);
    return (tmp != NULL);
}

static void
ps_expand_file_config(ps_decoder_t *ps, const char *arg, const char *extra_arg,
	              const char *hmmdir, const char *file)
{
    const char *val;
    if ((val = cmd_ln_str_r(ps->config, arg)) != NULL) {
	cmd_ln_set_str_extra_r(ps->config, extra_arg, val);
    } else if (hmmdir == NULL) {
        cmd_ln_set_str_extra_r(ps->config, extra_arg, NULL);
    } else {
        char *tmp = string_join(hmmdir, "/", file, NULL);
        if (file_exists(tmp))
	    cmd_ln_set_str_extra_r(ps->config, extra_arg, tmp);
	else
	    cmd_ln_set_str_extra_r(ps->config, extra_arg, NULL);
        ckd_free(tmp);
    }
}

/* Feature and front-end parameters that may be in feat.params */
static const arg_t feat_defn[] = {
    waveform_to_cepstral_command_line_macro(),
    cepstral_to_feature_command_line_macro(),
    CMDLN_EMPTY_OPTION
};

static void
ps_expand_model_config(ps_decoder_t *ps)
{
    char const *hmmdir, *featparams;

    /* Get acoustic model filenames and add them to the command-line */
    hmmdir = cmd_ln_str_r(ps->config, "-hmm");
    ps_expand_file_config(ps, "-mdef", "_mdef", hmmdir, "mdef");
    ps_expand_file_config(ps, "-mean", "_mean", hmmdir, "means");
    ps_expand_file_config(ps, "-var", "_var", hmmdir, "variances");
    ps_expand_file_config(ps, "-tmat", "_tmat", hmmdir, "transition_matrices");
    ps_expand_file_config(ps, "-mixw", "_mixw", hmmdir, "mixture_weights");
    ps_expand_file_config(ps, "-sendump", "_sendump", hmmdir, "sendump");
    ps_expand_file_config(ps, "-fdict", "_fdict", hmmdir, "noisedict");
    ps_expand_file_config(ps, "-featparams", "_featparams", hmmdir, "feat.params");
    ps_expand_file_config(ps, "-senmgau", "_senmgau", hmmdir, "senmgau");
    ps_expand_file_config(ps, "-dict", "_dict", hmmdir, "dict.txt");

    /* Look for feat.params in acoustic model dir. */
    if ((featparams = cmd_ln_str_r(ps->config, "_featparams"))) {
        if (NULL !=
            cmd_ln_parse_file_r(ps->config, feat_defn, featparams, FALSE))
            E_INFO("Parsed model-specific feature parameters from %s\n",
                    featparams);
    }
}

static void
ps_free_searches(ps_decoder_t *ps)
{
    if (ps->search) {
        ps_search_free(ps->search);
        ps->search = NULL;
    }
}

EXPORT int
ps_init_config(ps_decoder_t *ps, cmd_ln_t *config)
{
    if (config && config != ps->config) {
        cmd_ln_free_r(ps->config);
        ps->config = cmd_ln_retain(config);
    }

    /* Set up logging. We need to do this earlier because we want to dump
     * the information to the configured log, not to the stderr. */
    if (config) {
        const char *logfn, *loglevel;
        logfn = cmd_ln_str_r(ps->config, "-logfn");
        if (logfn) {
            if (err_set_logfile(logfn) < 0) {
                E_ERROR("Cannot redirect log output\n");
                return -1;
            }
        }
        loglevel = cmd_ln_str_r(ps->config, "-loglevel");
        if (loglevel) {
            if (err_set_loglevel_str(loglevel) == NULL) {
                E_ERROR("Invalid log level: %s\n", loglevel);
                return -1;
            }
        }
    }
    
    ps->mfclogdir = cmd_ln_str_r(ps->config, "-mfclogdir");
    ps->rawlogdir = cmd_ln_str_r(ps->config, "-rawlogdir");
    ps->senlogdir = cmd_ln_str_r(ps->config, "-senlogdir");

    /* Fill in some default arguments. */
    ps_expand_model_config(ps);

    /* Print out the config for logging. */
    cmd_ln_log_values_r(ps->config, ps_args());
    
    /* Logmath computation (used in acmod and search) */
    if (ps->lmath == NULL
        || (logmath_get_base(ps->lmath) !=
            (float64)cmd_ln_float32_r(ps->config, "-logbase"))) {
        if (ps->lmath)
            logmath_free(ps->lmath);
        ps->lmath = logmath_init
            ((float64)cmd_ln_float32_r(ps->config, "-logbase"), 0, TRUE);
    }

    /* Initialize performance timer. */
    ps->perf.name = "decode";
    ptmr_init(&ps->perf);

    return 0;
}

EXPORT int
ps_init_cleanup(ps_decoder_t *ps)
{
    /* Free old searches (do this before other reinit) */
    ps_free_searches(ps);

    return 0;
}

EXPORT acmod_t *
ps_init_acmod(ps_decoder_t *ps)
{
    if (ps->config == NULL)
        return NULL;
    if (ps->lmath == NULL)
        return NULL;
    /* Free old acmod. */
    acmod_free(ps->acmod);
    /* Acoustic model (this is basically everything that
     * uttproc.c, senscr.c, and others used to do) */
    ps->acmod = acmod_init(ps->config, ps->lmath, NULL, NULL);
    return ps->acmod;
}

EXPORT dict_t *
ps_init_dict(ps_decoder_t *ps)
{
    if (ps->config == NULL)
        return NULL;
    if (ps->acmod == NULL)
        return NULL;
    /* Free old dictionary */
    dict_free(ps->dict);
    /* Free d2p */
    dict2pid_free(ps->d2p);
    /* Dictionary and triphone mappings (depends on acmod). */
    /* FIXME: pass config, change arguments, implement LTS, etc. */
    if ((ps->dict = dict_init(ps->config, ps->acmod->mdef)) == NULL)
        return NULL;
    if ((ps->d2p = dict2pid_build(ps->acmod->mdef, ps->dict)) == NULL)
        return NULL;
    return ps->dict;
}

EXPORT int
ps_init_grammar(ps_decoder_t *ps)
{
    const char *path;
    int32 lw;

    lw = cmd_ln_float32_r(ps->config, "-lw");

    /* Load an FSG if one was specified in config */
    if ((path = cmd_ln_str_r(ps->config, "-fsg"))) {
        fsg_model_t *fsg = fsg_model_readfile(path, ps->lmath, lw);
        if (!fsg)
            return -1;
        if (ps_set_fsg(ps, PS_DEFAULT_SEARCH, fsg) != 0) {
            fsg_model_free(fsg);
            return -1;
        }
        fsg_model_free(fsg);
    }
    
    /* Or load a JSGF grammar */
    if ((path = cmd_ln_str_r(ps->config, "-jsgf"))) {
        if (ps_set_jsgf_file(ps, PS_DEFAULT_SEARCH, path) != 0)
            return -1;
    }
    return  0;
}

EXPORT fe_t *
ps_reinit_fe(ps_decoder_t *ps, cmd_ln_t *config)
{
    fe_t *new_fe;
    
    if (config && config != ps->config) {
        cmd_ln_free_r(ps->config);
        ps->config = cmd_ln_retain(config);
    }
    if ((new_fe = fe_init_auto_r(ps->config)) == NULL)
        return NULL;
    if (acmod_fe_mismatch(ps->acmod, new_fe)) {
        fe_free(new_fe);
        return NULL;
    }
    fe_free(ps->acmod->fe);
    ps->acmod->fe = new_fe;
    return new_fe;
}

EXPORT int
ps_reinit(ps_decoder_t *ps, cmd_ln_t *config)
{
    if (ps_init_config(ps, config) < 0)
        return -1;

    if (ps_init_cleanup(ps) < 0)
        return -1;

    if (ps_init_acmod(ps) == NULL)
        return -1;

    if (ps_init_dict(ps) == NULL)
        return -1;
    
    if (ps_init_grammar(ps) < 0)
        return -1;
    
    return 0;
}

EXPORT ps_decoder_t *
ps_init(cmd_ln_t *config)
{
    ps_decoder_t *ps;
    
    ps = ckd_calloc(1, sizeof(*ps));
    ps->refcount = 1;
    if (config) {
        if (ps_reinit(ps, config) < 0) {
            ps_free(ps);
            return NULL;
        }
    }
    return ps;
}

EXPORT arg_t const *
ps_args(void)
{
    return ps_args_def;
}

EXPORT ps_decoder_t *
ps_retain(ps_decoder_t *ps)
{
    ++ps->refcount;
    return ps;
}

EXPORT int
ps_free(ps_decoder_t *ps)
{
    if (ps == NULL)
        return 0;
    if (--ps->refcount > 0)
        return ps->refcount;
    ps_free_searches(ps);
    dict_free(ps->dict);
    dict2pid_free(ps->d2p);
    acmod_free(ps->acmod);
    logmath_free(ps->lmath);
    cmd_ln_free_r(ps->config);
    ckd_free(ps);
    return 0;
}

EXPORT cmd_ln_t *
ps_get_config(ps_decoder_t *ps)
{
    return ps->config;
}

EXPORT logmath_t *
ps_get_logmath(ps_decoder_t *ps)
{
    return ps->lmath;
}

fe_t *
ps_get_fe(ps_decoder_t *ps)
{
    return ps->acmod->fe;
}

feat_t *
ps_get_feat(ps_decoder_t *ps)
{
    return ps->acmod->fcb;
}

ps_mllr_t *
ps_update_mllr(ps_decoder_t *ps, ps_mllr_t *mllr)
{
    return acmod_update_mllr(ps->acmod, mllr);
}

EXPORT int
ps_set_fsg(ps_decoder_t *ps, const char *name, fsg_model_t *fsg)
{
    ps_search_t *search;
    search = fsg_search_init(name, fsg, ps->config, ps->acmod, ps->dict, ps->d2p);
    if (search == NULL)
        return -1;
    if (ps->search)
        ps_search_free(ps->search);
    ps->search = search;
    return 0;
}

int 
ps_set_jsgf_file(ps_decoder_t *ps, const char *name, const char *path)
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
  if ((toprule = cmd_ln_str_r(ps->config, "-toprule"))) {
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

  lw = cmd_ln_float32_r(ps->config, "-lw");
  fsg = jsgf_build_fsg(jsgf, rule, ps->lmath, lw);
  result = ps_set_fsg(ps, name, fsg);
  fsg_model_free(fsg);
  jsgf_grammar_free(jsgf);
  return result;
}

EXPORT int 
ps_set_jsgf_string(ps_decoder_t *ps, const char *name, const char *jsgf_string)
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
  if ((toprule = cmd_ln_str_r(ps->config, "-toprule"))) {
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

  lw = cmd_ln_float32_r(ps->config, "-lw");
  fsg = jsgf_build_fsg(jsgf, rule, ps->lmath, lw);
  result = ps_set_fsg(ps, name, fsg);
  fsg_model_free(fsg);
  jsgf_grammar_free(jsgf);
  return result;
}

EXPORT int
ps_add_word(ps_decoder_t *ps,
            char const *word,
            char const *phones,
            int update)
{
    int32 wid;
    s3cipid_t *pron;
    char **phonestr, *tmp;
    int np, i;

    /* Parse phones into an array of phone IDs. */
    tmp = ckd_salloc(phones);
    np = str2words(tmp, NULL, 0);
    phonestr = ckd_calloc(np, sizeof(*phonestr));
    str2words(tmp, phonestr, np);
    pron = ckd_calloc(np, sizeof(*pron));
    for (i = 0; i < np; ++i) {
        pron[i] = bin_mdef_ciphone_id(ps->acmod->mdef, phonestr[i]);
        if (pron[i] == -1) {
            E_ERROR("Unknown phone %s in phone string %s\n",
                    phonestr[i], tmp);
            ckd_free(phonestr);
            ckd_free(tmp);
            ckd_free(pron);
            return -1;
        }
    }
    /* No longer needed. */
    ckd_free(phonestr);
    ckd_free(tmp);

    /* Add it to the dictionary. */
    if ((wid = dict_add_word(ps->dict, word, pron, np)) == -1) {
        ckd_free(pron);
        return -1;
    }
    /* No longer needed. */
    ckd_free(pron);

    /* Now we also have to add it to dict2pid. */
    dict2pid_add_word(ps->d2p, wid);

    /* Reconfigure the search object, if any. */
    if (ps->search && update) {
        /* Note, this is not an error if there is no ps->search, we
         * will have updated the dictionary anyway. */
        ps_search_reinit(ps->search, ps->dict, ps->d2p);
    }

    /* Rebuild the widmap and search tree if requested. */
    return wid;
}

EXPORT char *
ps_lookup_word(ps_decoder_t *ps, const char *word)
{
    s3wid_t wid;
    int32 phlen, j;
    char *phones;
    dict_t *dict = ps->dict;
    
    wid = dict_wordid(dict, word);
    if (wid == BAD_S3WID)
	return NULL;

    for (phlen = j = 0; j < dict_pronlen(dict, wid); ++j)
        phlen += strlen(dict_ciphone_str(dict, wid, j)) + 1;
    phones = ckd_calloc(1, phlen);
    for (j = 0; j < dict_pronlen(dict, wid); ++j) {
        strcat(phones, dict_ciphone_str(dict, wid, j));
        if (j != dict_pronlen(dict, wid) - 1)
            strcat(phones, " ");
    }
    return phones;
}

long
ps_decode_raw(ps_decoder_t *ps, FILE *rawfh,
              long maxsamps)
{
    int16 *data;
    long total, pos, endpos;

    ps_start_utt(ps);

    /* If this file is seekable or maxsamps is specified, then decode
     * the whole thing at once. */
    if (maxsamps != -1) {
        data = ckd_calloc(maxsamps, sizeof(*data));
        total = fread(data, sizeof(*data), maxsamps, rawfh);
        ps_process_raw(ps, data, total, FALSE, TRUE);
        ckd_free(data);
    } else if ((pos = ftell(rawfh)) >= 0) {
        fseek(rawfh, 0, SEEK_END);
        endpos = ftell(rawfh);
        fseek(rawfh, pos, SEEK_SET);
        maxsamps = endpos - pos;

        data = ckd_calloc(maxsamps, sizeof(*data));
        total = fread(data, sizeof(*data), maxsamps, rawfh);
        ps_process_raw(ps, data, total, FALSE, TRUE);
        ckd_free(data);
    } else {
        /* Otherwise decode it in a stream. */
        total = 0;
        while (!feof(rawfh)) {
            int16 data[256];
            size_t nread;

            nread = fread(data, sizeof(*data), sizeof(data)/sizeof(*data), rawfh);
            ps_process_raw(ps, data, nread, FALSE, FALSE);
            total += nread;
        }
    }
    ps_end_utt(ps);
    return total;
}

EXPORT int
ps_start_utt(ps_decoder_t *ps)
{
    int rv;
    char uttid[16];
    
    if (ps->acmod->state == ACMOD_STARTED || ps->acmod->state == ACMOD_PROCESSING) {
	E_ERROR("Utterance already started\n");
	return -1;
    }

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }

    ptmr_reset(&ps->perf);
    ptmr_start(&ps->perf);

    sprintf(uttid, "%09u", ps->uttno);
    ++ps->uttno;

    /* Remove any residual word lattice and hypothesis. */
    ps_lattice_free(ps->search->dag);
    ps->search->dag = NULL;
    ps->search->last_link = NULL;
    ps->search->post = 0;
    ckd_free(ps->search->hyp_str);
    ps->search->hyp_str = NULL;

    if ((rv = acmod_start_utt(ps->acmod)) < 0)
        return rv;

    /* Start logging features and audio if requested. */
    if (ps->mfclogdir) {
        char *logfn = string_join(ps->mfclogdir, "/",
                                  uttid, ".mfc", NULL);
        FILE *mfcfh;
        E_INFO("Writing MFCC file: %s\n", logfn);
        if ((mfcfh = fopen(logfn, "wb")) == NULL) {
            E_ERROR_SYSTEM("Failed to open MFCC file %s", logfn);
            ckd_free(logfn);
            return -1;
        }
        ckd_free(logfn);
        acmod_set_mfcfh(ps->acmod, mfcfh);
    }
    if (ps->rawlogdir) {
        char *logfn = string_join(ps->rawlogdir, "/",
                                  uttid, ".raw", NULL);
        FILE *rawfh;
        E_INFO("Writing raw audio file: %s\n", logfn);
        if ((rawfh = fopen(logfn, "wb")) == NULL) {
            E_ERROR_SYSTEM("Failed to open raw audio file %s", logfn);
            ckd_free(logfn);
            return -1;
        }
        ckd_free(logfn);
        acmod_set_rawfh(ps->acmod, rawfh);
    }
    if (ps->senlogdir) {
        char *logfn = string_join(ps->senlogdir, "/",
                                  uttid, ".sen", NULL);
        FILE *senfh;
        E_INFO("Writing senone score file: %s\n", logfn);
        if ((senfh = fopen(logfn, "wb")) == NULL) {
            E_ERROR_SYSTEM("Failed to open senone score file %s", logfn);
            ckd_free(logfn);
            return -1;
        }
        ckd_free(logfn);
        acmod_set_senfh(ps->acmod, senfh);
    }

    return ps_search_start(ps->search);
}

static int
ps_search_forward(ps_decoder_t *ps)
{
    int nfr;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }
    nfr = 0;
    while (ps->acmod->n_feat_frame > 0) {
        int k;
        if ((k = ps_search_step(ps->search,
                                ps->acmod->output_frame)) < 0)
            return k;
        acmod_advance(ps->acmod);
        ++ps->n_frame;
        ++nfr;
    }
    return nfr;
}

int
ps_decode_senscr(ps_decoder_t *ps, FILE *senfh)
{
    int nfr, n_searchfr;

    ps_start_utt(ps);
    n_searchfr = 0;
    acmod_set_insenfh(ps->acmod, senfh);
    while ((nfr = acmod_read_scores(ps->acmod)) > 0) {
        if ((nfr = ps_search_forward(ps)) < 0) {
            ps_end_utt(ps);
            return nfr;
        }
        n_searchfr += nfr;
    }
    ps_end_utt(ps);
    acmod_set_insenfh(ps->acmod, NULL);

    return n_searchfr;
}

EXPORT int
ps_process_float32(ps_decoder_t *ps,
                   float32 const *data,
                   size_t n_samples,
                   int no_search,
                   int full_utt)
{
    int n_searchfr = 0;

    if (ps->acmod->state == ACMOD_IDLE) {
	E_ERROR("Failed to process data, utterance is not started. Use start_utt to start it\n");
	return 0;
    }

    if (no_search)
        acmod_set_grow(ps->acmod, TRUE);

    while (n_samples) {
        int nfr;

        /* Process some data into features. */
        if ((nfr = acmod_process_float32(ps->acmod, &data,
                                         &n_samples, full_utt)) < 0)
            return nfr;

        /* Score and search as much data as possible */
        if (no_search)
            continue;
        if ((nfr = ps_search_forward(ps)) < 0)
            return nfr;
        n_searchfr += nfr;
    }

    return n_searchfr;
}

int
ps_process_raw(ps_decoder_t *ps,
               int16 const *data,
               size_t n_samples,
               int no_search,
               int full_utt)
{
    int n_searchfr = 0;

    if (ps->acmod->state == ACMOD_IDLE) {
	E_ERROR("Failed to process data, utterance is not started. Use start_utt to start it\n");
	return 0;
    }

    if (no_search)
        acmod_set_grow(ps->acmod, TRUE);

    while (n_samples) {
        int nfr;

        /* Process some data into features. */
        if ((nfr = acmod_process_raw(ps->acmod, &data,
                                     &n_samples, full_utt)) < 0)
            return nfr;

        /* Score and search as much data as possible */
        if (no_search)
            continue;
        if ((nfr = ps_search_forward(ps)) < 0)
            return nfr;
        n_searchfr += nfr;
    }

    return n_searchfr;
}

int
ps_process_cep(ps_decoder_t *ps,
               mfcc_t **data,
               int32 n_frames,
               int no_search,
               int full_utt)
{
    int n_searchfr = 0;

    if (no_search)
        acmod_set_grow(ps->acmod, TRUE);

    while (n_frames) {
        int nfr;

        /* Process some data into features. */
        if ((nfr = acmod_process_cep(ps->acmod, &data,
                                     &n_frames, full_utt)) < 0)
            return nfr;

        /* Score and search as much data as possible */
        if (no_search)
            continue;
        if ((nfr = ps_search_forward(ps)) < 0)
            return nfr;
        n_searchfr += nfr;
    }

    return n_searchfr;
}

EXPORT int
ps_end_utt(ps_decoder_t *ps)
{
    int rv = 0;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }
    if (ps->acmod->state == ACMOD_ENDED || ps->acmod->state == ACMOD_IDLE) {
	E_ERROR("Utterance is not started\n");
	return -1;
    }
    acmod_end_utt(ps->acmod);

    /* Search any remaining frames. */
    if ((rv = ps_search_forward(ps)) < 0) {
        ptmr_stop(&ps->perf);
        return rv;
    } 
    /* Finish main search. */
    if ((rv = ps_search_finish(ps->search)) < 0) {
        ptmr_stop(&ps->perf);
        return rv;
    }
    ptmr_stop(&ps->perf);
    /* Log a backtrace if requested. */
    if (cmd_ln_boolean_r(ps->config, "-backtrace")) {
        const char* hyp;
        ps_seg_t *seg;
        int32 score;

        hyp = ps_get_hyp(ps, &score);
        
        if (hyp != NULL) {
    	    E_INFO("%s (%d)\n", hyp, score);
    	    E_INFO_NOFN("%-20s %-5s %-5s %-5s %-10s %-10s %-3s\n",
                    "word", "start", "end", "pprob", "ascr", "lscr", "lback");
    	    for (seg = ps_seg_iter(ps); seg;
        	 seg = ps_seg_next(seg)) {
    	        char const *word;
        	int sf, ef;
        	int32 post, lscr, ascr, lback;

        	word = ps_seg_word(seg);
        	ps_seg_frames(seg, &sf, &ef);
        	post = ps_seg_prob(seg, &ascr, &lscr, &lback);
        	E_INFO_NOFN("%-20s %-5d %-5d %-1.3f %-10d %-10d %-3d\n",
                    	    word, sf, ef, logmath_exp(ps_get_logmath(ps), post),
                    	ascr, lscr, lback);
    	    }
        }
    }
    return rv;
}

EXPORT char const *
ps_get_hyp(ps_decoder_t *ps, int32 *out_best_score)
{
    char const *hyp;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return NULL;
    }
    ptmr_start(&ps->perf);
    hyp = ps_search_hyp(ps->search, out_best_score);
    ptmr_stop(&ps->perf);
    return hyp;
}

EXPORT int32
ps_get_prob(ps_decoder_t *ps)
{
    int32 prob;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }
    ptmr_start(&ps->perf);
    prob = ps_search_prob(ps->search);
    ptmr_stop(&ps->perf);
    return prob;
}

EXPORT ps_seg_t *
ps_seg_iter(ps_decoder_t *ps)
{
    ps_seg_t *itor;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return NULL;
    }
    ptmr_start(&ps->perf);
    itor = ps_search_seg_iter(ps->search);
    ptmr_stop(&ps->perf);
    return itor;
}

EXPORT ps_seg_t *
ps_seg_next(ps_seg_t *seg)
{
    return ps_search_seg_next(seg);
}

EXPORT char const *
ps_seg_word(ps_seg_t *seg)
{
    return seg->word;
}

EXPORT void
ps_seg_frames(ps_seg_t *seg, int *out_sf, int *out_ef)
{
    if (out_sf) *out_sf = seg->sf;
    if (out_ef) *out_ef = seg->ef;
}

EXPORT int32
ps_seg_prob(ps_seg_t *seg, int32 *out_ascr, int32 *out_lscr, int32 *out_lback)
{
    if (out_ascr) *out_ascr = seg->ascr;
    if (out_lscr) *out_lscr = seg->lscr;
    if (out_lback) *out_lback = seg->lback;
    return seg->prob;
}

EXPORT void
ps_seg_free(ps_seg_t *seg)
{
    ps_search_seg_free(seg);
}

ps_lattice_t *
ps_get_lattice(ps_decoder_t *ps)
{
    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return NULL;
    }
    return ps_search_lattice(ps->search);
}

ps_nbest_t *
ps_nbest(ps_decoder_t *ps)
{
    ps_lattice_t *dag;
    ps_astar_t *nbest;
    void *lmset;
    float32 lwf;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return NULL;
    }
    if ((dag = ps_get_lattice(ps)) == NULL)
        return NULL;

    lmset = NULL;
    lwf = 1.0f;
    nbest = ps_astar_start(dag, lmset, lwf, 0, -1, -1, -1);
    nbest = ps_nbest_next(nbest);

    return (ps_nbest_t *)nbest;
}

void
ps_nbest_free(ps_nbest_t *nbest)
{
    ps_astar_finish(nbest);
}

ps_nbest_t *
ps_nbest_next(ps_nbest_t *nbest)
{
    ps_latpath_t *next;

    next = ps_astar_next(nbest);
    if (next == NULL) {
        ps_nbest_free(nbest);
        return NULL;
    }
    return nbest;
}

char const *
ps_nbest_hyp(ps_nbest_t *nbest, int32 *out_score)
{
    assert(nbest != NULL);

    if (nbest->top == NULL)
        return NULL;
    if (out_score) *out_score = nbest->top->score;
    return ps_astar_hyp(nbest, nbest->top);
}

ps_seg_t *
ps_nbest_seg(ps_nbest_t *nbest)
{
    if (nbest->top == NULL)
        return NULL;

    return ps_astar_seg_iter(nbest, nbest->top, 1.0);
}

int
ps_get_n_frames(ps_decoder_t *ps)
{
    return ps->acmod->output_frame + 1;
}

void
ps_get_utt_time(ps_decoder_t *ps, double *out_nspeech,
                double *out_ncpu, double *out_nwall)
{
    int32 frate;

    frate = cmd_ln_int32_r(ps->config, "-frate");
    *out_nspeech = (double)ps->acmod->output_frame / frate;
    *out_ncpu = ps->perf.t_cpu;
    *out_nwall = ps->perf.t_elapsed;
}

void
ps_get_all_time(ps_decoder_t *ps, double *out_nspeech,
                double *out_ncpu, double *out_nwall)
{
    int32 frate;

    frate = cmd_ln_int32_r(ps->config, "-frate");
    *out_nspeech = (double)ps->n_frame / frate;
    *out_ncpu = ps->perf.t_tot_cpu;
    *out_nwall = ps->perf.t_tot_elapsed;
}

void
ps_search_init(ps_search_t *search, ps_searchfuncs_t *vt,
	       const char *type,
	       const char *name,
               cmd_ln_t *config, acmod_t *acmod, dict_t *dict,
               dict2pid_t *d2p)
{
    search->vt = vt;
    search->name = ckd_salloc(name);
    search->type = ckd_salloc(type);

    search->config = config;
    search->acmod = acmod;
    if (d2p)
        search->d2p = dict2pid_retain(d2p);
    else
        search->d2p = NULL;
    if (dict) {
        search->dict = dict_retain(dict);
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
ps_search_base_free(ps_search_t *search)
{
    /* FIXME: We will have refcounting on acmod, config, etc, at which
     * point we will free them here too. */
    ckd_free(search->name);
    ckd_free(search->type);
    dict_free(search->dict);
    dict2pid_free(search->d2p);
    ckd_free(search->hyp_str);
    ps_lattice_free(search->dag);
}

void
ps_search_base_reinit(ps_search_t *search, dict_t *dict,
                      dict2pid_t *d2p)
{
    dict_free(search->dict);
    dict2pid_free(search->d2p);
    /* FIXME: _retain() should just return NULL if passed NULL. */
    if (dict) {
        search->dict = dict_retain(dict);
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
        search->d2p = dict2pid_retain(d2p);
    else
        search->d2p = NULL;
}

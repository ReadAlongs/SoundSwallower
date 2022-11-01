# cython: embedsignature=True, language_level=3
# Copyright (c) 2008-2020 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhdaines@gmail.com>

cdef extern from "soundswallower/err.h":
    cdef enum err_e:
        ERR_DEBUG,
        ERR_INFO,
        ERR_WARN,
        ERR_ERROR,
        ERR_FATAL,
        ERR_MAX
    ctypedef err_e err_lvl_t
    ctypedef void (*err_cb_f)(void* user_data, err_lvl_t lvl, const char *msg);
    void err_set_callback(err_cb_f callback, void *user_data)


cdef extern from "soundswallower/logmath.h":
    ctypedef struct logmath_t:
        pass
    
    logmath_t *logmath_init(double base, int shift, int use_table)
    int logmath_free(logmath_t *lmath)

    int logmath_log(logmath_t *lmath, double p)
    double logmath_exp(logmath_t *lmath, int p)

    int logmath_ln_to_log(logmath_t *lmath, double p)
    double logmath_log_to_ln(logmath_t *lmath, int p)

    int logmath_log10_to_log(logmath_t *lmath, double p)
    double logmath_log_to_log10(logmath_t *lmath, int p)

    int logmath_add(logmath_t *lmath, int p, int q)

    int logmath_get_zero(logmath_t *lmath)

cdef extern from "soundswallower/fe.h":
    ctypedef struct fe_t:
        pass

cdef extern from "soundswallower/hash_table.h":
    ctypedef struct hash_table_t:
        pass
    ctypedef struct hash_entry_t:
        const char *key
    ctypedef struct hash_iter_t:
        hash_entry_t *ent
    hash_iter_t *hash_table_iter(hash_table_t *h)
    hash_iter_t *hash_table_iter_next(hash_iter_t *h)
    const char *hash_entry_key(hash_entry_t *ent)


cdef extern from "soundswallower/configuration.h":
    ctypedef struct config_param_t:
        const char *name
        int type
        const char *deflt
        const char *doc
    ctypedef struct config_t:
        hash_table_t *ht
        const config_param_t *defn
    cdef enum config_type_t:
        ARG_REQUIRED,
        ARG_INTEGER,
        ARG_FLOATING,
        ARG_STRING,
        ARG_BOOLEAN,
        REQARG_INTEGER,
        REQARG_FLOATING,
        REQARG_STRING,
        REQARG_BOOLEAN
    ctypedef struct config_val_t:
        int type
    ctypedef union anytype_t:
        long i
        float fl
        void *ptr
        
    config_t *config_init(const config_param_t *defn)
    config_t *config_retain(config_t *config)
    int config_free(config_t *cmdln)
    int config_validate(config_t *config)
    void config_expand(config_t *config)
    config_t *config_parse_json(config_t *config, const char *json)
    const char *config_serialize_json(config_t *config)
    config_type_t config_typeof(config_t *config, const char *name)
    int config_exists(config_t *config, const char *name)
    const anytype_t *config_get(config_t *config, const char *name)

    double config_float(config_t *cmdln, const char *name)
    long config_int(config_t *cmdln, const char *name)
    const char *config_str(config_t *cmdln, const char *name)
    void config_set_str(config_t *cmdln, const char *name, const char *str)
    void config_set_int(config_t *cmdln, const char *name, long val)
    void config_set_bool(config_t *cmdln, const char *name, long val)
    void config_set_float(config_t *cmdln, const char *name, double val)


cdef extern from "soundswallower/fsg_model.h":
    ctypedef struct fsg_model_t:
        int start_state
        int final_state

    fsg_model_t *fsg_model_init(const char *name, logmath_t *lmath,
                                float lw, int n_state)
    fsg_model_t *fsg_model_readfile(const char *file, logmath_t *lmath,
                                    float lw)
    const char *fsg_model_name(fsg_model_t *fsg)
    fsg_model_t *fsg_model_retain(fsg_model_t *fsg)
    int fsg_model_free(fsg_model_t *fsg)

    int fsg_model_word_add(fsg_model_t *fsg, const char *word)
    int fsg_model_word_id(fsg_model_t *fsg, const char *word)
    void fsg_model_trans_add(fsg_model_t * fsg,
                             int source, int dest, int logp, int wid)
    int fsg_model_null_trans_add(fsg_model_t * fsg, int source, int dest,
                                 int logp)
    int fsg_model_tag_trans_add(fsg_model_t * fsg, int source, int dest,
                                int logp, int wid)
    void fsg_model_writefile(fsg_model_t *fsg, const char *file)


cdef extern from "soundswallower/jsgf.h":
    ctypedef struct jsgf_t:
        pass
    ctypedef struct jsgf_rule_t:
        pass
    fsg_model_t *jsgf_read_file(const char *name, logmath_t *lmath,
                                float lw)
    jsgf_t *jsgf_parse_string(const char *string, jsgf_t *parent)
    const char *jsgf_grammar_name(jsgf_t *jsgf)
    void jsgf_grammar_free(jsgf_t *jsgf)
    jsgf_rule_t *jsgf_get_rule(jsgf_t *grammar, const char *name)
    jsgf_rule_t *jsgf_get_public_rule(jsgf_t *grammar)
    fsg_model_t *jsgf_build_fsg(jsgf_t *grammar, jsgf_rule_t *rule,
                                logmath_t *lmath, float lw)


cdef extern from "soundswallower/decoder.h":
    ctypedef struct decoder_t:
        pass
    ctypedef struct seg_iter_t:
        pass
    config_param_t *decoder_args()
    decoder_t *decoder_init(config_t *config)
    int decoder_free(decoder_t *ps)
    int decoder_reinit(decoder_t *ps, config_t *config)
    fe_t *decoder_reinit_fe(decoder_t *ps, config_t *config)
    logmath_t *decoder_logmath(decoder_t *ps)
    int decoder_start_utt(decoder_t *ps)
    int decoder_process_int16(decoder_t *ps,
                              const short *data, size_t n_samples,
                              int no_search, int full_utt)
    int decoder_end_utt(decoder_t *ps)
    const char *decoder_hyp(decoder_t *ps, int *out_best_score)
    int decoder_prob(decoder_t *ps)
    seg_iter_t *decoder_seg_iter(decoder_t *ps)
    seg_iter_t *seg_iter_next(seg_iter_t *seg)
    const char *seg_iter_word(seg_iter_t *seg)
    void seg_iter_frames(seg_iter_t *seg, int *out_sf, int *out_ef)
    int seg_iter_prob(seg_iter_t *seg, int *out_ascr, int *out_lscr)
    void seg_iter_free(seg_iter_t *seg)
    int decoder_add_word(decoder_t *ps, char *word, char *phones, int update)
    char *decoder_lookup_word(decoder_t *d, const char *word)
    int decoder_set_fsg(decoder_t *ps, fsg_model_t *fsg)
    int decoder_set_jsgf_file(decoder_t *ps, const char *path)
    int decoder_set_jsgf_string(decoder_t *ps, const char *jsgf_string)
    const char *decoder_get_cmn(decoder_t *ps, int update)
    int decoder_set_cmn(decoder_t *ps, const char *cmn)

# cython: embedsignature=True
# Copyright (c) 2008-2020 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhdaines@gmail.com>

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


cdef extern from "soundswallower/cmd_ln.h":
    ctypedef struct cmd_ln_t:
        hash_table_t *ht
    ctypedef struct arg_t:
        pass
    cdef enum:
        ARG_REQUIRED,
        ARG_INTEGER,
        ARG_FLOATING,
        ARG_STRING,
        ARG_BOOLEAN,
        ARG_STRING_LIST,
        REQARG_INTEGER,
        REQARG_FLOATING,
        REQARG_STRING,
        REQARG_BOOLEAN
    ctypedef struct cmd_ln_val_t:
        int type
        
    cmd_ln_t *cmd_ln_parse_r(cmd_ln_t *inout_cmdln, arg_t *defn,
                             int argc, char **argv, int strict)
    int cmd_ln_free_r(cmd_ln_t *cmdln)

    double cmd_ln_float_r(cmd_ln_t *cmdln, const char *name)
    long cmd_ln_int_r(cmd_ln_t *cmdln, const char *name)
    const char *cmd_ln_str_r(cmd_ln_t *cmdln, const char *name)
    void cmd_ln_set_str_r(cmd_ln_t *cmdln, const char *name, const char *str)
    void cmd_ln_set_int_r(cmd_ln_t *cmdln, const char *name, long iv)
    void cmd_ln_set_float_r(cmd_ln_t *cmdln, const char *name, double fv)
    cmd_ln_val_t *cmd_ln_access_r(cmd_ln_t *cmdln, const char *name)

    int cmd_ln_exists_r(cmd_ln_t *cmdln, const char *name)


cdef extern from "soundswallower/fsg_model.h":
    ctypedef struct fsg_model_t:
        pass

    fsg_model_t *fsg_model_init(const char *name, logmath_t *lmath,
                                float lw, int n_state)
    fsg_model_t *fsg_model_readfile(const char *file, logmath_t *lmath,
                                    float lw)
    int fsg_model_free(fsg_model_t *fsg);


cdef extern from "soundswallower/jsgf.h":
    fsg_model_t *jsgf_read_file(const char *name, logmath_t *lmath,
                                float lw)


cdef extern from "soundswallower/pocketsphinx.h":
    ctypedef struct ps_decoder_t:
        pass
    ctypedef struct ps_seg_t:
        pass
    arg_t *ps_args()
    ps_decoder_t *ps_init(cmd_ln_t *config)
    int ps_free(ps_decoder_t *ps)
    logmath_t *ps_get_logmath(ps_decoder_t *ps)
    int ps_start_utt(ps_decoder_t *ps)
    int ps_process_raw(ps_decoder_t *ps,
                       const short *data, size_t n_samples,
                       int no_search, int full_utt)
    int ps_end_utt(ps_decoder_t *ps)
    const char *ps_get_hyp(ps_decoder_t *ps, int *out_best_score)
    int ps_get_prob(ps_decoder_t *ps)
    ps_seg_t *ps_seg_iter(ps_decoder_t *ps)
    ps_seg_t *ps_seg_next(ps_seg_t *seg)
    const char *ps_seg_word(ps_seg_t *seg)
    void ps_seg_frames(ps_seg_t *seg, int *out_sf, int *out_ef)
    int ps_seg_prob(ps_seg_t *seg, int *out_ascr, int *out_lscr, int *out_lback)
    void ps_seg_free(ps_seg_t *seg)
    int ps_load_dict(ps_decoder_t *ps, char *dictfile,
                     char *fdictfile, char *format)
    int ps_save_dict(ps_decoder_t *ps, char *dictfile, char *format)
    int ps_add_word(ps_decoder_t *ps, char *word, char *phones, int update)

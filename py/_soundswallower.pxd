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
    cdef enum config_type_e:
        ARG_REQUIRED,
        ARG_INTEGER,
        ARG_FLOATING,
        ARG_STRING,
        ARG_BOOLEAN,
        REQARG_INTEGER,
        REQARG_FLOATING,
        REQARG_STRING,
        REQARG_BOOLEAN
    ctypedef config_type_e config_type_t
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
    const anytype_t *config_unset(config_t *config, const char *name)
    const anytype_t *config_set(config_t *config, const char *name,
                                const anytype_t *val, config_type_t t)
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
    decoder_t *decoder_create(config_t *config)
    decoder_t *decoder_init(config_t *config)
    int decoder_free(decoder_t *ps)
    int decoder_reinit(decoder_t *ps, config_t *config)
    int decoder_reinit_feat(decoder_t *ps, config_t *config)
    config_t *decoder_config(decoder_t *ps)
    logmath_t *decoder_logmath(decoder_t *ps)
    int decoder_start_utt(decoder_t *ps)
    int decoder_process_int16(decoder_t *ps,
                              short *data, size_t n_samples,
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
    int decoder_set_align_text(decoder_t *d, const char *text)
    const alignment_t *decoder_alignment(decoder_t *d)
    const char *decoder_result_json(decoder_t *decoder, double start, int align_level)
    int decoder_n_frames(decoder_t *d)

cdef extern from "soundswallower/vad.h":
    ctypedef struct vad_t:
        pass
    cdef enum vad_mode_e:
        VAD_LOOSE,
        VAD_MEDIUM_LOOSE,
        VAD_MEDIUM_STRICT,
        VAD_STRICT
    ctypedef vad_mode_e vad_mode_t
    cdef enum vad_class_e:
        VAD_ERROR,
        VAD_NOT_SPEECH,
        VAD_SPEECH
    ctypedef vad_class_e vad_class_t
    cdef int VAD_DEFAULT_SAMPLE_RATE
    cdef double VAD_DEFAULT_FRAME_LENGTH

    vad_t *vad_init(vad_mode_t mode, int sample_rate, double frame_length)
    int vad_free(vad_t *vad)
    int vad_set_input_params(vad_t *vad, int sample_rate, double frame_length)
    int vad_sample_rate(vad_t *vad)
    size_t vad_frame_size(vad_t *vad)
    double vad_frame_length(vad_t *vad)
    vad_class_t vad_classify(vad_t *vad, const short *frame)

cdef extern from "soundswallower/endpointer.h":
    ctypedef struct endpointer_t:
        pass
    cdef double ENDPOINTER_DEFAULT_WINDOW
    cdef double ENDPOINTER_DEFAULT_RATIO
    endpointer_t *endpointer_init(double window,
                                        double ratio,
                                        vad_mode_t mode,
                                        int sample_rate, double frame_length)
    endpointer_t *endpointer_retain(endpointer_t *ep)
    int endpointer_free(endpointer_t *ep)
    vad_t *endpointer_vad(endpointer_t *ep)
    size_t endpointer_frame_size(endpointer_t *ep)
    double endpointer_frame_length(endpointer_t *ep)
    int endpointer_sample_rate(endpointer_t *ep)
    const short *endpointer_process(endpointer_t *ep,
                                       const short *frame)
    const short *endpointer_end_stream(endpointer_t *ep,
                                          const short *frame,
                                          size_t nsamp,
                                          size_t *out_nsamp)
    int endpointer_in_speech(endpointer_t *ep)
    double endpointer_speech_start(endpointer_t *ep)
    double endpointer_speech_end(endpointer_t *ep)

cdef extern from "soundswallower/alignment.h":
    ctypedef struct alignment_t:
        pass
    ctypedef struct alignment_iter_t:
        pass
    ctypedef struct pid_struct:
        short cipid
        unsigned short ssid
        int tmat
    ctypedef union id_union:
        int wid
        pid_struct pid
        unsigned short senid
    ctypedef struct alignment_entry_t:
        int start
        int duration
        int score
        id_union id
        int parent
        int child
    alignment_t *alignment_retain(alignment_t *al)
    int alignment_free(alignment_t *al)
    int alignment_n_words(alignment_t *al)
    int alignment_n_phones(alignment_t *al)
    int alignment_n_states(alignment_t *al)
    alignment_iter_t *alignment_words(alignment_t *al)
    alignment_iter_t *alignment_phones(alignment_t *al)
    alignment_iter_t *alignment_states(alignment_t *al)
    alignment_iter_t *alignment_iter_next(alignment_iter_t *itor)
    alignment_iter_t *alignment_iter_children(alignment_iter_t *itor)
    int alignment_iter_seg(alignment_iter_t *itor, int *start, int *duration)
    const char *alignment_iter_name(alignment_iter_t *itor)
    int alignment_iter_free(alignment_iter_t *itor)

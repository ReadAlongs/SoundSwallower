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
    logmath_t *logmath_retain(logmath_t *lmath)
    bint logmath_free(logmath_t *lmath)

    int logmath_log(logmath_t *lmath, double p)
    double logmath_exp(logmath_t *lmath, int p)

    int logmath_ln_to_log(logmath_t *lmath, double p)
    double logmath_log_to_ln(logmath_t *lmath, int p)

    int logmath_log10_to_log(logmath_t *lmath, double p)
    double logmath_log_to_log10(logmath_t *lmath, int p)

    int logmath_add(logmath_t *lmath, int p, int q)

    int logmath_get_zero(logmath_t *lmath)


cdef extern from "soundswallower/cmd_ln.h":
    ctypedef struct cmd_ln_t:
        pass
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
    void cmd_ln_free_r(cmd_ln_t *cmdln)

    double cmd_ln_float_r(cmd_ln_t *cmdln, const char *name)
    long cmd_ln_int_r(cmd_ln_t *cmdln, const char *name)
    const char *cmd_ln_str_r(cmd_ln_t *cmdln, const char *name)
    void cmd_ln_set_str_r(cmd_ln_t *cmdln, const char *name, const char *str)
    void cmd_ln_set_int_r(cmd_ln_t *cmdln, const char *name, long iv)
    void cmd_ln_set_float_r(cmd_ln_t *cmdln, const char *name, double fv)
    cmd_ln_val_t *cmd_ln_access_r(cmd_ln_t *cmdln, const char *name)

    int cmd_ln_exists_r(cmd_ln_t *cmdln, const char *name)


cdef extern from "soundswallower/pocketsphinx.h":
    arg_t *ps_args();

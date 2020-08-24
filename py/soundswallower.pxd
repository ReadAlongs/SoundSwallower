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

    cmd_ln_t *cmd_ln_parse_r(cmd_ln_t *inout_cmdln, arg_t *defn,
                             int argc, char **argv, int strict)
    void cmd_ln_free_r(cmd_ln_t *cmdln)
    float cmd_ln_float32_r(cmd_ln_t *cmdln, char *key)
    int cmd_ln_int32_r(cmd_ln_t *cmdln, char *key)
    bint cmd_ln_boolean_r(cmd_ln_t *cmdln, char *key)
    char *cmd_ln_str_r(cmd_ln_t *cmdln, char *key)


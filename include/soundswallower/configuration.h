/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
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
 * configuration.h -- Command line argument parsing.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 *
 * HISTORY
 *
 * 15-Jul-1997	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added required arguments types.
 *
 * 07-Dec-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created, based on Eric's implementation.  Basically, combined several
 *		functions into one, eliminated validation, and simplified the interface.
 */

#ifndef _LIBUTIL_CMD_LN_H_
#define _LIBUTIL_CMD_LN_H_

#include <stdarg.h>
#include <stdio.h>

#include <soundswallower/hash_table.h>
#include <soundswallower/prim_type.h>

/**
 * @file configuration.h
 * @brief Command-line and other configuration parsing and handling.
 *
 * Configuration parameters, optionally parsed from the command line.
 */

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

/**
 * @enum config_type_e
 * @brief Types of configuration parameters.
 */
typedef enum config_type_e {
    ARG_REQUIRED = (1 << 0), /*<< Bit indicating required argument. */
    ARG_INTEGER = (1 << 1), /*<< Integer up to 64 bits. */
    ARG_FLOATING = (1 << 2), /*<< Double-precision floating point. */
    ARG_STRING = (1 << 3), /*<< String. */
    ARG_BOOLEAN = (1 << 4), /*<< Boolean (true/false). */
    REQARG_INTEGER = (ARG_INTEGER | ARG_REQUIRED),
    REQARG_FLOATING = (ARG_FLOATING | ARG_REQUIRED),
    REQARG_STRING = (ARG_STRING | ARG_REQUIRED),
    REQARG_BOOLEAN = (ARG_BOOLEAN | ARG_REQUIRED)
} config_type_t;
/**
 * @typedef config_type_t
 * @brief Types of configuration parameters.
 */

/**
 * @struct config_param_t
 * Argument definition structure.
 */
typedef struct config_param_s {
    const char *name; /**< Name of the command line switch */
    int type; /**< Type of the argument in question */
    const char *deflt; /**< Default value (as a character string), or NULL if none */
    const char *doc; /**< Documentation/description string */
} config_param_t;

/**
 * @struct config_val_t
 * Configuration parameter structure.
 */
typedef struct config_val_s {
    anytype_t val;
    int type;
    char *name;
} config_val_t;

/**
 * @struct config_t
 * Structure (no longer opaque) used to hold the results of command-line parsing.
 */
typedef struct config_s {
    int refcount;
    hash_table_t *ht;
    config_param_t const *defn;
    char *json;
} config_t;

/**
 * Create a configuration with default values.
 *
 * @memberof config_t
 * @param defn Array of config_param_t defining and describing parameters,
 * terminated by an config_param_t with `name == NULL`.  You should usually
 * just pass NULL here, which will result in the standard set of
 * parameters being used.
 * @return Newly created configuration or NULL on failure (should not
 * happen, but check it anyway).
 */
config_t *config_init(const config_param_t *defn);

/**
 * Retain a pointer to a configuration object.
 * @memberof config_t
 */
config_t *config_retain(config_t *config);

/**
 * Release a configuration object.
 * @memberof config_t
 */
int config_free(config_t *config);

/**
 * Validate configuration.
 *
 * Currently this just checks that you haven't specified multiple
 * types of grammars or language models at the same time.
 *
 * @memberof config_t
 * @return 0 for success, <0 for failure.
 */
int config_validate(config_t *config);

/**
 * Expand model parameters in configuration.
 *
 * If "hmm" is set, this will provide default values for the various model files.
 * @memberof config_t
 */
void config_expand(config_t *config);

/**
 * Create or update a configuration by parsing slightly extended JSON.
 *
 * This function parses a JSON object in non-strict mode to produce a
 * config_t.  Configuration parameters are given *without* a
 * leading dash, and do not need to be quoted, nor does the object
 * need to be enclosed in curly braces, nor are commas necessary
 * between key/value pairs.  Basically, it's degenerate YAML.  So, for
 * example, this is accepted:
 *
 *     hmm: fr-fr
 *     samprate: 8000
 *     keyphrase: "hello world"
 *
 * Of course, valid JSON is also accepted, but who wants to use that.
 *
 * Well, mostly.  Unicode escape sequences (e.g. `"\u0020"`) are *not*
 * supported at the moment, so please don't use them.
 *
 * @memberof config_t
 * @arg config Previously existing config_t to update, or NULL to
 * create a new one.
 * @arg json JSON serialized object as null-terminated UTF-8,
 * containing configuration parameters.
 * @return Newly created configuration or NULL on failure (such as
 * invalid or missing parameters).
 */
config_t *config_parse_json(config_t *config, const char *json);

/**
 * Construct JSON from a configuration object.
 *
 * Unlike config_parse_json(), this actually produces valid JSON ;-)
 *
 * @memberof config_t
 * @arg config Configuration object
 * @return Newly created null-terminated JSON string.  The config_t
 * retains ownership of this pointer, which is only valid until the
 * next call to config_serialize_json().  You must copy it if you
 * wish to retain it.
 */
const char *config_serialize_json(config_t *config);

/**
 * Access the type of a configuration parameter.
 *
 * @memberof config_t
 * @param config Configuration object.
 * @param name Name of the parameter to retrieve.
 * @return the type of the parameter (as a combination of the ARG_*
 *         bits), or 0 if no such parameter exists.
 */
config_type_t config_typeof(config_t *config, const char *name);
#define config_exists(config, name) (config_typeof(config, name) != 0)

/**
 * Access the value of a configuration parameter.
 *
 * To actually do something with the value, you will need to know its
 * type, which can be obtained with config_typeof().  This function
 * is thus mainly useful for dynamic language bindings, and you should
 * use config_int(), config_float(), or config_str() instead.
 *
 * @memberof config_t
 * @param config Configuration object.
 * @param name Name of the parameter to retrieve.
 * @return Pointer to the parameter's value, or NULL if the parameter
 * does not exist.  Note that a string parameter can also have NULL as
 * a value, in which case the `ptr` field in the return value is NULL.
 * This pointer (and any pointers inside it) is owned by the config_t.
 */
const anytype_t *config_get(config_t *config, const char *name);

/**
 * Unset the value of a configuration parameter.
 *
 * For string parameters this sets the value to NULL.  For others,
 * since this is not possible, it restores the global default.
 *
 * @memberof config_t
 * @param config Configuration object.
 * @param name Name of the parameter to unset.  Must exist.
 * @return Pointer to the parameter's value, or NULL on failure
 * (unknown parameter, usually).  This pointer (and any pointers
 * inside it) is owned by the config_t.
 */
const anytype_t *config_unset(config_t *config, const char *name);

/**
 * Set or unset the value of a configuration parameter.
 *
 * This will coerce the value to the proper type, so you can, for
 * example, pass it a string with ARG_STRING as the type when adding
 * options from the command-line.  Note that the return pointer will
 * *not* be the same as the one passed in the value.
 *
 * @memberof config_t
 * @param config Configuration object.
 * @param name Name of the parameter to set.  Must exist.
 * @param val Pointer to the value (strings will be copied) inside an
 * anytype_t union.  On 64-bit little-endian platforms, you *can* cast
 * a pointer to int, long, double, or char* here, but that doesn't
 * necessarily mean that you *should*.  Passing NULL here does the
 * same thing as config_unset().
 * @param t Type of the value in `val`, will be coerced to the type of
 * the actual parameter if necessary.
 * @return Pointer to the parameter's value, or NULL on failure
 * (unknown parameter, usually).  This pointer (and any pointers
 * inside it) is owned by the config_t.
 */
const anytype_t *config_set(config_t *config, const char *name,
                            const anytype_t *val, config_type_t t);

/**
 * Get an integer-valued parameter.
 *
 * If the parameter does not have an integer or boolean type, this
 * will print an error and return 0.  So don't do that.
 *
 * @memberof config_t
 */
long config_int(config_t *config, const char *name);

/**
 * Get a boolean-valued parameter.
 *
 * If the parameter does not have an integer or boolean type, this
 * will print an error and return 0.  The return value is either 0 or
 * 1 (if the parameter has an integer type, any non-zero value will
 * return 1).
 *
 * @memberof config_t
 */
int config_bool(config_t *config, const char *name);

/**
 * Get a floating-point parameter.
 *
 * If the parameter does not have a floating-point type, this will
 * print an error and return 0.
 *
 * @memberof config_t
 */
double config_float(config_t *config, const char *name);

/**
 * Get a string parameter.
 *
 * If the parameter does not have a string type, this will print an
 * error and return NULL.  Notably, it will *NOT* format an integer or
 * float for you, because that would involve allocating memory.  So
 * don't do that.
 *
 * @memberof config_t
 */
const char *config_str(config_t *config, const char *name);

/**
 * Set an integer-valued parameter.
 *
 * If the parameter does not have an integer or boolean type, this
 * will convert `val` appropriately.
 *
 * @memberof config_t
 */
const anytype_t *config_set_int(config_t *config, const char *name, long val);

/**
 * Set a boolean-valued parameter.
 *
 * If the parameter does not have an integer or boolean type, this
 * will convert `val` appropriately.
 *
 * @memberof config_t
 */
const anytype_t *config_set_bool(config_t *config, const char *name, int val);

/**
 * Set a floating-point parameter.
 *
 * If the parameter does not have a floating-point type, this will
 * convert `val` appropriately.
 *
 * @memberof config_t
 */
const anytype_t *config_set_float(config_t *config,
                                  const char *name, double val);

/**
 * Set a string-valued parameter.
 *
 * If the parameter does not have a string type, this will convert
 * `val` appropriately.  For boolean parameters, any string matching
 * `/^[yt1]/` will be true, while any string matching `/^[nf0]/` will
 * be false.  NULL is also false.
 *
 * This function is used for configuration from JSON, you may want to
 * use it for your own configuration files too.
 *
 * @memberof config_t
 */
const anytype_t *config_set_str(config_t *config,
                                const char *name, const char *val);

/**
 * Print a help message listing the valid argument names, and the
 * associated attributes as given in defn.
 *
 * @param cmdln command-line object
 */
void config_log_help(config_t *cmdln);

/**
 * Print current configuration values and defaults.
 *
 * @param cmdln  command-line object
 */
void config_log_values(config_t *cmdln);

config_val_t *config_access(config_t *cmdln, const char *name);
anytype_t *anytype_from_str(anytype_t *val, int t, const char *str);
config_val_t *config_val_init(int t, const char *name, const char *str);

#ifdef __cplusplus
}
#endif

#endif

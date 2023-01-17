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
 * config.c -- Command line argument parsing.
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
 * 10-Sep-1998 M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 *             Changed strcasecmp() call in cmp_name() to strcmp_nocase() call.
 *
 * 15-Jul-1997    M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 *         Added required arguments handling.
 *
 * 07-Dec-96    M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 *         Created, based on Eric's implementation.  Basically, combined several
 *        functions into one, eliminated validation, and simplified the interface.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable : 4996 4018)
#endif

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <soundswallower/case.h>
#include <soundswallower/ckd_alloc.h>
#include <soundswallower/config_defs.h>
#include <soundswallower/configuration.h>
#include <soundswallower/err.h>
#include <soundswallower/strfuncs.h>

#include "jsmn.h"

static const config_param_t ps_args_def[] = {
    CONFIG_OPTIONS,
    CONFIG_EMPTY_OPTION
};

/*
 * Find max length of name and default fields in the given defn array.
 * Return #items in defn array.
 */
static size_t
arg_strlen(const config_param_t *defn, size_t *namelen, size_t *deflen)
{
    size_t i, l;

    *namelen = *deflen = 0;
    for (i = 0; defn[i].name; i++) {
        l = strlen(defn[i].name);
        if (*namelen < l)
            *namelen = l;

        if (defn[i].deflt)
            l = strlen(defn[i].deflt);
        else
            l = strlen("(null)");
        /*      E_INFO("string default, %s , name %s, length %d\n",defn[i].deflt,defn[i].name,l); */
        if (*deflen < l)
            *deflen = l;
    }

    return i;
}

static int32
cmp_name(const void *a, const void *b)
{
    return (strcmp_nocase((*(config_param_t **)a)->name,
                          (*(config_param_t **)b)->name));
}

static config_param_t const **
arg_sort(const config_param_t *defn, size_t n)
{
    const config_param_t **pos;
    size_t i;

    pos = (config_param_t const **)ckd_calloc(n, sizeof(config_param_t *));
    for (i = 0; i < n; ++i)
        pos[i] = &defn[i];
    qsort((void *)pos, n, sizeof(config_param_t *), cmp_name);

    return pos;
}

static void
arg_log_r(config_t *cmdln, const config_param_t *defn, int32 doc, int32 lineno)
{
    config_param_t const **pos;
    size_t i, n;
    size_t namelen, deflen;
    config_val_t const *vp;

    /* No definitions, do nothing. */
    if (defn == NULL)
        return;

    /* Find max lengths of name and default value fields, and #entries in defn */
    n = arg_strlen(defn, &namelen, &deflen);
    namelen += 4;
    deflen += 4;
    if (lineno)
        E_INFO("%-*s", namelen, "[NAME]");
    else
        E_INFOCONT("%-*s", namelen, "[NAME]");
    E_INFOCONT("%-*s", deflen, "[DEFLT]");
    if (doc) {
        E_INFOCONT("     [DESCR]\n");
    } else {
        E_INFOCONT("    [VALUE]\n");
    }

    /* Print current configuration, sorted by name */
    pos = arg_sort(defn, n);
    for (i = 0; i < n; i++) {
        if (lineno)
            E_INFO("%-*s", namelen, pos[i]->name);
        else
            E_INFOCONT("%-*s", namelen, pos[i]->name);
        if (pos[i]->deflt)
            E_INFOCONT("%-*s", deflen, pos[i]->deflt);
        else
            E_INFOCONT("%-*s", deflen, "");
        if (doc) {
            if (pos[i]->doc)
                E_INFOCONT("    %s", pos[i]->doc);
        } else {
            vp = config_access(cmdln, pos[i]->name);
            if (vp) {
                switch (pos[i]->type) {
                case ARG_INTEGER:
                case REQARG_INTEGER:
                    E_INFOCONT("    %ld", vp->val.i);
                    break;
                case ARG_FLOATING:
                case REQARG_FLOATING:
                    E_INFOCONT("    %e", vp->val.fl);
                    break;
                case ARG_STRING:
                case REQARG_STRING:
                    if (vp->val.ptr)
                        E_INFOCONT("    %s", (char *)vp->val.ptr);
                    break;
                case ARG_BOOLEAN:
                case REQARG_BOOLEAN:
                    E_INFOCONT("    %s", vp->val.i ? "yes" : "no");
                    break;
                default:
                    E_ERROR("Unknown argument type: %d\n", pos[i]->type);
                }
            }
        }

        E_INFOCONT("\n");
    }
    ckd_free((void *)pos);
    E_INFOCONT("\n");
}

void
config_log_help(config_t *cmdln)
{
    E_INFO("Arguments list definition:\n");
    if (cmdln == NULL) {
        cmdln = config_init(NULL);
        arg_log_r(cmdln, cmdln->defn, TRUE, FALSE);
        config_free(cmdln);
    } else
        arg_log_r(cmdln, cmdln->defn, TRUE, FALSE);
}

void
config_log_values(config_t *cmdln)
{
    E_INFO("Current configuration:\n");
    arg_log_r(cmdln, cmdln->defn, FALSE, FALSE);
}

config_val_t *
config_access(config_t *cmdln, const char *name)
{
    void *val;
    if (hash_table_lookup(cmdln->ht, name, &val) < 0) {
        E_ERROR("Unknown argument: %s\n", name);
        return NULL;
    }
    return (config_val_t *)val;
}

config_val_t *
config_val_init(int t, const char *name, const char *str)
{
    config_val_t *v;

    v = ckd_calloc(1, sizeof(*v));
    if (anytype_from_str(&v->val, t, str) == NULL) {
        ckd_free(v);
        return NULL;
    }
    v->type = t;
    v->name = ckd_salloc(name);

    return v;
}

void
config_val_free(config_val_t *val)
{
    if (val->type & ARG_STRING)
        ckd_free(val->val.ptr);
    ckd_free(val->name);
    ckd_free(val);
}

config_t *
config_init(const config_param_t *defn)
{
    config_t *config = ckd_calloc(1, sizeof(*config));
    int i, ndef;

    config->refcount = 1;
    if (defn)
        config->defn = defn;
    else
        config->defn = ps_args_def;
    for (ndef = 0; config->defn[ndef].name; ndef++)
        ;
    config->ht = hash_table_new(ndef, FALSE);
    for (i = 0; i < ndef; i++) {
        config_val_t *val;
        if ((val = config_val_init(config->defn[i].type,
                                   config->defn[i].name,
                                   config->defn[i].deflt))
            == NULL) {
            E_ERROR("Bad default argument value for %s: %s\n",
                    config->defn[i].name, config->defn[i].deflt);
            continue;
        }
        hash_table_enter(config->ht, val->name, (void *)val);
    }
    return config;
}

config_t *
config_retain(config_t *config)
{
    if (config == NULL)
        return NULL;
    ++config->refcount;
    return config;
}

int
config_free(config_t *config)
{
    if (config == NULL)
        return 0;
    if (--config->refcount > 0)
        return config->refcount;
    if (config->ht) {
        glist_t entries;
        gnode_t *gn;
        int32 n;

        entries = hash_table_tolist(config->ht, &n);
        for (gn = entries; gn; gn = gnode_next(gn)) {
            hash_entry_t *e = (hash_entry_t *)gnode_ptr(gn);
            config_val_free((config_val_t *)e->val);
        }
        glist_free(entries);
        hash_table_free(config->ht);
        config->ht = NULL;
    }

    if (config->json)
        ckd_free(config->json);
    ckd_free(config);
    return 0;
}

static const char *searches[] = {
    "jsgf",
    "fsg",
};
static const int nsearches = sizeof(searches) / sizeof(searches[0]);

int
config_validate(config_t *config)
{
    int i, found = 0;
    for (i = 0; i < nsearches; ++i) {
        if (config_str(config, searches[i]) != NULL)
            if (++found > 1)
                break;
    }
    if (found > 1) {
        int len = strlen("Only one of ");
        char *msg;
        for (i = 0; i < nsearches; ++i)
            len += strlen(searches[i]) + 2;
        len += strlen("can be enabled at a time in config\n");
        msg = ckd_malloc(len + 1);
        strcpy(msg, "Only one of ");
        for (i = 0; i < nsearches; ++i) {
            strcat(msg, searches[i]);
            strcat(msg, ", ");
        }
        strcat(msg, "can be enabled at a time in config\n");
        E_ERROR(msg);
        ckd_free(msg);
        return -1;
    }
    return 0;
}

void
json_error(int err)
{
    const char *errstr;
    switch (err) {
    case JSMN_ERROR_INVAL:
        errstr = "JSMN_ERROR_INVAL - bad token, JSON string is corrupted";
        break;
    case JSMN_ERROR_NOMEM:
        errstr = "JSMN_ERROR_NOMEM - not enough tokens, JSON string is too large";
        break;
    case JSMN_ERROR_PART:
        errstr = "JSMN_ERROR_PART - JSON string is too short, expecting more JSON data";
        break;
    case 0:
        errstr = "JSON string appears to be empty";
        break;
    default:
        errstr = "Unknown error";
    }
    E_ERROR("JSON parsing failed: %s\n", errstr);
}

size_t
unescape(char *out, const char *in, size_t len)
{
    char *ptr = out;
    size_t i;

    for (i = 0; i < len; ++i) {
        int c = in[i];
        if (c == '\\') {
            switch (in[i + 1]) {
            case '"':
                *ptr++ = '"';
                i++;
                break;
            case '\\':
                *ptr++ = '\\';
                i++;
                break;
            case 'b':
                *ptr++ = '\b';
                i++;
                break;
            case 'f':
                *ptr++ = '\f';
                i++;
                break;
            case 'n':
                *ptr++ = '\n';
                i++;
                break;
            case 'r':
                *ptr++ = '\r';
                i++;
                break;
            case 't':
                *ptr++ = '\t';
                i++;
                break;
            default:
                E_WARN("Unsupported escape sequence \\%c\n", in[i + 1]);
                *ptr++ = c;
            }
        } else {
            *ptr++ = c;
        }
    }
    *ptr = '\0';
    return ptr - out;
}

config_t *
config_parse_json(config_t *config,
                  const char *json)
{
    jsmn_parser parser;
    jsmntok_t *tokens = NULL;
    char *key = NULL, *val = NULL;
    int i, jslen, ntok, new_config = FALSE;

    if (json == NULL)
        return NULL;
    if (config == NULL) {
        if ((config = config_init(NULL)) == NULL)
            return NULL;
        new_config = TRUE;
    }
    jsmn_init(&parser);
    jslen = strlen(json);
    if ((ntok = jsmn_parse(&parser, json, jslen, NULL, 0)) <= 0) {
        json_error(ntok);
        goto error_out;
    }
    /* Need to reset the parser before second pass! */
    jsmn_init(&parser);
    tokens = ckd_calloc(ntok, sizeof(*tokens));
    if ((i = jsmn_parse(&parser, json, jslen, tokens, ntok)) != ntok) {
        json_error(i);
        goto error_out;
    }
    /* Accept missing top-level object. */
    i = 0;
    if (tokens[0].type == JSMN_OBJECT)
        ++i;
    while (i < ntok) {
        key = ckd_malloc(tokens[i].end - tokens[i].start + 1);
        unescape(key, json + tokens[i].start, tokens[i].end - tokens[i].start);
        if (tokens[i].type != JSMN_STRING && tokens[i].type != JSMN_PRIMITIVE) {
            E_ERROR("Expected string or primitive key, got %s\n", key);
            goto error_out;
        }
        if (++i == ntok) {
            E_ERROR("Missing value for %s\n", key);
            goto error_out;
        }
        val = ckd_malloc(tokens[i].end - tokens[i].start + 1);
        unescape(val, json + tokens[i].start, tokens[i].end - tokens[i].start);
        if (config_set_str(config, key, val) == NULL) {
            E_ERROR("Unknown or invalid parameter %s\n", key);
            goto error_out;
        }
        ckd_free(key);
        ckd_free(val);
        key = val = NULL;
        ++i;
    }

    ckd_free(key);
    ckd_free(val);
    ckd_free(tokens);
    return config;

error_out:
    if (key)
        ckd_free(key);
    if (val)
        ckd_free(val);
    if (tokens)
        ckd_free(tokens);
    if (new_config)
        config_free(config);
    return NULL;
}

/* Following two functions are:
 *
 * Copyright (C) 2014 James McLaughlin.  All rights reserved.
 * https://github.com/udp/json-builder
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
static size_t
measure_string(unsigned int length,
               const char *str)
{
    unsigned int i;
    size_t measured_length = 0;

    for (i = 0; i < length; ++i) {
        int c = str[i];

        switch (c) {
        case '"':
        case '\\':
        case '\b':
        case '\f':
        case '\n':
        case '\r':
        case '\t':

            measured_length += 2;
            break;

        default:

            ++measured_length;
            break;
        };
    };

    return measured_length;
}

#define PRINT_ESCAPED(c) \
    do {                 \
        *buf++ = '\\';   \
        *buf++ = (c);    \
    } while (0);

static size_t
serialize_string(char *buf,
                 unsigned int length,
                 const char *str)
{
    char *orig_buf = buf;
    unsigned int i;

    for (i = 0; i < length; ++i) {
        int c = str[i];

        switch (c) {
        case '"':
            PRINT_ESCAPED('\"');
            continue;
        case '\\':
            PRINT_ESCAPED('\\');
            continue;
        case '\b':
            PRINT_ESCAPED('b');
            continue;
        case '\f':
            PRINT_ESCAPED('f');
            continue;
        case '\n':
            PRINT_ESCAPED('n');
            continue;
        case '\r':
            PRINT_ESCAPED('r');
            continue;
        case '\t':
            PRINT_ESCAPED('t');
            continue;

        default:

            *buf++ = c;
            break;
        };
    };

    return buf - orig_buf;
}
/* End code from json-builder */

static int
serialize_key(char *ptr, int maxlen, const char *key)
{
    int slen, len = 0;
    if (ptr) {
        *ptr++ = '\t';
        *ptr++ = '"';
        maxlen -= 2;
    }
    len += 2; /* \t\" */
    if (ptr) {
        assert(maxlen > 0);
        slen = serialize_string(ptr, strlen(key), key);
        ptr += slen;
        maxlen -= slen;
    } else {
        slen = measure_string(strlen(key), key);
    }
    len += slen;

    if (ptr) {
        assert(maxlen > 0);
        *ptr++ = '"';
        *ptr++ = ':';
        *ptr++ = ' ';
        maxlen -= 3;
    }
    len += 3; /* "\": " */
    return len;
}

static int
serialize_value(char *ptr, int maxlen, const char *val)
{
    int slen, len = 0;
    if (ptr) {
        *ptr++ = '"';
        maxlen--;
    }
    len++; /* \" */
    if (ptr) {
        assert(maxlen > 0);
        slen = serialize_string(ptr, strlen(val), val);
        ptr += slen;
        maxlen -= slen;
    } else {
        slen = measure_string(strlen(val), val);
    }
    len += slen;

    if (ptr) {
        assert(maxlen > 0);
        *ptr++ = '"';
        *ptr++ = ',';
        *ptr++ = '\n';
        maxlen -= 3;
    }
    len += 3; /* "\",\n" */
    return len;
}

static int
build_json(config_t *config, char *json, int len)
{
    hash_iter_t *itor;
    char *ptr = json;
    int l, rv = 0;

    if ((l = snprintf(ptr, len, "{\n")) < 0)
        return -1;
    rv += l;
    if (ptr) {
        len -= l;
        ptr += l;
    }
    for (itor = hash_table_iter(config->ht); itor;
         itor = hash_table_iter_next(itor)) {
        const char *key = hash_entry_key(itor->ent);
        config_val_t *cval = hash_entry_val(itor->ent);
        if (cval->type & ARG_STRING && cval->val.ptr == NULL)
            continue;
        if ((l = serialize_key(ptr, len, key)) < 0)
            return -1;
        rv += l;
        if (ptr) {
            len -= l;
            ptr += l;
        }
        if (cval->type & ARG_STRING) {
            if ((l = serialize_value(ptr, len,
                                     (char *)cval->val.ptr))
                < 0)
                return -1;
        } else if (cval->type & ARG_INTEGER) {
            if ((l = snprintf(ptr, len, "%ld,\n",
                              cval->val.i))
                < 0)
                return -1;
        } else if (cval->type & ARG_BOOLEAN) {
            if ((l = snprintf(ptr, len, "%s,\n",
                              cval->val.i ? "true" : "false"))
                < 0)
                return -1;
        } else if (cval->type & ARG_FLOATING) {
            if ((l = snprintf(ptr, len, "%g,\n",
                              cval->val.fl))
                < 0)
                return -1;
        } else {
            E_ERROR("Unknown type %d for parameter %s\n",
                    cval->type, key);
        }
        rv += l;
        if (ptr) {
            len -= l;
            ptr += l;
        }
    }
    /* Back off last comma because JSON is awful */
    if (ptr && ptr > json + 1) {
        len += 2;
        ptr -= 2;
    }
    if ((l = snprintf(ptr, len, "\n}\n")) < 0)
        return -1;
    rv += l;
    if (ptr) {
        len -= l;
        ptr += l;
    }
    return rv;
}

const char *
config_serialize_json(config_t *config)
{
    int len = build_json(config, NULL, 0);
    if (len < 0)
        return NULL;
    if (config->json)
        ckd_free(config->json);
    config->json = ckd_malloc(len + 1);
    if (build_json(config, config->json, len + 1) != len) {
        ckd_free(config->json);
        config->json = NULL;
    }
    return config->json;
}

config_type_t
config_typeof(config_t *config, const char *name)
{
    void *val;
    if (hash_table_lookup(config->ht, name, &val) < 0)
        return 0;
    if (val == NULL)
        return 0;
    return ((config_val_t *)val)->type;
}

const anytype_t *
config_get(config_t *config, const char *name)
{
    void *val;
    if (hash_table_lookup(config->ht, name, &val) < 0)
        return NULL;
    if (val == NULL)
        return NULL;
    return &((config_val_t *)val)->val;
}

long
config_int(config_t *config, const char *name)
{
    config_val_t *val;
    val = config_access(config, name);
    if (val == NULL)
        return 0L;
    if (!(val->type & (ARG_INTEGER | ARG_BOOLEAN))) {
        E_ERROR("Argument %s does not have integer type\n", name);
        return 0L;
    }
    return val->val.i;
}

int
config_bool(config_t *config, const char *name)
{
    long val = config_int(config, name);
    return val != 0;
}

double
config_float(config_t *config, const char *name)
{
    config_val_t *val;
    val = config_access(config, name);
    if (val == NULL)
        return 0.0;
    if (!(val->type & ARG_FLOATING)) {
        E_ERROR("Argument %s does not have floating-point type\n", name);
        return 0.0;
    }
    return val->val.fl;
}

const char *
config_str(config_t *config, const char *name)
{
    config_val_t *val;
    val = config_access(config, name);
    if (val == NULL)
        return NULL;
    if (!(val->type & ARG_STRING)) {
        E_ERROR("Argument %s does not have string type\n", name);
        return NULL;
    }
    return (const char *)val->val.ptr;
}

const anytype_t *
config_unset(config_t *config, const char *name)
{
    config_val_t *cval = config_access(config, name);
    const config_param_t *arg;
    if (cval == NULL) {
        E_ERROR("Unknown parameter %s\n", name);
        return NULL;
    }
    /* FIXME: Perhaps config_val_t should store a pointer to config_param_t */
    for (arg = config->defn; arg->name; ++arg) {
        if (0 == strcmp(arg->name, name)) {
            if (anytype_from_str(&cval->val, cval->type, arg->deflt) == NULL)
                return NULL;
            break;
        }
    }
    if (arg->name == NULL) {
        E_ERROR("No definition found for %s\n", name);
        return NULL;
    }
    return &cval->val;
}

const anytype_t *
config_set(config_t *config, const char *name, const anytype_t *val, config_type_t t)
{
    if (val == NULL) {
        /* Restore default parameter */
        return config_unset(config, name);
    } else if (t & ARG_STRING)
        return config_set_str(config, name, val->ptr);
    else if (t & ARG_INTEGER)
        return config_set_int(config, name, val->i);
    else if (t & ARG_BOOLEAN)
        return config_set_bool(config, name, val->i);
    else if (t & ARG_FLOATING)
        return config_set_bool(config, name, val->fl);
    else {
        E_ERROR("Value has unknown type %d\n", name, t);
        return NULL;
    }
}

anytype_t *
anytype_from_str(anytype_t *val, int t, const char *str)
{
    if (val == NULL)
        return NULL;
    if (str == NULL) {
        if (val->ptr && (t & ARG_STRING))
            ckd_free(val->ptr);
        memset(val, 0, sizeof(*val));
        return val;
    }
    if (strlen(str) == 0)
        return NULL;
    switch (t) {
    case ARG_INTEGER:
    case REQARG_INTEGER:
        if (sscanf(str, "%ld", &val->i) != 1)
            return NULL;
        break;
    case ARG_FLOATING:
    case REQARG_FLOATING:
        val->fl = atof(str);
        break;
    case ARG_BOOLEAN:
    case REQARG_BOOLEAN:
        if ((str[0] == 'y') || (str[0] == 't') || (str[0] == 'Y') || (str[0] == 'T') || (str[0] == '1')) {
            val->i = TRUE;
        } else if ((str[0] == 'n') || (str[0] == 'f') || (str[0] == 'N') || (str[0] == 'F') | (str[0] == '0')) {
            val->i = FALSE;
        } else {
            E_ERROR("Unparsed boolean value '%s'\n", str);
            return NULL;
        }
        break;
    case ARG_STRING:
    case REQARG_STRING:
        if (val->ptr)
            ckd_free(val->ptr);
        val->ptr = ckd_salloc(str);
        break;
    default:
        E_ERROR("Unknown argument type: %d\n", t);
        return NULL;
    }
    return val;
}

const anytype_t *
config_set_str(config_t *config, const char *name, const char *val)
{
    config_val_t *cval = config_access(config, name);
    if (cval == NULL) {
        E_ERROR("Unknown parameter %s\n", name);
        return NULL;
    }
    if (anytype_from_str(&cval->val, cval->type, val) == NULL) {
        return NULL;
    }
    return &cval->val;
}

anytype_t *
anytype_from_int(anytype_t *val, int t, long i)
{
    if (val == NULL)
        return NULL;
    switch (t) {
    case ARG_INTEGER:
    case REQARG_INTEGER:
        val->i = i;
        break;
    case ARG_FLOATING:
    case REQARG_FLOATING:
        val->fl = (double)i;
        break;
    case ARG_BOOLEAN:
    case REQARG_BOOLEAN:
        val->i = (i != 0);
        break;
    case ARG_STRING:
    case REQARG_STRING: {
        int len = snprintf(NULL, 0, "%ld", i);
        if (len < 0) {
            E_ERROR_SYSTEM("snprintf() failed");
            return NULL;
        }
        val->ptr = ckd_malloc(len + 1);
        if (snprintf(val->ptr, len + 1, "%ld", i) != len) {
            E_ERROR_SYSTEM("snprintf() failed");
            return NULL;
        }
        break;
    }
    default:
        E_ERROR("Unknown argument type: %d\n", t);
        return NULL;
    }
    return val;
}

const anytype_t *
config_set_int(config_t *config, const char *name, long val)
{
    config_val_t *cval = config_access(config, name);
    if (cval == NULL) {
        E_ERROR("Unknown parameter %s\n", name);
        return NULL;
    }
    if (anytype_from_int(&cval->val, cval->type, val) == NULL) {
        return NULL;
    }
    return &cval->val;
}

anytype_t *
anytype_from_float(anytype_t *val, int t, double f)
{
    if (val == NULL)
        return NULL;
    switch (t) {
    case ARG_INTEGER:
    case REQARG_INTEGER:
        val->i = (long)f;
        break;
    case ARG_FLOATING:
    case REQARG_FLOATING:
        val->fl = f;
        break;
    case ARG_BOOLEAN:
    case REQARG_BOOLEAN:
        val->i = (f != 0.0);
        break;
    case ARG_STRING:
    case REQARG_STRING: {
        int len = snprintf(NULL, 0, "%g", f);
        if (len < 0) {
            E_ERROR_SYSTEM("snprintf() failed");
            return NULL;
        }
        val->ptr = ckd_malloc(len + 1);
        if (snprintf(val->ptr, len + 1, "%g", f) != len) {
            E_ERROR_SYSTEM("snprintf() failed");
            return NULL;
        }
        break;
    }
    default:
        E_ERROR("Unknown argument type: %d\n", t);
        return NULL;
    }
    return val;
}

const anytype_t *
config_set_float(config_t *config, const char *name, double val)
{
    config_val_t *cval = config_access(config, name);
    if (cval == NULL) {
        E_ERROR("Unknown parameter %s\n", name);
        return NULL;
    }
    if (anytype_from_float(&cval->val, cval->type, val) == NULL) {
        return NULL;
    }
    return &cval->val;
}

const anytype_t *
config_set_bool(config_t *config, const char *name, int val)
{
    return config_set_int(config, name, val != 0);
}

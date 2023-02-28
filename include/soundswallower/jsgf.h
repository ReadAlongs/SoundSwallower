/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2007 Carnegie Mellon University.  All rights
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

#ifndef __JSGF_H__
#define __JSGF_H__

/**
 * @file jsgf.h
 * @brief JSGF grammar compiler
 *
 * This file defines the data structures for parsing JSGF grammars
 * into Sphinx finite-state grammars.
 **/

#include <stdio.h>

#include <soundswallower/fsg_model.h>
#include <soundswallower/hash_table.h>
#include <soundswallower/logmath.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

typedef struct jsgf_s jsgf_t;
typedef struct jsgf_rule_s jsgf_rule_t;

/**
 * Create a new JSGF grammar.
 *
 * @param parent optional parent grammar for this one (NULL, usually).
 * @return new JSGF grammar object, or NULL on failure.
 */
jsgf_t *jsgf_grammar_new(jsgf_t *parent);

/**
 * Parse a JSGF grammar from a file.
 *
 * @param filename the name of the file to parse.
 * @param parent optional parent grammar for this one (NULL, usually).
 * @return new JSGF grammar object, or NULL on failure.
 */
jsgf_t *jsgf_parse_file(const char *filename, jsgf_t *parent);

/**
 * Parse a JSGF grammar from a string.
 *
 * @param 0-terminated string with grammar.
 * @param parent optional parent grammar for this one (NULL, usually).
 * @return new JSGF grammar object, or NULL on failure.
 */
jsgf_t *jsgf_parse_string(const char *string, jsgf_t *parent);

/**
 * Get the grammar name from the file.
 */
const char *jsgf_grammar_name(jsgf_t *jsgf);

/**
 * Free a JSGF grammar.
 */
void jsgf_grammar_free(jsgf_t *jsgf);

/**
 * Iterator over rules in a grammar.
 */
typedef hash_iter_t jsgf_rule_iter_t;

/**
 * Get an iterator over all rules in a grammar.
 */
jsgf_rule_iter_t *jsgf_rule_iter(jsgf_t *grammar);

/**
 * Advance an iterator to the next rule in the grammar.
 */
#define jsgf_rule_iter_next(itor) hash_table_iter_next(itor)

/**
 * Get the current rule in a rule iterator.
 */
#define jsgf_rule_iter_rule(itor) ((jsgf_rule_t *)(itor)->ent->val)

/**
 * Free a rule iterator (if the end hasn't been reached).
 */
#define jsgf_rule_iter_free(itor) hash_table_iter_free(itor)

/**
 * Get a rule by name from a grammar. Name should not contain brackets.
 */
jsgf_rule_t *jsgf_get_rule(jsgf_t *grammar, const char *name);

/**
 * Returns the first public rule of the grammar
 */
jsgf_rule_t *jsgf_get_public_rule(jsgf_t *grammar);

/**
 * Get the rule name from a rule.
 */
const char *jsgf_rule_name(jsgf_rule_t *rule);

/**
 * Test if a rule is public or not.
 */
int jsgf_rule_public(jsgf_rule_t *rule);

/**
 * Build a Sphinx FSG object from a JSGF rule.
 */
fsg_model_t *jsgf_build_fsg(jsgf_t *grammar, jsgf_rule_t *rule,
                            logmath_t *lmath, float32 lw);

/**
 * Build a Sphinx FSG object from a JSGF rule.
 *
 * This differs from jsgf_build_fsg() in that it does not do closure
 * on epsilon transitions or any other postprocessing.  For the time
 * being this is necessary in order to write it to a file - the FSG
 * code will be fixed soon.
 */
fsg_model_t *jsgf_build_fsg_raw(jsgf_t *grammar, jsgf_rule_t *rule,
                                logmath_t *lmath, float32 lw);

/**
 * Read JSGF from file and return FSG object from it.
 *
 * This function looks for a first public rule in jsgf and constructs JSGF from it.
 */
fsg_model_t *jsgf_read_file(const char *file, logmath_t *lmath, float32 lw);

/**
 * Read JSGF from string and return FSG object from it.
 *
 * This function looks for a first public rule in jsgf and constructs JSGF from it.
 */
fsg_model_t *jsgf_read_string(const char *string, logmath_t *lmath, float32 lw);

#define YY_NO_INPUT /* Silence a compiler warning. */

typedef struct jsgf_rhs_s jsgf_rhs_t;
typedef struct jsgf_atom_s jsgf_atom_t;
typedef struct jsgf_link_s jsgf_link_t;

struct jsgf_s {
    char *version; /**< JSGF version (from header) */
    char *charset; /**< JSGF charset (default UTF-8) */
    char *locale; /**< JSGF locale (default C) */
    char *name; /**< Grammar name */

    hash_table_t *rules; /**< Defined or imported rules in this grammar. */
    hash_table_t *imports; /**< Pointers to imported grammars. */
    jsgf_t *parent; /**< Parent grammar (if this is an imported one) */
    glist_t searchpath; /**< List of directories to search for grammars. */

    /* Scratch variables for FSG conversion. */
    int nstate; /**< Number of generated states. */
    glist_t links; /**< Generated FSG links. */
    glist_t rulestack; /**< Stack of currently expanded rules. */
};

struct jsgf_rule_s {
    int refcnt; /**< Reference count. */
    char *name; /**< Rule name (NULL for an alternation/grouping) */
    int is_public; /**< Is this rule marked 'public'? */
    jsgf_rhs_t *rhs; /**< Expansion */

    int entry; /**< Entry state for current instance of this rule. */
    int exit; /**< Exit state for current instance of this rule. */
};

struct jsgf_rhs_s {
    glist_t atoms; /**< Sequence of items */
    jsgf_rhs_t *alt; /**< Linked list of alternates */
};

struct jsgf_atom_s {
    char *name; /**< Rule or token name */
    glist_t tags; /**< Tags, if any (glist_t of char *) */
    float weight; /**< Weight (default 1) */
};

struct jsgf_link_s {
    jsgf_atom_t *atom; /**< Name, tags, weight */
    int from; /**< From state */
    int to; /**< To state */
};

#define jsgf_atom_is_rule(atom) ((atom)->name[0] == '<')

void jsgf_add_link(jsgf_t *grammar, jsgf_atom_t *atom, int from, int to);
jsgf_atom_t *jsgf_atom_new(char *name, float weight);
jsgf_atom_t *jsgf_kleene_new(jsgf_t *jsgf, jsgf_atom_t *atom, int plus);
jsgf_rule_t *jsgf_optional_new(jsgf_t *jsgf, jsgf_rhs_t *exp);
jsgf_rule_t *jsgf_define_rule(jsgf_t *jsgf, char *name, jsgf_rhs_t *rhs, int is_public);
jsgf_rule_t *jsgf_import_rule(jsgf_t *jsgf, char *name);

int jsgf_atom_free(jsgf_atom_t *atom);
int jsgf_rule_free(jsgf_rule_t *rule);
jsgf_rule_t *jsgf_rule_retain(jsgf_rule_t *rule);

#ifdef __cplusplus
}
#endif

#endif /* __JSGF_H__ */

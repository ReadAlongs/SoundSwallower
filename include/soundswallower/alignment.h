/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2010 Carnegie Mellon University.  All rights
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

/**
 * @file alignment.h Multi-level alignment structure
 */

#ifndef __ALIGNMENT_H__
#define __ALIGNMENT_H__

#include <soundswallower/dict2pid.h>
#include <soundswallower/hmm.h>
#include <soundswallower/prim_type.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * @struct alignment_entry_t
 * @brief Entry (phone, word, or state) in an alignment
 */
typedef struct alignment_entry_s {
    int32 start; /**< Start frame index. */
    int32 duration; /**< Duration in frames. */
    int32 score; /**< Alignment score (fairly meaningless). */
    /**
     * Index of parent node.
     *
     * You can use this to determine if you have crossed a parent
     * boundary.  For example if you wish to iterate only over phones
     * inside a word, you can store this for the first phone and stop
     * iterating once it changes. */
    int parent;
    int child; /**< Index of child node. */
    /**
     * ID or IDs for this entry.
     *
     * This is complex, though perhaps not needlessly so.  We need all
     * this information to do state alignment.
     */
    union {
        int32 wid; /**< Word ID (for words) */
        struct {
            int16 cipid; /**< Phone ID, which you care about. */
            uint16 ssid; /**< Senone sequence ID, which you don't. */
            int32 tmatid; /**< Transition matrix ID, almost certainly
                             the same as cipid. */
        } pid;
        uint16 senid;
    } id;
} alignment_entry_t;

struct alignment_vector_s {
    alignment_entry_t *seq;
    uint16 n_ent, n_alloc;
};
typedef struct alignment_vector_s alignment_vector_t;

/**
 * @struct alignment_t
 * @brief Multi-level alignment (words, phones, states) over an utterance.
 */
typedef struct alignment_s alignment_t;
struct alignment_s {
    int refcount;
    dict2pid_t *d2p;
    alignment_vector_t word;
    alignment_vector_t sseq;
    alignment_vector_t state;
};

/**
 * @struct alignment_iter_t
 * @brief Iterator over entries in an alignment.
 */
typedef struct alignment_iter_s alignment_iter_t;
struct alignment_iter_s {
    alignment_t *al;
    alignment_vector_t *vec;
    int pos;
    int parent;
    char *name;
};

#define alignment_n_words(al) (int)(al)->word.n_ent
#define alignment_n_phones(al) (int)(al)->sseq.n_ent
#define alignment_n_states(al) (int)(al)->state.n_ent

/**
 * Value indicating no parent or child for an entry.
 * @related alignment_t
 */
#define ALIGNMENT_NONE -1

/**
 * Create a new, empty alignment.
 */
alignment_t *alignment_init(dict2pid_t *d2p);

/**
 * Append a word.
 */
int alignment_add_word(alignment_t *al,
                       int32 wid, int start, int duration);

/**
 * Populate lower layers using available word information.
 */
int alignment_populate(alignment_t *al);

/**
 * Populate lower layers using context-independent phones.
 */
int alignment_populate_ci(alignment_t *al);

/**
 * Propagate timing information up from state sequence.
 */
int alignment_propagate(alignment_t *al);

/**
 * Get the alignment entry pointed to by an iterator.
 *
 * The iterator retains ownership of this so don't try to free it.
 */
alignment_entry_t *alignment_iter_get(alignment_iter_t *itor);

/**
 * Move alignment iterator to given index.
 */
alignment_iter_t *alignment_iter_goto(alignment_iter_t *itor, int pos);

/**
 * Retain an alighment
 * @memberof alignment_t
 */
alignment_t *alignment_retain(alignment_t *al);

/**
 * Release an alignment
 * @memberof alignment_t
 */
int alignment_free(alignment_t *al);

/**
 * Iterate over the alignment starting at the first word.
 * @memberof alignment_t
 */
alignment_iter_t *alignment_words(alignment_t *al);

/**
 * Iterate over the alignment starting at the first phone.
 * @memberof alignment_t
 */
alignment_iter_t *alignment_phones(alignment_t *al);

/**
 * Iterate over the alignment starting at the first state.
 * @memberof alignment_t
 */
alignment_iter_t *alignment_states(alignment_t *al);

/**
 * Get the human-readable name of the current segment for an alignment.
 *
 * @memberof alignment_iter_t
 * @return Name of this segment as a string (word, phone, or state
 * number).  This pointer is owned by the iterator, do not free it
 * yourself.
 */
const char *alignment_iter_name(alignment_iter_t *itor);

/**
 * Get the timing and score information for the current segment of an aligment.
 *
 * @memberof alignment_iter_t
 * @arg start Output pointer for start frame
 * @arg duration Output pointer for duration
 * @return Acoustic score for this segment
 */
int alignment_iter_seg(alignment_iter_t *itor, int *start, int *duration);

/**
 * Move an alignment iterator forward.
 *
 * If the end of the alignment is reached, this will free the iterator
 * and return NULL.
 *
 * @memberof alignment_iter_t
 */
alignment_iter_t *alignment_iter_next(alignment_iter_t *itor);

/**
 * Iterate over the children of the current alignment entry.
 *
 * If there are no child nodes, NULL is returned.
 *
 * @memberof alignment_iter_t
 */
alignment_iter_t *alignment_iter_children(alignment_iter_t *itor);

/**
 * Release an iterator before completing all iterations.
 *
 * @memberof alignment_iter_t
 */
int alignment_iter_free(alignment_iter_t *itor);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __ALIGNMENT_H__ */

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

#include <string.h>

#include <soundswallower/dict.h>
#include <soundswallower/strfuncs.h>

#define DELIM " \t\n" /* Set of field separator characters */
#define DEFAULT_NUM_PHONE (MAX_S3CIPID + 1)

#if WIN32
#define snprintf sprintf_s
#endif

extern const char *const cmu6_lts_phone_table[];

static s3cipid_t
dict_ciphone_id(dict_t *d, const char *str)
{
    if (d->nocase)
        return bin_mdef_ciphone_id_nocase(d->mdef, str);
    else
        return bin_mdef_ciphone_id(d->mdef, str);
}

const char *
dict_ciphone_str(dict_t *d, s3wid_t wid, int32 pos)
{
    assert(d != NULL);
    assert((wid >= 0) && (wid < d->n_word));
    assert((pos >= 0) && (pos < d->word[wid].pronlen));

    return bin_mdef_ciphone_str(d->mdef, d->word[wid].ciphone[pos]);
}

s3wid_t
dict_add_word(dict_t *d, const char *word, s3cipid_t const *p, int32 np)
{
    int32 len;
    dictword_t *wordp;
    s3wid_t newwid;
    char *wword;

    if (d->n_word >= d->max_words) {
        E_INFO("Reallocating to %d KiB for word entries\n",
               (d->max_words + S3DICT_INC_SZ) * sizeof(dictword_t) / 1024);
        d->word = (dictword_t *)ckd_realloc(d->word,
                                            (d->max_words + S3DICT_INC_SZ) * sizeof(dictword_t));
        d->max_words = d->max_words + S3DICT_INC_SZ;
    }

    wordp = d->word + d->n_word;
    wordp->word = (char *)ckd_salloc(word); /* Freed in dict_free */

    /* Determine base/alt wids */
    wword = ckd_salloc(word);
    if ((len = dict_word2basestr(wword)) > 0) {
        int32 w;

        /* Truncated to a baseword string; find its ID */
        if (hash_table_lookup_int32(d->ht, wword, &w) < 0) {
            E_ERROR("Missing base word for: %s\n", word);
            ckd_free(wword);
            ckd_free(wordp->word);
            wordp->word = NULL;
            return BAD_S3WID;
        }

        /* Link into alt list */
        wordp->basewid = w;
        wordp->alt = d->word[w].alt;
        d->word[w].alt = d->n_word;
    } else {
        wordp->alt = BAD_S3WID;
        wordp->basewid = d->n_word;
    }
    ckd_free(wword);

    /* Associate word string with d->n_word in hash table */
    if (hash_table_enter_int32(d->ht, wordp->word, d->n_word) != d->n_word) {
        ckd_free(wordp->word);
        wordp->word = NULL;
        return BAD_S3WID;
    }

    /* Fill in word entry, and set defaults */
    if (p && (np > 0)) {
        wordp->ciphone = (s3cipid_t *)ckd_malloc(np * sizeof(s3cipid_t)); /* Freed in dict_free */
        memcpy(wordp->ciphone, p, np * sizeof(s3cipid_t));
        wordp->pronlen = np;
    } else {
        wordp->ciphone = NULL;
        wordp->pronlen = 0;
    }

    newwid = d->n_word++;

    return newwid;
}

dict_t *
dict_init(config_t *config, bin_mdef_t *mdef)
{
    dict_t *d = NULL;
    s3file_t *dict = NULL;
    s3file_t *fdict = NULL;

    if (config) {
        const char *path;
        if ((path = config_str(config, "dict")) != NULL) {
            if ((dict = s3file_map_file(path)) == NULL) {
                E_ERROR_SYSTEM("Failed to read dictionary from %s", path);
                goto error_out;
            }
        }
        if ((path = config_str(config, "fdict")) != NULL) {
            if ((fdict = s3file_map_file(path)) == NULL) {
                E_ERROR_SYSTEM("Failed to read filler dictionary from %s", path);
                goto error_out;
            }
        }
    }
    d = dict_init_s3file(config, mdef, dict, fdict);
error_out:
    s3file_free(dict);
    s3file_free(fdict);
    return d;
}

static int32
dict_read_s3file(dict_t *d, s3file_t *dict)
{
    const char *line, *ptr;
    s3cipid_t *p = NULL;
    int32 lineno, nwd;
    s3wid_t w;
    int32 i, maxwd;
    size_t stralloc, phnalloc;

    lineno = 0;
    stralloc = phnalloc = 0;
    while ((ptr = line = s3file_nextline(dict)) != NULL) {
        char *word;

        lineno++;
        if (0 == strncmp(line, "##", 2)
            || 0 == strncmp(line, ";;", 2))
            continue;

        for (nwd = 0; s3file_nextword(dict, &ptr); ++nwd)
            /* Count words */;
        ptr = line; /* Reset to line start */
        if (p == NULL) {
            maxwd = nwd * 2; /* Some extra space */
            p = ckd_calloc(maxwd, sizeof(*p));
        }
        if (nwd == 0) /* Empty line */
            continue;
        /* Increase size of p, wptr if needed. */
        while (nwd > maxwd) {
            maxwd *= 2;
            E_INFO("Increased maximum words/phones to %ld\n", maxwd);
            p = (s3cipid_t *)ckd_realloc(p, maxwd * sizeof(*p));
        }
        /* FIXME: Might be slow with all the copying */
        word = s3file_copy_nextword(dict, &ptr);
        if (nwd == 1) {
            E_ERROR("Line %d: No pronunciation for word '%s'; ignored\n",
                    lineno, word);
            ckd_free(word);
            continue;
        }
        /* Convert pronunciation string to CI-phone-ids */
        for (i = 1; i < nwd; i++) {
            /* FIXME: Might be slow with all the copying */
            char *phone = s3file_copy_nextword(dict, &ptr);
            p[i - 1] = dict_ciphone_id(d, phone);
            if (NOT_S3CIPID(p[i - 1])) {
                E_ERROR("Line %d: Phone '%s' is missing in the acoustic model; word '%s' ignored\n",
                        lineno, phone, word);
                ckd_free(phone);
                break;
            }
            ckd_free(phone);
        }
        if (i == nwd) { /* All CI-phones successfully converted to IDs */
            w = dict_add_word(d, word, p, nwd - 1);
            if (NOT_S3WID(w))
                E_ERROR("Line %d: Failed to add the word '%s' (duplicate?); ignored\n",
                        lineno, word);
            else {
                stralloc += strlen(d->word[w].word);
                phnalloc += d->word[w].pronlen * sizeof(s3cipid_t);
            }
        }
        ckd_free(word);
    }
    E_INFO("Dictionary size %d, allocated %d KiB for strings, %d KiB for phones\n",
           dict_size(d), (int)stralloc / 1024, (int)phnalloc / 1024);
    ckd_free(p);

    return 0;
}

dict_t *
dict_init_s3file(config_t *config, bin_mdef_t *mdef, s3file_t *dict, s3file_t *fdict)
{
    int32 n;
    dict_t *d;
    s3cipid_t sil;
    const char *line;

    /*
     * First obtain #words in dictionary (for hash table allocation).
     * Reason: The PC NT system doesn't like to grow memory gradually.  Better to allocate
     * all the required memory in one go.
     */
    n = 0;
    if (dict) {
        while ((line = s3file_nextline(dict)) != NULL) {
            if (0 != strncmp(line, "##", 2)
                && 0 != strncmp(line, ";;", 2))
                n++;
        }
        dict->ptr = dict->buf;
    }

    if (fdict) {
        while ((line = s3file_nextline(fdict)) != NULL) {
            if (0 != strncmp(line, "##", 2)
                && 0 != strncmp(line, ";;", 2))
                n++;
        }
        fdict->ptr = fdict->buf;
    }

    /*
     * Allocate dict entries.  HACK!!  Allow some extra entries for words not in file.
     * Also check for type size restrictions.
     */
    d = (dict_t *)ckd_calloc(1, sizeof(dict_t)); /* freed in dict_free() */
    d->refcnt = 1;
    d->max_words = (n + S3DICT_INC_SZ < MAX_S3WID) ? n + S3DICT_INC_SZ : MAX_S3WID;
    if (n >= MAX_S3WID) {
        E_ERROR("Number of words in dictionaries (%d) exceeds limit (%d)\n", n,
                MAX_S3WID);
        goto error_out;
    }

    E_INFO("Allocating %d * %d bytes (%d KiB) for word entries\n",
           d->max_words, sizeof(dictword_t),
           d->max_words * sizeof(dictword_t) / 1024);
    d->word = (dictword_t *)ckd_calloc(d->max_words, sizeof(dictword_t)); /* freed in dict_free() */
    d->n_word = 0;
    if (mdef)
        d->mdef = bin_mdef_retain(mdef);

    /* Create new hash table for word strings; case-insensitive word strings */
    if (config && config_exists(config, "dictcase"))
        d->nocase = config_bool(config, "dictcase");
    d->ht = hash_table_new(d->max_words, d->nocase);

    /* Digest main dictionary file */
    if (dict) {
        dict_read_s3file(d, dict);
        E_INFO("%d words read\n", d->n_word);
    }

    if (dict_wordid(d, S3_START_WORD) != BAD_S3WID) {
        E_ERROR("Remove sentence start word '<s>' from the dictionary\n");
        goto error_out;
    }
    if (dict_wordid(d, S3_FINISH_WORD) != BAD_S3WID) {
        E_ERROR("Remove sentence start word '</s>' from the dictionary\n");
        goto error_out;
    }
    if (dict_wordid(d, S3_SILENCE_WORD) != BAD_S3WID) {
        E_ERROR("Remove silence word '<sil>' from the dictionary\n");
        goto error_out;
    }

    /* Now the filler dictionary file, if it exists */
    d->filler_start = d->n_word;
    if (fdict) {
        dict_read_s3file(d, fdict);
        E_INFO("%d words read\n", d->n_word - d->filler_start);
    }
    if (mdef)
        sil = bin_mdef_silphone(mdef);
    else
        sil = 0;
    if (dict_wordid(d, S3_START_WORD) == BAD_S3WID) {
        dict_add_word(d, S3_START_WORD, &sil, 1);
    }
    if (dict_wordid(d, S3_FINISH_WORD) == BAD_S3WID) {
        dict_add_word(d, S3_FINISH_WORD, &sil, 1);
    }
    if (dict_wordid(d, S3_SILENCE_WORD) == BAD_S3WID) {
        dict_add_word(d, S3_SILENCE_WORD, &sil, 1);
    }

    d->filler_end = d->n_word - 1;

    /* Initialize distinguished word-ids */
    d->startwid = dict_wordid(d, S3_START_WORD);
    d->finishwid = dict_wordid(d, S3_FINISH_WORD);
    d->silwid = dict_wordid(d, S3_SILENCE_WORD);

    if ((d->filler_start > d->filler_end)
        || (!dict_filler_word(d, d->silwid))) {
        E_ERROR("Word '%s' must occur (only) in filler dictionary\n",
                S3_SILENCE_WORD);
        goto error_out;
    }

    /* No check that alternative pronunciations for filler words are in filler range!! */
    return d;

error_out:
    dict_free(d);
    return NULL;
}

s3wid_t
dict_wordid(dict_t *d, const char *word)
{
    int32 w;

    assert(d);
    assert(word);

    if (hash_table_lookup_int32(d->ht, word, &w) < 0)
        return (BAD_S3WID);
    return w;
}

int
dict_filler_word(dict_t *d, s3wid_t w)
{
    assert(d);
    assert((w >= 0) && (w < d->n_word));

    w = dict_basewid(d, w);
    if ((w == d->startwid) || (w == d->finishwid))
        return 0;
    if ((w >= d->filler_start) && (w <= d->filler_end))
        return 1;
    return 0;
}

int
dict_real_word(dict_t *d, s3wid_t w)
{
    assert(d);
    assert((w >= 0) && (w < d->n_word));

    w = dict_basewid(d, w);
    if ((w == d->startwid) || (w == d->finishwid))
        return 0;
    if ((w >= d->filler_start) && (w <= d->filler_end))
        return 0;
    return 1;
}

int32
dict_word2basestr(char *word)
{
    int32 i, len;

    len = (int32)strlen(word);
    if (word[len - 1] == ')') {
        for (i = len - 2; (i > 0) && (word[i] != '('); --i)
            ;

        if (i > 0) {
            /* The word is of the form <baseword>(...); strip from left-paren */
            word[i] = '\0';
            return i;
        }
    }

    return -1;
}

dict_t *
dict_retain(dict_t *d)
{
    if (d == NULL)
        return NULL;
    ++d->refcnt;
    return d;
}

int
dict_free(dict_t *d)
{
    int i;
    dictword_t *word;

    if (d == NULL)
        return 0;
    if (--d->refcnt > 0)
        return d->refcnt;

    /* First Step, free all memory allocated for each word */
    for (i = 0; i < d->n_word; i++) {
        word = (dictword_t *)&(d->word[i]);
        if (word->word)
            ckd_free((void *)word->word);
        if (word->ciphone)
            ckd_free((void *)word->ciphone);
    }

    if (d->word)
        ckd_free((void *)d->word);
    if (d->ht)
        hash_table_free(d->ht);
    if (d->mdef)
        bin_mdef_free(d->mdef);
    ckd_free((void *)d);

    return 0;
}

void
dict_report(dict_t *d)
{
    E_INFO_NOFN("Initialization of dict_t, report:\n");
    E_INFO_NOFN("Max word: %d\n", d->max_words);
    E_INFO_NOFN("No of word: %d\n", d->n_word);
    E_INFO_NOFN("\n");
}

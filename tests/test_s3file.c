/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <soundswallower/ckd_alloc.h>
#include <soundswallower/err.h>
#include <soundswallower/s3file.h>
#include <soundswallower/tmat.h>
#include <stdio.h>

#include "test_macros.h"

const char data_le[] = "s3\n"
                       "key1 value1\n"
                       "key2  value2\n"
                       "# a comment\n"
                       "endhdr\n"
                       "\x44\x33\x22\x11"
                       "\xcd\xab"
                       "\xef\xbe\xad\xde"
                       "\x78\x56\x34\x12\xef\xbe\xad\xde";
const char data_be[] = "s3\n"
                       "key1 value1\n"
                       "key2  value2\n"
                       "# a comment\n"
                       "endhdr\n"
                       "\x11\x22\x33\x44"
                       "\xab\xcd"
                       "\xde\xad\xbe\xef"
                       "\xde\xad\xbe\xef\x12\x34\x56\x78";
const char data_str[] = "word1 word2 word3\n"
                        "\n" /* blank line */
                        "  word\n" /* leading whitespace */
                        "word   \n" /* trailing whitespace */
                        "word1 word2"; /* no EOL */

static void
should_have_nwords(s3file_t *s, size_t expected)
{
    const char *word, *ptr, *line;
    size_t nwords;

    ptr = line = s3file_nextline(s);
    TEST_ASSERT(ptr != NULL);
    nwords = 0;
    while ((word = s3file_nextword(s, &ptr)) != NULL) {
        E_INFO("|%.*s|\n", ptr - word, word);
        ++nwords;
    }
    TEST_EQUAL(nwords, expected);
}

static void
should_have_word(s3file_t *s, const char *expected)
{
    const char *word = s3file_nextword(s, NULL);
    TEST_ASSERT(word != NULL);
    TEST_ASSERT(0 == strncmp(word, expected, s->ptr - word));
}

static void
test_tokens(void)
{
    s3file_t *s;

    s = s3file_init(data_str, sizeof(data_str));
    /* Test line-oriented scanning (note that all this may be
       redundant with yyscan, which we should perhaps use instead) */
    should_have_nwords(s, 3);
    should_have_nwords(s, 0);
    should_have_nwords(s, 1);
    should_have_nwords(s, 1);
    should_have_nwords(s, 2);

    /* Test word-oriented scanning. */
    s->ptr = s->buf;
    should_have_word(s, "word1");
    should_have_word(s, "word2");
    should_have_word(s, "word3");
    should_have_word(s, "word");
    should_have_word(s, "word");
    should_have_word(s, "word1");
    should_have_word(s, "word2");
    s3file_free(s);
}

static void
test_read_tmat(s3file_t *s)
{
    int32 n_tmat, n_src, n_dst, n_val;
    float32 **tp;
    size_t i;

    TEST_EQUAL(0, s3file_parse_header(s, "1.0"));
    TEST_EQUAL(1, s3file_get(&n_tmat, sizeof(n_tmat), 1, s));
    TEST_EQUAL(1, s3file_get(&n_src, sizeof(n_src), 1, s));
    TEST_EQUAL(1, s3file_get(&n_dst, sizeof(n_dst), 1, s));
    TEST_EQUAL(1, s3file_get(&n_val, sizeof(n_val), 1, s));
    TEST_EQUAL(n_val, n_tmat * n_src * n_dst);
    tp = ckd_calloc_2d(n_src, n_dst, sizeof(**tp));
    for (i = 0; i < (size_t)n_tmat; ++i) {
        TEST_EQUAL((size_t)(n_src * n_dst),
                   s3file_get(tp[0], sizeof(**tp), n_src * n_dst, s));
    }
    if (s->do_chksum)
        TEST_ASSERT(0 == s3file_verify_chksum(s));
    TEST_EQUAL(s->ptr, s->end);
    ckd_free_2d(tp);
}

int
main(int argc, char *argv[])
{
    FILE *fh;
    size_t len;
    char *data;
    s3file_t *s;
    uint64 i64;
    uint32 i32;
    uint16 i16;

    (void)argc;
    (void)argv;
    err_set_loglevel(ERR_INFO);

    /* Little-endian data */
    s = s3file_init(data_le, sizeof(data_le));
    s = s3file_retain(s);
    TEST_EQUAL(0, s3file_parse_header(s, NULL));
    TEST_EQUAL(1, s3file_get(&i16, sizeof(i16), 1, s));
    TEST_EQUAL(0xabcd, i16);
    TEST_EQUAL(1, s3file_get(&i32, sizeof(i32), 1, s));
    TEST_EQUAL(0xdeadbeefUL, i32);
    TEST_EQUAL(1, s3file_get(&i64, sizeof(i64), 1, s));
    TEST_EQUAL(0xdeadbeef12345678ULL, i64);
    TEST_EQUAL(1, s3file_free(s));
    TEST_EQUAL(0, s3file_free(s));
    /* Big-endian data */
    s = s3file_init(data_be, sizeof(data_be));
    TEST_EQUAL(0, s3file_parse_header(s, NULL));
    TEST_EQUAL(1, s3file_get(&i16, sizeof(i16), 1, s));
    TEST_EQUAL(0xabcd, i16);
    TEST_EQUAL(1, s3file_get(&i32, sizeof(i32), 1, s));
    TEST_EQUAL(0xdeadbeefUL, i32);
    TEST_EQUAL(1, s3file_get(&i64, sizeof(i64), 1, s));
    TEST_EQUAL(0xdeadbeef12345678ULL, i64);
    /* Headers */
    TEST_ASSERT(s3file_header_name_is(s, 0, "key1"));
    TEST_ASSERT(s3file_header_value_is(s, 0, "value1"));
    TEST_ASSERT(s3file_header_name_is(s, 1, "key2"));
    TEST_ASSERT(s3file_header_value_is(s, 1, "value2"));
    data = s3file_copy_header_name(s, 0);
    TEST_EQUAL(0, strcmp(data, "key1"));
    ckd_free(data);
    data = s3file_copy_header_value(s, 0);
    TEST_EQUAL(0, strcmp(data, "value1"));
    ckd_free(data);
    TEST_EQUAL(0, s3file_free(s));

    /* An actual S3 file */
    fh = fopen(MODELDIR "/en-us/transition_matrices", "rb");
    TEST_ASSERT(fh != NULL);
    fseek(fh, 0, SEEK_END);
    len = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    data = ckd_malloc(len);
    TEST_EQUAL(len, fread(data, 1, len, fh));
    s = s3file_init(data, len);
    test_read_tmat(s);
    s3file_rewind(s);
    test_read_tmat(s);
    ckd_free(data);
    s3file_free(s);
    fclose(fh);

    /* Now with mmap */
    s = s3file_map_file(MODELDIR "/en-us/transition_matrices");
    test_read_tmat(s);
    s3file_free(s);

    /* Simple tokenization */
    test_tokens();

    return 0;
}

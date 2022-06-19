/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <soundswallower/s3file.h>
#include <soundswallower/err.h>
#include <soundswallower/ckd_alloc.h>
#include <stdio.h>

#include "test_macros.h"

const char data_le[] = "s3\n"
    "key1 value1\n"
    "key2  value2\n"
    "# a comment\n"
    "endhdr\n"
    "\x44\x33\x22\x11"
    "\xcd\xab"
    "\xef\xbe\xad\xde";
const char data_be[] = "s3\n"
    "key1 value1\n"
    "key2  value2\n"
    "# a comment\n"
    "endhdr\n"
    "\x11\x22\x33\x44"
    "\xab\xcd"
    "\xde\xad\xbe\xef";

int
main(int argc, char *argv[])
{
    FILE *fh;
    size_t len;
    char *data;
    s3file_t *s;
    uint32 i32;
    uint16 i16;

    err_set_loglevel(ERR_INFO);

    /* Little-endian data */
    s = s3file_init(data_le, sizeof(data_le));
    s = s3file_retain(s);
    TEST_EQUAL(0, s3file_parse_header(s));
    TEST_EQUAL(1, s3file_get(&i16, sizeof(i16), 1, s));
    TEST_EQUAL(0xabcd, i16);
    TEST_EQUAL(1, s3file_get(&i32, sizeof(i32), 1, s));
    TEST_EQUAL(0xdeadbeef, i32);
    TEST_EQUAL(1, s3file_free(s));
    TEST_EQUAL(0, s3file_free(s));
    /* Big-endian data */
    s = s3file_init(data_be, sizeof(data_be));
    TEST_EQUAL(0, s3file_parse_header(s));
    TEST_EQUAL(1, s3file_get(&i16, sizeof(i16), 1, s));
    TEST_EQUAL(0xabcd, i16);
    TEST_EQUAL(1, s3file_get(&i32, sizeof(i32), 1, s));
    TEST_EQUAL(0xdeadbeef, i32);
    /* Headers */
    TEST_ASSERT(s3file_header_name_is(s, 0, "key1"));
    TEST_ASSERT(s3file_header_value_is(s, 0, "value1"));
    TEST_ASSERT(s3file_header_name_is(s, 1, "key2"));
    TEST_ASSERT(s3file_header_value_is(s, 1, "value2"));
    data = s3file_header_name(s, 0);
    TEST_EQUAL(0, strcmp(data, "key1"));
    ckd_free(data);
    data = s3file_header_value(s, 0);
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
    TEST_EQUAL(0, s3file_parse_header(s));
    ckd_free(data);
    return 0;
}

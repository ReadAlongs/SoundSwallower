/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <soundswallower/ckd_alloc.h>
#include <soundswallower/strfuncs.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    if (argc < 2)
        return 1;

    if (!strcmp(argv[1], "string_join")) {
        char *foo = string_join("bar", "baz", "quux", NULL);
        if (strcmp(foo, "barbazquux") != 0) {
            printf("%s != barbazquux\n", foo);
            return 1;
        }
        foo = string_join("hello", NULL);
        if (strcmp(foo, "hello") != 0) {
            printf("%s != hello\n", foo);
            return 1;
        }
        return 0;
    } else if (!strcmp(argv[1], "string_trim")) {
        char *foo = ckd_salloc("\t foo bar baz  \n");
        string_trim(foo, STRING_BOTH);
        if (strcmp(foo, "foo bar baz") != 0) {
            printf("'%s' != 'foo bar baz'\n", foo);
            return 1;
        }
        string_trim(foo, STRING_BOTH);
        if (strcmp(foo, "foo bar baz") != 0) {
            printf("'%s' != 'foo bar baz'\n", foo);
            return 1;
        }
        strcpy(foo, "foo\nbar\n\n");
        string_trim(foo, STRING_END);
        if (strcmp(foo, "foo\nbar") != 0) {
            printf("'%s' != 'foo\\nbar'\n", foo);
            return 1;
        }
        strcpy(foo, " \t \t foobar\n");
        string_trim(foo, STRING_START);
        if (strcmp(foo, "foobar\n") != 0) {
            printf("'%s' != 'foobar\\n'\n", foo);
            return 1;
        }
    }
    return 0;
}

/* -*- c-basic-offset: 4 -*- */
#include <soundswallower/err.h>

int
main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    err_set_loglevel(ERR_INFO);
    err_msg(ERR_INFO, "blah", 1, "hello world %d %d\n", 2, 3);
    return 0;
}

/* -*- c-basic-offset: 4 -*- */
#include <soundswallower/decoder.h>
#include <soundswallower/err.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    config_t *config;
    decoder_t *ps;

    (void)argc;
    (void)argv;
    err_set_loglevel(ERR_INFO);
    err_msg(ERR_INFO, "blah", 1, "hello world %d %d\n", 2, 3);
    config = config_init(NULL);
    config_set_str(config, "hmm", MODELDIR "/en-us");
    config_set_str(config, "fsg", TESTDATADIR "/goforward.fsg");
    config_set_str(config, "dict", TESTDATADIR "/turtle.dic");
    config_set_str(config, "bestpath", "no");
    config_set_str(config, "logfn", "test.log");
    config_set_str(config, "loglevel", "INFO");
    config_set_str(config, "samprate", "16000");
    TEST_ASSERT(ps = decoder_init(config));
    decoder_set_logfile(ps, NULL);
    E_INFO("HELLO\n");
    decoder_set_logfile(ps, "test2.log");
    E_INFO("HELLO\n");
    decoder_free(ps);

    return 0;
}

/* -*- c-basic-offset: 4 -*- */
#include <soundswallower/err.h>
#include <soundswallower/pocketsphinx.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    config_t *config;
    ps_decoder_t *ps;

    (void)argc; (void)argv;
    err_set_loglevel(ERR_INFO);
    err_msg(ERR_INFO, "blah", 1, "hello world %d %d\n", 2, 3);
    config = cmd_ln_init(NULL, ps_args(), TRUE,
			 "-hmm", MODELDIR "/en-us",
			 "-fsg", TESTDATADIR "/goforward.fsg",
			 "-dict", TESTDATADIR "/turtle.dic",
			 "-bestpath", "no",
			 "-logfn", "test.log",
			 "-loglevel", "INFO",
			 "-samprate", "16000", NULL);
    ps = ps_init(config);
    config_free(config);
    ps_set_logfile(ps, NULL);
    E_INFO("HELLO\n");
    ps_set_logfile(ps, "test2.log");
    E_INFO("HELLO\n");
    ps_free(ps);
    
    return 0;
}

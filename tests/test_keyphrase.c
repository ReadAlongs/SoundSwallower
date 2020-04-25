#include "config.h"

#include <stdio.h>
#include <string.h>

#include <soundswallower/pocketsphinx.h>
#include <soundswallower/pocketsphinx_internal.h>

#include "test_macros.h"
#include "test_ps.c"

int
main(int argc, char *argv[])
{
    cmd_ln_t *config;

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/en-us",
                "-kws", TESTDATADIR "/goforward.kws",
                "-dict", MODELDIR "/en-us.dict", NULL));
    return ps_decoder_test(config, "KEYPHRASE", "forward");
}

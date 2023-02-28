#include "config.h"

#include <stdio.h>
#include <string.h>

#include <soundswallower/bin_mdef.h>
#include <soundswallower/decoder.h>
#include <soundswallower/dict.h>
#include <soundswallower/dict2pid.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    bin_mdef_t *mdef;
    dict_t *dict;
    dict2pid_t *d2p;
    config_t *config;

    (void)argc;
    (void)argv;
    TEST_ASSERT(config = config_init(NULL));
    config_set_str(config, "dict", MODELDIR "/en-us/dict.txt");
    config_set_str(config, "fdict", MODELDIR "/en-us/noisedict.txt");
    TEST_ASSERT(mdef = bin_mdef_read(NULL, MODELDIR "/en-us/mdef"));
    TEST_ASSERT(dict = dict_init(config, mdef));
    TEST_ASSERT(d2p = dict2pid_build(mdef, dict));

    dict_free(dict);
    dict2pid_free(d2p);
    bin_mdef_free(mdef);
    config_free(config);

    return 0;
}

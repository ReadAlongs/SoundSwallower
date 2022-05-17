#include "config.h"

#include <stdio.h>
#include <string.h>

#include <soundswallower/pocketsphinx.h>
#include <soundswallower/bin_mdef.h>
#include <soundswallower/dict.h>
#include <soundswallower/dict2pid.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	bin_mdef_t *mdef;
	dict_t *dict;
	dict2pid_t *d2p;
	cmd_ln_t *config;

	TEST_ASSERT(config = cmd_ln_init(NULL, NULL, FALSE,
						   "-dict", MODELDIR "/en-us/dict.txt",
						   "_fdict", MODELDIR "/en-us/noisedict",
						   NULL));
	TEST_ASSERT(mdef = bin_mdef_read(NULL, MODELDIR "/en-us/mdef"));
	TEST_ASSERT(dict = dict_init(config, mdef));
	TEST_ASSERT(d2p = dict2pid_build(mdef, dict));

	dict_free(dict);
	dict2pid_free(d2p);
	bin_mdef_free(mdef);
	cmd_ln_free_r(config);

	return 0;
}

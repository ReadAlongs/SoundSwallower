#include "config.h"

#include <stdio.h>
#include <string.h>

#include <soundswallower/bin_mdef.h>
#include <soundswallower/decoder.h>
#include <soundswallower/dict.h>
#include <soundswallower/strfuncs.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    bin_mdef_t *mdef;
    dict_t *dict;
    config_t *config;

    int i;
    char buf[100];

    (void)argc;
    (void)argv;
    TEST_ASSERT(config = config_init(NULL));
    config_set_str(config, "dict", MODELDIR "/en-us/dict.txt");
    config_set_str(config, "fdict", MODELDIR "/en-us/noisedict.txt");
    TEST_ASSERT(mdef = bin_mdef_read(NULL, MODELDIR "/en-us/mdef"));
    TEST_ASSERT(dict = dict_init(config, mdef));
    /* Test dictionary in standard fashion. */

    printf("Word ID (CARNEGIE) = %d\n",
           dict_wordid(dict, "CARNEGIE"));
    printf("Word ID (ASDFASFASSD) = %d\n",
           dict_wordid(dict, "ASDFASFASSD"));
    bin_mdef_free(mdef);
    dict_free(dict);

    /* Now test an empty dictionary. */
    TEST_ASSERT(dict = dict_init(NULL, NULL));
    printf("Word ID(<s>) = %d\n", dict_wordid(dict, "<s>"));
    TEST_ASSERT(BAD_S3WID != dict_add_word(dict, "FOOBIE", NULL, 0));
    TEST_ASSERT(BAD_S3WID != dict_add_word(dict, "BLETCH", NULL, 0));
    printf("Word ID(FOOBIE) = %d\n", dict_wordid(dict, "FOOBIE"));
    printf("Word ID(BLETCH) = %d\n", dict_wordid(dict, "BLETCH"));
    TEST_ASSERT(dict_real_word(dict, dict_wordid(dict, "FOOBIE")));
    TEST_ASSERT(dict_real_word(dict, dict_wordid(dict, "BLETCH")));
    TEST_ASSERT(!dict_real_word(dict, dict_wordid(dict, "</s>")));
    dict_free(dict);

    /* Test to add 500k words. */
    TEST_ASSERT(dict = dict_init(NULL, NULL));
    for (i = 0; i < 5000; i++) {
        sprintf(buf, "word_%d", i);
        TEST_ASSERT(BAD_S3WID != dict_add_word(dict, buf, NULL, 0));
    }
    dict_free(dict);

    config_free(config);

    return 0;
}

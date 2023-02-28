#include "config.h"

#include <stdio.h>
#include <string.h>

#include <soundswallower/bin_mdef.h>
#include <soundswallower/decoder.h>
#include <soundswallower/err.h>

#include "test_macros.h"

/* Test context-dependent phone lookup. */
void
test_cd_mdef(bin_mdef_t *mdef)
{
    /* Ask for CI phone, get CI phone. */
    TEST_EQUAL(5, bin_mdef_phone_id(mdef, bin_mdef_ciphone_id(mdef, "AO"), -1, -1, WORD_POSN_UNDEFINED));
    /* Ask for something impossible, get -1. */
    TEST_EQUAL(-1, bin_mdef_phone_id(mdef, bin_mdef_ciphone_id(mdef, "ZH"), bin_mdef_ciphone_id(mdef, "ZH"), bin_mdef_ciphone_id(mdef, "ZH"), WORD_POSN_SINGLE));
    /* Ask for something possible, get it. */
    TEST_EQUAL(42, bin_mdef_phone_id(mdef, bin_mdef_ciphone_id(mdef, "AA"), bin_mdef_ciphone_id(mdef, "AA"), bin_mdef_ciphone_id(mdef, "AA"), WORD_POSN_SINGLE));
    TEST_EQUAL(121854, bin_mdef_phone_id(mdef, bin_mdef_ciphone_id(mdef, "UW"), bin_mdef_ciphone_id(mdef, "F"), bin_mdef_ciphone_id(mdef, "B"), WORD_POSN_END));
    TEST_EQUAL(137094, bin_mdef_phone_id(mdef, bin_mdef_ciphone_id(mdef, "ZH"), bin_mdef_ciphone_id(mdef, "ZH"), bin_mdef_ciphone_id(mdef, "W"), WORD_POSN_BEGIN));
    /* Back off to CI phone (should do something smarter, but really, no). */
    TEST_EQUAL(41,
               bin_mdef_phone_id_nearest(mdef,
                                         bin_mdef_ciphone_id(mdef, "ZH"),
                                         bin_mdef_ciphone_id(mdef, "ZH"),
                                         bin_mdef_ciphone_id(mdef, "ZH"),
                                         WORD_POSN_SINGLE));
    /* Back off to silence context. */
    TEST_EQUAL(137005,
               bin_mdef_phone_id_nearest(mdef,
                                         bin_mdef_ciphone_id(mdef, "ZH"),
                                         bin_mdef_ciphone_id(mdef, "UW"),
                                         bin_mdef_ciphone_id(mdef, "+NSN+"),
                                         WORD_POSN_END));
}

/* Test context-independent phone lookup. */
void
test_ci_mdef(bin_mdef_t *mdef)
{
    TEST_EQUAL(2, bin_mdef_ciphone_id(mdef, "AA"));
    TEST_EQUAL(5, bin_mdef_ciphone_id(mdef, "AO"));
    TEST_EQUAL(41, bin_mdef_ciphone_id(mdef, "ZH"));
    TEST_EQUAL(2, bin_mdef_ciphone_id_nocase(mdef, "Aa"));
    TEST_EQUAL(41, bin_mdef_ciphone_id_nocase(mdef, "zH"));
    TEST_EQUAL(5, bin_mdef_ciphone_id_nocase(mdef, "ao"));
    TEST_EQUAL(41, bin_mdef_ciphone_id_nocase(mdef, "zh"));
    TEST_EQUAL(41, bin_mdef_ciphone_id_nocase(mdef, "ZH"));
    TEST_EQUAL(0, strcmp("ZH", bin_mdef_ciphone_str(mdef, bin_mdef_ciphone_id(mdef, "ZH"))));
    TEST_EQUAL(0, strcmp("AO", bin_mdef_ciphone_str(mdef, bin_mdef_ciphone_id(mdef, "AO"))));

    TEST_EQUAL(5, bin_mdef_phone_id(mdef, bin_mdef_ciphone_id(mdef, "AO"), -1, -1, WORD_POSN_UNDEFINED));
}

/* Test forced context-independent phone lookup. */
void
test_cionly_mdef(bin_mdef_t *mdef)
{
    /* Ask for CI phone, get CI phone. */
    TEST_EQUAL(5, bin_mdef_phone_id(mdef, bin_mdef_ciphone_id(mdef, "AO"), -1, -1, WORD_POSN_UNDEFINED));
    /* Ask for something impossible, get -1. */
    TEST_EQUAL(-1, bin_mdef_phone_id(mdef, bin_mdef_ciphone_id(mdef, "ZH"), bin_mdef_ciphone_id(mdef, "ZH"), bin_mdef_ciphone_id(mdef, "ZH"), WORD_POSN_SINGLE));
    /* Exact match is always impossible with no CD phones. */
    TEST_EQUAL(-1,
               bin_mdef_phone_id(mdef,
                                 bin_mdef_ciphone_id(mdef, "AA"),
                                 bin_mdef_ciphone_id(mdef, "AA"),
                                 bin_mdef_ciphone_id(mdef, "AA"),
                                 WORD_POSN_SINGLE));
    TEST_EQUAL(-1,
               bin_mdef_phone_id(mdef,
                                 bin_mdef_ciphone_id(mdef, "UW"),
                                 bin_mdef_ciphone_id(mdef, "F"),
                                 bin_mdef_ciphone_id(mdef, "B"),
                                 WORD_POSN_END));
    TEST_EQUAL(-1,
               bin_mdef_phone_id(mdef,
                                 bin_mdef_ciphone_id(mdef, "ZH"),
                                 bin_mdef_ciphone_id(mdef, "ZH"),
                                 bin_mdef_ciphone_id(mdef, "W"),
                                 WORD_POSN_BEGIN));
    /* Nearest match always gives CI phone */
    TEST_EQUAL(bin_mdef_ciphone_id(mdef, "AA"),
               bin_mdef_phone_id_nearest(mdef,
                                         bin_mdef_ciphone_id(mdef, "AA"),
                                         bin_mdef_ciphone_id(mdef, "AA"),
                                         bin_mdef_ciphone_id(mdef, "AA"),
                                         WORD_POSN_SINGLE));
    TEST_EQUAL(bin_mdef_ciphone_id(mdef, "UW"),
               bin_mdef_phone_id_nearest(mdef,
                                         bin_mdef_ciphone_id(mdef, "UW"),
                                         bin_mdef_ciphone_id(mdef, "F"),
                                         bin_mdef_ciphone_id(mdef, "B"),
                                         WORD_POSN_END));
    TEST_EQUAL(bin_mdef_ciphone_id(mdef, "ZH"),
               bin_mdef_phone_id_nearest(mdef,
                                         bin_mdef_ciphone_id(mdef, "ZH"),
                                         bin_mdef_ciphone_id(mdef, "ZH"),
                                         bin_mdef_ciphone_id(mdef, "W"),
                                         WORD_POSN_BEGIN));
    TEST_EQUAL(bin_mdef_ciphone_id(mdef, "ZH"),
               bin_mdef_phone_id_nearest(mdef,
                                         bin_mdef_ciphone_id(mdef, "ZH"),
                                         bin_mdef_ciphone_id(mdef, "ZH"),
                                         bin_mdef_ciphone_id(mdef, "ZH"),
                                         WORD_POSN_SINGLE));
    TEST_EQUAL(bin_mdef_ciphone_id(mdef, "ZH"),
               bin_mdef_phone_id_nearest(mdef,
                                         bin_mdef_ciphone_id(mdef, "ZH"),
                                         bin_mdef_ciphone_id(mdef, "UW"),
                                         bin_mdef_ciphone_id(mdef, "+NSN+"),
                                         WORD_POSN_END));
}

int
main(int argc, char *argv[])
{
    config_t *config;
    bin_mdef_t *mdef;

    (void)argc;
    (void)argv;
    err_set_loglevel(ERR_INFO);
    config = config_init(NULL);

    E_INFO("Testing text mdef read\n");
    mdef = bin_mdef_read(config, MODELDIR "/en-us/mdef.txt");
    TEST_ASSERT(mdef != NULL);
    test_ci_mdef(mdef);
    test_cd_mdef(mdef);
    bin_mdef_free(mdef);

    E_INFO("Testing text mdef -cionly read\n");
    config_set_bool(config, "cionly", TRUE);
    mdef = bin_mdef_read(config, MODELDIR "/en-us/mdef.txt");
    TEST_ASSERT(mdef != NULL);
    test_ci_mdef(mdef);
    test_cionly_mdef(mdef);
    bin_mdef_free(mdef);

    E_INFO("Testing binary mdef read\n");
    config_set_bool(config, "cionly", FALSE);
    mdef = bin_mdef_read(config, MODELDIR "/en-us/mdef");
    TEST_ASSERT(mdef != NULL);
    test_ci_mdef(mdef);
    test_cd_mdef(mdef);
    bin_mdef_free(mdef);

    E_INFO("Testing binary mdef -cionly read\n");
    config_set_bool(config, "cionly", TRUE);
    mdef = bin_mdef_read(config, MODELDIR "/en-us/mdef");
    TEST_ASSERT(mdef != NULL);
    test_ci_mdef(mdef);
    test_cionly_mdef(mdef);
    bin_mdef_free(mdef);

    config_free(config);
    return 0;
}

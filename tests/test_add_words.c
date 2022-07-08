/* -*- c-basic-offset: 4 -*- */
#include "config.h"

#include <soundswallower/pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    fsg_model_t *fsg;
    cmd_ln_t *config;
    const char *hyp;
    char *phones;
    int32 score, prob;
    FILE *rawfh;
    int16 buf[2048];
    size_t nread;

    (void)argc; (void)argv;
    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
			"-hmm", MODELDIR "/en-us",
			"-dict", TESTDATADIR "/turtle.dic",
			"-input_endian", "little", /* raw data demands it */
			"-bestpath", "no",
			"-loglevel", "INFO",
			"-samprate", "16000", NULL));
    TEST_ASSERT(ps = ps_init(config));
    TEST_ASSERT(ps_add_word(ps, "_forward", "F AO R W ER D", FALSE) != -1);
    TEST_ASSERT(ps_add_word(ps, "_backward", "B AE K W ER D", FALSE) != -1);
    phones = ps_lookup_word(ps, "_forward");
    TEST_ASSERT(0 == strcmp(phones, "F AO R W ER D"));
    ckd_free(phones);
    phones = ps_lookup_word(ps, "_backward");
    TEST_ASSERT(0 == strcmp(phones, "B AE K W ER D"));
    ckd_free(phones);
    fsg = fsg_model_readfile(TESTDATADIR "/goforward3.fsg", ps_get_logmath(ps), 1.0);
    ps_set_fsg(ps, "_default", fsg);
    fsg_model_free(fsg);
    TEST_ASSERT(rawfh = fopen(TESTDATADIR "/goforward.raw", "rb"));
    ps_start_utt(ps);
    while (!feof(rawfh)) {
	nread = fread(buf, sizeof(*buf), sizeof(buf)/sizeof(*buf), rawfh);
        ps_process_raw(ps, buf, nread, FALSE, FALSE);
    }
    fclose(rawfh);
    ps_end_utt(ps);
    hyp = ps_get_hyp(ps, &score);
    prob = ps_get_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_EQUAL(0, strcmp("go _forward two meters", hyp));
    ps_free(ps);
    cmd_ln_free_r(config);

    return 0;
}

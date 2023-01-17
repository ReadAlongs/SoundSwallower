/* -*- c-basic-offset: 4 -*- */
#include "config.h"

#include <soundswallower/decoder.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    decoder_t *ps;
    fsg_model_t *fsg;
    config_t *config;
    const char *hyp;
    char *phones;
    int32 score, prob;
    FILE *rawfh;
    int16 buf[2048];
    size_t nread;

    (void)argc;
    (void)argv;
    TEST_ASSERT(config = config_init(NULL));
    config_set_str(config, "hmm", MODELDIR "/en-us");
    config_set_str(config, "dict", TESTDATADIR "/turtle.dic");
    config_set_str(config, "input_endian", "little"); /* raw data demands it */
    config_set_str(config, "bestpath", "no");
    config_set_str(config, "loglevel", "INFO");
    config_set_str(config, "samprate", "16000");
    config_expand(config);
    TEST_ASSERT(ps = decoder_init(config));
    TEST_ASSERT(decoder_add_word(ps, "_forward", "F AO R W ER D", FALSE) != -1);
    TEST_ASSERT(decoder_add_word(ps, "_backward", "B AE K W ER D", FALSE) != -1);
    phones = decoder_lookup_word(ps, "_forward");
    TEST_ASSERT(0 == strcmp(phones, "F AO R W ER D"));
    ckd_free(phones);
    phones = decoder_lookup_word(ps, "_backward");
    TEST_ASSERT(0 == strcmp(phones, "B AE K W ER D"));
    ckd_free(phones);
    fsg = fsg_model_readfile(TESTDATADIR "/goforward3.fsg", decoder_logmath(ps), 1.0);
    decoder_set_fsg(ps, fsg);
    TEST_ASSERT(rawfh = fopen(TESTDATADIR "/goforward.raw", "rb"));
    decoder_start_utt(ps);
    while (!feof(rawfh)) {
        nread = fread(buf, sizeof(*buf), sizeof(buf) / sizeof(*buf), rawfh);
        decoder_process_int16(ps, buf, nread, FALSE, FALSE);
    }
    fclose(rawfh);
    decoder_end_utt(ps);
    hyp = decoder_hyp(ps, &score);
    prob = decoder_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_ASSERT(hyp);
    TEST_EQUAL(0, strcmp("go _forward two meters", hyp));
    decoder_free(ps);

    return 0;
}

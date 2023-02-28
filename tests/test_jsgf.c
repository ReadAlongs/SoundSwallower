/* -*- c-basic-offset: 4 -*- */
#include "config.h"

#include <soundswallower/decoder.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <soundswallower/fsg_model.h>
#include <soundswallower/jsgf.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    decoder_t *ps;
    config_t *config;
    jsgf_t *jsgf;
    jsgf_rule_t *rule;
    fsg_model_t *fsg;
    FILE *rawfh;
    char const *hyp;
    int32 score, prob;
    int16 buf[2048];
    size_t nread;

    (void)argc;
    (void)argv;
    TEST_ASSERT(config = config_init(NULL));
    config_set_str(config, "hmm", MODELDIR "/en-us");
    config_set_str(config, "dict", TESTDATADIR "/turtle.dic");
    config_set_str(config, "input_endian", "little");
    config_set_str(config, "samprate", "16000");
    TEST_ASSERT(ps = decoder_init(config));

    jsgf = jsgf_parse_file(TESTDATADIR "/goforward.gram", NULL);
    TEST_ASSERT(jsgf);
    rule = jsgf_get_rule(jsgf, "goforward.move2");
    TEST_ASSERT(rule);
    fsg = jsgf_build_fsg(jsgf, rule, ps->lmath, 7.5);
    TEST_ASSERT(fsg);
    fsg_model_write(fsg, stdout);
    decoder_set_fsg(ps, fsg);
    TEST_ASSERT(rawfh = fopen(TESTDATADIR "/goforward.raw", "rb"));
    decoder_start_utt(ps);
    while (!feof(rawfh)) {
        nread = fread(buf, sizeof(*buf), sizeof(buf) / sizeof(*buf), rawfh);
        decoder_process_int16(ps, buf, nread, FALSE, FALSE);
    }
    decoder_end_utt(ps);
    hyp = decoder_hyp(ps, &score);
    prob = decoder_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_ASSERT(hyp);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));
    decoder_free(ps);
    fclose(rawfh);

    TEST_ASSERT(config = config_init(NULL));
    config_set_str(config, "hmm", MODELDIR "/en-us");
    config_set_str(config, "dict", TESTDATADIR "/turtle.dic");
    config_set_str(config, "jsgf", TESTDATADIR "/goforward.gram");
    config_set_str(config, "input_endian", "little");
    config_set_str(config, "samprate", "16000");
    TEST_ASSERT(ps = decoder_init(config));
    TEST_ASSERT(rawfh = fopen(TESTDATADIR "/goforward.raw", "rb"));
    decoder_start_utt(ps);
    while (!feof(rawfh)) {
        nread = fread(buf, sizeof(*buf), sizeof(buf) / sizeof(*buf), rawfh);
        decoder_process_int16(ps, buf, nread, FALSE, FALSE);
    }
    decoder_end_utt(ps);
    hyp = decoder_hyp(ps, &score);
    prob = decoder_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_ASSERT(hyp);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));
    decoder_free(ps);
    fclose(rawfh);

    TEST_ASSERT(config = config_init(NULL));
    config_set_str(config, "hmm", MODELDIR "/en-us");
    config_set_str(config, "dict", TESTDATADIR "/turtle.dic");
    config_set_str(config, "jsgf", TESTDATADIR "/goforward.gram");
    config_set_str(config, "toprule", "goforward.move2");
    config_set_str(config, "input_endian", "little");
    config_set_str(config, "samprate", "16000");
    TEST_ASSERT(ps = decoder_init(config));
    TEST_ASSERT(rawfh = fopen(TESTDATADIR "/goforward.raw", "rb"));
    decoder_start_utt(ps);
    while (!feof(rawfh)) {
        nread = fread(buf, sizeof(*buf), sizeof(buf) / sizeof(*buf), rawfh);
        decoder_process_int16(ps, buf, nread, FALSE, FALSE);
    }
    decoder_end_utt(ps);
    hyp = decoder_hyp(ps, &score);
    prob = decoder_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_ASSERT(hyp);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));
    decoder_free(ps);
    fclose(rawfh);

    TEST_ASSERT(config = config_init(NULL));
    config_set_str(config, "hmm", MODELDIR "/en-us");
    config_set_str(config, "dict", TESTDATADIR "/turtle.dic");
    config_set_str(config, "jsgf", TESTDATADIR "/defective.gram");
    TEST_ASSERT(NULL == decoder_init(config));

    return 0;
}

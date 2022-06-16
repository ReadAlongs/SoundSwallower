/* -*- c-basic-offset: 4 -*- */
#include "config.h"

#include <soundswallower/pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <soundswallower/jsgf.h>
#include <soundswallower/fsg_model.h>
#include <soundswallower/pocketsphinx_internal.h>
#include <soundswallower/fsg_search_internal.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    cmd_ln_t *config;
    jsgf_t *jsgf;
    jsgf_rule_t *rule;
    fsg_model_t *fsg;
    FILE *rawfh;
    char const *hyp;
    int32 score, prob;
    int16 buf[2048];
    size_t nread;

    (void)argc; (void)argv;
    TEST_ASSERT(config =
		cmd_ln_init(NULL, ps_args(), TRUE,
			    "-hmm", MODELDIR "/en-us",
			    "-dict", TESTDATADIR "/turtle.dic",
			    "-samprate", "16000", NULL));
    TEST_ASSERT(ps = ps_init(config));

    jsgf = jsgf_parse_file(TESTDATADIR "/goforward.gram", NULL);
    TEST_ASSERT(jsgf);
    rule = jsgf_get_rule(jsgf, "goforward.move2");
    TEST_ASSERT(rule);
    fsg = jsgf_build_fsg(jsgf, rule, ps->lmath, 7.5);
    TEST_ASSERT(fsg);
    fsg_model_write(fsg, stdout);
    ps_set_fsg(ps, "goforward.move2", fsg);
    TEST_ASSERT(rawfh = fopen(TESTDATADIR "/goforward.raw", "rb"));
    ps_start_utt(ps);
    while (!feof(rawfh)) {
	nread = fread(buf, sizeof(*buf), sizeof(buf)/sizeof(*buf), rawfh);
        ps_process_raw(ps, buf, nread, FALSE, FALSE);
    }
    ps_end_utt(ps);
    hyp = ps_get_hyp(ps, &score);
    prob = ps_get_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));
    ps_free(ps);
    fclose(rawfh);
    cmd_ln_free_r(config);


    TEST_ASSERT(config =
		cmd_ln_init(NULL, ps_args(), TRUE,
			    "-hmm", MODELDIR "/en-us",
			    "-dict", TESTDATADIR "/turtle.dic",
			    "-jsgf", TESTDATADIR "/goforward.gram",
			    "-samprate", "16000", NULL));
    TEST_ASSERT(ps = ps_init(config));
    TEST_ASSERT(rawfh = fopen(TESTDATADIR "/goforward.raw", "rb"));
    ps_start_utt(ps);
    while (!feof(rawfh)) {
	nread = fread(buf, sizeof(*buf), sizeof(buf)/sizeof(*buf), rawfh);
        ps_process_raw(ps, buf, nread, FALSE, FALSE);
    }
    ps_end_utt(ps);
    hyp = ps_get_hyp(ps, &score);
    prob = ps_get_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));
    ps_free(ps);
    fclose(rawfh);
    cmd_ln_free_r(config);

    TEST_ASSERT(config =
		cmd_ln_init(NULL, ps_args(), TRUE,
			    "-hmm", MODELDIR "/en-us",
			    "-dict", TESTDATADIR "/turtle.dic",
			    "-jsgf", TESTDATADIR "/goforward.gram",
			    "-toprule", "goforward.move2",
			    "-samprate", "16000", NULL));
    TEST_ASSERT(ps = ps_init(config));
    TEST_ASSERT(rawfh = fopen(TESTDATADIR "/goforward.raw", "rb"));
    ps_start_utt(ps);
    while (!feof(rawfh)) {
	nread = fread(buf, sizeof(*buf), sizeof(buf)/sizeof(*buf), rawfh);
        ps_process_raw(ps, buf, nread, FALSE, FALSE);
    }
    ps_end_utt(ps);
    hyp = ps_get_hyp(ps, &score);
    prob = ps_get_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));
    ps_free(ps);
    cmd_ln_free_r(config);
    fclose(rawfh);

    TEST_ASSERT(config =
		cmd_ln_init(NULL, ps_args(), TRUE,
			    "-hmm", MODELDIR "/en-us",
			    "-dict", TESTDATADIR "/turtle.dic",
			    "-jsgf", TESTDATADIR "/defective.gram",
			    NULL));
    TEST_ASSERT(NULL == ps_init(config));
    cmd_ln_free_r(config);

    return 0;
}

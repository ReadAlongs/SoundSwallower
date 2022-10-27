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
    config_t *config;
    lattice_t *dag;
    const char *hyp;
    seg_iter_t *seg;
    int32 score, prob;
    FILE *rawfh;
    int16 buf[2048];
    size_t nread;

    (void)argc; (void)argv;
    TEST_ASSERT(config = config_init(NULL));
    config_set_str(config, "hmm", MODELDIR "/en-us");
    config_set_str(config, "fsg", TESTDATADIR "/goforward.fsg");
    config_set_str(config, "dict", TESTDATADIR "/turtle.dic");
    config_set_str(config, "input_endian", "little");
    config_set_str(config, "samprate", "16000");
    TEST_ASSERT(ps = decoder_init(config));

    TEST_ASSERT(rawfh = fopen(TESTDATADIR "/goforward.raw", "rb"));
    decoder_start_utt(ps);
    while (!feof(rawfh)) {
	nread = fread(buf, sizeof(*buf), sizeof(buf)/sizeof(*buf), rawfh);
        decoder_process_int16(ps, buf, nread, FALSE, FALSE);
    }
    fclose(rawfh);
    decoder_end_utt(ps);
    hyp = decoder_hyp(ps, &score);
    prob = decoder_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));


    for (seg = decoder_seg_iter(ps); seg;
         seg = seg_iter_next(seg)) {
        char const *word;
        int sf, ef;
        int32 post, lscr, ascr;

        word = seg_iter_word(seg);
        seg_iter_frames(seg, &sf, &ef);
        if (sf == ef)
            continue;
        post = seg_iter_prob(seg, &ascr, &lscr);
        printf("%s (%d:%d) P(w|o) = %f ascr = %d lscr = %d\n", word, sf, ef,
               logmath_exp(decoder_logmath(ps), post), ascr, lscr);
    }

    /* Now get the DAG and play with it. */
    dag = decoder_lattice(ps);
    printf("BESTPATH: %s\n",
           lattice_hyp(dag, lattice_bestpath(dag, 15.0)));
    lattice_posterior(dag, 15.0);
    decoder_free(ps);

    return 0;
}

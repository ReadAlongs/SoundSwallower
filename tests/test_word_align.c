/* -*- c-basic-offset: 4 -*- */
#include <soundswallower.h>

#include "test_macros.h"

#define AUSTEN_TEXT "he was not an ill disposed young man"
static int
do_decode(decoder_t *ps)
{
    FILE *rawfh;
    short *data;
    long nsamp;
    int nfr;

    TEST_ASSERT(rawfh = fopen(TESTDATADIR "/sense_and_sensibility_01_austen_64kb-0880.wav", "rb"));
    fseek(rawfh, 0, SEEK_END);
    nsamp = (ftell(rawfh) - 44) / 2;
    fseek(rawfh, 44, SEEK_SET);
    data = ckd_malloc(nsamp * 2);
    TEST_EQUAL(fread(data, 2, nsamp, rawfh), (size_t)nsamp);
    TEST_EQUAL(0, decoder_start_utt(ps));
    nfr = decoder_process_int16(ps, data, nsamp, FALSE, TRUE);
    TEST_ASSERT(nfr > 0);
    TEST_EQUAL(0, decoder_end_utt(ps));
    fclose(rawfh);
    ckd_free(data);

    return 0;
}

int
main(int argc, char *argv[])
{
    decoder_t *ps;
    alignment_t *al;
    alignment_iter_t *itor;
    seg_iter_t *seg;
    config_t *config;
    int i, sf, ef, last_ef;
    int *sfs, *efs;

    (void)argc;
    (void)argv;
    err_set_loglevel(ERR_INFO);
    config = config_init(NULL);
    config_set_str(config, "loglevel", "INFO");
    config_set_str(config, "hmm", MODELDIR "/en-us");
    config_set_int(config, "samprate", 8000);
    TEST_ASSERT(ps = decoder_init(config));

    /* First test just word alignment */
    TEST_EQUAL(0, decoder_set_align_text(ps, AUSTEN_TEXT));
    do_decode(ps);
    TEST_EQUAL(0, strcmp(decoder_hyp(ps, &i), AUSTEN_TEXT));
    printf("Word alignment:\n");
    i = 0;
    last_ef = -1;
    for (seg = decoder_seg_iter(ps); seg; seg = seg_iter_next(seg)) {
        seg_iter_frames(seg, &sf, &ef);
        printf("%s %d %d\n", seg_iter_word(seg), sf, ef);
        TEST_ASSERT(sf == last_ef + 1);
        TEST_ASSERT(ef > sf);
        last_ef = ef;
        i++;
    }
    TEST_EQUAL(NULL, seg);

    /* Save start and end points for comparison */
    sfs = ckd_calloc(i, sizeof(*sfs));
    efs = ckd_calloc(i, sizeof(*efs));
    i = 0;
    for (seg = decoder_seg_iter(ps); seg; seg = seg_iter_next(seg)) {
        seg_iter_frames(seg, &sfs[i], &efs[i]);
        i++;
    }

    /* Test two-pass alignment.  Ensure that alignment and seg give
     * the same results and that phones have constraints propagated to
     * them. */
    TEST_EQUAL(0, decoder_set_align_text(ps, AUSTEN_TEXT));
    do_decode(ps);
    TEST_EQUAL(0, strcmp(decoder_hyp(ps, &i), AUSTEN_TEXT));
    TEST_ASSERT(al = decoder_alignment(ps));
    /* Make sure that we reuse the existing alignment if nothing changes. */
    TEST_EQUAL(al, decoder_alignment(ps));
    printf("Subword alignment:\n");
    /* It should have durations assigned (and properly constrained). */
    for (i = 0, seg = decoder_seg_iter(ps), itor = alignment_words(al); itor;
         i++, seg = seg_iter_next(seg), itor = alignment_iter_next(itor)) {
        int score, start, duration;
        alignment_iter_t *pitor;

        seg_iter_frames(seg, &sf, &ef);
        TEST_ASSERT(seg);
        score = alignment_iter_seg(itor, &start, &duration);
        printf("%s %d %d %d %d %s %d %d %d\n", seg_iter_word(seg),
               sfs[i], efs[i], sf, ef,
               alignment_iter_name(itor), start, duration, score);
        TEST_EQUAL(sf, sfs[i]);
        TEST_EQUAL(ef, efs[i]);
        TEST_ASSERT(score != 0);
        TEST_EQUAL(start, sf);
        TEST_EQUAL(duration, ef - sf + 1);

        /* Phone segmentations should be constrained by words */
        pitor = alignment_iter_children(itor);
        score = alignment_iter_seg(pitor, &start, &duration);
        /* First phone should be aligned with word */
        TEST_EQUAL(start, sf);
        while (pitor) {
            alignment_iter_t *sitor;
            int state_start, state_duration;
            score = alignment_iter_seg(pitor, &start, &duration);
            printf("%s %d %d %s %d %d %d\n", seg_iter_word(seg), sf, ef,
                   alignment_iter_name(pitor), start, duration, score);
            /* State segmentations should be constrained by phones */
            sitor = alignment_iter_children(pitor);
            score = alignment_iter_seg(sitor, &state_start, &state_duration);
            /* First state should be aligned with phone */
            TEST_EQUAL(state_start, start);
            while (sitor) {
                score = alignment_iter_seg(sitor, &state_start, &state_duration);
                printf("%s %d %d %s %d %d %d\n", seg_iter_word(seg), sf, ef,
                       alignment_iter_name(sitor), state_start, state_duration,
                       score);
                sitor = alignment_iter_next(sitor);
            }
            /* Last state should fill phone duration */
            TEST_EQUAL(state_start + state_duration, start + duration);
            pitor = alignment_iter_next(pitor);
        }
        /* Last phone should fill word duration */
        TEST_EQUAL(start + duration - 1, ef);
    }

    /* Segmentations should all be contiguous */
    last_ef = 0;
    for (itor = alignment_words(al); itor;
         itor = alignment_iter_next(itor)) {
        int start, duration;
        (void)alignment_iter_seg(itor, &start, &duration);
        TEST_EQUAL(start, last_ef);
        last_ef = start + duration;
    }
    last_ef = 0;
    for (itor = alignment_phones(al); itor;
         itor = alignment_iter_next(itor)) {
        int start, duration;
        (void)alignment_iter_seg(itor, &start, &duration);
        TEST_EQUAL(start, last_ef);
        last_ef = start + duration;
    }
    last_ef = 0;
    for (itor = alignment_states(al); itor;
         itor = alignment_iter_next(itor)) {
        int start, duration;
        (void)alignment_iter_seg(itor, &start, &duration);
        TEST_EQUAL(start, last_ef);
        last_ef = start + duration;
    }

    ckd_free(sfs);
    ckd_free(efs);
    decoder_free(ps);

    return 0;
}

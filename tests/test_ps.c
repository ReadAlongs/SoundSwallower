#include <soundswallower/decoder.h>
#include <stdio.h>
#include <string.h>

#include "test_macros.h"

int
decoder_test(config_t *config, char const *sname, char const *expected)
{
    decoder_t *ps;
    mfcc_t **cepbuf;
    FILE *rawfh;
    int16 *buf;
    int16 *bptr;
    size_t nread;
    size_t nsamps;
    int32 nfr, i, score, prob;
    char const *hyp;
    double n_speech, n_cpu, n_wall;
    seg_iter_t *seg;

    TEST_ASSERT(ps = decoder_init(config));
    /* Test it first with pocketsphinx_decode_raw() */
    TEST_ASSERT(rawfh = fopen(TESTDATADIR "/goforward.raw", "rb"));
    decoder_decode_raw(ps, rawfh, -1);
    hyp = decoder_get_hyp(ps, &score);
    prob = decoder_get_prob(ps);
    printf("%s: %s (%d, %d)\n", sname, hyp, score, prob);
    TEST_ASSERT(hyp);
    TEST_EQUAL(0, strcmp(hyp, expected));
    TEST_ASSERT(prob <= 0);
    decoder_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
    printf("%.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    printf("%.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);

    /* Test it with decoder_process_raw() */
    clearerr(rawfh);
    fseek(rawfh, 0, SEEK_END);
    nsamps = ftell(rawfh) / sizeof(*buf);
    fseek(rawfh, 0, SEEK_SET);
    TEST_EQUAL(0, decoder_start_utt(ps));
    nsamps = 2048;
    buf = ckd_calloc(nsamps, sizeof(*buf));
    while (!feof(rawfh)) {
        nread = fread(buf, sizeof(*buf), nsamps, rawfh);
        decoder_process_raw(ps, buf, nread, FALSE, FALSE);
    }
    TEST_EQUAL(0, decoder_end_utt(ps));
    hyp = decoder_get_hyp(ps, &score);
    prob = decoder_get_prob(ps);
    printf("%s: %s (%d, %d)\n", sname, hyp, score, prob);
    TEST_ASSERT(hyp);
    TEST_EQUAL(0, strcmp(hyp, expected));
    decoder_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
    printf("%.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    printf("%.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);

    /* Now read the whole file and produce an MFCC buffer. */
    clearerr(rawfh);
    fseek(rawfh, 0, SEEK_END);
    nsamps = ftell(rawfh) / sizeof(*buf);
    fseek(rawfh, 0, SEEK_SET);
    bptr = buf = ckd_realloc(buf, nsamps * sizeof(*buf));
    TEST_EQUAL(nsamps, fread(buf, sizeof(*buf), nsamps, rawfh));
    fe_process_frames(ps->acmod->fe, &bptr, &nsamps, NULL, &nfr);
    cepbuf = ckd_calloc_2d(nfr + 1,
                           fe_get_output_size(ps->acmod->fe),
                           sizeof(**cepbuf));
    fe_start(ps->acmod->fe);
    fe_process_frames(ps->acmod->fe, &bptr, &nsamps, cepbuf, &nfr);
    fe_end(ps->acmod->fe, cepbuf[nfr], &i);

    /* Decode it with process_cep() */
    TEST_EQUAL(0, decoder_start_utt(ps));
    for (i = 0; i < nfr; ++i) {
        decoder_process_cep(ps, cepbuf + i, 1, FALSE, FALSE);
    }
    TEST_EQUAL(0, decoder_end_utt(ps));
    hyp = decoder_get_hyp(ps, &score);
    prob = decoder_get_prob(ps);
    printf("%s: %s (%d, %d)\n", sname, hyp, score, prob);
    TEST_ASSERT(hyp);
    TEST_EQUAL(0, strcmp(hyp, expected));
    TEST_ASSERT(prob <= 0);
    for (seg = decoder_seg_iter(ps); seg;
         seg = decoder_seg_next(seg)) {
        char const *word;
        int sf, ef;
        int32 post, lscr, ascr, lback;

        word = decoder_seg_word(seg);
        decoder_seg_frames(seg, &sf, &ef);
        post = decoder_seg_prob(seg, &ascr, &lscr, &lback);
        printf("%s (%d:%d) P(w|o) = %f ascr = %d lscr = %d lback = %d post=%d\n", word, sf, ef,
               logmath_exp(decoder_get_logmath(ps), post), ascr, lscr, lback, post);
    }

    decoder_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
    printf("%.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    printf("%.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);
    decoder_get_all_time(ps, &n_speech, &n_cpu, &n_wall);
    printf("TOTAL: %.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    printf("TOTAL: %.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);

    fclose(rawfh);
    decoder_free(ps);
    ckd_free_2d(cepbuf);
    ckd_free(buf);

    return 0;
}

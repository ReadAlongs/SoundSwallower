#include "config.h"

#include <stdio.h>
#include <string.h>

#include <soundswallower/acmod.h>
#include <soundswallower/decoder.h>
#include <soundswallower/err.h>
#include <soundswallower/logmath.h>

#include "test_macros.h"

static const mfcc_t cmninit[13] = {
    FLOAT2MFCC(41.00),
    FLOAT2MFCC(-5.29),
    FLOAT2MFCC(-0.12),
    FLOAT2MFCC(5.09),
    FLOAT2MFCC(2.48),
    FLOAT2MFCC(-4.07),
    FLOAT2MFCC(-1.37),
    FLOAT2MFCC(-1.78),
    FLOAT2MFCC(-5.08),
    FLOAT2MFCC(-2.05),
    FLOAT2MFCC(-6.45),
    FLOAT2MFCC(-1.42),
    FLOAT2MFCC(1.17)
};

#define NUM_BEST_SEN 270

int
main(int argc, char *argv[])
{
    acmod_t *acmod;
    logmath_t *lmath;
    config_t *config;
    FILE *rawfh;
    int16 *buf;
    int16 *bptr;
    mfcc_t **cepbuf, **cptr;
    size_t nread, nsamps;
    fe_t *fe;
    feat_t *fcb;
    int nfr;
    int frame_counter;
    int bestsen1[NUM_BEST_SEN];

    (void)argc;
    (void)argv;
    err_set_loglevel(ERR_INFO);
    lmath = logmath_init(1.0001, 0, 0);
    config = config_parse_json(
        NULL,
        "hmm: \"" MODELDIR "/en-us\","
        "compallsen: true, cmn: live, tmatfloor: 0.0001,"
        "mixwfloor: 0.001, varfloor: 0.0001,"
        "topn: 4, ds: 1, samprate: 0");
    TEST_ASSERT(config);
    config_expand(config);

    TEST_ASSERT(config);
    fe = fe_init(config);

    /* Test automatic sampling rate and fft */
    TEST_EQUAL(fe->sampling_rate, 8000);
    TEST_EQUAL(fe->fft_size, 256);

    fcb = feat_init(config);
    TEST_ASSERT(acmod = acmod_init(config, lmath, fe, fcb));
    cmn_live_set(acmod->fcb->cmn_struct, cmninit);
    TEST_EQUAL(acmod_set_grow(acmod, FALSE), ACMOD_GROW_DEFAULT);

    nsamps = 2048;
    frame_counter = 0;
    buf = ckd_calloc(nsamps, sizeof(*buf));
    TEST_ASSERT(rawfh = fopen(TESTDATADIR "/goforward.raw", "rb"));
    TEST_EQUAL(0, acmod_start_utt(acmod));
    E_INFO("Incremental(2048):\n");
    while (!feof(rawfh)) {
        nread = fread(buf, sizeof(*buf), nsamps, rawfh);
        bptr = buf;
        while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0 || nread > 0) {
            int16 best_score;
            int frame_idx = -1, best_senid;
            while (acmod->n_feat_frame > 0) {
                acmod_score(acmod, &frame_idx);
                acmod_advance(acmod);
                best_score = acmod_best_score(acmod, &best_senid);
                E_INFO("Frame %d best senone %d score %d\n",
                       frame_idx, best_senid, best_score);
                TEST_EQUAL(frame_counter, frame_idx);
                if (frame_counter < NUM_BEST_SEN)
                    bestsen1[frame_counter] = best_score;
                ++frame_counter;
                frame_idx = -1;
            }
        }
    }
    TEST_EQUAL(1, acmod_end_utt(acmod));
    nread = 0;
    {
        int16 best_score;
        int frame_idx = -1, best_senid;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            E_INFO("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            if (frame_counter < NUM_BEST_SEN)
                bestsen1[frame_counter] = best_score;
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    /* Now try to process the whole thing at once. */
    E_INFO("Whole utterance:\n");
    cmn_live_set(acmod->fcb->cmn_struct, cmninit);
    nsamps = ftell(rawfh) / sizeof(*buf);
    clearerr(rawfh);
    fseek(rawfh, 0, SEEK_SET);
    buf = ckd_realloc(buf, nsamps * sizeof(*buf));
    TEST_EQUAL(nsamps, fread(buf, sizeof(*buf), nsamps, rawfh));
    bptr = buf;
    TEST_EQUAL(0, acmod_start_utt(acmod));
    acmod_process_raw(acmod, &bptr, &nsamps, TRUE);
    TEST_EQUAL(0, acmod_end_utt(acmod));
    {
        int16 best_score;
        int frame_idx = -1, best_senid;
        frame_counter = 0;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            E_INFO("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            if (frame_counter < NUM_BEST_SEN)
                TEST_EQUAL_LOG(best_score, bestsen1[frame_counter]);
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    /* Now process MFCCs and make sure we get the same results. */
    cepbuf = ckd_calloc_2d(frame_counter,
                           fe_get_output_size(acmod->fe),
                           sizeof(**cepbuf));
    fe_start(acmod->fe);
    nsamps = ftell(rawfh) / sizeof(*buf);
    bptr = buf;
    nfr = frame_counter;
    fe_process_int16(acmod->fe, &bptr, &nsamps, cepbuf, nfr);
    fe_end(acmod->fe, cepbuf + frame_counter - 1, 1);

    E_INFO("Incremental(MFCC):\n");
    cmn_live_set(acmod->fcb->cmn_struct, cmninit);
    TEST_EQUAL(0, acmod_start_utt(acmod));
    cptr = cepbuf;
    nfr = frame_counter;
    frame_counter = 0;
    while ((acmod_process_cep(acmod, &cptr, &nfr, FALSE)) > 0) {
        int16 best_score;
        int frame_idx = -1, best_senid;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            E_INFO("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            TEST_EQUAL(frame_counter, frame_idx);
            if (frame_counter < NUM_BEST_SEN)
                TEST_EQUAL_LOG(best_score, bestsen1[frame_counter]);
            ++frame_counter;
            frame_idx = -1;
        }
    }
    TEST_EQUAL(0, acmod_end_utt(acmod));
    nfr = 0;
    acmod_process_cep(acmod, &cptr, &nfr, FALSE);
    {
        int16 best_score;
        int frame_idx = -1, best_senid;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            E_INFO("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            TEST_EQUAL(frame_counter, frame_idx);
            if (frame_counter < NUM_BEST_SEN)
                TEST_EQUAL_LOG(best_score, bestsen1[frame_counter]);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    /* Note that we have to process the whole thing again because
     * !#@$@ s2mfc2feat modifies its argument (not for long) */
    fe_start(acmod->fe);
    nsamps = ftell(rawfh) / sizeof(*buf);
    bptr = buf;
    nfr = frame_counter;
    fe_process_int16(acmod->fe, &bptr, &nsamps, cepbuf, nfr);
    fe_end(acmod->fe, cepbuf + frame_counter - 1, 1);

    E_INFO("Whole utterance (MFCC):\n");
    cmn_live_set(acmod->fcb->cmn_struct, cmninit);
    TEST_EQUAL(0, acmod_start_utt(acmod));
    cptr = cepbuf;
    nfr = frame_counter;
    acmod_process_cep(acmod, &cptr, &nfr, TRUE);
    TEST_EQUAL(0, acmod_end_utt(acmod));
    {
        int16 best_score;
        int frame_idx = -1, best_senid;
        frame_counter = 0;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            E_INFO("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            if (frame_counter < NUM_BEST_SEN)
                TEST_EQUAL_LOG(best_score, bestsen1[frame_counter]);
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    E_INFO("Rewound (MFCC):\n");
    TEST_EQUAL(0, acmod_rewind(acmod));
    {
        int16 best_score;
        int frame_idx = -1, best_senid;
        frame_counter = 0;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            E_INFO("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            if (frame_counter < NUM_BEST_SEN)
                TEST_EQUAL_LOG(best_score, bestsen1[frame_counter]);
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    /* Clean up, go home. */
    ckd_free_2d(cepbuf);
    fclose(rawfh);
    ckd_free(buf);
    acmod_free(acmod);
    fe_free(fe);
    feat_free(fcb);
    logmath_free(lmath);
    config_free(config);
    return 0;
}

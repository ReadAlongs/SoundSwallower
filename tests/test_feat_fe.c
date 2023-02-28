#include "config.h"

#include <errno.h>
#include <stdio.h>

#include <soundswallower/ckd_alloc.h>
#include <soundswallower/config_defs.h>
#include <soundswallower/configuration.h>
#include <soundswallower/fe.h>
#include <soundswallower/feat.h>

#include "test_macros.h"

enum { AGC_NONE };

int
main(int argc, char *argv[])
{
    static const config_param_t fe_args[] = {
        FE_OPTIONS,
        FEAT_OPTIONS,
        { NULL, 0, NULL, NULL }
    };
    FILE *raw;
    config_t *config;
    fe_t *fe;
    feat_t *fcb;
    int16 buf[2048];
    mfcc_t **cepbuf, **cptr;
    mfcc_t ***featbuf1, ***featbuf2, ***fptr;
    size_t nsamp;
    int32 total_frames, ncep, nfr, i;

    (void)argc;
    (void)argv;
    if ((raw = fopen(TESTDATADIR "/goforward.raw", "rb")) == NULL) {
        perror(TESTDATADIR "/goforward.raw");
        return 1;
    }

    config = config_init(fe_args);
    fe = fe_init(config);
    fcb = feat_init(config);

    /* Determine how much data and how many MFCC frames we need. */
    fseek(raw, 0, SEEK_END);
    nsamp = ftell(raw) / sizeof(int16);
    total_frames = fe_process_int16(fe, NULL, &nsamp, NULL, 0);
    printf("%ld samples, %d frames\n", nsamp, total_frames);
    cepbuf = ckd_calloc_2d(total_frames, fe_get_output_size(fe), sizeof(**cepbuf));
    fseek(raw, 0, SEEK_SET);

    /* fe_process_frames() had a BAD API so I changed it */
    fe_start(fe);
    cptr = cepbuf;
    nfr = total_frames;
    while ((nsamp = fread(buf, sizeof(int16), 2048, raw)) > 0) {
        int16 *bptr = buf;
        while (nsamp) {
            int ncep = fe_process_int16(fe, &bptr, &nsamp,
                                        cptr, nfr);
            nfr -= ncep;
            cptr += ncep;
        }
    }
    fe_end(fe, cptr, nfr);

    /* Now test some feature extraction problems. */
    featbuf1 = feat_array_alloc(fcb, total_frames);
    featbuf2 = feat_array_alloc(fcb, total_frames);

    /* Whole utterance: canonical, assumed to be correct. */
    ncep = total_frames;
    TEST_EQUAL(total_frames,
               feat_s2mfc2feat_live(fcb, cepbuf,
                                    &ncep, TRUE, TRUE,
                                    featbuf1));
    TEST_EQUAL(ncep, total_frames);

    /* Process one frame at a time. */
    cptr = cepbuf;
    fptr = featbuf2;
    ncep = 1;
    nfr = feat_s2mfc2feat_live(fcb, cptr, &ncep, TRUE, FALSE, fptr);
    TEST_EQUAL(nfr, 0); /* Not possible to make any frames yet. */
    TEST_EQUAL(ncep, 1); /* But we shold have consumed one. */
    cptr += ncep;
    for (i = 1; i < total_frames - 1; ++i) {
        ncep = 1;
        nfr = feat_s2mfc2feat_live(fcb, cptr, &ncep, FALSE, FALSE, fptr);
        cptr += ncep;
        fptr += nfr;
    }
    nfr = feat_s2mfc2feat_live(fcb, cptr, &ncep, FALSE, TRUE, fptr);
    TEST_EQUAL(nfr, 4); /* This should have dumped the trailing window. */
    TEST_EQUAL(ncep, 1); /* And only consumed one frame of MFCCs. */
    cptr += ncep;
    fptr += nfr;
    /* Verify that we actually got the correct number of frames. */
    TEST_EQUAL(cptr - cepbuf, total_frames);
    TEST_EQUAL(fptr - featbuf2, total_frames);

    /* Now verify that the results are equal. */
    for (i = 0; i < total_frames; ++i) {
        int32 j;
        printf("%-4d ", i);
        for (j = 0; (uint32)j < feat_dimension(fcb); ++j) {
            TEST_EQUAL_FLOAT(featbuf1[i][0][j], featbuf2[i][0][j]);
        }
        if (i % 10 == 9)
            printf("\n");
    }
    printf("\n");

    /* Process large chunks of frames at once, so as to exceed the
     * internal ringbuffer size in feat_s2mfc2feat_live(). */
    cptr = cepbuf;
    fptr = featbuf2;
    ncep = total_frames;
    nfr = feat_s2mfc2feat_live(fcb, cptr, &ncep, TRUE, FALSE, fptr);
    TEST_ASSERT(ncep != nfr);
    cptr += ncep;
    fptr += nfr;
    ncep = total_frames - ncep;
    while (ncep) {
        int32 tmp_ncep;
        tmp_ncep = ncep;
        nfr = feat_s2mfc2feat_live(fcb, cptr, &tmp_ncep, FALSE, FALSE, fptr);
        cptr += tmp_ncep;
        fptr += nfr;
        ncep -= tmp_ncep;
    }
    nfr = feat_s2mfc2feat_live(fcb, cptr, &ncep, FALSE, TRUE, fptr);
    cptr += ncep;
    fptr += nfr;
    TEST_EQUAL(cptr - cepbuf, total_frames);
    TEST_EQUAL(fptr - featbuf2, total_frames);

    /* Now verify that the results are equal. */
    for (i = 0; i < total_frames; ++i) {
        int32 j;
        printf("%-4d ", i);
        for (j = 0; (uint32)j < feat_dimension(fcb); ++j)
            TEST_EQUAL_FLOAT(featbuf1[i][0][j], featbuf2[i][0][j]);
        if (i % 10 == 9)
            printf("\n");
    }
    printf("\n");

    fclose(raw);
    fe_free(fe);
    feat_array_free(featbuf1);
    feat_array_free(featbuf2);
    feat_free(fcb);
    ckd_free_2d(cepbuf);
    config_free(config);

    return 0;
}

/* -*- c-basic-offset: 4 -*- */
#include "config.h"

#include <stdio.h>
#include <string.h>

#include <soundswallower/fe.h>

#include <soundswallower/byteorder.h>
#include <soundswallower/ckd_alloc.h>
#include <soundswallower/config_defs.h>
#include <soundswallower/configuration.h>
#include <soundswallower/err.h>

#include "test_macros.h"

/**
 * Create a "reference" MFCC processing only whole frames
 */
mfcc_t **
create_reference(fe_t *fe, float32 *data, size_t nsamp)
{
    int32 frame_shift, frame_size;
    int nfr_full, nfr_output, ncep, i;
    mfcc_t **cepbuf;

    fe_get_input_size(fe, &frame_shift, &frame_size);
    /* Number of full frames that can be extracted from the given
     * number of samples. */
    nfr_full = 1 + (int)((nsamp - frame_size) / frame_shift);
    printf("1 + (%lu samples - %d frame_size) / %d frame_shift = %d\n",
           nsamp, frame_size, frame_shift, nfr_full);
    /* Number that will be extracted overall. */
    if ((size_t)(nfr_full - 1) * frame_shift + frame_size < nsamp) {
        nfr_output = nfr_full + 1;
        printf("%lu extra samples, nfr = %d\n",
               nsamp - ((nfr_full - 1) * frame_shift + frame_size),
               nfr_output);
    } else {
        nfr_output = nfr_full;
    }
    ncep = fe_get_output_size(fe);
    printf("ncep = %d\n", ncep);
    cepbuf = ckd_calloc_2d(nfr_output, ncep, sizeof(**cepbuf));

    for (i = 0; i < nfr_full; ++i) {
        printf("frame %d from %d to %d\n",
               i, i * frame_shift, i * frame_shift + frame_size);
        fe_read_frame_float32(fe, data + (i * frame_shift), frame_size);
        fe_write_frame(fe, cepbuf[i]);
    }

    /* Create the last frame explicitly to ensure no lossage. */
    if (nfr_output > nfr_full) {
        int last_frame_size = (int)(nsamp - nfr_full * frame_shift);
        float32 *last_frame = ckd_calloc(last_frame_size,
                                         sizeof(*last_frame));
        printf("frame %d from %d to %lu (%d samples)\n",
               nfr_full, nfr_full * frame_shift, nsamp,
               last_frame_size);
        memcpy(last_frame,
               data + nfr_full * frame_shift,
               last_frame_size * sizeof(*data));
        fe_read_frame_float32(fe, last_frame, last_frame_size);
        fe_write_frame(fe, cepbuf[nfr_full]);
        ckd_free(last_frame);
    }

    for (i = 0; i < 5; ++i) {
        int j;
        printf("%d: ", i);
        for (j = 0; j < ncep; ++j) {
            printf("%.2f ",
                   MFCC2FLOAT(cepbuf[i][j]));
        }
        printf("\n");
    }
    return cepbuf;
}

/**
 * Create MFCC using shift_frame
 */
mfcc_t **
create_shifted(fe_t *fe, float32 *data, size_t nsamp)
{
    int32 frame_shift, frame_size;
    int nfr_full, nfr_output, ncep, i;
    mfcc_t **cepbuf;
    float32 *inptr;

    fe_get_input_size(fe, &frame_shift, &frame_size);
    /* Number of full frames that can be extracted from the given
     * number of samples. */
    nfr_full = 1 + (int)((nsamp - frame_size) / frame_shift);
    /* Number that will be extracted overall. */
    if ((size_t)(nfr_full - 1) * frame_shift + frame_size < nsamp)
        nfr_output = nfr_full + 1;
    else
        nfr_output = nfr_full;
    ncep = fe_get_output_size(fe);
    cepbuf = ckd_calloc_2d(nfr_output, ncep, sizeof(**cepbuf));

    inptr = data;
    printf("start inptr = %lu\n", inptr - data);
    inptr += fe_read_frame_float32(fe, inptr, frame_size);
    fe_write_frame(fe, cepbuf[0]);
    printf("after first frame = %lu\n", inptr - data);
    for (i = 1; i < nfr_output; ++i) {
        inptr += fe_shift_frame_float32(fe, inptr, data + nsamp - inptr);
        fe_write_frame(fe, cepbuf[i]);
        printf("after frame %d = %lu\n", i, inptr - data);
    }
    TEST_EQUAL(inptr, data + nsamp);

    for (i = 0; i < nfr_output; ++i) {
        int j;
        printf("%d: ", i);
        for (j = 0; j < ncep; ++j) {
            printf("%.2f ",
                   MFCC2FLOAT(cepbuf[i][j]));
        }
        printf("\n");
    }
    return cepbuf;
}

mfcc_t **
create_full(fe_t *fe, float32 *data, size_t nsamp)
{
    mfcc_t **cepbuf;
    float32 *inptr;
    int rv, nfr, ncep;

    TEST_EQUAL(0, fe_start(fe));
    nfr = fe_process_float32(fe, NULL, &nsamp, NULL, 0);
    TEST_EQUAL(5, nfr);
    ncep = fe_get_output_size(fe);

    cepbuf = ckd_calloc_2d(nfr, ncep, sizeof(**cepbuf));
    inptr = data;
    rv = fe_process_float32(fe, &inptr, &nsamp, cepbuf, nfr);
    nfr -= rv;
    printf("fe_process_float32 produced %d frames, "
           " %lu samples remaining\n",
           rv, nsamp);
    TEST_EQUAL(rv, 4);
    TEST_EQUAL(nfr, 1);
    TEST_EQUAL(inptr - data, 1024);
    TEST_EQUAL(nsamp, 0);
    /* Should get a frame here due to overflow samples. */
    rv = fe_end(fe, cepbuf + rv, nfr);
    printf("fe_end rv %d\n", rv);
    TEST_EQUAL(rv, 1);

    return cepbuf;
}

mfcc_t **
create_process_frames(fe_t *fe, float32 *data, size_t nsamp)
{
    mfcc_t **cepbuf;
    float32 *inptr;
    int i, rv, nfr, ncep, frame_shift, frame_size;

    fe_get_input_size(fe, &frame_shift, &frame_size);
    TEST_EQUAL(0, fe_start(fe));
    nfr = fe_process_float32(fe, NULL, &nsamp, NULL, 0);
    TEST_EQUAL(5, nfr);
    ncep = fe_get_output_size(fe);

    cepbuf = ckd_calloc_2d(nfr, ncep, sizeof(**cepbuf));
    inptr = data;

    for (i = 0; i < 4; ++i) {
        rv = fe_process_float32(fe, &inptr, &nsamp, &cepbuf[i], 1);
        printf("frame %d updated inptr %lu remaining nsamp %lu "
               "processed %d\n",
               i, inptr - data, nsamp, rv);
        TEST_EQUAL(rv, 1);
        if (i < 3) {
            /* Confusingly, it will read an extra frame_shift data
               in order to make the next frame... */
            TEST_EQUAL(inptr - data, frame_size + (i + 1) * frame_shift);
        } else {
            TEST_EQUAL(inptr - data, 1024);
        }
    }

    /* Should get a frame here due to overflow samples. */
    rv = fe_end(fe, cepbuf + 4, 1);
    printf("fe_end rv %d\n", rv);
    TEST_EQUAL(rv, 1);

    return cepbuf;
}

mfcc_t **
create_fragments(fe_t *fe, float32 *data, size_t nsamp)
{
    mfcc_t **cepbuf, **cepptr;
    float32 *inptr;
    int i, rv, nfr, ncep, frame_shift, frame_size;
    /* Should total 1024 :) */
    size_t fragments[] = {
        1, 145, 39, 350, 410, 79
    };

    fe_get_input_size(fe, &frame_shift, &frame_size);
    TEST_EQUAL(0, fe_start(fe));
    nfr = fe_process_float32(fe, NULL, &nsamp, NULL, 0);
    TEST_EQUAL(5, nfr);
    ncep = fe_get_output_size(fe);

    cepptr = cepbuf = ckd_calloc_2d(nfr, ncep, sizeof(**cepbuf));
    inptr = data;

    /* Process with fragments of unusual size. */
    for (i = 0; (size_t)i < sizeof(fragments) / sizeof(fragments[0]); ++i) {
        size_t fragment = fragments[i];
        rv = fe_process_float32(fe, &inptr, &fragment, cepptr, nfr);
        nfr -= rv;
        printf("fragment %d updated inptr %lu remaining nsamp %lu "
               "processed %d remaining nfr %d\n",
               i, inptr - data, fragment, rv, nfr);
        TEST_EQUAL(0, fragment);
        cepptr += rv;
    }

    /* Should get a frame here due to overflow samples. */
    rv = fe_end(fe, cepptr, 1);
    printf("fe_end rv %d\n", rv);
    TEST_EQUAL(rv, 1);

    return cepbuf;
}

mfcc_t **
create_mixed_fragments(fe_t *fe, float32 *data, int16 *idata, size_t nsamp, int odd)
{
    mfcc_t **cepbuf, **cepptr;
    float32 *inptr;
    int16 *iinptr;
    int i, rv, nfr, ncep, frame_shift, frame_size;
    /* Should total 1024 :) */
    size_t fragments[] = {
        1, 145, 39, 350, 450, 39
    };

    fe_get_input_size(fe, &frame_shift, &frame_size);
    TEST_EQUAL(0, fe_start(fe));
    nfr = fe_process_float32(fe, NULL, &nsamp, NULL, 0);
    TEST_EQUAL(5, nfr);
    ncep = fe_get_output_size(fe);

    cepptr = cepbuf = ckd_calloc_2d(nfr, ncep, sizeof(**cepbuf));
    inptr = data;
    iinptr = idata;

    /* Process with fragments of unusual size. */
    for (i = 0; (size_t)i < sizeof(fragments) / sizeof(fragments[0]); ++i) {
        size_t fragment = fragments[i];
        int do_float = odd ? (i & 1) : (!(i & 1));
        if (do_float) {
            rv = fe_process_float32(fe, &inptr, &fragment, cepptr, nfr);
            nfr -= rv;
            iinptr += (inptr - data) - (iinptr - idata);
        } else {
            rv = fe_process_int16(fe, &iinptr, &fragment, cepptr, nfr);
            nfr -= rv;
            inptr += (iinptr - idata) - (inptr - data);
        }
        printf("%s fragment %d updated inptr %lu %lu remaining nsamp %lu "
               "processed %d remaining nfr %d\n",
               do_float ? "float" : "int",
               i, inptr - data, iinptr - idata, fragment, rv, nfr);
        TEST_EQUAL(0, fragment);
        cepptr += rv;
    }

    /* Should get a frame here due to overflow samples. */
    rv = fe_end(fe, cepptr, 1);
    printf("fe_end rv %d\n", rv);
    TEST_EQUAL(rv, 1);

    return cepbuf;
}

void
compare_cepstra(mfcc_t **c1, mfcc_t **c2, int nfr, int ncep)
{
    int i, j;
    for (i = 0; i < nfr; ++i) {
        printf("%d: ", i);
        for (j = 0; j < ncep; ++j) {
            printf("%.2f,%.2f ",
                   MFCC2FLOAT(c1[i][j]),
                   MFCC2FLOAT(c2[i][j]));
            TEST_EQUAL_FLOAT(c1[i][j], c2[i][j]);
        }
        printf("\n");
    }
}

int
main(int argc, char *argv[])
{
    static const config_param_t fe_args[] = {
        FE_OPTIONS,
        { NULL, 0, NULL, NULL }
    };
    FILE *raw;
    config_t *config;
    fe_t *fe;
    int16 ibuf[1024];
    float32 buf[1024];
    int32 frame_shift, frame_size;
    mfcc_t **cepbuf, **cepbuf1;
    size_t i;

    (void)argc;
    (void)argv;
    err_set_loglevel_str("INFO");
    TEST_ASSERT(config = config_init(fe_args));
    /* Even though we make our own float32 data, we will ensure it's
       little-endian to be consistent. */
    config_set_str(config, "input_endian", "little");
    TEST_ASSERT(fe = fe_init(config));

    TEST_EQUAL(fe_get_output_size(fe), DEFAULT_NUM_CEPSTRA);

    fe_get_input_size(fe, &frame_shift, &frame_size);
    TEST_EQUAL(frame_shift, DEFAULT_FRAME_SHIFT);
    TEST_EQUAL(frame_size, (int)(DEFAULT_WINDOW_LENGTH * DEFAULT_SAMPLING_RATE));

    TEST_ASSERT(raw = fopen(TESTDATADIR "/goforward.raw", "rb"));
    TEST_EQUAL(1024, fread(ibuf, sizeof(int16), 1024, raw));
    for (i = 0; i < 1024; ++i) {
        int16 sample = ibuf[i];
        SWAP_LE_16(&sample);
        buf[i] = sample / FLOAT32_SCALE;
        /* Ensure ibuf and buf are both little-endian as noted above */
        SWAP_LE_32((int32 *)(char *)&buf[i]);
    }

    printf("Creating reference features\n");
    cepbuf = create_reference(fe, buf, 1024);

    printf("Creating features with frame_shift\n");
    cepbuf1 = create_shifted(fe, buf, 1024);
    compare_cepstra(cepbuf, cepbuf1, 5, DEFAULT_NUM_CEPSTRA);
    ckd_free_2d(cepbuf1);

    printf("Creating features with full buffer\n");
    cepbuf1 = create_full(fe, buf, 1024);
    compare_cepstra(cepbuf, cepbuf1, 5, DEFAULT_NUM_CEPSTRA);
    ckd_free_2d(cepbuf1);

    printf("Creating features with individual frames\n");
    cepbuf1 = create_process_frames(fe, buf, 1024);
    compare_cepstra(cepbuf, cepbuf1, 5, DEFAULT_NUM_CEPSTRA);
    ckd_free_2d(cepbuf1);

    printf("Creating features with oddly sized fragments\n");
    cepbuf1 = create_fragments(fe, buf, 1024);
    compare_cepstra(cepbuf, cepbuf1, 5, DEFAULT_NUM_CEPSTRA);
    ckd_free_2d(cepbuf1);

    printf("Creating features with oddly sized fragments of mixed types\n");
    cepbuf1 = create_mixed_fragments(fe, buf, ibuf, 1024, 1);
    compare_cepstra(cepbuf, cepbuf1, 5, DEFAULT_NUM_CEPSTRA);
    ckd_free_2d(cepbuf1);

    printf("Creating features with oddly sized fragments of mixed types (other order)\n");
    cepbuf1 = create_mixed_fragments(fe, buf, ibuf, 1024, 0);
    compare_cepstra(cepbuf, cepbuf1, 5, DEFAULT_NUM_CEPSTRA);
    ckd_free_2d(cepbuf1);

    ckd_free_2d(cepbuf);
    fclose(raw);
    fe_free(fe);
    config_free(config);

    return 0;
}

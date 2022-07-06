#include "config.h"

#include <stdio.h>
#include <string.h>

#include <soundswallower/fe.h>
#include <soundswallower/fe_internal.h>
#include <soundswallower/err.h>
#include <soundswallower/cmd_ln.h>
#include <soundswallower/ckd_alloc.h>

#include "test_macros.h"

/**
 * Create a "reference" MFCC processing only whole frames
 */
mfcc_t **
create_reference(fe_t *fe, int16 *data, size_t nsamp)
{
    int32 frame_shift, frame_size;
    int nfr_full, nfr_output, ncep, i;
    mfcc_t **cepbuf;

    fe_get_input_size(fe, &frame_shift, &frame_size);
    /* Number of full frames that can be extracted from the given
     * number of samples. */
    nfr_full = 1 + (nsamp - frame_size) / frame_shift;
    E_INFO("1 + (%d samples - %d frame_size) / %d frame_shift = %d\n",
           nsamp, frame_size, frame_shift, nfr_full);
    /* Number that will be extracted overall. */
    if ((size_t)(nfr_full - 1) * frame_shift + frame_size < nsamp) {
        nfr_output = nfr_full + 1;
        E_INFO("%d extra samples, nfr = %d\n",
               nsamp - ((nfr_full - 1) * frame_shift + frame_size),
               nfr_output);
    }
    else {
        nfr_output = nfr_full;
    }
    ncep = fe_get_output_size(fe);
    E_INFO("ncep = %d\n", ncep);
    cepbuf = ckd_calloc_2d(nfr_output, ncep, sizeof(**cepbuf));

    for (i = 0; i < nfr_full; ++i) {
        E_INFO("frame %d from %d to %d\n",
               i, i * frame_shift, i * frame_shift + frame_size);
        fe_read_frame_int16(fe, data + (i * frame_shift), frame_size);
        fe_write_frame(fe, cepbuf[i]);
    }

    /* Create the last frame explicitly to ensure no lossage. */
    if (nfr_output > nfr_full) {
        int last_frame_size = nsamp - nfr_full * frame_shift;
        int16 *last_frame = ckd_calloc(last_frame_size,
                                       sizeof(*last_frame));
        E_INFO("frame %d from %d to %d (%d samples)\n",
               nfr_full, nfr_full * frame_shift, nsamp,
               last_frame_size);
        memcpy(last_frame,
               data + nfr_full * frame_shift,
               last_frame_size * sizeof(*data));
        fe_read_frame_int16(fe, last_frame, last_frame_size);
        fe_write_frame(fe, cepbuf[nfr_full]);
    }

    for (i = 0; i < 5; ++i) {
        int j;
        E_INFO("%d: ", i);
        for (j = 0; j < ncep; ++j) {
            E_INFOCONT("%.2f ",
                       MFCC2FLOAT(cepbuf[i][j]));
        }
        E_INFOCONT("\n");
    }
    return cepbuf;
}

/**
 * Create MFCC using shift_frame
 */
mfcc_t **
create_shifted(fe_t *fe, int16 *data, size_t nsamp)
{
    int32 frame_shift, frame_size;
    int nfr_full, nfr_output, ncep, i;
    mfcc_t **cepbuf;
    int16 *inptr;

    fe_get_input_size(fe, &frame_shift, &frame_size);
    /* Number of full frames that can be extracted from the given
     * number of samples. */
    nfr_full = 1 + (nsamp - frame_size) / frame_shift;
    /* Number that will be extracted overall. */
    if ((size_t)(nfr_full - 1) * frame_shift + frame_size < nsamp)
        nfr_output = nfr_full + 1;
    else
        nfr_output = nfr_full;
    ncep = fe_get_output_size(fe);
    cepbuf = ckd_calloc_2d(nfr_output, ncep, sizeof(**cepbuf));

    inptr = data;
    E_INFO("start inptr = %ld\n", inptr - data);
    inptr += fe_read_frame_int16(fe, inptr, frame_size);
    fe_write_frame(fe, cepbuf[0]);
    E_INFO("after first frame = %ld\n", inptr - data);
    for (i = 1; i < nfr_output; ++i) {
        inptr += fe_shift_frame_int16(fe, inptr, data + nsamp - inptr);
        fe_write_frame(fe, cepbuf[i]);
        E_INFO("after frame %d = %ld\n", i, inptr - data);
    }
    TEST_EQUAL(inptr, data + nsamp);

    for (i = 0; i < nfr_output; ++i) {
        int j;
        E_INFO("%d: ", i);
        for (j = 0; j < ncep; ++j) {
            E_INFOCONT("%.2f ",
                       MFCC2FLOAT(cepbuf[i][j]));
        }
        E_INFOCONT("\n");
    }
    return cepbuf;
}

mfcc_t **
create_frames(fe_t *fe, const int16 *data, size_t nsamp)
{
    mfcc_t **cepbuf;
    const int16 *inptr;
    int rv, nfr, ncep;
    
    TEST_EQUAL(0, fe_start_utt(fe));
    rv = fe_process(fe, NULL, &nsamp, NULL, &nfr);
    TEST_EQUAL(0, rv);
    TEST_EQUAL(4, nfr);
    ncep = fe_get_output_size(fe);

    /* Allow an extra overflow frame. */
    ++nfr;
    cepbuf = ckd_calloc_2d(nfr, ncep, sizeof(**cepbuf));
    inptr = data;

    rv = fe_process(fe, &inptr, &nsamp, cepbuf, &nfr);
    E_INFO("fe_process_frames produced %d frames, "
           " %d samples remaining\n", rv, nsamp);
    TEST_EQUAL(rv, 4);
    TEST_EQUAL(nfr, 1);
    TEST_EQUAL(inptr - data, 1024);
    TEST_EQUAL(nsamp, 0);
    /* Should get a frame here due to overflow samples. */
    rv = fe_end_utt(fe, cepbuf[4], &nfr);
    E_INFO("fe_end_utt rv %d nfr %d\n", rv, nfr);
    TEST_EQUAL(rv, 1);
    TEST_EQUAL(nfr, 0);

    return cepbuf;
}

mfcc_t **
create_full(fe_t *fe, const int16 *data, size_t nsamp)
{
    mfcc_t **cepbuf;
    const int16 *inptr;
    int rv, nfr, ncep;
    
    TEST_EQUAL(0, fe_start_utt(fe));
    rv = fe_process(fe, NULL, &nsamp, NULL, &nfr);
    TEST_EQUAL(0, rv);
    TEST_EQUAL(4, nfr);
    ncep = fe_get_output_size(fe);

    /* Allow an extra overflow frame. */
    ++nfr;
    cepbuf = ckd_calloc_2d(nfr, ncep, sizeof(**cepbuf));
    inptr = data;
    rv = fe_process(fe, &inptr, &nsamp, cepbuf, &nfr);
    E_INFO("fe_process_frames produced %d frames, "
           " %d samples remaining\n", rv, nsamp);
    TEST_EQUAL(rv, 4);
    TEST_EQUAL(nfr, 1);
    TEST_EQUAL(inptr - data, 1024);
    TEST_EQUAL(nsamp, 0);
    /* Should get a frame here due to overflow samples. */
    rv = fe_end_utt(fe, cepbuf[4], &nfr);
    E_INFO("fe_end_utt rv %d nfr %d\n", rv, nfr);
    TEST_EQUAL(rv, 1);
    TEST_EQUAL(nfr, 0);

    return cepbuf;
}

mfcc_t **
create_process_frames(fe_t *fe, const int16 *data, size_t nsamp)
{
    mfcc_t **cepbuf;
    const int16 *inptr;
    int i, rv, nfr, ncep, frame_shift, frame_size;
    
    fe_get_input_size(fe, &frame_shift, &frame_size);
    TEST_EQUAL(0, fe_start_utt(fe));
    rv = fe_process(fe, NULL, &nsamp, NULL, &nfr);
    TEST_EQUAL(0, rv);
    TEST_EQUAL(4, nfr);
    ncep = fe_get_output_size(fe);

    /* Allow an extra overflow frame. */
    ++nfr;
    cepbuf = ckd_calloc_2d(nfr, ncep, sizeof(**cepbuf));
    inptr = data;

    for (i = 0; i < 4; ++i) {
        nfr = 1;
        rv = fe_process(fe, &inptr, &nsamp, &cepbuf[i], &nfr);
        E_INFO("frame %d updated inptr %ld remaining nsamp %ld "
               "processed %d remaining nfr %d\n",
               i, inptr - data, nsamp, rv, nfr);
        TEST_EQUAL(rv, 1);
        TEST_EQUAL(nfr, 0);
        if (i < 3) {
            TEST_EQUAL(inptr - data, frame_size + i * frame_shift);
        }
        else {
            TEST_EQUAL(inptr - data, 1024);
        }
    }

    /* Should get a frame here due to overflow samples. */
    rv = fe_end_utt(fe, cepbuf[4], &nfr);
    E_INFO("fe_end_utt rv %d nfr %d\n", rv, nfr);
    TEST_EQUAL(rv, 1);
    TEST_EQUAL(nfr, 0);

    return cepbuf;
}

void
compare_cepstra(mfcc_t **c1, mfcc_t **c2, int nfr, int ncep)
{
    int i, j;
    for (i = 0; i < nfr; ++i) {
        E_INFO("%d: ", i);
        for (j = 0; j < ncep; ++j) {
            E_INFOCONT("%.2f,%.2f ",
                       MFCC2FLOAT(c1[i][j]),
                       MFCC2FLOAT(c2[i][j]));
            TEST_EQUAL_FLOAT(c1[i][j], c2[i][j]);
        }
        E_INFOCONT("\n");
    }
}

int
main(int argc, char *argv[])
{
    static const arg_t fe_args[] = {
        waveform_to_cepstral_command_line_macro(),
        { NULL, 0, NULL, NULL }
    };
    FILE *raw;
    cmd_ln_t *config;
    fe_t *fe;
    int16 buf[1024];
    int16 const *inptr;
    int32 frame_shift, frame_size;
    mfcc_t **cepbuf, **cepbuf1, **cepbuf2, **cptr;
    int32 nfr, nvec, i;
    size_t nsamp;

    err_set_loglevel_str("INFO");
    TEST_ASSERT(config = cmd_ln_parse_r(NULL, fe_args, argc, argv, FALSE));
    TEST_ASSERT(fe = fe_init(config));

    TEST_EQUAL(fe_get_output_size(fe), DEFAULT_NUM_CEPSTRA);

    fe_get_input_size(fe, &frame_shift, &frame_size);
    TEST_EQUAL(frame_shift, DEFAULT_FRAME_SHIFT);
    TEST_EQUAL(frame_size, (int)(DEFAULT_WINDOW_LENGTH
                                 * DEFAULT_SAMPLING_RATE));


    TEST_ASSERT(raw = fopen(TESTDATADIR "/goforward.raw", "rb"));
    TEST_EQUAL(1024, fread(buf, sizeof(int16), 1024, raw));

    E_INFO("Creating reference features\n");
    cepbuf = create_reference(fe, buf, 1024);
 
    E_INFO("Creating features with frame_shift\n");
    cepbuf1 = create_shifted(fe, buf, 1024);
    compare_cepstra(cepbuf, cepbuf1, 5, DEFAULT_NUM_CEPSTRA);
    ckd_free_2d(cepbuf1);

    E_INFO("Creating features with full buffer\n");
    cepbuf1 = create_full(fe, buf, 1024);
    compare_cepstra(cepbuf, cepbuf1, 5, DEFAULT_NUM_CEPSTRA);
    ckd_free_2d(cepbuf1);

    E_INFO("Creating features with individual frames\n");
    cepbuf1 = create_process_frames(fe, buf, 1024);
    compare_cepstra(cepbuf, cepbuf1, 5, DEFAULT_NUM_CEPSTRA);
    ckd_free_2d(cepbuf1);


#if 0
    /* Test that the output we get by processing one frame at a time
     * is exactly the same as what we get from doing them all at once. */
    
    /* Now, also test to make sure that even if we feed data in
     * little tiny bits we can still make things work. */
    E_INFO("Testing inputs smaller than one frame (256 samples)\n");
    memset(cepbuf2[0], 0, 5 * DEFAULT_NUM_CEPSTRA * sizeof(**cepbuf2));
    inptr = &buf[0];
    cptr = &cepbuf2[0];
    nfr = 5;
    i = 5;
    nsamp = 256;
    TEST_EQUAL(0, fe_start_utt(fe));
    /* Process up to 5 frames (that will not happen) */
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cptr, &i) >= 0);
    E_INFO("updated inptr %ld remaining nsamp %ld processed nfr %d\n",
	   inptr - buf, nsamp, i);
    cptr += i;
    /* Process up to however many frames are left to make 5 */
    nfr -= i;
    i = nfr;
    nsamp = 256;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cptr, &i) >= 0);
    E_INFO("updated inptr %ld remaining nsamp %ld processed nfr %d\n",
	   inptr - buf, nsamp, i);
    cptr += i;
    nfr -= i;
    i = nfr;
    nsamp = 256;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cptr, &i) >= 0);
    E_INFO("updated inptr %ld remaining nsamp %ld processed nfr %d\n",
	   inptr - buf, nsamp, i);
    cptr += i;
    nfr -= i;
    i = nfr;
    nsamp = 256;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cptr, &i) >= 0);
    E_INFO("updated inptr %ld remaining nsamp %ld processed nfr %d\n",
	   inptr - buf, nsamp, i);
    cptr += i;
    nfr -= i;
    E_INFO("nfr %d\n", nfr);
    /* We processed 1024 bytes, which should give us 4 frames */
    TEST_EQUAL(nfr, 1);
    TEST_ASSERT(fe_end_utt(fe, *cptr, &nfr) >= 0);
    E_INFO("nfr %d\n", nfr);
    TEST_EQUAL(nfr, 1);

    /* output features stored in cepbuf should be the same */
    for (nfr = 0; nfr < 5; ++nfr) {
      E_INFO("%d: ", nfr);
      for (i = 0; i < DEFAULT_NUM_CEPSTRA; ++i) {
        E_INFOCONT("%.2f,%.2f ",
		   MFCC2FLOAT(cepbuf1[nfr][i]),
		   MFCC2FLOAT(cepbuf2[nfr][i]));
        TEST_EQUAL_FLOAT(cepbuf1[nfr][i], cepbuf2[nfr][i]);
      }
      E_INFOCONT("\n");
    }
    ckd_free_2d(cepbuf);
    ckd_free_2d(cepbuf1);
    ckd_free_2d(cepbuf2);
#endif
    fclose(raw);
    fe_free(fe);
    cmd_ln_free_r(config);

    return 0;
}

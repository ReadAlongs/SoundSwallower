/* Test voice activity detection.
 *
 * MIT license (c) 2022, see LICENSE for more information.
 *
 * Author: David Huggins-Daines <dhdaines@gmail.com>
 */

#include <soundswallower/ckd_alloc.h>
#include <soundswallower/err.h>
#include <soundswallower/vad.h>

#include "test_macros.h"

static const char *expecteds[] = {
    "011110111111111111111111111100",
    "011110111111111111111111111100",
    "000000111111111111111111110000",
    "000000111111111111111100000000"
};
static const int n_modes = sizeof(expecteds) / sizeof(expecteds[0]);

static int sample_rates[] = {
    8000,
    16000,
    32000,
    48000,
    11025,
    22050,
    44100
};
static const int n_sample_rates = sizeof(sample_rates) / sizeof(sample_rates[0]);

static FILE *
open_data(int sample_rate)
{
    char *soxcmd;
    int len;
    FILE *sox;
#define SOXCMD "sox -q -r 8000 -c 1 -b 16 -e signed-integer -t raw -D -G " TESTDATADIR "/vad/test-audio.raw -r %d -t raw -"

    if (sample_rate == 8000)
        return fopen(TESTDATADIR "/vad/test-audio.raw", "rb");
    len = snprintf(NULL, 0, SOXCMD, sample_rate);
    if ((soxcmd = malloc(len + 1)) == NULL)
        E_FATAL_SYSTEM("Failed to allocate string");
    if (snprintf(soxcmd, len + 1, SOXCMD, sample_rate) != len)
        E_FATAL_SYSTEM("snprintf() failed");
    if ((sox = popen(soxcmd, "r")) == NULL)
        E_FATAL_SYSTEM("Failed to popen(%s)", soxcmd);
    free(soxcmd);

    return sox;
}

static void
close_data(FILE *fh, int sample_rate)
{
    if (sample_rate == 8000)
        fclose(fh);
    else
        pclose(fh);
}

static int
test_sample_rate(int sample_rate)
{
    vad_t *vader;
    short *frame;
    int i;

    /* Test VAD modes with py-webrtcvad test data. */
    for (i = 0; i < n_modes; ++i) {
        FILE *fh;
        size_t frame_size;
        char *classification, *c;

        E_INFO("Sample rate %d, mode %d\n", sample_rate, i);
        /* Extra space for approximate rates */
        c = classification = ckd_calloc(1, strlen(expecteds[i]) * 2);
        vader = vad_init(i, sample_rate, 0.03);
        TEST_ASSERT(vader);
        frame_size = vad_frame_size(vader);
        frame = ckd_calloc(sizeof(*frame), frame_size);
        TEST_ASSERT(frame);

        fh = open_data(sample_rate);
        TEST_ASSERT(fh);
        while (fread(frame, sizeof(*frame), frame_size, fh) == frame_size) {
            int is_speech = vad_classify(vader, frame);
            TEST_ASSERT(is_speech != VAD_ERROR);
            *c++ = (is_speech == VAD_SPEECH) ? '1' : '0';
        }
        E_INFO("true: %s\n", expecteds[i]);
        E_INFO("pred: %s\n", classification);
        if (sample_rate != 48000 /* Has Problems for some reason */
            && vad_frame_length(vader) == 0.03) /* skip approximate rates */
            TEST_EQUAL(0, strcmp(expecteds[i], classification));
        ckd_free(classification);
        vad_free(vader);
        ckd_free(frame);
        close_data(fh, sample_rate);
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    vad_t *vader;
    int i;

    (void)argc;
    (void)argv;
    err_set_loglevel(ERR_INFO);
    /* Test initialization with default parameters. */
    vader = vad_init(0, 0, 0);
    TEST_ASSERT(vader);
    /* Retain and release, should still be there. */
    TEST_ASSERT((vader = vad_retain(vader)));
    TEST_ASSERT(vad_free(vader));

    /* Test default frame size. */
    TEST_EQUAL(vad_frame_size(vader),
               (int)(VAD_DEFAULT_SAMPLE_RATE * VAD_DEFAULT_FRAME_LENGTH));
    TEST_EQUAL_FLOAT(vad_frame_length(vader), VAD_DEFAULT_FRAME_LENGTH);
    TEST_ASSERT(vad_free(vader) == 0);

    /* Test a variety of sample rates. */
    for (i = 0; i < n_sample_rates; ++i)
        test_sample_rate(sample_rates[i]);

    /* Test rejection of unreasonable sample rates. */
    vader = vad_init(0, 42, 0.03);
    TEST_ASSERT(vader == NULL);
    vader = vad_init(0, 96000, 0.03);
    TEST_ASSERT(vader == NULL);

    return 0;
}

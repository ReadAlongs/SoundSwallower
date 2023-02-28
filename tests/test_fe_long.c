/* Test long input buffers.
 *
 * MIT license (c) 2022, see LICENSE for more information.
 *
 * Author: David Huggins-Daines <dhdaines@gmail.com>
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <soundswallower/fe.h>

#include "test_macros.h"
#include <soundswallower/ckd_alloc.h>
#include <soundswallower/config_defs.h>
#include <soundswallower/configuration.h>
#include <soundswallower/err.h>

static FILE *
open_data(int sample_rate)
{
    char *soxcmd;
    int len;
    FILE *sox;
#define SOXCMD "sox -q -D -G " TESTDATADIR "/goforward.wav -r %d -t raw -"

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
close_data(FILE *fh)
{
    pclose(fh);
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
    int16 *data, *inptr;
    size_t bufsiz, nsamp;
    mfcc_t **cepbuf;
    int rv, nfr, ncep;

    (void)argc;
    (void)argv;
    err_set_loglevel_str("INFO");
    TEST_ASSERT(config = config_init(fe_args));
    config_set_str(config, "input_endian", "little");
    config_set_int(config, "samprate", 44100);
    TEST_ASSERT(fe = fe_init(config));

    bufsiz = 65536;
    inptr = data = ckd_malloc(bufsiz * sizeof(*data));
    TEST_ASSERT(raw = open_data(44100));
    while (1) {
        nsamp = fread(inptr, sizeof(*data), 2048, raw);
        inptr += nsamp;
        if (nsamp < 2048)
            break;
        if ((size_t)(inptr + 2048 - data) > bufsiz) {
            size_t offset = inptr - data;
            bufsiz *= 2;
            data = ckd_realloc(data, bufsiz * sizeof(*data));
            inptr = data + offset;
        }
    }
    nsamp = inptr - data;
    E_INFO("Read %d samples\n", nsamp);
    TEST_EQUAL(0, fe_start(fe));
    nfr = fe_process_int16(fe, NULL, &nsamp, NULL, 0);
    E_INFO("Will require %d frames\n", nfr);
    ncep = fe_get_output_size(fe);
    cepbuf = ckd_calloc_2d(nfr, ncep, sizeof(**cepbuf));
    inptr = data;
    rv = fe_process_int16(fe, &inptr, &nsamp, cepbuf, nfr);
    nfr -= rv;
    printf("fe_process_int16 produced %d frames, "
           " %lu samples remaining\n",
           rv, nsamp);
    TEST_EQUAL(1, fe_end(fe, cepbuf + rv, nfr));
    close_data(raw);
    ckd_free_2d(cepbuf);
    ckd_free(data);
    fe_free(fe);
    config_free(config);
}

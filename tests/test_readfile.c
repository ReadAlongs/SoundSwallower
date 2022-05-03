/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file test_readfile.c: Test for the methods to read the file
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <soundswallower/ckd_alloc.h>
#include <soundswallower/bio.h>
#include <soundswallower/err.h>

#include "test_macros.h"


int
main(int argc, char *argv[])
{
    size_t nsamps;
    int16 *data;
    
    data = bio_read_wavfile(TESTDATADIR, "goforward", ".wav", 44, FALSE, &nsamps);
    E_INFO("Read %d samples\n", nsamps);
    TEST_EQUAL(44580, nsamps);
    
    ckd_free(data);

    return 0;
}

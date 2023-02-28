#include "config.h"

#include <stdio.h>
#include <time.h>

#include <soundswallower/byteorder.h>
#include <soundswallower/profile.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    uint32 ui32 = 0xdeadbeef;
    int32 i32 = 0x12345678;
    uint16 ui16 = 0xfeed;
    int16 i16 = 0x1234;
    float32 f32a = 382.23328f;
    float32 f32b = -0.1246f;
    ptmr_t t;
    int i;

    (void)argc;
    (void)argv;
    SWAP_INT32(&ui32);
    TEST_EQUAL(0xefbeadde, ui32);
    SWAP_INT32(&i32);
    TEST_EQUAL(0x78563412, i32);

    SWAP_INT16(&ui16);
    TEST_EQUAL(0xedfe, ui16);
    SWAP_INT16(&i16);
    TEST_EQUAL(0x3412, i16);

    SWAP_FLOAT32(&f32a);
    TEST_ASSERT(fabs(f32a - -1.77607e+17f) < 1e16f);
    SWAP_FLOAT32(&f32a);
    TEST_EQUAL_FLOAT(f32a, 382.23328f);
    SWAP_FLOAT32(&f32b);
    TEST_ASSERT(fabs(f32b - 716796.0f) < 1.0f);
    SWAP_FLOAT32(&f32b);
    TEST_EQUAL_FLOAT(f32b, -0.1246f);

    ptmr_start(&t);
    for (i = 0; i < 10000000; ++i) {
        SWAP_FLOAT32(&f32a);
    }
    ptmr_stop(&t);
    printf("10M SWAP_FLOAT32 in %g\n", t.t_cpu);
    ptmr_reset(&t);
    ptmr_start(&t);
    for (i = 0; i < 10000000; ++i) {
        SWAP_INT32(&ui32);
    }
    ptmr_stop(&t);
    printf("10M SWAP_INT32 in %g\n", t.t_cpu);
    return 0;
}

#!/bin/bash

: ${CMAKE_BINARY_DIR:=$(pwd)}
. ${CMAKE_BINARY_DIR}/tests/testfuncs.sh

ulimit -c 0
if $CMAKE_BINARY_DIR/test_ckd_alloc_abort; then
    fail expected_failure
else
    pass expected_failure
fi


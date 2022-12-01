find_program(BASH_PROGRAM bash)
configure_file(test_macros.h.in test_macros.h)
configure_file(testfuncs.sh.in testfuncs.sh @ONLY)
# All tests that require no particular intervention
set(TESTS
  test_acmod
  test_acmod_grow
  test_add_words
  test_bitvec
  test_byteorder
  test_ckd_alloc
  test_dict2pid
  test_dict
  test_endpointer
  test_err
  test_fe_long
  test_feat_fe
  test_feat_live
  test_fsg
  test_hash_iter
  test_jsgf
  test_listelem_alloc
  test_log_shifted
  test_mdef
  test_ptm_mgau
  test_s3file
  test_subvq
  test_vad
  test_word_align
  )

foreach(TEST_EXECUTABLE ${TESTS})
  add_executable(${TEST_EXECUTABLE} EXCLUDE_FROM_ALL ${TEST_EXECUTABLE}.c)
  target_link_libraries(${TEST_EXECUTABLE} soundswallower)
  target_include_directories(
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_SOURCE_DIR}/src
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_BINARY_DIR}
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_test(NAME ${TEST_EXECUTABLE} COMMAND ${TEST_EXECUTABLE})
  add_dependencies(check ${TEST_EXECUTABLE})
endforeach()

# Tests that require separate definition (expected to fail, comparing
# output, etc)
set(TEST_EXECUTABLES
  test_case
  test_display_hash
  test_delete_hash
  test_feat
  test_fe
  test_fe_float32
  test_pitch
  )
foreach(TEST_EXECUTABLE ${TEST_EXECUTABLES})
  add_executable(${TEST_EXECUTABLE} EXCLUDE_FROM_ALL ${TEST_EXECUTABLE}.c)
  target_link_libraries(${TEST_EXECUTABLE} soundswallower)
  target_include_directories(
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_SOURCE_DIR}/src
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_BINARY_DIR}
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_dependencies(check ${TEST_EXECUTABLE})
endforeach()
# CTest won't find test_case here unless we use the named-argument
# format (see this bug, mysteriously closed with no explanation:
# https://gitlab.kitware.com/cmake/cmake/-/issues/21099)
add_test(NAME lcase1 COMMAND test_case lcase 5 3)
add_test(NAME lcase2 COMMAND test_case lcase 1 1)
add_test(NAME lcase3 COMMAND test_case lcase)
add_test(NAME ucase1 COMMAND test_case ucase 2 4)
add_test(NAME ucase2 COMMAND test_case ucase 1 1)
add_test(NAME ucase3 COMMAND test_case ucase)
add_test(NAME strcmp1 COMMAND test_case strcmp_nocase 2 3)
add_test(NAME strcmp2 COMMAND test_case strcmp_nocase 1 1)
add_test(NAME strcmp3 COMMAND test_case strcmp_nocase)

set(TEST_SCRIPTS
  _hash_delete1.test
  _hash_delete2.test
  _hash_delete3.test
  _hash_delete4.test
  _hash_delete5.test
  _test_feat.test
  _test_fe.test
  _test_fe_float32.test
  _test_pitch.test
)
foreach(TEST ${TEST_SCRIPTS})
  add_test(NAME ${TEST} COMMAND ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/${TEST})
  set_property(TEST ${TEST} PROPERTY ENVIRONMENT CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR})
endforeach()

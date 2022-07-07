configure_file(test_macros.h.in test_macros.h)
set(TESTS
  test_acmod
  test_acmod_grow
  test_add_words
  test_bitvec
  test_case
  test_err
  test_ckd_alloc_catch
  test_ckd_alloc_fail
  test_cmdln_parse_r
  test_delete_hash
  test_dict2pid
  test_dict
  test_display_hash
  test_feat
  test_feat_fe
  test_feat_live
  test_fe
  test_fe_float32
  test_filename
  test_fsg
  test_hash_iter
  test_heap
  test_jsgf
  test_listelem_alloc
  test_mdef
  test_log_shifted
  test_pitch
  test_ptm_mgau
  test_s3file
  test_strfuncs
  test_subvq)
foreach(TEST_EXECUTABLE ${TESTS})
  add_executable(${TEST_EXECUTABLE} ${TEST_EXECUTABLE}.c)
  target_link_libraries(${TEST_EXECUTABLE} soundswallower)
  target_include_directories(
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_SOURCE_DIR}/src
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_BINARY_DIR}
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_test(NAME ${TEST_EXECUTABLE} COMMAND ${TEST_EXECUTABLE})
endforeach()

# weird case-changing tests, not sure where they came from
add_test(lcase1 test_case lcase 5 3)
add_test(lcase2 test_case lcase 1 1)
add_test(lcase3 test_case lcase)
add_test(ucase1 test_case ucase 2 4)
add_test(ucase2 test_case ucase 1 1)
add_test(lcase3 test_case ucase)
add_test(strcmp1 test_case strcmp_nocase 2 3)
add_test(strcmp2 test_case strcmp_nocase 1 1)
add_test(strcmp3 test_case strcmp_nocase)

# string tests
add_test(string_join test_strfuncs string_join)
add_test(string_trim test_strfuncs string_trim)

# utility tests
add_test(bitvec test_bitvec)
add_test(ckd_alloc_catch test_ckd_alloc_catch)
add_test(ckd_alloc_fail test_ckd_alloc_fail)
set_tests_properties(ckd_alloc_fail PROPERTIES WILL_FAIL TRUE)
add_test(filename test_filename)
add_test(hash_iter test_hash_iter)
add_test(heap test_heap)
# invert requires output FIXME
add_test(listelem_alloc test_listelem_alloc)
# solve requires output FIXME

# command-line tests require output FIXME

# delete and display hash tests require output FIXME

# determinant test requires output FIXME


# component tests
add_test(acmod test_acmod)
add_test(acmod_grow test_acmod_grow)
add_test(dict2pid test_dict2pid)
add_test(dict test_dict)
add_test(fe test_fe)
add_test(feat_fe test_feat_fe)
add_test(feat_live test_feat_live)
# test_feat reqiures output FIXME
add_test(ptm_mgau test_ptm_mgau)
add_test(subvq test_subvq)

# functional-ish tests
add_test(jsgf test_jsgf)
add_test(fsg test_fsg)
add_test(pitch test_pitch)

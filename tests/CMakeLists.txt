configure_file(test_macros.h.in test_macros.h)
set(TESTS
  test_acmod.c
  test_acmod_grow.c
  test_bit_encode.c
  test_bitvec.c
  test_build_directory.c
  test_case.c
  test_ckd_alloc_catch.c
  test_ckd_alloc_fail.c
  test_cmdln_parse_r.c
  test_delete_hash.c
  test_dict2pid.c
  test_dict.c
  test_display_hash.c
  test_feat.c
  test_feat_fe.c
  test_feat_live.c
  test_fe.c
  test_fe_float32.c
  test_filename.c
  test_fsg.c
  test_hash_iter.c
  test_heap.c
  test_jsgf.c
  test_lineiter.c
  test_listelem_alloc.c
  test_log_int16.c
  test_log_int8.c
  test_log_shifted.c
  test_pitch.c
  test_ptm_mgau.c
  test_readfile.c
  test_strfuncs.c
  test_subvq.c)
foreach(TEST_SOURCE ${TESTS})
  get_filename_component(TEST_EXECUTABLE ${TEST_SOURCE} NAME_WE)
  add_executable(${TEST_EXECUTABLE} ${TEST_SOURCE})
  target_link_libraries(${TEST_EXECUTABLE} soundswallower)
  target_include_directories(${TEST_EXECUTABLE} PRIVATE
    ${CMAKE_BINARY_DIR} # for config.h
    ${CMAKE_CURRENT_BINARY_DIR} # for test_macros.h
    ${PROJECT_SOURCE_DIR}/src)
    add_custom_command(TARGET ${TEST_EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:soundswallower>
        $<TARGET_FILE_DIR:${TEST_EXECUTABLE}>)
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
add_test(nextword test_strfuncs nextword)
add_test(str2words test_strfuncs str2words)
add_test(string_join test_strfuncs string_join)
add_test(string_trim test_strfuncs string_trim)
add_test(fread_line test_strfuncs fread_line)

# utility tests
add_test(bit_encode test_bit_encode)
add_test(bitvec test_bitvec)
add_test(ckd_alloc_catch test_ckd_alloc_catch)
add_test(ckd_alloc_fail test_ckd_alloc_fail)
set_tests_properties(ckd_alloc_fail PROPERTIES WILL_FAIL TRUE)
add_test(build_directory test_build_directory)
add_test(filename test_filename)
add_test(hash_iter test_hash_iter)
add_test(heap test_heap)
# invert requires output FIXME
add_test(lineiter test_lineiter)
add_test(listelem_alloc test_listelem_alloc)
add_test(log_int16 test_log_int16)
add_test(log_int8 test_log_int8)
add_test(log_shifted test_log_shifted)
add_test(readfile test_readfile)
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

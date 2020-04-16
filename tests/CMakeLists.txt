add_compile_definitions(TESTDATADIR=\"${CMAKE_CURRENT_SOURCE_DIR}/data\")
set(TESTS
  test_acmod.c
  test_acmod_grow.c
  test_alignment.c
  test_allphone.c
  test_atof.c
  test_bitarr.c
  test_bit_encode.c
  test_bitvec.c
  test_build_directory.c
  test_case.c
  test_ckd_alloc_abort.c
  test_ckd_alloc_catch.c
  test_ckd_alloc_fail.c
  test_cmdln.c
  test_cmdln_multiple.c
  test_cmdln_parse_r.c
  test_delete_hash.c
  test_determinant.c
  test_dict2pid.c
  test_dict.c
  test_display_hash.c
  test_event.c
  test_feat.c
  test_feat_fe.c
  test_feat_live.c
  test_fe.c
  test_filename.c
  test_fopen.c
  test_fsg.c
  test_fwdflat.c
  test_fwdtree_bestpath.c
  test_fwdtree.c
  test_hash_iter.c
  test_heap.c
  test_init.c
  test_invert.c
  test_jsgf.c
  test_keyphrase.c
  test_lattice.c
  test_lineiter.c
  test_listelem_alloc.c
  test_lm_read.c
  test_log_int16.c
  test_log_int8.c
  test_log_shifted.c
  test_mllr.c
  test_msgq.c
  test_nbest.c
  test_pitch.c
  test_posterior.c
  test_ps.c
  test_ptm_mgau.c
  test_readfile.c
  test_reinit.c
  test_senfh.c
  test_set_search.c
  test_simple.c
  test_solve.c
  test_state_align.c
  test_strfuncs.c
  test_subvq.c
  test_thread.c)
foreach(TEST_SOURCE ${TESTS})
  get_filename_component(TEST_EXECUTABLE ${TEST_SOURCE} NAME_WE)
  add_executable(${TEST_EXECUTABLE} ${TEST_SOURCE})
  target_link_libraries(${TEST_EXECUTABLE} soundswallower)
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

set(SOURCES
  acmod.c
  bin_mdef.c
  bio.c
  bitarr.c
  bitvec.c
  blkarray_list.c
  case.c
  ckd_alloc.c
  cmd_ln.c
  cmn.c
  cmn_live.c
  dict2pid.c
  dict.c
  err.c
  errno.c
  feat.c
  fe_interface.c
  fe_noise.c
  fe_prespch_buf.c
  fe_sigproc.c
  fe_warp_affine.c
  fe_warp.c
  fe_warp_inverse_linear.c
  fe_warp_piecewise_linear.c
  filename.c
  fsg_history.c
  fsg_lextree.c
  fsg_model.c
  fsg_search.c
  genrand.c
  glist.c
  hash_table.c
  heap.c
  hmm.c
  jsgf.c
  jsgf_parser.c
  jsgf_scanner.c
  kws_search.c
  kws_detections.c
  listelem_alloc.c
  logmath.c
  matrix.c
  mdef.c
  mmio.c
  ms_gauden.c
  ms_mgau.c
  ms_senone.c
  phone_loop_search.c
  pio.c
  pocketsphinx.c
  priority_queue.c
  profile.c
  ps_alignment.c
  ps_lattice.c
  ps_mllr.c
  ptm_mgau.c
  s2_semi_mgau.c
  state_align_search.c
  strfuncs.c
  tmat.c
  vector.c
  yin.c
  )

if(LAPACK_LITE)
  set(LAPACK_SOURCES
    slapack_lite.c
    blas_lite.c
    f2c_lite.c
    slamch.c)
endif()

add_library(soundswallower ${SOURCES} ${LAPACK_SOURCES})
target_include_directories(soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/src
  soundswallower PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  soundswallower PUBLIC ${PROJECT_SOURCE_DIR}/include)
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(soundswallower PUBLIC ${MATH_LIBRARY})
endif()
if(HAVE_LIBLAPACK)
  target_link_libraries(soundswallower PUBLIC lapack)
endif()

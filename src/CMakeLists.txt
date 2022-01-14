set(SOURCES
  acmod.c
  bin_mdef.c
  bio.c
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
  feat.c
  fe_interface.c
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
  listelem_alloc.c
  logmath.c
  mdef.c
  mmio.c
  ms_gauden.c
  ms_mgau.c
  ms_senone.c
  pio.c
  pocketsphinx.c
  profile.c
  ps_lattice.c
  ps_mllr.c
  strfuncs.c
  tmat.c
  vector.c
  yin.c
  )

if(WITH_PTM_MGAU)
  set(PTM_SOURCES ptm_mgau.c)
endif()

if(WITH_S2_SEMI_MGAU)
  set(S2_SEMI_SOURCES s2_semi_mgau.c)
endif()

add_library(soundswallower ${SOURCES} ${LAPACK_SOURCES}
  ${PTM_SOURCES} ${S2_SEMI_SOURCES})
set_property(TARGET soundswallower PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
target_include_directories(soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/src
  soundswallower PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  soundswallower PUBLIC ${PROJECT_SOURCE_DIR}/include)
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(soundswallower PUBLIC ${MATH_LIBRARY})
endif()

cmake_minimum_required(VERSION 3.10)
include(CheckTypeSize)
include(CheckSymbolExists)

project(soundswallower VERSION 0.1
  DESCRIPTION "An even smaller speech recognizer")

CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(sys/stat.h HAVE_SYS_STAT_H)
CHECK_SYMBOL_EXISTS(snprintf stdio.h HAVE_SNPRINTF)
CHECK_TYPE_SIZE("long long" LONG_LONG)

configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SOURCES
  src/acmod.c
  src/bin_mdef.c
  src/bio.c
  src/bitarr.c
  src/bitvec.c
  src/blas_lite.c
  src/blkarray_list.c
  src/case.c
  src/ckd_alloc.c
  src/cmd_ln.c
  src/cmn.c
  src/cmn_live.c
  src/dict2pid.c
  src/dict.c
  src/dtoa.c
  src/err.c
  src/errno.c
  src/f2c_lite.c
  src/feat.c
  src/fe_interface.c
  src/fe_noise.c
  src/fe_prespch_buf.c
  src/fe_sigproc.c
  src/fe_warp_affine.c
  src/fe_warp.c
  src/fe_warp_inverse_linear.c
  src/fe_warp_piecewise_linear.c
  src/filename.c
  src/fsg_history.c
  src/fsg_lextree.c
  src/fsg_model.c
  src/fsg_search.c
  src/genrand.c
  src/glist.c
  src/hash_table.c
  src/heap.c
  src/hmm.c
  src/jsgf.c
  src/jsgf_parser.c
  src/jsgf_scanner.c
  src/kws_search.c
  src/kws_detections.c
  src/listelem_alloc.c
  src/logmath.c
  src/matrix.c
  src/mdef.c
  src/mmio.c
  src/ms_gauden.c
  src/ms_mgau.c
  src/ms_senone.c
  src/phone_loop_search.c
  src/pio.c
  src/pocketsphinx.c
  src/priority_queue.c
  src/profile.c
  src/ps_alignment.c
  src/ps_lattice.c
  src/ps_mllr.c
  src/ptm_mgau.c
  src/s2_semi_mgau.c
  src/slamch.c
  src/slapack_lite.c
  src/state_align_search.c
  src/strfuncs.c
  src/tmat.c
  src/vector.c
  src/yin.c
  )

add_library(soundswallower ${SOURCES})
target_include_directories(soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/src
  soundswallower PUBLIC ${PROJECT_SOURCE_DIR}/include)
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(soundswallower PUBLIC ${MATH_LIBRARY})
endif()

add_executable(ssbatch batch.c)
target_link_libraries(ssbatch soundswallower)
target_include_directories(ssbatch PRIVATE ${PROJECT_SOURCE_DIR}/src)

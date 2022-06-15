cmake_minimum_required(VERSION 3.13)
include(CheckTypeSize)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(CMakePrintHelpers)
include(TestBigEndian)

project(soundswallower VERSION 0.2.3
  DESCRIPTION "An even smaller speech recognizer")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
  enable_testing()
endif()

CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(sys/stat.h HAVE_SYS_STAT_H)
CHECK_SYMBOL_EXISTS(snprintf stdio.h HAVE_SNPRINTF)

# Testing endianness is stupidly hard with CMake
if(EMSCRIPTEN)
  # FIXME: and doesn't work at all with emscripten, maybe it's just
  # always little-endian?
  set(WORDS_BIGENDIAN 0)
else()
  test_big_endian(WORDS_BIGENDIAN)
endif()

configure_file(config.h.in config.h)
add_definitions(-DHAVE_CONFIG_H)

# Did I mention that CMake is really stupid
if(MSVC)
  add_compile_options(/W3) # /WX)
else()
  add_compile_options(-Wall) # -Wextra -Wpedantic -Werror)
endif()

# For MSVC/vscode in Windows: don't warn about functions only MS considers deprecated
add_definitions(-D_CRT_SECURE_NO_WARINIGS)
add_definitions(-D_CRT_SECURE_NO_DEPRECATE)

# Core library sources (only built to a library in C)
set(SOUNDSWALLOWER_SOURCES
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
  lda.c
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
  ptm_mgau.c
  s2_semi_mgau.c
  strfuncs.c
  tmat.c
  vector.c
  yin.c
  )
list(TRANSFORM
  SOUNDSWALLOWER_SOURCES
  PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)

if(EMSCRIPTEN)
  # Specific configuration for JavaScript build
  add_subdirectory(js)
else() # not EMSCRIPTEN
  # Normal build and installation
  option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
  add_library(soundswallower ${SOUNDSWALLOWER_SOURCES})
  set_property(TARGET soundswallower PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
  target_include_directories(soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/src
    soundswallower PRIVATE ${CMAKE_BINARY_DIR} # for config.h
    soundswallower PUBLIC ${PROJECT_SOURCE_DIR}/include)
  find_library(MATH_LIBRARY m)
  if(MATH_LIBRARY)
    target_link_libraries(soundswallower PUBLIC ${MATH_LIBRARY})
  endif()

  if(SKBUILD)
    # Specific configuration for Python extension build
    add_subdirectory(py)
  else() # not SKBUILD
    add_subdirectory(include/soundswallower)
    if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
      add_subdirectory(tests)
    endif()
    install(TARGETS soundswallower DESTINATION lib)
  endif() # not SKBUILD
endif() # not EMSCRIPTEN

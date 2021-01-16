cmake_minimum_required(VERSION 3.13)
include(CheckTypeSize)
include(CheckSymbolExists)
include(CheckLibraryExists)

project(soundswallower VERSION 0.1.2
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

option(WITH_LAPACK "Enable linear algebra functions" OFF)
if(WITH_LAPACK)
  CHECK_LIBRARY_EXISTS(lapack sgesv_ "" HAVE_LIBLAPACK)
  if(NOT HAVE_LIBLAPACK)
    set(LAPACK_LITE 1)
  endif()
endif()
option(WITH_PTM_MGAU "Enable special phonetically-tied mixture code" ON)
option(WITH_S2_SEMI_MGAU "Enable special semi-continuous GMM code" OFF)

configure_file(config.h.in config.h)
add_definitions(-DHAVE_CONFIG_H)

# For MSVC/vscode in Windows: don't warn about functions only MS considers deprecated
add_definitions(-D_CRT_SECURE_NO_WARINIGS)
add_definitions(-D_CRT_SECURE_NO_DEPRECATE)

# Always build the core library source
add_subdirectory(src)

if(SKBUILD)
  # Specific configuration for Python extension build
  add_subdirectory(py)
elseif(EMSCRIPTEN)
  # Specific configuration for JavaScript build
  add_subdirectory(js)
else()
  # Normal build and installation
  option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
  add_subdirectory(include/soundswallower)
  if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
  endif()
  install(TARGETS soundswallower DESTINATION lib)
endif()

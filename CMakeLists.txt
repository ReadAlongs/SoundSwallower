cmake_minimum_required(VERSION 3.13)
include(CheckTypeSize)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(TestBigEndian)

project(soundswallower VERSION 0.2
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

cmake_minimum_required(VERSION 3.13)
include(CheckTypeSize)
include(CheckSymbolExists)

project(soundswallower VERSION 0.1
  DESCRIPTION "An even smaller speech recognizer")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
  enable_testing()
endif()

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(sys/stat.h HAVE_SYS_STAT_H)
CHECK_SYMBOL_EXISTS(snprintf stdio.h HAVE_SNPRINTF)

configure_file(config.h.in config.h)

add_subdirectory(src)
add_subdirectory(swig)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
  add_subdirectory(tests)
endif()

add_executable(ssbatch batch.c)
target_link_libraries(ssbatch soundswallower)
target_include_directories(ssbatch PRIVATE ${PROJECT_SOURCE_DIR}/src
  ssbatch PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )

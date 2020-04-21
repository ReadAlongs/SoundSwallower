cmake_minimum_required(VERSION 3.13)

find_package(SWIG)
find_package(PythonExtensions REQUIRED)
find_package(Python COMPONENTS Interpreter Development)
include(UseSWIG)
include(CheckTypeSize)
include(CheckSymbolExists)

CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(sys/stat.h HAVE_SYS_STAT_H)
CHECK_SYMBOL_EXISTS(snprintf stdio.h HAVE_SNPRINTF)
configure_file(../config.h.in config.h)

add_subdirectory(../src clib)
set_property(TARGET soundswallower PROPERTY POSITION_INDEPENDENT_CODE on)

set(SWIG_SOURCES
  ../swig/soundswallower.i)
swig_add_library(sspy
  LANGUAGE python
  SOURCES ${SWIG_SOURCES})
set_property(TARGET sspy PROPERTY OUTPUT_NAME soundswallower)
target_link_libraries(sspy soundswallower)
target_include_directories(
  soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/../src
  soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/../include
  soundswallower PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )
target_include_directories(
  sspy PRIVATE ${PYTHON_INCLUDE_DIR}
  sspy PRIVATE ${PROJECT_SOURCE_DIR}/../src
  sspy PRIVATE ${PROJECT_SOURCE_DIR}/../include
  sspy PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )
python_extension_module(_soundswallower)
# FIXME: Not really sure why this is necessary?!?!
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/soundswallower.py DESTINATION soundswallower)
install(TARGETS sspy LIBRARY DESTINATION soundswallower)

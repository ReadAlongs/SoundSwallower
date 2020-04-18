cmake_minimum_required(VERSION 3.13)

find_package(SWIG)
find_package(Python COMPONENTS Interpreter Development)
include(UseSWIG)

set(SWIG_SOURCES
  ../soundswallower.i)

swig_add_library(sspy
  LANGUAGE python
  SOURCES ${SWIG_SOURCES})
set_property(TARGET sspy PROPERTY OUTPUT_NAME soundswallower)
target_link_libraries(sspy soundswallower)
target_include_directories(
  sspy PRIVATE ..
  sspy PRIVATE ${Python_INCLUDE_DIRS}
  sspy PRIVATE ${PROJECT_SOURCE_DIR}/src
  sspy PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )

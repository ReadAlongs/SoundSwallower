cmake_minimum_required(VERSION 3.13)
include(CheckTypeSize)
include(CheckSymbolExists)
include(CheckLibraryExists)

project(soundswallower VERSION 0.1.1
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

option(WITH_LAPACK "Enable linear algebra functions" OFF)
if(WITH_LAPACK)
  CHECK_LIBRARY_EXISTS(lapack sgesv_ "" HAVE_LIBLAPACK)
  if(NOT HAVE_LIBLAPACK)
    set(LAPACK_LITE 1)
  endif()
endif()

configure_file(config.h.in config.h)
add_definitions(-DHAVE_CONFIG_H)

add_subdirectory(src)
add_subdirectory(include/soundswallower)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
  add_subdirectory(tests)
endif()

add_executable(ssbatch batch.c)
target_link_libraries(ssbatch soundswallower)
target_include_directories(
  ssbatch PRIVATE ${PROJECT_SOURCE_DIR}/src
  ssbatch PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )

install(TARGETS soundswallower DESTINATION lib)
install(TARGETS ssbatch DESTINATION bin)

# Specific configuration for Python extension build
if(SKBUILD)
  find_package(PythonExtensions REQUIRED)
  find_package(Python COMPONENTS Interpreter Development)

  set(BUILD_SHARED_LIBS OFF)
  set_property(TARGET soundswallower PROPERTY POSITION_INDEPENDENT_CODE on)

  add_library(_soundswallower MODULE pyext/soundswallower_wrap.c)
  set_property(TARGET _soundswallower PROPERTY PREFIX "")
  target_link_libraries(_soundswallower soundswallower)
  target_include_directories(
    _soundswallower PRIVATE ${PYTHON_INCLUDE_DIR}
    _soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/src
    _soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/include
    _soundswallower PRIVATE ${CMAKE_BINARY_DIR} # for config.h
    )

  set(MODELDIR ${PROJECT_SOURCE_DIR}/model)
  install(DIRECTORY ${MODELDIR} DESTINATION soundswallower)
endif()

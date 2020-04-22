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

# Always build the core library source
add_subdirectory(src)

if(SKBUILD)
  # Specific configuration for Python extension build
  find_package(PythonExtensions REQUIRED)
  find_package(Python COMPONENTS Interpreter Development)

  set_property(TARGET soundswallower PROPERTY POSITION_INDEPENDENT_CODE on)

  add_library(_soundswallower MODULE soundswallower_wrap.c)
  set_property(TARGET _soundswallower PROPERTY PREFIX "")
  target_link_libraries(_soundswallower soundswallower)
  target_include_directories(
    _soundswallower PRIVATE ${PYTHON_INCLUDE_DIR}
    _soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/src
    _soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/include
    _soundswallower PRIVATE ${CMAKE_BINARY_DIR} # for config.h
    )
  python_extension_module(_soundswallower)
  install(TARGETS _soundswallower LIBRARY DESTINATION soundswallower)
  
  set(MODELDIR ${PROJECT_SOURCE_DIR}/model)
  install(DIRECTORY ${MODELDIR} DESTINATION soundswallower)
else()
  # Normal build and installation
  option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
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
endif()

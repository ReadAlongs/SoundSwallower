cmake_minimum_required(VERSION 3.13)

find_package(PythonExtensions REQUIRED)
find_package(Python COMPONENTS Interpreter Development)
include(CheckTypeSize)
include(CheckSymbolExists)

CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(sys/stat.h HAVE_SYS_STAT_H)
CHECK_SYMBOL_EXISTS(snprintf stdio.h HAVE_SNPRINTF)
configure_file(../config.h.in config.h)

add_subdirectory(../src clib)
set_property(TARGET soundswallower PROPERTY POSITION_INDEPENDENT_CODE on)

add_library(_soundswallower MODULE soundswallower_wrap.c)
set_property(TARGET _soundswallower PROPERTY PREFIX "")
target_link_libraries(_soundswallower soundswallower)
target_include_directories(
  soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/../src
  soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/../include
  soundswallower PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )
target_include_directories(
  _soundswallower PRIVATE ${PYTHON_INCLUDE_DIR}
  _soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/../src
  _soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/../include
  _soundswallower PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )
python_extension_module(_soundswallower)
install(TARGETS _soundswallower LIBRARY DESTINATION soundswallower)

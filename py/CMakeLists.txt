find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
find_program(CYTHON "cython")

set_property(TARGET soundswallower PROPERTY POSITION_INDEPENDENT_CODE on)

add_custom_command(
  OUTPUT _soundswallower.c
  DEPENDS _soundswallower.pyx
  VERBATIM
  COMMAND "${CYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/_soundswallower.pyx"
  --output-file "${CMAKE_CURRENT_BINARY_DIR}/_soundswallower.c")

python_add_library(_soundswallower MODULE
  "${CMAKE_CURRENT_BINARY_DIR}/_soundswallower.c" WITH_SOABI)
target_link_libraries(_soundswallower PRIVATE soundswallower)
target_include_directories(
  _soundswallower PRIVATE ${PYTHON_INCLUDE_DIR}
  _soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/src
  _soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/include
  _soundswallower PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )
install(TARGETS _soundswallower LIBRARY DESTINATION soundswallower)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/model DESTINATION soundswallower)

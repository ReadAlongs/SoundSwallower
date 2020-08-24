find_package(PythonExtensions REQUIRED)
find_package(Python COMPONENTS Interpreter Development)
find_package(Cython)

set_property(TARGET soundswallower PROPERTY POSITION_INDEPENDENT_CODE on)

add_cython_target(_soundswallower soundswallower.pyx)
add_library(_soundswallower MODULE ${_soundswallower})
set_property(TARGET _soundswallower PROPERTY PREFIX "")
target_link_libraries(_soundswallower soundswallower)
target_include_directories(
  _soundswallower PRIVATE ${PYTHON_INCLUDE_DIR}
  _soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/src
  _soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/include
  _soundswallower PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )
python_extension_module(_soundswallower)
# Need to install the library in the same place as the module or
# scikit-build, which is not very smart, gets very confused.
install(TARGETS _soundswallower LIBRARY DESTINATION py/soundswallower)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/model DESTINATION py/soundswallower)

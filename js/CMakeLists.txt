add_executable(ssjs soundswallower.c)
set_property(TARGET ssjs PROPERTY OUTPUT_NAME soundswallower)
set_property(TARGET ssjs PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TARGET ssjs PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TARGET ssjs PROPERTY LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Have to use --whole-archive due to
# https://github.com/emscripten-core/emscripten/issues/6233 (and see
# also
# https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html#binding-libraries)
target_link_libraries(ssjs -Wl,--whole-archive soundswallower -Wl,--no-whole-archive)
target_compile_options(soundswallower PRIVATE -Oz)
target_compile_options(ssjs PRIVATE -Oz)
# Cannot, will not set lists of functions directly on the command line
#
# https://github.com/emscripten-core/emscripten/pull/17403
# https://stackoverflow.com/questions/68775616/how-not-to-have-dollar-sign-in-target-link-options-mangled
# https://discourse.cmake.org/t/how-not-to-have-dollar-sign-in-target-link-options-mangled/3939/3
target_link_options(ssjs PRIVATE
  -Oz
  -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=@${CMAKE_SOURCE_DIR}/js/library_funcs.txt
  -sEXPORTED_FUNCTIONS=@${CMAKE_SOURCE_DIR}/js/exported_functions.txt
  -sFILESYSTEM=0 -sMODULARIZE=1 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432)
# See
# https://github.com/emscripten-core/emscripten/blob/main/cmake/Modules/Platform/Emscripten.cmake
# ...sure would be nice if this were documented
em_link_post_js(ssjs api.js)
target_include_directories(
  ssjs PRIVATE ${PYTHON_INCLUDE_DIR}
  ssjs PRIVATE ${PROJECT_SOURCE_DIR}/src
  ssjs PRIVATE ${PROJECT_SOURCE_DIR}/include
  ssjs PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )
# Copy model into build directory
add_custom_command(OUTPUT model
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/model
  ${CMAKE_BINARY_DIR}/model)
add_custom_target(copy-models ALL DEPENDS model)
# Copy test data into build directory
add_custom_command(OUTPUT testdata
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/tests/data
  ${CMAKE_BINARY_DIR}/testdata)
add_custom_target(copy-test ALL DEPENDS testdata)
# Copy test and package files into build directory
set(JSFILES
  .npmignore
  package.json
  README.md
  server.py
  test_node.js
  test_typescript.ts
  test_web.html
  soundswallower.d.ts
  tsconfig.json
  )
list(TRANSFORM JSFILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
add_custom_target(copy-js ALL
  COMMAND ${CMAKE_COMMAND} -E copy ${JSFILES} ${CMAKE_BINARY_DIR})

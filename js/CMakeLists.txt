add_executable(ssjs soundswallower.c)
set_property(TARGET ssjs PROPERTY OUTPUT_NAME soundswallower)
set_property(TARGET ssjs PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Have to use --whole-archive due to
# https://github.com/emscripten-core/emscripten/issues/6233 (and see
# also
# https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html#binding-libraries)
target_link_libraries(ssjs -Wl,--whole-archive soundswallower -Wl,--no-whole-archive)
target_compile_options(soundswallower PRIVATE -Oz)
target_compile_options(ssjs PRIVATE -Oz)
target_link_options(ssjs PRIVATE
  -Oz -sFORCE_FILESYSTEM=1 -sMODULARIZE=1
  -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432
  $<$<CONFIG:DEBUG>:-gsource-map --source-map-base=/>)
# See
# https://github.com/emscripten-core/emscripten/blob/main/cmake/Modules/Platform/Emscripten.cmake
# ...sure would be nice if this were documented
em_link_pre_js(ssjs pre_soundswallower.js)
target_include_directories(
  ssjs PRIVATE ${PYTHON_INCLUDE_DIR}
  ssjs PRIVATE ${PROJECT_SOURCE_DIR}/src
  ssjs PRIVATE ${PROJECT_SOURCE_DIR}/include
  ssjs PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )
# Copy model into this directory
add_custom_command(OUTPUT model
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/model
  ${CMAKE_BINARY_DIR}/model)
add_custom_target(copy-models ALL DEPENDS model)

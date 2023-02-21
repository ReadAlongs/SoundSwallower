add_executable(soundswallower.web soundswallower.c)
add_executable(soundswallower.node soundswallower.c)
add_executable(soundswallower.jsonly soundswallower.c)
add_executable(soundswallower.bundle soundswallower.c)
foreach(JSBUILD soundswallower.bundle soundswallower.jsonly soundswallower.web soundswallower.node)
  set_property(TARGET ${JSBUILD} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  set_property(TARGET ${JSBUILD} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  set_property(TARGET ${JSBUILD} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  # Have to use --whole-archive due to
  # https://github.com/emscripten-core/emscripten/issues/6233 (and see
  # also
  # https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html#binding-libraries)
  target_link_libraries(${JSBUILD} -Wl,--whole-archive soundswallower -Wl,--no-whole-archive)
  target_compile_options(soundswallower PRIVATE -Oz -sSUPPORT_LONGJMP=0 -sSTRICT=1)
  target_compile_options(${JSBUILD} PRIVATE -Oz -sSUPPORT_LONGJMP=0 -sSTRICT=1)
  target_include_directories(
    ${JSBUILD} PRIVATE ${PYTHON_INCLUDE_DIR}
    ${JSBUILD} PRIVATE ${PROJECT_SOURCE_DIR}/src
    ${JSBUILD} PRIVATE ${PROJECT_SOURCE_DIR}/include
    ${JSBUILD} PRIVATE ${CMAKE_BINARY_DIR} # for config.h
    )
endforeach()
set_property(TARGET soundswallower.jsonly PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/jsonly)
set_property(TARGET soundswallower.jsonly PROPERTY RUNTIME_OUTPUT_NAME index)
# Cannot, will not set lists of functions directly on the command line
#
# https://github.com/emscripten-core/emscripten/pull/17403
# https://stackoverflow.com/questions/68775616/how-not-to-have-dollar-sign-in-target-link-options-mangled
# https://discourse.cmake.org/t/how-not-to-have-dollar-sign-in-target-link-options-mangled/3939/3
target_link_options(soundswallower.web PRIVATE
  @${CMAKE_SOURCE_DIR}/js/linker_options.txt
  --extern-pre-js ${CMAKE_SOURCE_DIR}/js/api-web-pre.js
  -sENVIRONMENT=web -sWASM=1
  -sMODULARIZE=1 -sEXPORT_ES6=1
  -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=@${CMAKE_SOURCE_DIR}/js/library_funcs.txt
  -sEXPORTED_FUNCTIONS=@${CMAKE_SOURCE_DIR}/js/exported_functions.txt)
# Sadly, it seems they cannot share the same wasm file, for no good reason
target_link_options(soundswallower.node PRIVATE
  @${CMAKE_SOURCE_DIR}/js/linker_options.txt
  --extern-pre-js ${CMAKE_SOURCE_DIR}/js/api-node-pre.js
  -sENVIRONMENT=node -sWASM=1
  -sMODULARIZE=1 -sEXPORT_ES6=1
  -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=@${CMAKE_SOURCE_DIR}/js/library_funcs.txt
  -sEXPORTED_FUNCTIONS=@${CMAKE_SOURCE_DIR}/js/exported_functions.txt)
# -sWASM=2 does not seem to work so make an explicit standalone, JS-only ESM
target_link_options(soundswallower.jsonly PRIVATE
  @${CMAKE_SOURCE_DIR}/js/linker_options.txt
  --extern-pre-js ${CMAKE_SOURCE_DIR}/js/api-web-pre.js
  -sENVIRONMENT=web -sWASM=0
  -sMODULARIZE=1 -sEXPORT_ES6=1
  -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=@${CMAKE_SOURCE_DIR}/js/library_funcs.txt
  -sEXPORTED_FUNCTIONS=@${CMAKE_SOURCE_DIR}/js/exported_functions.txt)
# finally, an specific version for webpack to make a UMD, which we
# manually make into an ESM because EXPORT_ES6 does many things we do
# not want (which it shouldn't do with -sWASM=0, but whatever)
target_link_options(soundswallower.bundle PRIVATE
  @${CMAKE_SOURCE_DIR}/js/linker_options.txt
  --extern-pre-js ${CMAKE_SOURCE_DIR}/js/api-web-pre.js
  --extern-post-js ${CMAKE_SOURCE_DIR}/js/api-bundle-post.js
  -sENVIRONMENT=web -sWASM=0
  -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=@${CMAKE_SOURCE_DIR}/js/library_funcs.txt
  -sEXPORTED_FUNCTIONS=@${CMAKE_SOURCE_DIR}/js/exported_functions.txt)
# See
# https://github.com/emscripten-core/emscripten/blob/main/cmake/Modules/Platform/Emscripten.cmake
# ...sure would be nice if this were documented
em_link_pre_js(soundswallower.jsonly api-jsonly-pre.js)
em_link_post_js(soundswallower.jsonly api-web.js api.js)
em_link_post_js(soundswallower.bundle api-web.js api.js)
em_link_post_js(soundswallower.web api-web.js api.js)
em_link_post_js(soundswallower.node api-node.js api.js)

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
  test_typescript.ts
  test_web.html
  soundswallower.spec.js
  # DO NOT CALL THIS soundswallower.d.ts because otherwise
  # jsonly/index.d.ts cannot be found by broken typescript
  index.d.ts
  tests.js
  tsconfig.json
  webpack.config.cjs
  )
list(TRANSFORM JSFILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
add_custom_target(copy-js ALL
  COMMAND ${CMAKE_COMMAND} -E copy ${JSFILES} ${CMAKE_BINARY_DIR})
# Copy types into subdirectory to satisfy broken typescript
add_custom_target(copy-ts-bork-bork-bork ALL
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_SOURCE_DIR}/index.d.ts ${CMAKE_BINARY_DIR}/jsonly/index.d.ts)

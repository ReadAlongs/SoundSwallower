add_executable(ssjs psRecognizer.cpp)
set_property(TARGET ssjs PROPERTY OUTPUT_NAME soundswallower)
target_link_libraries(ssjs soundswallower)
target_compile_options(soundswallower PRIVATE -Oz)
target_compile_options(ssjs PRIVATE --bind -Oz)
target_link_options(ssjs PRIVATE --bind -Oz)
# FIXME: Need to optimize with recognizer.js to get only what we need
target_link_options(ssjs PRIVATE "SHELL:-s FORCE_FILESYSTEM=1")
target_link_options(ssjs PRIVATE "SHELL:-s TOTAL_MEMORY=100663296")
target_link_options(ssjs PRIVATE "SHELL:-s ERROR_ON_UNDEFINED_SYMBOLS=0")
target_link_options(ssjs PRIVATE "SHELL:--memory-init-file 0")
target_include_directories(
  ssjs PRIVATE ${PYTHON_INCLUDE_DIR}
  ssjs PRIVATE ${PROJECT_SOURCE_DIR}/src
  ssjs PRIVATE ${PROJECT_SOURCE_DIR}/include
  ssjs PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )

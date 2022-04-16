add_executable(ssjs psRecognizer.cpp)
set_property(TARGET ssjs PROPERTY OUTPUT_NAME soundswallower)
target_link_libraries(ssjs soundswallower)
target_compile_options(soundswallower PRIVATE -Oz)
target_compile_options(ssjs PRIVATE -Oz)
target_link_options(ssjs PRIVATE
  -lembind -Oz -sFORCE_FILESYSTEM=1
  -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432
  $<$<CONFIG:DEBUG>:-gsource-map --source-map-base=/js/>)
target_include_directories(
  ssjs PRIVATE ${PYTHON_INCLUDE_DIR}
  ssjs PRIVATE ${PROJECT_SOURCE_DIR}/src
  ssjs PRIVATE ${PROJECT_SOURCE_DIR}/include
  ssjs PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )

#############################################
#
# CMakeLists.txt for SoundSwallower
# adapted from PocketSphinx.js
#
#############################################

cmake_minimum_required(VERSION 3.13)

project(soundswallower.js VERSION 0.1)

include(CheckTypeSize)
include(CheckSymbolExists)

CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(sys/stat.h HAVE_SYS_STAT_H)
CHECK_SYMBOL_EXISTS(snprintf stdio.h HAVE_SNPRINTF)
configure_file(../config.h.in config.h)

option(WASM "Build to WebAssembly" ON)
SET_PROPERTY(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

add_definitions(-DHAVE_CONFIG_H)

if(HMM_EMBED)
  if ((NOT HMM_BASE) OR (NOT HMM_FOLDERS))
    set(HMM_BASE "${PROJECT_SOURCE_DIR}/../model/en-us")
    set(HMM_FOLDERS "en-us")
  endif()
  # Copying acoustic models into the build tree
  foreach(model ${HMM_FOLDERS})
    message("Copying ${HMM_BASE}/${model} to binary dir")
    file(COPY ${HMM_BASE}/${model} DESTINATION ${CMAKE_BINARY_DIR})
  endforeach()

  foreach(model ${HMM_FOLDERS})
    set(EMBED "${EMBED}" "--embed-file" "${model}" " ")
  endforeach()
endif(HMM_EMBED)

# Package dictionaries if needed
if(DICT_BASE AND DICT_FILES)
  foreach(dict ${DICT_FILES})
    message("Copying ${DICT_BASE}/${dict} to binary dir")
    file(COPY ${DICT_BASE}/${dict} DESTINATION ${CMAKE_BINARY_DIR})
    set(EMBED "${EMBED}" "--embed-file" "${dict}" " ")
  endforeach()
endif()

# Compile to WebAssembly?
if(WASM)
  separate_arguments(binaryen UNIX_COMMAND "-s WASM=1")
else(WASM)
  separate_arguments(binaryen UNIX_COMMAND "-s WASM=0")
endif(WASM)

# We are using the C++ binding utility of emscripten, this needs to be
# added to the compilation command
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Oz -DMODELDIR=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Oz --bind")


# Get the C library
add_subdirectory(../src src)
target_sources(
  soundswallower PRIVATE psRecognizer.cpp)
target_include_directories(
  soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/../src
  soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/../include
  soundswallower PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )

# Adding custom target for the JavaScript library.
add_custom_target(soundswallower.js ALL
  COMMAND ${CMAKE_C_COMPILER} -Oz ${binaryen}
  -s ERROR_ON_UNDEFINED_SYMBOLS=0
  -s TOTAL_MEMORY=100663296
  --bind --memory-init-file 0
  $<TARGET_FILE:soundswallower>
  -o soundswallower.js ${EMBED}
  DEPENDS soundswallower
  )

configure_file(pocketsphinxjs-config.h.in pocketsphinxjs-config.h)

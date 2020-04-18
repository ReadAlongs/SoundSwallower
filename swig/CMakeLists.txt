cmake_minimum_required(VERSION 3.13)

set(SWIG_SOURCES
  soundswallower.i)

set(SWIG_INCLUDES
  cmd_ln.i
  feat.i
  fe.i
  fsg_model.i
  iterators.i
  jsgf.i
  logmath.i
  typemaps.i
  ps_decoder.i
  ps_lattice.i)


option(BUILD_PYTHON "Build Python SWIG module" ON)
if(BUILD_PYTHON)
  add_subdirectory(python)
endif()

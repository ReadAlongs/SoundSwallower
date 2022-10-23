set(SOURCES
blkarray_list.c
mdef.c
cmn.c
fe_warp_inverse_linear.c
fe_sigproc.c
jsgf.c
ms_senone.c
ckd_alloc.c
logmath.c
bitvec.c
vector.c
s3file.c
jsgf_parser.c
genrand.c
hash_table.c
yin.c
strfuncs.c
fsg_lextree.c
acmod.c
lda.c
ms_gauden.c
ps_mllr.c
fe_interface.c
feat.c
cmn_live.c
dict.c
decoder.c
case.c
err.c
glist.c
fe_warp_affine.c
fe_warp.c
config.c
ms_mgau.c
ptm_mgau.c
hmm.c
profile.c
s2_semi_mgau.c
jsgf_scanner.c
filename.c
fe_noise.c
fe_warp_piecewise_linear.c
ps_lattice.c
fsg_model.c
bin_mdef.c
listelem_alloc.c
common_audio/signal_processing/min_max_operations.c
common_audio/signal_processing/resample.c
common_audio/signal_processing/spl_inl.c
common_audio/signal_processing/resample_48khz.c
common_audio/signal_processing/downsample_fast.c
common_audio/signal_processing/get_scaling_square.c
common_audio/signal_processing/vector_scaling_operations.c
common_audio/signal_processing/division_operations.c
common_audio/signal_processing/resample_fractional.c
common_audio/signal_processing/resample_by_2_internal.c
common_audio/signal_processing/energy.c
common_audio/signal_processing/cross_correlation.c
common_audio/vad/vad_core.c
common_audio/vad/vad_sp.c
common_audio/vad/vad_gmm.c
common_audio/vad/vad_filterbank.c
common_audio/vad/webrtc_vad.c
fsg_history.c
mmio.c
fsg_search.c
tmat.c
dict2pid.c
  )

add_library(soundswallower ${SOURCES})
set_property(TARGET soundswallower PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
target_include_directories(soundswallower PRIVATE ${PROJECT_SOURCE_DIR}/src
  soundswallower PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  soundswallower PUBLIC ${PROJECT_SOURCE_DIR}/include)
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(soundswallower PUBLIC ${MATH_LIBRARY})
endif()

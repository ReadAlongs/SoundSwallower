set(SOURCES
acmod.c
bin_mdef.c
bitvec.c
blkarray_list.c
case.c
ckd_alloc.c
cmn.c
cmn_live.c
common_audio/signal_processing/cross_correlation.c
common_audio/signal_processing/division_operations.c
common_audio/signal_processing/downsample_fast.c
common_audio/signal_processing/energy.c
common_audio/signal_processing/get_scaling_square.c
common_audio/signal_processing/min_max_operations.c
common_audio/signal_processing/resample_48khz.c
common_audio/signal_processing/resample_by_2_internal.c
common_audio/signal_processing/resample.c
common_audio/signal_processing/resample_fractional.c
common_audio/signal_processing/spl_inl.c
common_audio/signal_processing/vector_scaling_operations.c
common_audio/vad/vad_core.c
common_audio/vad/vad_filterbank.c
common_audio/vad/vad_gmm.c
common_audio/vad/vad_sp.c
common_audio/vad/webrtc_vad.c
config.c
decoder.c
dict2pid.c
dict.c
err.c
feat.c
fe_interface.c
fe_noise.c
fe_sigproc.c
fe_warp_affine.c
fe_warp.c
fe_warp_inverse_linear.c
fe_warp_piecewise_linear.c
fsg_history.c
fsg_lextree.c
fsg_model.c
fsg_search.c
genrand.c
glist.c
hash_table.c
hmm.c
jsgf.c
jsgf_parser.c
jsgf_scanner.c
lda.c
listelem_alloc.c
logmath.c
mdef.c
mmio.c
ms_gauden.c
ms_mgau.c
ms_senone.c
profile.c
ps_alignment.c
ps_endpointer.c
ps_lattice.c
ps_mllr.c
ps_vad.c
ptm_mgau.c
s2_semi_mgau.c
s3file.c
strfuncs.c
state_align_search.c
tmat.c
vector.c
yin.c
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

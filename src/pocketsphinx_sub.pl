# header names
s,soundswallower/pocketsphinx\.h,soundswallower/decoder.h,g;
s,^.*#include.*pocketsphinx_internal\.h.*$,,;
s,^.*#include.*_internal\.h.*$,,;
s,soundswallower/ps_alignment\.h,soundswallower/alignment.h,g;
s,soundswallower/ps_mllr\.h,soundswallower/mllr.h,g;
s,soundswallower/ps_lattice\.h,soundswallower/lattice.h,g;

# type names
s,ps_decoder_(t|s),decoder_${1},g;
s,ps_lat(\w+_[ts]),lat${1},g;
s,ps_seg_(t|s),seg_iter_${1},g;
s,ps_astar_(t|s),astar_search_${1},g;
s,ps_nbest_t,hyp_iter_t,g;
s,ps_mgau_(t|s),mgau_${1},g;
s,ps_mllr_(t|s),mllr_${1},g;
s,ps_alignment(_\w*[ts]),alignment${1},g;
s,ps_mgaufuncs_(t|s),mgaufuncs_${1},g;
s,ps_searchfuncs_(t|s),searchfuncs_${1},g;

# type and methods
s,ps_search,search_module,g;
s,ps_lattice,lattice,g;
s,ps_astar,astar,g;
s,ps_mllr,mllr,g;

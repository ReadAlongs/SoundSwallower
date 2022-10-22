# header names
s,soundswallower/cmd_ln.h,soundswallower/configuration.h,g;
s,soundswallower/cmdln_macro.h,soundswallower/config_defs.h,g;
# type names
s/cmd_ln_t/config_t/g;
s/cmd_ln_val_t/config_val_t/g;
s/arg_t/config_param_t/g;
# dashes
s/cmd_ln_((?:set_)?[^_]+)_r(.*?)"-/config_${1}${2}"/g;
# declarations, etc
s/cmd_ln_((?:set_)?[^_]+)_r/config_${1}/g;
# boolean -> bool
s/config(_set)?_boolean/config${1}_bool/g;
# int, float
s/config(_set)?_(int|float)(32|64)/config${1}_${2}/g;

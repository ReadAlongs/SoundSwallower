# type name
s/cmd_ln_t/config_t/g;
# dashes
s/cmd_ln_((?:set_)?[^_]+)_r(.*?)"-/config_${1}${2}"/g;
# declarations, etc
s/cmd_ln_((?:set_)?[^_]+)_r/config_${1}/g;
# boolean -> bool
s/config(_set)?_boolean/config${1}_bool/g;

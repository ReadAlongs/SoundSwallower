// Preamble to SoundSwallower JS library, wraps some C functions

const ARG_INTEGER = (1 << 1);
const ARG_FLOATING = (1 << 2);
const ARG_STRING = (1 << 3);
const ARG_BOOLEAN = (1 << 4);

Module.cmd_ln_init = function() {
    return _cmd_ln_parse_r(0, _ps_args(), 0, 0, 0);
}
Module.cmd_ln_type = cwrap('cmd_ln_type_r', 'number',
			   ['number', 'string']);
Module.cmd_ln_set_str = cwrap('cmd_ln_set_str_r', null,
			      ['number', 'string', 'string']);
Module.cmd_ln_get_str = cwrap('cmd_ln_str_r', 'string',
			      ['number', 'string']);
Module.cmd_ln_set_int = cwrap('cmd_ln_set_int_r', null,
			      ['number', 'string', 'number']);
Module.cmd_ln_get_int = cwrap('cmd_ln_int_r', 'number',
			      ['number', 'string']);
Module.cmd_ln_set_float = cwrap('cmd_ln_set_float_r', null,
				['number', 'string', 'number']);
Module.cmd_ln_get_float = cwrap('cmd_ln_float_r', 'number',
				['number', 'string']);
Module.cmd_ln_set = function(cmd_ln, key, val) {
    let type = this.cmd_ln_type(cmd_ln, key);
    if (type == 0) {
	throw new ReferenceError("Unknown cmd_ln parameter "+key);
    }
    if (type & ARG_STRING) {
	this.cmd_ln_set_str(cmd_ln, key, val);
    }
    else if (type & ARG_FLOATING) {
	this.cmd_ln_set_float(cmd_ln, key, val);
    }
    else if (type & (ARG_INTEGER | ARG_BOOLEAN)) {
	this.cmd_ln_set_int(cmd_ln, key, val);
    }
    else {
	throw new TypeError("Unsupported type "+type+"for parameter"+key);
    }
}
Module.cmd_ln_get = function(cmd_ln, key) {
    let type = this.cmd_ln_type(cmd_ln, key);
    if (type == 0) {
	throw new ReferenceError("Unknown cmd_ln parameter "+key);
    }
    if (type & ARG_STRING) {
	return this.cmd_ln_get_str(cmd_ln, key);
    }
    else if (type & ARG_FLOATING) {
	return this.cmd_ln_get_float(cmd_ln, key);
    }
    else if (type & (ARG_INTEGER | ARG_BOOLEAN)) {
	return this.cmd_ln_get_int(cmd_ln, key);
    }
    else {
	throw new TypeError("Unsupported type "+type+"for parameter"+key);
    }
}
Module.ps_process_raw = cwrap('ps_process_raw', 'number',
			      ['number', 'array', 'number', 'number', 'number']);
Module.ps_get_hyp = cwrap('ps_get_hyp', 'string', ['number', 'number']);

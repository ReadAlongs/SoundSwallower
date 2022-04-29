// Preamble to SoundSwallower JS library, wraps some C functions

// our classes use delete() following embind usage: https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html#memory-management

const ARG_INTEGER = (1 << 1);
const ARG_FLOATING = (1 << 2);
const ARG_STRING = (1 << 3);
const ARG_BOOLEAN = (1 << 4);

Module.cmd_ln_init = function() {
    return _cmd_ln_parse_r(0, _ps_args(), 0, 0, 0);
};
Module.cmd_ln_type = cwrap('cmd_ln_type_r', 'number',
			   ['number', 'string']);
Module.cmd_ln_set = function(cmd_ln, key, val) {
    let ckey = allocateUTF8OnStack(key);
    let type = Module._cmd_ln_type_r(cmd_ln, ckey);
    if (type == 0) {
	throw new ReferenceError("Unknown cmd_ln parameter "+key);
    }
    if (type & ARG_STRING) {
	let cval = allocateUTF8OnStack(val);
	Module._cmd_ln_set_str_r(cmd_ln, ckey, cval);
    }
    else if (type & ARG_FLOATING) {
	Module._cmd_ln_set_float_r(cmd_ln, ckey, val);
    }
    else if (type & (ARG_INTEGER | ARG_BOOLEAN)) {
	Module._cmd_ln_set_int_r(cmd_ln, ckey, val);
    }
    else {
	return false;
    }
    return true;
};
Module.cmd_ln_get = function(cmd_ln, key) {
    let ckey = allocateUTF8OnStack(key);
    let type = Module._cmd_ln_type_r(cmd_ln, ckey);
    if (type == 0) {
	throw new ReferenceError("Unknown cmd_ln parameter "+key);
    }
    if (type & ARG_STRING) {
	return UTF8ToString(Module._cmd_ln_str_r(cmd_ln, ckey));
    }
    else if (type & ARG_FLOATING) {
	return Module._cmd_ln_float_r(cmd_ln, ckey);
    }
    else if (type & ARG_INTEGER) {
	return Module._cmd_ln_int_r(cmd_ln, ckey);
    }
    else if (type & ARG_BOOLEAN) {
	return Boolean(Module._cmd_ln_int_r(cmd_ln, ckey));
    }
    else {
	throw new TypeError("Unsupported type "+type+" for parameter"+key);
    }
};
Module.ps_process_raw = cwrap('ps_process_raw', 'number',
			      ['number', 'array', 'number', 'number', 'number']);
Module.ps_get_hyp = cwrap('ps_get_hyp', 'string', ['number', 'number']);
Module.ps_get_hypseg = function(decoder) {
    let itor = Module._ps_seg_iter(decoder);
    let config = Module._ps_get_config(decoder);
    let frate = Module._cmd_ln_int_r(config, allocateUTF8OnStack("-frate"));
    let seg = [];
    while (itor != 0) {
	let frames = stackAlloc(8);
	Module._ps_seg_frames(itor, frames, frames + 4);
	let start_frame = getValue(frames, 'i32');
	let end_frame = getValue(frames + 4, 'i32');
	seg_item = {
	    word: ccall('ps_seg_word', 'string', ['number'], [itor]),
	    start: start_frame / frate,
	    end: end_frame / frate
	};
	seg.push(seg_item);
	itor = Module._ps_seg_next(itor);
    }
    return seg;
};
Module.ps_add_word = cwrap('ps_add_word', 'number',
			   ['number', 'string', 'string', 'number']);
Module.ps_create_fsg = function(decoder, name, start_state, final_state, transitions) {
    let logmath = Module._ps_get_logmath(decoder);
    let config = Module._ps_get_config(decoder);
    let lw = Module._cmd_ln_float_r(config, allocateUTF8OnStack("-lw"));
    let n_state = 0;
    for (let t of transitions) {
	n_state = Math.max(n_state, t.from, t.to);
    }
    n_state++;
    let fsg = ccall('fsg_model_init',
		    'number', ['string', 'number', 'number', 'number'],
		    [name, logmath, lw, n_state]);
    Module._fsg_set_states(fsg, start_state, final_state);
    for (let t of transitions) {
	let logprob = 0;
	if ('prob' in t) {
	    logprob = Module._logmath_log(logmath, t.prob);
	}
	if ('word' in t) {
	    let wid = ccall('fsg_model_word_add', 'number',
			    ['number', 'string'],
			    [fsg, t.word]);
	    if (wid == -1) {
		Module._fsg_model_free(fsg);
		return 0;
	    }
	    Module._fsg_model_trans_add(fsg, t.from, t.to, logprob, wid);
	}
	else {
	    Module._fsg_model_null_trans_add(fsg, t.from, t.to, logprob);
	}
    }
    return fsg;
};
Module.ps_set_fsg = function(decoder, fsg) {
    if (Module._ps_set_fsg(decoder, "_default", fsg) != 0) {
	throw new Error("Failed to set FSG in decoder");
    }
};

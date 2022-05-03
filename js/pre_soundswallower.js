// SoundSwallower JavaScript API code.
// our classes use delete() following embind usage:
// https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html#memory-management

const ARG_INTEGER = (1 << 1);
const ARG_FLOATING = (1 << 2);
const ARG_STRING = (1 << 3);
const ARG_BOOLEAN = (1 << 4);

Module.Config = class {
    constructor(dict) {
	this.cmd_ln = Module._cmd_ln_parse_r(0, Module._ps_args(), 0, 0, 0);
	if (dict == undefined)
	    return;
	for (const key in dict) {
	    this.set(key, dict[key]);
	}
    }
    delete() {
	if (this.cmd_ln)
	    Module._cmd_ln_free_r(this.cmd_ln);
	this.cmd_ln = 0;
    }
    normalize_key(key) {
	if (key.length > 0) {
	    if (key[0] == '_') {
		// Ask for underscore, get underscore
		return key;
	    }
	    else if (key[0] == '-') {
		// Ask for dash, get underscore or dash
		const under_key = "_" + key.substr(1);
		if (this.has(under_key))
		    return under_key;
		else
		    return key;
	    }
	    else {
		// No dash or underscore, try underscore then dash
		const under_key = "_" + key;
		if (this.has(under_key))
		    return under_key;
		const dash_key = "-" + key;
		if (this.has(dash_key))
		    return dash_key;
		return key;
	    }
	}
    }
    normalize_ckey(ckey) {
	let key = UTF8ToString(ckey);
	if (key.length == 0)
	    return key;
	else if (key[0] == '-' || key[0] == '_')
	    return key.substr(1);
	return key;
    }
    set(key, val) {
	let ckey = allocateUTF8OnStack(this.normalize_key(key));
	let type = Module._cmd_ln_type_r(this.cmd_ln, ckey);
	if (type == 0) {
	    throw new ReferenceError("Unknown cmd_ln parameter "+key);
	}
	if (type & ARG_STRING) {
	    let cval = allocateUTF8OnStack(val);
	    Module._cmd_ln_set_str_r(this.cmd_ln, ckey, cval);
	}
	else if (type & ARG_FLOATING) {
	    Module._cmd_ln_set_float_r(this.cmd_ln, ckey, val);
	}
	else if (type & (ARG_INTEGER | ARG_BOOLEAN)) {
	    Module._cmd_ln_set_int_r(this.cmd_ln, ckey, val);
	}
	else {
	    return false;
	}
	return true;
    }
    get(key) {
	let ckey = allocateUTF8OnStack(this.normalize_key(key));
	let type = Module._cmd_ln_type_r(this.cmd_ln, ckey);
	if (type == 0) {
	    throw new ReferenceError("Unknown cmd_ln parameter "+key);
	}
	if (type & ARG_STRING) {
	    let val = Module._cmd_ln_str_r(this.cmd_ln, ckey);
	    if (val == 0)
		return null;
	    return UTF8ToString(val);
	}
	else if (type & ARG_FLOATING) {
	    return Module._cmd_ln_float_r(this.cmd_ln, ckey);
	}
	else if (type & ARG_INTEGER) {
	    return Module._cmd_ln_int_r(this.cmd_ln, ckey);
	}
	else if (type & ARG_BOOLEAN) {
	    return Boolean(Module._cmd_ln_int_r(this.cmd_ln, ckey));
	}
	else {
	    throw new TypeError("Unsupported type "+type+" for parameter"+key);
	}
    }
    has(key) {
	let ckey = allocateUTF8OnStack(key);
	return Module._cmd_ln_exists_r(this.cmd_ln, ckey);
    }
    *[Symbol.iterator]() {
	let itor = Module._cmd_ln_hash_iter(this.cmd_ln);
	let seen = new Set();
	while (itor != 0) {
	    let ckey = Module._hash_iter_key(itor);
	    let key = this.normalize_ckey(ckey);
	    if (seen.has(key))
		continue;
	    seen.add(key);
	    itor = Module._hash_table_iter_next(itor);
	    yield key;
	}
    }
};

Module.Decoder = class {
    constructor(config) {
	if (config && typeof(config) == 'object' && 'cmd_ln' in config) {
	    this.config = config;
	    this.ps = Module._ps_init(this.config.cmd_ln);
	}
	else {
	    this.config = new Module.Config(...arguments);
	    this.ps = Module._ps_init(this.config.cmd_ln);
	}
	if (this.ps == 0) {
	    throw new Error("Failed to initialize decoder");
	}
    }

    delete() {
	this.config.delete();
	if (this.ps)
	    Module._ps_free(this.ps);
	this.ps = 0;
    }

    start() {
	if (Module._ps_start_utt(this.ps) < 0) {
	    throw new Error("Failed to start utterance processing");
	}
    }

    stop() {
	if (Module._ps_end_utt(this.ps) < 0) {
	    throw new Error("Failed to stop utterance processing");
	}
    }

    process_raw(pcm, no_search=false, full_utt=false) {
	let pcm_bytes = pcm.length * pcm.BYTES_PER_ELEMENT;
	let pcm_addr = Module._malloc(pcm_bytes);
	let pcm_u8 = new Uint8Array(pcm.buffer);
	// Emscripten documentation fails to mention that this
	// function specifically takes a Uint8Array
	writeArrayToMemory(pcm_u8, pcm_addr);
	let rv = Module._ps_process_raw(this.ps, pcm_addr, pcm_bytes / 2,
					no_search, full_utt);
	Module._free(pcm_addr);
	if (rv < 0) {
	    throw new Error("Utterance processing failed");
	}
	return rv;
    }
    
    get_hyp() {
	return UTF8ToString(Module._ps_get_hyp(this.ps, 0));
    }

    get_hypseg() {
	let itor = Module._ps_seg_iter(this.ps);
	let config = Module._ps_get_config(this.ps);
	let frate = Module._cmd_ln_int_r(config, allocateUTF8OnStack("-frate"));
	let seg = [];
	while (itor != 0) {
	    let frames = stackAlloc(8);
	    Module._ps_seg_frames(itor, frames, frames + 4);
	    let start_frame = getValue(frames, 'i32');
	    let end_frame = getValue(frames + 4, 'i32');
	    let seg_item = {
		word: ccall('ps_seg_word', 'string', ['number'], [itor]),
		start: start_frame / frate,
		end: end_frame / frate
	    };
	    seg.push(seg_item);
	    itor = Module._ps_seg_next(itor);
	}
	return seg;
    }

    add_word(word, pron, update=true) {
	let cword = allocateUTF8OnStack(word);
	let cpron = allocateUTF8OnStack(pron);
	let wid = Module._ps_add_word(this.ps, cword, cpron, update);
	if (wid < 0)
	    throw new Error("Failed to add word "+word+" with pronunciation "+
			    pron+" to the dictionary.");
	return wid;
    }

    create_fsg(name, start_state, final_state, transitions) {
	let logmath = Module._ps_get_logmath(this.ps);
	let config = Module._ps_get_config(this.ps);
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
	return {
	    fsg: fsg,
	    delete() {
		Module._fsg_model_free(this.fsg);
		this.fsg = 0;
	    }
	};
    }
    set_fsg(fsg) {
	if (Module._ps_set_fsg(this.ps, "_default", fsg.fsg) != 0) {
	    throw new Error("Failed to set FSG in decoder");
	}
    }
};

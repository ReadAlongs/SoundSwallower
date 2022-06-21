// SoundSwallower JavaScript API code.
// our classes use delete() following embind usage:
// https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html#memory-management

const ARG_INTEGER = (1 << 1);
const ARG_FLOATING = (1 << 2);
const ARG_STRING = (1 << 3);
const ARG_BOOLEAN = (1 << 4);

const DEFAULT_MODEL = 'en-us';

// FIXME: Emscripten already defines something like this in its
// runtime but I have no $#@! idea how to access its definition from a
// pre-js, much like various other things, WTF.
const RUNNING_ON_WEB = (typeof window == 'object'
			|| typeof importScripts == 'function');
// User can specify a default model, or none at all
if (typeof(Module.defaultModel) === 'undefined') {
    Module.defaultModel = DEFAULT_MODEL;
}
// User can also specify the base URL for models
if (typeof(Module.modelBase) === 'undefined') {
    if (RUNNING_ON_WEB) {
	Module.modelBase = "model/";
    }
    else {
	Module.modelBase = require("./model/index.js");
    }
}

/**
 * Configuration object for SoundSwallower recognizer.
 *
 * There is a fixed set of configuration values, which can be iterated
 * over using the iterator property.  Many have default values.
 */
class Config {
    /**
     * Create a configuration object.
     * @param {Object} [dict] - Initial configuration parameters and
     * values.  Some of the more common are noted below, the full list
     * can be found at
     * https://soundswallower.readthedocs.io/en/latest/config_params.html
     * @param {string} [dict.hmm=Module.get_model_path(Module.defaultModel)]
     *                 - Directory or base URL of acoustic model.
     * @param {string} [dict.loglevel="ERROR"] - Verbosity of logging.
     * @param {number} [dict.samprate=44100] - Sampling rate of input.
     */
    constructor(dict) {
	this.cmd_ln = Module._cmd_ln_parse_r(0, Module._ps_args(), 0, 0, 0);
	if (typeof(dict) === 'undefined') {
	    if (Module.defaultModel !== null)
		dict = { hmm: Module.get_model_path(Module.defaultModel) };
	    else
		return;
	}
	else if (Module.defaultModel !== null) {
	    if (!("hmm" in dict))
		dict.hmm = Module.get_model_path(Module.defaultModel);
	}
	for (const key in dict) {
	    this.set(key, dict[key]);
	}
    }
    /**
     * Free Emscripten memory associated with this Config.
     */
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
	else
	    return "";
    }
    normalize_ckey(ckey) {
	const key = UTF8ToString(ckey);
	if (key.length == 0)
	    return key;
	else if (key[0] == '-' || key[0] == '_')
	    return key.substr(1);
	return key;
    }
    /**
     * Set a configuration parameter.
     * @param {string} key - Parameter name.
     * @param {number|string} val - Parameter value.
     * @throws {ReferenceError} Throws ReferenceError if key is not a known parameter.
     */
    set(key, val) {
	const nkey = this.normalize_key(key);
	const ckey = allocateUTF8OnStack(nkey);
	const type = Module._cmd_ln_type_r(this.cmd_ln, ckey);
	if (type == 0) {
	    throw new ReferenceError("Unknown cmd_ln parameter "+key);
	}
	if (type & ARG_STRING) {
	    const cval = allocateUTF8OnStack(val);
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
    /**
     * Get a configuration parameter's value.
     * @param {string} key - Parameter name.
     * @returns {number|string} Parameter value.
     * @throws {ReferenceError} Throws ReferenceError if key is not a known parameter.
     */
    get(key) {
	const ckey = allocateUTF8OnStack(this.normalize_key(key));
	const type = Module._cmd_ln_type_r(this.cmd_ln, ckey);
	if (type == 0) {
	    throw new ReferenceError("Unknown cmd_ln parameter "+key);
	}
	if (type & ARG_STRING) {
	    const val = Module._cmd_ln_str_r(this.cmd_ln, ckey);
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
    /**
     * Get a model parameter with backoff to path inside current model.
     */
    model_file_path(key, modelfile) {
	const val = this.get(key);
	if (val != null)
	    return val;
	const hmmpath = this.get("hmm");
	if (hmmpath == null)
	    throw new Error("Could not get "+key+" from config or model directory");
	return hmmpath + "/" + modelfile;
    }
    /**
     * Test if a key is a known parameter.
     * @param {string} key - Key whose existence to check.
     */
    has(key) {
	const ckey = allocateUTF8OnStack(key);
	return Module._cmd_ln_exists_r(this.cmd_ln, ckey);
    }
    *[Symbol.iterator]() {
	let itor = Module._cmd_ln_hash_iter(this.cmd_ln);
	const seen = new Set();
	while (itor != 0) {
	    const ckey = Module._hash_iter_key(itor);
	    const key = this.normalize_ckey(ckey);
	    if (seen.has(key))
		continue;
	    seen.add(key);
	    itor = Module._hash_table_iter_next(itor);
	    yield key;
	}
    }
};

/**
 * Speech recognizer object.
 */
class Decoder {
    /**
     * Create the decoder.
     *
     * This can be called with a previously created Config
     * object, or with an Object containing configuration keys and
     * values from which a Config will be created.
     *
     * @param {Object|Config} [config] - Configuration parameters.
     */
    constructor(config) {
	if (config && typeof(config) == 'object' && 'cmd_ln' in config)
	    this.config = config;
	else
	    this.config = new Module.Config(...arguments);
	this.initialized = false;
	this.ps = Module._ps_init(0);
	if (this.ps == 0)
	    throw new Error("Failed to construct Decoder");
    }
    /**
     * (Re-)initialize the decoder asynchronously.
     * @param {Object|Config} [config] - New configuration parameters
     * to apply, if desired.
     * @returns {Promise} Promise resolved once decoder is ready.
     */
    async initialize(config) {
	if (this.ps == 0)
	    throw new Error("Decoder was somehow not constructed (ps==0)");
	if (config !== undefined) {
	    if (this.config)
		this.config.delete();
	    if (typeof(config) == 'object' && 'cmd_ln' in config)
		this.config = config;
	    else
		this.config = new Module.Config(...arguments);
	}
	await this.init_config();
	await this.init_fe();
	await this.init_feat();
	await this.init_acmod();
	await this.load_acmod_files();
	await this.init_dict();
	await this.init_grammar();

	this.initialized = true;
    }

    /**
     * Initialize decoder configuration.
     */
    async init_config() {
	await this.init_featparams()
	let rv = Module._ps_init_config(this.ps, this.config.cmd_ln);
	if (rv < 0)
	    throw new Error("Failed to initialize basic configuration");
	rv = Module._ps_init_cleanup(this.ps);
	if (rv < 0)
	    throw new Error("Failed to clean up decoder internals");
    }

    /**
     * Read feature parameters from acoustic model.
     */
    async init_featparams() {
	const featparams = this.config.model_file_path("featparams", "feat.params");
	for await (const pair of read_featparams(featparams)) {
	    if (this.config.has(pair[0])) /* Sometimes it doesn't */
		this.config.set(pair[0], pair[1]);
	}
    }
    
    /**
     * Create front-end from configuration.
     */
    async init_fe() {
	const rv = Module._ps_init_fe(this.ps);
	if (rv == 0)
	    throw new Error("Failed to initialize frontend");
    }

    /**
     * Create dynamic feature module from configuration.
     */
    async init_feat() {
	let rv;
	try {
	    const lda_path = this.config.model_file_path("lda", "feature_transform");
	    const lda = await load_to_s3file(lda_path);
	    rv = Module._ps_init_feat_s3file(this.ps, lda);
	}
	catch (e) {
	    rv = Module._ps_init_feat_s3file(this.ps, 0);
	}
	if (rv == 0)
	    throw new Error("Failed to initialize feature module");
    }

    /**
     * Create acoustic model from configuration.
     */
    async init_acmod() {
	const rv = Module._ps_init_acmod_pre(this.ps);
	if (rv == 0)
	    throw new Error("Failed to initialize acoustic model");
    }

    /**
     * Load acoustic model files
     */
    async load_acmod_files() {
	const mdef = this.config.model_file_path("mdef", "mdef.bin");
	await this.load_mdef(mdef);
	const tmat = this.config.model_file_path("tmat", "transition_matrices");
	await this.load_tmat(tmat);
	const means = this.config.model_file_path("mean", "means");
	const variances = this.config.model_file_path("var", "variances");
	const sendump = this.config.model_file_path("sendump", "sendump");
	const mixw = this.config.model_file_path("mixw", "mixture_weights");
	await this.load_gmm(means, variances, sendump, mixw);
	const rv = Module._ps_init_acmod_post(this.ps);
	if (rv < 0)
	    throw new Error("Failed to initialize acoustic scoring");
    }

    /**
     * Load binary model definition file
     */
    async load_mdef(mdef_path) {
	const s3f = await load_to_s3file(mdef_path);
	const mdef = Module._bin_mdef_read_s3file(s3f);
	Module._s3file_free(s3f);
	if (mdef == 0)
	    throw new Error("Failed to read mdef");
	Module._set_mdef(this.ps, mdef);
    }

    /**
     * Load transition matrices
     */
    async load_tmat(tmat_path) {
	const s3f = await load_to_s3file(tmat_path);
	const logmath = Module._ps_get_logmath(this.ps);
	const tpfloor = this.config.get("tmatfloor");
	const tmat = Module._tmat_init_s3file(s3f, logmath, tpfloor);
	Module._s3file_free(s3f);
	if (tmat == 0)
	    throw new Error("Failed to read tmat");
	Module._set_tmat(this.ps, tmat);
    }

    /**
     * Load Gaussian mixture models
     */
    async load_gmm(means_path, variances_path, sendump_path, mixw_path) {
	const means = await load_to_s3file(means_path);
	const variances = await load_to_s3file(variances_path);
	var sendump, mixw;
	/* Prefer sendump if available. */
	try {
	    sendump = await load_to_s3file(sendump_path);
	    mixw = 0;
	}
	catch (e) {
	    sendump = 0;
	    mixw = await load_to_s3file(mixw_path);
	}
	if (Module._load_gmm(this.ps, means, variances, mixw, sendump) < 0)
	    throw new Error("Failed to load GMM parameters");
    }

    /**
     * Load dictionary from configuration.
     */
    async init_dict() {
	const dict_path = this.config.model_file_path("dict", "dict.txt");
	const dict = await load_to_s3file(dict_path);
	let fdict;
	try {
	    const fdict_path = this.config.model_file_path("fdict", "noisedict");
	    fdict = await load_to_s3file(fdict_path);
	}
	catch (e) {
	    fdict = 0;
	}
	const rv = Module._ps_init_dict_s3file(this.ps, dict, fdict);
	if (rv == 0)
	    throw new Error("Failed to initialize dictionaries");
    }

    /**
     * Load grammar from configuration.
     */
    async init_grammar() {
	let fsg = 0, jsgf = 0;
	const jsgf_path = this.config.get("jsgf");
	if (jsgf_path != null)
	    jsgf = await load_to_s3file(jsgf_path)
	const fsg_path = this.config.get("fsg");
	if (fsg_path != null)
	    fsg = await load_to_s3file(fsg_path)
	if (fsg || jsgf) {
	    const rv = Module._ps_init_grammar_s3file(this.ps, fsg, jsgf);
	    if (rv < 0)
		throw new Error("Failed to initialize grammar");
	}
    }

    /**
     * Throw an error if decoder is not initialized.
     * @throws {Error} If decoder is not initialized.
     */
    assert_initialized() {
	if (!this.initialized)
	    throw new Error("Decoder not yet initialized");
    }

    /**
     * Re-initialize only the audio feature extraction.
     * @returns {Promise} Promise resolved once reinitialized.
     */
    async reinitialize_audio() {
	this.assert_initialized();
	Module._ps_reinit_fe(this.ps, 0);
    }

    /**
     * Release the Emscripten memory associated with a Decoder.
     */
    delete() {
	if (this.config)
	    this.config.delete();
	if (this.ps)
	    Module._ps_free(this.ps);
	this.ps = 0;
    }

    /**
     * Start processing input asynchronously.
     * @returns {Promise} Promise resolved once processing is started.
     */
    async start() {
	this.assert_initialized();
	if (Module._ps_start_utt(this.ps) < 0) {
	    throw new Error("Failed to start utterance processing");
	}
    }

    /**
     * Finish processing input asynchronously.
     * @returns {Promise} Promise resolved once processing is finished.
     */
    async stop() {
	this.assert_initialized();
	if (Module._ps_end_utt(this.ps) < 0) {
	    throw new Error("Failed to stop utterance processing");
	}
    }

    /**
     * Process a block of audio data asynchronously.
     * @param {Float32Array} pcm - Audio data, in float32 format, in
     * the range [-1.0, 1.0].
     * @returns {Promise<number>} Promise resolved to the number of
     * frames processed.
     */
    async process(pcm, no_search=false, full_utt=false) {
	this.assert_initialized();
	const pcm_bytes = pcm.length * pcm.BYTES_PER_ELEMENT;
	const pcm_addr = Module._malloc(pcm_bytes);
	const pcm_u8 = new Uint8Array(pcm.buffer);
	// Emscripten documentation fails to mention that this
	// function specifically takes a Uint8Array
	writeArrayToMemory(pcm_u8, pcm_addr);
	const rv = Module._ps_process_float32(this.ps, pcm_addr, pcm_bytes / 4,
					    no_search, full_utt);
	Module._free(pcm_addr);
	if (rv < 0) {
	    throw new Error("Utterance processing failed");
	}
	return rv;
    }

    /**
     * Get the currently recognized text.
     * @returns {string} Currently recognized text.
     */
    get_hyp() {
	this.assert_initialized();
	return UTF8ToString(Module._ps_get_hyp(this.ps, 0));
    }

    /**
     * Get the current recognition result as a word segmentation.
     * @returns {Array<Object>} Array of Objects for the words
     * recognized, each with the keys `word`, `start` and `end`.
     */
    get_hypseg() {
	this.assert_initialized();
	let itor = Module._ps_seg_iter(this.ps);
	const config = Module._ps_get_config(this.ps);
	const frate = Module._cmd_ln_int_r(config, allocateUTF8OnStack("-frate"));
	const seg = [];
	while (itor != 0) {
	    const frames = stackAlloc(8);
	    Module._ps_seg_frames(itor, frames, frames + 4);
	    const start_frame = getValue(frames, 'i32');
	    const end_frame = getValue(frames + 4, 'i32');
	    const seg_item = {
		word: ccall('ps_seg_word', 'string', ['number'], [itor]),
		start: start_frame / frate,
		end: end_frame / frate
	    };
	    seg.push(seg_item);
	    itor = Module._ps_seg_next(itor);
	}
	return seg;
    }

    /**
     * Look up a word in the pronunciation dictionary.
     * @param {string} word - Text of word to look up.
     * @returns {string} - Space-separated list of phones, or `null` if
     * word is not in the dictionary.
     */
    lookup_word(word) {
	this.assert_initialized();
	const cword = allocateUTF8OnStack(word);
	const cpron = Module._ps_lookup_word(this.ps, cword);
	if (cpron == 0)
	    return null;
	return UTF8ToString(cpron);
    }
    
    /**
     * Add a word to the pronunciation dictionary asynchronously.
     * @param {string} word - Text of word to add.
     * @param {string} pron - Pronunciation of word as space-separated list of phonemes.
     * @param {number} update - Update decoder immediately (set to
     * false when adding a list of words, except for the last word).
     * @returns {Promise} - Promise resolved once word has been added.
     */
    async add_word(word, pron, update=true) {
	this.assert_initialized();
	const cword = allocateUTF8OnStack(word);
	const cpron = allocateUTF8OnStack(pron);
	const wid = Module._ps_add_word(this.ps, cword, cpron, update);
	if (wid < 0)
	    throw new Error("Failed to add word "+word+" with pronunciation "+
			    pron+" to the dictionary.");
	return wid;
    }

    /**
     * Create a finite-state grammar from a list of transitions.
     * @param {string} name - Name of grammar.
     * @param {number} start_state - Index of starting state.
     * @param {number} final_state - Index of ending state.
     * @param {Array<Object>} transitions - Array of transitions, each
     * of which is an Object with the keys `from`, `to`, `word`, and
     * `prob`.  The word must exist in the dictionary.
     * @returns {Object} Newly created grammar - you *must* free this
     * by calling its delete() method once it is no longer needed,
     * such as after passing to set_fsg().
     */
    create_fsg(name, start_state, final_state, transitions) {
	this.assert_initialized();
	const logmath = Module._ps_get_logmath(this.ps);
	const config = Module._ps_get_config(this.ps);
	const lw = Module._cmd_ln_float_r(config, allocateUTF8OnStack("-lw"));
	let n_state = 0;
	for (const t of transitions) {
	    n_state = Math.max(n_state, t.from, t.to);
	}
	n_state++;
	const fsg = ccall('fsg_model_init',
			'number', ['string', 'number', 'number', 'number'],
			[name, logmath, lw, n_state]);
	Module._fsg_set_states(fsg, start_state, final_state);
	for (const t of transitions) {
	    let logprob = 0;
	    if ('prob' in t) {
		logprob = Module._logmath_log(logmath, t.prob);
	    }
	    if ('word' in t) {
		const wid = ccall('fsg_model_word_add', 'number',
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

    /**
     * Create a grammar from JSGF.
     * @param {string} jsgf_string - String containing JSGF grammar.
     * @param {string} [toprule] - Name of starting rule for grammar,
     * if not specified, the first public rule will be used.
     * @returns {Object} Newly created grammar - you *must* free this
     * by calling its delete() method once it is no longer needed,
     * such as after passing to set_fsg().
     */
    parse_jsgf(jsgf_string, toprule=null) {
	this.assert_initialized();
	const logmath = Module._ps_get_logmath(this.ps);
	const config = Module._ps_get_config(this.ps);
	const lw = Module._cmd_ln_float_r(config, allocateUTF8OnStack("-lw"));
	const cjsgf = allocateUTF8OnStack(jsgf_string);
	const jsgf = Module._jsgf_parse_string(cjsgf, 0);
	if (jsgf == 0)
	    throw new Error("Failed to parse JSGF");
	let rule;
	if (toprule !== null) {
	    const crule = allocateUTF8OnStack(toprule);
	    rule = Module._jsgf_get_rule(jsgf, crule);
	    if (rule == 0)
		throw new Error("Failed to find top rule " + toprule);
	}
	else {
	    rule = Module._jsgf_get_public_rule(jsgf);
	    if (rule == 0)
		throw new Error("No public rules found in JSGF");
	}
	const fsg = Module._jsgf_build_fsg(jsgf, rule, logmath, lw);
	Module._jsgf_grammar_free(jsgf);
	return {
	    fsg: fsg,
	    delete() {
		Module._fsg_model_free(this.fsg);
		this.fsg = 0;
	    }
	};
    }

    /**
     * Set the grammar for recognition, asynchronously.
     * @param {Object} fsg - Grammar produced by parse_jsgf() or
     * create_fsg().  You must call its delete() method after
     * passing it here if you do not intend to reuse it.
     * @returns {Promise} Promise fulfilled once grammar is updated.
     */
    async set_fsg(fsg) {
	this.assert_initialized();
	if (Module._ps_set_fsg(this.ps, "_default", fsg.fsg) != 0) {
	    throw new Error("Failed to set FSG in decoder");
	}
    }
};

/**
 * Generate [key,value] pairs from feat.params file/URL.
 */
async function* read_featparams(featparams) {
    let fpdata;
    if (RUNNING_ON_WEB) {
	const response = await fetch(featparams);
	if (response.ok)
	    fpdata = await response.text();
	else
	    throw new Error("Failed to fetch " + featparams + " :"
			    + response.statusText);
    }
    else {
	const fs = require("fs/promises");
	fpdata = await fs.readFile(featparams, {encoding: "utf8"});
    }
    const line_re = /^.*$/mg;
    const arg_re = /"([^"]*)"|'([^'])'|(\S+)/g;
    let key = null;
    for (const m of fpdata.matchAll(line_re)) {
	const line = m[0].trim()
	for (const arg of line.matchAll(arg_re)) {
	    const token = arg[1] ?? arg[2] ?? arg[3];
	    if (token == '#')
		break;
	    if (key !== null) {
		yield [key, token];
		key = null;
	    }
	    else
		key = token;
	}
    }
    if (key !== null)
	throw new Error("Odd number of arguments in "+featparams);
}

/**
 * Load a file from disk or Internet and make it into an s3file_t.
 */
async function load_to_s3file(path) {
    let blob_u8;
    if (RUNNING_ON_WEB) {
	const response = await fetch(path);
	if (response.ok) {
	    const blob = await response.blob();
	    const blob_buf = await blob.arrayBuffer();
	    blob_u8 = new Uint8Array(blob_buf);
	}
	else
	    throw new Error("Failed to fetch " + path + " :" + response.statusText);
    }
    else {
	const fs = require("fs/promises");
	// FIXME: Should read directly to emscripten memory... how?
	const blob = await fs.readFile(path);
	blob_u8 = new Uint8Array(blob.buffer);
    }
    const blob_len = blob_u8.length + 1;
    const blob_addr = Module._malloc(blob_len);
    if (blob_addr == 0)
	throw new Error("Failed to allocate "+blob_len+" bytes for "+path);
    writeArrayToMemory(blob_u8, blob_addr);
    // Ensure it is NUL-terminated in case someone treats it as a string
    HEAP8[blob_addr + blob_len] = 0;
    // But exclude the trailing NUL from file size so it works normally
    return Module._s3file_init(blob_addr, blob_len - 1);
}

/**
 * Get a model or model file from the built-in model path.
 */
function get_model_path(subpath) {
    if (RUNNING_ON_WEB) {
	return Module.modelBase + subpath;
    }
    else {
	const path = require("path");
	return path.join(Module.modelBase, subpath);
    }
}

Module.get_model_path = get_model_path;
Module.Config = Config;
Module.Decoder = Decoder;

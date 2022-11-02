// SoundSwallower JavaScript API code.

const ARG_INTEGER = (1 << 1);
const ARG_FLOATING = (1 << 2);
const ARG_STRING = (1 << 3);
const ARG_BOOLEAN = (1 << 4);

const DEFAULT_MODEL = 'en-us';

// User can specify a default model, or none at all
if (typeof(Module.defaultModel) === 'undefined') {
    Module.defaultModel = DEFAULT_MODEL;
}
// User can also specify the base URL for models
if (typeof(Module.modelBase) === 'undefined') {
    if (ENVIRONMENT_IS_WEB) {
	Module.modelBase = "model/";
    }
    else {
	Module.modelBase = require("./model/index.js");
    }
}

/**
 * Speech recognizer object.
 */
class Decoder {
    /**
     * Create the decoder.
     *
     * You may optionally call this with an Object containing
     * configuration keys and values.
     *
     * @param {Object} [config] - Configuration parameters.
     */
    constructor(config) {
	this.initialized = false;
        const cconfig = Module._config_init(0);
	this.cdecoder = Module._decoder_create(cconfig);
	if (this.cdecoder == 0)
	    throw new Error("Failed to construct Decoder");
	if (Module.defaultModel !== null
            && this.get_config("hmm") === null)
            this.set_config("hmm", Module.get_model_path(Module.defaultModel));
	for (const key in config)
	    this.set_config(key, config[key]);
    }
    /**
     * Free resources used by the decoder.
     */
    delete() {
        if (this.cdecoder != 0)
            Module._decoder_free(this.cdecoder);
        this.cdecoder = 0;
    }
    /**
     * Set a configuration parameter.
     * @param {string} key - Parameter name.
     * @param {number|string} val - Parameter value.
     * @throws {ReferenceError} Throws ReferenceError if key is not a known parameter.
     */
    set_config(key, val) {
	const ckey = allocateUTF8OnStack(key);
        const cconfig = Module._decoder_config(this.cdecoder);
	const type = Module._config_typeof(cconfig, ckey);
	if (type == 0) {
	    throw new ReferenceError("Unknown cmd_ln parameter "+key);
	}
	if (type & ARG_STRING) {
	    const cval = allocateUTF8OnStack(val);
	    Module._config_set_str(cconfig, ckey, cval);
	}
	else if (type & ARG_FLOATING) {
	    Module._config_set_float(cconfig, ckey, val);
	}
	else if (type & (ARG_INTEGER | ARG_BOOLEAN)) {
	    Module._config_set_int(cconfig, ckey, val);
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
    get_config(key) {
	const ckey = allocateUTF8OnStack(key);
        const cconfig = Module._decoder_config(this.cdecoder);
	const type = Module._config_typeof(cconfig, ckey);
	if (type == 0) {
	    throw new ReferenceError("Unknown cmd_ln parameter "+key);
	}
	if (type & ARG_STRING) {
	    const val = Module._config_str(cconfig, ckey);
	    if (val == 0)
		return null;
	    return UTF8ToString(val);
	}
	else if (type & ARG_FLOATING) {
	    return Module._config_float(cconfig, ckey);
	}
	else if (type & ARG_INTEGER) {
	    return Module._config_int(cconfig, ckey);
	}
	else if (type & ARG_BOOLEAN) {
	    return Boolean(Module._config_int(cconfig, ckey));
	}
	else {
	    throw new TypeError("Unsupported type "+type+" for parameter"+key);
	}
    }
    /**
     * Get a model parameter with backoff to path inside current model.
     */
    model_file_path(key, modelfile) {
	const val = this.get_config(key);
	if (val != null)
	    return val;
	const hmmpath = this.get_config("hmm");
	if (hmmpath == null)
	    throw new Error("Could not get "+key+" from config or model directory");
	return hmmpath + "/" + modelfile;
    }
    /**
     * Test if a key is a known parameter.
     * @param {string} key - Key whose existence to check.
     */
    has_config(key) {
	const ckey = allocateUTF8OnStack(key);
        const cconfig = Module._decoder_config(this.cdecoder);
	return Module._config_typeof(cconfig, ckey) != 0;
    }
    /**
     * Initialize the decoder asynchronously.
     * @returns {Promise} Promise resolved once decoder is ready.
     */
    async initialize() {
	if (this.cdecoder == 0)
	    throw new Error("Decoder was somehow not constructed (ps==0)");
	await this.init_featparams()
	await this.init_cleanup();
	await this.init_fe();
	await this.init_feat();
	await this.init_acmod();
	await this.load_acmod_files();
	await this.init_dict();
	await this.init_grammar();

	this.initialized = true;
    }

    /**
     * Read feature parameters from acoustic model.
     */
    async init_featparams() {
	const featparams = this.model_file_path("featparams", "feat_params.json");
        const fpdata = await load_json(featparams);
	for (const key in fpdata) {
	    if (this.has_config(key)) /* Sometimes it doesn't */
		this.set_config(key, fpdata[key]);
	}
    }
    
    /**
     * Clean up any lingering search modules.
     */
    async init_cleanup() {
	let rv = Module._decoder_init_cleanup(this.cdecoder);
	if (rv < 0)
	    throw new Error("Failed to clean up decoder internals");
    }

    /**
     * Create front-end from configuration.
     */
    async init_fe() {
	const rv = Module._decoder_init_fe(this.cdecoder);
	if (rv == 0)
	    throw new Error("Failed to initialize frontend");
    }

    /**
     * Create dynamic feature module from configuration.
     */
    async init_feat() {
	let rv;
	try {
	    const lda_path = this.model_file_path("lda", "feature_transform");
	    const lda = await load_to_s3file(lda_path);
	    rv = Module._decoder_init_feat_s3file(this.cdecoder, lda);
	}
	catch (e) {
	    rv = Module._decoder_init_feat_s3file(this.cdecoder, 0);
	}
	if (rv == 0)
	    throw new Error("Failed to initialize feature module");
    }

    /**
     * Create acoustic model from configuration.
     */
    async init_acmod() {
	const rv = Module._decoder_init_acmod_pre(this.cdecoder);
	if (rv == 0)
	    throw new Error("Failed to initialize acoustic model");
    }

    /**
     * Load acoustic model files
     */
    async load_acmod_files() {
	await this.load_mdef();
	const tmat = this.model_file_path("tmat", "transition_matrices");
	await this.load_tmat(tmat);
	const means = this.model_file_path("mean", "means");
	const variances = this.model_file_path("var", "variances");
	const sendump = this.model_file_path("sendump", "sendump");
	const mixw = this.model_file_path("mixw", "mixture_weights");
	await this.load_gmm(means, variances, sendump, mixw);
	const rv = Module._decoder_init_acmod_post(this.cdecoder);
	if (rv < 0)
	    throw new Error("Failed to initialize acoustic scoring");
    }

    /**
     * Load binary model definition file
     */
    async load_mdef() {
	var mdef_path, s3f;
	mdef_path = this.model_file_path("mdef", "mdef");
	s3f = await load_to_s3file(mdef_path);
        const mdef = Module._bin_mdef_read_s3file(s3f, this.get_config("cionly"));
	Module._s3file_free(s3f);
	if (mdef == 0)
	    throw new Error("Failed to read mdef");
	Module._set_mdef(this.cdecoder, mdef);
    }

    /**
     * Load transition matrices
     */
    async load_tmat(tmat_path) {
	const s3f = await load_to_s3file(tmat_path);
	const logmath = Module._decoder_logmath(this.cdecoder);
	const tpfloor = this.get_config("tmatfloor");
	const tmat = Module._tmat_init_s3file(s3f, logmath, tpfloor);
	Module._s3file_free(s3f);
	if (tmat == 0)
	    throw new Error("Failed to read tmat");
	Module._set_tmat(this.cdecoder, tmat);
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
	if (Module._load_gmm(this.cdecoder, means, variances, mixw, sendump) < 0)
	    throw new Error("Failed to load GMM parameters");
    }

    /**
     * Load dictionary from configuration.
     */
    async init_dict() {
	const dict_path = this.model_file_path("dict", "dict.txt");
	const dict = await load_to_s3file(dict_path);
	let fdict;
	try {
	    const fdict_path = this.model_file_path("fdict", "noisedict.txt");
	    fdict = await load_to_s3file(fdict_path);
	}
	catch (e) {
	    fdict = 0;
	}
	const rv = Module._decoder_init_dict_s3file(this.cdecoder, dict, fdict);
	if (rv == 0)
	    throw new Error("Failed to initialize dictionaries");
    }

    /**
     * Load grammar from configuration.
     */
    async init_grammar() {
	let fsg = 0, jsgf = 0;
	const jsgf_path = this.get_config("jsgf");
	if (jsgf_path != null)
	    jsgf = await load_to_s3file(jsgf_path)
	const fsg_path = this.get_config("fsg");
	if (fsg_path != null)
	    fsg = await load_to_s3file(fsg_path)
	if (fsg || jsgf) {
	    const rv = Module._decoder_init_grammar_s3file(this.cdecoder, fsg, jsgf);
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
	Module._decoder_reinit_fe(this.cdecoder, 0);
    }

    /**
     * Start processing input asynchronously.
     * @returns {Promise} Promise resolved once processing is started.
     */
    async start() {
	this.assert_initialized();
	if (Module._decoder_start_utt(this.cdecoder) < 0) {
	    throw new Error("Failed to start utterance processing");
	}
    }

    /**
     * Finish processing input asynchronously.
     * @returns {Promise} Promise resolved once processing is finished.
     */
    async stop() {
	this.assert_initialized();
	if (Module._decoder_end_utt(this.cdecoder) < 0) {
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
	const rv = Module._decoder_process_float32(this.cdecoder, pcm_addr, pcm_bytes / 4,
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
	return UTF8ToString(Module._decoder_hyp(this.cdecoder, 0));
    }

    /**
     * Get the current recognition result as a word segmentation.
     * @returns {Array<Object>} Array of Objects for the words
     * recognized, each with the keys `word`, `start` and `end`.
     */
    get_hypseg() {
	this.assert_initialized();
	let itor = Module._decoder_seg_iter(this.cdecoder);
	const config = Module._decoder_config(this.cdecoder);
	const frate = Module._config_int(config, allocateUTF8OnStack("frate"));
	const seg = [];
	while (itor != 0) {
	    const frames = stackAlloc(8);
	    Module._seg_iter_frames(itor, frames, frames + 4);
	    const start_frame = getValue(frames, 'i32');
	    const end_frame = getValue(frames + 4, 'i32');
	    const seg_item = {
		word: UTF8ToString(Module._seg_iter_word(itor)),
		start: start_frame / frate,
		end: end_frame / frate
	    };
	    seg.push(seg_item);
	    itor = Module._seg_iter_next(itor);
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
	const cpron = Module._decoder_lookup_word(this.cdecoder, cword);
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
	const wid = Module._decoder_add_word(this.cdecoder, cword, cpron, update);
	if (wid < 0)
	    throw new Error("Failed to add word "+word+" with pronunciation "+
			    pron+" to the dictionary.");
	return wid;
    }

    /**
     * Set recognition grammar from a list of transitions.
     * @param {string} name - Name of grammar.
     * @param {number} start_state - Index of starting state.
     * @param {number} final_state - Index of ending state.
     * @param {Array<Object>} transitions - Array of transitions, each
     * of which is an Object with the keys `from`, `to`, `word`, and
     * `prob`.  The word must exist in the dictionary.
     * @returns {Object} Newly created grammar.
     */
    async set_fsg(name, start_state, final_state, transitions) {
	this.assert_initialized();
	const logmath = Module._decoder_logmath(this.cdecoder);
	const config = Module._decoder_config(this.cdecoder);
	const lw = Module._config_float(config, allocateUTF8OnStack("lw"));
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
	if (Module._decoder_set_fsg(this.cdecoder, fsg) != 0)
	    throw new Error("Failed to set FSG in decoder");
    }

    /**
     * Set recognition grammar from JSGF.
     * @param {string} jsgf_string - String containing JSGF grammar.
     * @param {string} [toprule] - Name of starting rule for grammar,
     * if not specified, the first public rule will be used.
     */
    async set_jsgf(jsgf_string, toprule=null) {
	this.assert_initialized();
	const logmath = Module._decoder_logmath(this.cdecoder);
	const config = Module._decoder_config(this.cdecoder);
	const lw = Module._config_float(config, allocateUTF8OnStack("lw"));
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
	if (Module._decoder_set_fsg(this.cdecoder, fsg) != 0)
	    throw new Error("Failed to set FSG in decoder");
    }
};

/**
 * Async read some JSON (maybe there is a built-in that does this?)
 */
async function load_json(path) {
    if (ENVIRONMENT_IS_WEB) {
	const response = await fetch(path);
	if (response.ok)
	    return response.json();
	else
	    throw new Error("Failed to fetch " + path + " :"
			    + response.statusText);
    }
    else {
	const fs = require("fs/promises");
	const data = await fs.readFile(path, {encoding: "utf8"});
        return JSON.parse(data);
    }
}

/**
 * Load a file from disk or Internet and make it into an s3file_t.
 */
async function load_to_s3file(path) {
    let blob_u8;
    if (ENVIRONMENT_IS_WEB) {
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
 *
 * The base path can be set by modifying the `modelBase` property of
 * the module object, at initialization or any other time.  Or you can
 * also just override this function if you have special needs.
 *
 * This function is used by `Decoder` to find the default model, which
 * is equivalent to `Model.modelBase + Model.defaultModel`.
 *
 * @param {string} subpath - path to model directory or parameter
 * file, e.g. "en-us", "en-us/variances", etc
 * @returns {string} concatenated path. Note this is a simple string
 * concatenation on the Web, so ensure that `modelBase` has a trailing
 * slash if it is a directory.
 */
function get_model_path(subpath) {
    if (ENVIRONMENT_IS_WEB) {
	return Module.modelBase + subpath;
    }
    else {
	const path = require("path");
	return path.join(Module.modelBase, subpath);
    }
}

Module.get_model_path = get_model_path;
Module.Decoder = Decoder;

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
     * @param {Object} [config] Configuration parameters.
     */
    constructor(config) {
	this.initialized = false;
        if (typeof(config) === "undefined")
            config = {};
	if (Module.defaultModel !== null && config.hmm === undefined)
            config.hmm = Module.get_model_path(Module.defaultModel);
	const cjson = allocateUTF8OnStack(JSON.stringify(config));
        const cconfig = Module._config_parse_json(0, cjson);
	this.cdecoder = Module._decoder_create(cconfig);
	if (this.cdecoder == 0)
	    throw new Error("Failed to construct Decoder");
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
     * Get configuration as JSON.
     */
    get_config_json() {
        const cconfig = Module._decoder_config(this.cdecoder);
        const cjson = Module._config_serialize_json(cconfig);
        return UTF8ToString(cjson);
    }
    /**
     * Set a configuration parameter.
     * @param {string} key Parameter name.
     * @param {number|string} val Parameter value.
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
     * Reset a configuration parameter to its default value.
     * @param {string} key Parameter name.
     * @throws {ReferenceError} Throws ReferenceError if key is not a known parameter.
     */
    unset_config(key) {
	const ckey = allocateUTF8OnStack(key);
        const cconfig = Module._decoder_config(this.cdecoder);
	const type = Module._config_typeof(cconfig, ckey);
	if (type == 0) {
	    throw new ReferenceError("Unknown cmd_ln parameter "+key);
	}
        Module._config_set(cconfig, ckey, 0, type);
    }
    /**
     * Get a configuration parameter's value.
     * @param {string} key Parameter name.
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
     * Test if a key is a known parameter.
     * @param {string} key Key whose existence to check.
     */
    has_config(key) {
	const ckey = allocateUTF8OnStack(key);
        const cconfig = Module._decoder_config(this.cdecoder);
	return Module._config_typeof(cconfig, ckey) != 0;
    }
    /**
     * Initialize or reinitialize the decoder asynchronously.
     * @returns {Promise} Promise resolved once decoder is ready.
     */
    async initialize() {
	if (this.cdecoder == 0)
	    throw new Error("Decoder was somehow not constructed (ps==0)");
	await this.init_featparams()
	await this.init_cleanup();
	await this.init_fe();
	await this.init_feat();
	this.cacmod = await this.init_acmod();
	await this.load_acmod_files();
	await this.init_dict();
	await this.init_grammar();

	this.initialized = true;
    }

    /**
     * Read feature parameters from acoustic model.
     */
    async init_featparams() {
        const fpdata = await load_json(this.get_config("featparams"));
	for (const key in fpdata) {
	    if (this.has_config(key)) /* Sometimes it doesn't */
		this.set_config(key, fpdata[key]);
	}
        return fpdata;
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
        return rv;
    }

    /**
     * Create dynamic feature module from configuration.
     */
    async init_feat() {
	let rv;
	try {
	    const lda = await load_to_s3file(this.get_config("lda"));
	    rv = Module._decoder_init_feat_s3file(this.cdecoder, lda);
	}
	catch (e) {
	    rv = Module._decoder_init_feat_s3file(this.cdecoder, 0);
	}
	if (rv == 0)
	    throw new Error("Failed to initialize feature module");
        return rv;
    }

    /**
     * Create acoustic model from configuration.
     */
    async init_acmod() {
	const rv = Module._decoder_init_acmod_pre(this.cdecoder);
	if (rv == 0)
	    throw new Error("Failed to initialize acoustic model");
        return rv;
    }

    /**
     * Load acoustic model files
     */
    async load_acmod_files() {
	await this.load_mdef();
	await this.load_tmat(this.get_config("tmat"));
	const means = this.get_config("mean");
	const variances = this.get_config("var");
	const sendump = this.get_config("sendump");
	const mixw = this.get_config("mixw");
	await this.load_gmm(means, variances, sendump, mixw);
	const rv = Module._decoder_init_acmod_post(this.cdecoder);
	if (rv < 0)
	    throw new Error("Failed to initialize acoustic scoring");
    }

    /**
     * Load binary model definition file
     */
    async load_mdef() {
	const s3f = await load_to_s3file(this.get_config("mdef"));
	if (s3f == 0)
	    throw new Error("Failed to read mdef from " + this.get_config("mdef"));
        const mdef = Module._bin_mdef_read_s3file(s3f, this.get_config("cionly"));
	Module._s3file_free(s3f);
	if (mdef == 0)
	    throw new Error("Failed to read mdef from " + this.get_config("mdef"));
	Module._set_mdef(this.cdecoder, mdef);
        return mdef;
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
        return tmat;
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
        let dict;
        try {
	    dict = await load_to_s3file(this.get_config("dict"));
	}
	catch (e) {
	    dict = 0;
	}
	let fdict;
	try {
	    fdict = await load_to_s3file(this.get_config("fdict"));
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
        const fe = await this.init_fe();
        const fcb = await this.init_feat();
	if (Module._acmod_reinit_feat(this.cacmod, fe, fcb) < 0) {
	    throw new Error("Failed to reinitialize audio parameters");
        }
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
     * @param {Float32Array} pcm Audio data, in float32 format, in
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
     * Run alignment and get word and phone segmentation as JSON
     * @param start Start time to add to returned segment times.
     * @param align_level 0 for no subword alignments, 1 for phone
     *                    alignments, 2 for phone and state alignments.
     * @returns JSON with a detailed word and phone-level alignment.
     */
    async get_alignment_json(start=0.0, align_level=1) {
        this.assert_initialized();
        /* FIXME: This could block for some time, decompose it. */
        const cjson = Module._decoder_result_json(this.cdecoder, start,
                                                  align_level);
        return UTF8ToString(cjson);
    }

    /**
     * Look up a word in the pronunciation dictionary.
     * @param {string} word Text of word to look up.
     * @returns {string} Space-separated list of phones, or `null` if
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
     * @param {string} word Text of word to add.
     * @param {string} pron Pronunciation of word as space-separated list of phonemes.
     * @param {number} update Update decoder immediately (set to
     * false when adding a list of words, except for the last word).
     * @returns {Promise} Promise resolved once word has been added.
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
     * Set recognition grammar from a list of transitions asynchronously.
     * @param {string} name Name of grammar.
     * @param {number} start_state Index of starting state.
     * @param {number} final_state Index of ending state.
     * @param {Array<Object>} transitions Array of transitions, each
     * of which is an Object with the keys `from`, `to`, `word`, and
     * `prob`.  The word must exist in the dictionary.
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
                    throw new Error(`Failed to add word ${t.word} to FSG`);
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
     * @param {string} jsgf_string String containing JSGF grammar.
     * @param {string} [toprule] Name of starting rule for grammar,
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
	if (Module._decoder_set_fsg(this.cdecoder, fsg) < 0)
	    throw new Error("Failed to set FSG in decoder");
    }
    /**
     * Set word sequence for alignment.
     * @param {string} text Sentence to align, as whitespace-separated
     *                        words.  All words must be present in the
     *                        dictionary.
     */
    async set_align_text(text) {
	this.assert_initialized();
        const ctext = allocateUTF8OnStack(text);
        if (Module._decoder_set_align_text(this.cdecoder, ctext) < 0)
            throw new Error("Failed to set alignment text");
    }
};

/**
 * Simple endpointer using voice activity detection.
 */
class Endpointer {
    /**
     * Create the endpointer
     *
     * @param {number} [sample_rate] Sampling rate of the input audio.
     * @param {number} [frame_length] Length in seconds of an input
     * frame, must be 0.01, 0.02, or 0.03.
     * @param {number} [mode] Aggressiveness of voice activity detction,
     * must be 0, 1, 2, or 3.  Higher numbers will create "tighter"
     * endpoints at the possible expense of clipping the start of
     * utterances.
     * @param {number} [window] Length in seconds of the window used to
     * make a speech/non-speech decision.
     * @param {number} [ratio] Ratio of `window` that must be detected as
     * speech (or not speech) in order to trigger decision.
     * @throws {Error} on invalid parameters.
     */
    constructor(sample_rate, frame_length=0.03, mode=0, window=0.3, ratio=0.9) {
        this.cep = Module._endpointer_init(window, ratio, mode,
                                           sample_rate, frame_length);
        if (this.cep == 0)
            throw new Error("Invalid endpointer or VAD parameters");
    }

    /**
     * Get the effective length of a frame in samples.
     *
     * Note that you *must* pass this many samples in each input
     * frame, no more, no less.
     *
     * @returns {number} Size of required frame in samples.
     */
    get_frame_size() {
        return Module._vad_frame_size(Module._endpointer_vad(this.cep));
    }

    /**
     * Get the effective length of a frame in seconds (may be
     * different from the one requested in the constructor)
     *
     * @returns {number} Length of a frame in seconds.
     */
    get_frame_length() {
        return Module._vad_frame_length(Module._endpointer_vad(this.cep));
    }

    /**
     * Is the endpointer currently in a speech segment?
     * 
     * To detect transitions from non-speech to speech, check this
     * before process().  If it was `false` but process() returns
     * data, then speech has started.
     *
     * Likewise, to detect transitions from speech to non-speech, call
     * this *after* process().  If process() returned data but
     * this returns `false`, then speech has stopped.
     *
     * For example:
     *
     * .. code-block:: javascript
     *
     *     let prev_in_speech = ep.get_in_speech();
     *     let frame_size = ep.get_frame_size();
     *     // Presume `frame` is a Float32Array of frame_size or less
     *     let speech;
     *     if (frame.size < frame_size)
     *         speech = ep.end_stream(frame);
     *     else
     *         speech = ep.process(frame);
     *     if (speech !== null) {
     *         if (!prev_in_speech)
     *             console.log("Speech started at " + ep.get_speech_start());
     *         if (!ep.get_in_speech())
     *             console.log("Speech ended at " + ep.get_speech_end());
     *     }
     *
     * @returns {Boolean} are we currently in a speech region?
     */
    get_in_speech() {
        return Module._endpointer_in_speech(this.cep) != 0;
    }

    /**
     * Get start time of current speech region.
     * @returns {number} Time in seconds.
     */
    get_speech_start() {
        return Module._endpointer_speech_start(this.cep);
    }

    /**
     * Get end time of current speech region.
     * @returns {number} Time in seconds.
     */
    get_speech_end() {
        return Module._endpointer_speech_end(this.cep);
    }

    /**
     * Read a frame of data and return speech if detected.
     * @param {Float32Array} pcm Audio data, in float32 format, in
     * the range [-1.0, 1.0].  Must contain `get_frame_size()` samples.
     * @returns {Float32Array} Speech data, if any, or `null` if none.
     */
    process(frame) {
        // Have to convert it to int16 for (fixed-point) VAD
        const pcm_i16 = Int16Array.from(frame, x => (x > 0 ? x * 0x7FFF : x * 0x8000));
	const pcm_u8 = new Uint8Array(pcm_i16.buffer);
	// Emscripten documentation fails to mention that this
	// function specifically takes a Uint8Array
	const pcm_addr = Module._malloc(pcm_u8.length);
	writeArrayToMemory(pcm_u8, pcm_addr);
	const rv = Module._endpointer_process(this.cep, pcm_addr);
	Module._free(pcm_addr);
        if (rv != 0) {
            const pcm_i16 = new Int16Array(HEAP8.buffer, rv, this.get_frame_size() * 2);
            return Float32Array.from(pcm_i16, x => (x > 0 ? x / 0x7fff : x / 0x8000));
        }
        else
	    return null;
    }

    /**
     * Read a final frame of data and return speech if any.
     *
     * This function should only be called at the end of the input
     * stream (and then, only if you are currently in a speech
     * region).  It will return any remaining speech data detected by
     * the endpointer.
     *
     * @param {Float32Array} pcm Audio data, in float32 format, in
     * the range [-1.0, 1.0].  Must contain `get_frame_size()` samples
     * or less.
     * @returns {Float32Array} Speech data, if any, or `null` if none.
     */
    end_stream(frame) {
        // Have to convert it to int16 for (fixed-point) VAD
        const pcm_i16 = Int16Array.from(frame.map(x => (x > 0 ? x * 0x7FFF : x * 0x8000)))
	const pcm_u8 = new Uint8Array(pcm_i16.buffer);
	// Emscripten documentation fails to mention that this
	// function specifically takes a Uint8Array
	const pcm_addr = Module._malloc(pcm_u8.length);
	writeArrayToMemory(pcm_u8, pcm_addr);
        // FIXME: Depends on size_t being 32 bits on Emscripten (it is)
        const nsamp_addr = stackAlloc(4);
	const rv = Module._endpointer_end_stream(this.cep, pcm_addr, pcm_i16.length,
                                                 nsamp_addr);
	Module._free(pcm_addr);
	if (rv != 0) {
            const nsamp = getValue(nsamp_addr, 'i32');
            const pcm_i16 = new Int16Array(HEAP8.buffer, rv, nsamp * 2);
            return Float32Array.from(pcm_i16, x => (x > 0 ? x / 0x7fff : x / 0x8000));
        }
        else
            return null;
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
 * @param {string} subpath path to model directory or parameter
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
Module.Endpointer = Endpointer;

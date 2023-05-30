const ARG_INTEGER = 1 << 1;
const ARG_FLOATING = 1 << 2;
const ARG_STRING = 1 << 3;
const ARG_BOOLEAN = 1 << 4;

const DEFAULT_MODEL = "en-us";

if (typeof Module.modelBase === "undefined") {
  Module.modelBase = "model/";
}
if (typeof Module.defaultModel === "undefined") {
  Module.defaultModel = DEFAULT_MODEL;
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
    if (config === undefined) config = {};
    if (Module.defaultModel !== null && config.hmm === undefined)
      config.hmm = Module.get_model_path(Module.defaultModel);
    const cjson = stringToNewUTF8(JSON.stringify(config));
    const cconfig = Module._config_parse_json(0, cjson);
    Module._free(cjson);
    this.cdecoder = Module._decoder_create(cconfig);
    if (this.cdecoder == 0) throw new Error("Failed to construct Decoder");
  }
  /**
   * Free resources used by the decoder.
   */
  delete() {
    if (this.cdecoder != 0) Module._decoder_free(this.cdecoder);
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
    const ckey = stringToNewUTF8(key);
    const cconfig = Module._decoder_config(this.cdecoder);
    const type = Module._config_typeof(cconfig, ckey);
    if (type == 0) {
      Module._free(ckey);
      throw new ReferenceError(`Unknown configuration parameter ${key}`);
    }
    if (type & ARG_STRING) {
      const cval = stringToNewUTF8(val);
      Module._config_set_str(cconfig, ckey, cval);
      Module._free(cval);
    } else if (type & ARG_FLOATING) {
      Module._config_set_float(cconfig, ckey, val);
    } else if (type & (ARG_INTEGER | ARG_BOOLEAN)) {
      Module._config_set_int(cconfig, ckey, val);
    } else {
      Module._free(ckey);
      return false;
    }
    Module._free(ckey);
    return true;
  }
  /**
   * Reset a configuration parameter to its default value.
   * @param {string} key Parameter name.
   * @throws {ReferenceError} Throws ReferenceError if key is not a known parameter.
   */
  unset_config(key) {
    const ckey = stringToNewUTF8(key);
    const cconfig = Module._decoder_config(this.cdecoder);
    const type = Module._config_typeof(cconfig, ckey);
    if (type == 0) {
      Module._free(ckey);
      throw new ReferenceError(`Unknown configuration parameter ${key}`);
    }
    Module._config_set(cconfig, ckey, 0, type);
    Module._free(ckey);
  }
  /**
   * Get a configuration parameter's value.
   * @param {string} key Parameter name.
   * @returns {number|string} Parameter value.
   * @throws {ReferenceError} Throws ReferenceError if key is not a known parameter.
   */
  get_config(key) {
    const ckey = stringToNewUTF8(key);
    const cconfig = Module._decoder_config(this.cdecoder);
    const type = Module._config_typeof(cconfig, ckey);
    if (type == 0) {
      Module._free(ckey);
      throw new ReferenceError(`Unknown configuration parameter ${key}`);
    }
    let rv;
    if (type & ARG_STRING) {
      const val = Module._config_str(cconfig, ckey);
      if (val == 0) rv = null;
      else rv = UTF8ToString(val);
    } else if (type & ARG_FLOATING) {
      rv = Module._config_float(cconfig, ckey);
    } else if (type & ARG_INTEGER) {
      rv = Module._config_int(cconfig, ckey);
    } else if (type & ARG_BOOLEAN) {
      rv = Boolean(Module._config_int(cconfig, ckey));
    }
    Module._free(ckey);
    if (rv === undefined)
      throw new TypeError(`Unsupported type ${type} for parameter ${key}`);
    return rv;
  }
  /**
   * Test if a key is a known parameter.
   * @param {string} key Key whose existence to check.
   */
  has_config(key) {
    const ckey = stringToNewUTF8(key);
    const cconfig = Module._decoder_config(this.cdecoder);
    const rv = Module._config_typeof(cconfig, ckey) != 0;
    Module._free(ckey);
    return rv;
  }
  /**
   * Initialize or reinitialize the decoder asynchronously.
   * @returns {Promise} Promise resolved once decoder is ready.
   */
  async initialize() {
    if (this.cdecoder == 0)
      throw new Error("Decoder was somehow not constructed (ps==0)");
    await this.init_featparams();
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
      if (this.has_config(key))
        /* Sometimes it doesn't */
        this.set_config(key, fpdata[key]);
    }
    return fpdata;
  }

  /**
   * Clean up any lingering search modules.
   */
  async init_cleanup() {
    let rv = Module._decoder_init_cleanup(this.cdecoder);
    if (rv < 0) throw new Error("Failed to clean up decoder internals");
  }

  /**
   * Create front-end from configuration.
   */
  async init_fe() {
    const rv = Module._decoder_init_fe(this.cdecoder);
    if (rv == 0) throw new Error("Failed to initialize frontend");
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
    } catch (e) {
      rv = Module._decoder_init_feat_s3file(this.cdecoder, 0);
    }
    if (rv == 0) throw new Error("Failed to initialize feature module");
    return rv;
  }

  /**
   * Create acoustic model from configuration.
   */
  async init_acmod() {
    const rv = Module._decoder_init_acmod_pre(this.cdecoder);
    if (rv == 0) throw new Error("Failed to initialize acoustic model");
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
    if (rv < 0) throw new Error("Failed to initialize acoustic scoring");
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
    if (tmat == 0) throw new Error("Failed to read tmat");
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
    } catch (e) {
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
    } catch (e) {
      dict = 0;
    }
    let fdict;
    try {
      fdict = await load_to_s3file(this.get_config("fdict"));
    } catch (e) {
      fdict = 0;
    }
    const rv = Module._decoder_init_dict_s3file(this.cdecoder, dict, fdict);
    if (rv == 0) throw new Error("Failed to initialize dictionaries");
  }

  /**
   * Load grammar from configuration.
   */
  async init_grammar() {
    let fsg = 0,
      jsgf = 0;
    const jsgf_path = this.get_config("jsgf");
    if (jsgf_path != null) jsgf = await load_to_s3file(jsgf_path);
    const fsg_path = this.get_config("fsg");
    if (fsg_path != null) fsg = await load_to_s3file(fsg_path);
    if (fsg || jsgf) {
      const rv = Module._decoder_init_grammar_s3file(this.cdecoder, fsg, jsgf);
      if (rv < 0) throw new Error("Failed to initialize grammar");
    }
  }

  /**
   * Throw an error if decoder is not initialized.
   * @throws {Error} If decoder is not initialized.
   */
  assert_initialized() {
    if (!this.initialized) throw new Error("Decoder not yet initialized");
  }

  /**
   * Re-initialize only the audio feature extraction.
   * @returns {Promise} Promise resolved once reinitialized.
   */
  async reinitialize_audio() {
    this.assert_initialized();
    const fe = await this.init_fe();
    const fcb = await this.init_feat();
    if (Module._acmod_reinit_feat(this.cacmod, fe, fcb) < 0)
      throw new Error("Failed to reinitialize audio parameters");
  }

  /**
   * Start processing input.
   */
  start() {
    this.assert_initialized();
    if (Module._decoder_start_utt(this.cdecoder) < 0)
      throw new Error("Failed to start utterance processing");
  }

  /**
   * Finish processing input.
   */
  stop() {
    this.assert_initialized();
    if (Module._decoder_end_utt(this.cdecoder) < 0)
      throw new Error("Failed to stop utterance processing");
  }

  /**
   * Process a block of audio data.
   * @param {Float32Array} pcm Audio data, in float32 format, in
   * the range [-1.0, 1.0].
   * @returns Number of frames processed.
   */
  process_audio(pcm, no_search = false, full_utt = false) {
    this.assert_initialized();
    const pcm_bytes = pcm.length * pcm.BYTES_PER_ELEMENT;
    const pcm_addr = Module._malloc(pcm_bytes);
    // This Javascript API is rather stupid.  DO NOT forget byteOffset and length.
    const pcm_u8 = new Uint8Array(pcm.buffer, pcm.byteOffset, pcm_bytes);
    // Emscripten documentation (what documentation) fails to
    // mention that this function specifically takes a Uint8Array
    writeArrayToMemory(pcm_u8, pcm_addr);
    const rv = Module._decoder_process_float32(
      this.cdecoder,
      pcm_addr,
      pcm_bytes / 4,
      no_search,
      full_utt
    );
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
  get_text() {
    this.assert_initialized();
    return UTF8ToString(Module._decoder_hyp(this.cdecoder, 0));
  }

  /**
   * Get the current recognition result as a word (and possibly phone) segmentation.
   * @param {Object} config
   * @param {number} config.start Start time to add to returned segment times.
   * @param {number} config.align_level 0 for no word alignments only, 1 for wor
    and phone alignments, 2 for word, phone and state alignments.
   * @returns {Array<Segment>} Array of segments for the words recognized, each
   * with the keys `t`, `b` and `d`, for text, start time, and duration,
   * respectively.
   */
  get_alignment({ start = 0.0, align_level = 0 } = {}) {
    this.assert_initialized();
    if (align_level > 2) throw new Error(`Invalid align_level ${align_level}`);
    const cjson = Module._decoder_result_json(
      this.cdecoder,
      start,
      align_level
    );
    const json = UTF8ToString(cjson);
    return JSON.parse(json);
  }

  /**
   * Look up a word in the pronunciation dictionary.
   * @param {string} word Text of word to look up.
   * @returns {string} Space-separated list of phones, or `null` if
   * word is not in the dictionary.
   */
  lookup_word(word) {
    this.assert_initialized();
    const cword = stringToNewUTF8(word);
    const cpron = Module._decoder_lookup_word(this.cdecoder, cword);
    Module._free(cword);
    if (cpron == 0) return null;
    return UTF8ToString(cpron);
  }

  /**
   * Add words to the pronunciation dictionary.
   *
   * Example:
   *
   *    decoder.add_words(["hello", "H EH L OW"], ["world", "W ER L D"]);
   *
   * @param (...Array) words Any number of 2-element arrays containing the
   *        word text in position 0 and a string with whitespace-separated
   *        phones in position 1.
   */
  add_words(...words) {
    this.assert_initialized();
    for (let i = 0; i < words.length; ++i) {
      const [text, pron] = words[i];
      if (text === undefined || pron === undefined)
        throw new Error(
          `Word at position ${i} has missing text or pronunciation`
        );
      const ctext = stringToNewUTF8(text);
      const cpron = stringToNewUTF8(pron);
      const update = i == words.length - 1;
      const wid = Module._decoder_add_word(this.cdecoder, ctext, cpron, update);
      Module._free(ctext);
      Module._free(cpron);
      if (wid < 0)
        throw new Error(`Failed to add "${word}:${pron}" to the dictionary`);
    }
  }

  /**
   * Set recognition grammar from JSGF.
   * @param {string} jsgf_string String containing JSGF grammar.
   * @param {string} [toprule] Name of starting rule for grammar,
   * if not specified, the first public rule will be used.
   */
  set_grammar(jsgf_string, toprule = null) {
    this.assert_initialized();
    const logmath = Module._decoder_logmath(this.cdecoder);
    const config = Module._decoder_config(this.cdecoder);
    const lw = this.get_config("lw");
    const cjsgf = stringToNewUTF8(jsgf_string);
    const jsgf = Module._jsgf_parse_string(cjsgf, 0);
    Module._free(cjsgf);
    if (jsgf == 0) throw new Error("Failed to parse JSGF");
    let rule;
    if (toprule !== null) {
      const crule = stringToNewUTF8(toprule);
      rule = Module._jsgf_get_rule(jsgf, crule);
      Module._free(crule);
      if (rule == 0) throw new Error("Failed to find top rule " + toprule);
    } else {
      rule = Module._jsgf_get_public_rule(jsgf);
      if (rule == 0) throw new Error("No public rules found in JSGF");
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
  set_align_text(text) {
    this.assert_initialized();
    const ctext = stringToNewUTF8(text);
    const rv = Module._decoder_set_align_text(this.cdecoder, ctext);
    Module._free(ctext);
    if (rv < 0) throw new Error("Failed to set alignment text");
  }
  /**
   * Compute spectrogram from audio
   * @param {Float32Array} pcm Audio data, in float32 format, in
   * the range [-1.0, 1.0].
   * @returns {Promise<FeatureBuffer>} Promise resolved to an object
   * containing `data`, `nfr`, and `nfeat` properties.
   */
  spectrogram(pcm) {
    this.assert_initialized();
    const cfe = Module._decoder_fe(this.cdecoder);
    if (cfe == 0) throw new Error("Could not get front end from decoder");
    /* Unfortunately we have to copy the data into the heap space,
     * create the spectrum in heap space, then copy it out. */
    const pcm_bytes = pcm.length * pcm.BYTES_PER_ELEMENT;
    const pcm_addr = Module._malloc(pcm_bytes);
    // This Javascript API is rather stupid.  DO NOT forget byteOffset and length.
    const pcm_u8 = new Uint8Array(pcm.buffer, pcm.byteOffset, pcm_bytes);
    writeArrayToMemory(pcm_u8, pcm_addr);
    /* Note, pointers and size_t are 4 bytes */
    const shape = Module._malloc(8);
    const cpfeats = Module._spectrogram(
      cfe,
      pcm_addr,
      pcm_bytes / 4,
      shape,
      shape + 4
    );
    if (cpfeats == 0) throw new Error("Spectrogram calculation failed");
    Module._free(pcm_addr);
    const cfeats = getValue(cpfeats, "*");
    const nfr = getValue(shape, "*");
    const nfeat = getValue(shape + 4, "*");
    Module._free(shape);
    const data = new Float32Array(
      /* This copies the data, which is what we want. */
      HEAP8.slice(cfeats, cfeats + nfr * nfeat * 4).buffer
    );
    Module._ckd_free_2d(cpfeats);
    return { data, nfr, nfeat };
  }
}

/**
 * Simple endpointer using voice activity detection.
 */
class Endpointer {
  /**
   * Create the endpointer
   *
   * @param {Object} config
   * @param {number} config.samprate Sampling rate of the input audio.
   * @param {number} config.frame_length Length in seconds of an input
   * frame, must be 0.01, 0.02, or 0.03.
   * @param {number} config.mode Aggressiveness of voice activity detction,
   * must be 0, 1, 2, or 3.  Higher numbers will create "tighter"
   * endpoints at the possible expense of clipping the start of
   * utterances.
   * @param {number} config.window Length in seconds of the window used to
   * make a speech/non-speech decision.
   * @param {number} config.ratio Ratio of `window` that must be detected as
   * speech (or not speech) in order to trigger decision.
   * @throws {Error} on invalid parameters.
   */
  constructor({
    samprate,
    frame_length = 0.03,
    mode = 0,
    window = 0.3,
    ratio = 0.9,
  } = {}) {
    this.cep = Module._endpointer_init(
      window,
      ratio,
      mode,
      samprate,
      frame_length
    );
    if (this.cep == 0) throw new Error("Invalid endpointer or VAD parameters");
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
    const pcm_i16 = Int16Array.from(frame, (x) =>
      x > 0 ? x * 0x7fff : x * 0x8000
    );
    const pcm_u8 = new Uint8Array(pcm_i16.buffer);
    // Emscripten documentation fails to mention that this
    // function specifically takes a Uint8Array
    const pcm_addr = Module._malloc(pcm_u8.length);
    writeArrayToMemory(pcm_u8, pcm_addr);
    const rv = Module._endpointer_process(this.cep, pcm_addr);
    Module._free(pcm_addr);
    if (rv != 0) {
      /* Yes, the *offset* is in bytes, but the *length* is in
       * items.  No, I don't know what Google/Apple/Mozilla/Microsoft/W3C
       * brain genius thought that was a good idea (it is logical, and yet
       * very much not obvious... */
      const pcm_i16 = new Int16Array(HEAP8.buffer, rv, this.get_frame_size());
      return Float32Array.from(pcm_i16, (x) =>
        x > 0 ? x / 0x7fff : x / 0x8000
      );
    } else return null;
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
    const pcm_i16 = Int16Array.from(
      frame.map((x) => (x > 0 ? x * 0x7fff : x * 0x8000))
    );
    const pcm_u8 = new Uint8Array(pcm_i16.buffer);
    // Emscripten documentation fails to mention that this
    // function specifically takes a Uint8Array
    const pcm_addr = Module._malloc(pcm_u8.length);
    writeArrayToMemory(pcm_u8, pcm_addr);
    // FIXME: Depends on size_t being 32 bits on Emscripten (it is)
    const nsamp_addr = Module._malloc(4);
    const rv = Module._endpointer_end_stream(
      this.cep,
      pcm_addr,
      pcm_i16.length,
      nsamp_addr
    );
    Module._free(nsamp_addr);
    Module._free(pcm_addr);
    if (rv != 0) {
      const nsamp = getValue(nsamp_addr, "i32");
      const pcm_i16 = new Int16Array(HEAP8.buffer, rv, nsamp * 2);
      return Float32Array.from(pcm_i16, (x) =>
        x > 0 ? x / 0x7fff : x / 0x8000
      );
    } else return null;
  }
}

Module.get_model_path = get_model_path;
Module.load_json = load_json;
Module.Decoder = Decoder;
Module.Endpointer = Endpointer;

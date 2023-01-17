# cython: embedsignature=True, language_level=3
# Copyright (c) 2008-2020 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhdaines@gmail.com>

from libc.stdlib cimport free, malloc

import itertools
import logging

import soundswallower

cimport _soundswallower

LOGGER = logging.getLogger("soundswallower")

cdef class Config:
    """Configuration object for SoundSwallower.

    The SoundSwallower can be configured either implicitly, by passing
    keyword arguments to `Decoder`, or by creating and manipulating
    `Config` objects.  There are a large number of parameters, most of
    which are not important or subject to change.  These mostly
    correspond to the command-line arguments used by PocketSphinx.

    A `Config` can be initialized with keyword arguments::

        config = Config(hmm="path/to/things", dict="my.dict")

    It can also be initialized by parsing JSON (either as bytes or str)::

        config = Config.parse_json('''{"hmm": "path/to/things",
                                       "dict": "my.dict"}''')

    The "parser" is very much not strict, so you can also pass a sort
    of pseudo-YAML to it, e.g.::

        config = Config.parse_json("hmm: path/to/things, dict: my.dict")

    You can also initialize an empty `Config` and set arguments in it
    directly::

        config = Config()
        config["hmm"] = "path/to/things"

    In general, a `Config` mostly acts like a dictionary, and can be
    iterated over in the same fashion.  However, attempting to access
    a parameter that does not already exist will raise a `KeyError`.

    See :doc:`config_params` for a description of existing parameters.

 """
    cdef config_t *config

    # This is __init__ so we can bypass it if necessary
    def __init__(self, **kwargs):
        self.config = config_init(NULL);
        for k, v in kwargs.items():
            ckey = self._normalize_key(k)
            self[ckey] = v

    @staticmethod
    cdef create_from_ptr(config_t *config):
        cdef Config self = Config.__new__(Config)
        self.config = config
        return self

    @staticmethod
    def parse_json(json):
        """Parse JSON (or pseudo-YAML) configuration

        Args:
            json(bytes|str): JSON data.
        Returns:
            Config: Parsed config, or None on error.
        """
        cdef config_t *config
        if not isinstance(json, bytes):
            json = json.encode("utf-8")
        config = config_parse_json(NULL, json)
        if config == NULL:
            return None
        return Config.create_from_ptr(config)

    def dumps(self):
        """Serialize configuration to a JSON-formatted `str`.

        This produces JSON from a configuration object, with default
        values included.

        Returns:
            str: Serialized JSON
        Raises:
            RuntimeError: if serialization fails somehow.
        """
        cdef const char *json = config_serialize_json(self.config)
        if json == NULL:
            raise RuntimeError("JSON serialization failed")
        return json.decode("utf-8")

    def __dealloc__(self):
        config_free(self.config)

    cdef _normalize_key(self, key):
        if isinstance(key, bytes):
            # Assume already normalized
            return key
        else:
            if key[0] in "-_":
                key = key[1:]
            return key.encode('utf-8')

    def __contains__(self, key):
        return config_typeof(self.config, self._normalize_key(key)) != 0

    def __getitem__(self, key):
        cdef const char *cval
        cdef const anytype_t *at;
        cdef config_type_t t

        ckey = self._normalize_key(key)
        at = config_get(self.config, ckey)
        if at == NULL:
            raise KeyError("Unknown key %s" % key)
        t = config_typeof(self.config, ckey)
        if t & ARG_STRING:
            cval = <const char *>at.ptr
            if cval == NULL:
                return None
            else:
                return cval.decode('utf-8')
        elif t & ARG_INTEGER:
            return at.i
        elif t & ARG_FLOATING:
            return at.fl
        elif t & ARG_BOOLEAN:
            return bool(at.i)
        else:
            raise ValueError("Unable to handle parameter type %d" % t)

    def __setitem__(self, key, val):
        cdef config_type_t t
        ckey = self._normalize_key(key)
        t = config_typeof(self.config, ckey)
        if t == 0:
            raise KeyError("Unknown key %s" % key)
        if t & ARG_STRING:
            if val is None:
                config_set_str(self.config, ckey, NULL)
            else:
                config_set_str(self.config, ckey, str(val).encode('utf-8'))
        elif t & ARG_INTEGER:
            config_set_int(self.config, ckey, int(val))
        elif t & ARG_FLOATING:
            config_set_float(self.config, ckey, float(val))
        elif t & ARG_BOOLEAN:
            config_set_bool(self.config, ckey, bool(val))
        else:
            raise ValueError("Unable to handle parameter type %d" % t)

    def __delitem__(self, key):
        cdef config_type_t t
        ckey = self._normalize_key(key)
        t = config_typeof(self.config, ckey)
        if t == 0:
            raise KeyError("Unknown key %s" % key)
        config_set(self.config, ckey, NULL, t)

    def __iter__(self):
        cdef hash_table_t *ht = self.config.ht
        cdef hash_iter_t *itor
        itor = hash_table_iter(self.config.ht)
        while itor != NULL:
            ckey = hash_entry_key(itor.ent)
            yield ckey.decode('utf-8')
            itor = hash_table_iter_next(itor)

    def items(self):
        for key in self:
            yield (key, self[key])

    def __len__(self):
        # Incredibly, the only way to do this
        return sum(1 for _ in self)

    def describe(self):
        """Iterate over parameter descriptions.

        This function returns a generator over the parameters defined
        in a configuration, as `Arg` objects.

        Returns:
            Iterable[Arg]: Descriptions of parameters including their
            default values and documentation

        """
        cdef const config_param_t *arg = self.config.defn
        cdef int base_type
        while arg != NULL and arg.name != NULL:
            name = arg.name.decode('utf-8')
            if name[0] == '-':
                name = name[1:]
            if arg.deflt == NULL:
                default = None
            else:
                default = arg.deflt.decode('utf-8')
            if arg.doc == NULL:
                doc = None
            else:
                doc = arg.doc.decode('utf-8')
            required = (arg.type & ARG_REQUIRED) != 0
            base_type = arg.type & ~ARG_REQUIRED
            if base_type == ARG_INTEGER:
                arg_type = int
            elif base_type == ARG_FLOATING:
                arg_type = float
            elif base_type == ARG_STRING:
                arg_type = str
            elif base_type == ARG_BOOLEAN:
                arg_type = bool
            else:
                raise ValueError("Unknown type %d in argument %s"
                                 % (base_type, name))
            arg = arg + 1
            yield soundswallower.Arg(name=name, default=default, doc=doc,
                                     type=arg_type, required=required)

cdef class FsgModel:
    """Finite-state recognition grammar.

    Note that you *cannot* create one of these directly, as it depends
    on some internal configuration from the `Decoder`.  Use the
    factory methods such as `create_fsg` or `parse_jsgf` instead.
    """
    cdef fsg_model_t *fsg

    def __cinit__(self):
        # Unsure this is the right way to do this.  The FsgModel
        # constructor isn't meant to be called from Python.
        self.fsg = NULL

    def __dealloc__(self):
        fsg_model_free(self.fsg)


cdef class Decoder:
    """Main class for speech recognition and alignment in SoundSwallower.

    See :doc:`config_params` for a description of keyword arguments.

    Args:
        hmm(str): Path to directory containing acoustic model files.
        dict(str): Path to pronunciation dictionary.
        jsgf(str): Path to JSGF grammar file.
        fsg(str): Path to FSG grammar file (only one of ``jsgf`` or ``fsg`` should
                  be specified).
        toprule(str): Name of top-level rule in JSGF file to use as entry point.
        samprate(float): Sampling rate for raw audio data.
        logfn(str): File to write log messages to (set to `os.devnull` to
                    silence these messages)
    Raises:
        ValueError: on invalid configuration options.
        RuntimeError: on failure to create decoder.
    """
    cdef decoder_t *_ps

    def __init__(self, *args, **kwargs):
        cdef Config config
        if len(args) == 1 and isinstance(args[0], Config):
            config = args[0]
        else:
            config = Config(*args, **kwargs)
        if config is None:
            raise ValueError, "Invalid configuration"
        # Python owns it but so does the decoder now
        self._ps = decoder_init(config_retain(config.config))
        if self._ps == NULL:
            raise RuntimeError("Failed to initialize decoder")

    @staticmethod
    def create(*args, **kwargs):
        """Create and configure, but do not initialize, the decoder.

        This method exists if you wish to override or unset some of the
        parameters which are filled in automatically when the model is
        loaded, in particular ``dict``, if you wish to create a dictionary
        programmatically rather than loading from a file.  For example:

            d = Decoder.create()
            del d.config["dict"]
            d.initialize()
            d.add_word("word", "W ER D", True) # Just one word! (or more)

        See :doc:`config_params` for a description of keyword arguments.

        Args:
            hmm(str): Path to directory containing acoustic model files.
            dict(str): Path to pronunciation dictionary.
            jsgf(str): Path to JSGF grammar file.
            fsg(str): Path to FSG grammar file (only one of ``jsgf`` or ``fsg`` should
                      be specified).
            toprule(str): Name of top-level rule in JSGF file to use as entry point.
            samprate(float): Sampling rate for raw audio data.
            logfn(str): File to write log messages to (set to `os.devnull` to
                        silence these messages)
        Raises:
            ValueError: on invalid configuration options.
        """
        cdef Config config
        cdef Decoder self
        if len(args) == 1 and isinstance(args[0], Config):
            config = args[0]
        else:
            config = Config(*args, **kwargs)
        if config is None:
            raise ValueError, "Invalid configuration"
        self = Decoder.__new__(Decoder)
        self._ps = decoder_create(config_retain(config.config))
        if self._ps == NULL:
            raise RuntimeError("Failed to create decoder")
        return self

    def __dealloc__(self):
        decoder_free(self._ps)

    def initialize(self, Config config=None):
        """(Re-)initialize the decoder.

        Args:
            config(Config): Optional new configuration to apply, otherwise
                            the existing configuration in the `config`
                            attribute will be reloaded.
        Raises:
            RuntimeError: On invalid configuration or other failure to
                          reinitialize decoder.
        """
        cdef config_t *cconfig
        if config is None:
            cconfig = NULL
        else:
            self.config = config
            # Because decoder owns configs, but Python does too
            cconfig = config_retain(config.config)
        if decoder_reinit(self._ps, cconfig) != 0:
            raise RuntimeError("Failed to initialize decoder")

    def reinit_feat(self, Config config=None):
        """Reinitialize only the feature computation.

        Args:
            config(Config): Optional new configuration to apply, otherwise
                            the existing configuration in the `config`
                            attribute will be reloaded.
        Raises:
            RuntimeError: On invalid configuration or other failure to
                          initialize feature computation.
        """
        cdef config_t *cconfig
        if config is None:
            cconfig = NULL
        else:
            self.config = config
            cconfig = config_retain(config.config)
        if decoder_reinit_feat(self._ps, cconfig) < 0:
            raise RuntimeError("Failed to reinitialize feature extraction")

    @property
    def config(self):
        """Property containing configuration object.

        You may use this to set or unset configuration options, but be
        aware that you must call `initialize` to apply them, e.g.:

            del decoder.config["dict"]
            decoder.config["loglevel"] = "INFO"
            decoder.config["bestpath"] = True
            decoder.initialize()

        Note that this will also remove any dictionary words or
        grammars which you have loaded.  See `reinit_feat` for an
        alternative if you do not wish to do this.

        Returns:
            Config: decoder configuration.
        """
        # Decoder owns the config, so we must retain it
        cdef config_t *config = decoder_config(self._ps)
        return Config.create_from_ptr(config_retain(config))

    def update_cmn(self):
        """Update current cepstral mean.

        Returns:
          str: New cepstral mean as a comma-separated list of numbers.
        """
        cdef const char *cmn = decoder_get_cmn(self._ps, True)
        return cmn.decode("utf-8")

    @property
    def cmn(self):
        """Get current cepstral mean.

        Args:
          update(boolean): Update the mean based on current utterance.
        Returns:
          str: Cepstral mean as a comma-separated list of numbers.
        """
        cdef const char *cmn = decoder_get_cmn(self._ps, False)
        return cmn.decode("utf-8")

    @cmn.setter
    def cmn(self, cmn):
        """Set current cepstral mean.

        Args:
          cmn(str): Cepstral mean as a comma-separated list of numbers.
        """
        cdef int rv = decoder_set_cmn(self._ps, cmn.encode("utf-8"))
        if rv != 0:
            raise ValueError("Invalid CMN string")

    def start_utt(self):
        """Start processing raw audio input.

        This method must be called at the beginning of each separate
        "utterance" of raw audio input.

        Raises:
            RuntimeError: If processing fails to start (usually if it
                          has already been started).
        """
        if decoder_start_utt(self._ps) < 0:
            raise RuntimeError, "Failed to start utterance processing"

    def process_raw(self, data, no_search=False, full_utt=False):
        """Process a block of raw audio.

        Args:
            data(bytes): Raw audio data, a block of 16-bit signed integer binary data.
            no_search(bool): If `True`, do not do any decoding on this data.
            full_utt(bool): If `True`, assume this is the entire utterance, for
                            purposes of acoustic normalization.
        Raises:
            RuntimeError: If processing fails.
        """
        cdef const unsigned char[:] cdata = data
        cdef Py_ssize_t n_samples = len(cdata) // 2
        if decoder_process_int16(self._ps, <const short *>&cdata[0],
                            n_samples, no_search, full_utt) < 0:
            raise RuntimeError, "Failed to process %d samples of audio data" % len / 2

    def end_utt(self):
        """Finish processing raw audio input.

        This method must be called at the end of each separate
        "utterance" of raw audio input.  It takes care of flushing any
        internal buffers and finalizing recognition results.

        """
        if decoder_end_utt(self._ps) < 0:
            raise RuntimeError, "Failed to stop utterance processing"

    @property
    def hyp(self):
        """Current recognition hypothesis.

        Returns:
            Hyp: Current recognition output.

        """
        cdef const char *hyp
        cdef logmath_t *lmath
        cdef int score

        hyp = decoder_hyp(self._ps, &score)
        if hyp == NULL:
             return soundswallower.Hyp(text=None, score=0., prob=0.)
        lmath = decoder_logmath(self._ps)
        prob = decoder_prob(self._ps)
        return soundswallower.Hyp(text=hyp.decode('utf-8'),
                                  score=logmath_exp(lmath, score),
                                  prob=logmath_exp(lmath, prob))

    def add_word(self, str word, str phones, update=True):
        """Add a word to the pronunciation dictionary.

        Args:
            word(str): Text of word to be added.
            phones(str): Space-separated list of phones for this
                         word's pronunciation.  This will depend on
                         the underlying acoustic model but is probably
                         in ARPABET.  FIXME: Should accept IPA, duh.
            update(bool): Update the recognizer immediately.  You can
                          set this to `False` if you are adding a lot
                          of words, to speed things up.  FIXME: This
                          API is bad and will be changed.
        Returns:
            int: Word ID of added word.
        Raises:
            KeyError: If word already exists in dictionary.
        """
        cdef rv = decoder_add_word(self._ps, word.encode("utf-8"),
                                   phones.encode("utf-8"), update)
        if rv < 0:
            raise KeyError("Word %s already exists" % word)

    def lookup_word(self, str word):
        """Look up a word in the dictionary and return phone transcription
        for it.

        Args:
            word(str): Text of word to search for.
        Returns:
            str: Space-separated list of phones, or None if not found.
        """
        cdef char *cphones
        cphones = decoder_lookup_word(self._ps, word.encode("utf-8"))
        if cphones == NULL:
            return None
        else:
            phones = cphones.decode("utf-8")
            free(cphones)
            return phones

    @property
    def seg(self):
        """Current word segmentation.

        Returns:
            Iterable[Seg]: Generator over word segmentations.

        """
        cdef config_t *cconfig = decoder_config(self._ps)
        cdef logmath_t *lmath = decoder_logmath(self._ps)
        cdef seg_iter_t *itor = decoder_seg_iter(self._ps)
        cdef int frate = config_int(cconfig, "frate")
        cdef int prob, ascr, lscr, sf, ef
        if itor == NULL:
            return
        while itor != NULL:
            seg_iter_frames(itor, &sf, &ef)
            prob = seg_iter_prob(itor, &ascr, &lscr)
            yield soundswallower.Seg(
                text=seg_iter_word(itor).decode('utf-8'),
                start=<double>sf / frate,
                duration=<double>(ef + 1 - sf) / frate,
                ascore=logmath_exp(lmath, ascr),
                lscore=logmath_exp(lmath, lscr))
            itor = seg_iter_next(itor)

    def read_fsg(self, filename):
        """Read a grammar from an FSG file.

        Args:
            filename(str): Path to FSG file.

        Returns:
            FsgModel: Newly loaded finite-state grammar.
        """
        cdef config_t *cconfig = decoder_config(self._ps)
        cdef logmath_t *lmath = decoder_logmath(self._ps)
        cdef float lw

        lw = config_float(cconfig, "lw")
        fsg = FsgModel()
        # FIXME: not the proper way to encode filenames on Windows, I think
        fsg.fsg = fsg_model_readfile(filename.encode(), lmath, lw)
        if fsg.fsg == NULL:
            raise RuntimeError("Failed to read FSG from %s" % filename)
        return fsg

    def read_jsgf(self, filename):
        """Read a grammar from a JSGF file.

        The top rule used is the one specified by the "toprule"
        configuration parameter.

        Args:
            filename(str): Path to JSGF file.
        Returns:
            FsgModel: Newly loaded finite-state grammar.
        """
        cdef config_t *cconfig = decoder_config(self._ps)
        cdef logmath_t *lmath = decoder_logmath(self._ps)
        cdef float lw

        lw = config_float(cconfig, "lw")
        fsg = FsgModel()
        fsg.fsg = jsgf_read_file(filename.encode(), lmath, lw)
        if fsg.fsg == NULL:
            raise RuntimeError("Failed to read JSGF from %s" % filename)
        return fsg

    def create_fsg(self, name, start_state, final_state, transitions):
        """Create a finite-state grammar.

        This method allows the creation of a grammar directly from a
        list of transitions.  States and words will be created
        implicitly from the state numbers and word strings present in
        this list.  Make sure that the pronunciation dictionary
        contains the words, or you will not be able to recognize.
        Basic usage::

            fsg = decoder.create_fsg("mygrammar",
                                     start_state=0, final_state=3,
                                     transitions=[(0, 1, 0.75, "hello"),
                                                  (0, 1, 0.25, "goodbye"),
                                                  (1, 2, 0.75, "beautiful"),
                                                  (1, 2, 0.25, "cruel"),
                                                  (2, 3, 1.0, "world")])

        Args:
            name(str): Name to give this FSG (not very important).
            start_state(int): Index of starting state.
            final_state(int): Index of end state.
            transitions(list): List of transitions, each of which is a 3-
                               or 4-tuple of (from, to, probability[, word]).
                               If the word is not specified, this is an
                               epsilon (null) transition that will always be
                               followed.

        Returns:
            FsgModel: Newly created finite-state grammar.
        Raises:
            ValueError: On invalid input.
        """
        cdef config_t *cconfig = decoder_config(self._ps)
        cdef logmath_t *lmath = decoder_logmath(self._ps)
        cdef float lw
        cdef int wid

        lw = config_float(cconfig, "lw")
        fsg = FsgModel()
        n_state = max(itertools.chain(*((t[0], t[1]) for t in transitions))) + 1
        fsg.fsg = fsg_model_init(name.encode("utf-8"), lmath, lw, n_state)
        fsg.fsg.start_state = start_state
        fsg.fsg.final_state = final_state
        for t in transitions:
            source, dest, prob = t[0:3]
            if len(t) > 3:
                word = t[3]
                wid = fsg_model_word_add(fsg.fsg, word.encode("utf-8"))
                if wid == -1:
                    raise RuntimeError("Failed to add word to FSG: %s" % word)
                fsg_model_trans_add(fsg.fsg, source, dest,
                                    logmath_log(lmath, prob), wid)
            else:
                fsg_model_null_trans_add(fsg.fsg, source, dest,
                                         logmath_log(lmath, prob))
        return fsg

    def parse_jsgf(self, jsgf_string, toprule=None):
        """Parse a JSGF grammar from bytes or string.

        Because SoundSwallower uses UTF-8 internally, it is more
        efficient to parse from bytes, as a string will get encoded
        and subsequently decoded.

        Args:
            jsgf_string(bytes|str): JSGF grammar as string or UTF-8
                                    encoded bytes.
            toprule(str): Name of starting rule in grammar (will
                          default to first public rule).
        Returns:
            FsgModel: Newly loaded finite-state grammar.
        Raises:
            ValueError: On failure to parse or find `toprule`.
            RuntimeError: If JSGF has no public rules.
        """
        cdef config_t *cconfig = decoder_config(self._ps)
        cdef logmath_t *lmath = decoder_logmath(self._ps)
        cdef jsgf_t *jsgf
        cdef jsgf_rule_t *rule
        cdef float lw

        if not isinstance(jsgf_string, bytes):
            jsgf_string = jsgf_string.encode("utf-8")
        jsgf = jsgf_parse_string(jsgf_string, NULL)
        if jsgf == NULL:
            raise ValueError("Failed to parse JSGF")
        if toprule is not None:
            rule = jsgf_get_rule(jsgf, toprule.encode('utf-8'))
            if rule == NULL:
                jsgf_grammar_free(jsgf)
                raise ValueError("Failed to find top rule %s" % toprule)
        else:
            rule = jsgf_get_public_rule(jsgf)
            if rule == NULL:
                jsgf_grammar_free(jsgf)
                raise RuntimeError("No public rules found in JSGF")
        lw = config_float(cconfig, "lw")
        fsg = FsgModel()
        fsg.fsg = jsgf_build_fsg(jsgf, rule, lmath, lw)
        jsgf_grammar_free(jsgf)
        return fsg

    def set_fsg(self, FsgModel fsg):
        """Set the grammar for recognition.

        Args:
            fsg(FsgModel): Previously loaded or constructed grammar.

        """
        # Decoder owns FSG, but so does Python
        if decoder_set_fsg(self._ps, fsg_model_retain(fsg.fsg)) != 0:
            raise RuntimeError("Failed to set FSG in decoder")

    def set_jsgf_file(self, filename):
        """Set the grammar for recognition from a JSGF file.

        Args:
            filename(str): Path to a JSGF file to load.
        """
        if decoder_set_jsgf_file(self._ps, filename.encode()) != 0:
            raise RuntimeError("Failed to set JSGF from %s" % filename)

    def set_jsgf_string(self, jsgf_string):
        """Set the grammar for recognition from JSGF bytes or string.

        Args:
            jsgf_string(bytes): JSGF grammar as string or UTF-8 encoded
                                bytes.
        """
        if not isinstance(jsgf_string, bytes):
            jsgf_string = jsgf_string.encode("utf-8")
        if decoder_set_jsgf_string(self._ps, jsgf_string) != 0:
            raise RuntimeError("Failed to parse JSGF in decoder")

    def decode_file(self, input_file):
        """Decode audio from a file in the filesystem.

        Currently supports single-channel WAV and raw audio files.  If
        the sampling rate for a WAV file differs from the one set in
        the decoder's configuration, the configuration will be updated
        to match it.

        Note that we always decode the entire file at once. It would
        have to be really huge for this to cause memory problems, in
        which case the decoder would explode anyway. Otherwise, CMN
        doesn't work as well, which causes unnecessary recognition
        errors.

        Args:
            input_file: Path to an audio file.

        Returns:
            (str, Iterable[Seg]): Recognized text, Word segmentation.

        """
        data, sample_rate = soundswallower.get_audio_data(input_file)
        if sample_rate is None:
            sample_rate = self.config["samprate"]
        # Reinitialize the decoder if necessary
        if sample_rate != self.config["samprate"]:
            LOGGER.info("Setting sample rate to %d", sample_rate)
            self.config["samprate"] = sample_rate
            self.reinit_feat()

        self.start_utt()
        self.process_raw(data, no_search=False, full_utt=True)
        self.end_utt()

        if self.hyp.text is None:
            raise RuntimeError("Decoding produced no segments, "
                               "please examine dictionary/grammar and input audio.")

        return self.hyp.text, self.seg

    def dumps(self, start_time=0., align_level=0):
        """Get decoding result as JSON."""
        cdef const char *json_result = decoder_result_json(self._ps, start_time,
                                                           align_level)
        return json_result.decode("utf-8")

    def set_align_text(self, text):
        """Set a word sequence for alignment.

        You must do any text normalization yourself.  For word-level
        alignment, once you call this, simply decode and get the
        segmentation in the usual manner.  For phone-level alignment,
        you can use the `alignment` property.

        Args:
            text(str): Sentence to align, as whitespace-separated
                       words.  All words must be present in the
                       dictionary.
        Raises:
            RuntimeError: If text is invalid somehow.
        """
        cdef int rv = decoder_set_align_text(self._ps, text.encode("utf-8"))
        if rv < 0:
            raise RuntimeError("Failed to set up alignment of %s" % (text))

    @property
    def alignment(self):
        """The current sub-word alignment, if any.

        This property may take some time to access as it runs a second
        pass of decoding.

        Returns:
            Alignment - if an alignment exists.

        """
        cdef alignment_t *al = decoder_alignment(self._ps)
        if al == NULL:
            return None
        return Alignment.create_from_ptr(alignment_retain(al))

    @property
    def n_frames(self):
        """The number of frames processed up to this point.

        Returns:
            int: Like it says.
        """
        return decoder_n_frames(self._ps)


cdef class Vad:
    """Voice activity detection class.

    Args:
      mode(int): Aggressiveness of voice activity detction (0-3)
      sample_rate(int): Sampling rate of input, default is 16000.
                        Rates other than 8000, 16000, 32000, 48000
                        are only approximately supported, see note
                        in `frame_length`.  Outlandish sampling
                        rates like 3924 and 115200 will raise a
                        `ValueError`.
      frame_length(float): Desired input frame length in seconds,
                           default is 0.03.  The *actual* frame
                           length may be different if an
                           approximately supported sampling rate is
                           requested.  You must *always* use the
                           `frame_bytes` and `frame_length`
                           attributes to determine the input size.

    Raises:
      ValueError: Invalid input parameter (see above).
    """
    cdef vad_t *_vad
    LOOSE = VAD_LOOSE
    MEDIUM_LOOSE = VAD_MEDIUM_LOOSE
    MEDIUM_STRICT = VAD_MEDIUM_STRICT
    STRICT = VAD_STRICT
    DEFAULT_SAMPLE_RATE = VAD_DEFAULT_SAMPLE_RATE
    DEFAULT_FRAME_LENGTH = VAD_DEFAULT_FRAME_LENGTH

    def __init__(self, mode=VAD_LOOSE,
                 sample_rate=VAD_DEFAULT_SAMPLE_RATE,
                 frame_length=VAD_DEFAULT_FRAME_LENGTH):
        self._vad = vad_init(mode, sample_rate, frame_length)
        if self._vad == NULL:
            raise ValueError("Invalid VAD parameters")

    def __dealloc__(self):
        vad_free(self._vad)

    @property
    def frame_bytes(self):
        """int: Number of bytes (not samples) required in an input frame.

        You *must* pass input of this size, as `bytes`, to the `Vad`.
        """
        return vad_frame_size(self._vad) * 2

    @property
    def frame_length(self):
        """float: Length of a frame in seconds (*may be different from the one
        requested in the constructor*!)"""
        return vad_frame_length(self._vad)

    @property
    def sample_rate(self):
        """int: Sampling rate of input data."""
        return vad_sample_rate(self._vad)

    def is_speech(self, frame, sample_rate=None):
        """Classify a frame as speech or not.

        Args:
          frame(bytes): Buffer containing speech data (16-bit signed
                        integers).  Must be of length `frame_bytes`
                        (in bytes).
        Returns:
          bool: Classification as speech or not speech.
        Raises:
          IndexError: `buf` is of invalid size.
          ValueError: Other internal VAD error.
        """
        cdef const unsigned char[:] cframe = frame
        cdef Py_ssize_t n_samples = len(cframe) // 2
        if len(cframe) != self.frame_bytes:
            raise IndexError("Frame size must be %d bytes" % self.frame_bytes)
        rv = vad_classify(self._vad, <const short *>&cframe[0])
        if rv < 0:
            raise ValueError("VAD classification failed")
        return rv == VAD_SPEECH

cdef class Endpointer:
    """Simple endpointer using voice activity detection.

    Args:
      window(float): Length in seconds of window for decision.
      ratio(float): Fraction of window that must be speech or
                    non-speech to make a transition.
      mode(int): Aggressiveness of voice activity detction (0-3)
      sample_rate(int): Sampling rate of input, default is 16000.
                        Rates other than 8000, 16000, 32000, 48000
                        are only approximately supported, see note
                        in `frame_length`.  Outlandish sampling
                        rates like 3924 and 115200 will raise a
                        `ValueError`.
      frame_length(float): Desired input frame length in seconds,
                           default is 0.03.  The *actual* frame
                           length may be different if an
                           approximately supported sampling rate is
                           requested.  You must *always* use the
                           `frame_bytes` and `frame_length`
                           attributes to determine the input size.

    Raises:
      ValueError: Invalid input parameter.  Also raised if the ratio
                  makes it impossible to do endpointing (i.e. it
                  is more than N-1 or less than 1 frame).
    """
    cdef endpointer_t *_ep
    DEFAULT_WINDOW = ENDPOINTER_DEFAULT_WINDOW
    DEFAULT_RATIO = ENDPOINTER_DEFAULT_RATIO
    def __init__(
        self,
        window=0.3,
        ratio=0.9,
        vad_mode=Vad.LOOSE,
        sample_rate=Vad.DEFAULT_SAMPLE_RATE,
        frame_length=Vad.DEFAULT_FRAME_LENGTH,
    ):
        self._ep = endpointer_init(window, ratio,
                                   vad_mode, sample_rate, frame_length)
        if (self._ep == NULL):
            raise ValueError("Invalid endpointer or VAD parameters")

    @property
    def frame_bytes(self):
        """int: Number of bytes (not samples) required in an input frame.

        You *must* pass input of this size, as `bytes`, to the `Endpointer`.
        """
        return endpointer_frame_size(self._ep) * 2

    @property
    def frame_length(self):
        """float: Length of a frame in seconds (*may be different from the one
        requested in the constructor*!)"""
        return endpointer_frame_length(self._ep)

    @property
    def sample_rate(self):
        """int: Sampling rate of input data."""
        return endpointer_sample_rate(self._ep)

    @property
    def in_speech(self):
        """bool: Is the endpointer currently in a speech segment?

        To detect transitions from non-speech to speech, check this
        before `process`.  If it was `False` but `process` returns
        data, then speech has started::

            prev_in_speech = ep.in_speech
            speech = ep.process(frame)
            if speech is not None:
                if not prev_in_speech:
                    print("Speech started at", ep.speech_start)

        Likewise, to detect transitions from speech to non-speech,
        call this *after* `process`.  If `process` returned data but
        this returns `False`, then speech has stopped::

            speech = ep.process(frame)
            if speech is not None:
                if not ep.in_speech:
                    print("Speech ended at", ep.speech_end)
        """
        return endpointer_in_speech(self._ep)

    @property
    def speech_start(self):
        """float: Start time of current speech region."""
        return endpointer_speech_start(self._ep)

    @property
    def speech_end(self):
        """float: End time of current speech region."""
        return endpointer_speech_end(self._ep)

    def process(self, frame):
        """Read a frame of data and return speech if detected.

        Args:
          frame(bytes): Buffer containing speech data (16-bit signed
                        integers).  Must be of length `frame_bytes`
                        (in bytes).
        Returns:
          bytes: Frame of speech data, or None if none detected.
        Raises:
          IndexError: `buf` is of invalid size.
          ValueError: Other internal VAD error.
        """
        cdef const unsigned char[:] cframe = frame
        cdef Py_ssize_t n_samples = len(cframe) // 2
        cdef const short *outframe
        if len(cframe) != self.frame_bytes:
            raise IndexError("Frame size must be %d bytes" % self.frame_bytes)
        outframe = endpointer_process(self._ep,
                                      <const short *>&cframe[0])
        if outframe == NULL:
            return None
        return (<const unsigned char *>&outframe[0])[:n_samples * 2]

    def end_stream(self, frame):
        """Read a final frame of data and return speech if any.

        This function should only be called at the end of the input
        stream (and then, only if you are currently in a speech
        region).  It will return any remaining speech data detected by
        the endpointer.

        Args:
          frame(bytes): Buffer containing speech data (16-bit signed
                        integers).  Must be of length `frame_bytes`
                        (in bytes) *or less*.
        Returns:
          bytes: Remaining speech data (could be more than one frame),
          or None if none detected.
        Raises:
          IndexError: `buf` is of invalid size.
          ValueError: Other internal VAD error.

        """
        cdef const unsigned char[:] cframe = frame
        cdef Py_ssize_t n_samples = len(cframe) // 2
        cdef const short *outbuf
        cdef size_t out_n_samples
        if len(cframe) > self.frame_bytes:
            raise IndexError("Frame size must be %d bytes or less" % self.frame_bytes)
        outbuf = endpointer_end_stream(self._ep,
                                       <const short *>&cframe[0],
                                       n_samples,
                                       &out_n_samples)
        if outbuf == NULL:
            return None
        return (<const unsigned char *>&outbuf[0])[:out_n_samples * 2]

cdef class AlignmentEntry:
    """Entry (word, phone, state) in an alignment.

    Iterating over this will iterate over its children (i.e. the
    phones in a word or the states in a phone) if any.  For example::

        for word in decoder.get_alignment():
            print("%s from %.2f to %.2f" % (word.name, word.start,
                                            word.start + word.duration))
            for phone in word:
                print("%s at %.2f duration %.2f" %
                      (phone.name, phone.start, phone.duration))

    Attributes:
      name(str): Name of segment (word, phone name, state id)
      start(int): Index of start frame.
      duration(int): Duration in frames.
      score(float): Acoustic score (density).
    """
    cdef public int start
    cdef public int duration
    cdef public int score
    cdef public str name
    # DANGER! Not retained!
    cdef alignment_iter_t *itor
    @staticmethod
    cdef create_from_iter(alignment_iter_t *itor):
        cdef AlignmentEntry self
        self = AlignmentEntry.__new__(AlignmentEntry)
        self.score = alignment_iter_seg(itor, &self.start, &self.duration)
        self.name = alignment_iter_name(itor).decode('utf-8')
        self.itor = itor  # DANGER! DANGER!
        return self

    def __iter__(self):
        cdef alignment_iter_t *itor = alignment_iter_children(self.itor)
        while itor != NULL:
            c = AlignmentEntry.create_from_iter(itor)
            yield c
            itor = alignment_iter_next(itor)
        # FIXME: will leak memory if iteration stopped short!

cdef class Alignment:
    """Sub-word alignment as returned by `get_alignment`.

    For the moment this is read-only.  You are able to iterate over
    the words, phones, or states in it, as well as sub-iterating over
    each of their children, as described in `AlignmentEntry`.
    """
    cdef alignment_t *_al

    @staticmethod
    cdef create_from_ptr(alignment_t *al):
        cdef Alignment self = Alignment.__new__(Alignment)
        self._al = al
        return self

    def __dealloc__(self):
        if self._al != NULL:
            alignment_free(self._al)

    def __iter__(self):
        return self.words()

    def words(self):
        """Iterate over words in the alignment."""
        cdef alignment_iter_t *itor = alignment_words(self._al)
        while itor != NULL:
            w = AlignmentEntry.create_from_iter(itor)
            yield w
            itor = alignment_iter_next(itor)
        # FIXME: will leak memory if iteration stopped short!

    def phones(self):
        """Iterate over phones in the alignment."""
        cdef alignment_iter_t *itor = alignment_phones(self._al)
        while itor != NULL:
            p = AlignmentEntry.create_from_iter(itor)
            yield p
            itor = alignment_iter_next(itor)

    def states(self):
        """Iterate over states in the alignment."""
        cdef alignment_iter_t *itor = alignment_states(self._al)
        while itor != NULL:
            s = AlignmentEntry.create_from_iter(itor)
            yield s
            itor = alignment_iter_next(itor)

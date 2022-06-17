# cython: embedsignature=True
# Copyright (c) 2008-2020 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhdaines@gmail.com>

from libc.stdlib cimport malloc, free
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

    A `Config` can be initialized from a list of arguments as on a
    command line::

        config = Config("-hmm", "path/to/things", "-dict", "my.dict")

    It can also be initialized with keyword arguments::

        config = Config(hmm="path/to/things", dict="my.dict")

    It is possible to access the `Config` either with the set_*
    methods or directly by setting and getting keys, as with a Python
    dictionary.  The set_* methods are not recommended as they require
    the user to know the type of the configuration parameter and to
    pre-pend a leading '-' character.  That is, the following are
    equivalent::

        config.get_string("-dict")
        config["dict"]

    In a future version, probably the next major one, these methods
    will be deprecated or may just disappear.

    In general, a `Config` mostly acts like a dictionary, and can be
    iterated over in the same fashion.  However, attempting to access
    a parameter that does not already exist will raise a `KeyError`.

    See :doc:`config_params` for a description of existing parameters.

 """
    cdef cmd_ln_t *cmd_ln
    
    def __cinit__(self, *args, **kwargs):
        cdef char **argv
        if args or kwargs:
            args = [str(k).encode('utf-8')
                    for k in args]
            for k, v in kwargs.items():
                if len(k) > 0 and k[0] != '-':
                    k = "-" + k
                args.append(k.encode('utf-8'))
                if v is None:
                    args.append(None)
                else:
                    args.append(str(v).encode('utf-8'))
            argv = <char **> malloc((len(args) + 1) * sizeof(char *))
            argv[len(args)] = NULL
            for i, buf in enumerate(args):
                if buf is None:
                    argv[i] = NULL
                else:
                    argv[i] = buf
            self.cmd_ln = cmd_ln_parse_r(NULL, ps_args(),
                                         len(args), argv, 0)
            free(argv)
        else:
            self.cmd_ln = cmd_ln_parse_r(NULL, ps_args(), 0, NULL, 0)

    def __init__(self, *args, **kwargs):
        pass

    def __dealloc__(self):
        cmd_ln_free_r(self.cmd_ln)

    def get_float(self, key):
        return cmd_ln_float_r(self.cmd_ln, key.encode('utf-8'))

    def get_int(self, key):
        return cmd_ln_int_r(self.cmd_ln, key.encode('utf-8'))

    def get_string(self, key):
        cdef const char *val = cmd_ln_str_r(self.cmd_ln,
                                            key.encode('utf-8'))
        if val == NULL:
            return None
        else:
            return val.decode('utf-8')

    def get_boolean(self, key):
        return cmd_ln_int_r(self.cmd_ln, key.encode('utf-8')) != 0

    def set_float(self, key, double val):
        cmd_ln_set_float_r(self.cmd_ln, key.encode('utf-8'), val)

    def set_int(self, key, long val):
        cmd_ln_set_int_r(self.cmd_ln, key.encode('utf-8'), val)

    def set_boolean(self, key, val):
        cmd_ln_set_int_r(self.cmd_ln, key.encode('utf-8'), val != 0)

    def set_string(self, key, val):
        if val == None:
            cmd_ln_set_str_r(self.cmd_ln, key.encode('utf-8'), NULL)
        else:
            cmd_ln_set_str_r(self.cmd_ln, key.encode('utf-8'), val.encode('utf-8'))

    def exists(self, key):
        return key in self

    cdef _normalize_key(self, key):
        # Note, returns a Python bytes string, to avoid unsafe temps
        if len(key) > 0:
            if key[0] == "_":
                # Ask for underscore, get underscore
                return key.encode('utf-8')
            elif key[0] == "-":
                # Ask for dash, get underscore or dash
                under_key = ("_" + key[1:]).encode('utf-8')
                if cmd_ln_exists_r(self.cmd_ln, under_key):
                    return under_key
                else:
                    return key.encode('utf-8')
            else:
                # No dash or underscore, try underscore, then dash
                under_key = ("_" + key).encode('utf-8')
                if cmd_ln_exists_r(self.cmd_ln, under_key):
                    return under_key
                dash_key = ("-" + key).encode('utf-8')
                if cmd_ln_exists_r(self.cmd_ln, dash_key):
                    return dash_key
        return key.encode('utf-8')

    cdef _normalize_ckey(self, const char *ckey):
        key = ckey.decode('utf-8')
        if len(key) == 0:
            return key
        if key[0] in "-_":
            return key[1:]

    def __contains__(self, key):
        return cmd_ln_exists_r(self.cmd_ln, self._normalize_key(key))

    def __getitem__(self, key):
        cdef const char *cval
        cdef cmd_ln_val_t *at;
        ckey = self._normalize_key(key)
        at = cmd_ln_access_r(self.cmd_ln, ckey)
        if at == NULL:
            raise KeyError("Unknown key %s" % key)
        elif at.type & ARG_STRING:
            cval = cmd_ln_str_r(self.cmd_ln, ckey)
            if cval == NULL:
                return None
            else:
                return cval.decode('utf-8')
        elif at.type & ARG_INTEGER:
            return cmd_ln_int_r(self.cmd_ln, ckey)
        elif at.type & ARG_FLOATING:
            return cmd_ln_float_r(self.cmd_ln, ckey)
        elif at.type & ARG_BOOLEAN:
            return cmd_ln_int_r(self.cmd_ln, ckey) != 0
        else:
            raise ValueError("Unable to handle parameter type %d" % at.type)

    def __setitem__(self, key, val):
        cdef cmd_ln_val_t *at;
        ckey = self._normalize_key(key)
        at = cmd_ln_access_r(self.cmd_ln, ckey)
        if at == NULL:
            # FIXME: for now ... but should handle this
            raise KeyError("Unknown key %s" % key)
        elif at.type & ARG_STRING:
            if val is None:
                cmd_ln_set_str_r(self.cmd_ln, ckey, NULL)
            else:
                cmd_ln_set_str_r(self.cmd_ln, ckey, str(val).encode('utf-8'))
        elif at.type & ARG_INTEGER:
            cmd_ln_set_int_r(self.cmd_ln, ckey, int(val))
        elif at.type & ARG_FLOATING:
            cmd_ln_set_float_r(self.cmd_ln, ckey, float(val))
        elif at.type & ARG_BOOLEAN:
            cmd_ln_set_int_r(self.cmd_ln, ckey, val != 0)
        else:
            raise ValueError("Unable to handle parameter type %d" % at.type)

    def __iter__(self):
        cdef hash_table_t *ht = self.cmd_ln.ht
        cdef hash_iter_t *itor
        cdef const char *ckey
        keys = set()
        itor = hash_table_iter(self.cmd_ln.ht)
        while itor != NULL:
            ckey = hash_entry_key(itor.ent)
            key = self._normalize_ckey(ckey)
            keys.add(key)
            itor = hash_table_iter_next(itor)
        return iter(keys)

    def items(self):
        keys = list(self)
        for key in keys:
            yield (key, self[key])

    def __len__(self):
        # Incredibly, the only way to do this, but necessary also
        # because of dash and underscore magic.
        return sum(1 for _ in self)

    def describe(self):
        """Iterate over parameter descriptions.

        This function returns a generator over the parameters defined
        in a configuration, as `Arg` objects.

        Returns:
            Iterable[Arg]: Descriptions of parameters including their
            default values and documentation

        """
        cdef const arg_t *arg = self.cmd_ln.defn
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
                raise RuntimeError("Unknown type %d in argument %s"
                                   % (base_type, name))
            arg = arg + 1
            yield soundswallower.Arg(name=name, default=default, doc=doc,
                                     type=arg_type, required=required)
    
cdef class Segment:
    """Word segmentation, as generated by `Decoder.seg`.

    Attributes:
      word(str): Name of word.
      start_frame(int): Index of start frame.
      end_frame(int): Index of start frame.
      ascore(float): Acoustic score (density).
      lscore(float): Language model score (joint probability).
    """
    cdef public str word
    cdef public int start_frame
    cdef public int end_frame
    cdef public double ascore
    cdef public double prob
    cdef public double lscore

    cdef set_seg(self, ps_seg_t *seg, logmath_t *lmath):
        cdef int ascr, lscr
        cdef int sf, ef

        self.word = ps_seg_word(seg).decode('utf-8')
        ps_seg_frames(seg, &sf, &ef)
        self.start_frame = sf
        self.end_frame = ef
        self.prob = logmath_exp(lmath,
                                ps_seg_prob(seg, &ascr, &lscr));
        self.ascore = logmath_exp(lmath, ascr)
        self.lscore = logmath_exp(lmath, lscr)


cdef class Hypothesis:
    """Recognition hypothesis, as returned by `Decoder.hyp`.

    Attributes:
      hypstr(str): Recognized text.
      score(float): Recognition score.
      prob(float): Posterior probability.
    """
    cdef public str hypstr
    cdef public double score
    cdef public double prob

    def __init__(self, hypstr, score, prob):
        self.hypstr = hypstr
        self.score = score
        self.prob = prob


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
        config(Config): Optional configuration object.  You can also
                        use keyword arguments, the most important of
                        which are noted below.  See :doc:`config_params`
                        for more information.
        hmm(str): Path to directory containing acoustic model files.
        dict(str): Path to pronunciation dictionary.
        jsgf(str): Path to JSGF grammar file.
        fsg(str): Path to FSG grammar file (only one of ``jsgf`` or ``fsg`` should
                  be specified).
        toprule(str): Name of top-level rule in JSGF file to use as entry point.
        samprate(float): Sampling rate for raw audio data.
        logfn(str): File to write log messages to (set to `os.devnull` to
                    silence these messages)

    """
    cdef ps_decoder_t *ps
    cdef public Config config

    def __cinit__(self, *args, **kwargs):
        if len(args) == 1 and isinstance(args[0], Config):
            self.config = args[0]
        else:
            self.config = Config(*args, **kwargs)
        if self.config is None:
            raise RuntimeError, "Failed to parse argument list"
        self.ps = ps_init(self.config.cmd_ln)
        if self.ps == NULL:
            raise RuntimeError, "Failed to initialize PocketSphinx"

    def __init__(self, *args, **kwargs):
        pass

    def __dealloc__(self):
        ps_free(self.ps)

    @classmethod
    def default_config(_):
        """Return default configuraiton.

        Actually this does the same thing as just creating a `Config`.
        
        Returns:
            Config: Newly created default configuration.
        """
        return Config()

    def reinit(self, Config config=None):
        """Reinitialize the decoder.

        Args:
            config(Config): Optional new configuration to apply, otherwise
                            the existing configuration in the `config`
                            attribute will be reloaded.

        """
        cdef cmd_ln_t *cconfig
        if config is None:
            cconfig = NULL
        else:
            self.config = config
            cconfig = config.cmd_ln
        if ps_reinit(self.ps, cconfig) != 0:
            raise RuntimeError("Failed to reinitialize decoder configuration")

    def reinit_fe(self, Config config=None):
        """Reinitialize only the feature extraction.

        Args:
            config(Config): Optional new configuration to apply, otherwise
                            the existing configuration in the `config`
                            attribute will be reloaded.

        """
        cdef cmd_ln_t *cconfig
        if config is None:
            cconfig = NULL
        else:
            self.config = config
            cconfig = config.cmd_ln
        if ps_reinit_fe(self.ps, cconfig) == NULL:
            raise RuntimeError("Failed to reinitialize feature extraction")

    def start_utt(self):
        """Start processing raw audio input.

        This method must be called at the beginning of each separate
        "utterance" of raw audio input.

        """
        if ps_start_utt(self.ps) < 0:
            raise RuntimeError, "Failed to start utterance processing"

    def process_raw(self, data, no_search=False, full_utt=False):
        """Process a block of raw audio.

        Args:
            data(bytes): Raw audio data, a block of 16-bit signed integer binary data.
            no_search(bool): If `True`, do not do any decoding on this data.
            full_utt(bool): If `True`, assume this is the entire utterance, for
                            purposes of acoustic normalization.

        """
        cdef const unsigned char[:] cdata = data
        cdef Py_ssize_t n_samples = len(cdata) // 2
        if ps_process_raw(self.ps, <const short *>&cdata[0],
                          n_samples, no_search, full_utt) < 0:
            raise RuntimeError, "Failed to process %d samples of audio data" % len / 2

    def end_utt(self):
        """Finish processing raw audio input.

        This method must be called at the end of each separate
        "utterance" of raw audio input.  It takes care of flushing any
        internal buffers and finalizing recognition results.

        """
        if ps_end_utt(self.ps) < 0:
            raise RuntimeError, "Failed to stop utterance processing"

    def hyp(self):
        """Get current recognition hypothesis.
        
        Returns:
            Hypothesis: Current recognition output.

        """
        cdef const char *hyp
        cdef logmath_t *lmath
        cdef int score

        hyp = ps_get_hyp(self.ps, &score)
        if hyp == NULL:
             return None
        lmath = ps_get_logmath(self.ps)
        prob = ps_get_prob(self.ps)
        return Hypothesis(hyp.decode('utf-8'),
                          logmath_exp(lmath, score),
                          logmath_exp(lmath, prob))

    def get_prob(self):
        """Posterior probability of current recogntion hypothesis.

        Returns:
            float: Posterior probability of current hypothesis.  FIXME:
            At the moment this is almost certainly 1.0, as confidence
            estimation is not well supported.

        """
        cdef logmath_t *lmath
        cdef const char *uttid
        lmath = ps_get_logmath(self.ps)
        return logmath_exp(lmath, ps_get_prob(self.ps))

    def add_word(self, word, phones, update=True):
        """Add a word to the pronunciation dictionary.

        Args:
            word(str): Text of word to be added.
            phones(str): Space-separated list of phones for this
                         word's pronunciation.  This will depend on
                         the underlying acoustic model but is probably
                         in ARPABET.
            update(bool): Update the recognizer immediately.  You can
                          set this to `False` if you are adding a lot
                          of words, to speed things up.

        """
        return ps_add_word(self.ps, word, phones, update)

    def seg(self):
        """Get current word segmentation.
        
        Returns:
            Iterable[Segment]: Generator over word segmentations.

        """
        cdef ps_seg_t *itor
        cdef logmath_t *lmath
        itor = ps_seg_iter(self.ps)
        if itor == NULL:
            return
        lmath = ps_get_logmath(self.ps)
        while itor != NULL:
            seg = Segment()
            seg.set_seg(itor, lmath)
            yield seg
            itor = ps_seg_next(itor)

    def read_fsg(self, filename):
        """Read a grammar from an FSG file.

        Args:
            filename(str): Path to FSG file.

        Returns:
            FsgModel: Newly loaded finite-state grammar.

        """
        cdef logmath_t *lmath
        cdef float lw

        lw = cmd_ln_float_r(self.config.cmd_ln, "-lw")
        lmath = ps_get_logmath(self.ps)
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
        cdef logmath_t *lmath
        cdef float lw

        lw = cmd_ln_float_r(self.config.cmd_ln, "-lw")
        lmath = ps_get_logmath(self.ps)
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

        """
        cdef logmath_t *lmath
        cdef float lw
        cdef int wid

        lw = cmd_ln_float_r(self.config.cmd_ln, "-lw")
        lmath = ps_get_logmath(self.ps)
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
            jsgf_string(bytes): JSGF grammar as string or UTF-8 encoded
                                bytes.

        Returns:
            FsgModel: Newly loaded finite-state grammar.

        """
        cdef jsgf_t *jsgf
        cdef jsgf_rule_t *rule
        cdef logmath_t *lmath
        cdef float lw

        if not isinstance(jsgf_string, bytes):
            jsgf_string = jsgf_string.encode("utf-8")
        jsgf = jsgf_parse_string(jsgf_string, NULL)
        if jsgf == NULL:
            raise RuntimeError("Failed to parse JSGF")
        if toprule is not None:
            rule = jsgf_get_rule(jsgf, toprule.encode('utf-8'))
            if rule == NULL:
                jsgf_grammar_free(jsgf)
                raise RuntimeError("Failed to find top rule %s" % toprule)
        else:
            rule = jsgf_get_public_rule(jsgf)
            if rule == NULL:
                jsgf_grammar_free(jsgf)
                raise RuntimeError("No public rules found in JSGF")
        lw = cmd_ln_float_r(self.config.cmd_ln, "-lw")
        lmath = ps_get_logmath(self.ps)
        fsg = FsgModel()
        fsg.fsg = jsgf_build_fsg(jsgf, rule, lmath, lw)
        jsgf_grammar_free(jsgf)
        return fsg

    def set_fsg(self, FsgModel fsg):
        """Set the grammar for recognition.

        Args:
            fsg(FsgModel): Previously loaded or constructed grammar.

        """
        if ps_set_fsg(self.ps, fsg_model_name(fsg.fsg), fsg.fsg) != 0:
            raise RuntimeError("Failed to set FSG in decoder")

    def set_jsgf_file(self, filename, name="_default"):
        """Set the grammar for recognition from a JSGF file.

        Args:
            filename(str): Path to a JSGF file to load.
            name(str): Optional name to give the grammar (not very useful).
        """
        if ps_set_jsgf_file(self.ps, name.encode("utf-8"),
                            filename.encode()) != 0:
            raise RuntimeError("Failed to set JSGF from %s" % filename)

    def set_jsgf_string(self, jsgf_string, name="_default"):
        """Set the grammar for recognition from JSGF bytes or string.

        Args:
            jsgf_string(bytes): JSGF grammar as string or UTF-8 encoded
                                bytes.
            name(str): Optional name to give the grammar (not very useful).
        """
        if not isinstance(jsgf_string, bytes):
            jsgf_string = jsgf_string.encode("utf-8")
        if ps_set_jsgf_string(self.ps, name.encode("utf-8"), jsgf_string) != 0:
            raise RuntimeError("Failed to parse JSGF in decoder")

    def decode_file(self, input_file, include_silence=False):
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
            include_silence: Include silence segments in output.

        Returns:
            (str, Iterable[(str, float, float)]): Recognized text, Word
                                                  segmentation.

        """
        data, sample_rate = soundswallower.get_audio_data(input_file)
        if sample_rate is None:
            sample_rate = self.config.get_float("-samprate")
        # Reinitialize the decoder if necessary
        if sample_rate != self.config.get_float("-samprate"):
            LOGGER.info("Setting sample rate to %d", sample_rate)
            self.config["samprate"] = sample_rate
            self.reinit_fe()
        frame_size = 1.0 / self.config.get_int('-frate')

        self.start_utt()
        self.process_raw(data, no_search=False, full_utt=True)
        self.end_utt()

        if not self.seg():
            raise RuntimeError("Decoding produced no segments, "
                               "please examine dictionary/grammar and input audio.")

        segmentation = []
        frame_size = 1.0 / self.config.get_int('-frate')
        for seg in self.seg():
            start = seg.start_frame * frame_size
            end = (seg.end_frame + 1) * frame_size
            if not include_silence and seg.word in ('<sil>', '[NOISE]', '(NULL)'):
                continue
            else:
                segmentation.append((seg.word, start, end))

        if len(segmentation) == 0:
            raise RuntimeError("Decoding produced only noise or silence segments, "
                               "please examine dictionary and input audio and text.")

        return self.hyp().hypstr, segmentation

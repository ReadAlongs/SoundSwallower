# Copyright (c) 2008-2020 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhdaines@gmail.com>

from libc.stdlib cimport malloc, free
cimport _soundswallower

cdef class LogMath:
    """
    Log-space math class.
    
    This class provides fast logarithmic math functions in base
    1.000+epsilon, useful for fixed point speech recognition.

    @param base: The base B in which computation is to be done.
    @type base: float
    @param shift: Log values are shifted right by this many bits.
    @type shift: int
    @param use_table Whether to use an add table or not
    @type use_table: bool
    """
    cdef _soundswallower.logmath_t *lmath
    
    def __cinit__(self, base=1.0001, shift=0, use_table=1):
        self.lmath = logmath_init(base, shift, use_table)
        if self.lmath is NULL:
            raise MemoryError()

    def __init__(self, base=1.0001, shift=0, use_table=1):
        pass

    def __dealloc__(self):
        """
        Destructor for LogMath class.
        """
        if self.lmath is not NULL:
            logmath_free(self.lmath)

    def get_zero(self):
        """
        Get the log-zero value.

        @return: Smallest number representable by this object.
        @rtype: int
        """
        return logmath_get_zero(self.lmath)

    def add(self, a, b):
        """
        Add two numbers in log-space.

        @param a: Logarithm A.
        @type a: int
        @param b: Logarithm B.
        @type b: int
        @return: log(exp(a)+exp(b))
        @rtype: int
        """
        return logmath_add(self.lmath, a, b)

    def log(self, x):
        """
        Return log-value of a number.

        @param x: Number (in linear space)
        @type x: float
        @return: Log-value of x.
        @rtype: int
        """
        return logmath_log(self.lmath, x)

    def exp(self, x):
        """
        Return linear of a log-value

        @param x: Logarithm X (in this object's base)
        @type x: int
        @return: Exponent (linear value) of X.
        @rtype: float
        """
        return logmath_exp(self.lmath, x)

    def log_to_ln(self, x):
        """
        Return natural logarithm of a log-value.

        @param x: Logarithm X (in this object's base)
        @type x: int
        @return: Natural log equivalent of x.
        @rtype: float
        """
        return logmath_log_to_ln(self.lmath, x)

    def log_to_log10(self, x):
        """
        Return logarithm in base 10 of a log-value.

        @param x: Logarithm X (in this object's base)
        @type x: int
        @return: log10 equivalent of x.
        @rtype: float
        """
        return logmath_log_to_log10(self.lmath, x)

    def ln_to_log(self, x):
        """
        Return log-value of a natural logarithm.

        @param x: Logarithm X (in base e)
        @type x: float
        @return: Log-value equivalent of x.
        @rtype: int
        """
        return logmath_ln_to_log(self.lmath, x)

    def log10_to_log(self, x):
        """
        Return log-value of a base 10 logarithm.

        @param x: Logarithm X (in base 10)
        @type x: float
        @return: Log-value equivalent of x.
        @rtype: int
        """
        return logmath_log10_to_log(self.lmath, x)


cdef class Config:
    """
    SoundSwallower configuration manager.
    """
    cdef cmd_ln_t *cmd_ln
    
    def __cinit__(self, *args, **kwargs):
        """
        Create a config from arguments.
        """
        cdef char **argv
        if kwargs:
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
        """
        Create a config from arguments.
        """
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

    def __contains__(self, key):
        if len(key) > 0 and key[0] != '-':
            dash_key = "-" + key
            if cmd_ln_exists_r(self.cmd_ln, dash_key.encode('utf-8')):
                return True
        return cmd_ln_exists_r(self.cmd_ln, key.encode('utf-8'))

    def __getitem__(self, key):
        cdef cmd_ln_val_t *at;
        if len(key) > 0 and key[0] != '-':
            dash_key = "-" + key
            if cmd_ln_exists_r(self.cmd_ln, dash_key.encode('utf-8')):
                key = dash_key
        at = cmd_ln_access_r(self.cmd_ln, key.encode('utf-8'))
        if at == NULL:
            raise KeyError("Unknown key %s" % key)
        elif at.type & ARG_STRING:
            return self.get_string(key)
        elif at.type & ARG_INTEGER:
            return self.get_int(key)
        elif at.type & ARG_FLOATING:
            return self.get_float(key)
        elif at.type & ARG_BOOLEAN:
            return self.get_boolean(key)
        else:
            raise ValueError("Unable to handle parameter type %d" % at.type)

    def __setitem__(self, key, value):
        cdef cmd_ln_val_t *at;
        if len(key) > 0 and key[0] != '-':
            dash_key = "-" + key
            if cmd_ln_exists_r(self.cmd_ln, dash_key.encode('utf-8')):
                key = dash_key
        at = cmd_ln_access_r(self.cmd_ln, key.encode('utf-8'))
        if at == NULL:
            # FIXME: for now ... but should handle this
            raise KeyError("Unknown key %s" % key)
        elif at.type & ARG_STRING:
            self.set_string(key, value)
        elif at.type & ARG_INTEGER:
            self.set_int(key, value)
        elif at.type & ARG_FLOATING:
            self.set_float(key, value)
        elif at.type & ARG_BOOLEAN:
            self.set_boolean(key, value)
        else:
            raise ValueError("Unable to handle parameter type %d" % at.type)

    def __iter__(self):
        pass

    def __len__(self):
        return 69


cdef class Segment:
    cdef ps_seg_t *seg
    cdef public str word
    cdef public int start_frame
    cdef public int end_frame
    cdef public int ascore
    cdef public int prob
    cdef public int lscore
    cdef public int lback

    def __init__(self):
        self.seg = NULL

    cdef set_seg(self, ps_seg_t *seg):
        cdef int ascr, lscr, lback
        cdef int sf, ef
        self.seg = seg
        self.word = ps_seg_word(self.seg).decode('utf-8')
        ps_seg_frames(self.seg, &sf, &ef)
        self.start_frame = sf
        self.end_frame = sf
        self.prob = ps_seg_prob(self.seg, &ascr, &lscr, &lback)
        self.ascore = ascr
        self.lscore = lscr
        self.lback = lback


cdef class SegmentIterator:
    """
    Iterator for best hypothesis word segments of best hypothesis
    """
    cdef ps_seg_t *itor
    cdef int first_seg

    def __init__(self):
        self.itor = NULL
        self.first_seg = False

    cdef set_iter(self, ps_seg_t *seg):
        self.itor = seg
        self.first_seg = True

    def __iter__(self):
        return self
    
    def __next__(self):
        cdef Segment seg
        if self.first_seg:
            self.first_seg = False
        else:
            self.itor = ps_seg_next(self.itor)
        if NULL == self.itor:
            raise StopIteration
        else:
            seg = Segment()
            seg.set_seg(self.itor)
        return seg


class Hypothesis:
    def __init__(self, hypstr, score, prob):
        self.hypstr = hypstr
        self.score = score
        self.prob = prob


cdef class Decoder:
    """
    Speech decoder.

    To initialize the decoder, pass a list of keyword
    arguments to the constructor::

     d = soundswallower.Decoder(hmm='/path/to/acoustic/model',
                                fsg='/path/to/grammar',
                                dict='/path/to/dictionary',
                                beam='1e-80')
    """
    cdef ps_decoder_t *ps
    cdef Config config

    def __cinit__(self, *args, **kwargs):
        self.ps = NULL

    def __init__(self, *args, **kwargs):
        if len(args) == 1 and isinstance(args[0], Config):
            self.config = args[0]
        else:
            self.config = Config(*args, **kwargs)
        if self.config is None:
            raise RuntimeError, "Failed to parse argument list"
        self.ps = ps_init(self.config.cmd_ln)
        if self.ps == NULL:
            raise RuntimeError, "Failed to initialize PocketSphinx"

    def __dealloc__(self):
        ps_free(self.ps)

    @classmethod
    def default_config(_):
        return Config()

    def start_utt(self):
        """
        Prepare the decoder to recognize an utterance.
        """
        if ps_start_utt(self.ps) < 0:
            raise RuntimeError, "Failed to start utterance processing"

    def process_raw(self, data, no_search=False, full_utt=False):
        """Process (decode) some audio data.

        @param data: Audio data to process.  This is packed binary
        data, which consists of single-channel, 16-bit PCM audio, at
        the sample rate specified when the decoder was initialized.
        @type data: bytes
        @param no_search: Buffer the data without actually processing
        it (default is to process the data as it is received).
        @type no_search: bool
        @param full_utt: This block of data is an entire utterance.
        Processing an entire utterance at once may improve
        recognition, particularly for the first utterance passed to
        the decoder.
        @type full_utt: bool

        """
        cdef const unsigned char[:] cdata = data
        cdef Py_ssize_t n_samples = len(cdata) // 2
        if ps_process_raw(self.ps, <const short *>&cdata[0],
                          n_samples, no_search, full_utt) < 0:
            raise RuntimeError, "Failed to process %d samples of audio data" % len / 2

    def end_utt(self):
        """
        Finish processing an utterance.
        """
        if ps_end_utt(self.ps) < 0:
            raise RuntimeError, "Failed to stop utterance processing"

    def hyp(self):
        cdef const char *hyp
        cdef int score

        hyp = ps_get_hyp(self.ps, &score)
        if hyp == NULL:
             return None
        prob = ps_get_prob(self.ps)
        return Hypothesis(hyp.decode('utf-8'), score, prob)

    def get_prob(self):
        """
	Get a posterior probability.
	
	Returns the posterior in linear scale.
	
	@return: posterior probability of the result
	@rtype: float
	"""
        cdef logmath_t *lmath
        cdef const char *uttid
        lmath = ps_get_logmath(self.ps)
        return logmath_exp(lmath, ps_get_prob(self.ps))

    def add_word(self, word, phones, update=True):
        """
        Add a word to the dictionary and current language model.

        @param word: Name of the word to add.
        @type word: str
        @param phones: Pronunciation of the word, a space-separated list of phones.
        @type phones: str
        @param update: Update the decoder to recognize this new word.
        If adding a number of words at once you may wish to pass
        C{False} here.
        @type update: bool
        """
        return ps_add_word(self.ps, word, phones, update)

    def load_dict(self, dictfile, fdictfile=None, format=None):
        """
        Load a new pronunciation dictionary.

        @param dictfile: Dictionary filename.
        @type dictfile: str
        @param fdictfile: Filler dictionary filename.
        @type fdictfile: str
        @param format: Dictionary format, currently unused.
        @type format: str
        """
        return ps_load_dict(self.ps, dictfile, fdictfile, format)

    def save_dict(self, dictfile, format=None):
        """
        Save current pronunciation dictionary to a file.

        @param dictfile: Dictionary filename.
        @type dictfile: str
        @param format: Dictionary format, currently unused.
        @type format: str
        """
        return ps_save_dict(self.ps, dictfile, format)

    def seg(self):
        cdef int score
        cdef ps_seg_t *first_seg
        cdef SegmentIterator itor
        first_seg = ps_seg_iter(self.ps)
        if first_seg == NULL:
            raise RuntimeError, "Failed to create best path word segment iterator"
        itor = SegmentIterator()
        itor.set_iter(first_seg)
        return itor

    def get_in_speech(self):
        """
        Checks if the last feed audio buffer contained speech

        @param ps Decoder.
        @return 1 if last buffer contained speech, 0 - otherwise
        """
        return ps_get_in_speech(self.ps)

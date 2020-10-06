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
    cdef _soundswallower.cmd_ln_t *cmd_ln
    
    def __cinit__(self, **kwargs):
        """
        Create a config from arguments.
        """
        cdef char **argv
        if kwargs:
            # Store temporary byte buffers in a Python list
            arglist = []
            for k, v in kwargs.items():
                if len(k) > 0 and k[0] != '-':
                    k = "-" + k
                arglist.append(k.encode('utf-8'))
                if v is None:
                    arglist.append(None)
                else:
                    arglist.append(str(v).encode('utf-8'))
            argv = <char **> malloc((len(arglist) + 1) * sizeof(char *))
            argv[len(arglist)] = NULL
            for i, buf in enumerate(arglist):
                if buf is None:
                    argv[i] = NULL
                else:
                    argv[i] = buf
            self.cmd_ln = cmd_ln_parse_r(NULL, ps_args(),
                                         len(arglist), argv, 0)
            free(argv)
        else:
            self.cmd_ln = cmd_ln_parse_r(NULL, ps_args(), 0, NULL, 0)

    def __init__(self, **kwargs):
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
        pass

    def __delitem__(self, key):
        pass

    def __iter__(self):
        pass

    def __len__(self):
        pass

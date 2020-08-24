# Copyright (c) 2008-2020 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhdaines@gmail.com>

cimport soundswallower

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
    cdef soundswallower.logmath_t *lmath
    
    def __init__(self, base=1.0001, shift=0, use_table=1):
        self.lmath = logmath_init(base, shift, use_table)

    def __dealloc__(self):
        """
        Destructor for LogMath class.
        """
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

#!/usr/bin/env python3
# Copyright (c) 1999-2020 Carnegie Mellon University. All rights
# reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# This work was supported in part by funding from the Defense Advanced
# Research Projects Agency and the National Science Foundation of the
# United States of America, and the CMU Sphinx Speech Consortium.
#
# THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND
# ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
# NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import unittest
import math
from soundswallower import LogMath


class TestLogMath(unittest.TestCase):
    def test_default_base(self):
        """Test some basic log-space calculations and conversions."""
        lm = LogMath()
        self.assertEqual(lm.get_zero(), -536870912)
        self.assertAlmostEqual(lm.exp(lm.add(lm.log(0.1), lm.log(0.1))), 0.2, 1)
        self.assertNotAlmostEqual(lm.exp(lm.add(lm.log(0.1), lm.log(0.1))), 0.3, 1)
        self.assertAlmostEqual(lm.exp(lm.log(1e-10) + lm.log(5)), 5e-10, 10)
        self.assertNotAlmostEqual(lm.exp(lm.log(1e-10) + lm.log(5)), 2e-10, 10)
        self.assertAlmostEqual(math.pow(10, lm.log_to_log10(lm.log(0.123))), 0.123, 3)
        self.assertAlmostEqual(math.exp(lm.log_to_ln(lm.log(0.123))), 0.123, 3)
        self.assertAlmostEqual(lm.exp(lm.log10_to_log(-2)), 0.01, 2)
        self.assertAlmostEqual(lm.exp(lm.ln_to_log(math.log(0.01))), 0.01, 2)

    def test_other_base(self):
        lm1 = LogMath(base=1.0001)
        lm3 = LogMath(base=1.0003)
        self.assertNotEqual(lm3.log(0.1), lm1.log(0.1))
        self.assertAlmostEqual(lm3.exp(lm3.add(lm3.log(0.1), lm3.log(0.1))), 0.2, 1)
        self.assertNotAlmostEqual(lm3.exp(lm3.add(lm3.log(0.1), lm3.log(0.1))), 0.3, 1)


if __name__ == '__main__':
    unittest.main()

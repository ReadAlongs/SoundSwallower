#!/usr/bin/env python3
# Copyright (c) 1999-2016 Carnegie Mellon University. All rights
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
import os
from soundswallower import Config, Decoder, get_model_path


class TestConfig(unittest.TestCase):

    def test_config_get_float(self):
        config = Config()
        self.assertEqual(config.get_float('-samprate'), 16000.0)

    def test_config_set_float(self):
        config = Config()
        config.set_float('-samprate', 8000.0)
        self.assertEqual(config.get_float('-samprate'), 8000.0)

    def test_config_get_int(self):
        config = Config()
        self.assertEqual(config.get_int('-nfft'), 0)

    def test_config_set_int(self):
        config = Config()
        config.set_int('-nfft', 256)
        self.assertEqual(config.get_int('-nfft'), 256)

    def test_config_get_string(self):
        config = Config()
        self.assertEqual(config.get_string('-rawlogdir'), None)

    def test_config_set_string(self):
        config = Config()
        config.set_string('-rawlogdir', '~/pocketsphinx')
        self.assertEqual(config.get_string('-rawlogdir'), '~/pocketsphinx')

    def test_config_get_boolean(self):
        config = Config()
        self.assertEqual(config.get_boolean('-backtrace'), False)

    def test_config_set_boolean(self):
        config = Config()
        config.set_boolean('-backtrace', True)
        self.assertEqual(config.get_boolean('-backtrace'), True)


class TestConfigHash(unittest.TestCase):
    def test_config__getitem(self):
        config = Config()
        self.assertEqual(config['samprate'], 16000.)
        self.assertEqual(config['nfft'], 0)
        self.assertEqual(config['rawlogdir'], None)
        self.assertEqual(config['backtrace'], False)
        self.assertEqual(config['feat'], '1s_c_d_dd')

    def test_config_easyinit(self):
        config = Config(samprate=11025.,
                        rawlogdir=None,
                        backtrace=False,
                        feat="1s_c_d_dd")
        self.assertEqual(config['samprate'], 11025.)
        self.assertEqual(config.get_float('-samprate'), 11025.)
        self.assertEqual(config['nfft'], 0)
        self.assertEqual(config['rawlogdir'], None)
        self.assertEqual(config['backtrace'], False)
        self.assertEqual(config['feat'], '1s_c_d_dd')

    def test_config_coercion(self):
        config = Config()
        config["samprate"] = 48000
        self.assertEqual(config['samprate'], 48000.)
        config["nfft"] = "1024"
        self.assertEqual(config['nfft'], 1024)


DATADIR = os.path.join(os.path.dirname(__file__),
                       "..", "..", "tests", "data")


class TestConfigIter(unittest.TestCase):
    def test_config__iter(self):
        config = Config()
        default_len = len(config)
        for key in config:
            self.assertTrue(key in config)
        for key, value in config.items():
            self.assertTrue(key in config)
            self.assertEqual(config[key], value)
        config = Decoder.default_config()
        self.assertEqual(default_len, len(config))
        config['hmm'] = os.path.join(get_model_path(), 'en-us')
        config['fsg'] = os.path.join(DATADIR, 'goforward.fsg')
        config['dict'] = os.path.join(DATADIR, 'turtle.dic')
        self.assertEqual(default_len, len(config))
        for key in config:
            self.assertTrue(key in config)
        for key, value in config.items():
            self.assertTrue(key in config)
            self.assertEqual(config[key], value)
        self.assertIsNone(config["mdef"])
        # Now check weird extra value stuff.  Length should never change
        _ = Decoder(config)
        # But mdef, etc, should be filled in
        default_mdef = config["mdef"]
        self.assertIsNotNone(default_mdef)
        # And we should get them for dash and underscore versions too
        self.assertEqual(default_mdef, config["-mdef"])
        self.assertEqual(default_mdef, config["_mdef"])
        self.assertEqual(default_len, len(config))
        for key in config:
            self.assertTrue(key in config)
        for key, value in config.items():
            self.assertTrue(key in config)
            self.assertEqual(config[key], value)


class TestConfigDefn(unittest.TestCase):
    def test_config_describe(self):
        config = Config()
        for defn in config.describe():
            if defn.name == 'hmm':
                self.assertTrue(defn.required)


if __name__ == '__main__':
    unittest.main()

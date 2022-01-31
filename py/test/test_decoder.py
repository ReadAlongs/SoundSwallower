#!/usr/bin/python3

import os
import unittest
from soundswallower import Decoder, get_model_path


DATADIR = os.path.join(os.path.dirname(__file__),
                       "..", "..", "tests", "data")


class TestDecoder(unittest.TestCase):
    def test_turtle(self):
        config = Decoder.default_config()
        config['-hmm'] = os.path.join(get_model_path(), 'en-us')
        config['-fsg'] = os.path.join(DATADIR, 'goforward.fsg')
        config['-dict'] = os.path.join(DATADIR, 'turtle.dic')
        config['-logfn'] = '/dev/null'
        decoder = Decoder(config)
        with open(os.path.join(DATADIR, 'goforward.raw'), 'rb') as fh:
            buf = fh.read()
            decoder.start_utt()
            decoder.process_raw(buf, full_utt=True)
            decoder.end_utt()
            self.assertEqual(decoder.hyp().hypstr, "go forward ten meters")

    def test_live(self):
        config = Decoder.default_config()
        config['-hmm'] = os.path.join(get_model_path(), 'en-us')
        config['-fsg'] = os.path.join(DATADIR, 'goforward.fsg')
        config['-dict'] = os.path.join(DATADIR, 'turtle.dic')
        config['-logfn'] = '/dev/null'
        decoder = Decoder(config)
        with open(os.path.join(DATADIR, 'goforward.raw'), 'rb') as fh:
            decoder.start_utt()
            while True:
                buf = fh.read(1024)
                if not buf:
                    break
                decoder.process_raw(buf, full_utt=False)
                hyp = decoder.hyp()
                if hyp is not None:
                    self.assertTrue("go forward ten meters".startswith(hyp.hypstr))
            decoder.end_utt()
            self.assertEqual(decoder.hyp().hypstr, "go forward ten meters")


if __name__ == "__main__":
    unittest.main()
    

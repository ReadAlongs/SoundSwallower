#!/usr/bin/python3

import os
import unittest
from soundswallower import Decoder, get_model_path


DATADIR = os.path.join(os.path.dirname(__file__),
                       "..", "..", "tests", "data")
JSGF = """#JSGF V1.0;

/**
 * JSGF Grammar for Turtle example
 */

grammar goforward;

public <move> = go forward ten meters;

public <move2> = go <direction> <distance> [meter | meters];

<direction> = forward | backward;

<distance> = one | two | three | four | five | six | seven | eight | nine | ten;
"""

class TestDecodeFSG(unittest.TestCase):
    def _run_decode(self, decoder):
        with open(os.path.join(DATADIR, 'goforward.raw'), "rb") as fh:
            buf = fh.read()
            decoder.start_utt()
            decoder.process_raw(buf, full_utt=True)
            decoder.end_utt()
            self.assertEqual(decoder.hyp().hypstr, "go forward ten meters")
            words = []
            for seg in decoder.seg():
                if seg.word != "(NULL)":
                    words.append(seg.word)
            self.assertEqual(words, "<sil> go forward ten meters <sil>".split())

    def test_fsg_loading(self):
        config = Decoder.default_config()
        config['-hmm'] = os.path.join(get_model_path(), 'en-us')
        config['-dict'] = os.path.join(DATADIR, 'turtle.dic')
        config['-logfn'] = '/dev/null'
        decoder = Decoder(config)
        # Read a file that isn't a FSG
        with self.assertRaises(RuntimeError):
            fsg = decoder.read_fsg(os.path.join(DATADIR, 'goforward.raw'))
        # OK, read the real thing :)
        fsg = decoder.read_fsg(os.path.join(DATADIR, 'goforward.fsg'))
        decoder.set_fsg(fsg)
        self._run_decode(decoder)

    def test_jsgf_loading(self):
        config = Decoder.default_config()
        config['-hmm'] = os.path.join(get_model_path(), 'en-us')
        config['-dict'] = os.path.join(DATADIR, 'turtle.dic')
        #config['-logfn'] = '/dev/null'
        decoder = Decoder(config)
        # Read a file that isn't a JSGF
        with self.assertRaises(RuntimeError):
            decoder.set_jsgf_file(os.path.join(DATADIR, 'goforward.fsg'))
        # OK, read the real thing :)
        decoder.set_jsgf_file(os.path.join(DATADIR, 'goforward.gram'))
        self._run_decode(decoder)
        # Try loading it as a string
        decoder.set_jsgf_string(JSGF)
        self._run_decode(decoder)

if __name__ == "__main__":
    unittest.main()
    

#!/usr/bin/python3

import os
import unittest
from soundswallower import Decoder, get_model_path


DATADIR = os.path.join(os.path.dirname(__file__),
                       "..", "..", "tests", "data")


class TestDecoder(unittest.TestCase):
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

    def test_from_scratch(self):
        decoder = Decoder(hmm=os.path.join(get_model_path(), 'en-us'),
                          logfn="/dev/null", beam=0., wbeam=0., pbeam=0.)
        words = [("go", "G OW"),
                 ("forward", "F AO R W ER D"),
                 ("ten", "T EH N"),
                 ("meters", "M IY T ER Z")]
        for word, pron in words:
            decoder.add_word(word.encode("UTF-8"), pron.encode("UTF-8"),
                             # FIXME: should update them all at once
                             update=True)
        fsg = decoder.create_fsg("align",
                                 start_state=0, final_state=len(words),
                                 transitions=[(idx, idx + 1, 1.0, w[0])
                                              for idx, w
                                              in enumerate(words)])
        decoder.set_fsg(fsg)
        self._run_decode(decoder)

    def test_reinit(self):
        decoder = Decoder(hmm=os.path.join(get_model_path(), 'en-us'),
                          fsg=os.path.join(DATADIR, 'goforward.fsg'),
                          dict=os.path.join(DATADIR, 'turtle.dic'),
                          logfn="/dev/null")
        self._run_decode(decoder)
        # Reinitialize with a bogus FSG that recognizes the wrong word
        decoder.config["fsg"] = os.path.join(DATADIR, 'goforward2.fsg')
        decoder.reinit()
        # And verify that this fails :)
        with self.assertRaises(AssertionError):
            self._run_decode(decoder)
        
    def test_turtle(self):
        config = Decoder.default_config()
        config['-hmm'] = os.path.join(get_model_path(), 'en-us')
        config['-fsg'] = os.path.join(DATADIR, 'goforward.fsg')
        config['-dict'] = os.path.join(DATADIR, 'turtle.dic')
        config['-logfn'] = '/dev/null'
        decoder = Decoder(config)
        self._run_decode(decoder)

    def test_live(self):
        config = Decoder.default_config()
        config['-hmm'] = os.path.join(get_model_path(), 'en-us')
        config['-fsg'] = os.path.join(DATADIR, 'goforward.fsg')
        config['-dict'] = os.path.join(DATADIR, 'turtle.dic')
        config['-logfn'] = '/dev/null'
        decoder = Decoder(config)
        self._run_decode(decoder)


if __name__ == "__main__":
    unittest.main()
    

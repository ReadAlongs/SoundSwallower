#!/usr/bin/python3

import os
import unittest
from typing import Iterator

from soundswallower import Decoder, Seg, get_model_path

DATADIR = os.path.join(os.path.dirname(__file__), "..", "..", "tests", "data")


class TestDecoder(unittest.TestCase):
    def _run_decode(self, decoder: Decoder) -> None:
        with open(os.path.join(DATADIR, "goforward.raw"), "rb") as fh:
            buf = fh.read()
            decoder.start_utt()
            decoder.process_raw(buf, full_utt=True)
            decoder.end_utt()
            self._check_hyp(decoder.hyp.text, decoder.seg)

    def _check_hyp(self, hyp: str, hypseg: Iterator[Seg]) -> None:
        self.assertEqual(hyp, "go forward ten meters")
        words = []
        for seg in hypseg:
            if seg.text not in ("<sil>", "(NULL)"):
                words.append(seg.text)
        self.assertEqual(words, "go forward ten meters".split())

    def test_from_scratch(self) -> None:
        decoder = Decoder.create(
            hmm=os.path.join(get_model_path(), "en-us"),
            beam=0.0,
            wbeam=0.0,
            pbeam=0.0,
            loglevel="INFO",
        )
        del decoder.config["dict"]
        decoder.initialize()
        words = [
            ("go", "G OW"),
            ("forward", "F AO R W ER D"),
            ("ten", "T EH N"),
            ("meters", "M IY T ER Z"),
        ]
        for word, pron in words:
            decoder.add_word(word, pron, update=True)
        fsg = decoder.create_fsg(
            "align",
            start_state=0,
            final_state=len(words),
            transitions=[(idx, idx + 1, 1.0, w[0]) for idx, w in enumerate(words)],
        )
        decoder.set_fsg(fsg)
        self._run_decode(decoder)

    def test_reinit(self) -> None:
        decoder = Decoder(
            hmm=os.path.join(get_model_path(), "en-us"),
            fsg=os.path.join(DATADIR, "goforward.fsg"),
            dict=os.path.join(DATADIR, "turtle.dic"),
        )
        self._run_decode(decoder)
        # Reinitialize with a bogus FSG that recognizes the wrong word
        decoder.config["fsg"] = os.path.join(DATADIR, "goforward2.fsg")
        decoder.initialize()
        # And verify that this fails :)
        with self.assertRaises(AssertionError):
            self._run_decode(decoder)

    def test_reinit_feat(self) -> None:
        # Initialize with wrong sampling rate
        decoder = Decoder(
            hmm=os.path.join(get_model_path("en-us")),
            dict=os.path.join(DATADIR, "turtle.dic"),
            samprate=11025,
        )
        # Verify that reinit_fe does not break current grammar
        fsg = decoder.read_fsg(os.path.join(DATADIR, "goforward.fsg"))
        decoder.set_fsg(fsg)
        decoder.config["samprate"] = 16000
        decoder.reinit_feat()
        self._run_decode(decoder)

    def test_turtle(self) -> None:
        decoder = Decoder(
            hmm=os.path.join(get_model_path("en-us")),
            fsg=os.path.join(DATADIR, "goforward.fsg"),
            dict=os.path.join(DATADIR, "turtle.dic"),
        )
        self._run_decode(decoder)

    def test_loglevel(self) -> None:
        Decoder(hmm=os.path.join(get_model_path(), "en-us"), loglevel="FATAL")
        with self.assertRaises(RuntimeError):
            Decoder(
                hmm=os.path.join(get_model_path(), "en-us"), loglevel="FOOBIEBLETCH"
            )

    def test_decode_file(self) -> None:
        """Test decode_file on a variety of data."""
        decoder = Decoder(
            hmm=os.path.join(get_model_path("en-us")),
            dict=os.path.join(DATADIR, "turtle.dic"),
        )
        # Verify that decode_file does not break current grammar
        fsg = decoder.read_fsg(os.path.join(DATADIR, "goforward.fsg"))
        decoder.set_fsg(fsg)
        hyp, hypseg = decoder.decode_file(os.path.join(DATADIR, "goforward.wav"))
        self._check_hyp(hyp, hypseg)
        # Verify that decoding a too-narrow-band file will fail
        with self.assertRaises(RuntimeError):
            hyp, hypseg = decoder.decode_file(os.path.join(DATADIR, "goforward4k.wav"))
            self._check_hyp(hyp, hypseg)

    def test_decode_fail(self) -> None:
        """Test failure to initialize (should not segfault!)"""
        with self.assertRaises(RuntimeError):
            _ = Decoder(
                hmm=os.path.join(get_model_path("en-us")),
                fsg=os.path.join(DATADIR, "goforward.fsg"),
                dict=os.path.join(DATADIR, "turtle.dic"),
                samprate=4000,
            )


if __name__ == "__main__":
    unittest.main()

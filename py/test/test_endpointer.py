#!/usr/bin/python3

import os
import unittest

import numpy as np
from soundswallower import Endpointer

DATADIR = os.path.join(os.path.dirname(__file__), "..", "..", "tests", "data")


class TestEndpointer(unittest.TestCase):
    def test_endpointer(self) -> None:
        ep = Endpointer(sample_rate=16000)
        nsamp = 0
        with open(os.path.join(DATADIR, "goforward-float32.raw"), "rb") as fh:
            while True:
                npframe = np.frombuffer(fh.read(ep.frame_bytes * 2), dtype=np.float32)
                nsamp += len(npframe)
                frame = (npframe * 32767).astype(np.int16).tobytes()
                prev_in_speech = ep.in_speech
                if len(frame) == 0:
                    break
                elif len(frame) < ep.frame_bytes:
                    speech = ep.end_stream(frame)
                else:
                    speech = ep.process(frame)
                if speech is not None:
                    if not prev_in_speech:
                        print("Start speech at", ep.speech_start)
                        self.assertAlmostEqual(ep.speech_start, 1.53)
                    if not ep.in_speech:
                        print("End speech at", ep.speech_end)
                        self.assertAlmostEqual(ep.speech_end, 3.39)
        print("Read", nsamp, "samples")


if __name__ == "__main__":
    unittest.main()

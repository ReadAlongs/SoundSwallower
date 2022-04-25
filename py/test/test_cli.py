#!/usr/bin/python3

import os
import unittest
from soundswallower import cli


DATADIR = os.path.join(os.path.dirname(__file__),
                       "..", "..", "tests", "data")


class TestCLI(unittest.TestCase):
    cli.main(("--grammar", os.path.join(DATADIR, "goforward.gram"),
              os.path.join(DATADIR, "goforward.wav"),
              os.path.join(DATADIR, "goforward.raw")))


if __name__ == "__main__":
    unittest.main()
    

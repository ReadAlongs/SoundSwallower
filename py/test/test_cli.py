#!/usr/bin/python3

import os
import unittest
from soundswallower import cli, get_model_path


DATADIR = os.path.join(os.path.dirname(__file__),
                       "..", "..", "tests", "data")


class TestCLI(unittest.TestCase):
    def test_cli_basic(self):
        cli.main(("--grammar", os.path.join(DATADIR, "goforward.gram"),
                  os.path.join(DATADIR, "goforward.wav"),
                  os.path.join(DATADIR, "goforward.raw")))
        self.assertTrue(True)

    def test_cli_other_model(self):
        cli.main(("--grammar", os.path.join(DATADIR, "goforward_fr.gram"),
                  "--model", get_model_path("fr-fr"),
                  os.path.join(DATADIR, "goforward_fr.wav"),
                  os.path.join(DATADIR, "goforward_fr.raw")))
        self.assertTrue(True)


if __name__ == "__main__":
    unittest.main()
    

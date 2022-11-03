#!/usr/bin/python3

import os
import re
import json
import unittest
from soundswallower import cli, get_model_path
from tempfile import TemporaryDirectory

DATADIR = os.path.join(os.path.dirname(__file__),
                       "..", "..", "tests", "data")


def baseword(w):
    return re.sub(r"\(\d+\)$", "", w["t"])


class TestCLI(unittest.TestCase):
    def check_output(self, jpath, text="go forward ten meters"):
        with open(jpath, "rt") as infh:
            for line in infh:
                result = json.loads(line)
                self.assertTrue(result)
                self.assertEqual(result["t"], text)
                for word, ref in zip((w for w in result["w"]
                                      if w["t"] != "<sil>"),
                                     text.split()):
                    self.assertEqual(baseword(word), ref)

    def test_cli_basic(self):
        with TemporaryDirectory() as tmpdir:
            jpath = os.path.join(tmpdir, "output.json")
            cli.main(("--grammar", os.path.join(DATADIR, "goforward.gram"),
                      "-o", jpath,
                      os.path.join(DATADIR, "goforward.wav"),
                      os.path.join(DATADIR, "goforward.raw")))
            self.check_output(jpath)

    def test_cli_other_model(self):
        with TemporaryDirectory() as tmpdir:
            jpath = os.path.join(tmpdir, "output.json")
            cli.main(["--grammar", os.path.join(DATADIR, "goforward_fr.gram"),
                      "--model", get_model_path("fr-fr"),
                      "--output", jpath,
                      os.path.join(DATADIR, "goforward_fr.wav"),
                      os.path.join(DATADIR, "goforward_fr.raw")])
            self.check_output(jpath, "avance de dix mètres")

    def test_cli_config(self):
        with TemporaryDirectory() as tmpdir:
            jpath = os.path.join(tmpdir, "config.json")
            cli.main(["--write-config", jpath])
            with open(jpath, "rt") as infh:
                self.assertTrue(json.load(infh))

    def test_cli_align(self):
        with TemporaryDirectory() as tmpdir:
            jpath = os.path.join(tmpdir, "output.json")
            cli.main(("--output", jpath,
                      "--align-text", "go forward ten meters",
                      "--phone-align",
                      os.path.join(DATADIR, "goforward.wav"),
                      os.path.join(DATADIR, "goforward.raw")))
            self.check_output(jpath)
            cli.main(("--output", jpath,
                      "--align-text", "go forward ten meters",
                      os.path.join(DATADIR, "goforward.raw")))
            # Check that a single output also works
            with open(jpath, "rt") as infh:
                self.assertTrue(json.load(infh))


if __name__ == "__main__":
    unittest.main()

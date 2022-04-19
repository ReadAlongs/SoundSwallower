#!/usr/bin/python3

import os
import unittest
from soundswallower import cli


DATADIR = os.path.join(os.path.dirname(__file__),
                       "..", "..", "tests", "data")


class TestCLI(unittest.TestCase):
    pass


if __name__ == "__main__":
    unittest.main()
    

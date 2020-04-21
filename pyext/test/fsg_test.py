#!/usr/bin/python3

import sys

from sphinxbase import LogMath
from sphinxbase import FsgModel

lmath = LogMath()
fsg = FsgModel("simple_grammar", lmath, 1.0, 10)
fsg.word_add("hello")
fsg.word_add("world")
print (fsg.word_id("world"))

fsg.add_silence("<sil>", 1, 0.5)
fsg.trans_add(0, 1, lmath.log(1.0), fsg.word_id('hello'))
fsg.trans_add(1, 1, lmath.log(0.5), fsg.word_id('hello'))
fsg.trans_add(1, 2, lmath.log(0.5), fsg.word_id('world'))
fsg.set_final_state(2)


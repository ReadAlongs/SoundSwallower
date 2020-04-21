#!/usr/bin/python

from os import environ, path

from soundswallower import Decoder

MODELDIR = "../model"
DATADIR = "../tests/data"

# Create a decoder with certain model
config = Decoder.default_config()
config.set_string('-hmm', path.join(MODELDIR, 'en-us/en-us'))
config.set_string('-fsg', path.join(DATADIR, 'goforward.fsg'))
config.set_string('-dict', path.join(DATADIR, 'turtle.dic'))
decoder = Decoder(config)

decoder.start_utt()
stream = open(path.join(DATADIR, 'goforward.raw'), 'rb')
while True:
    buf = stream.read(1024)
    if buf:
         decoder.process_raw(buf, False, False)
    else:
         break
decoder.end_utt()

decoder.get_lattice().write('goforward.lat')
decoder.get_lattice().write_htk('goforward.htk')

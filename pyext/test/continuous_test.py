#!/usr/bin/python3

from os import environ, path

from soundswallower import Decoder

MODELDIR = "../model"
DATADIR = "../tests/data"

config = Decoder.default_config()
config.set_string('-hmm', path.join(MODELDIR, 'en-us/en-us'))
config.set_string('-fsg', path.join(DATADIR, 'goforward.fsg'))
config.set_string('-dict', path.join(DATADIR, 'turtle.dic'))
config.set_string('-logfn', '/dev/null')
decoder = Decoder(config)

stream = open(path.join(DATADIR, 'goforward.raw'), 'rb')

in_speech_bf = False
decoder.start_utt()
while True:
    buf = stream.read(1024)
    if buf:
        decoder.process_raw(buf, False, False)
        if decoder.get_in_speech() != in_speech_bf:
            in_speech_bf = decoder.get_in_speech()
            if not in_speech_bf:
                decoder.end_utt()
                print('Result:', decoder.hyp().hypstr)
                decoder.start_utt()
    else:
        break
decoder.end_utt()

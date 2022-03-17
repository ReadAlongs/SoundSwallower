#!/usr/bin/env python3

"""Command-line interface for SoundSwallower.  Takes audio files as
input and outputs JSON (to standard output, or a file) containing a
list of time alignments for recognized words.

Basic usage, to recognize using a JSGF grammar:

  soundswallower --grammar input.gram audio.wav

To force-align text to audio:

  soundswallower --align input.txt input.wav

To use a different model:

  soundswallower --model fr-fr ...
  soundswallower --model /path/to/model/

To use a custom dictionary:

  soundswallower --dict /path/to/dictionary.dict

"""

import soundswallower as ss
import logging
import argparse
import tempfile
import json
import wave
import os


def make_argparse():
    """Function to make the argument parser (for auto-documentation purposes)"""
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("inputs", nargs="*", help="Input files.")
    parser.add_argument("--dict", help="Custom dictionary file.")
    parser.add_argument("--model",
                        help="Specific model, built-in or from directory.",
                        default='en-us')
    parser.add_argument("--config", help="JSON file with decoder configuration.")
    parser.add_argument("--write-config",
                        help="Write full configuration as JSON to OUTPUT "
                        "(or standard output if none given) and exit.")
    parser.add_argument("-o", "--output",
                        help="Filename for output (default is standard output")
    grammars = parser.add_mutually_exclusive_group(required=True)
    grammars.add_argument("-a", "--align", help="Input text for force alignment.")
    grammars.add_argument("-g", "--grammar", help="Grammar file for recognition.")
    parser.add_argument_group(grammars)
    return parser


def make_decoder_config(args):
    """Make a decoder configuration from command-line arguments, possibly
    including a JSON configuration file."""
    config = ss.Decoder.default_config()
    if args.config is not None:
        with open(args.config) as fh:
            json_config = json.load(fh)
            for key, value in json_config.items():
                config[key] = value
    model_path = ss.get_model_path()
    if args.model in os.listdir(model_path):
        config["hmm"] = os.path.join(model_path, args.model)
        config["dict"] = os.path.join(model_path,
                                      args.model + '.dict')
    else:
        config["hmm"] = args.model
        config["dict"] = os.path.normpath(args.model) + '.dict'
    # FIXME: This actually should be in addition to the built-in
    # dictionary, or we should have G2P support, which shouldn't be
    # all that hard, we hope.
    if args.dict is not None:
        config["dict"] = args.dict

    if args.grammar is not None:
        config["jsgf"] = args.grammar

    return config


def write_config(config, output=None):
    """Write the full configuraiton as JSON to output file or standard output."""
    config_dict = {}
    # FIXNE: Not yet implemented, requires iteration over config
    pass


def get_audio_data(input_file):
    """Try to get single-channel audio data in the most portable way
    possible."""
    wavfile = wave.open(input_file)
    if wavfile.getnchannels() != 1:
        raise ValueError("Only supporting single-channel WAV")
    data = wavfile.readframes(wavfile.getnframes())
    sample_rate = wavfile.getframerate()
    # FIXME: Do that above in a try block, then switch to pydub if failed
    return data, sample_rate


def decode_file(decoder, config, args, input_file):
    """Decode one input audio file."""
    # Note that we always decode the entire file at once. It would
    # have to be really huge for this to cause memory problems, in
    # which case the decoder would explode anyway. Otherwise, CMN
    # doesn't work as well, which causes unnecessary recognition
    # errors.
    data, sample_rate = get_audio_data(input_file)
    # Reinitialize the decoder if necessary
    if sample_rate != config.get_float("-samprate"):
        logging.info("Setting sample rate to %d", sample_rate)
        config["samprate"] = sample_rate
        # Calculate the minimum required FFT size for the sample rate
        frame_points = int(sample_rate * config.get_float("-wlen"))
        fft_size = 1
        while fft_size < frame_points:
            fft_size = fft_size << 1
        if fft_size > config.get_int("-nfft"):
            logging.info("Increasing FFT size to %d for sample rate %d",
                         fft_size, sample_rate)
            config["nfft"] = fft_size
        decoder.reinit(config)
    frame_size = 1.0 / config.get_int('-frate')

    decoder.start_utt()
    decoder.process_raw(data, no_search=False, full_utt=True)
    decoder.end_utt()

    if not decoder.seg():
        raise RuntimeError("Decoding produced no segments, "
                           "please examine dictionary/grammar and input audio.")

    results = []
    for seg in decoder.seg():
        start = seg.start_frame * frame_size
        end = (seg.end_frame + 1) * frame_size
        if seg.word in ('<sil>', '[NOISE]'):
            continue
        else:
            results.append({
                "id": seg.word,
                "start": start,
                "end": end
            })
        logging.info("Segment: %s (%.3f : %.3f)",
                     seg.word, start, end)

    if len(results) == 0:
        raise RuntimeError("Decoding produced only noise or silence segments, "
                           "please examine dictionary and input audio and text.")

    return results


def make_fsg(outfh, words):
    """Make an FSG for word-level "force-alignment" that is just a linear
    chain of all the words."""
    outfh.write("""FSG_BEGIN align
NUM_STATES %d
START_STATE 0
FINAL_STATE %d
""" % (len(words) + 1, len(words)))
    for i, w in enumerate(words):
        outfh.write("TRANSITION %d %d 1.0 %s\n" % (i, i + 1, w))
    outfh.write("FSG_END\n")
    outfh.flush()


def main(argv=None):
    """Main entry point for SoundSwallower."""
    logging.basicConfig(level=logging.INFO)
    parser = make_argparse()
    args = parser.parse_args(argv)
    config = make_decoder_config(args)
    if args.write_config:
        write_config(config, args.output)
    if args.align:
        words = []
        with open(args.align) as fh:
            words = fh.read().strip().split()
        fsg_temp = tempfile.NamedTemporaryFile(mode="w")
        make_fsg(fsg_temp, words)
        config["fsg"] = fsg_temp.name
    decoder = ss.Decoder(config)
    results = []
    for input_file in args.inputs:
        file_align = decode_file(decoder, config, args, input_file)
        results.append(file_align)
    if args.output is not None:
        with open(args.output, 'w') as outfh:
            json.dump(results, outfh)
    else:
        print(json.dumps(results))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

"""Command-line interface for SoundSwallower.  Takes audio files as
input and outputs JSON (to standard output, or a file) containing a
list of time alignments for recognized words.

Basic usage, to recognize using a JSGF grammar::

  soundswallower --grammar input.gram audio.wav

To force-align text to audio::

  soundswallower --align input.txt input.wav

To use a different model::

  soundswallower --model fr-fr ...
  soundswallower --model /path/to/model/

To use a custom dictionary::

  soundswallower --dict /path/to/dictionary.dict

"""

from soundswallower import Decoder, get_model_path
import logging
import argparse
import json
import sys
import os


def make_argparse():
    """Function to make the argument parser (for auto-documentation purposes)"""
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("inputs", nargs="*", help="Input files.")
    parser.add_argument("--help-config", action="store_true",
                        help="Print help for decoder configuration parameters.")
    parser.add_argument("--dict", help="Custom dictionary file.")
    parser.add_argument("--model",
                        help="Specific model, built-in or from directory.",
                        default='en-us')
    parser.add_argument("--config", help="JSON file with decoder configuration.")
    parser.add_argument("-s", "--set", action="append",
                        help="Set configuration parameter (KEY=VALUE).")
    parser.add_argument("--write-config",
                        help="Write full configuration as JSON to OUTPUT "
                        "(or standard output if none given) and exit.")
    parser.add_argument("-o", "--output",
                        help="Filename for output (default is standard output")
    parser.add_argument("-v", "--verbose",
                        action="store_true", help="Be verbose.")
    grammars = parser.add_mutually_exclusive_group()
    grammars.add_argument("-a", "--align", help="Input text file for force alignment.")
    grammars.add_argument("-t", "--align-text",
                          help="Input text for force alignment.")
    grammars.add_argument("-g", "--grammar", help="Grammar file for recognition.")
    grammars.add_argument("-f", "--fsg", help="FSG file for recognition.")
    parser.add_argument_group(grammars)
    return parser


def print_config_help():
    """Describe the decoder configuration parameters."""
    config = Decoder.default_config()
    print("Configuration parameters:")
    for defn in config.describe():
        print("\t%s (%s%s%s):\n\t\t%s"
              % (defn.name,
                 defn.type.__name__,
                 ", required" if defn.required else "",
                 (", default: %s" % defn.default) if defn.default else "",
                 defn.doc))


def make_decoder_config(args):
    """Make a decoder configuration from command-line arguments, possibly
    including a JSON configuration file."""
    config = Decoder.default_config()
    if args.config is not None:
        with open(args.config) as fh:
            json_config = json.load(fh)
            for key, value in json_config.items():
                config[key] = value
    model_path = get_model_path()
    if args.model in os.listdir(model_path):
        config["hmm"] = os.path.join(model_path, args.model)
        config["dict"] = os.path.join(model_path,
                                      args.model, 'dict.txt')
    else:
        config["hmm"] = args.model
        config["dict"] = os.path.join(os.path.normpath(args.model), 'dict.txt')
    # FIXME: This actually should be in addition to the built-in
    # dictionary, or we should have G2P support, which shouldn't be
    # all that hard, we hope.
    if args.dict is not None:
        config["dict"] = args.dict

    if args.grammar is not None:
        config["jsgf"] = args.grammar

    if args.fsg is not None:
        config["fsg"] = args.fsg

    if args.verbose:
        config["loglevel"] = "INFO"
        config["backtrace"] = True

    if args.set:
        for kv in args.set:
            key, value = kv.split('=')
            config[key] = value
    return config


def write_config(config, output=None):
    """Write the full configuraiton as JSON to output file or standard output."""
    if output is not None:
        outfh = open(output, "wt")
    else:
        outfh = sys.stdout
    json.dump(dict(config.items()), outfh)
    if output is not None:
        outfh.close()


def set_alignment_fsg(decoder, words):
    """Make an FSG for word-level "force-alignment" that is just a linear
    chain of all the words."""
    fsg = decoder.create_fsg("align",
                             start_state=0, final_state=len(words),
                             transitions=[(idx, idx + 1, 1.0, w)
                                          for idx, w
                                          in enumerate(words)])
    decoder.set_fsg(fsg)


def main(argv=None):
    """Main entry point for SoundSwallower."""
    logging.basicConfig(level=logging.INFO)
    parser = make_argparse()
    args = parser.parse_args(argv)
    if args.help_config:
        print_config_help()
        sys.exit(0)
    config = make_decoder_config(args)
    if args.write_config is not None:
        write_config(config, args.write_config)
    words = None
    if args.align_text:
        words = args.align_text.split()
    elif args.align:
        words = []
        with open(args.align) as fh:
            words = fh.read().strip().split()
    elif args.grammar or args.fsg:
        pass
    else:
        # Nothing to do!
        return
    decoder = Decoder(config)
    if words is not None:
        set_alignment_fsg(decoder, words)
    results = []
    for input_file in args.inputs:
        _, file_align = decoder.decode_file(input_file)
        results.append([{"id": word,
                         "start": start,
                         "end": end} for word, start, end in file_align])
    if args.output is not None:
        with open(args.output, 'w') as outfh:
            json.dump(results, outfh)
    else:
        print(json.dumps(results))


if __name__ == "__main__":
    main()

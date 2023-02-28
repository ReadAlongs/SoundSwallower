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

import argparse
import logging
import os
import sys
from typing import Any, Optional, Sequence

from soundswallower import Config, Decoder, get_model_path


def make_argparse() -> argparse.ArgumentParser:
    """Function to make the argument parser (for auto-documentation purposes)"""
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("inputs", nargs="*", help="Input files.")
    parser.add_argument(
        "--help-config",
        action="store_true",
        help="Print help for decoder configuration parameters.",
    )
    parser.add_argument("--dict", help="Custom dictionary file.")
    parser.add_argument(
        "--model", help="Specific model, built-in or from directory.", default="en-us"
    )
    parser.add_argument("--config", help="JSON file with decoder configuration.")
    parser.add_argument(
        "-s", "--set", action="append", help="Set configuration parameter (KEY=VALUE)."
    )
    parser.add_argument(
        "--write-config",
        help="Write full configuration as JSON to OUTPUT "
        "(or standard output if none given) and exit.",
    )
    parser.add_argument(
        "-o", "--output", help="Filename for output (default is standard output)"
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="Be verbose.")
    parser.add_argument(
        "--phone-align", help="Produce phone-level alignments", action="store_true"
    )
    grammars = parser.add_mutually_exclusive_group()
    grammars.add_argument("-a", "--align", help="Input text file for force alignment.")
    grammars.add_argument("-t", "--align-text", help="Input text for force alignment.")
    grammars.add_argument("-g", "--grammar", help="Grammar file for recognition.")
    grammars.add_argument("-f", "--fsg", help="FSG file for recognition.")
    parser.add_argument_group(grammars)
    return parser


def print_config_help(config: Config) -> None:
    """Describe the decoder configuration parameters."""
    print("Configuration parameters:")
    for defn in config.describe():
        print(
            "\t%s (%s%s%s):\n\t\t%s"
            % (
                defn.name,
                defn.type.__name__,
                ", required" if defn.required else "",
                (", default: %s" % defn.default) if defn.default else "",
                defn.doc,
            )
        )


def make_decoder_config(args: argparse.Namespace) -> Config:
    """Make a decoder configuration from command-line arguments, possibly
    including a JSON configuration file."""
    config = Config()
    if args.config is not None:
        with open(args.config) as fh:
            config_json = fh.read()
            config.parse_json(config_json)
    model_path = get_model_path()
    if args.model in os.listdir(model_path):
        config["hmm"] = os.path.join(model_path, args.model)
    else:
        config["hmm"] = args.model

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
            key, value = kv.split("=")
            config[key] = value
    return config


def write_config(config: Config, output: Optional[str] = None) -> None:
    """Write the full configuraiton as JSON to output file or standard output."""
    outfh: Any
    if output is not None:
        outfh = open(output, "wt")
    else:
        outfh = sys.stdout
    outfh.write(config.dumps())
    if output is not None:
        outfh.close()


def main(argv: Optional[Sequence[str]] = None) -> None:
    """Main entry point for SoundSwallower."""
    logging.basicConfig(level=logging.INFO)
    parser = make_argparse()
    args = parser.parse_args(argv)
    config = make_decoder_config(args)
    if args.help_config:
        print_config_help(config)
        sys.exit(0)
    if args.write_config is not None:
        write_config(config, args.write_config)
    if args.align:
        with open(args.align) as fh:
            args.align_text = fh.read().strip()
    elif args.grammar or args.fsg or args.align_text:
        pass
    else:
        # Nothing to do!
        return
    decoder = Decoder(config)
    if args.align_text is not None:
        decoder.set_align_text(args.align_text)
    results = []
    for input_file in args.inputs:
        decoder.decode_file(input_file)
        results.append(decoder.dumps(align_level=args.phone_align))
    if args.output is not None:
        with open(args.output, "w") as outfh:
            for json_line in results:
                outfh.write(json_line)
    else:
        for json_line in results:
            print(json_line)


if __name__ == "__main__":
    main()

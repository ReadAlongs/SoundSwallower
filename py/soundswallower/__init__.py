"""Main module for the SoundSwallower speech recognizer.

SoundSwallower is a small and not particularly powerful speech
recognition engine for constrained grammars.  It can also be used to
align text to audio.  Most of the functionality is contained in the
`Decoder` class.  Basic usage::

  from soundswallower import Decoder, get_model_path
  decoder = Decoder(hmm=get_model_path("en-us"),
                    dict=get_model_path("en-us.dict"),
                    jsgf="some_grammar_file.gram")
  hyp, seg = decoder.decode_file("example.wav")
  print("Recognized text:", hyp)
  for word, start, end in seg:
      print("Word %s from %.3f to %.3f" % (word, start, end))

"""
import collections
import os
import wave
from typing import Optional, Tuple

from ._soundswallower import Config  # noqa: F401
from ._soundswallower import Decoder  # noqa: F401
from ._soundswallower import Endpointer  # noqa: F401
from ._soundswallower import FsgModel  # noqa: F401
from ._soundswallower import Vad  # noqa: F401


def get_model_path(subpath: Optional[str] = None) -> str:
    """Return path to the model directory, or optionally, a specific file
    or directory within it.

    Args:
        subpath: An optional path to add to the model directory.

    Returns:
        The requested path within the model directory."""
    model_path = os.path.join(os.path.dirname(__file__), "model")
    if subpath is not None:
        return os.path.join(model_path, subpath)
    else:
        return model_path


def get_audio_data(input_file: str) -> Tuple[bytes, Optional[int]]:
    """Try to get single-channel audio data in the most portable way
    possible.

    Currently suports only single-channel WAV and raw audio.

    Args:
        input_file: Path to an audio file.

    Returns:
        (bytes, int): Raw audio data, sampling rate or `None` for a raw file.
    """
    try:
        with wave.open(input_file) as wavfile:
            if wavfile.getnchannels() != 1:
                raise ValueError("Only supporting single-channel WAV")
            data = wavfile.readframes(wavfile.getnframes())
            sample_rate = wavfile.getframerate()
            return data, sample_rate
    except wave.Error:
        with open(input_file, "rb") as rawfile:
            return rawfile.read(), None


Arg = collections.namedtuple("Arg", ["name", "default", "doc", "type", "required"])
Arg.__doc__ = "Description of a configuration parameter."
Arg.name.__doc__ = "Parameter name."
Arg.default.__doc__ = "Default value of parameter."
Arg.doc.__doc__ = "Description of parameter."
Arg.type.__doc__ = "Type (as a Python type object) of parameter value."
Arg.required.__doc__ = "Is this parameter required?"

Seg = collections.namedtuple("Seg", ["text", "start", "duration", "ascore", "lscore"])
Seg.__doc__ = "Segment in a word segmentation."
Seg.text.__doc__ = "Word text."
Seg.start.__doc__ = "Start time in the audio stream in seconds."
Seg.duration.__doc__ = "Duration in seconds."
Seg.ascore.__doc__ = "Acoustic match score."
Seg.lscore.__doc__ = "Language (grammar) match score."

Hyp = collections.namedtuple("Hyp", ["text", "score", "prob"])
Hyp.__doc__ = "Recognition hypothesis."
Hyp.text.__doc__ = "Recognized text."
Hyp.score.__doc__ = "Best path score."
Hyp.prob.__doc__ = "Posterior probability of hypothesis (often 1.0, sorry)."

__all__ = [
    "Config",
    "Decoder",
    "FsgModel",
    "Vad",
    "Endpointer",
    "Arg",
    "Seg",
    "Hyp",
    "get_model_path",
    "get_audio_data",
]

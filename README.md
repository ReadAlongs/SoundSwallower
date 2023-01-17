SoundSwallower: an even smaller speech recognizer
=================================================

> "Time and change have a voice; eternity is silent. The human ear is
> always searching for one or the other."<br>
> Leena Krohn, *Datura, or a delusion we all see*

SoundSwallower is a very small and simple speech recognizer intended
primarily for embedding in web applications.  The goal is not to
provide a fast implementation of large-vocabulary continuous speech
recognition, but rather to provide a *small* implementation of simple,
useful speech technologies.

With that in mind the current version is limited to finite-state
grammar recognition.  In addition, the eternally problematic and
badly-designed audio library as well as (almost) all other external
dependencies have been removed.

Compiling SoundSwallower
========================

Currently SoundSwallower can be built in several different ways. To
build the C library, run CMake in the standard way:

    cmake -S . -B build
    cmake --build build
    cmake --build build --target check
    sudo cmake --build --target install

Note that this isn't terribly useful as there is no command-line
frontend, and shared libraries are not built by default (pass
`-DBUILD_SHARED_LIBS=ON` if you insist).  You probably want to target
JavaScript or Python.

Installing the Python module and CLI
------------------------------------

The SoundSwallower command-line is a Python module
(soundswallower.cli) and can be installed using `pip`.  It is highly
recommended to do this in a `virtualenv`.  You can simply install it
from PyPI:

    pip install soundswallower

Or compile from source:

    pip install .

For development, you can install it in-place, but please make sure to
remove any existing global installation:

    pip uninstall soundswallower
    pip install -e .

The command-line supports JSGF grammars and word-level force
alignment for one or more input files, for example:

    soundswallower --align tests/data/goforward.txt tests/data/goforward.wav
    soundswallower --align-text "go forward ten meters" tests/data/goforward.wav
    soundswallower --grammar tests/data/goforward.gram tests/data/goforward.wav

Note that multiple input files are not particularly useful for
`--align` or `--align-text` as they will simply (try to) align the
same text to each file.  The output results (a list of time-aligned
words) can be written to a JSON file with `--output`.  To obtain
phoneme-level alignments, add the `--phone-align` flag.  The JSON
format (which has recently changed) is the same as used in
[PocketSphinx 5.0](https://github.com/cmusphinx/pocketsphinx) and is
more compact than it is readable, but briefly, it consists of one
dictionary (or "object" in JavaScript-ese) per line, where the `t`
attribute is the recognized text and the `w` attribute contains a list
of word segmentations, with start time in `b` and duration in `d` and,
optionally, a list of phone segmentations in the `w` attribute with
the same format.

See also the [full documentation of the Python
API](https://soundswallower.readthedocs.io/en/latest/soundswallower.html).

Compiling to JavaScript/WebAssembly
-----------------------------------

To use the JavaScript library in your projects:

    npm install soundswallower

To build the JavaScript library, use CMake with
[Emscripten](https://emscripten.org/):

    emcmake cmake -S . -B jsbuild
    cmake --build jsbuild

This will create `soundswallower.js` and `soundswallower.wasm` in the
`jsbuild` directory, which you can then include in your projects.  You
can also use `npm link` to link it to your `node_modules` folder for
development Demo applications can be seen at
https://github.com/dhdaines/alignment-demo and
https://github.com/dhdaines/soundswallower-demo.

To run the JavaScript tests:

    cd jsbuild
    npm install
    npm test
    npx tsc
    node test_typescript.js

And in the browser:

    cd jsbuild
    python server.py
    # Navigate to http://localhost:8000/test_web.html

For more details on the JavaScript implementation and API, see
[js/README.js](https://github.com/ReadAlongs/SoundSwallower/blob/master/js/README.md).

See also the [documentation of the JavaScript
API](https://soundswallower.readthedocs.io/en/latest/soundswallower.js.html).

Creating binary distributions for Python
========================================

To build the Python extension, I suggest using `build`, as it will
ensure that everything is done in a totally clean environment.  Run
this from the top-level directory

    python -m build

In all cases the resulting binary wheel (found in `dist`) is
self-contained and should not need any other components aside from the
system libraries.  To create wheels that are compatible with multiple
Linux distributions, see the instructions in
[README.manylinux.md](https://github.com/ReadAlongs/SoundSwallower/blob/master/README.manylinux.md).

Compiling on Windows in Visual Studio Code
==========================================

The method for building distributions noted above will also work on
Windows, from within a Conda environment, provided you have Visual
Studio or the Visual Studio Build Tools installed.  This is somewhat
magic.

If you don't have Conda, then what you will need to do is:

- Install Visual Studio build tools.  Unfortunately, a direct link
  does not seem to exist, but you can find them under [Microsoft's
  downloads page](https://visualstudio.microsoft.com/downloads/). The
  2019 version is probably the optimal one to use as it is compatible
  with all recent versions of Windows.
- Install the version of Python you wish to use.
- Launch the Visual Studio command-line prompt from the Start menu.
  Note that if your Python is 64-bit (recommended), you must be sure
  to launch the "x64 Native Command Line Prompt".
- Create and activate a virtual environment using your Python binary,
  which may or may not be in your AppData directory:

        %USERPROFILE%\AppData\Local\Programs\Python\Python310\python -m venv py310
        py310\scripts\activate

- now you can build wheels with pip, using the same method mentioned above.

Authors
-------

SoundSwallower is based on PocketSphinx, which is based on Sphinx-II,
which is based on Sphinx, which is based on Harpy, and so on, and so
on, back to somewhere around the Unix Epoch.  Thanks to Kevin Lenzo
for releasing CMU Sphinx under a BSD license and making this possible,
and Ravishankar Mosur who actually wrote most of the decoder.  Many
others also contributed along the way, take a look at [the AUTHORS
file in
PocketSphinx](https://github.com/cmusphinx/pocketsphinx/blob/master/AUTHORS)
for an idea.

This document and SoundSwallower are now being developed by David
Huggins-Daines.

SoundSwallower: an even smaller speech recognizer
=================================================

> "Time and change have a voice; eternity is silent. The human ear is
> always searching for one or the other."<br>
> Leena Krohn, *Datura, or a delusion we all see*

SoundSwallower is a refactored version of PocketSphinx intended
primarily for embedding in web applications.  The goal is not to
provide a fast implementation of large-vocabulary continuous speech
recognition, but rather to provide a *small* implementation of simple,
useful speech technologies.

With that in mind the current version is limited to finite-state
grammar recognition.  In addition, the eternally problematic and
badly-designed audio library as well as all other external
dependencies have been removed.

Compiling SoundSwallower
========================

Currently SoundSwallower can be built in several different ways. To
build the C shared library, run CMake in the standard way:

    mkdir build
    cd build
    cmake ..
    make
    make test
    make install

Note that this isn't terribly useful as there is no command-line
frontend.  You probably want to target JavaScript or Python.

Installing the Python module and CLI
------------------------------------

The SoundSwallower command-line is a Python module
(soundswallower.cli) and can be installed using `setup.py` or `pip`.
It is highly recommended to do this in a `virtualenv`.  You can simply
install it from PyPI:

    pip install soundswallower

Or compile from source:

    pip install .

For development, you can install it in-place, but please make sure to
remove any existing global installation:

    pip uninstall soundswallower
    python setup.py develop

The command-line supports JSGF grammars and word-level force
alignment for one or more input files, for example:

    soundswallower --align tests/data/goforward.txt tests/data/goforward.wav
    soundswallower --align-text "go forward ten meters" tests/data/goforward.wav
    soundswallower --grammar tests/data/goforward.gram tests/data/goforward.wav

Note that multiple input files are not particularly useful for
`--align` or `--align-text` as they will simply (try to) align the
same text to each file.  The output results (a list of time-aligned
words) can be written to a JSON file with `--output`.

Compiling to JavaScript/WebAssembly
-----------------------------------

To build the JavaScript library, use CMake with
[Emscripten](https://emscripten.org/):

    mkdir jsbuild
    cd jsbuild
    emcmake cmake ..
    emmake make

This will create `js/soundswallower.js` and `js/soundswallower.wasm`
in the `jsbuild` directory, which you can then include in your
projects.  It will also set up this directory to run a trivial demo
application, which you can launch with `server.py`, and access at
[http://localhost:8000](http://localhost:8000/).  It seems to work
fine with recent versions of Chrome, Firefox, and Edge.

For more details on JavaScript, see
[js/README.js](https://github.com/ReadAlongs/SoundSwallower/blob/master/js/README.md).

Creating binary distributions for Python
========================================

To build the Python extension, I suggest using pip, as it will install
the build dependencies:

    pip wheel .

In all cases the resulting binary is self-contained and should not
need any other components aside from the system libraries.  To create
wheels that are compatible with multiple Linux distributions, see the
instructions in
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

SoundSwallower: an even smaller speech recognizer
-------------------------------------------------

"Time and change have a voice; eternity is silent. The human ear is
always searching for one or the other."
- Leena Krohn, *Datura, or a delusion we all see*

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
build the C shared library and binaries, run CMake in the standard way:

	mkdir build
	cd build
	cmake ..
	make
	make test
	make install

To build the JavaScript library, use CMake with Emscripten:

	mkdir jsbuild
	cd jsbuild
	emcmake cmake ..
	make
	
NOTE! The JavaScript library has not been tested at this time.

Creating binary distributions for Python
========================================

To build the Python extension, I suggest using pip, as it will install
the build dependencies:

	pip wheel .

In all cases the resulting binary is self-contained and should not
need any other components aside from the system libraries.  To create
wheels that are compatible with multiple Linux distributions, see the
instructions in [README.manylinux.md](/README.manylinux.md).

Compiling on Windows in Visual Studio Code
==========================================

The method for building distributions noted above will also work on
Windows, from within a Conda environment, provided you have Visual
Studio or the Visual Studio Build Tools installed.  This is somewhat
magic.

If you don't have Conda, then what you will need to do is:

 - install Visual Studio build tools, unfortunately a direct link does
   not seem to exist, but you can find them under
   https://visualstudio.microsoft.com/downloads/ - The 2019 version is
   probably the optimal one to use as it is compatible with all recent
   versions of Windows.
 - install the version of Python you wish to use.
 - launch the Visual Studio command-line prompt from the Start menu.
   Note that if your Python is 64-bit (recommended), you must be sure
   to launch the "x64 Native Command Line Prompt".
 - create and activate a virtual environment using your Python binary,
   which may or may not be in your AppData directory:

        %USERPROFILE%\AppData\Local\Programs\Python\Python310\python -m venv py310
        py310\scripts\activate
 
 - now you can build wheels with pip, using the same method mentioned above.

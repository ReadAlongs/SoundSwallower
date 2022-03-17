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

To build the Python extension, use setup.py in the standard way:

	python setup.py bdist_wheel

In all cases the resulting binary is self-contained and should not
need any other components aside from the system libraries.

### Compiling on Windows in Visual Studio Code

Warning: your mileage may vary... this only partially worked for me, but I'm
adding it to the documentation in case it can help someone else. Complete and
fix this if you succeed. -EJ

 - install vscode with the C/C++ compiler option (https://code.visualstudio.com/download)
 - install CMake for Windows (https://cmake.org/download/)
 - install extensions CMake and CMake Tools (from within the vscode Extensions manager)
 - Select Kit (by clicking "Select Kit" on the status bar): Visual Studio Build Tools 2019 Release - amd64 (question: is this the best choice?)
 - Build: hit Ctrl-Shift-P to launch Command Palette (CP hereafter) and type "CMake: Build"
 - Run test suites: CP: CMake: Run Tests
 - Install: CP: CMake: Install (installs to C:/Program Files (x86)/soundswallower by default)
 - Build python wheel - TODO...

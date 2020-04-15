SoundSwallower: an even smaller speech recognizer
-------------------------------------------------

"mumblemumble sound swallower" - Leena Krohn, Datura

SoundSwallower is a refactored version of PocketSphinx intended
primarily for embedding in web applications.  The goal is not to
provide a fast implementation of large-vocabulary continuous speech
recognition, but rather to provide a *small* implementation of simple,
useful speech technologies.

With that in mind the current version is limited to keyword spotting,
finite-state grammar recognition and force alignment.  In addition,
all external dependencies have been removed and only the generic GMM
acoustic modeling code (with MLLR adaptation) is supported.  In the
future we hope to support DNN acoustic models as they require even
less code and can be efficiently quantized.

For the moment the API will remain compatible with (the subset) of
PocketSphinx implemented.

TODO

- Switch everything over to the new directory layout and build system

  - keep filenames the same but change all the paths
  - look for examples on how to do this in cmake projects
  - set the minimal necessary configuration macros (WORDS_BIGENDIAN, etc)
  - remove fixed point
  - maybe remove VAD as it is confusing (not sure about this)

- What binary to test with?

  - keep (equivalent of) pocketsphinx_batch

- SWIG wrappers?

- Bison/Flex?

- Fix the stupid configuration system somehow (but this will break the API...)

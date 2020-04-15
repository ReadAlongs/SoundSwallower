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

With that in mind the current version is limited to keyword spotting,
finite-state grammar recognition and force alignment.  In addition,
the eternally problematic and badly-designed audio library as well as
all other external dependencies have been removed.

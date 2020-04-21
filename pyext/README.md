soundswallower: an even smaller speech recognizer (Python extension)
--------------------------------------------------------------------

This extension uses scikit-build to compile.  We have pre-generated
the SWIG wrappers because sometimes it is a pain to get SWIG to
install, but if you wish to regenerate them, you can run:

swig -o soundswallower_wrap.c -outdir soundswallower -python ../swig/soundswallower.i

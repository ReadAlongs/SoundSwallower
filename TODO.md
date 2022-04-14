- Test JavaScript for alignment

- Default configuration

- Make a segmentation of something (goforward.wav will do)
  - In a notebook!!!
  - Implement code needed to create a state sequence from it
  - Either in C or in Cython, look and see...

- Look at use case in readalongs.align
  - Refactor it
  - What does it use in SoundSwallower
  - How could that API be better
  - Implement that better API
  
- No point in just recreating the SWIG API, because it's not like anybody used it anyway!

- Things I want
  - default_config is good
  - set_boolean and friends are Bad, we know the types, duh
  - helper methods to get frame size in samples
  - automatically set FFT size
  - get results in time rather than frames if requested

- Steps to implement:
  - do unit tests for
	- config: implement set_boolean and friends
	- config: figure out Python API for accepting generic objects
	- decoder: default\_config
	- decoder: start\_utt, process\_raw, end\_utt
	- decoder: seg
	- segment: word, start, end

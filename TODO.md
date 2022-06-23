Roadmap:

- 0.2.x: Size and speed optimizations for JS in particular, bug fixes
  - Add back LDA (DONE)
  - Asynchronous JS loading (DONE)
	- Before: Test JSGF 981ms, JS 88k, WASM 214k, zip 119k
	- After: Test JSGF 478ms, JS 43k, WASM 206k, zip 104k
  - Better browser support and packaging
	- Just more examples and documentation for now
  - Add phoneset description and IPA input
    - phoneset.txt in model directory (DONE)
	- 
  - Rename s3file_t to something more neutral
  - Fix C library tests (as in PocketSphinx)
  - Add -cionly option
- 0.4.0: User experience
  - Simple VAD with solid API
	- Principles
	  - Decoder must never discard data: audio input -> alignments
	  - Recognizer can discard data but not timestamps
	  - Recognizer log indicates mapping of timeline to decoding
	- Draft use case:
	  - Recognizer states:
		- idle (doing nothing, no data)
		- listening (processing data, undecided silence/speech)
		- silence (processing data, waiting for speech)
		- speech (procesing data, passing data to decoder)
	  - transitions:
		- idle -> *: user initiative only
		- listening -> silence: system or user
		- listening -> speech: user only
		- silence -> speech: system or user
		- speech -> silence: system or user
		- * -> listening: user
		- * -> idle: user
	  - all transitions logged with global timestamp
	  - transitions fire events that can be listened to
	  - do this in JavaScript only at first, figure out how to make it
        in C/Python later
  - Generic enrollment API (just CMN, perhaps VTLN for now)

- 0.5.0: API stabilization
  - Bring C API in line with Python and JavaScript
	- `cmd_ln_t` -> `config_t`
	- `ps_decoder_t` -> `decoder_t`
	- return times rather than frame indices
  - Change ownership semantics to fit use cases
  - Better solution for float vs. int in front-end

- 1.0.0: Phone-level alignment

- 2.0.0: Improved modeling
  - DNN acoustic models?

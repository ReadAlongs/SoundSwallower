Miscellaneous things to try:

- Running G2P under Pyodide
- Calling SoundSwallower-JS from Pyodide
- Running ReadAlong-Studio under Pyodide

Code to rethink or eliminate:

- Logging to files (irrelevant for JavaScript)
- WTF is `bio_read_wavfile` for?
- `ckd_set_jump` and `ckd_alloc` in general
- File I/O in general for JavaScript
  - Replace with JSON or direct binary load to memory
  - Many things very Unix/cmdln-centric like pio.h
- Reading configuration from file (use JSON instead)
  - Need new solution for feat.params
- Configuration in general...

Roadmap:

- 0.2: new JavaScript API
  - Fix automatic -nfft and other ps_reinit problems DONE
  - Implement automatic -nfft at fe init time DONE
  - Bring back arg_dump for INFO level logging DONE
  - Floating-point input to front-end
    Default to pre-loading in Node and fix README.js DONE
  - Write API tests DONE
  - Write API documentation DONE
	- Fix links in sphinx-js DONE
	- Fix names in sphinx-js DONE
  - Dealing with models DONE
  - Dealing with grammars as assets DONE
  - Loading words from extra dictionary DONE

- 0.3: fix various Python APIs and remove code
  - Translate back from JavaScript into Cython/Python ;)
  - FSG construction API is quite bad (no error handling for invalid
	inputs, forgetting start/end state)
  - FSG and search names need to GTFO
	- No, just need to synchronize them properly
	- Useful to be able to identify an FSG object
  - Remove all stdio from JavaScript version at least
  - Remove superfluous I/O code in general

- 0.4: phone-level alignment

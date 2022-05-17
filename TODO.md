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
  - Bring back arg_dump for INFO level logging
  - Floating-point input to front-end
  - AudioWorklet implementation
  - Write API tests (in progress)
  - Write API documentation
	- Use JSDoc and Sphinx-JS https://pypi.org/project/sphinx-js/
	- Should be able to use this.foo = function() not Module.foo?
	- sinon just assign to Module afterwards
  - Dealing with models
	- `load_module(module_name,module_url)`

- 0.3: fix various Python APIs and remove code
  - Translate back from JavaScript into Cython/Python ;)
  - FSG construction API is quite bad (no error handling for invalid
	inputs, forgetting start/end state)
  - FSG and search names need to GTFO
	- No, just need to synchronize them properly
	- Useful to be able to identify an FSG object

- 0.4: phone-level alignment

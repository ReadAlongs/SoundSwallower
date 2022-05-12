Miscellaneous things to try:

- Running G2P under Pyodide
- Calling SoundSwallower-JS from Pyodide
- Running ReadAlong-Studio under Pyodide

Roadmap:

- 0.2: new JavaScript API
  - Floating-point input to front-end
  - Write API tests (in progress)
  - Write API documentation
	- Manually in Sphinx-RST? JSDoc?
  - Dealing with models
	- Node.js - require, works fine, but cumbersome
	- Will *not* work for webpack, need to use URL(), unclear exactly how
	- Models should be loadable:
	  - As bundled (and with a default)

- 0.3: fix various Python APIs and remove code
  - Translate back from JavaScript into Cython/Python ;)
  - FSG construction API is quite bad (no error handling for invalid
	inputs, forgetting start/end state)
  - FSG and search names need to GTFO
	- No, just need to synchronize them properly
	- Useful to be able to identify an FSG object

- 0.4: phone-level alignment

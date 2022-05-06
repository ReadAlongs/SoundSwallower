Miscellaneous things to try:

- Running G2P under Pyodide
- Calling SoundSwallower-JS from Pyodide
- Running ReadAlong-Studio under Pyodide

Roadmap:

- 0.2: new JavaScript API
  - Implement hypseg DONE
  - Implement FSG creation/setting DONE
  - Implement adding words DONE
  - Compare C and C++/embind sizes DONE
  - Wrap C API with JS objects, handle memory management DONE
  - Implement configuration from JSON/Object DONE
	- Parameter normalization as in Python DONE
	- Iteration over configuration DONE
  - Fix logging
	- Add -loglevel option DONE
  - NPM setup
  - Write API tests DONE (in progress)
  - Write API documentation
	- Manually in Sphinx-RST? JSDoc?
  - Async API
	- we may still want to run it in a worker but this will mean less work later
	- ps_init is a factory function so let's make it one in the API
	- decompose ps_reinit into chain of promises DONE
	  - some are long-running and need to be decomposed further
		- acmod in particular:
		  - fe -> feat -> mdef -> {tmat, mean, var} -> mgau
	  - note we can use async iterators to decompose some things
  - Modernize PSJS example
	- Have to still use web workers because of lazy loading (synchronous XHRs)
	- Keep recognizer.js for the time being, then
  - Very simple alignment example
	- Load wave file into browser using readDataAsURL
	- Enter text into a text field
	- Click align - show times, words become clickable
- 0.3: fix various Python APIs and remove code
  - Translate back from JavaScript into Cython/Python ;)
  - FSG construction API is quite bad (no error handling for invalid
	inputs, forgetting start/end state)
  - FSG and search names need to GTFO
	- No, just need to synchronize them properly
	- Useful to be able to identify an FSG object

- 0.4: phone-level alignment

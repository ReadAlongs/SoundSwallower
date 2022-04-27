Roadmap:

- 0.1.5:
  - basic test of CLI DONE
  - Travis-CI continuous integration DONE
  - Documentation DONE
  - More interesting test data
- 0.2: new JavaScript API
  - Implement hypseg DONE
  - Implement FSG creation/setting DONE
  - Implement adding words DONE
  - Complete error handling
  - Compare C and C++/embind sizes DONE
	- C is 70k smaller
	- implement same API with thin C++ bindings (no STL, no argv)
	- embind may be more efficient due to no exporting
  - Implement configuration from JSON/Object
- 0.3: remove redundant Python APIs
- 0.4: phone-level alignment

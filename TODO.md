Roadmap:

- 0.2: new JavaScript API
  - Implement hypseg DONE
  - Implement FSG creation/setting DONE
  - Implement adding words DONE
  - Compare C and C++/embind sizes DONE
	- C is 70k smaller
	- implement same API with thin C++ bindings (no STL, no argv)
	- embind may be more efficient due to no exporting
  - Wrap C API with JS objects, handle memory management
  - Implement configuration from JSON/Object
  - Write API documentation
- 0.3: fix various APIs and remove code
  - FSG construction API is quite bad (no error handling for invalid
	inputs, forgetting start/end state)
  - FSG and search names need to GTFO
- 0.4: phone-level alignment

Roadmap:

- 0.2: new JavaScript API
  - Implement hypseg DONE
  - Implement FSG creation/setting DONE
  - Implement adding words DONE
  - Compare C and C++/embind sizes DONE
  - Wrap C API with JS objects, handle memory management DONE
  - Implement configuration from JSON/Object DONE
	- Parameter normalization as in Python
	- Return configuration as JSON
	- To Proxy or not to Proxy?
  - Very simple alignment example
	- Load wave file into browser using readDataAsURL
	- Enter text into a text field
	- Click align - show times, words become clickable
  - NPM setup
  - Write API tests
	- Using what framework? MochaJS?
  - Write API documentation
	- Manually in Sphinx-RST
- 0.3: fix various Python APIs and remove code
  - FSG construction API is quite bad (no error handling for invalid
	inputs, forgetting start/end state)
  - FSG and search names need to GTFO
	- No, just need to synchronize them properly
	- Useful to be able to identify an FSG object
- 0.4: phone-level alignment

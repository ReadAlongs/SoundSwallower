Roadmap:

- 0.2.x: Size and speed optimizations for JS in particular, bug fixes
  - Add back LDA (DONE)
  - Add phoneset description and IPA input
  - Add -cionly option
  - Properly modularize JS code
  - Fully async init and direct load binary files
	- Go through all initialization code and pull out filesystem access
  - Preserve existing Python and JS API, remove everything else!

- 0.3.x: New model format

- 0.4.x: Phone-level alignment

- 0.5.x: Improved modeling
  - DNN acoustic models?
  - VAD?

Notes:

- must expose phoneset and give description (closest IPA equivalents - support IPA or X-SAMPA input)

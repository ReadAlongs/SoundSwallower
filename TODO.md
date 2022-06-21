Roadmap:

- 0.3.0: Size and speed optimizations for JS in particular, bug fixes
  - Add back LDA (DONE)
  - Asynchronous JS loading (DONE)
	- Before: Test JSGF 981ms, JS 88k, WASM 214k, zip 119k
	- After: Test JSGF 478ms, JS 43k, WASM 206k, zip 104k
  - Better browser support and packaging?
  - Add phoneset description and IPA input
  - Add -cionly option

- 0.4.x: New model format

- 1.0.x: Phone-level alignment

- 2.0.x: Improved modeling
  - DNN acoustic models?
  - VAD?

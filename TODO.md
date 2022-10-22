Roadmap:

- 0.4.0: Merge PocketSphinx 5.0.0 and update API
  - Config updates, but smaller (no s3kr3t command-line, etc)
  - CMN interface
  - Overflow fixes
  - VAD/Endpointer
  - Two-pass alignment (but easier)
  - JSGF correctness
  - JSON decoding/alignment output
  - Eliminate "pocketsphinx" API
    - `ps_decoder_t` => `decoder_t`
    - likewise for config, lattice, endpointer, vad
  - Rusty move semantics
    - passing config, fsg, etc to decoder consumes it

- 1.0.0: Finalize API
  - Support IPA dictionaries
  - ES6 module
  - Clearly define use cases
    - Live: feed data asynchronously, check results synchronously or emit events
      - *recording* is real-time in a separate thread
      - *decoding* is not real-time, can be decomposed to microtasks
    - Single: pass data with promise of result, emit progress events
  - Easier support for observable/event type uses
    - async/await/promise is actually not a great fit
  - Support web audio formats directly
  - Better solution for float vs. int in front-end

- 2.0.0: Improved modeling
  - DNN acoustic models?
  - WFST search?

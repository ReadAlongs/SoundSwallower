Roadmap:

- 0.4.0: Merge PocketSphinx 5.0.0
  - VAD/Endpointer
  - Two-pass alignment (but easier)
  - Config updates, but smaller (no s3kr3t command-line, etc),
    actually we were mostly there already
  - JSGF correctness
  - JSON decoding/alignment output
  - Support IPA dictionaries
  - ES6 module

- 1.0.0: Update API
  - Clearly define use cases
    - Live: feed data asynchronously, check results synchronously or emit events
      - *recording* is real-time in a separate thread
      - *decoding* is not real-time, can be decomposed to microtasks
    - Single: pass data with promise of result, emit progress events
  - Remove remaining PocketSphinx API junk if any
  - Easier support for observable/event type uses
    - async/await/promise is actually not a great fit
  - Support web audio formats directly
  - Change ownership semantics to fit use cases
  - Better solution for float vs. int in front-end

- 2.0.0: Improved modeling
  - DNN acoustic models?
  - WFST search?

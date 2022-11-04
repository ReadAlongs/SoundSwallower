Roadmap:

- 1.0: Finalize API
  - ES6 module (possibly separate for Node vs. Web)
  - Optimize JSGF compiler
  - Support IPA dictionaries
  - Improve Endpointer/VAD
  - Clearly define use cases and restructure API for them
    - Live: feed data asynchronously, check results synchronously or emit events
      - *recording* is real-time in a separate thread
        - VAD/Endpointing should be done in the recording thread
        - Feature extraction could be done there but this is not necessary
      - *decoding* is not real-time, can be decomposed to microtasks
    - Single/Batch: pass data with promise of result, emit progress events
      - Good fit for async
  - Easier support for observable/event type uses
    - async/await/promise is actually not a great fit
  - Support web audio formats directly
  - Better solution for float vs. int in front-end

- 2.0: Improved modeling
  - WFST search
  - DNN acoustic models

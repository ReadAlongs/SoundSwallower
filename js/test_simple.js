#!/usr/bin/env node

(async () => {
    const ssjs = await require('./js/soundswallower.js')();
    // this API sucks
    let config = new ssjs.Config();
    config.push_back(["-hmm", "../model/en-us"]);
    config.push_back(["-dict", "../model/en-us.dict"]);
    config.push_back(["-fsg", "../tests/data/goforward.fsg"]);
    let rec = new ssjs.Recognizer(config);
    const fs = require('fs/promises');
    let buf = await fs.readFile("../tests/data/goforward.raw");
    // this API sucks even more
    let pcm = new ssjs.AudioBuffer()
    for (let i = 0; i < buf.length; i += 2) {
	pcm.push_back(buf.readInt16LE(i));
    }
    rec.start();
    let output = rec.process(pcm);
    rec.stop();
    console.log(rec.getHyp());
})();

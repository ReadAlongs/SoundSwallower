#!/usr/bin/env node

(async () => {
    const ssjs = await require('./js/soundswallower.js')();
    // this API sucks
    let config = new ssjs.Config();
    config.push_back(["-hmm", "en-us"]);
    config.push_back(["-dict", "en-us.dict"]);
    config.push_back(["-fsg", "goforward.fsg"]);
    ssjs.FS_createPath("/", "en-us", true, true);
    ssjs.FS_createLazyFile("/", "en-us.dict", "../model/en-us.dict");
    ssjs.FS_createLazyFile("/", "en-us.dict", "../model/en-us.dict");
    ssjs.FS_createLazyFile("/", "goforward.fsg", "../goforward.fsg");
    ssjs.FS_createLazyFile("/en-us", "feat.params", "../model/en-us/feat.params");
    ssjs.FS_createLazyFile("/en-us", "mdef", "../model/en-us/mdef");
    ssjs.FS_createLazyFile("/en-us", "means", "../model/en-us/means");
    ssjs.FS_createLazyFile("/en-us", "noisedict", "../model/en-us/noisedict");
    ssjs.FS_createLazyFile("/en-us", "sendump", "../model/en-us/sendump");
    ssjs.FS_createLazyFile("/en-us", "transition_matrices", "../model/en-us/transition_matrices");
    ssjs.FS_createLazyFile("/en-us", "variances", "../model/en-us/variances");
    let rec = new ssjs.Recognizer(config);
    const fs = require('fs/promises');
    let buf = await fs.readFile("goforward.raw");
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

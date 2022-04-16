#!/usr/bin/env node

// Monkey-patch this so that preloading files will work (see
// https://github.com/emscripten-core/emscripten/issues/16742)
Browser = {
    handledByPreloadPlugin() {
	return false;
    }
};

(async () => {
    const Module = {
	preRun: () => {
	    // Cannot use lazy loading as it corrupts binary files
	    // (see
	    // https://github.com/emscripten-core/emscripten/issues/16740)
	    Module.FS_createPath("/", "en-us", true, true);
	    Module.FS_createPreloadedFile("/", "en-us.dict", "../model/en-us.dict", true, true);
	    Module.FS_createPreloadedFile("/", "goforward.fsg", "../tests/data/goforward.fsg", true, true);
	    Module.FS_createPreloadedFile("/en-us", "transition_matrices", "../model/en-us/transition_matrices", true, true);
	    Module.FS_createPreloadedFile("/en-us", "feat.params", "../model/en-us/feat.params", true, true);
	    Module.FS_createPreloadedFile("/en-us", "mdef", "../model/en-us/mdef", true, true);
	    Module.FS_createPreloadedFile("/en-us", "means", "../model/en-us/means", true, true);
	    Module.FS_createPreloadedFile("/en-us", "noisedict", "../model/en-us/noisedict", true, true);
	    Module.FS_createPreloadedFile("/en-us", "sendump", "../model/en-us/sendump", true, true);
	    Module.FS_createPreloadedFile("/en-us", "variances", "../model/en-us/variances", true, true);
	}
    };
    const ssjs = await require('./js/soundswallower.js')(Module);
    // this API sucks
    let config = new ssjs.Config();
    config.push_back(["-hmm", "en-us"]);
    config.push_back(["-dict", "en-us.dict"]);
    config.push_back(["-fsg", "goforward.fsg"]);
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

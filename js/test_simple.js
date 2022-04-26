#!/usr/bin/env node

// Monkey-patch this so that preloading files will work (see
// https://github.com/emscripten-core/emscripten/issues/16742)
Browser = {
    handledByPreloadPlugin() {
	return false;
    }
};

// Cannot use lazy loading as it corrupts binary files (see
// https://github.com/emscripten-core/emscripten/issues/16740)
function preload_models() {
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
Module = {
    preRun: preload_models
};

var ssjs; // Loaded below
const fs = require('fs/promises');


// Wrap in async function so we can wait for WASM load
(async () => {
    ssjs = await require('./js/soundswallower.js')(Module);
    let config = ssjs.cmd_ln_init();
    ssjs.cmd_ln_set(config, "-hmm", "en-us");
    ssjs.cmd_ln_set(config, "-dict", "en-us.dict");
    ssjs.cmd_ln_set(config, "-fsg", "goforward.fsg");
    let decoder = ssjs.ps_init(config);
    ssjs.ps_start_utt(decoder);
    let pcm = await fs.readFile("../tests/data/goforward.raw");
    ssjs.ps_process_raw(decoder, pcm, pcm.length / 2, 0, 1);
    ssjs.ps_end_utt(decoder);
    console.log(ssjs.ps_get_hyp(decoder, 0));
})();

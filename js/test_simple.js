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
    ssjs.cmd_ln_free(config); // has been retained by decoder
    let pcm = await fs.readFile("../tests/data/goforward.raw");
    ssjs.ps_start_utt(decoder);
    ssjs.ps_process_raw(decoder, pcm, pcm.length / 2, false, true);
    ssjs.ps_end_utt(decoder);
    console.log(ssjs.ps_get_hyp(decoder, 0));
    console.log(ssjs.ps_get_hypseg(decoder)); 
    ssjs.ps_add_word(decoder, "_go", "G OW", 0);
    ssjs.ps_add_word(decoder, "_forward", "F AO R W ER D", 0);
    ssjs.ps_add_word(decoder, "_ten", "T EH N", 0);
    ssjs.ps_add_word(decoder, "_meters", "M IY T ER Z", 1);
    fsg = ssjs.ps_create_fsg(decoder, "goforward", 0, 4, [
	{from: 0, to: 1, prob: 1.0, word: "_go"},
	{from: 1, to: 2, prob: 1.0, word: "_forward"},
	{from: 2, to: 3, prob: 1.0, word: "_ten"},
	{from: 3, to: 4, prob: 1.0, word: "_meters"}
    ]);
    ssjs.ps_set_fsg(decoder, fsg);
    ssjs.fsg_model_free(fsg); // has been retained by decoder
    ssjs.ps_start_utt(decoder);
    ssjs.ps_process_raw(decoder, pcm, pcm.length / 2, false, true);
    ssjs.ps_end_utt(decoder);
    console.log(ssjs.ps_get_hyp(decoder, 0));
    console.log(ssjs.ps_get_hypseg(decoder));
    ssjs.ps_free(decoder);
})();

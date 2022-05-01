#!/usr/bin/env node

// Note that we could use NODERAWFS but the module would have to be
// recompiled for the browser.  So, we monkey-patch this so that
// preloading files will work (see
// https://github.com/emscripten-core/emscripten/issues/16742).  Note
// also that we cannot use lazy loading as it corrupts binary files
// (see https://github.com/emscripten-core/emscripten/issues/16740)
Browser = {
    handledByPreloadPlugin() {
	return false;
    }
};

// Emscripten will use this as the Module object inside ssjs.
// Unfortunately preRun() is not called as a method, so we have to bind
// the object to a name here in order to access the Emscripten runtime.
var modinit = {
    preRun() {
	modinit.FS_createPath("/", "en-us", true, true);
	modinit.FS_createPreloadedFile("/", "en-us.dict", "../model/en-us.dict",
				      true, true);
	modinit.FS_createPreloadedFile("/", "goforward.fsg",
				      "../tests/data/goforward.fsg", true, true);
	modinit.FS_createPreloadedFile("/en-us", "transition_matrices",
				      "../model/en-us/transition_matrices", true, true);
	modinit.FS_createPreloadedFile("/en-us", "feat.params",
				      "../model/en-us/feat.params", true, true);
	modinit.FS_createPreloadedFile("/en-us", "mdef",
				      "../model/en-us/mdef", true, true);
	modinit.FS_createPreloadedFile("/en-us", "means",
				      "../model/en-us/means", true, true);
	modinit.FS_createPreloadedFile("/en-us", "noisedict",
				      "../model/en-us/noisedict", true, true);
	modinit.FS_createPreloadedFile("/en-us", "sendump",
				      "../model/en-us/sendump", true, true);
	modinit.FS_createPreloadedFile("/en-us", "variances",
				      "../model/en-us/variances", true, true);
    }
};

// Wrap in async function so we can wait for WASM load
(async () => {
    const fs = await require('fs/promises');
    // Note that ssjs and modinit are (probably) the same object after this
    var ssjs = await require('./js/soundswallower.js')(modinit);
    let decoder = new ssjs.Decoder({
	hmm: "en-us",
	dict: "en-us.dict",
	fsg: "goforward.fsg"
    });
    let pcm = await fs.readFile("../tests/data/goforward.raw");
    decoder.start();
    decoder.process_raw(pcm, false, true);
    decoder.stop();
    console.log(decoder.get_hyp());
    console.log(decoder.get_hypseg());
    decoder.add_word("_go", "G OW", false);
    decoder.add_word("_forward", "F AO R W ER D", false);
    decoder.add_word("_ten", "T EH N", false);
    decoder.add_word("_meters", "M IY T ER Z", true);
    fsg = decoder.create_fsg("goforward", 0, 4, [
	{from: 0, to: 1, prob: 1.0, word: "_go"},
	{from: 1, to: 2, prob: 1.0, word: "_forward"},
	{from: 2, to: 3, prob: 1.0, word: "_ten"},
	{from: 3, to: 4, prob: 1.0, word: "_meters"}
    ]);
    decoder.set_fsg(fsg);
    decoder.config.set("-backtrace", true);
    fsg.delete() // has been retained by decoder
    decoder.start();
    decoder.process_raw(pcm, false, true);
    decoder.stop();
    console.log(decoder.get_hyp());
    console.log(decoder.get_hypseg());
    decoder.delete();
})();

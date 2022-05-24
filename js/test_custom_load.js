(async () => {
    const assert = require('assert');
    const fs = require('fs/promises');
    const ssjs = {
	// Do not load the default model
	defaultModel: null,
	preRun() {
	    ssjs.FS_createPreloadedFile("/", "goforward.fsg",
					"../tests/data/goforward.fsg", true, true);
	    // Test loading a different dictionary
	    ssjs.load_model("en-us", "model/en-us", "../tests/data/turtle.dic");
	}
    };
    before(async () => {
	await require('./soundswallower.js')(ssjs);
    });
    describe("Test initialization", () => {
	it("Should load the WASM module", () => {
	    assert.ok(ssjs);
	});
    });
    describe("Test decoding", () => {
	it('Should recognize "go forward ten meters"', async () => {
	    let decoder = new ssjs.Decoder({
		// Need to specify this since we made defaultModel null
		hmm: "en-us",
		fsg: "goforward.fsg",
		backtrace: true
	    });
	    await decoder.initialize();
	    let pcm = await fs.readFile("../tests/data/goforward-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("go forward ten meters", decoder.get_hyp());
	    let hypseg = decoder.get_hypseg();
	    let hypseg_words = []
	    for (const seg of hypseg) {
		assert.ok(seg.end >= seg.start);
		hypseg_words.push(seg.word);
	    }
	    assert.deepStrictEqual(hypseg_words,
				   ["<sil>", "go", "forward",
				    "(NULL)", "ten", "meters"]);
	    decoder.delete();
	});
    });
})();

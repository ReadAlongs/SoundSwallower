(async () => {
    const assert = require('assert');
    const fs = require('fs/promises');
    const ssjs = {};
    before(async () => {
	await require('./soundswallower.js')(ssjs);
    });
    describe("Test initialization", () => {
	it("Should load the WASM module", () => {
	    assert.ok(ssjs);
	});
    });
    describe("Test acoustic model loading", () => {
	it('Should load acoustic model', async () => {
	    let decoder = new ssjs.Decoder();
	    await decoder.init_config();
	    await decoder.init_fe();
	    await decoder.init_feat();
	    await decoder.init_acmod();
	    await decoder.load_acmod_files();
            assert.ok(decoder);
	});
    });
    describe("Test decoder initialization", () => {
	it("Should initialize the decoder", async () => {
	    let decoder = new ssjs.Decoder({
		fsg: "testdata/goforward.fsg",
		samprate: 16000,
	    });
            assert.ok(decoder);
        });
    });
    describe("Test decoding", () => {
	it('Should recognize "go forward ten meters"', async () => {
	    let decoder = new ssjs.Decoder({
		fsg: "testdata/goforward.fsg",
		samprate: 16000,
	    });
	    await decoder.initialize();
	    let pcm = await fs.readFile("testdata/goforward-float32.raw");
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
	});
	it('Should accept Float32Array as well as UInt8Array', async () => {
	    let decoder = new ssjs.Decoder({
		fsg: "testdata/goforward.fsg",
		samprate: 16000
	    });
	    await decoder.initialize();
	    let pcm = await fs.readFile("testdata/goforward-float32.raw");
	    let pcm32 = new Float32Array(pcm.buffer);
	    await decoder.start();
	    await decoder.process(pcm32, false, true);
	    await decoder.stop();
	    assert.equal("go forward ten meters", decoder.get_hyp());
	});
    });
    describe("Test dictionary and FSG", () => {
	it('Should recognize "_go _forward _ten _meters"', async () => {
	    let decoder = new ssjs.Decoder({samprate: 16000});
	    await decoder.initialize();
	    await decoder.add_word("_go", "G OW", false);
	    await decoder.add_word("_forward", "F AO R W ER D", false);
	    await decoder.add_word("_ten", "T EH N", false);
	    await decoder.add_word("_meters", "M IY T ER Z", true);
	    let fsg = decoder.create_fsg("goforward", 0, 4, [
		{from: 0, to: 1, prob: 1.0, word: "_go"},
		{from: 1, to: 2, prob: 1.0, word: "_forward"},
		{from: 2, to: 3, prob: 1.0, word: "_ten"},
		{from: 3, to: 4, prob: 1.0, word: "_meters"}
	    ]);
	    await decoder.set_fsg(fsg);
	    let pcm = await fs.readFile("testdata/goforward-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("_go _forward _ten _meters", decoder.get_hyp());
	});
    });
    describe("Test loading model for other language", () => {
	it('Should recognize "avance de dix mètres"', async () => {
	    let decoder = new ssjs.Decoder({hmm: ssjs.get_model_path("fr-fr"),
					    samprate: 16000});
	    await decoder.initialize();
	    let fsg = decoder.create_fsg("goforward", 0, 4, [
		{from: 0, to: 1, prob: 0.5, word: "avance"},
		{from: 0, to: 1, prob: 0.5, word: "recule"},
		{from: 1, to: 2, prob: 0.1, word: "d'"},
		{from: 1, to: 2, prob: 0.9, word: "de"},
		{from: 2, to: 3, prob: 0.1, word: "un"},
		{from: 2, to: 3, prob: 0.1, word: "deux"},
		{from: 2, to: 3, prob: 0.1, word: "trois"},
		{from: 2, to: 3, prob: 0.1, word: "quatre"},
		{from: 2, to: 3, prob: 0.1, word: "cinq"},
		{from: 2, to: 3, prob: 0.1, word: "six"},
		{from: 2, to: 3, prob: 0.1, word: "sept"},
		{from: 2, to: 3, prob: 0.1, word: "huit"},
		{from: 2, to: 3, prob: 0.1, word: "neuf"},
		{from: 2, to: 3, prob: 0.1, word: "dix"},
		{from: 3, to: 4, prob: 0.1, word: "mètre"},
		{from: 3, to: 4, prob: 0.9, word: "mètres"}
	    ]);
	    await decoder.set_fsg(fsg);
	    let pcm = await fs.readFile("testdata/goforward_fr-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("avance de dix mètres", decoder.get_hyp());
	});
    });
    describe("Test JSGF", () => {
	it('Should recognize "yo gimme four large all dressed pizzas"', async () => {
	    let decoder = new ssjs.Decoder({
		jsgf: "testdata/pizza.gram",
		samprate: 16000
	    });
	    await decoder.initialize();
	    let pcm = await fs.readFile("testdata/pizza-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("yo gimme four large all dressed pizzas", decoder.get_hyp());
	});
    });
    describe("Test JSGF string", () => {
	it('Should recognize "yo gimme four large all dressed pizzas"', async () => {
	    let decoder = new ssjs.Decoder({samprate: 16000});
	    await decoder.initialize();
	    let fsg = decoder.parse_jsgf(`#JSGF V1.0;
grammar pizza;
public <order> = [<greeting>] [<want>] [<quantity>] [<size>] [<style>]
       [(pizza | pizzas)] [<toppings>];
<greeting> = hi | hello | yo | howdy;
<want> = i want | gimme | give me | i'd like to order | order | i wanna;
<quantity> = a | one | two | three | four | five;
<size> = small | medium | large | extra large | x large | x l;
<style> = hawaiian | veggie | vegetarian | margarita | meat lover's | all dressed;
<toppings> = [with] <topping> ([and] <topping>)*;
<topping> = pepperoni | ham | olives | mushrooms | tomatoes | (green | hot) peppers | pineapple;
`);
	    await decoder.set_fsg(fsg);
	    let pcm = await fs.readFile("testdata/pizza-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("yo gimme four large all dressed pizzas", decoder.get_hyp());
	});
    });
    describe("Test reinitialize_audio", () => {
	it('Should recognize "go forward ten meters"', async () => {
	    let decoder = new ssjs.Decoder({
		fsg: "testdata/goforward.fsg",
		samprate: 11025
	    });
	    await decoder.initialize();
	    let pcm = await fs.readFile("testdata/goforward-float32.raw");
	    decoder.config.set("samprate", 16000);
	    await decoder.reinitialize_audio();
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
	});
    });
    describe("Test dictionary lookup", () => {
	it('Should return "W AH N"', async () => {
	    let decoder = new ssjs.Decoder();
	    await decoder.initialize();
	    const phones = decoder.lookup_word("one");
	    assert.equal("W AH N", phones);
	});
    });
})();

(async () => {
    const assert = require('assert');
    const fs = require('fs/promises');
    const ssjs = {
	// Pre-load the grammar we use below
	preRun() {
	    ssjs.FS_createPreloadedFile("/", "goforward.fsg",
					"../tests/data/goforward.fsg", true, true);
	    ssjs.FS_createPreloadedFile("/", "pizza.gram",
					"../tests/data/pizza.gram", true, true);
	    ssjs.load_model("fr-fr", "model/fr-fr");
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
    describe("Test get/set API", () => {
	it("Should return en-us.dict after setting", () => {
	    let conf = new ssjs.Config();
	    conf.set("-dict", "en-us.dict");
	    assert.equal("en-us.dict", conf.get("-dict"));
	});
	it("Should contain -dict after setting", () => {
	    let conf = new ssjs.Config();
	    conf.set("-dict", "en-us.dict");
	    assert.ok(conf.has('-dict'));
	});
	it("Should contain default -hmm", () => {
	    let conf = new ssjs.Config();
	    assert.ok(conf.has('-hmm'));
	    assert.equal(conf.get('-hmm'), ssjs.defaultModel);
	});
	it("Should contain default -hmm when initialized", () => {
	    let conf = new ssjs.Config({dict: "en-us.dict"});
	    assert.ok(conf.has('-hmm'));
	    assert.equal(conf.get('-hmm'), ssjs.defaultModel);
	});
	it("Unset string key should have null value", () => {
	    let conf = new ssjs.Config();
	    assert.equal(null, conf.get('-dict'));
	});
	it("Should not contain -foobiebletch", () => {
	    let conf = new ssjs.Config();
	    conf.set("-hmm", "en-us");
	    assert.ok(!conf.has('-foobiebletch'));
	});
	it("Should normalize keys without dash", () => {
	    let conf = new ssjs.Config();
	    conf.set("hmm", "en-us");
	    assert.equal("en-us", conf.get("-hmm"));
	    assert.equal("en-us", conf.get("hmm"));
	});
    });
    describe("Test iteration on Config", () => {
	it("Should iterate over known keys, which are all defined", () => {
	    let conf = new ssjs.Config();
	    let count = 0;
	    for (const key of conf) {
		let val = conf.get(key);
		// It could be 0 or null, but it should be defined
		assert.notStrictEqual(val, undefined);
		count++;
	    }
	    assert.ok(count > 0);
	});
    });
    describe("Test decoding", () => {
	it('Should recognize "go forward ten meters"', async () => {
	    let decoder = new ssjs.Decoder({
		fsg: "goforward.fsg"
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
	it('Should accept Float32Array as well as UInt8Array', async () => {
	    let decoder = new ssjs.Decoder({
		fsg: "goforward.fsg"
	    });
	    await decoder.initialize();
	    let pcm = await fs.readFile("../tests/data/goforward-float32.raw");
	    let pcm32 = new Float32Array(pcm.buffer);
	    await decoder.start();
	    await decoder.process(pcm32, false, true);
	    await decoder.stop();
	    assert.equal("go forward ten meters", decoder.get_hyp());
	});
    });
    describe("Test dictionary and FSG", () => {
	it('Should recognize "_go _forward _ten _meters"', async () => {
	    let decoder = new ssjs.Decoder();
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
	    fsg.delete(); // has been retained by decoder
	    let pcm = await fs.readFile("../tests/data/goforward-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("_go _forward _ten _meters", decoder.get_hyp());
	    decoder.delete();
	});
    });
    describe("Test loading model for other language", () => {
	it('Should recognize "avance de dix mètres"', async () => {
	    let decoder = new ssjs.Decoder({hmm: "fr-fr"});
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
	    fsg.delete(); // has been retained by decoder
	    let pcm = await fs.readFile("../tests/data/goforward_fr-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("avance de dix mètres", decoder.get_hyp());
	    decoder.delete();
	});
    });
    describe("Test JSGF", () => {
	it('Should recognize "yo gimme four large all dressed pizzas"', async () => {
	    let decoder = new ssjs.Decoder({
		jsgf: "pizza.gram",
	    });
	    await decoder.initialize();
	    let pcm = await fs.readFile("../tests/data/pizza-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("yo gimme four large all dressed pizzas", decoder.get_hyp());
	    decoder.delete();
	});
    });
    describe("Test JSGF string", () => {
	it('Should recognize "yo gimme four large all dressed pizzas"', async () => {
	    let decoder = new ssjs.Decoder();
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
	    fsg.delete();
	    let pcm = await fs.readFile("../tests/data/pizza-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("yo gimme four large all dressed pizzas", decoder.get_hyp());
	    decoder.delete();
	});
    });
})();

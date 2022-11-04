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
    describe("Test decoder initialization", () => {
	it("Should initialize the decoder", async () => {
	    let decoder = new ssjs.Decoder({
		fsg: "testdata/goforward.fsg",
		samprate: 16000,
	    });
            assert.ok(decoder);
            assert.equal(decoder.get_config("hmm"),
                         ssjs.get_model_path(ssjs.defaultModel));
            decoder.delete();
        });
    });
    describe("Test configuration as JSON", () => {
        it("Should contain default configuration", () => {
	    let decoder = new ssjs.Decoder({
		fsg: "testdata/goforward.fsg",
		samprate: 16000,
	    });
            assert.ok(decoder);
            let json = decoder.get_config_json();
            let config = JSON.parse(json);
            assert.ok(config);
            assert.equal(config.hmm,
                         ssjs.get_model_path(ssjs.defaultModel));
            assert.equal(config.loglevel, "WARN");
            assert.equal(config.fsg, "testdata/goforward.fsg");
        });
    });
    describe("Test acoustic model loading", () => {
	it('Should load acoustic model', async () => {
	    let decoder = new ssjs.Decoder();
	    await decoder.init_featparams();
	    await decoder.init_fe();
	    await decoder.init_feat();
	    await decoder.init_acmod();
	    await decoder.load_acmod_files();
            assert.ok(decoder);
            decoder.delete();
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
            decoder.delete();
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
            decoder.delete();
	});
	it('Should align "go forward ten meters"', async () => {
	    let decoder = new ssjs.Decoder({
		samprate: 16000,
	    });
	    await decoder.initialize();
	    let pcm = await fs.readFile("testdata/goforward-float32.raw");
            await decoder.set_align_text("go forward ten meters");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("go forward ten meters", decoder.get_hyp());
	    let hypseg = decoder.get_hypseg();
	    let hypseg_words = []
	    for (const seg of hypseg) {
		assert.ok(seg.end >= seg.start);
                if (seg.word != "<sil>" && seg.word != "(NULL)")
		    hypseg_words.push(seg.word);
	    }
	    assert.equal(hypseg_words.join(" "), "go forward ten meters");
            let jseg = await decoder.get_alignment_json();
            let result = JSON.parse(jseg);
            assert.ok(result);
	    assert.equal(result.t, "go forward ten meters");
            let json_words = [];
            let json_phones = [];
            for (const word of result.w) {
                if (word.t == "<sil>")
                    continue;
                json_words.push(word.t);
                for (const phone of word.w) {
                    json_phones.push(phone.t);
                }
            }
            assert.equal(json_words.join(" "), "go forward ten meters");
            assert.equal(json_phones.join(" "),
                         "G OW F AO R W ER D T EH N M IY T ER Z");

            decoder.delete();
	});
    });
    describe("Test dictionary and FSG", () => {
	it('Should recognize "_go _forward _ten _meters"', async () => {
	    let decoder = new ssjs.Decoder({samprate: 16000});
            decoder.unset_config("dict");
	    await decoder.initialize();
	    await decoder.add_word("_go", "G OW", false);
	    await decoder.add_word("_forward", "F AO R W ER D", false);
	    await decoder.add_word("_ten", "T EH N", false);
	    await decoder.add_word("_meters", "M IY T ER Z", true);
	    await decoder.set_fsg("goforward", 0, 4, [
		{from: 0, to: 1, prob: 1.0, word: "_go"},
		{from: 1, to: 2, prob: 1.0, word: "_forward"},
		{from: 2, to: 3, prob: 1.0, word: "_ten"},
		{from: 3, to: 4, prob: 1.0, word: "_meters"}
	    ]);
	    let pcm = await fs.readFile("testdata/goforward-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("_go _forward _ten _meters", decoder.get_hyp());
            decoder.delete();
	});
    });
    describe("Test reinitialization", () => {
	it('Should recognize "go forward ten meters"', async () => {
	    let decoder = new ssjs.Decoder({samprate: 16000});
	    await decoder.initialize();
	    await decoder.add_word("_go", "G OW", false);
	    await decoder.add_word("_forward", "F AO R W ER D", false);
	    await decoder.add_word("_ten", "T EH N", false);
	    await decoder.add_word("_meters", "M IY T ER Z", true);
	    await decoder.set_fsg("goforward", 0, 4, [
		{from: 0, to: 1, prob: 1.0, word: "_go"},
		{from: 1, to: 2, prob: 1.0, word: "_forward"},
		{from: 2, to: 3, prob: 1.0, word: "_ten"},
		{from: 3, to: 4, prob: 1.0, word: "_meters"}
	    ]);
            decoder.set_config("dict",
                               ssjs.get_model_path(ssjs.defaultModel) + "/dict.txt");
	    decoder.set_config("fsg", "testdata/goforward.fsg");
	    await decoder.initialize();
	    let pcm = await fs.readFile("testdata/goforward-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("go forward ten meters", decoder.get_hyp());
            decoder.delete();
	});
    });
    describe("Test loading model for other language", () => {
	it('Should recognize "avance de dix mètres"', async () => {
	    let decoder = new ssjs.Decoder({hmm: ssjs.get_model_path("fr-fr"),
					    samprate: 16000});
	    await decoder.initialize();
	    await decoder.set_fsg("goforward", 0, 4, [
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
	    let pcm = await fs.readFile("testdata/goforward_fr-float32.raw");
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
		jsgf: "testdata/pizza.gram",
		samprate: 16000
	    });
	    await decoder.initialize();
	    let pcm = await fs.readFile("testdata/pizza-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("yo gimme four large all dressed pizzas", decoder.get_hyp());
            decoder.delete();
	});
    });
    describe("Test JSGF string", () => {
	it('Should recognize "yo gimme four large all dressed pizzas"', async () => {
	    let decoder = new ssjs.Decoder({samprate: 16000});
	    await decoder.initialize();
	    await decoder.set_jsgf(`#JSGF V1.0;
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
	    let pcm = await fs.readFile("testdata/pizza-float32.raw");
	    await decoder.start();
	    await decoder.process(pcm, false, true);
	    await decoder.stop();
	    assert.equal("yo gimme four large all dressed pizzas", decoder.get_hyp());
            decoder.delete();
	});
    });
    describe("Test reinitialize_audio", () => {
	it('Should recognize "go forward ten meters"', async () => {
	    let decoder = new ssjs.Decoder({
		fsg: "testdata/goforward.fsg",
		samprate: 11025,
                loglevel: "INFO"
	    });
	    await decoder.initialize();
	    let pcm = await fs.readFile("testdata/goforward-float32.raw");
	    decoder.set_config("samprate", 16000);
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
            decoder.delete();
	});
    });
    describe("Test endpointer", () => {
        it("Should do endpointing", async () => {
            const ep = new ssjs.Endpointer(16000);
            const frame_size = ep.get_frame_size();
	    const pcm = await fs.readFile("testdata/goforward-float32.raw");
            const pcm32 = new Float32Array(pcm.buffer);
            console.log(`read ${pcm32.length} samples`);
            let idx = 0;
            while (idx < pcm32.length) {
                let prev_in_speech = ep.get_in_speech();
                let frame, speech;
                if (idx + frame_size > pcm32.length) {
                    frame = pcm32.subarray(idx);
                    speech = ep.end_stream(frame);
                }
                else {
                    frame = pcm32.subarray(idx, idx + frame_size);
                    speech = ep.process(frame);
                }
                if (speech !== null) {
                    if (!prev_in_speech) {
                        console.log("Speech started at " + ep.get_speech_start());
                        assert.ok(Math.abs(ep.get_speech_start()- 1.53) < 0.1);
                    }
                    if (!ep.get_in_speech()) {
                        console.log("Speech ended at " + ep.get_speech_end());
                        assert.ok(Math.abs(ep.get_speech_end()- 3.39) < 0.1);
                    }
                }
                idx += frame_size;
            }
        });
    });
    describe("Test dictionary lookup", () => {
	it('Should return "W AH N"', async () => {
	    let decoder = new ssjs.Decoder();
	    await decoder.initialize();
	    const phones = decoder.lookup_word("one");
	    assert.equal("W AH N", phones);
            decoder.delete();
	});
    });
})();

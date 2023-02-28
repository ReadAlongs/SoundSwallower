/* We have to pass "assert" here because JavaScript won't let us import it from
 *  different places in the same script depending on which environment we're
 *  running in (Node versus the Web) */
function check_alignment(hypseg, text, assert) {
  let hypseg_words = [];
  let prev = -1;
  for (const { t, b, d } of hypseg.w) {
    assert.ok(d > 0);
    assert.ok(b > prev);
    prev = b;
    if (t != "<sil>" && t != "(NULL)") hypseg_words.push(t);
  }
  assert.equal(hypseg_words.join(" "), text);
}

/* Same problem here with soundswallower and load_binary_file */
export function make_tests(soundswallower, load_binary_file, assert) {
  describe("Test initialization", () => {
    it("Should load the WASM module", () => {
      assert.ok(soundswallower);
    });
  });
  describe("Test decoder initialization", () => {
    it("Should initialize the decoder", async () => {
      let decoder = new soundswallower.Decoder({
        fsg: "testdata/goforward.fsg",
        samprate: 16000,
      });
      assert.ok(decoder);
      assert.equal(
        decoder.get_config("hmm"),
        soundswallower.get_model_path(soundswallower.defaultModel)
      );
      decoder.delete();
    });
  });
  describe("Test configuration as JSON", () => {
    it("Should contain default configuration", () => {
      let decoder = new soundswallower.Decoder({
        fsg: "testdata/goforward.fsg",
        samprate: 16000,
      });
      assert.ok(decoder);
      let json = decoder.get_config_json();
      let config = JSON.parse(json);
      assert.ok(config);
      assert.equal(
        config.hmm,
        soundswallower.get_model_path(soundswallower.defaultModel)
      );
      assert.equal(config.loglevel, "WARN");
      assert.equal(config.fsg, "testdata/goforward.fsg");
    });
  });
  describe("Test acoustic model loading", () => {
    it("Should load acoustic model", async () => {
      let decoder = new soundswallower.Decoder();
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
      let decoder = new soundswallower.Decoder({
        fsg: "testdata/goforward.fsg",
        samprate: 16000,
      });
      await decoder.initialize();
      let pcm = await load_binary_file("testdata/goforward-float32.raw");
      decoder.start();
      // 128-sample buffers to simulate bogus Web Audio API
      for (let pos = 0; pos < pcm.length; pos += 128) {
        let len = pcm.length - pos;
        if (len > 128) len = 128;
        decoder.process_audio(pcm.subarray(pos, pos + len), false, false);
      }
      decoder.stop();
      assert.equal("go forward ten meters", decoder.get_text());
      check_alignment(decoder.get_alignment(), "go forward ten meters", assert);
      decoder.delete();
    });
    it("Should accept Float32Array as well as UInt8Array", async () => {
      let decoder = new soundswallower.Decoder({
        fsg: "testdata/goforward.fsg",
        samprate: 16000,
      });
      await decoder.initialize();
      let pcm = await load_binary_file("testdata/goforward-float32.raw");
      let pcm32 = new Float32Array(pcm.buffer);
      decoder.start();
      decoder.process_audio(pcm32, false, true);
      decoder.stop();
      assert.equal("go forward ten meters", decoder.get_text());
      decoder.delete();
    });
    it('Should align "go forward ten meters"', async () => {
      let decoder = new soundswallower.Decoder({
        samprate: 16000,
      });
      await decoder.initialize();
      let pcm = await load_binary_file("testdata/goforward-float32.raw");
      decoder.set_align_text("go forward ten meters");
      decoder.start();
      decoder.process_audio(pcm, false, true);
      decoder.stop();
      assert.equal("go forward ten meters", decoder.get_text());
      check_alignment(decoder.get_alignment(), "go forward ten meters", assert);

      let result = decoder.get_alignment({ align_level: 1 });
      assert.ok(result);
      assert.equal(result.t, "go forward ten meters");
      let json_words = [];
      let json_phones = [];
      for (const word of result.w) {
        if (word.t == "<sil>") continue;
        json_words.push(word.t);
        for (const phone of word.w) {
          json_phones.push(phone.t);
        }
      }
      assert.equal(json_words.join(" "), "go forward ten meters");
      assert.equal(
        json_phones.join(" "),
        "G OW F AO R W ER D T EH N M IY T ER Z"
      );

      decoder.delete();
    });
  });
  describe("Test dictionary and FSG", () => {
    it('Should recognize "_go _forward _ten _meters"', async () => {
      let decoder = new soundswallower.Decoder({ samprate: 16000 });
      decoder.unset_config("dict");
      await decoder.initialize();
      decoder.add_words(
        ["_go", "G OW"],
        ["_forward", "F AO R W ER D"],
        ["_ten", "T EH N"],
        ["_meters", "M IY T ER Z"]
      );
      decoder.set_align_text("_go _forward _ten _meters");
      let pcm = await load_binary_file("testdata/goforward-float32.raw");
      decoder.start();
      decoder.process_audio(pcm, false, true);
      decoder.stop();
      assert.equal("_go _forward _ten _meters", decoder.get_text());
      decoder.delete();
    });
  });
  describe("Test reinitialization", () => {
    it('Should recognize "go forward ten meters"', async () => {
      let decoder = new soundswallower.Decoder({ samprate: 16000 });
      await decoder.initialize();
      decoder.add_words(
        ["_go", "G OW"],
        ["_forward", "F AO R W ER D"],
        ["_ten", "T EH N"],
        ["_meters", "M IY T ER Z"]
      );
      decoder.set_align_text("_go _forward _ten _meters");
      decoder.set_config(
        "dict",
        soundswallower.get_model_path(soundswallower.defaultModel) + "/dict.txt"
      );
      decoder.set_config("fsg", "testdata/goforward.fsg");
      await decoder.initialize();
      let pcm = await load_binary_file("testdata/goforward-float32.raw");
      decoder.start();
      decoder.process_audio(pcm, false, true);
      decoder.stop();
      assert.equal("go forward ten meters", decoder.get_text());
      decoder.delete();
    });
  });
  describe("Test loading model for other language", () => {
    it('Should recognize "avance de dix mètres"', async () => {
      let decoder = new soundswallower.Decoder({
        hmm: soundswallower.get_model_path("fr-fr"),
        samprate: 16000,
      });
      await decoder.initialize();
      decoder.set_grammar(`#JSGF V1.0;
grammar avance;
public <avance> = (avance | recule) (d' | de) <nombre> (mètre | mètres);
<nombre> = un | deux | trois | quatre | cinq | six | sept | huit | neuf | dix;
`);
      let pcm = await load_binary_file("testdata/goforward_fr-float32.raw");
      decoder.start();
      decoder.process_audio(pcm, false, true);
      decoder.stop();
      assert.equal("avance de dix mètres", decoder.get_text());
      decoder.delete();
    });
  });
  describe("Test JSGF", () => {
    it('Should recognize "yo gimme four large all dressed pizzas"', async () => {
      let decoder = new soundswallower.Decoder({
        jsgf: "testdata/pizza.gram",
        samprate: 16000,
      });
      await decoder.initialize();
      let pcm = await load_binary_file("testdata/pizza-float32.raw");
      decoder.start();
      decoder.process_audio(pcm, false, true);
      decoder.stop();
      assert.equal(
        "yo gimme four large all dressed pizzas",
        decoder.get_text()
      );
      decoder.delete();
    });
  });
  describe("Test JSGF string", () => {
    it('Should recognize "yo gimme four large all dressed pizzas"', async () => {
      let decoder = new soundswallower.Decoder({ samprate: 16000 });
      await decoder.initialize();
      decoder.set_grammar(`#JSGF V1.0;
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
      let pcm = await load_binary_file("testdata/pizza-float32.raw");
      decoder.start();
      decoder.process_audio(pcm, false, true);
      decoder.stop();
      assert.equal(
        "yo gimme four large all dressed pizzas",
        decoder.get_text()
      );
      /* Make sure we can do phone alignment here too */
      check_alignment(
        decoder.get_alignment({ align_level: 1 }),
        "yo gimme four large all dressed pizzas",
        assert
      );
      decoder.delete();
    });
  });
  describe("Test reinitialize_audio", () => {
    it('Should recognize "go forward ten meters"', async () => {
      let decoder = new soundswallower.Decoder({
        fsg: "testdata/goforward.fsg",
        samprate: 11025,
        loglevel: "INFO",
      });
      await decoder.initialize();
      let pcm = await load_binary_file("testdata/goforward-float32.raw");
      decoder.set_config("samprate", 16000);
      await decoder.reinitialize_audio();
      decoder.start();
      decoder.process_audio(pcm, false, true);
      decoder.stop();
      assert.equal("go forward ten meters", decoder.get_text());
      check_alignment(decoder.get_alignment(), "go forward ten meters", assert);
      decoder.delete();
    });
  });
  describe("Test dictionary lookup", () => {
    it('Should return "W AH N"', async () => {
      let decoder = new soundswallower.Decoder();
      await decoder.initialize();
      const phones = decoder.lookup_word("one");
      assert.equal("W AH N", phones);
      decoder.delete();
    });
  });
  describe("Test spectrogram", () => {
    it("Should create a spectrogram", async () => {
      let decoder = new soundswallower.Decoder();
      await decoder.initialize();
      let pcm = await load_binary_file("testdata/goforward-float32.raw");
      let { data, nfr, nfeat } = decoder.spectrogram(pcm);
      assert.equal(nfr, 129);
      assert.equal(nfeat, 20);
    });
  });
}

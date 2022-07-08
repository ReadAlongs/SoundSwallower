/* -*- javascript -*- */
import soundswallower_factory from "./soundswallower.js";
import {promises as fs} from "fs";
import * as assert from "assert";

(async () => {
    const soundswallower = await soundswallower_factory();
    const config = new soundswallower.Config();
    console.log(config.get("samprate"));
    let nkeys = 0;
    for (const key of config) {
        ++nkeys;
    }
    console.log(`Found ${nkeys} keys in config`);
    const decoder = new soundswallower.Decoder({
	fsg: "testdata/goforward.fsg",
	samprate: 16000,
    });
    await decoder.initialize();
    let pcm = await fs.readFile("testdata/goforward-float32.raw");
    await decoder.start();
    await decoder.process(pcm, false, true);
    await decoder.stop();
    let hyp = decoder.get_hyp();
    console.log(`recognized: ${hyp}`);
    assert.equal("go forward ten meters", decoder.get_hyp());
    let hypseg = decoder.get_hypseg();
    let hypseg_words = []
    for (const seg of hypseg) {
	assert.ok(seg.end >= seg.start);
        if (seg.word != "<sil>" && seg.word != "(NULL)")
	    hypseg_words.push(seg.word);
    }
    assert.deepStrictEqual(hypseg_words,
			   ["go", "forward", "ten", "meters"]);
    decoder.delete();
})();

/* -*- javascript -*- */
import soundswallower_factory from "./soundswallower.js";
import {SoundSwallowerModule, Decoder, Segment} from "./soundswallower.js";
import {promises as fs} from "fs";
import * as assert from "assert";

(async () => {
    const soundswallower: SoundSwallowerModule = await soundswallower_factory();
    /* Basic test */
    const decoder: Decoder = new soundswallower.Decoder({
	fsg: "testdata/goforward.fsg",
	samprate: 16000,
    });
    await decoder.initialize();
    let pcm: Uint8Array = await fs.readFile("testdata/goforward-float32.raw");
    await decoder.start();
    await decoder.process(pcm, false, true);
    await decoder.stop();
    let hyp: string = decoder.get_hyp();
    console.log(`recognized: ${hyp}`);
    assert.equal("go forward ten meters", hyp);
    let hypseg: Array<Segment> = decoder.get_hypseg();
    let hypseg_words = [];
    for (const seg of hypseg) {
	assert.ok(seg.end >= seg.start);
        if (seg.word != "<sil>" && seg.word != "(NULL)")
	    hypseg_words.push(seg.word);
    }
    assert.deepStrictEqual(hypseg_words,
			   ["go", "forward", "ten", "meters"]);

    /* Test create_fsg */
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
    pcm = await fs.readFile("testdata/goforward-float32.raw");
    await decoder.start();
    await decoder.process(pcm, false, true);
    await decoder.stop();
    hyp = decoder.get_hyp();
    console.log(`recognized: ${hyp}`);
    assert.equal("_go _forward _ten _meters", hyp);

    /* Test JSGF */
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
    pcm = await fs.readFile("testdata/pizza-float32.raw");
    await decoder.start();
    await decoder.process(pcm, false, true);
    await decoder.stop();
    hyp = decoder.get_hyp();
    console.log(`recognized: ${hyp}`);
    assert.equal("yo gimme four large all dressed pizzas", hyp);
})();

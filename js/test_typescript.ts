/* We have separate test code for TypeScript to make sure that type-checking
 * works properly, but perhaps we should just put the main tests here and use
 * tsc to transpile them as necessary, which could solve some problems and
 * eliminate some profanity in the comments. */
import {
  default as soundswallower_factory,
  SoundSwallowerModule,
  Decoder,
  Segment,
} from "./soundswallower.node.js";
import { promises as fs } from "node:fs";
import * as assert from "node:assert";

function check_alignment(hypseg: Segment, text: string) {
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

(async () => {
  const soundswallower: SoundSwallowerModule = await soundswallower_factory();
  /* Basic test */
  const decoder: Decoder = new soundswallower.Decoder({
    fsg: "testdata/goforward.fsg",
    samprate: 16000,
  });
  await decoder.initialize();
  let pcm: Uint8Array = await fs.readFile("testdata/goforward-float32.raw");
  decoder.start();
  decoder.process_audio(pcm, false, true);
  decoder.stop();
  let hyp: string = decoder.get_text();
  console.log(`recognized: ${hyp}`);
  assert.equal("go forward ten meters", hyp);
  check_alignment(decoder.get_alignment(), "go forward ten meters");

  pcm = await fs.readFile("testdata/goforward-float32.raw");
  decoder.add_words(
    ["_go", "G OW"],
    ["_forward", "F AO R W ER D"],
    ["_ten", "T EH N"],
    ["_meters", "M IY T ER Z"]
  );
  decoder.set_align_text("_go _forward _ten _meters");
  decoder.start();
  decoder.process_audio(pcm, false, true);
  decoder.stop();
  hyp = decoder.get_text();
  console.log(`recognized: ${hyp}`);
  assert.equal("_go _forward _ten _meters", hyp);
  check_alignment(decoder.get_alignment(), "_go _forward _ten _meters");

  /* Test JSGF */
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
  pcm = await fs.readFile("testdata/pizza-float32.raw");
  decoder.start();
  decoder.process_audio(pcm, false, true);
  decoder.stop();
  hyp = decoder.get_text();
  console.log(`recognized: ${hyp}`);
  assert.equal("yo gimme four large all dressed pizzas", hyp);
  check_alignment(
    decoder.get_alignment({ align_level: 1 }),
    "yo gimme four large all dressed pizzas"
  );
})();

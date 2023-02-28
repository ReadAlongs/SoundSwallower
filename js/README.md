# SoundSwallower: an even smaller speech recognizer

> "Time and change have a voice; eternity is silent. The human ear is
> always searching for one or the other."<br>
> Leena Krohn, _Datura, or a delusion we all see_

SoundSwallower is a refactored version of PocketSphinx intended for embedding in
web applications. The goal is not to provide a fast implementation of
large-vocabulary continuous speech recognition, but rather to provide a _small_
implementation of simple, useful speech technologies.

With that in mind the current version is limited to finite-state
grammar recognition.

## Installation

SoundSwallower can be installed in your NPM project:

    # From the Internets
    npm install soundswallower

Or you can use it directly from
[https://unpkg.com/soundswallower/umd/bundle.js](https://unpkg.com/soundswallower/umd/bundle.js)
in your HTML:

```html
<script src="https://unpkg.com/soundswallower/umd/bundle.js"></script>
<script>
decoder = soundswallower.Decoder();
// etcetera
</script>
```

In this case the `model` directory must be in the same location as
your HTML, or alternately you can specify a URL to your model with the
`url` parameter to `Decoder()`.

You can also build and install it from source, provided you have recent
versions of Emscripten and CMake installed:

    # From top-level soundswallower directory
    emcmake cmake -S . -B jsbuild
    cd jsbuild && make && npm install && npm run bundle
    # From your project's directory
    cd /path/to/my/project
    npm link /path/to/soundswallower/jsbuild

To run the tests, from the `jsbuild` directory:

    npm test
    npm run tstest
    npm run webtest

If you use it from TypeScript, you might encounter problems unless you set
`moduleResolution` to `"node16"` in your `tsconfig.json` [because
Reasons](https://stackoverflow.com/questions/58990498/package-json-exports-field-not-working-with-typescript).

Currently, because it's not really clear how to distribute data with JavaScript
packages, if you are using this with webpack, you will also need to add some
stuff to your `webpack.config.js` to get the model files in your output, and if
anyone can figure out how to make this happen automatically, please let me know
(possibly it involves `new URL(..., import.meta.url)`):

```js
const CopyPlugin = require("copy-webpack-plugin");
model.exports = {
  plugins: [
    new CopyPlugin({
      patterns: [
        {
          from: "node_modules/soundswallower/model",
          to: "model",
        },
      ],
    }),
  ],
};
```

NOTE also that with Angular you will find yourself in a world of pain unless you
add a custom webpack config that _enables_ the parsing of `import.meta.url`
which is disabled for some reason:

```js
module.exports = {
  module: {
    parser: {
      javascript: {
        url: true,
      },
    },
  },
};
```

For an "end-to-end" example of using SoundSwallower on the Web, look at the
[SoundSwallower-Demo
repository](https://github.com/dhdaines/soundswallower-demo).

## Basic Usage

The entire package is contained within a module compiled by
Emscripten. The NPM package includes only the compiled code, but you
can rebuild it yourself using [the full source code from
GitHub](https://github.com/ReadAlongs/SoundSwallower) which also
includes C and Python implementations.

As of version 0.6.2, we _only_ provide ES6 and UMD modules as
described above. This means you must have Node.js 14 or higher, and,
as noted above, you might need to add `"moduleResolution": "node16"`
to your `tsconfig.json`. It also means that you might need to use a
bleeding-edge version of `emcc` if you compile from source. Sorry
about this, but the JavaScript "ecosystem" is a toxic waste dump.

Unless you are using the `unpkg` bundle mentioned above, it uses
WebAssembly, so must be imported asynchronously. But... because, even
though ES6 `import` is async, you can't directly `import` WebAssembly
with it, you have to use this bogus incantation:

```js
import createModule from "soundswallower"; // or whatever name you like
const soundswallower = await createModule();
```

There is also a `jsonly` ES6 module which does not use WebAssembly, but
again, because Reasons, it has the same API:

```js
import createModule from "soundswallower/jsonly";
const soundswallower = await createModule();
```

Unfortunately, in this case, Emscripten's generated code currently has
[a bug](https://github.com/emscripten-core/emscripten/issues/18811)
where it still *tries* to access the non-existent WebAssembly file, so
you must add this incantation to your Webpack config:

```js
new webpack.IgnorePlugin({
    resourceRegExp: /\.wasm$/,
}),
```

Because the module needs access to the compiled WebAssembly code, all
of the actual API is contained within this object that you get back
from `createModule`. Now you can try to initialize a recognizer and
recognize some speech. This is asynchronous, because it does a bunch
of file loading and such, but the rest of the API isn't, because
Javascript promises are not actually coroutines and thus will block
your program even if they appear to be "asynchronous".

```js
let decoder = new soundswallower.Decoder({
  loglevel: "INFO",
  backtrace: true,
});
await decoder.initialize();
```

The optional `loglevel` and `backtrace` options will make it a bit
more verbose, so you can be sure it's actually doing something.

The simplest use case is to recognize some text we already know, which is called
"force alignment". In this case you set this text, which must already be
preprocessed to be a whitespace-separated string containing only words in the
dictionary, using `set_align_text`:

```js
decoder.set_align_text("go forward ten meters");
```

It is also possible to parse a grammar in
[JSGF](https://en.wikipedia.org/wiki/JSGF) format, see below for an
example.

Okay, let's wreck a nice beach! Record yourself saying something,
preferably the sentence "go forward ten meters", using SoX, for
example. Note that we record at 44.1kHz in 32-bit floating point
format as this is the default under JavaScript (due to WebAudio
limitations).

```sh
sox -c 1 -r 44100 -b 32 -e floating-point -d goforward.raw
```

Now you can load it and recognize it with:

```js
let audio = await fs.readFile("goforward.raw");
decoder.start();
decoder.process_audio(audio, false, true);
decoder.stop();
```

The text result can be obtained with `get_text()` or in a more detailed format
with time alignments using `get_alignment()`.

```js
console.log(decoder.get_text());
console.log(decoder.get_alignment());
```

For more detail on the alignment format, see [the PocketSphinx
documentation](https://github.com/cmusphinx/pocketsphinx#usage) as it is
borrowed from there. For example:

```js
const result = decoder.get_alignment_json({ align_level: 1 });
for (const { w, t, b, d, p } of result.w) {
  console.log(`word ${t} at ${b} has duration ${d} and probability ${p}`);
  for (const { t, b, d } of w) {
    console.log(`phone ${t} at ${b} has duration ${d}`);
  }
}
```

Finally, if your program is long-running and you think you might make multiple
recognizers, you ought to delete them, because JavaScript in its infinite wisdom
doesn't really do destructors or finalizers (yes, I am Aware of the existence of
[finalizer-like
objects](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/FinalizationRegistry)
which are entirely inscrutable, not guaranteed to actually do anything at all,
and probably not applicable to WASM bindings):

```js
decoder.delete();
```

## Loading models

By default, SoundSwallower will use a not particularly good acoustic
model and a reasonable dictionary for US English. A model for French
is also available, which you can load by default by setting the
`defaultModel` property in the module object before loading:

```js
import createModule from "soundswallower";
const soundswallower = {
  defaultModel: "fr-fr",
};
await createModule(soundswallower);
```

The default model is expected to live under the `model/` directory
relative to the current web page (on the web) or the `soundswallower`
module (in Node.js). You can modify this by setting the `modelBase`
property in the module object when loading, e.g.:

```js
import createModule from "soundswallower";
const soundswallower = {
  modelBase: "/assets/models/" /* Trailing slash is necessary */,
  defaultModel: "fr-fr",
};
await createModule(soundswallower);
```

This is simply concatenated to the model name, so you should make sure
to include the trailing slash, e.g. "model/" and not "model"!

## Using grammars

We currently support JSGF for writing grammars. You can parse one
from a JavaScript string and set it in the decoder like this (a
hypothetical pizza-ordering grammar):

```js
decoder.set_grammar(`#JSGF V1.0;
grammar pizza;
public <order> = [<greeting>] [<want>] [<quantity>] [<size>] [pizza] <toppings>;
<greeting> = hi | hello | yo | howdy;
<want> = i want | gimme | give me | i'd like to order | order | i wanna;
<quantity> = a | one | two | three | four | five;
<size> = small | medium | large | extra large | x large | x l;
<toppings> = [with] <topping> ([and] <topping>)*;
<topping> = olives | mushrooms | tomatoes | (green | hot) peppers | pineapple;
`);
```

Note that all the words in the grammar must first be defined in the
dictionary. You can add custom dictionary words using the `add_words`
method on the `Decoder` object, as long as you speak ArpaBet (or
whatever phoneset the acoustic model uses). IPA and
grapheme-to-phoneme support may become possible in the near future.

```js
decoder.add_words([
  "supercalifragilisticexpialidocious",
  "S UW P ER K AE L IH F R AE JH IH L IH S T IH K EH K S P IY AE L IH D OW SH Y UH S",
]);
```

## Voice activity detection / Endpointing

This is a work in progress, but it is also possible to detect the
start and end of speech in an input stream using an `Endpointer`
object. This requires you to pass buffers of a specific size, which
is understandably difficult since WebAudio also only wants to _give_
you buffers of a specific (and entirely different) size. A better
example is forthcoming but it looks a bit like this (copied directly
from [the
documentation](https://soundswallower.readthedocs.io/en/latest/soundswallower.js.html#Endpointer.get_in_speech):

```js
const ep = new Endpointer({ samprate: decoder.get_config("samprate"} });
let prev_in_speech = ep.get_in_speech();
let frame_size = ep.get_frame_size();
// Presume `frame` is a Float32Array of frame_size or less
let speech;
if (frame.size < frame_size) speech = ep.end_stream(frame);
else speech = ep.process(frame);
if (speech !== null) {
  if (!prev_in_speech)
    console.log("Speech started at " + ep.get_speech_start());
  if (!ep.get_in_speech())
    console.log("Speech ended at " + ep.get_speech_end());
}
```

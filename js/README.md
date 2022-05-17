SoundSwallower: an even smaller speech recognizer
=================================================

> "Time and change have a voice; eternity is silent. The human ear is
> always searching for one or the other."<br>
> Leena Krohn, *Datura, or a delusion we all see*

SoundSwallower is a refactored version of PocketSphinx intended for
embedding in web applications.  The goal is not to provide a fast
implementation of large-vocabulary continuous speech recognition, but
rather to provide a *small*, *asynchronous* implementation of simple,
useful speech technologies.

With that in mind the current version is limited to finite-state
grammar recognition.

Installation
------------

SoundSwallower can be installed in your NPM project:

	# From the Internets
	npm install soundswallower
	
You can also build and install it from source, provided you have
Emscripten and CMake installed:

	# From soundswallower/js
	npm run build:dev
	# From your project's directory
	cd /path/to/my/project
	npm link /path/to/soundswallower/js

For use in Node.js, no other particular action is required on your
part, though currently you will have to pre-load any model files you
want to use, as described below.

Because Webpack is blissfully unaware of all things
Emscripten-related, you will need a number of extra incantations in
your `webpack.config.js` if you want to include SoundSwallower in an
application that uses it, and there currently seems to be [no way
around this problem](https://github.com/webpack/webpack/issues/7352).
You will need to add this to the top of your `webpack.config.js`:

```js
const CopyPlugin = require("copy-webpack-plugin");
const modelDir = require("soundswallower/model");
```

Then to the `plugins` section in `config` or `module.exports`
(depending on how your config is structured):

```js
new CopyPlugin({
    patterns: [
		{ from: "node_modules/soundswallower/soundswallower.wasm*",
		  to: "[name][ext]"},
	{ from: modelDir,
	   to: "model"},
   ],
}),
```

And finally to the `resolve` section:

```js
fallback: {
	crypto: false,
	fs: false,
	path: false,
},
```

Basic Usage
-----------

The entire package is contained within a module compiled by
Emscripten.  The NPM package includes only the compiled code, but you
can rebuild it yourself using [the full source code from
GitHub](https://github.com/ReadAlongs/SoundSwallower) which also
includes C and Python implementations.

It follows the usual conventions of Emscripten, meaning that you must
require it as a CommonJS or Node.js module, which returns a function
that returns a promise that is fulfilled with the actual module once
the WASM code is fully loaded:

```js
const ssjs = await require('soundswallower')();
```

Once you figure out how to get the module, you can try to initialize
the recognizer and recognize some speech.

Great, so let's initialize the recognizer.  Anything that changes the
state of the recognizer is an async function.  So everything except
getting the current recognition result.  We follow the
construct-then-initialize pattern:

```js
let decoder = new ssjs.Decoder({
    loglevel: "INFO",
    backtrace: true
});
await decoder.initialize();
```

The optional `loglevel` and `backtrace` options will make it a bit
more verbose, so you can be sure it's actually doing something.  Now
we will create the world's stupidest grammar, which recognizes one
sentence:

```js
let fsg = decoder.create_fsg("goforward", 0, 4, [
    {from: 0, to: 1, prob: 1.0, word: "go"},
    {from: 1, to: 2, prob: 1.0, word: "forward"},
    {from: 2, to: 3, prob: 1.0, word: "ten"},
    {from: 3, to: 4, prob: 1.0, word: "meters"}
]);
await decoder.set_fsg(fsg);
```

You should `delete()` it, unless of course you intend to create a
bunch of them and swap them in and out.  It is also possible to parse
a grammar in [JSGF](https://en.wikipedia.org/wiki/JSGF) format,
documentation coming soon.

Okay, let's wreck a nice beach!  Grab [this 16kHz, 16-bit signed raw
PCM audio
file](https://github.com/ReadAlongs/SoundSwallower/raw/master/tests/data/goforward.raw)
or record your own.  Now you can load it and recognize it with:

```js
let pcm = await fs.readFile("goforward.raw");
await decoder.start();
await decoder.process_raw(pcm, false, true);
await decoder.stop();
```

The results can be obtained with `get_hyp()` or in a more detailed
format with time alignments using `get_hypseg()`.  These are not
asynchronous methods, as they do not depend on or change the state of
the decoder:

```js
console.log(decoder.get_hyp());
console.log(decoder.get_hypseg());
```

Finally, if your program is long-running and you think you might make
multiple recognizers, you ought to delete them, because JavaScript is
awful:

```js
decoder.delete();
```

Loading models
--------------

By default, SoundSwallower will use a not particularly good acoustic
model and a reasonable dictionary for US English.  Currently it will
support any Sphinx format acoustic model, many of which are available
for download at [the SourceForge
page](https://sourceforge.net/projects/cmusphinx/files/Acoustic%20and%20Language%20Models/).

On the web (see below for Node.js particularities), this can be done
by calling the `load_model` method on the SoundSwallower module, with
the base URL (usually relative) for the model as a parameter, e.g.:

```js
ssjs.load_model("some-model", "/assets/model/some-model", false);
```

The model path can be a directory path on the local filesystem
(for Node.js) or a relative or absolute URL (for the web).  This
assumes that your dictionary is located in the file `dict.txt` undern
the model path.  If not, you can pass the path to it as a fourth
argument to `load_model`, e.g.:

```js
ssjs.load_model("some-model", "/assets/model/some-model", false,
	            "alternate-dictionary.txt");
```

On Node.js, currently you have to preload models, which you can do by
pre-populating the module object with a `preRun()` method before
passing it to `require("soundswallower")`.  You will need to pass
`true` as the third argument to `load_model` to force pre-loading.
The exact incantation required is:

```js
const ssjs = {
    preRun() {
		ssjs.load_model(model_name, model_path, true);
    }
};
await require('soundswallower')(ssjs);
```

Note also that preloading actually *only* works on Node.js.

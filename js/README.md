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

Basic Usage
-----------

The entire package is contained within a module compiled by
Emscripten.  The NPM package includes only the compiled code, but you
can rebuild it yourself using [the full source code from
GitHub](https://github.com/ReadAlongs/SoundSwallower) which also
includes C and Python implementations.

It follows the usual, somewhat idiosyncratic conventions of
Emscripten, meaning that you must require it as a CommonJS or Node.js
module, which returns a function that returns a promise that is
fulfilled with the actual module once the WASM code is fully loaded:

```js
const ssjs = await require('soundswallower')();
```

This isn't wonderful, so it will probably change soon.

Once you figure out how to get the module, you can try to initialize
the recognizer and recognize some speech.  SoundSwallower is built
with the "MEMFS" file system emulation, so you have to load these into
Emscripten's virtual filesystem in order to access them.  This is done
by calling the methods `FS_createPath` and either `FS_createLazyFile`
or `FS_createPreloadedFile` on the module object with all of the
directories and files you wish to access.

So, let's start over again.  Under Node.js, the model directory can be
found using `require("soundswallower/model")`.  Then you have to
preload them, which you can do by pre-populating the module object
with a `preRun()` method before passing it to
`require("soundswallower")`.  The exact incantation required, which
will soon be automated for you, is:

```js
const ssjs = {
    preRun() {
        const model_dir = require('soundswallower/model');
        ssjs.FS_createPath("/", "en-us", true, true);

        ssjs.FS_createPreloadedFile("/", "en-us.dict",
                           model_dir + "/en-us.dict",
                           true, true);
        for (name of ["transition_matrices", "feat.params", "mdef",
                  "means", "noisedict", "sendump", "variances"]) {
            ssjs.FS_createPreloadedFile("/en-us", name,
                           model_dir + "/en-us/" + name,
                           true, true);
        }
    }
};
await require('soundswallower')(ssjs);
```

All this will become much easier soon!  I promise!  This promise will
not be rejected!

Great, so let's initialize the recognizer.  Anything that changes the
state of the recognizer is an async function.  So everything except
getting the current recognition result.  We follow the
construct-then-initialize pattern:

```js
let decoder = new ssjs.Decoder({
    hmm: "en-us",
    dict: "en-us.dict",
    loglevel: "INFO",
    backtrace: true
});
await decoder.initialize();
```

The `loglevel` and `backtrace` options will make it a bit more
verbose, so you can be sure it's actually doing something.  Now we
will create the world's stupidest grammar, which recognizes one
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

That's all for now!  A more user-friendly package is coming soon!

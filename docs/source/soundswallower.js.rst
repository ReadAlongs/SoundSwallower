soundswallower JavaScript package
=================================

SoundSwallower can be called either from Python (see
:doc:`soundswallower` for API details) or from JavaScript.  In the
case of JavaScript, we use `Emscripten <https://www.emscripten.org>`_
to compile the C library into WebAssembly, which is loaded by a
JavaScript wrapper module.  This means that there are certain
idiosyncracies that must be taken into account when using the library,
because JavaScript does not always have direct access to the
filesystem, and synchronous I/O is not always possible.  Also, the
runtime is quite different between `Node.js <https://nodejs.dev>`_ and
the Web, and these details cannot be completely hidden from the user.

Using SoundSwallower on the Web
-------------------------------

We are currently in the process of converting SoundSwallower-JS to use
asynchronous I/O at all times, but this is not yet complete.  In
particular, the initialization phase blocks when reading models.  For
this reason, you will need to run SoundSwallower-JS inside a `Web
Worker
<https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Using_web_workers>`_
or an `AudioWorkletNode
<https://developer.mozilla.org/en-US/docs/Web/API/AudioWorkletNode>`_.
A full example can be found at
https://github.com/dhdaines/soundswallower-demo - in particular, see
https://github.com/dhdaines/soundswallower-demo/blob/main/src/recognizer.js
for the details of initializing and calling the recognizer.

Most of the methods of :js:class:`Decoder` are asynchronous.  This
means that you must either call them from within an asynchronous
function using ``await``, or use the ``Promise`` they return in the
usual manner.  If this means nothing to you, please consult
https://developer.mozilla.org/en-US/docs/Learn/JavaScript/Asynchronous.

This may seem superfluous given that you already have to go to the
extra effort to run inside a Worker, but it allows us to have the same
API under Node.js.  One may ask if this is really worthwhile since it
seems that having code that is reusable on different runtimes is not a
big priority for the JavaScript community.

By default, a narrow-bandwidth English acoustic model is loaded and
made available.  If you want to use a different one, you can use
:js:func:`load_model` to make it visible to the library, upon which
you can then refer to it in the decoder configuration.  That sounds
confusing! Here is an example, presuming that you have downloaded and
unpacked the Brazilian Portuguese model and dictionary from
https://sourceforge.net/projects/cmusphinx/files/Acoustic%20and%20Language%20Models/Portuguese/
and placed them under ``/model`` in your web server root:

.. code-block:: javascript

    // Avoid loading the default model
    const ssjs = { defaultModel: null };
    await require('soundswallower')(ssjs);
    // Make Portuguese model visible, along with its dictionary
    ssjs.load_model("pt-br", "/model/cmusphinx-pt-br-5.2", "br-pt.dic");
    // Tell the decoder to use it
    const decoder = new ssjs.Decoder({hmm: "pt-br"});
    await decoder.initialize();

Using SoundSwallower under Node.js
----------------------------------

Using SoundSwallower-JS in Node.js is mostly straightforward.  Here is
a fairly minimal example.  First you can record yourself saying some
digits:

.. code-block:: console

   sox -c 1 -r 16000 -b 16 -d digits.raw

Now run this with ``node``:

.. code-block:: javascript

    (async () => { // Wrap everything in an async function call
	// Load the library and pre-load the default model
	const ssjs = await require("soundswallower")();
	const decoder = new ssjs.Decoder();
	// Initialization is asynchronous
	await decoder.initialize();
	const grammar = decoder.parse_jsgf(`#JSGF V1.0;
    grammar digits;
    public <digits> = <digit>*;
    <digit> = one | two | three | four | five | six | seven | eight
	| nine | ten | eleven;`); // It goes to eleven
	// Anything that changes decoder state is asynchronous
	await decoder.set_fsg(grammar);
	// We must manually release memory, because JavaScript
	// has no destructors, whose great idea was that?
	grammar.delete();
	// Default input is 16kHz, 16-bit integer PCM
	const fs = require("fs/promises");
	let pcm = await fs.readFile("digits.raw");
	// Start speech processing
	await decoder.start();
	// Takes a typed array, as returned by readFile
	await decoder.process_raw(pcm);
	// Finalize speech processing
	await decoder.stop();
	// Get recognized text (NOTE: synchronous method)
	console.log(decoder.get_hyp());
	// Again we must manually release memory
	decoder.delete();
    })();

One caveat is that just as on the Web, configuration options such as
``hmm`` (for the acoustic model) or ``jsgf`` (for grammars) do not
have access to the filesystem, so any files you refer to in `Config`
must be "loaded" into the virtual Emscripten filesystem.  Even worse,
doing this "lazily" is currently broken under Node.js.  So you must do
this in a ``preRun()`` method which is passed when loading the
library, as in this example from the test suite:

.. code-block:: javascript

    const ssjs = {
	preRun() {
	    ssjs.FS_createPreloadedFile("/", "goforward.fsg",
					"../tests/data/goforward.fsg", true, true);
	    ssjs.FS_createPreloadedFile("/", "pizza.gram",
					"../tests/data/pizza.gram", true, true);
	    ssjs.load_model("fr-fr", "model/fr-fr");
	}
    };
    await require('./soundswallower.js')(ssjs);

Unfortunately, even though a solution exists with the `NODERAWFS
<https://emscripten.org/docs/api_reference/Filesystem-API.html#noderawfs>`_
option to Emscripten, as with everything in JavaScript-land, it
requires you to recompile for a specific environment, and the code
then no longer works on the Web.  So we don't do this at the moment.

Decoder class
-------------

.. js:autoclass:: pre_soundswallower.Decoder
   :members:
   :short-name:

Config class
-------------

.. js:autoclass:: pre_soundswallower.Config
   :members:
   :short-name:

Functions
---------

.. js:autofunction:: pre_soundswallower.load_model
   :short-name:

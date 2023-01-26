soundswallower JavaScript package
=================================

SoundSwallower can be called either from Python (see
:doc:`soundswallower` for API details) or from JavaScript.  In the
case of JavaScript, we use `Emscripten <https://www.emscripten.org>`_
to compile the C library into WebAssembly, which is loaded by a
JavaScript wrapper module.  This means that there are certain
idiosyncracies that must be taken into account when using the library,
mostly with respect to deployment and initialization.

Using SoundSwallower on the Web
-------------------------------

Since version 0.3.0, SoundSwallower's JavaScript API can be used
directly from a web page without any need to wrap it in a Web Worker.
You may still wish to do so if you are processing large blocks of data
or running on a slower machine.  Doing so is currently outside the
scope of this document.

Initialization of :js:class:`Decoder` is separate from configuration
and asynchronous.  This means that you must either call it from within
an asynchronous function using ``await``, or use the ``Promise`` it
returns in the usual manner.  If this means nothing to you, please
consult
https://developer.mozilla.org/en-US/docs/Learn/JavaScript/Asynchronous.

For the moment, to use SoundSwallower with Webpack, an incantation is
required in your `webpack.config.js`.  Sorry, I don't make the rules:

.. code-block:: javascript

    const CopyPlugin = require("copy-webpack-plugin");

    // Then... in your `module_exports` or `config` or whatever:
    plugins: [
        new CopyPlugin({
            patterns: [
            // Copy the model files (add any excludes you like)
            { from: "node_modules/soundswallower/model",
              to: "model"},
        ],

For a more elaborate example, see [the soundswallower-demo
code](https://github.com/dhdaines/soundswallower-demo).

Using SoundSwallower under Node.js
----------------------------------

Using SoundSwallower-JS in Node.js is mostly straightforward.  Here is
a fairly minimal example.  First you can record yourself saying some
digits (note that we record in 32-bit floating-point at 44.1kHz, which
is the default format for WebAudio and thus the default in
SoundSwallower-JS as well):

.. code-block:: console

   sox -c 1 -r 44100 -b 32 -e floating-point -d digits.raw

Now run this with ``node``:

.. code-block:: javascript

    import createModule from "soundswallower";
    import load_binary_file from "soundswallower";
    (async () => { // Wrap everything in an async function call
	const soundswallower = createModule();
	const decoder = new soundswallower.Decoder();
	// Initialization is asynchronous
	await decoder.initialize();
	const grammar = decoder.set_jsgf(`#JSGF V1.0;
    grammar digits;
    public <digits> = <digit>*;
    <digit> = one | two | three | four | five | six | seven | eight
	| nine | ten | eleven;`); // It goes to eleven
	// Default input is 16kHz, 32-bit floating-point PCM
	let pcm = await load_binary_file("digits.raw");
	// Start speech processing
	decoder.start();
	// Takes a typed array, as returned by readFile
	decoder.process_audio(pcm);
	// Finalize speech processing
	decoder.stop();
	// Get recognized text (NOTE: synchronous method)
	console.log(decoder.get_text());
	// We must manually release memory...
	decoder.delete();
    })();


Decoder class
-------------

.. js:autoclass:: api.Decoder
   :members:
   :short-name:

Endpointer class
----------------

.. js:autoclass:: api.Endpointer
   :members:
   :short-name:

Functions
---------

.. js:autofunction:: api.get_model_path
   :short-name:

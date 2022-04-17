SoundSwallower JavaScript
=========================

SoundSwallower can be compiled to JavaScript and run in Node.js or a
browser (recent versions of Chrome, Firefox, and Edge all seem to
work).

By default the library is compiled with `-sMODULARIZE=1`,
meaning that `require('soundswallower.js')` in Node.js will return a
factory function, which, when called, returns a promise that is
resolved after the WASM is loaded and the compiled code is ready to be
invoked, e.g.:

    const ssjs = await require("js/soundswallower.js");
	let config = new ssjs.Config();
	let recognizer = new ssjs.Recognizer(config);
	// etc, etc...

For web applications, this function will be available as `Module` in
the global namespace after loading `soundswallower.js`, from a Web
Worker, for instance, and you can also call this asynchronously to get
the instance, for example:

	(async () => {
		importScripts("js/soundswallower.js");
		const ssjs = await Module();
		let config = new ssjs.Config();
		// etc, etc...
	})();

There may be a better way to do this, but I am not yet knowledgeable
enough in JavaScript to say what it is.

The JavaScript API will probably change soon, so it is, sadly, not
documented here.

The rest of this file is the documentation inherited from
[pocketsphinx.js](http://syl22-00.github.io/pocketsphinx.js/) and will
be subject to change.

Audio Recorder
==============

The audio recorder is a small JavaScript library that captures audio
from the microphone, resamples it at a the desired sample rate, and
feeds it to any desired audio application (such as
`pocketsphinx.js`). It makes use of the web audio API, WebRTC and web
workers and it is derived from
[Recorderjs](https://github.com/mattdiamond/Recorderjs). It works on
Chrome and Firefox (25+). To know more about audio capture and
playback on the web, you can have a look at this [overview of audio on
the
Web](https://github.com/syl22-00/TechDocs/blob/master/AudioInBrowser.md).

# 1. Files

You need to include both `audioRecorder.js` and
`audioRecorderWorker.js` in your web application. You can find them in
the `webapp/js/` folder. The default parameters assume that
`audioRecorderWorker.js` is in a `js/` subfolder starting from the
HTML file. You can change this with a parameter to pass at
initialization time.

# 2. Usage

In your HTML, include `audioRecorder.js`:

```html
<script src="js/audioRecorder.js"></script>
```

When the page is loaded, you can initialize the recorder. To create a
new instance of `AudioRecorder`, you need an input node generated from
a stream created by `getUserMedia`. So, what you need to do is:

* Get an instance of `AudioContext`.
* Create a callback function to pass to `getUserMedia` in case of
  success (and also one in case of failure).
* In the callback, create a web audio API node with
  `createMediaStreamSource` using the stream given by `getUserMedia`.
* In the callback, instantiate the `AudioRecorder` using the newly created node.
* Attach consumers to the recorder.
* Call getUserMedia, with `{audio: true}` as parameter.

In practice, it could look like this:

```javascript
// Deal with prefixed APIs
window.AudioContext = window.AudioContext || window.webkitAudioContext;
navigator.getUserMedia = navigator.getUserMedia ||
                         navigator.webkitGetUserMedia ||
                         navigator.mozGetUserMedia;
    
// Instantiating AudioContext
try {
    var audioContext = new AudioContext();
} catch (e) {
    console.log("Error initializing Web Audio");
}

var recorder;
// Callback once the user authorizes access to the microphone:
function startUserMedia(stream) {
    var input = audioContext.createMediaStreamSource(stream);
    recorder = new AudioRecorder(input);
    // We can, for instance, add a recognizer as consumer
    if (recognizer) recorder.consumers.push(recognizer);
 };
    
// Actually call getUserMedia
if (navigator.getUserMedia)
    navigator.getUserMedia({audio: true},
                           startUserMedia,
                           function(e) {console.log("No live audio input in this browser");}
                          );
else console.log("No web audio support in this browser");
```


# 3. Parameters

In the previous section, we kept the default values of the parameters,
but a config object can be used at initialization time:

```javascript
recorder = new AudioRecorder(input, config);
```

where `config` can have the following fields:

* `errorCallback`: Callback when the recorder encounters an issue. The
  callback takes one argument which is a string that describes the
  error. For now, the only possible error is the self-explanatory
  `"silent"`.
* `inputBufferLength`: Sets the length of buffer to use for audio
  input (defaults to 4096, no matter the input sample rate).
* `outputBufferLength`: Sets the length of buffer to send to consumers
  (defaults to 4000, no matter the output sample rate).
* `outputSampleRate`: Sets the output sample rate for the data to be
  sent to the consumers (defaults to 16000Hz). The output sample rate
  should not be greater than the input sample rate (which depends on
  the audio hardware, usually 44.1kHz or 48kHz).
* `worker`: Location of the `audioRecorderWorker.js` file, starting
  from the HTML file that loads `audioRecorder.js` (defaults to
  `js/audioRecorderWorker.js`).

# 4. API

In this section, we assume we have an `AudioRecorder` instance called `recorder`.

## 4.a Start, Stop, Cancel

* To start recording, call `recorder.start(data)`, where `data` is
  anything you want to send to the consumers when recording
  starts. For instance if your consumer is a speech recognizer, you
  might want to pass the language model to use.
* To stop, use `recorder.stop()` or `recorder.cancel()`.

## 4.b Consumers

Audio samples generated by the audio recorder are then passed to
consumers. You can have many consumers listening to the data. For
instance, if you have a speech recognizer and an audio visualizer:

```javascript
recorder.consumers = [recognizer, visualizer];
```

Consumers should be web workers that understand the following messages:

* `{command: 'start', data: data}`, posted when `recorder.start(data)`
  is called. `data` can be anything useful to the consumer. For
  instance it can indicate which language model to use to a speech
  recognizer.
* `{command: 'stop'}`, posted when `recorder.stop()` is called.
* `{command: 'process', data: audioSamples}`, posted every time an
  audio buffer is available, the buffer is stored in `data`. The
  buffer is a `Int16Array` typed array. If you need to pass it in a
  message for Google's PPAPI, you might need to take the underlying
  `ArrayBuffer` with `audioSamples.buffer`.


# 5. License

The files `webapp/js/audioRecorder.js` and
`webapp/js/audioRecorderWorker.js` are based on
[Recorder.js](https://github.com/mattdiamond/Recorderjs), which is
under the MIT license (Copyright © 2013 Matt Diamond).

The remaining of this software is licensed under the MIT license:

Copyright © 2013-2014 Sylvain Chevalier

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

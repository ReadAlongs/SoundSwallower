// Originally written by and
// Copyright © 2013-2017 Sylvain Chevalier
// MIT license, see LICENSE for details
// Substantially revised by David Huggins-Daines

// After loading emscripten module, the instance will be stored here.
var ssjs;

function startup(onMessage) {
    this.onmessage = async (event) => {
	importScripts("soundswallower.js");
	ssjs = await Module();
        this.onmessage = onMessage;
        this.postMessage({});
    };
}

startup(async (event) => {
    switch(event.data.command){
    case 'initialize':
	await initialize(event.data.data, event.data.callbackId);
	break;
    case 'lazyLoadModel':
	lazyLoadModel(event.data.data, event.data.callbackId);
	break;
    case 'addWords':
	addWords(event.data.data, event.data.callbackId);
	break;
    case 'setGrammar':
	await setGrammar(event.data.data, event.data.callbackId);
	break;
    case 'start':
	await start(event.data.data);
	break;
    case 'stop':
	await stop();
	break;
    case 'process':
	await process(event.data.data);
	break;
    }
});

// Make a shortcut for this.postMessage
var mySelf = this;
var post = function(message) {
    mySelf.postMessage(message);
};

var recognizer;
async function initialize(data, clbId) {
    let config = data;
    recognizer = new ssjs.Decoder(config);
    await recognizer.initialize();
    if (recognizer === undefined) post({status: "error", command: "initialize", code: -1});
    else post({status: "done", command: "initialize", id: clbId});
}

function lazyLoadModel(data, clbId) {
    const model_name = data;
    const dest_model_dir = "/" + model_name;
    const src_model_dir = "model/" + model_name;
    const folders = [["/", model_name]];
    const files = [
        ["/", model_name + ".dict", src_model_dir + ".dict"],
	[dest_model_dir, "feat.params", src_model_dir + "/feat.params"],
	[dest_model_dir, "mdef", src_model_dir + "/mdef"],
	[dest_model_dir, "means", src_model_dir + "/means"],
	[dest_model_dir, "noisedict", src_model_dir + "/noisedict"],
	[dest_model_dir, "sendump", src_model_dir + "/sendump"],
	[dest_model_dir, "transition_matrices", src_model_dir + "/transition_matrices"],
	[dest_model_dir, "variances", src_model_dir + "/variances"]
    ];
    const preloadFiles = () => {
	for (const folder of folders)
	    ssjs.FS_createPath(folder[0], folder[1], true, true);
	for (const file of files)
	    ssjs.FS_createLazyFile(file[0], file[1], file[2], true, true);
    };
    if (ssjs['calledRun']) {
	preloadFiles();
    } else {
	if (!ssjs['preRun']) ssjs['preRun'] = [];
	ssjs["preRun"].push(preloadFiles); // FS is not initialized yet, wait for it
    }
    post({status: "done", command: "lazyLoadModel", id: clbId});
}

function addWords(data, clbId) {
    if (recognizer) {
	for (var i = 0 ; i < data.length ; i++) {
	    var w = data[i];
	    if (w.length == 2) {
		var rv = recognizer.add_word(w[0], w[1], (i == data.length - 1));
		if (rv == -1)
		    break;
	    }
	}
	if (rv == -1) post({status: "error", command: "addWords", code: output});
	else post({id: clbId});
    } else post({status: "error", command: "addWords", code: "js-no-recognizer"});
}

async function setGrammar(data, clbId) {
    var output;
    if (recognizer) {
	if (data.hasOwnProperty('numStates') && data.numStates > 0 &&
	    data.hasOwnProperty('start') &&
	    data.hasOwnProperty('end') &&
	    data.hasOwnProperty('transitions') && data.transitions.length > 0) {
	    try {
		let fsg = recognizer.create_fsg("_default",
						data.start, data.end,
						data.transitions);
		await recognizer.set_fsg(fsg);
		post({id: clbId, data: 0, status: "done", command: "setGrammar"});
	    }
	    catch (e) {
		post({status: "error", command: "setGrammar", code: e.message});
	    }
	} else post({status: "error", command: "setGrammar", code: "js-data"});
    } else post({status: "error", command: "setGrammar", code: "js-no-recognizer"});
}

async function start() {
    if (recognizer) {
	try {
	    await recognizer.start();
	}
	catch (e) {
	    post({status: "error", command: "start", code: e.message});
	}
    } else {
	post({status: "error", command: "start", code: "js-no-recognizer"});
    }
}

async function stop() {
    if (recognizer) {
	try {
	    await recognizer.stop();
	    let hyp = recognizer.get_hyp();
	    let hypseg = recognizer.get_hypseg();
	    post({hyp: hyp, hypseg: hypseg, final: true});
	}
	catch (e) {
	    post({status: "error", command: "stop", code: e.message});
	}
    } else {
	post({status: "error", command: "stop", code: "js-no-recognizer"});
    }
}

async function process(array) {
    if (recognizer) {
	try {
	    var output = await recognizer.process_raw(array);
	    let hyp = recognizer.get_hyp();
	    let hypseg = recognizer.get_hypseg();
	    post({hyp: hyp, hypseg: hypseg});
	}
	catch (e) {
	    post({status: "error", command: "process", code: e.message});
	}
    } else {
	post({status: "error", command: "process", code: "js-no-recognizer"});
    }
}

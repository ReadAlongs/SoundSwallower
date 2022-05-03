// Originally written by and
// Copyright © 2013-2017 Sylvain Chevalier
// MIT license, see LICENSE for details
// Substantially revised by David Huggins-Daines

// After loading emscripten module, the instance will be stored here.
var ssjs;

function startup(onMessage) {
    const self = this;
    self.onmessage = async function(event) {
	importScripts("js/soundswallower.js");
	ssjs = await Module({
	    locateFile(path, scriptDirectory) {
		return scriptDirectory + "js/" + path;
	    }
	});
        self.onmessage = onMessage;
        self.postMessage({});
    };
}

startup(function(event) {
    switch(event.data.command){
    case 'initialize':
	initialize(event.data.data, event.data.callbackId);
	break;
    case 'lazyLoad':
	lazyLoad(event.data.data, event.data.callbackId);
	break;
    case 'addWords':
	addWords(event.data.data, event.data.callbackId);
	break;
    case 'setGrammar':
	setGrammar(event.data.data, event.data.callbackId);
	break;
    case 'start':
	start(event.data.data);
	break;
    case 'stop':
	stop();
	break;
    case 'process':
	process(event.data.data);
	break;
    }
});

var mySelf = this;
var post = function(message) {
    mySelf.postMessage(message);
};

var recognizer;

function initialize(data, clbId) {
    var config = new ssjs.Config();
    if (data) {
	while (data.length > 0) {
	    var p = data.pop();
	    if (p.length == 2) {
		config.set(p[0], p[1]);
	    } else {
		post({status: "error", command: "initialize", code: "js-data"});
	    }
	}
    }
    recognizer = new ssjs.Decoder(config);
    config.delete();
    if (recognizer === undefined) post({status: "error", command: "initialize", code: -1});
    else post({status: "done", command: "initialize", id: clbId});
}

function lazyLoad(data, clbId) {
    var files = [];
    var folders = [];
    data['folders'].forEach(function(folder) {folders.push([folder[0], folder[1]]);});
    data['files'].forEach(function(file) {files.push([file[0], file[1], file[2]]);});
    var preloadFiles = function() {
	folders.forEach(function(folder) {
	    ssjs.FS_createPath(folder[0], folder[1], true, true);
	});
	files.forEach(function(file) {
	    ssjs.FS_createLazyFile(file[0], file[1], file[2], true, true);
	});
    };
    if (ssjs['calledRun']) {
	preloadFiles();
    } else {
	if (!ssjs['preRun']) ssjs['preRun'] = [];
	ssjs["preRun"].push(preloadFiles); // FS is not initialized yet, wait for it
    }
    post({status: "done", command: "lazyLoad", id: clbId});
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

function setGrammar(data, clbId) {
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
		recognizer.set_fsg(fsg);
		post({id: clbId, data: 0, status: "done", command: "setGrammar"});
	    }
	    catch (e) {
		post({status: "error", command: "setGrammar", code: e.message});
	    }
	} else post({status: "error", command: "setGrammar", code: "js-data"});
    } else post({status: "error", command: "setGrammar", code: "js-no-recognizer"});
}

function start() {
    if (recognizer) {
	try {
	    recognizer.start();
	}
	catch (e) {
	    post({status: "error", command: "start", code: e.message});
	}
    } else {
	post({status: "error", command: "start", code: "js-no-recognizer"});
    }
}

function stop() {
    if (recognizer) {
	try {
	    recognizer.stop();
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

function process(array) {
    if (recognizer) {
	try {
	    var output = recognizer.process_raw(array);
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

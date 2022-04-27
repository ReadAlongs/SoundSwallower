// Originally written by and
// Copyright © 2013-2017 Sylvain Chevalier
// MIT license, see LICENSE for details
// Substantially revised by David Huggins-Daines

// After loading emscripten module, the instance will be stored here.
var ssjs;

function startup(onMessage) {
    const self = this;
    self.onmessage = async function(event) {
        var soundswallowerJS = (event.data && 'soundswallower.js' in event.data) ? event.data['soundswallower.js'] : 'soundswallower.js';
	// FIXME: We should maybe just require or import it?
	importScripts(soundswallowerJS);
	ssjs = await Module()
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
    var config = ssjs.cmd_ln_init();
    if (data) {
	while (data.length > 0) {
	    var p = data.pop();
	    if (p.length == 2) {
		ssjs.cmd_ln_set(config, p[0], p[1]);
	    } else {
		post({status: "error", command: "initialize", code: "js-data"});
	    }
	}
    }
    recognizer = ssjs.ps_init(config);
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
		var rv = ssjs.ps_add_word(recognizer, w[0], w[1],
					  (i == data.length - 1));
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
	    let fsg = ssjs.ps_create_fsg(recognizer, "_default",
					 data.start, data.end,
					 data.transitions);
	    if (!fsg) {
		output = -1;
	    }
	    else {
		output = ssjs.ps_set_fsg(recognizer, fsg);
	    }
	    if (output != 0) post({status: "error", command: "setGrammar", code: output});
	    else post({id: clbId, data: 0, status: "done", command: "setGrammar"});
	} else post({status: "error", command: "setGrammar", code: "js-data"});

    } else post({status: "error", command: "setGrammar", code: "js-no-recognizer"});
}

function start() {
    if (recognizer) {
	var output = ssjs.ps_start_utt(recognizer);
	if (output != 0)
	    post({status: "error", command: "start", code: output});
    } else {
	post({status: "error", command: "start", code: "js-no-recognizer"});
    }
}

function stop() {
    if (recognizer) {
	var output = ssjs.ps_end_utt(recognizer);
	if (output != 0)
	    post({status: "error", command: "stop", code: output});
	else {
	    let hyp = ssjs.ps_get_hyp(recognizer, 0);
	    let hypseg = ssjs.ps_get_hypseg(recognizer);
	    post({hyp: hyp, hypseg: hypseg, final: true});
	}
    } else {
	post({status: "error", command: "stop", code: "js-no-recognizer"});
    }
}

function process(array) {
    if (recognizer) {
	let byte_array = new Int8Array(array.buffer);
	var output = ssjs.ps_process_raw(recognizer, byte_array,
					 byte_array.length / 2,
					 false, false);
	if (output < 0)
	    post({status: "error", command: "process", code: output});
	else {
	    let hyp = ssjs.ps_get_hyp(recognizer, 0);
	    let hypseg = ssjs.ps_get_hypseg(recognizer);
	    post({hyp: hyp, hypseg: hypseg});
	}
    } else {
	post({status: "error", command: "process", code: "js-no-recognizer"});
    }
}

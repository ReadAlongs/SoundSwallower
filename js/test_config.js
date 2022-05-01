var ssjs;

(async () => {
    ssjs = await require('./js/soundswallower.js')();

    let conf = new ssjs.Config();
    conf.set("-hmm", "en-us")
    console.log(conf.get("-hmm"));
    console.log('cmd_ln' in conf);
    conf.delete();

    conf = new ssjs.Config({hmm: "en-us", bestpath: true});
    console.log(conf.get("-hmm"));
    console.log(conf.get("-bestpath"));
})();

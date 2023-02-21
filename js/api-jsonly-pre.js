// Partially work around Emscripten's broken -sWASM=0 (Webpack
// configuration also needed to avoid trying to bundle the missing
// .wasm file)
Module.locateFile = () => null;

/**
 * Async read some JSON (maybe there is a built-in that does this?)
 */
async function load_json(path) {
  const response = await fetch(path);
  if (response.ok) return response.json();
  else throw new Error("Failed to fetch " + path + " :" + response.statusText);
}

/**
 * Load a file from disk or Internet and make it into an s3file_t.
 */
async function load_to_s3file(path) {
  let blob_u8;
  const response = await fetch(path);
  if (response.ok) {
    const blob = await response.blob();
    const blob_buf = await blob.arrayBuffer();
    blob_u8 = new Uint8Array(blob_buf);
  } else
    throw new Error("Failed to fetch " + path + " :" + response.statusText);
  const blob_len = blob_u8.length + 1;
  const blob_addr = Module._malloc(blob_len);
  if (blob_addr == 0)
    throw new Error("Failed to allocate " + blob_len + " bytes for " + path);
  writeArrayToMemory(blob_u8, blob_addr);
  // Ensure it is NUL-terminated in case someone treats it as a string
  HEAP8[blob_addr + blob_len] = 0;
  // But exclude the trailing NUL from file size so it works normally
  return Module._s3file_init(blob_addr, blob_len - 1);
}

/**
 * Get a model or model file from the built-in model path.
 *
 * The base path can be set by modifying the `modelBase` property of
 * the module object, at initialization or any other time.  Or you can
 * also just override this function if you have special needs.
 *
 * This function is used by `Decoder` to find the default model, which
 * is equivalent to `Model.modelBase + Model.defaultModel`.
 *
 * @param {string} subpath path to model directory or parameter
 * file, e.g. "en-us", "en-us/variances", etc
 * @returns {string} concatenated path. Note this is a simple string
 * concatenation on the Web, so ensure that `modelBase` has a trailing
 * slash if it is a directory.
 */
function get_model_path(subpath) {
  return Module.modelBase + subpath;
}

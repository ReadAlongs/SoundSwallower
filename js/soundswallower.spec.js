import { default as createModule, load_binary_file } from "soundswallower";
import { make_tests } from "./tests.js";
import { assert } from "chai";
(async () => {
  const soundswallower = await createModule();
  make_tests(soundswallower, load_binary_file, assert);
})();

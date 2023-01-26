import { default as createModule, load_binary_file} from "soundswallower";
import { make_tests } from "./tests.js"; import { assert } from "chai";
make_tests(createModule, load_binary_file, assert);

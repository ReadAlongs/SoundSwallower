import { readFile } from "node:fs/promises";
import { join } from "node:path";

/**
 * Async read some binary data
 */
export async function load_binary_file(path) {
  return readFile(path);
}

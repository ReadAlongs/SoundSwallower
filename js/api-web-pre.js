import { Blob } from "blob-polyfill";

/**
 * Async read some binary data
 */
export async function load_binary_file(path) {
  const response = await fetch(path);
  if (response.ok) {
    let blob = await response.blob();
    return new Uint8Array(await blob.arrayBuffer());
  } else
    throw new Error("Failed to fetch " + path + " :" + response.statusText);
}

/// <reference types="emscripten" />
export class Decoder {
  initialized: boolean;
  delete(): void;
  get_config_json(): string;
  set_config(key: string, val: string | number): boolean;
  unset_config(key: string): void;
  get_config(key: string): string | number;
  has_config(key: string): boolean;
  initialize(): Promise<any>;
  reinitialize_audio(): Promise<void>;
  start(): void;
  stop(): void;
  process_audio(
    pcm: Float32Array | Uint8Array,
    no_search?: boolean,
    full_utt?: boolean
  ): number;
  get_text(): string;
  get_alignment({
    start,
    align_level,
  }?: {
    start?: number;
    align_level?: number;
  }): Segment;
  lookup_word(word: string): string;
  add_words(...words: Array<DictEntry>): void;
  set_grammar(jsgf_string: string, toprule?: string): void;
  set_align_text(text: string): void;
  spectrogram(pcm: Float32Array | Uint8Array): FeatureBuffer;
}
export class Endpointer {
  get_frame_size(): number;
  get_frame_length(): number;
  get_in_speech(): boolean;
  get_speech_start(): number;
  get_speech_end(): number;
  process(frame: Float32Array): Float32Array;
  end_stream(frame: Float32Array): Float32Array;
}
export type DictEntry = [string, string];
export interface Segment {
  t: string;
  b: number;
  d: number;
  p: number;
  w?: Array<Segment>;
}
export interface Config {
  [key: string]: string | number | boolean;
}
export interface FeatureBuffer {
  data: Float32Array;
  nfr: number;
  nfeat: number;
}

export interface SoundSwallowerModule extends EmscriptenModule {
  get_model_path(subpath: string): string;
  load_json(path: string): any;
  Decoder: {
    new (config?: Config): Decoder;
  };
  Endpointer: {
    new ({
      samprate,
      frame_length,
      mode,
      window,
      ratio,
    }: {
      samprate: number;
      frame_length?: number;
      mode?: number;
      window?: number;
      ratio?: number;
    }): Endpointer;
  };
}
declare const createModule: EmscriptenModuleFactory<SoundSwallowerModule>;
export default createModule;

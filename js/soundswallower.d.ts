/// <reference types="emscripten" />
export class Config implements Iterable<string> {
    delete(): void;
    set(key: string, val: string|number): boolean;
    get(key: string): string|number;
    model_file_path(key: string, modelfile: string): string;
    has(key: string): boolean;
    [Symbol.iterator](): IterableIterator<string>;
}
export class Decoder {
    delete(): void;
    initialize(config?: Config|Object): Promise<any>;
    reinitialize_audio(): Promise<void>;
    start(): Promise<void>;
    stop(): Promise<void>;
    process(pcm: Float32Array|Uint8Array, no_search:boolean, full_utt:boolean): Promise<number>;
    get_hyp(): string;
    get_hypseg(): Array<Segment>;
    lookup_word(word: string): string;
    add_word(word:string, pron: string, update?: boolean): Promise<number>;
    create_fsg(name:string, start_state: number, final_state: number,
               transitions: Array<Transition>): Grammar;
    parse_jsgf(jsgf_string:string, toprule?: string): Grammar;
    set_fsg(fsg: Grammar): Promise<void>;
}
export interface Grammar {
    delete(): void;
}
export interface Transition {
    from: number;
    to: number;
    prob?: number;
    word?: string;
}
export interface Segment {
    start: number;
    end: number;
    word: string;
}
export interface SoundSwallowerModule extends EmscriptenModule {
    get_model_path(string): string;
    Config: {
        /* Everything else is declared above.. */
        new(dict?: Object): Config;
    }
    Decoder: {
        new(config?: Config|Object): Decoder;
    }
}
declare const Module: EmscriptenModuleFactory<SoundSwallowerModule>;
export default Module;

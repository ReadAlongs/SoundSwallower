/// <reference types="emscripten" />
declare class Config implements Iterable<string> {
    delete(): void;
    set(key: string, val: string|number): boolean;
    get(key: string): string|number;
    model_file_path(key: string, modelfile: string): string;
    has(key: string): boolean;
    [Symbol.iterator](): IterableIterator<any>;
}
declare class Decoder {
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
               transitions: Array<Transition>): FSG;
    parse_jsgf(jsgf_string:string, toprule?: string): FSG;
    set_fsg(fsg: FSG): Promise<void>;
}
interface FSG {
    fsg: any;
    delete(): void;
}
interface Transition {
    from: number;
    to: number;
    prob?: number;
    word?: string;
}
interface Segment {
    start: number;
    end: number;
    word: string;
}
interface SoundSwallowerModule extends EmscriptenModule {
    get_model_path(string): string;
    Config: {
        /* Everything else is declared above.. */
        new(dict?: Object): Config;
    }
    Decoder: {
        new(config?: Config|Object): Decoder;
    }
}
declare const soundswallower_factory: EmscriptenModuleFactory<SoundSwallowerModule>;
export = soundswallower_factory;

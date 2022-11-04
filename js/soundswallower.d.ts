/// <reference types="emscripten" />
export class Decoder {
    initialized: boolean;
    delete(): void;
    get_config_json(): string;
    set_config(key: string, val: string|number): boolean;
    unset_config(key: string): void;
    get_config(key: string): string|number;
    has_config(key: string): boolean;
    initialize(): Promise<any>;
    reinitialize_audio(): Promise<void>;
    start(): Promise<void>;
    stop(): Promise<void>;
    process(pcm: Float32Array|Uint8Array, no_search: boolean, full_utt: boolean): Promise<number>;
    get_hyp(): string;
    get_hypseg(): Array<Segment>;
    get_alignment_json(start?: number, align_level?: number): Promise<string>;
    lookup_word(word: string): string;
    add_word(word: string, pron: string, update?: boolean): Promise<number>;
    set_fsg(name: string, start_state: number, final_state: number,
            transitions: Array<Transition>): Promise<void>;
    set_jsgf(jsgf_string: string, toprule?: string): Promise<void>;
    set_align_text(text: string): Promise<void>;
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
    get_model_path(subpath: string): string;
    Decoder: {
        new(config?: Object): Decoder;
    }
}
declare const Module: EmscriptenModuleFactory<SoundSwallowerModule>;
export default Module;

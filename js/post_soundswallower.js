// Preamble to SoundSwallower JS library, exports some C functions
// that don't need any wrappers and aren't defined at the moment where
// pre_soundswallower.js runs.
Module.ps_init = _ps_init;
Module.ps_start_utt = _ps_start_utt;
Module.ps_end_utt = _ps_end_utt;
Module.ps_free = _ps_free;
Module.cmd_ln_free = _cmd_ln_free_r;
Module.fsg_model_free = _fsg_model_free;

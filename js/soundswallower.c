/**
 * soundswallower.c: Extra C code specific to SoundSwallower in JavaScript
 *
 * C:  -rw-rw-r-- 1 dhd dhd  80726 avr 26 21:13 soundswallower.js
 * -rwxrwxr-x 1 dhd dhd 204808 avr 26 21:13 soundswallower.wasm
 *
 * C++: -rw-rw-r-- 1 dhd dhd 119254 avr 26 09:34 soundswallower.js
 * -rwxrwxr-x 1 dhd dhd 231829 avr 26 09:34 soundswallower.wasm
 *
 */

#include <emscripten.h>
#include <soundswallower/fsg_model.h>

EMSCRIPTEN_KEEPALIVE void
fsg_set_states(fsg_model_t *fsg, int start_state, int final_state)
{
  fsg->start_state = start_state;
  fsg->final_state = final_state;
}

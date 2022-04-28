/**
 * soundswallower.c: Extra C code specific to SoundSwallower in JavaScript
 */

#include <emscripten.h>
#include <soundswallower/fsg_model.h>

EMSCRIPTEN_KEEPALIVE void
fsg_set_states(fsg_model_t *fsg, int start_state, int final_state)
{
  fsg->start_state = start_state;
  fsg->final_state = final_state;
}

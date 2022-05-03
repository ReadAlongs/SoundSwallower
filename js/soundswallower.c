/**
 * soundswallower.c: Extra C code specific to SoundSwallower in JavaScript
 */

#include <emscripten.h>
#include <soundswallower/fsg_model.h>
#include <soundswallower/cmd_ln.h>
#include <soundswallower/hash_table.h>

EMSCRIPTEN_KEEPALIVE void
fsg_set_states(fsg_model_t *fsg, int start_state, int final_state)
{
  fsg->start_state = start_state;
  fsg->final_state = final_state;
}

EMSCRIPTEN_KEEPALIVE hash_iter_t *
cmd_ln_hash_iter(cmd_ln_t *cmd_ln)
{
  if (cmd_ln == NULL || cmd_ln->ht == NULL)
    return NULL;
  return hash_table_iter(cmd_ln->ht);
}

EMSCRIPTEN_KEEPALIVE char const *
hash_iter_key(hash_iter_t *itor)
{
  if (itor == NULL || itor->ent == NULL)
    return NULL;
  return hash_entry_key(itor->ent);
}

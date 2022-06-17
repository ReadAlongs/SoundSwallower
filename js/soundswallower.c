/* -*- c-basic-offset: 4; c-file-style: "bsd" -*- */
/**
 * soundswallower.c: Extra C code specific to SoundSwallower in JavaScript
 */

#include <emscripten.h>
#include <soundswallower/fsg_model.h>
#include <soundswallower/cmd_ln.h>
#include <soundswallower/hash_table.h>
#include <soundswallower/pocketsphinx_internal.h>
#include <soundswallower/acmod.h>

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

EMSCRIPTEN_KEEPALIVE bin_mdef_t *
create_mdef_from_blob(ps_decoder_t *ps, char *data, size_t len)
{
    char *ptr;
    E_INFO("Reading binary model definition from %ld bytes at %p\n", len, data);
    
    return NULL;
}

EMSCRIPTEN_KEEPALIVE tmat_t *
create_tmat_from_blob(ps_decoder_t *ps, char *data, size_t len)
{
    return NULL;
}

EMSCRIPTEN_KEEPALIVE tmat_t *
create_mgau_from_blobs(ps_decoder_t *ps,
		       char *means_data, size_t means_len,
		       char *variances_data, size_t variances_len,
		       char *sendump_data, size_t sendump_len)
{
    return NULL;
}

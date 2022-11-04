/* -*- c-basic-offset: 4; c-file-style: "bsd" -*- */
/**
 * soundswallower.c: Extra C code specific to SoundSwallower in JavaScript
 */

#include <emscripten.h>
#include <soundswallower/fsg_model.h>
#include <soundswallower/configuration.h>
#include <soundswallower/hash_table.h>

#include <soundswallower/acmod.h>
#include <soundswallower/bin_mdef.h>
#include <soundswallower/tmat.h>
#include <soundswallower/ptm_mgau.h>
#include <soundswallower/s2_semi_mgau.h>
#include <soundswallower/ms_mgau.h>
#include <soundswallower/decoder.h>

EMSCRIPTEN_KEEPALIVE void
fsg_set_states(fsg_model_t *fsg, int start_state, int final_state)
{
    fsg->start_state = start_state;
    fsg->final_state = final_state;
}

EMSCRIPTEN_KEEPALIVE hash_iter_t *
cmd_ln_hash_iter(config_t *cmd_ln)
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

EMSCRIPTEN_KEEPALIVE void
set_mdef(decoder_t *ps, bin_mdef_t *mdef)
{
    acmod_t *acmod = ps->acmod;
    acmod->mdef = mdef;
}

EMSCRIPTEN_KEEPALIVE void
set_tmat(decoder_t *ps, tmat_t *tmat)
{
    acmod_t *acmod = ps->acmod;
    acmod->tmat = tmat;
}

EMSCRIPTEN_KEEPALIVE int
load_gmm(decoder_t *ps, s3file_t *means, s3file_t *vars, s3file_t *mixw, s3file_t *sendump)
{
    acmod_t *acmod = ps->acmod;

    E_INFO("Attempting to use PTM computation module\n");
    if ((acmod->mgau = ptm_mgau_init_s3file(acmod, means, vars, mixw, sendump)) == NULL) {
	E_INFO("Attempting to use semi-continuous computation module\n");
	s3file_rewind(means);
	s3file_rewind(vars);
	s3file_rewind(mixw);
	s3file_rewind(sendump);
	if ((acmod->mgau = s2_semi_mgau_init_s3file(acmod, means, vars, mixw, sendump)) == NULL) {
	    E_INFO("Falling back to general multi-stream GMM computation\n");
	    s3file_rewind(means);
	    s3file_rewind(vars);
	    s3file_rewind(mixw);
	    s3file_rewind(sendump);
	    acmod->mgau = ms_mgau_init_s3file(acmod, means, vars, mixw, NULL);
	    if (acmod->mgau == NULL) {
		E_ERROR("Failed to read acoustic model\n");
		return -1;
	    }
	}
    }

    /* FIXME: Apply MLLR as well... */
    return 0;
}

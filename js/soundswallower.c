/* -*- c-basic-offset: 4; c-file-style: "bsd" -*- */
/**
 * soundswallower.c: Extra C code specific to SoundSwallower in JavaScript
 */

#include <emscripten.h>
#include <soundswallower/configuration.h>
#include <soundswallower/fsg_model.h>
#include <soundswallower/hash_table.h>

#include <soundswallower/acmod.h>
#include <soundswallower/bin_mdef.h>
#include <soundswallower/decoder.h>
#include <soundswallower/ms_mgau.h>
#include <soundswallower/ptm_mgau.h>
#include <soundswallower/s2_semi_mgau.h>
#include <soundswallower/tmat.h>

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

EMSCRIPTEN_KEEPALIVE float32 **
spectrogram(fe_t *fe, float32 *pcm, size_t nsamp, size_t *nfr, size_t *nfeat, int smooth)
{
    config_t *config;
    float32 **spec;
    int rv, prev_spec, prev_ncep;

    config = fe_get_config(fe);
    assert(config != NULL);
    if (nfeat == NULL || nfr == NULL || pcm == NULL)
        return NULL;
    *nfeat = config_int(config, "nfilt");
    *nfr = fe_process_float32(fe, NULL, &nsamp, NULL, 0);
    /* HACK! Until we fix the stupid API */
    prev_spec = fe->log_spec;
    prev_ncep = fe->feature_dimension;
    fe->log_spec = smooth ? SMOOTH_LOG_SPEC : RAW_LOG_SPEC;
    fe->feature_dimension = *nfeat;

    spec = ckd_calloc_2d(*nfr, *nfeat, 4);
    rv = fe_process_float32(fe, &pcm, &nsamp, spec, *nfr);
    fe_end(fe, spec + rv, *nfr - rv);

    fe->log_spec = prev_spec;
    fe->feature_dimension = prev_ncep;
    return spec;
}

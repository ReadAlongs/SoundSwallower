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
#include <soundswallower/byteorder.h>

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

int32
get_int32(char **ptr, size_t *len, int swap)
{
    int32 val;
    if (*len < 4) {
        E_ERROR("4 bytes not available at %p\n", *ptr);
	/* FIXME: will be a macro that can handle this */
    }
    val = *(int32 *)*ptr;
    *ptr += 4;
    *len -= 4;
    if (swap)
        SWAP_INT32(&val);
    return val;
}

EMSCRIPTEN_KEEPALIVE bin_mdef_t *
create_mdef_from_blob(ps_decoder_t *ps, char *data, size_t len)
{
    char *ptr = data;
    size_t avail = len;
    bin_mdef_t *m;
    int32 val;
    int swap, i;
    size_t tree_start;
    int32 *sseq_size;

    E_INFO("Reading binary model definition from %ld bytes at %p\n", len, data);
    val = get_int32(&ptr, &avail, FALSE);
    swap = 0;
    if (val == BIN_MDEF_OTHER_ENDIAN) {
        swap = 1;
        E_INFO("Must byte-swap mdef\n");
    }
    val = get_int32(&ptr, &avail, swap);
    if (val > BIN_MDEF_FORMAT_VERSION) {
        E_ERROR("File format version %d is newer than library\n", val);
        return NULL;
    }
    /* Length of format descriptor. */
    val = get_int32(&ptr, &avail, swap);
    E_INFO("Format descriptor is %d bytes\n", val);
    if (avail < (size_t)val) {
	E_ERROR("File truncated in format descriptor!");
	return NULL;
    }
    ptr += val;
    /* Finally allocate it. */
    m = ckd_calloc(1, sizeof(*m));
    m->refcnt = 1;

    m->n_ciphone = get_int32(&ptr, &avail, swap);
    m->n_phone = get_int32(&ptr, &avail, swap);
    m->n_emit_state = get_int32(&ptr, &avail, swap);
    m->n_ci_sen = get_int32(&ptr, &avail, swap);
    m->n_sen = get_int32(&ptr, &avail, swap);
    m->n_tmat = get_int32(&ptr, &avail, swap);
    m->n_sseq = get_int32(&ptr, &avail, swap);
    m->n_ctx = get_int32(&ptr, &avail, swap);
    m->n_cd_tree = get_int32(&ptr, &avail, swap);
    m->sil = get_int32(&ptr, &avail, swap);
    
    /* CI names are first in the file. */
    m->ciname = ckd_calloc(m->n_ciphone, sizeof(*m->ciname));

    /* Set the base pointer. */
    m->ciname[0] = ptr;
    /* Success! */
    m->alloc_mode = BIN_MDEF_IN_MEMORY;

    for (i = 1; i < m->n_ciphone; ++i) {
        m->ciname[i] = m->ciname[i - 1] + strlen(m->ciname[i - 1]) + 1;
    }

    /* Skip past the padding. */
    tree_start =
        m->ciname[i - 1] + strlen(m->ciname[i - 1]) + 1 - m->ciname[0];
    tree_start = (tree_start + 3) & ~3;
    m->cd_tree = (cd_tree_t *) (m->ciname[0] + tree_start);
    if (swap) {
        for (i = 0; i < m->n_cd_tree; ++i) {
            SWAP_INT16(&m->cd_tree[i].ctx);
            SWAP_INT16(&m->cd_tree[i].n_down);
            SWAP_INT32(&m->cd_tree[i].c.down);
        }
    }
    m->phone = (mdef_entry_t *) (m->cd_tree + m->n_cd_tree);
    if (swap) {
        for (i = 0; i < m->n_phone; ++i) {
            SWAP_INT32(&m->phone[i].ssid);
            SWAP_INT32(&m->phone[i].tmat);
        }
    }
    sseq_size = (int32 *) (m->phone + m->n_phone);
    if (swap)
        SWAP_INT32(sseq_size);
    m->sseq = ckd_calloc(m->n_sseq, sizeof(*m->sseq));
    m->sseq[0] = (uint16 *) (sseq_size + 1);
    if (swap) {
        for (i = 0; i < *sseq_size; ++i)
            SWAP_INT16(m->sseq[0] + i);
    }
    if (m->n_emit_state) {
        for (i = 1; i < m->n_sseq; ++i)
            m->sseq[i] = m->sseq[0] + i * m->n_emit_state;
    }
    else {
        m->sseq_len = (uint8 *) (m->sseq[0] + *sseq_size);
        for (i = 1; i < m->n_sseq; ++i)
            m->sseq[i] = m->sseq[i - 1] + m->sseq_len[i - 1];
    }

    /* Now build the CD-to-CI mappings using the senone sequences.
     * This is the only really accurate way to do it, though it is
     * still inaccurate in the case of heterogeneous topologies or
     * cross-state tying. */
    m->cd2cisen = (int16 *) ckd_malloc(m->n_sen * sizeof(*m->cd2cisen));
    m->sen2cimap = (int16 *) ckd_malloc(m->n_sen * sizeof(*m->sen2cimap));

    /* Default mappings (identity, none) */
    for (i = 0; i < m->n_ci_sen; ++i)
        m->cd2cisen[i] = i;
    for (; i < m->n_sen; ++i)
        m->cd2cisen[i] = -1;
    for (i = 0; i < m->n_sen; ++i)
        m->sen2cimap[i] = -1;
    for (i = 0; i < m->n_phone; ++i) {
        int32 j, ssid = m->phone[i].ssid;

        for (j = 0; j < bin_mdef_n_emit_state_phone(m, i); ++j) {
            int s = bin_mdef_sseq2sen(m, ssid, j);
            int ci = bin_mdef_pid2ci(m, i);
            /* Take the first one and warn if we have cross-state tying. */
            if (m->sen2cimap[s] == -1)
                m->sen2cimap[s] = ci;
            if (m->sen2cimap[s] != ci)
                E_WARN
                    ("Senone %d is shared between multiple base phones\n",
                     s);

            if (j > bin_mdef_n_emit_state_phone(m, ci))
                E_WARN("CD phone %d has fewer states than CI phone %d\n",
                       i, ci);
            else
                m->cd2cisen[s] =
                    bin_mdef_sseq2sen(m, m->phone[ci].ssid, j);
        }
    }

    /* Set the silence phone. */
    m->sil = bin_mdef_ciphone_id(m, S3_SILENCE_CIPHONE);

    E_INFO
        ("%d CI-phone, %d CD-phone, %d emitstate/phone, %d CI-sen, %d Sen, %d Sen-Seq\n",
         m->n_ciphone, m->n_phone - m->n_ciphone, m->n_emit_state,
         m->n_ci_sen, m->n_sen, m->n_sseq);
    return m;
}

EMSCRIPTEN_KEEPALIVE tmat_t *
create_tmat_from_blob(ps_decoder_t *ps, char *data, size_t len)
{
    char *ptr;
    E_INFO("Reading binary transition matrix from %ld bytes at %p\n", len, data);

    return NULL;
}

EMSCRIPTEN_KEEPALIVE tmat_t *
create_mgau_from_blobs(ps_decoder_t *ps,
		       char *means_data, size_t means_len,
		       char *variances_data, size_t variances_len,
		       char *sendump_data, size_t sendump_len)
{
    char *ptr;
    E_INFO("Reading Gaussian means from %ld bytes at %p\n", means_len, means_data);
    E_INFO("Reading Gaussian variances from %ld bytes at %p\n", variances_len, variances_data);
    E_INFO("Reading mixture weights from %ld bytes at %p\n", sendump_len, sendump_data);
    return NULL;
}

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <soundswallower/decoder.h>

#include <soundswallower/ptm_mgau.h>
#include <soundswallower/ms_mgau.h>

#include "test_macros.h"

static const mfcc_t cmninit[13] = {
	FLOAT2MFCC(41.00),
	FLOAT2MFCC(-5.29),
	FLOAT2MFCC(-0.12),
	FLOAT2MFCC(5.09),
	FLOAT2MFCC(2.48),
	FLOAT2MFCC(-4.07),
	FLOAT2MFCC(-1.37),
	FLOAT2MFCC(-1.78),
	FLOAT2MFCC(-5.08),
	FLOAT2MFCC(-2.05),
	FLOAT2MFCC(-6.45),
	FLOAT2MFCC(-1.42),
	FLOAT2MFCC(1.17)
};

void
run_acmod_test(acmod_t *acmod)
{
	FILE *rawfh;
	int16 *buf;
	int16 const *bptr;
	size_t nread, nsamps;
	int nfr;
	int frame_counter;

	cmn_live_set(acmod->fcb->cmn_struct, cmninit);
	nsamps = 2048;
	frame_counter = 0;
	buf = ckd_calloc(nsamps, sizeof(*buf));
	TEST_ASSERT(rawfh = fopen(TESTDATADIR "/goforward.raw", "rb"));
	TEST_EQUAL(0, acmod_start_utt(acmod));
	printf("Incremental(2048):\n");
	while (!feof(rawfh)) {
		nread = fread(buf, sizeof(*buf), nsamps, rawfh);
		bptr = buf;
		while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
			int16 best_score;
			int frame_idx = -1, best_senid;
			while (acmod->n_feat_frame > 0) {
				acmod_score(acmod, &frame_idx);
				acmod_advance(acmod);
				best_score = acmod_best_score(acmod, &best_senid);
				printf("Frame %d best senone %d score %d\n",
				       frame_idx, best_senid, best_score);
				TEST_EQUAL(frame_counter, frame_idx);
				++frame_counter;
				frame_idx = -1;
			}
		}
	}
	/* Match pocketsphinx-0.7 as we do not remove silence anymore */
	TEST_EQUAL(1, acmod_end_utt(acmod));
	nread = 0;
	{
		int16 best_score;
		int frame_idx = -1, best_senid;
		while (acmod->n_feat_frame > 0) {
			acmod_score(acmod, &frame_idx);
			acmod_advance(acmod);
			best_score = acmod_best_score(acmod, &best_senid);
			printf("Frame %d best senone %d score %d\n",
			       frame_idx, best_senid, best_score);
			TEST_EQUAL(frame_counter, frame_idx);
			++frame_counter;
			frame_idx = -1;
		}
	}
	fclose(rawfh);
	ckd_free(buf);
}

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	config_t *config;
	acmod_t *acmod;
	fe_t *fe;
	feat_t *fcb;
	mgau_t *ps;
	ptm_mgau_t *s;
	int i, lastcb;

	(void)argc; (void)argv;
	lmath = logmath_init(1.0001, 0, 0);
	config = config_init(NULL);
        config_set_str(config, "compallsen", "yes");
        config_set_str(config, "input_endian", "little");
        config_expand(config);
	
	TEST_ASSERT(config);
	fe = fe_init(config);
	fcb = feat_init(config);
	TEST_ASSERT((acmod = acmod_init(config, lmath, fe, fcb)));
	TEST_ASSERT((ps = acmod->mgau));
	TEST_EQUAL(0, strcmp(ps->vt->name, "ptm"));
	s = (ptm_mgau_t *)ps;
	E_DEBUG("PTM model loaded: %d codebooks, %d senones, %d frames of history\n",
                s->g->n_mgau, s->n_sen, s->n_fast_hist);
        E_DEBUG("Senone to codebook mappings:\n");
        lastcb = s->sen2cb[0];
        E_DEBUG("\t%d: 0", lastcb);
        for (i = 0; i < s->n_sen; ++i) {
            if (s->sen2cb[i] != lastcb) {
                lastcb = s->sen2cb[i];
                E_DEBUG("-%d\n", i-1);
                E_DEBUG("\t%d: %d", lastcb, i);
            }
        }
        E_INFOCONT("-%d\n", i-1);
        run_acmod_test(acmod);

#if 0
/* Replace it with ms_mgau. */
        ptm_mgau_free(ps);
        config_set_str(config,
                       "-mixw",
                       MODELDIR "/en-us/mixture_weights");
	TEST_ASSERT((acmod->mgau = ms_mgau_init(acmod, lmath, acmod->mdef)));
	run_acmod_test(acmod);
#endif
	acmod_free(acmod);
	fe_free(fe);
	feat_free(fcb);
	logmath_free(lmath);
	config_free(config);
	return 0;
}

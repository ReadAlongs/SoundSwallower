#include "config.h"

#include <stdio.h>
#include <string.h>

#include <soundswallower/pocketsphinx.h>
#include <soundswallower/bin_mdef.h>
#include <soundswallower/dict.h>
#include <soundswallower/pio.h>
#include <soundswallower/strfuncs.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	bin_mdef_t *mdef;
	dict_t *dict;
	cmd_ln_t *config;
	FILE *fh, *fh_orig;
	lineiter_t *li, *li_orig;

	int i;
	char *space;
	char buf[100];

	TEST_ASSERT(config = cmd_ln_init(NULL, NULL, FALSE,
						   "-dict", MODELDIR "/en-us/dict.txt",
						   "_fdict", MODELDIR "/en-us/noisedict",
						   NULL));

	/* Test dictionary in standard fashion. */
	TEST_ASSERT(mdef = bin_mdef_read(NULL, MODELDIR "/en-us/mdef"));
	TEST_ASSERT(dict = dict_init(config, mdef));

	printf("Word ID (CARNEGIE) = %d\n",
	       dict_wordid(dict, "CARNEGIE"));
	printf("Word ID (ASDFASFASSD) = %d\n",
	       dict_wordid(dict, "ASDFASFASSD"));

	TEST_EQUAL(0, dict_write(dict, "_cmu07a.dic", NULL));
	fh_orig = fopen(MODELDIR "/en-us/dict.txt", "r");
	TEST_ASSERT(fh_orig != NULL);
	fh = fopen("_cmu07a.dic", "r");
	TEST_ASSERT(fh != NULL);
	li = lineiter_start(fh);
	li_orig = lineiter_start(fh_orig);
	while (li && li_orig) {
	  while (strlen(li->buf) == 0 || 0 == strncmp(li->buf, ";;", 2))
	    li = lineiter_next(li);
	  while (strlen(li_orig->buf) == 0 || 0 == strncmp(li_orig->buf, ";;", 2))
	    li_orig = lineiter_next(li_orig);
	  string_trim(li->buf, STRING_BOTH);
	  string_trim(li_orig->buf, STRING_BOTH);
	  space = strchr(li->buf, ' ');
	  string_trim(space + 1, STRING_START);
	  space = strchr(li_orig->buf, ' ');
	  string_trim(space + 1, STRING_START);
	  printf("%s\n%s\n", li->buf, li_orig->buf);
	  TEST_EQUAL_STRING(li->buf, li_orig->buf);
	  li = lineiter_next(li);
	  li_orig = lineiter_next(li);
	}
	  
	dict_free(dict);
	bin_mdef_free(mdef);

	/* Now test an empty dictionary. */
	TEST_ASSERT(dict = dict_init(NULL, NULL));
	printf("Word ID(<s>) = %d\n", dict_wordid(dict, "<s>"));
	TEST_ASSERT(BAD_S3WID != dict_add_word(dict, "FOOBIE", NULL, 0));
	TEST_ASSERT(BAD_S3WID != dict_add_word(dict, "BLETCH", NULL, 0));
	printf("Word ID(FOOBIE) = %d\n", dict_wordid(dict, "FOOBIE"));
	printf("Word ID(BLETCH) = %d\n", dict_wordid(dict, "BLETCH"));
	TEST_ASSERT(dict_real_word(dict, dict_wordid(dict, "FOOBIE")));
	TEST_ASSERT(dict_real_word(dict, dict_wordid(dict, "BLETCH")));
	TEST_ASSERT(!dict_real_word(dict, dict_wordid(dict, "</s>")));
	dict_free(dict);

	/* Test to add 500k words. */
	TEST_ASSERT(dict = dict_init(NULL, NULL));
	for (i = 0; i < 5000; i++) {
	    sprintf(buf, "word_%d", i);
    	    TEST_ASSERT(BAD_S3WID != dict_add_word(dict, buf, NULL, 0));
	}
	dict_free(dict);

	cmd_ln_free_r(config);

	return 0;
}

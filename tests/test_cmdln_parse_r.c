/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <soundswallower/configuration.h>
#include <soundswallower/ckd_alloc.h>

const config_param_t defs[] = {
    { "-a", ARG_INTEGER, "42", "This is the first argument." },
    { "-b", ARG_STRING, NULL, "This is the second argument." },
    { "-c", ARG_BOOLEAN, "no", "This is the third argument." },
    { "-d", ARG_FLOATING, "1e-50", "This is the fourth argument." },
    { NULL, 0, NULL, NULL }
};

int
main(int argc, char *argv[])
{
    config_t *config;

    config = config_parse(NULL, defs, argc, argv, TRUE);
    if (config == NULL)
        return 1;
    printf("%d %s %d %f\n",
           config_int(config, "a"),
           config_str(config, "b") ? config_str(config, "b") : "(null)",
           config_bool(config, "c"),
           config_float(config, "d"));
    config_free(config);

    config = cmd_ln_init(NULL, NULL, FALSE,
                         "-b", "foobie", NULL);
    if (config == NULL)
        return 1;
    config_free(config);

    config = cmd_ln_init(NULL, defs, TRUE,
                         "-b", "foobie", NULL);
    if (config == NULL)
        return 1;
    printf("%d %s %d %f\n",
           config_int(config, "a"),
           config_str(config, "b") ? config_str(config, "b") : "(null)",
           config_bool(config, "c"),
           config_float(config, "d"));
    config_free(config);

    config = cmd_ln_init(NULL, NULL, FALSE,
                         "-b", "foobie", NULL);
    if (config == NULL)
        return 1;
    printf("%s\n",
           config_str(config, "b") ? config_str(config, "b") : "(null)");
    config_set_str(config, "b", "blatz");
    printf("%s\n",
           config_str(config, "b") ? config_str(config, "b") : "(null)");
    config_free(config);
           
    return 0;
}

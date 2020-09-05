#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cfgstore.h"
#include "xallocs.h"

struct cfg_store {
    config_value config_data[CFG_END];
};

void cfg_destroy(cfg_store *cfg) {
    xfree(cfg);
}

cfg_store *cfg_create(void) {
    cfg_store *cfg = xmalloc(sizeof *cfg);
    memset(cfg, 0, sizeof(cfg_store));
    return cfg;
}

config_value get_config_value(cfg_store *cfg, enum config_key key) {
    assert(cfg);
    assert(key < CFG_END);
    return cfg->config_data[key];
}

void set_config_value(cfg_store *cfg, enum config_key key, config_value val) {
    assert(cfg);
    assert(key < CFG_END);
    cfg->config_data[key] = val;
}

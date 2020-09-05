#ifndef _H_CFGSTORE_
#define _H_CFGSTORE_

#include <stddef.h>

/*Holds various configuration items used in the engine*/

enum config_key {
    WINDOW_WIDTH,
    WINDOW_HEIGHT,
    WINDOW_TITLE,
    DEBUG_VERBOSE,
    CFG_END     /*Must be last*/
};

/*Identify the different value_types in a config_value*/
enum config_type {
    CFG_CHAR_P,
    CFG_SIZE,
    CFG_REAL,
    CFG_BOOLEAN
};

typedef struct config_value {
    enum config_type value_type;
    union {
        char *char_p;
        size_t size;
        double real;
        int    boolean;
    } as;
} config_value;

/*Configuration storage handle*/
typedef struct cfg_store cfg_store;

config_value get_config_value(cfg_store *cfg, enum config_key key); /*Returns a config_value struct corresponding to ID key*/
void         set_config_value(cfg_store *cfg, enum config_key key, config_value val);

cfg_store *cfg_create(void);
void       cfg_destroy(cfg_store *cfg);

/*Convenience macros (could be used to implement type-checking of config_value)*/
#define CFG_GET_SIZE(cfg, key) get_config_value((cfg), (key)).as.size
#define CFG_GET_REAL(cfg, key) get_config_value((cfg), (key)).as.double
#define CFG_GET_CHAR_P(cfg, key) get_config_value((cfg), (key)).as.char_p
#define CFG_GET_BOOLEAN(cfg, key) get_config_value((cfg), (key)).as.boolean

#define CFG_SET_SIZE(cfg, key, val) set_config_value((cfg), (key), (struct config_value){.value_type = CFG_SIZE, .as.size = val})
#define CFG_SET_REAL(cfg, key, val) set_config_value((cfg), (key), (struct config_value){.value_type = CFG_REAL, .as.double = val})
#define CFG_SET_CHAR_P(cfg, key, val) set_config_value((cfg), (key), (struct config_value){.value_type = CFG_CHAR_P, .as.char_p = val})
#define CFG_SET_BOOLEAN(cfg, key, val) set_config_value((cfg), (key), (struct config_value){.value_type = CFG_BOOLEAN, .as.boolean = val})

#endif

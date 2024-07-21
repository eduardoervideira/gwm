#ifndef CONFIG_H
#define CONFIG_H

#include "structs.h"

// Window Manager options
/*#define WORKSPACE_LAYOUTS                               {STACKING_MODE, STACKING_MODE, MONOCLE_MODE}
#define MOD1_KEY                                        XCB_MOD_MASK_1
#define MOD4_KEY                                        XCB_MOD_MASK_4
*/


typedef void (*ConfigHandler)(void *config_field, char *value);

typedef struct {
    const char *key;
    ConfigHandler handler;
    void *field;
} ConfigMap;

extern ConfigMap config_handlers[];

int load_configuration(Config *config);
int parse_config_file(ConfigMap *config_handlers, const char *path);
int check_config_file(Config *config);

#endif
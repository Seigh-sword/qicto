#ifndef QICTO_CONFIG_H
#define QICTO_CONFIG_H

#include "qicto.h"

config_t* config_create(void);
void config_destroy(config_t* cfg);
int config_load(config_t* cfg, const char* path);
int config_save(const config_t* cfg, const char* path);
const char* config_get(const config_t* cfg, const char* key);
void config_set(config_t* cfg, const char* key, const char* value);
int config_load_builtin_defaults(config_t* cfg);

#endif

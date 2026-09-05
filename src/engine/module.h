#ifndef QICTO_MODULE_H
#define QICTO_MODULE_H

#include "qicto.h"

#define QICTO_MOD_MAGIC "QICTO_MOD"
#define QICTO_MOD_API_VERSION 1

typedef struct {
    void* handle;
    char name[QICTO_MAX_MOD_NAME];
    char path[QICTO_MAX_PATH_LEN];
    qicto_mod_api_t api;
    int ref_count;
} mod_entry_t;

mod_entry_t* mod_load(const char* path);
void mod_unload(mod_entry_t* entry);
int mod_init(mod_entry_t* entry, editor_t* ed);
void mod_cleanup(mod_entry_t* entry, editor_t* ed);
qkey_t mod_dispatch_key(mod_entry_t* entry, editor_t* ed, qkey_t key);
void mod_render(mod_entry_t* entry, editor_t* ed, void* ncp);
void mod_on_buffer_opened(mod_entry_t* entry, editor_t* ed, buffer_t* buf);
void mod_on_buffer_changed(mod_entry_t* entry, editor_t* ed, buffer_t* buf);
void mod_on_mode_change(mod_entry_t* entry, editor_t* ed, qicto_mode_t from, qicto_mode_t to);
void mod_on_config_reload(mod_entry_t* entry, editor_t* ed, config_t* cfg);
qicto_cmd_result_t mod_on_command(mod_entry_t* entry, editor_t* ed, const char* cmd, char** out);

struct mod_registry_t {
    mod_entry_t** entries;
    size_t count;
    size_t capacity;
};
typedef struct mod_registry_t mod_registry_t;

mod_registry_t* mod_registry_create(void);
void mod_registry_destroy(mod_registry_t* reg);
int mod_registry_add(mod_registry_t* reg, mod_entry_t* entry);
mod_entry_t* mod_registry_find(mod_registry_t* reg, const char* name);
void mod_registry_remove(mod_registry_t* reg, const char* name);
void mod_registry_init_all(mod_registry_t* reg, editor_t* ed);
void mod_registry_cleanup_all(mod_registry_t* reg, editor_t* ed);
qkey_t mod_registry_dispatch_key(mod_registry_t* reg, editor_t* ed, qkey_t key);
void mod_registry_render_all(mod_registry_t* reg, editor_t* ed, void* ncp);
void mod_registry_load_dir(mod_registry_t* reg, editor_t* ed, const char* dir);
void mod_registry_on_buffer_opened(mod_registry_t* reg, editor_t* ed, buffer_t* buf);
void mod_registry_on_buffer_changed(mod_registry_t* reg, editor_t* ed, buffer_t* buf);
void mod_registry_on_mode_change(mod_registry_t* reg, editor_t* ed, qicto_mode_t from, qicto_mode_t to);
qicto_cmd_result_t mod_registry_on_command(mod_registry_t* reg, editor_t* ed, const char* cmd, char** out);
void mod_builtins_register(editor_t* ed);

#endif

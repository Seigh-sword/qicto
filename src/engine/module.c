#include "module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <cwalk.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

mod_registry_t* mod_registry_create(void) {
    mod_registry_t* reg = calloc(1, sizeof(mod_registry_t));
    return reg;
}

void mod_registry_destroy(mod_registry_t* reg) {
    if (!reg) return;
    for (size_t i = 0; i < reg->count; i++) {
        mod_unload(reg->entries[i]);
    }
    free(reg->entries);
    free(reg);
}

int mod_registry_add(mod_registry_t* reg, mod_entry_t* entry) {
    if (!reg || !entry) return -1;
    if (reg->count >= reg->capacity) {
        size_t newcap = reg->capacity ? reg->capacity * 2 : 8;
        mod_entry_t** ne = realloc(reg->entries, newcap * sizeof(mod_entry_t*));
        if (!ne) return -1;
        reg->entries = ne;
        reg->capacity = newcap;
    }
    reg->entries[reg->count++] = entry;
    return 0;
}

mod_entry_t* mod_registry_find(mod_registry_t* reg, const char* name) {
    if (!reg || !name) return NULL;
    for (size_t i = 0; i < reg->count; i++) {
        if (strcmp(reg->entries[i]->name, name) == 0) {
            return reg->entries[i];
        }
    }
    return NULL;
}

void mod_registry_remove(mod_registry_t* reg, const char* name) {
    if (!reg || !name) return;
    for (size_t i = 0; i < reg->count; i++) {
        if (strcmp(reg->entries[i]->name, name) == 0) {
            mod_entry_t* entry = reg->entries[i];
            memmove(reg->entries + i, reg->entries + i + 1,
                    (reg->count - i - 1) * sizeof(mod_entry_t*));
            reg->count--;
            mod_unload(entry);
            break;
        }
    }
}

void mod_registry_init_all(mod_registry_t* reg, editor_t* ed) {
    if (!reg || !ed) return;
    for (size_t i = 0; i < reg->count; i++) {
        mod_init(reg->entries[i], ed);
    }
}

void mod_registry_cleanup_all(mod_registry_t* reg, editor_t* ed) {
    if (!reg || !ed) return;
    for (size_t i = 0; i < reg->count; i++) {
        mod_cleanup(reg->entries[i], ed);
    }
}

qkey_t mod_registry_dispatch_key(mod_registry_t* reg, editor_t* ed, qkey_t key) {
    if (!reg || !ed) return key;
    for (size_t i = 0; i < reg->count; i++) {
        if (reg->entries[i]->api.on_key) {
            key = reg->entries[i]->api.on_key(ed, key);
            if (key == 0) break;
        }
    }
    return key;
}

void mod_registry_render_all(mod_registry_t* reg, editor_t* ed, void* ncp) {
    if (!reg || !ed) return;
    for (size_t i = 0; i < reg->count; i++) {
        if (reg->entries[i]->api.on_render) {
            reg->entries[i]->api.on_render(ed, ncp);
        }
    }
}

void mod_registry_on_buffer_opened(mod_registry_t* reg, editor_t* ed, buffer_t* buf) {
    if (!reg || !ed || !buf) return;
    for (size_t i = 0; i < reg->count; i++) {
        if (reg->entries[i]->api.on_buffer_opened) {
            reg->entries[i]->api.on_buffer_opened(ed, buf);
        }
    }
}

void mod_registry_on_buffer_changed(mod_registry_t* reg, editor_t* ed, buffer_t* buf) {
    if (!reg || !ed || !buf) return;
    for (size_t i = 0; i < reg->count; i++) {
        if (reg->entries[i]->api.on_buffer_changed) {
            reg->entries[i]->api.on_buffer_changed(ed, buf);
        }
    }
}

void mod_registry_on_mode_change(mod_registry_t* reg, editor_t* ed, qicto_mode_t from, qicto_mode_t to) {
    if (!reg || !ed) return;
    for (size_t i = 0; i < reg->count; i++) {
        if (reg->entries[i]->api.on_mode_change) {
            reg->entries[i]->api.on_mode_change(ed, from, to);
        }
    }
}

void mod_registry_on_config_reload(mod_registry_t* reg, editor_t* ed, config_t* cfg) {
    if (!reg || !ed || !cfg) return;
    for (size_t i = 0; i < reg->count; i++) {
        if (reg->entries[i]->api.on_config_reload) {
            reg->entries[i]->api.on_config_reload(ed, cfg);
        }
    }
}

qicto_cmd_result_t mod_registry_on_command(mod_registry_t* reg, editor_t* ed, const char* cmd, char** out) {
    if (!reg || !ed || !cmd) return QICTO_CMD_UNKNOWN;
    for (size_t i = 0; i < reg->count; i++) {
        if (reg->entries[i]->api.on_command) {
            qicto_cmd_result_t rc = reg->entries[i]->api.on_command(ed, cmd, out);
            if (rc != QICTO_CMD_UNKNOWN) return rc;
        }
    }
    return QICTO_CMD_UNKNOWN;
}

void mod_registry_load_dir(mod_registry_t* reg, editor_t* ed, const char* dir) {
    if (!reg || !dir || !ed) return;

#ifdef _WIN32
    char search_path[4096];
    snprintf(search_path, sizeof(search_path), "%s\\*.mod", dir);

    WIN32_FIND_DATAA fdata;
    HANDLE hFind = FindFirstFileA(search_path, &fdata);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            char full_path[4096];
            cwk_path_join(dir, fdata.cFileName, full_path, sizeof(full_path));
            mod_entry_t* entry = mod_load(full_path);
            if (entry) {
                mod_registry_add(reg, entry);
                mod_init(entry, ed);
            }
        }
    } while (FindNextFileA(hFind, &fdata));
    FindClose(hFind);
#else
    DIR* dirp = opendir(dir);
    if (!dirp) return;
    struct dirent* ent;
    while ((ent = readdir(dirp)) != NULL) {
        size_t len = strlen(ent->d_name);
        if (len > 4 && strcmp(ent->d_name + len - 4, ".mod") == 0) {
            char full_path[4096];
            cwk_path_join(dir, ent->d_name, full_path, sizeof(full_path));
            mod_entry_t* entry = mod_load(full_path);
            if (entry) {
                mod_registry_add(reg, entry);
                mod_init(entry, ed);
            }
        }
    }
    closedir(dirp);
#endif
}

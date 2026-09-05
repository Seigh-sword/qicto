#include "module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

mod_entry_t* mod_load(const char* path) {
    if (!path) return NULL;

    mod_entry_t* entry = calloc(1, sizeof(mod_entry_t));
    if (!entry) return NULL;

    strncpy(entry->path, path, QICTO_MAX_PATH_LEN - 1);

#ifdef _WIN32
    HMODULE handle = LoadLibraryA(path);
    if (!handle) {
        DWORD err = GetLastError();
        fprintf(stderr, "mod_load: failed to load %s (error %lu)\n", path, err);
        free(entry);
        return NULL;
    }
    entry->handle = handle;

    FARPROC get_api = GetProcAddress(handle, "qicto_mod_get_api");
    if (!get_api) {
        fprintf(stderr, "mod_load: no qicto_mod_get_api in %s\n", path);
        FreeLibrary(handle);
        free(entry);
        return NULL;
    }

    typedef const qicto_mod_api_t* (*get_api_fn)(void);
    const qicto_mod_api_t* api = ((get_api_fn)get_api)();
    if (!api) {
        fprintf(stderr, "mod_load: qicto_mod_get_api returned NULL for %s\n", path);
        FreeLibrary(handle);
        free(entry);
        return NULL;
    }
#else
    void* handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "mod_load: failed to load %s: %s\n", path, dlerror());
        free(entry);
        return NULL;
    }
    entry->handle = handle;

    void* sym = dlsym(handle, "qicto_mod_get_api");
    if (!sym) {
        fprintf(stderr, "mod_load: no qicto_mod_get_api in %s: %s\n", path, dlerror());
        dlclose(handle);
        free(entry);
        return NULL;
    }

    typedef const qicto_mod_api_t* (*get_api_fn)(void);
    const qicto_mod_api_t* api = ((get_api_fn)sym)();
    if (!api) {
        fprintf(stderr, "mod_load: qicto_mod_get_api returned NULL for %s\n", path);
        dlclose(handle);
        free(entry);
        return NULL;
    }
#endif

    memcpy(&entry->api, api, sizeof(qicto_mod_api_t));
    strncpy(entry->name, entry->api.name ? entry->api.name : "unknown", QICTO_MAX_MOD_NAME - 1);
    entry->ref_count = 1;
    return entry;
}

void mod_unload(mod_entry_t* entry) {
    if (!entry || !entry->handle) return;
    entry->ref_count--;
    if (entry->ref_count > 0) return;

#ifdef _WIN32
    FreeLibrary((HMODULE)entry->handle);
#else
    dlclose(entry->handle);
#endif
    entry->handle = NULL;
    free(entry);
}

int mod_init(mod_entry_t* entry, editor_t* ed) {
    if (!entry || !ed) return -1;
    if (entry->api.init) {
        entry->api.init(ed);
    }
    entry->ref_count++;
    return 0;
}

void mod_cleanup(mod_entry_t* entry, editor_t* ed) {
    if (!entry || !ed) return;
    if (entry->api.cleanup) {
        entry->api.cleanup(ed);
    }
    entry->ref_count--;
}

qkey_t mod_dispatch_key(mod_entry_t* entry, editor_t* ed, qkey_t key) {
    if (!entry || !ed || !entry->api.on_key) return key;
    return entry->api.on_key(ed, key);
}

void mod_render(mod_entry_t* entry, editor_t* ed, void* ncp) {
    if (!entry || !ed || !entry->api.on_render) return;
    entry->api.on_render(ed, ncp);
}

void mod_on_buffer_opened(mod_entry_t* entry, editor_t* ed, buffer_t* buf) {
    if (!entry || !ed || !buf || !entry->api.on_buffer_opened) return;
    entry->api.on_buffer_opened(ed, buf);
}

void mod_on_buffer_changed(mod_entry_t* entry, editor_t* ed, buffer_t* buf) {
    if (!entry || !ed || !buf || !entry->api.on_buffer_changed) return;
    entry->api.on_buffer_changed(ed, buf);
}

void mod_on_mode_change(mod_entry_t* entry, editor_t* ed, qicto_mode_t from, qicto_mode_t to) {
    if (!entry || !ed || !entry->api.on_mode_change) return;
    entry->api.on_mode_change(ed, from, to);
}

void mod_on_config_reload(mod_entry_t* entry, editor_t* ed, config_t* cfg) {
    if (!entry || !ed || !cfg || !entry->api.on_config_reload) return;
    entry->api.on_config_reload(ed, cfg);
}

qicto_cmd_result_t mod_on_command(mod_entry_t* entry, editor_t* ed, const char* cmd, char** out) {
    if (!entry || !ed || !cmd || !entry->api.on_command) {
        return QICTO_CMD_UNKNOWN;
    }
    return entry->api.on_command(ed, cmd, out);
}

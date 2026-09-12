#define _POSIX_C_SOURCE 200809L

#include "hotreload_mod.h"
#include "buffer.h"
#include "editor.h"
#include "module.h"
#include "command.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef _WIN32
#define MOD_EXT ".dll"
#else
#define MOD_EXT ".so"
#endif

#define QICTO_HR_MAX_MODS 64
#define POLL_INTERVAL_NS 1000000000L

typedef struct {
    char path[QICTO_MAX_PATH_LEN];
    time_t mtime;
    bool present;
} hr_tracked_t;

static hr_tracked_t s_tracked[QICTO_HR_MAX_MODS];
static size_t s_count = 0;
static struct timespec s_last_poll = {0, 0};

static time_t mtime_of(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return st.st_mtime;
}

static int find_tracked(const char* path) {
    for (size_t i = 0; i < s_count; i++) {
        if (strcmp(s_tracked[i].path, path) == 0) return (int)i;
    }
    return -1;
}

static void rescan(editor_t* ed, const char* dir) {
    if (!dir || !*dir) return;
    DIR* d = opendir(dir);
    if (!d) return;
    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        const char* dot = strrchr(ent->d_name, '.');
        if (!dot || strcmp(dot, MOD_EXT) != 0) continue;
        char path[QICTO_MAX_PATH_LEN];
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        time_t mt = mtime_of(path);
        if (mt == 0) continue;

        int idx = find_tracked(path);
        if (idx < 0) {
            if (s_count < QICTO_HR_MAX_MODS) {
                hr_tracked_t* t = &s_tracked[s_count++];
                snprintf(t->path, sizeof(t->path), "%s", path);
                t->mtime = mt;
                t->present = true;
                if (ed && ed->mods) {
                    mod_entry_t* m = mod_load(path);
                    if (m) {
                        mod_registry_add(ed->mods, m);
                        if (m->api.init) m->api.init(ed);
                        editor_set_status(ed, "loaded %s", ent->d_name);
                    }
                }
            }
        } else {
            if (mt > s_tracked[idx].mtime) {
                s_tracked[idx].mtime = mt;
                if (ed && ed->mods) {
                    mod_entry_t* existing = NULL;
                    for (size_t i = 0; i < ed->mods->count; i++) {
                        if (strcmp(ed->mods->entries[i]->path, path) == 0) {
                            existing = ed->mods->entries[i];
                            break;
                        }
                    }
                    if (existing) {
                        if (existing->api.cleanup) existing->api.cleanup(ed);
                        char modname[QICTO_MAX_MOD_NAME];
                        snprintf(modname, sizeof(modname), "%s", existing->name);
                        mod_registry_remove(ed->mods, modname);
                        mod_entry_t* fresh = mod_load(path);
                        if (fresh) {
                            mod_registry_add(ed->mods, fresh);
                            if (fresh->api.init) fresh->api.init(ed);
                            editor_set_status(ed, "reloaded %s", ent->d_name);
                        }
                    }
                }
            }
        }
    }
    closedir(d);
}

static void hr_tick(editor_t* ed) {
    if (!ed || !ed->config) return;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long elapsed = (now.tv_sec - s_last_poll.tv_sec) * 1000000000L
                 + (now.tv_nsec - s_last_poll.tv_nsec);
    if (s_last_poll.tv_sec != 0 && elapsed < POLL_INTERVAL_NS) return;
    s_last_poll = now;
    rescan(ed, ed->config->mods_dir);
}

qicto_cmd_result_t cmd_modreload(editor_t* ed, const char* args, char** out) {
    (void)args;
    if (!ed || !ed->config || !ed->config->mods_dir[0]) {
        if (out) *out = strdup("no mods_dir configured");
        return QICTO_CMD_ERROR;
    }
    rescan(ed, ed->config->mods_dir);
    if (out) *out = strdup("rescanned mods");
    return QICTO_CMD_SUCCESS;
}

static qicto_cmd_result_t hr_init(editor_t* ed) {
    if (!ed) return QICTO_CMD_ERROR;
    commands_register(ed->commands, "modreload", cmd_modreload, "Reload mods from mods_dir");
    if (ed->config && ed->config->mods_dir[0]) {
        rescan(ed, ed->config->mods_dir);
    }
    return QICTO_CMD_SUCCESS;
}

static void hr_cleanup(editor_t* ed) {
    (void)ed;
    s_count = 0;
}

static void hr_on_buffer_opened(editor_t* ed, buffer_t* buf) {
    (void)ed; (void)buf;
}
static void hr_on_buffer_changed(editor_t* ed, buffer_t* buf) {
    (void)ed; (void)buf;
}
static void hr_on_render(editor_t* ed, void* ncp) {
    (void)ed; (void)ncp;
}
static qicto_cmd_result_t hr_on_command(editor_t* ed, const char* cmd, char** out) {
    (void)ed; (void)cmd; (void)out;
    return QICTO_CMD_UNKNOWN;
}

static const char hr_name[] = "hotreload";
static const char hr_version[] = "0.1.0";

static qicto_mod_api_t s_hr_api = {
    .name = hr_name,
    .version = hr_version,
    .description = "Watch mods_dir and auto-reload .so/.dll on mtime change (1s polling)",
    .init = hr_init,
    .cleanup = hr_cleanup,
    .on_render = hr_on_render,
    .on_key = NULL,
    .on_buffer_opened = hr_on_buffer_opened,
    .on_buffer_changed = hr_on_buffer_changed,
    .on_mode_change = NULL,
    .on_config_reload = NULL,
    .on_command = hr_on_command,
};

const qicto_mod_api_t* hotreload_mod_get_api(void) {
    return &s_hr_api;
}

/* Entry point called from the main loop to poll for changes. */
void hotreload_tick(editor_t* ed) {
    hr_tick(ed);
}

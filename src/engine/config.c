#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <ini.h>

static int config_handler(void* user, const char* section, const char* name, const char* value) {
    config_t* cfg = (config_t*)user;
    char key[256];
    snprintf(key, sizeof(key), "%s:%s", section, name);
    config_set(cfg, key, value);
    return 1;
}

config_t* config_create(void) {
    config_t* cfg = calloc(1, sizeof(config_t));
    if (!cfg) return NULL;
    config_load_builtin_defaults(cfg);
    return cfg;
}

void config_destroy(config_t* cfg) {
    if (!cfg) return;
    if (cfg->settings) free(cfg->settings);
    free(cfg);
}

int config_load_builtin_defaults(config_t* cfg) {
    if (!cfg) return -1;
    memset(cfg, 0, sizeof(config_t));

    cfg->tab_width = 4;
    cfg->expand_tabs = true;
    cfg->show_line_numbers = true;
    cfg->show_whitespace = false;
    cfg->show_minimap = false;
    cfg->auto_indent = true;
    cfg->auto_complete = true;
    cfg->word_wrap = false;
    cfg->scroll_offset = 5;
    strncpy(cfg->font_family, "monospace", sizeof(cfg->font_family) - 1);
    cfg->font_size = 12;
    cfg->default_syntax = QICTO_SYNTAX_UNKNOWN;
    strncpy(cfg->theme, "default", sizeof(cfg->theme) - 1);
    strncpy(cfg->mods_dir, ".qicto/mods", sizeof(cfg->mods_dir) - 1);
    strncpy(cfg->keymap, "qicto", sizeof(cfg->keymap) - 1);
    strncpy(cfg->project_dir, ".", sizeof(cfg->project_dir) - 1);

    return 0;
}

int config_load(config_t* cfg, const char* path) {
    if (!cfg || !path) return -1;

    int rc = ini_parse(path, config_handler, cfg);
    if (rc == -1) return -1;
    if (rc == -2) {
        fprintf(stderr, "config_load: failed to parse %s\n", path);
        return -1;
    }

    const char* tv = config_get(cfg, "editor:tab_width");
    if (tv) cfg->tab_width = atoi(tv);
    const char* et = config_get(cfg, "editor:expand_tabs");
    if (et) cfg->expand_tabs = (strcasecmp(et, "true") == 0 || strcmp(et, "1") == 0);
    const char* ln = config_get(cfg, "editor:show_line_numbers");
    if (ln) cfg->show_line_numbers = (strcasecmp(ln, "true") == 0 || strcmp(ln, "1") == 0);
    const char* mw = config_get(cfg, "editor:show_minimap");
    if (mw) cfg->show_minimap = (strcasecmp(mw, "true") == 0 || strcmp(mw, "1") == 0);
    const char* ai = config_get(cfg, "editor:auto_indent");
    if (ai) cfg->auto_indent = (strcasecmp(ai, "true") == 0 || strcmp(ai, "1") == 0);
    const char* ww = config_get(cfg, "editor:word_wrap");
    if (ww) cfg->word_wrap = (strcasecmp(ww, "true") == 0 || strcmp(ww, "1") == 0);
    const char* so = config_get(cfg, "editor:scroll_offset");
    if (so) cfg->scroll_offset = atoi(so);
    const char* th = config_get(cfg, "editor:theme");
    if (th) strncpy(cfg->theme, th, sizeof(cfg->theme) - 1);
    const char* md = config_get(cfg, "editor:mods_dir");
    if (md) strncpy(cfg->mods_dir, md, sizeof(cfg->mods_dir) - 1);

    config_set(cfg, "config_file", path);
    return 0;
}

int config_save(const config_t* cfg, const char* path) {
    if (!cfg || !path) return -1;
    FILE* f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "[editor]\n");
    fprintf(f, "tab_width=%d\n", cfg->tab_width);
    fprintf(f, "expand_tabs=%s\n", cfg->expand_tabs ? "true" : "false");
    fprintf(f, "show_line_numbers=%s\n", cfg->show_line_numbers ? "true" : "false");
    fprintf(f, "show_minimap=%s\n", cfg->show_minimap ? "true" : "false");
    fprintf(f, "auto_indent=%s\n", cfg->auto_indent ? "true" : "false");
    fprintf(f, "word_wrap=%s\n", cfg->word_wrap ? "true" : "false");
    fprintf(f, "scroll_offset=%d\n", cfg->scroll_offset);
    fprintf(f, "theme=%s\n", cfg->theme);
    fprintf(f, "mods_dir=%s\n", cfg->mods_dir);
    fclose(f);
    return 0;
}

const char* config_get(const config_t* cfg, const char* key) {
    if (!cfg || !key || !cfg->settings) return NULL;
    for (size_t i = 0; i < cfg->count; i++) {
        if (strcmp(cfg->settings[i].key, key) == 0) {
            return cfg->settings[i].value;
        }
    }
    return NULL;
}

void config_set(config_t* cfg, const char* key, const char* value) {
    if (!cfg || !key || !value) return;

    for (size_t i = 0; i < cfg->count; i++) {
        if (strcmp(cfg->settings[i].key, key) == 0) {
            strncpy(cfg->settings[i].value, value, 255);
            cfg->settings[i].value[255] = '\0';
            return;
        }
    }

    if (cfg->count >= cfg->capacity) {
        size_t newcap = cfg->capacity ? cfg->capacity * 2 : 16;
        qicto_setting_t* ns = realloc(cfg->settings, newcap * sizeof(qicto_setting_t));
        if (!ns) return;
        cfg->settings = ns;
        cfg->capacity = newcap;
    }

    strncpy(cfg->settings[cfg->count].key, key, 31);
    cfg->settings[cfg->count].key[31] = '\0';
    strncpy(cfg->settings[cfg->count].value, value, 255);
    cfg->settings[cfg->count].value[255] = '\0';
    cfg->count++;
}

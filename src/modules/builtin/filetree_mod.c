#define _POSIX_C_SOURCE 200809L

#include "filetree_mod.h"
#include "buffer.h"
#include "editor.h"
#include "module.h"
#include "command.h"
#include "ui/layout.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#endif

#define QICTO_FT_MAX_ENTRIES 4096

typedef struct {
    char path[QICTO_MAX_PATH_LEN];
    char status[16];
    int is_dir;
} ft_entry_t;

static ft_entry_t s_entries[QICTO_FT_MAX_ENTRIES];
static size_t s_count = 0;
static char s_root[QICTO_MAX_PATH_LEN] = {0};

static int collect_recursive(const char* dir, const char* rel_prefix, int depth) {
    if (depth > 6) return 0;
    if (s_count >= QICTO_FT_MAX_ENTRIES) return 0;
    DIR* d = opendir(dir);
    if (!d) return 0;

    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            if (strcmp(ent->d_name, ".git") == 0) continue;
            if (strcmp(ent->d_name, ".qicto") == 0) continue;
        }
        char child[QICTO_MAX_PATH_LEN];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        char rel[QICTO_MAX_PATH_LEN];
        if (rel_prefix[0]) {
            snprintf(rel, sizeof(rel), "%s/%s", rel_prefix, ent->d_name);
        } else {
            snprintf(rel, sizeof(rel), "%s", ent->d_name);
        }

        struct stat st;
        bool is_dir = false;
        if (stat(child, &st) == 0) {
            is_dir = S_ISDIR(st.st_mode);
        }

        if (s_count < QICTO_FT_MAX_ENTRIES) {
            ft_entry_t* e = &s_entries[s_count++];
            snprintf(e->path, sizeof(e->path), "%s", rel);
            snprintf(e->status, sizeof(e->status), "%s", "");
            e->is_dir = is_dir ? 1 : 0;
        }

        if (is_dir) {
            collect_recursive(child, rel, depth + 1);
        }
        if (s_count >= QICTO_FT_MAX_ENTRIES) break;
    }
    closedir(d);
    return 0;
}

static void load_git_status(const char* root) {
    for (size_t i = 0; i < s_count; i++) {
        s_entries[i].status[0] = '\0';
    }
    char cmd[QICTO_MAX_PATH_LEN * 2];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "cd \"%s\" 2>nul && git status --porcelain 2>nul", root);
#else
    snprintf(cmd, sizeof(cmd), "cd '%s' >/dev/null 2>&1 && git status --porcelain 2>/dev/null", root);
#endif
    FILE* fp = popen(cmd, "r");
    if (!fp) return;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (strlen(line) < 4) continue;
        char code[3] = { line[0], line[1], '\0' };
        char* p = line + 3;
        char* end = p + strlen(p);
        while (end > p && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' '))
            end--;
        *end = '\0';
        char path[QICTO_MAX_PATH_LEN];
#ifdef _WIN32
        if (strncmp(p, "\"", 1) == 0 && strlen(p) >= 2 && p[strlen(p)-1] == '"') {
            snprintf(path, sizeof(path), "%.*s", (int)(strlen(p)-2), p+1);
        } else {
            snprintf(path, sizeof(path), "%s", p);
        }
#else
        snprintf(path, sizeof(path), "%s", p);
#endif
        for (size_t i = 0; i < s_count; i++) {
            if (strcmp(s_entries[i].path, path) == 0) {
                snprintf(s_entries[i].status, sizeof(s_entries[i].status),
                         "%s", code);
                break;
            }
        }
    }
    pclose(fp);
}

static void populate_buffer(editor_t* ed, buffer_t* buf) {
    buffer_clear_text(buf);
    for (size_t i = 0; i < s_count; i++) {
        char line[QICTO_MAX_PATH_LEN + 32];
        const char* tag = "  ";
        if (s_entries[i].status[0] == 'M' || s_entries[i].status[1] == 'M') tag = " M";
        else if (s_entries[i].status[0] == 'A' || s_entries[i].status[1] == 'A') tag = " A";
        else if (s_entries[i].status[0] == '?' || s_entries[i].status[1] == '?') tag = " ?";
        else if (s_entries[i].status[0] == 'D' || s_entries[i].status[1] == 'D') tag = " D";
        else if (s_entries[i].status[0] == 'R' || s_entries[i].status[1] == 'R') tag = " R";
        snprintf(line, sizeof(line), "%s%s%s\n",
                 s_entries[i].is_dir ? "[D] " : "    ",
                 tag,
                 s_entries[i].path);
        buffer_append_line(buf, line);
    }
    buf->syntax = QICTO_SYNTAX_UNKNOWN;
    buf->render_valid = false;
    buffer_update_render(buf);
    if (ed->mods) {
        mod_registry_on_buffer_changed(ed->mods, ed, buf);
    }
}

qicto_cmd_result_t cmd_tree(editor_t* ed, const char* args, char** out);

qicto_cmd_result_t cmd_tree(editor_t* ed, const char* args, char** out) {
    const char* root = args && *args ? args : (ed->config && ed->config->project_dir[0]
                                              ? ed->config->project_dir
                                              : NULL);
    if (!root || !*root) {
        if (out) *out = strdup("no project dir; pass :tree <path>");
        return QICTO_CMD_ERROR;
    }
    snprintf(s_root, sizeof(s_root), "%s", root);
    s_count = 0;
    collect_recursive(root, "", 0);
    load_git_status(root);

    buffer_t* buf = NULL;
    char tree_buf_name[QICTO_MAX_NAME_LEN];
    snprintf(tree_buf_name, sizeof(tree_buf_name), "[tree:%s]", root);
    buffer_t* existing = ed->buffers;
    while (existing) {
        if (strcmp(existing->display_name, tree_buf_name) == 0) {
            buf = existing;
            break;
        }
        existing = existing->next;
    }
    if (!buf) {
        buf = buffer_new(NULL);
        if (!buf) return QICTO_CMD_ERROR;
        snprintf(buf->display_name, sizeof(buf->display_name), "%s", tree_buf_name);
        snprintf(buf->filename, sizeof(buf->filename), "%s", "");
        buf->btype = QICTO_BUFTYPE_TEXT;
        buf->next = ed->buffers;
        if (ed->buffers) ed->buffers->prev = buf;
        ed->buffers = buf;
        ed->buffer_count++;
        if (ed->mods) mod_registry_on_buffer_opened(ed->mods, ed, buf);
    }
    populate_buffer(ed, buf);
    ed->current_buffer = buf;
    if (ed->layout.active) ed->layout.active->buffer = buf;
    if (out) *out = strdup("tree");
    return QICTO_CMD_SUCCESS;
}

static qicto_cmd_result_t ft_init(editor_t* ed) {
    (void)ed;
    commands_register(ed->commands, "tree", cmd_tree, "Show file tree (:tree [path])");
    return QICTO_CMD_SUCCESS;
}

static void ft_cleanup(editor_t* ed) {
    (void)ed;
    s_count = 0;
}

static void ft_on_buffer_opened(editor_t* ed, buffer_t* buf) {
    (void)ed;
    (void)buf;
}

static void ft_on_buffer_changed(editor_t* ed, buffer_t* buf) {
    (void)ed;
    (void)buf;
}

static void ft_on_render(editor_t* ed, void* ncp) {
    (void)ed;
    (void)ncp;
}

static qicto_cmd_result_t ft_on_command(editor_t* ed, const char* cmd, char** out) {
    (void)ed;
    (void)cmd;
    (void)out;
    return QICTO_CMD_UNKNOWN;
}

static const char ft_name[] = "filetree";
static const char ft_version[] = "0.2.0";

static qicto_mod_api_t s_ft_api = {
    .name = ft_name,
    .version = ft_version,
    .description = "Recursive directory listing with git status indicators (:tree [path])",
    .init = ft_init,
    .cleanup = ft_cleanup,
    .on_render = ft_on_render,
    .on_key = NULL,
    .on_buffer_opened = ft_on_buffer_opened,
    .on_buffer_changed = ft_on_buffer_changed,
    .on_mode_change = NULL,
    .on_config_reload = NULL,
    .on_command = ft_on_command,
};

const qicto_mod_api_t* filetree_mod_get_api(void) {
    return &s_ft_api;
}

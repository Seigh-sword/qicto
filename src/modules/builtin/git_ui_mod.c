#define _POSIX_C_SOURCE 200809L

#include "git_ui_mod.h"
#include "buffer.h"
#include "editor.h"
#include "module.h"
#include "command.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#endif

static char* capture_cmd(const char* cmd) {
    FILE* fp = popen(cmd, "r");
    if (!fp) return NULL;
    size_t cap = 4096, len = 0;
    char* out = malloc(cap);
    if (!out) { pclose(fp); return NULL; }
    char chunk[1024];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
        if (len + n + 1 > cap) {
            while (len + n + 1 > cap) cap *= 2;
            char* nn = realloc(out, cap);
            if (!nn) { free(out); pclose(fp); return NULL; }
            out = nn;
        }
        memcpy(out + len, chunk, n);
        len += n;
    }
    out[len] = '\0';
    pclose(fp);
    return out;
}

static char* capture_with_root(const char* root, const char* subcmd) {
    char cmd[QICTO_MAX_PATH_LEN * 2];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "cd \"%s\" 2>nul && git %s 2>nul", root, subcmd);
#else
    snprintf(cmd, sizeof(cmd), "cd '%s' >/dev/null 2>&1 && git %s 2>/dev/null", root, subcmd);
#endif
    return capture_cmd(cmd);
}

static buffer_t* open_scratch(editor_t* ed, const char* name) {
    buffer_t* existing = ed->buffers;
    while (existing) {
        if (strcmp(existing->display_name, name) == 0) {
            buffer_clear_text(existing);
            return existing;
        }
        existing = existing->next;
    }
    buffer_t* b = buffer_new(NULL);
    if (!b) return NULL;
    snprintf(b->display_name, sizeof(b->display_name), "%s", name);
    b->btype = QICTO_BUFTYPE_TEXT;
    b->next = ed->buffers;
    if (ed->buffers) ed->buffers->prev = b;
    ed->buffers = b;
    ed->buffer_count++;
    if (ed->mods) mod_registry_on_buffer_opened(ed->mods, ed, b);
    return b;
}

static void fill_buffer(buffer_t* b, const char* text) {
    buffer_replace_text(b, text ? text : "(empty)");
    b->render_valid = false;
    buffer_update_render(b);
    if (b->cursor.cursor_line >= b->text.line_count) {
        b->cursor.cursor_line = 0;
        b->cursor.cursor_col = 0;
    }
}

static const char* git_root(editor_t* ed) {
    if (ed->config && ed->config->project_dir[0]) return ed->config->project_dir;
    if (ed->current_buffer && ed->current_buffer->filename[0]) return ed->current_buffer->filename;
    return NULL;
}

qicto_cmd_result_t cmd_git(editor_t* ed, const char* args, char** out);

qicto_cmd_result_t cmd_git_status(editor_t* ed, const char* args, char** out) {
    (void)args;
    const char* root = git_root(ed);
    if (!root) {
        if (out) *out = strdup("no git root");
        return QICTO_CMD_ERROR;
    }
    char* txt = capture_with_root(root, "status");
    buffer_t* b = open_scratch(ed, "[git:status]");
    if (!b) { free(txt); return QICTO_CMD_ERROR; }
    fill_buffer(b, txt);
    free(txt);
    ed->current_buffer = b;
    if (ed->layout.active) ed->layout.active->buffer = b;
    if (out) *out = strdup("git status");
    return QICTO_CMD_SUCCESS;
}

qicto_cmd_result_t cmd_git_diff(editor_t* ed, const char* args, char** out) {
    const char* root = git_root(ed);
    if (!root) {
        if (out) *out = strdup("no git root");
        return QICTO_CMD_ERROR;
    }
    char sub[QICTO_MAX_PATH_LEN];
    if (args && *args) {
        snprintf(sub, sizeof(sub), "diff -- %s", args);
    } else {
        snprintf(sub, sizeof(sub), "diff");
    }
    char* txt = capture_with_root(root, sub);
    buffer_t* b = open_scratch(ed, "[git:diff]");
    if (!b) { free(txt); return QICTO_CMD_ERROR; }
    fill_buffer(b, txt);
    free(txt);
    ed->current_buffer = b;
    if (ed->layout.active) ed->layout.active->buffer = b;
    if (out) *out = strdup("git diff");
    return QICTO_CMD_SUCCESS;
}

qicto_cmd_result_t cmd_git_blame(editor_t* ed, const char* args, char** out) {
    const char* root = git_root(ed);
    if (!root) {
        if (out) *out = strdup("no git root");
        return QICTO_CMD_ERROR;
    }
    const char* target = args && *args ? args
                      : (ed->current_buffer && ed->current_buffer->filename[0]
                         ? ed->current_buffer->filename : NULL);
    if (!target) {
        if (out) *out = strdup("git blame <file>");
        return QICTO_CMD_ERROR;
    }
    char sub[QICTO_MAX_PATH_LEN];
    snprintf(sub, sizeof(sub), "blame %s", target);
    char* txt = capture_with_root(root, sub);
    buffer_t* b = open_scratch(ed, "[git:blame]");
    if (!b) { free(txt); return QICTO_CMD_ERROR; }
    fill_buffer(b, txt);
    free(txt);
    ed->current_buffer = b;
    if (ed->layout.active) ed->layout.active->buffer = b;
    if (out) *out = strdup("git blame");
    return QICTO_CMD_SUCCESS;
}

qicto_cmd_result_t cmd_git_log(editor_t* ed, const char* args, char** out) {
    const char* root = git_root(ed);
    if (!root) {
        if (out) *out = strdup("no git root");
        return QICTO_CMD_ERROR;
    }
    char sub[QICTO_MAX_PATH_LEN];
    int n = 50;
    if (args && *args) {
        const char* p = args;
        while (*p && !isdigit((unsigned char)*p)) p++;
        if (*p) n = atoi(p);
    }
    snprintf(sub, sizeof(sub), "log --oneline -n %d", n);
    char* txt = capture_with_root(root, sub);
    buffer_t* b = open_scratch(ed, "[git:log]");
    if (!b) { free(txt); return QICTO_CMD_ERROR; }
    fill_buffer(b, txt);
    free(txt);
    ed->current_buffer = b;
    if (ed->layout.active) ed->layout.active->buffer = b;
    if (out) *out = strdup("git log");
    return QICTO_CMD_SUCCESS;
}

qicto_cmd_result_t cmd_git_branch(editor_t* ed, const char* args, char** out) {
    (void)args;
    const char* root = git_root(ed);
    if (!root) {
        if (out) *out = strdup("no git root");
        return QICTO_CMD_ERROR;
    }
    char* txt = capture_with_root(root, "branch -a");
    buffer_t* b = open_scratch(ed, "[git:branch]");
    if (!b) { free(txt); return QICTO_CMD_ERROR; }
    fill_buffer(b, txt);
    free(txt);
    ed->current_buffer = b;
    if (ed->layout.active) ed->layout.active->buffer = b;
    if (out) *out = strdup("git branch");
    return QICTO_CMD_SUCCESS;
}

qicto_cmd_result_t cmd_git(editor_t* ed, const char* args, char** out) {
    (void)ed; (void)args;
    if (out) {
        *out = strdup("git subcommands: status, diff [path], blame [file], log [n], branch");
    }
    return QICTO_CMD_SUCCESS;
}

static qicto_cmd_result_t git_init(editor_t* ed) {
    commands_register(ed->commands, "git", cmd_git, "Run a git subcommand");
    commands_register(ed->commands, "gitstatus", cmd_git_status, ":git status");
    commands_register(ed->commands, "gitdiff", cmd_git_diff, ":git diff [path]");
    commands_register(ed->commands, "gitblame", cmd_git_blame, ":git blame [file]");
    commands_register(ed->commands, "gitlog", cmd_git_log, ":git log [n]");
    commands_register(ed->commands, "gitbranch", cmd_git_branch, ":git branch");
    return QICTO_CMD_SUCCESS;
}

static void git_cleanup(editor_t* ed) {
    (void)ed;
}

static void git_on_buffer_opened(editor_t* ed, buffer_t* buf) {
    (void)ed; (void)buf;
}
static void git_on_buffer_changed(editor_t* ed, buffer_t* buf) {
    (void)ed; (void)buf;
}
static void git_on_render(editor_t* ed, void* ncp) {
    (void)ed; (void)ncp;
}
static qicto_cmd_result_t git_on_command(editor_t* ed, const char* cmd, char** out) {
    (void)ed; (void)cmd; (void)out;
    return QICTO_CMD_UNKNOWN;
}

static const char g_name[] = "git-ui";
static const char g_version[] = "0.1.0";

static qicto_mod_api_t s_git_api = {
    .name = g_name,
    .version = g_version,
    .description = "Git status/diff/blame/log/branch commands via popen",
    .init = git_init,
    .cleanup = git_cleanup,
    .on_render = git_on_render,
    .on_key = NULL,
    .on_buffer_opened = git_on_buffer_opened,
    .on_buffer_changed = git_on_buffer_changed,
    .on_mode_change = NULL,
    .on_config_reload = NULL,
    .on_command = git_on_command,
};

const qicto_mod_api_t* git_ui_mod_get_api(void) {
    return &s_git_api;
}

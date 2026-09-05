#define _POSIX_C_SOURCE 200809L
#include "command.h"
#include "editor.h"
#include "buffer.h"
#include "module.h"
#include "utils/strings.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

command_registry_t* commands_create(void) {
    command_registry_t* cmds = calloc(1, sizeof(command_registry_t));
    return cmds;
}

void commands_destroy(command_registry_t* cmds) {
    if (!cmds) return;
    free(cmds->entries);
    free(cmds);
}

int commands_register(command_registry_t* cmds, const char* name, command_fn fn, const char* help) {
    if (!cmds || !name || !fn) return -1;
    if (cmds->count >= cmds->capacity) {
        size_t newcap = cmds->capacity ? cmds->capacity * 2 : 16;
        command_entry_t* ne = realloc(cmds->entries, newcap * sizeof(command_entry_t));
        if (!ne) return -1;
        cmds->entries = ne;
        cmds->capacity = newcap;
    }
    strncpy(cmds->entries[cmds->count].name, name, 63);
    cmds->entries[cmds->count].name[63] = '\0';
    cmds->entries[cmds->count].fn = fn;
    cmds->entries[cmds->count].help = help ? help : "";
    cmds->count++;
    return 0;
}

command_entry_t* commands_find(command_registry_t* cmds, const char* name) {
    if (!cmds || !name) return NULL;
    /* exact match first */
    for (size_t i = 0; i < cmds->count; i++) {
        if (strcmp(cmds->entries[i].name, name) == 0) {
            return &cmds->entries[i];
        }
    }
    /* prefix match: entry's name starts with the user's input.
     * skip if user input equals an entry name (already handled above) and
     * require the entry to be unambiguously longer than the input. */
    command_entry_t* prefix_match = NULL;
    size_t input_len = strlen(name);
    if (input_len == 0) return NULL;
    for (size_t i = 0; i < cmds->count; i++) {
        size_t entry_len = strlen(cmds->entries[i].name);
        if (entry_len > input_len &&
            strncmp(cmds->entries[i].name, name, input_len) == 0) {
            if (prefix_match) {
                /* ambiguous — multiple matches, refuse to guess */
                return NULL;
            }
            prefix_match = &cmds->entries[i];
        }
    }
    return prefix_match;
}

qicto_cmd_result_t commands_execute(command_registry_t* cmds, editor_t* ed, const char* input, char** out) {
    if (!cmds || !ed || !input) return QICTO_CMD_UNKNOWN;

    char buf[QICTO_MAX_CMD_LEN];
    strncpy(buf, input, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* cmd_name = strtok(buf, " ");
    char* args = strtok(NULL, "");

    command_entry_t* entry = commands_find(cmds, cmd_name);
    if (!entry) {
        return QICTO_CMD_UNKNOWN;
    }

    return entry->fn(ed, args, out);
}

/* Returns a freshly-allocated array of malloc'd C strings (one per
 * command). The caller owns both the array and the strings and must
 * free each entry plus the array itself. */
void commands_list(command_registry_t* cmds, char*** names, size_t* count) {
    if (!cmds || !names || !count) return;
    *count = cmds->count;
    *names = NULL;
    if (cmds->count == 0) return;
    *names = calloc(cmds->count, sizeof(char*));
    if (!*names) {
        *count = 0;
        return;
    }
    for (size_t i = 0; i < cmds->count; i++) {
        (*names)[i] = qstr_dup(cmds->entries[i].name);
    }
}

qicto_cmd_result_t cmd_quit(editor_t* ed, const char* args, char** out) {
    (void)out;
    if (!ed) return QICTO_CMD_ERROR;
    if (args && args[0] == '!') {
        ed->force_quit = true;
    }
    editor_quit(ed);
    return QICTO_CMD_SUCCESS;
}

qicto_cmd_result_t cmd_write(editor_t* ed, const char* args, char** out) {
    if (!ed || !ed->current_buffer) return QICTO_CMD_ERROR;

    const char* filename = args ? args : ed->current_buffer->filename;
    if (!filename || !filename[0]) return QICTO_CMD_ERROR;

    int rc = buffer_save(ed->current_buffer, filename);
    if (rc != 0) {
        if (out) *out = strdup("failed to save");
        return QICTO_CMD_ERROR;
    }

    if (out) *out = strdup("file saved");
    return QICTO_CMD_SUCCESS;
}

qicto_cmd_result_t cmd_edit(editor_t* ed, const char* args, char** out) {
    if (!ed || !args) return QICTO_CMD_ERROR;
    buffer_t* buf = editor_open_file(ed, args);
    if (!buf) {
        if (out) *out = strdup("failed to open file");
        return QICTO_CMD_ERROR;
    }
    if (out) {
        char tmp[512];
        snprintf(tmp, sizeof(tmp), "opened: %s", args);
        *out = strdup(tmp);
    }
    return QICTO_CMD_SUCCESS;
}

qicto_cmd_result_t cmd_buffer(editor_t* ed, const char* args, char** out) {
    if (!ed) return QICTO_CMD_ERROR;

    if (!args || strlen(args) == 0) {
        char tmp[1024] = {0};
        buffer_t* b = ed->buffers;
        while (b) {
            char marker = ' ';
            if (b == ed->current_buffer) marker = '%';
            char line[256];
            snprintf(line, sizeof(line), " %c[%d] %s%s\n",
                     marker, b->id, buffer_display_name(b),
                     b->dirty ? " *" : "");
            strncat(tmp, line, sizeof(tmp) - strlen(tmp) - 1);
            b = b->next;
        }
        if (out) *out = strdup(tmp);
        return QICTO_CMD_SUCCESS;
    }

    /* :buffer N — switch to buffer by id */
    char* endp = NULL;
    long target = strtol(args, &endp, 10);
    if (endp == args || target < 0) {
        if (out) *out = strdup("usage: :buffer <id>");
        return QICTO_CMD_ERROR;
    }
    buffer_t* b = ed->buffers;
    while (b) {
        if ((long)b->id == target) {
            ed->current_buffer = b;
            if (out) {
                char tmp[256];
                snprintf(tmp, sizeof(tmp), "switched to buffer %ld: %s",
                         target, buffer_display_name(b));
                *out = strdup(tmp);
            }
            return QICTO_CMD_SUCCESS;
        }
        b = b->next;
    }
    if (out) {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "no buffer with id %ld", target);
        *out = strdup(tmp);
    }
    return QICTO_CMD_ERROR;
}

qicto_cmd_result_t cmd_help(editor_t* ed, const char* args, char** out) {
    if (!ed || !ed->commands) return QICTO_CMD_ERROR;
    (void)args;

    char** names = NULL;
    size_t count = 0;
    commands_list(ed->commands, &names, &count);

    if (out) {
        size_t cap = 4096;
        char* buf = malloc(cap);
        if (buf) {
            size_t off = 0;
            buf[0] = '\0';
            for (size_t i = 0; i < count; i++) {
                /* look up the help string for this command */
                command_entry_t* entry = commands_find(ed->commands, names[i]);
                const char* help = (entry && entry->help) ? entry->help : "";
                int written = snprintf(buf + off, cap - off, "  %-12s  %s\n",
                                       names[i], help);
                if (written < 0 || (size_t)written >= cap - off) break;
                off += (size_t)written;
            }
            *out = buf;
        }
    }
    /* commands_list returns a malloc'd char**; the strings are strdup'd.
     * We must free both. */
    if (names) {
        for (size_t i = 0; i < count; i++) free(names[i]);
        free(names);
    }
    return QICTO_CMD_SUCCESS;
}

qicto_cmd_result_t cmd_version(editor_t* ed, const char* args, char** out) {
    (void)ed; (void)args;
    if (out) {
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "qicto %s", QICTO_VERSION_STRING);
        *out = strdup(tmp);
    }
    return QICTO_CMD_SUCCESS;
}

qicto_cmd_result_t cmd_lsmods(editor_t* ed, const char* args, char** out) {
    (void)args;
    if (!ed || !ed->mods) {
        if (out) *out = strdup("no mods loaded");
        return QICTO_CMD_SUCCESS;
    }
    char tmp[1024] = {0};
    for (size_t i = 0; i < ed->mods->count; i++) {
        char line[256];
        snprintf(line, sizeof(line), "  %s v%s\n",
                 ed->mods->entries[i]->name,
                 ed->mods->entries[i]->api.version);
        strncat(tmp, line, sizeof(tmp) - strlen(tmp) - 1);
    }
    if (out) *out = strdup(tmp);
    return QICTO_CMD_SUCCESS;
}

qicto_cmd_result_t cmd_undo(editor_t* ed, const char* args, char** out) {
    (void)args;
    if (!ed || !ed->current_buffer) return QICTO_CMD_ERROR;
    if (buffer_undo(ed->current_buffer) == 0) {
        if (out) *out = strdup("undo");
        return QICTO_CMD_SUCCESS;
    }
    if (out) *out = strdup("nothing to undo");
    return QICTO_CMD_ERROR;
}

/* Search using pcre2 is wired through Dependencies.cmake, but the C API
 * surface is significant. For now do a simple case-sensitive substring
 * scan. This is the same fallback vim uses for non-regex searches and
 * is plenty for the common case. Returns 0 on match, -1 if not found. */
int find_substr(editor_t* ed, const char* needle, int reverse) {
    if (!ed || !ed->current_buffer || !needle || !*needle) return -1;
    buffer_t* buf = ed->current_buffer;
    size_t nlen = strlen(needle);
    if (reverse) {
        if (buf->cursor.cursor_line == 0 && buf->cursor.cursor_col == 0) return -1;
        size_t line = buf->cursor.cursor_line;
        size_t col = buf->cursor.cursor_col;
        /* search backward: scan current line up to col, then prior lines */
        while (1) {
            const char* ln = buf->text.lines[line];
            size_t llen = ln ? strlen(ln) : 0;
            size_t start = (line == buf->cursor.cursor_line) ? col : llen;
            if (start > 0) {
                for (size_t i = start; i-- > 0; ) {
                    if (i + nlen <= llen &&
                        strncmp(ln + i, needle, nlen) == 0) {
                        buf->cursor.cursor_line = line;
                        buf->cursor.cursor_col = i;
                        return 0;
                    }
                }
            }
            if (line == 0) break;
            line--;
        }
        return -1;
    } else {
        size_t line = buf->cursor.cursor_line;
        size_t col = buf->cursor.cursor_col;
        while (line < buf->text.line_count) {
            const char* ln = buf->text.lines[line];
            size_t llen = ln ? strlen(ln) : 0;
            size_t start = (line == buf->cursor.cursor_line) ? col + 1 : 0;
            if (start < llen) {
                for (size_t i = start; i + nlen <= llen; i++) {
                    if (strncmp(ln + i, needle, nlen) == 0) {
                        buf->cursor.cursor_line = line;
                        buf->cursor.cursor_col = i;
                        return 0;
                    }
                }
            }
            line++;
        }
        return -1;
    }
}

qicto_cmd_result_t cmd_search(editor_t* ed, const char* args, char** out) {
    if (!ed || !ed->current_buffer) return QICTO_CMD_ERROR;
    if (!args || !*args) {
        if (out) *out = strdup("usage: :search <text>  (or /text in NORMAL mode)");
        return QICTO_CMD_ERROR;
    }
    int rc = find_substr(ed, args, 0);
    if (rc == 0) {
        if (out) {
            char tmp[256];
            snprintf(tmp, sizeof(tmp), "found: %s", args);
            *out = strdup(tmp);
        }
        return QICTO_CMD_SUCCESS;
    }
    if (out) {
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "not found: %s", args);
        *out = strdup(tmp);
    }
    return QICTO_CMD_ERROR;
}

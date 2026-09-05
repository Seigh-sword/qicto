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

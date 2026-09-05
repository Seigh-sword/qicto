#include "command.h"
#include "editor.h"
#include "buffer.h"
#include "module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>

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
    for (size_t i = 0; i < cmds->count; i++) {
        if (strcmp(cmds->entries[i].name, name) == 0) {
            return &cmds->entries[i];
        }
    }
    for (size_t i = 0; i < cmds->count; i++) {
        if (strncmp(cmds->entries[i].name, name, strlen(cmds->entries[i].name)) == 0) {
            return &cmds->entries[i];
        }
    }
    return NULL;
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

void commands_list(command_registry_t* cmds, char*** names, size_t* count) {
    if (!cmds || !names || !count) return;
    *count = cmds->count;
    if (cmds->count == 0) {
        *names = NULL;
        return;
    }
    *names = malloc(cmds->count * sizeof(char*));
    for (size_t i = 0; i < cmds->count; i++) {
        (*names)[i] = strdup(cmds->entries[i].name);
    }
}

qicto_cmd_result_t cmd_quit(editor_t* ed, const char* args, char** out) {
    (void)args;
    (void)out;
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
            char line[256];
            snprintf(line, sizeof(line), "  [%d] %s%s\n",
                     b->id, buffer_display_name(b),
                     b->dirty ? " *" : "");
            strncat(tmp, line, sizeof(tmp) - strlen(tmp) - 1);
            b = b->next;
        }
        if (out) *out = strdup(tmp);
        return QICTO_CMD_SUCCESS;
    }

    return QICTO_CMD_UNKNOWN;
}

qicto_cmd_result_t cmd_help(editor_t* ed, const char* args, char** out) {
    if (!ed || !ed->commands) return QICTO_CMD_ERROR;

    char** names = NULL;
    size_t count = 0;
    commands_list(ed->commands, &names, &count);

    if (out) {
        char* buf = malloc(4096);
        if (buf) {
            buf[0] = '\0';
            for (size_t i = 0; i < count && i < 100; i++) {
                strncat(buf, names[i], 255);
                strncat(buf, "\n", 1);
            }
            *out = buf;
        }
    }
    free(names);
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

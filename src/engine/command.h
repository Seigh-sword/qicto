#ifndef QICTO_COMMAND_H
#define QICTO_COMMAND_H

#include "qicto.h"

typedef qicto_cmd_result_t (*command_fn)(editor_t* ed, const char* args, char** out);

typedef struct {
    char name[64];
    command_fn fn;
    const char* help;
} command_entry_t;

struct command_registry_t {
    command_entry_t* entries;
    size_t count;
    size_t capacity;
};
typedef struct command_registry_t command_registry_t;

command_registry_t* commands_create(void);
void commands_destroy(command_registry_t* cmds);
int commands_register(command_registry_t* cmds, const char* name, command_fn fn, const char* help);
command_entry_t* commands_find(command_registry_t* cmds, const char* name);
qicto_cmd_result_t commands_execute(command_registry_t* cmds, editor_t* ed, const char* input, char** out);
void commands_list(command_registry_t* cmds, char*** names, size_t* count);

/* Substring search used by :search and n/N repeat. Returns 0 on match,
 * -1 if not found. */
int find_substr(editor_t* ed, const char* needle, int reverse);
qicto_cmd_result_t cmd_quit(editor_t* ed, const char* args, char** out);
qicto_cmd_result_t cmd_write(editor_t* ed, const char* args, char** out);
qicto_cmd_result_t cmd_edit(editor_t* ed, const char* args, char** out);
qicto_cmd_result_t cmd_buffer(editor_t* ed, const char* args, char** out);
qicto_cmd_result_t cmd_help(editor_t* ed, const char* args, char** out);
qicto_cmd_result_t cmd_version(editor_t* ed, const char* args, char** out);
qicto_cmd_result_t cmd_lsmods(editor_t* ed, const char* args, char** out);
qicto_cmd_result_t cmd_undo(editor_t* ed, const char* args, char** out);
qicto_cmd_result_t cmd_search(editor_t* ed, const char* args, char** out);

#endif

#include "editor.h"
#include "buffer.h"
#include "command.h"
#include "config.h"
#include "module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

static void editor_update_display_name(editor_t* ed, buffer_t* buf) {
    if (!buf->display_name[0] && buf->filename[0]) {
        const char* base = strrchr(buf->filename, '/');
        if (!base) base = strrchr(buf->filename, '\\');
        if (base) base++;
        else base = buf->filename;
        strncpy(buf->display_name, base, QICTO_MAX_NAME_LEN - 1);
    }
}

editor_t* editor_create(void) {
    editor_t* ed = calloc(1, sizeof(editor_t));
    if (!ed) return NULL;

    ed->mode = QICTO_MODE_NORMAL;
    ed->config = config_create();
    config_load_builtin_defaults(ed->config);
    ed->current_buffer = NULL;
    ed->buffer_count = 0;
    ed->buffers = NULL;
    ed->commands = commands_create();

    commands_register(ed->commands, "quit", cmd_quit, "Quit the editor");
    commands_register(ed->commands, "q", cmd_quit, "Quit the editor");
    commands_register(ed->commands, "write", cmd_write, "Save current buffer");
    commands_register(ed->commands, "w", cmd_write, "Save current buffer");
    commands_register(ed->commands, "edit", cmd_edit, "Open a file");
    commands_register(ed->commands, "e", cmd_edit, "Open a file");
    commands_register(ed->commands, "buffer", cmd_buffer, "List or switch buffers");
    commands_register(ed->commands, "ls", cmd_buffer, "List buffers");
    commands_register(ed->commands, "help", cmd_help, "Show help");
    commands_register(ed->commands, "version", cmd_version, "Show version");
    commands_register(ed->commands, "lsmods", cmd_lsmods, "List loaded modules");

    ed->layout.windows = NULL;
    ed->layout.window_count = 0;
    ed->layout.active = NULL;
    ed->layout.split_mode = 0;

    ed->statusmsg[0] = '\0';
    ed->statusmsg_time = 0;

    return ed;
}

void editor_destroy(editor_t* ed) {
    if (!ed) return;

    if (ed->commands) commands_destroy(ed->commands);
    if (ed->mods) mod_registry_destroy(ed->mods);

    buffer_t* buf = ed->buffers;
    while (buf) {
        buffer_t* next = buf->next;
        buffer_free(buf);
        buf = next;
    }

    if (ed->config) config_destroy(ed->config);
    free(ed);
}

void editor_set_mode(editor_t* ed, qicto_mode_t mode) {
    if (!ed) return;
    qicto_mode_t old = ed->mode;
    ed->mode = mode;
    if (ed->mods) {
        mod_registry_on_mode_change(ed->mods, ed, old, mode);
    }
}

buffer_t* editor_open_file(editor_t* ed, const char* filename) {
    if (!ed || !filename) return NULL;

    buffer_t* buf = ed->buffers;
    while (buf) {
        if (strcmp(buf->filename, filename) == 0) {
            ed->current_buffer = buf;
            editor_update_display_name(ed, buf);
            return buf;
        }
        buf = buf->next;
    }

    buf = buffer_new(filename);
    if (!buf) return NULL;

    buffer_load_file(buf, filename);
    editor_update_display_name(ed, buf);

    buf->next = ed->buffers;
    if (ed->buffers) ed->buffers->prev = buf;
    ed->buffers = buf;
    ed->buffer_count++;
    ed->current_buffer = buf;

    if (ed->mods) {
        mod_registry_on_buffer_opened(ed->mods, ed, buf);
    }

    return buf;
}

buffer_t* editor_current_buffer(editor_t* ed) {
    return ed ? ed->current_buffer : NULL;
}

void editor_cycle_buffer(editor_t* ed, int direction) {
    if (!ed || !ed->buffers || !ed->current_buffer) return;

    if (direction > 0) {
        if (ed->current_buffer->next)
            ed->current_buffer = ed->current_buffer->next;
        else
            ed->current_buffer = ed->buffers;
    } else {
        if (ed->current_buffer->prev)
            ed->current_buffer = ed->current_buffer->prev;
        else {
            buffer_t* last = ed->buffers;
            while (last->next) last = last->next;
            ed->current_buffer = last;
        }
    }
}

void editor_close_buffer(editor_t* ed, buffer_t* buf) {
    if (!ed || !buf) return;

    if (buf->prev) buf->prev->next = buf->next;
    else ed->buffers = buf->next;
    if (buf->next) buf->next->prev = buf->prev;

    ed->buffer_count--;
    if (ed->current_buffer == buf) {
        ed->current_buffer = ed->buffers ? ed->buffers : NULL;
    }

    buffer_free(buf);
}

void editor_quit(editor_t* ed) {
    (void)ed;
    exit(0);
}

void editor_redraw(editor_t* ed) {
    if (!ed) return;
    if (ed->current_buffer) {
        buffer_update_render(ed->current_buffer);
    }
}

void editor_set_status(editor_t* ed, const char* fmt, ...) {
    if (!ed || !fmt) return;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(ed->statusmsg, sizeof(ed->statusmsg), fmt, ap);
    va_end(ap);
    ed->statusmsg_time = time(NULL);
}

config_t* editor_config(editor_t* ed) {
    return ed ? ed->config : NULL;
}

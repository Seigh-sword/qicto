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

    commands_register(ed->commands, "quit", cmd_quit, "Quit the editor (:quit! to force)");
    commands_register(ed->commands, "q", cmd_quit, "Quit the editor (q! to force)");
    commands_register(ed->commands, "write", cmd_write, "Save current buffer (:w [path])");
    commands_register(ed->commands, "w", cmd_write, "Save current buffer");
    commands_register(ed->commands, "edit", cmd_edit, "Open a file (:e <path>)");
    commands_register(ed->commands, "e", cmd_edit, "Open a file");
    commands_register(ed->commands, "buffer", cmd_buffer, "List or switch buffers (:buffer [N])");
    commands_register(ed->commands, "ls", cmd_buffer, "List buffers");
    commands_register(ed->commands, "b", cmd_buffer, "Switch to buffer by id");
    commands_register(ed->commands, "help", cmd_help, "Show help");
    commands_register(ed->commands, "version", cmd_version, "Show version");
    commands_register(ed->commands, "lsmods", cmd_lsmods, "List loaded modules");
    commands_register(ed->commands, "undo", cmd_undo, "Undo last edit (:undo)");
    commands_register(ed->commands, "u", cmd_undo, "Undo last edit");
    commands_register(ed->commands, "search", cmd_search, "Search forward (:search <text>)");
    commands_register(ed->commands, "/", cmd_search, "Search forward (alias)");

    ed->layout.root = NULL;
    ed->layout.active = NULL;
    ed->layout.window_count = 0;
    ed->layout.line_num_width = 4;
    ed->layout.status_height = 2;
    ed->layout.cmd_height = 1;

    ed->statusmsg[0] = '\0';
    ed->statusmsg_time = 0;
    ed->top_line = 0;
    ed->col_offset = 0;
    ed->scroll_offset = 5;
    ed->last_cmd[0] = '\0';
    ed->last_cmd_len = 0;
    ed->pending[0] = '\0';
    ed->pending_len = 0;
    ed->quit_requested = false;
    ed->force_quit = false;

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
    if (!ed) return;
    if (ed->current_buffer && ed->current_buffer->dirty && !ed->force_quit) {
        editor_set_status(ed, "unsaved changes, use :q! to force quit");
        return;
    }
    ed->quit_requested = true;
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

bool editor_status_active(editor_t* ed) {
    if (!ed || !ed->statusmsg[0]) return false;
    return (time(NULL) - ed->statusmsg_time) < 5;
}

void editor_clear_status(editor_t* ed) {
    if (!ed) return;
    ed->statusmsg[0] = '\0';
    ed->statusmsg_time = 0;
}

config_t* editor_config(editor_t* ed) {
    return ed ? ed->config : NULL;
}

void editor_scroll_to_cursor(editor_t* ed, int viewport_height, int viewport_width) {
    if (!ed || !ed->current_buffer) return;
    if (viewport_height <= 0) viewport_height = 24;
    if (viewport_width <= 0) viewport_width = 80;
    buffer_t* buf = ed->current_buffer;
    size_t cur = buf->cursor.cursor_line;
    int so = ed->scroll_offset;
    if (so < 0) so = 0;
    if ((int)cur < (int)ed->top_line + so) {
        if (cur >= (size_t)so) ed->top_line = cur - so;
        else ed->top_line = 0;
    } else if ((int)cur >= (int)ed->top_line + viewport_height - so) {
        size_t new_top = (size_t)cur + (size_t)so + 1;
        if (new_top >= (size_t)viewport_height) ed->top_line = new_top - (size_t)viewport_height;
        else ed->top_line = 0;
    }
    size_t cw = buf->cursor.cursor_col;
    if ((int)cw < (int)ed->col_offset + 5) {
        ed->col_offset = (cw >= 5) ? cw - 5 : 0;
    } else if ((int)cw >= (int)ed->col_offset + viewport_width - 5) {
        ed->col_offset = (size_t)cw + 5 >= (size_t)viewport_width
            ? cw + 5 - viewport_width : 0;
    }
}

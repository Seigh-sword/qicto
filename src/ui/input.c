#include "input.h"
#include "buffer.h"
#include "editor.h"
#include "command.h"
#include "config.h"
#include "module.h"
#include "strings.h"
#include <notcurses/notcurses.h>
#include <utf8proc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

qkey_t input_map_nckey(struct ncinput* input, int nc_key) {
    if (nc_key <= 0) return QICTO_KEY_NONE;

    if (input) {
        uint8_t mods = 0;
#ifdef NCINPUT_MODIFIER_SHIFT
        mods = input->modifiers;
#elif defined(HAVE_NCINPUT_MODIFIERS)
        mods = input->modifiers;
#else
        mods = 0;
#endif
        bool shifted = false;
#if defined(NCKEY_MAP_SHIFT) || defined(NCMOD_SHIFT)
        shifted = (mods & 0x01) ? true : false;
#endif
        (void)shifted;
        (void)input;
    }

    switch (nc_key) {
        case NCKEY_UP:    return QICTO_KEY_UP;
        case NCKEY_DOWN:  return QICTO_KEY_DOWN;
        case NCKEY_LEFT:  return QICTO_KEY_LEFT;
        case NCKEY_RIGHT: return QICTO_KEY_RIGHT;
        case NCKEY_HOME:  return QICTO_KEY_HOME;
        case NCKEY_END:   return QICTO_KEY_END;
        case NCKEY_PGUP:  return QICTO_KEY_PAGE_UP;
        case NCKEY_PGDOWN:  return QICTO_KEY_PAGE_DOWN;
        case NCKEY_DEL:   return QICTO_KEY_DELETE;
        case NCKEY_INS:   return QICTO_KEY_INSERT;
        case NCKEY_F01:   return QICTO_KEY_F1;
        case NCKEY_F02:   return QICTO_KEY_F2;
        case NCKEY_F03:   return QICTO_KEY_F3;
        case NCKEY_F04:   return QICTO_KEY_F4;
        case NCKEY_F05:   return QICTO_KEY_F5;
        case NCKEY_F06:   return QICTO_KEY_F6;
        case NCKEY_F07:   return QICTO_KEY_F7;
        case NCKEY_F08:   return QICTO_KEY_F8;
        case NCKEY_F09:   return QICTO_KEY_F9;
        case NCKEY_F10:   return QICTO_KEY_F10;
        case NCKEY_F11:   return QICTO_KEY_F11;
        case NCKEY_F12:   return QICTO_KEY_F12;
        case NCKEY_ENTER: return QICTO_KEY_ENTER;
        case NCKEY_BACKSPACE: return QICTO_KEY_BACKSPACE;
        default:
            if (nc_key >= 0x20 && nc_key < 0x7f) return (qkey_t)nc_key;
            if (nc_key >= 0x80) return (qkey_t)nc_key;
            return (qkey_t)nc_key;
    }
}

qkey_t input_map_char(uint32_t codepoint) {
    return (qkey_t)codepoint;
}

int input_is_modifier(qkey_t key) {
    return (key >= QICTO_KEY_UP && key <= QICTO_KEY_F12) ? 1 : 0;
}

const char* key_name(qkey_t key) {
    switch (key) {
        case QICTO_KEY_UP: return "Up";
        case QICTO_KEY_DOWN: return "Down";
        case QICTO_KEY_LEFT: return "Left";
        case QICTO_KEY_RIGHT: return "Right";
        case QICTO_KEY_HOME: return "Home";
        case QICTO_KEY_END: return "End";
        case QICTO_KEY_PAGE_UP: return "PgUp";
        case QICTO_KEY_PAGE_DOWN: return "PgDn";
        case QICTO_KEY_DELETE: return "Del";
        case QICTO_KEY_INSERT: return "Ins";
        case QICTO_KEY_ENTER: return "Enter";
        case QICTO_KEY_BACKSPACE: return "BS";
        case QICTO_KEY_TAB: return "Tab";
        case QICTO_KEY_ESC: return "Esc";
        case QICTO_KEY_F1: return "F1";
        case QICTO_KEY_F2: return "F2";
        case QICTO_KEY_F3: return "F3";
        case QICTO_KEY_F4: return "F4";
        case QICTO_KEY_F5: return "F5";
        case QICTO_KEY_F6: return "F6";
        case QICTO_KEY_F7: return "F7";
        case QICTO_KEY_F8: return "F8";
        case QICTO_KEY_F9: return "F9";
        case QICTO_KEY_F10: return "F10";
        case QICTO_KEY_F11: return "F11";
        case QICTO_KEY_F12: return "F12";
        default:
            if (key >= 0x20 && key < 0x7f) {
                static char buf[2];
                buf[0] = (char)key;
                buf[1] = '\0';
                return buf;
            }
            return "?";
    }
}

static void move_cursor_left(editor_t* ed) {
    buffer_t* buf = ed->current_buffer;
    if (!buf) return;
    if (buf->cursor.cursor_col > 0) {
        buf->cursor.cursor_col--;
    } else if (buf->cursor.cursor_line > 0) {
        buf->cursor.cursor_line--;
        buf->cursor.cursor_col = buffer_line_length(buf, buf->cursor.cursor_line);
    }
}

static void move_cursor_right(editor_t* ed) {
    buffer_t* buf = ed->current_buffer;
    if (!buf) return;
    size_t len = buffer_line_length(buf, buf->cursor.cursor_line);
    if (buf->cursor.cursor_col < len) {
        buf->cursor.cursor_col++;
    } else if (buf->cursor.cursor_line < buf->text.line_count - 1) {
        buf->cursor.cursor_line++;
        buf->cursor.cursor_col = 0;
    }
}

static void move_cursor_up(editor_t* ed) {
    buffer_t* buf = ed->current_buffer;
    if (!buf || buf->cursor.cursor_line == 0) return;
    buf->cursor.cursor_line--;
    size_t len = buffer_line_length(buf, buf->cursor.cursor_line);
    if (buf->cursor.cursor_col > len) {
        buf->cursor.cursor_col = len;
    }
}

static void move_cursor_down(editor_t* ed) {
    buffer_t* buf = ed->current_buffer;
    if (!buf || buf->cursor.cursor_line >= buf->text.line_count - 1) return;
    buf->cursor.cursor_line++;
    size_t len = buffer_line_length(buf, buf->cursor.cursor_line);
    if (buf->cursor.cursor_col > len) {
        buf->cursor.cursor_col = len;
    }
}

static void handle_insert_char(editor_t* ed, qkey_t key) {
    buffer_t* buf = ed->current_buffer;
    if (!buf) return;

    if (key == QICTO_KEY_BACKSPACE) {
        if (buf->cursor.cursor_col > 0) {
            buffer_remove_char(buf, buf->cursor.cursor_line, buf->cursor.cursor_col - 1);
            buf->cursor.cursor_col--;
        } else if (buf->cursor.cursor_line > 0) {
            size_t prev_len = buffer_line_length(buf, buf->cursor.cursor_line - 1);
            buffer_join_lines(buf, buf->cursor.cursor_line);
            buf->cursor.cursor_line--;
            buf->cursor.cursor_col = prev_len;
        }
        buf->render_valid = false;
        return;
    }

    if (key == QICTO_KEY_DELETE) {
        buffer_remove_char(buf, buf->cursor.cursor_line, buf->cursor.cursor_col);
        buf->render_valid = false;
        return;
    }

    if (key == QICTO_KEY_ENTER) {
        buffer_split_line(buf, buf->cursor.cursor_line, buf->cursor.cursor_col);
        buf->cursor.cursor_line++;
        buf->cursor.cursor_col = 0;
        buf->render_valid = false;
        return;
    }

    if (key == QICTO_KEY_TAB) {
        buffer_t* b = ed->current_buffer;
        if (ed->config ? ed->config->expand_tabs : 1) {
            int tw = ed->config ? ed->config->tab_width : 4;
            for (int i = 0; i < tw; i++) {
                buffer_insert_char(b, b->cursor.cursor_line, b->cursor.cursor_col, ' ');
                b->cursor.cursor_col++;
            }
        } else {
            buffer_insert_char(b, buf->cursor.cursor_line, buf->cursor.cursor_col, '\t');
            buf->cursor.cursor_col++;
        }
        buf->render_valid = false;
        return;
    }

    if (key >= 0x20 && key < 0x100) {
        char ch = (char)key;
        buffer_insert_char(buf, buf->cursor.cursor_line, buf->cursor.cursor_col, ch);
        buf->cursor.cursor_col++;
        buf->render_valid = false;
    } else if (key >= 0x100) {
        utf8proc_int32_t cp = (utf8proc_int32_t)key;
        utf8proc_uint8_t dst[8];
        utf8proc_ssize_t len = utf8proc_encode_char(cp, dst);
        if (len > 0) {
            char tmp[9] = {0};
            memcpy(tmp, dst, len);
            buffer_insert_text(buf, buf->cursor.cursor_line, buf->cursor.cursor_col, tmp);
            buf->cursor.cursor_col += len;
        }
    }
}

void input_handle_normal(editor_t* ed, qkey_t key) {
    if (!ed || !ed->current_buffer) return;
    buffer_t* buf = ed->current_buffer;

    switch (key) {
        case 'i':
            editor_set_mode(ed, QICTO_MODE_INSERT);
            break;
        case 'h': move_cursor_left(ed); break;
        case 'j': move_cursor_down(ed); break;
        case 'k': move_cursor_up(ed); break;
        case 'l': move_cursor_right(ed); break;
        case '(': case ')': case '0': case '$': case '^':
            break;
        case 'x':
            buffer_remove_char(buf, buf->cursor.cursor_line, buf->cursor.cursor_col);
            buffer_validate_cursor(buf);
            buf->render_valid = false;
            break;
        case 'X':
            if (buf->cursor.cursor_col > 0) {
                buffer_remove_char(buf, buf->cursor.cursor_line, buf->cursor.cursor_col - 1);
                buf->cursor.cursor_col--;
                buf->render_valid = false;
            }
            break;
        case 'a':
            move_cursor_right(ed);
            editor_set_mode(ed, QICTO_MODE_INSERT);
            break;
        case 'o': {
            size_t linelen = buffer_line_length(buf, buf->cursor.cursor_line);
            buffer_split_line(buf, buf->cursor.cursor_line, buf->cursor.cursor_col);
            move_cursor_down(ed);
            buf->cursor.cursor_col = 0;
            editor_set_mode(ed, QICTO_MODE_INSERT);
            buf->render_valid = false;
            break;
        }
        case 'O':
            buffer_split_line(buf, buf->cursor.cursor_line, 0);
            buf->cursor.cursor_col = 0;
            editor_set_mode(ed, QICTO_MODE_INSERT);
            buf->render_valid = false;
            break;
        case 'v':
            ed->mode = QICTO_MODE_VISUAL;
            buf->cursor.has_selection = true;
            buf->cursor.sel_anchor_line = buf->cursor.cursor_line;
            buf->cursor.sel_anchor_col = buf->cursor.cursor_col;
            buf->cursor.sel_cursor_line = buf->cursor.cursor_line;
            buf->cursor.sel_cursor_col = buf->cursor.cursor_col;
            break;
        case 'V':
            ed->mode = QICTO_MODE_VISUAL;
            buf->cursor.has_selection = true;
            buf->cursor.sel_anchor_line = buf->cursor.cursor_line;
            buf->cursor.sel_anchor_col = 0;
            buf->cursor.sel_cursor_line = buf->cursor.cursor_line;
            buf->cursor.sel_cursor_col = buffer_line_length(buf, buf->cursor.cursor_line);
            break;
        case ':':
            editor_set_mode(ed, QICTO_MODE_COMMAND);
            memset(ed->cmd_buffer, 0, sizeof(ed->cmd_buffer));
            ed->cmd_cursor = 0;
            break;
        case '/':
            editor_set_mode(ed, QICTO_MODE_SEARCH);
            memset(ed->cmd_buffer, 0, sizeof(ed->cmd_buffer));
            ed->cmd_cursor = 0;
            break;
        case 'u':
            break;
        case 'n': case 'N':
            break;
        case 'w':
            if (buf->filename[0]) {
                buffer_save(buf, buf->filename);
                editor_set_status(ed, "saved: %s", buf->filename);
            }
            break;
        case 'q':
            if (buf->dirty) {
                editor_set_status(ed, "unsaved changes, force quit with :q!");
            } else {
                editor_quit(ed);
            }
            break;
        case 'b':
            editor_cycle_buffer(ed, 1);
            break;
        case 'B':
            editor_cycle_buffer(ed, -1);
            break;
    }

    editor_redraw(ed);
}

void input_handle_insert(editor_t* ed, qkey_t key) {
    if (!ed) return;
    if (key == QICTO_KEY_ESC) {
        editor_set_mode(ed, QICTO_MODE_NORMAL);
        if (ed->current_buffer) ed->current_buffer->render_valid = false;
        return;
    }
    handle_insert_char(ed, key);
    if (ed->mods) {
        mod_registry_on_buffer_changed(ed->mods, ed, ed->current_buffer);
    }
}

void input_handle_visual(editor_t* ed, qkey_t key) {
    if (!ed || !ed->current_buffer) return;
    buffer_t* buf = ed->current_buffer;

    if (key == QICTO_KEY_ESC) {
        ed->mode = QICTO_MODE_NORMAL;
        buf->cursor.has_selection = false;
        return;
    }

    switch (key) {
        case 'h': move_cursor_left(ed); break;
        case 'j': move_cursor_down(ed); break;
        case 'k': move_cursor_up(ed); break;
        case 'l': move_cursor_right(ed); break;
        case 'v':
            ed->mode = QICTO_MODE_NORMAL;
            buf->cursor.has_selection = false;
            break;
    }
    buf->cursor.sel_cursor_line = buf->cursor.cursor_line;
    buf->cursor.sel_cursor_col = buf->cursor.cursor_col;
}

void input_handle_command(editor_t* ed, qkey_t key) {
    if (!ed) return;
    char* cmd = ed->cmd_buffer;
    size_t* cursor = &ed->cmd_cursor;

    if (key == QICTO_KEY_ESC) {
        editor_set_mode(ed, QICTO_MODE_NORMAL);
        memset(cmd, 0, sizeof(ed->cmd_buffer));
        *cursor = 0;
        return;
    }

    if (key == QICTO_KEY_ENTER) {
        char* out = NULL;
        qicto_cmd_result_t rc = commands_execute(ed->commands, ed, cmd, &out);
        if (rc == QICTO_CMD_UNKNOWN) {
            if (ed->mods) {
                char* mod_out = NULL;
                rc = mod_registry_on_command(ed->mods, ed, cmd, &mod_out);
                if (out) free(out);
                out = mod_out;
            }
        }
        if (out) {
            editor_set_status(ed, "%s", out);
            free(out);
        }
        memset(cmd, 0, sizeof(ed->cmd_buffer));
        *cursor = 0;
        editor_set_mode(ed, QICTO_MODE_NORMAL);
        return;
    }

    if (key == QICTO_KEY_BACKSPACE) {
        if (*cursor > 0) {
            memmove(cmd + *cursor - 1, cmd + *cursor, strlen(cmd + *cursor) + 1);
            (*cursor)--;
        }
        return;
    }

    if (key >= 0x20 && key < 0x100 && *cursor < sizeof(ed->cmd_buffer) - 1) {
        memmove(cmd + *cursor + 1, cmd + *cursor, strlen(cmd + *cursor) + 1);
        cmd[*cursor] = (char)key;
        (*cursor)++;
    }
}

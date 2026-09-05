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

static int is_word_char(uint32_t cp) {
    if (cp == '_') return 1;
    return (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') ||
           (cp >= '0' && cp <= '9');
}
static int is_space(uint32_t cp) {
    return cp == ' ' || cp == '\t';
}

static size_t word_right(editor_t* ed, size_t line, size_t col) {
    buffer_t* buf = ed->current_buffer;
    if (!buf || line >= buf->text.line_count) return col;
    const char* ln = buf->text.lines[line];
    size_t len = strlen(ln);
    if (col >= len) return len;
    int cur_word = is_word_char((unsigned char)ln[col]);
    int cur_space = is_space((unsigned char)ln[col]);
    size_t i = col;
    if (cur_space) {
        while (i < len && is_space((unsigned char)ln[i])) i++;
    } else {
        while (i < len && is_word_char((unsigned char)ln[i]) == cur_word) i++;
    }
    while (i < len && is_space((unsigned char)ln[i])) i++;
    return i;
}

static size_t word_left(editor_t* ed, size_t line, size_t col) {
    buffer_t* buf = ed->current_buffer;
    if (!buf || line >= buf->text.line_count) return col;
    const char* ln = buf->text.lines[line];
    if (col == 0) {
        if (line == 0) return 0;
        return strlen(buf->text.lines[line - 1]);
    }
    size_t i = col;
    int at_word = is_word_char((unsigned char)ln[i - 1]);
    while (i > 0 && is_word_char((unsigned char)ln[i - 1]) == at_word &&
           !is_space((unsigned char)ln[i - 1])) {
        i--;
    }
    while (i > 0 && is_space((unsigned char)ln[i - 1])) i--;
    return i;
}

static size_t word_end(editor_t* ed, size_t line, size_t col) {
    buffer_t* buf = ed->current_buffer;
    if (!buf || line >= buf->text.line_count) return col;
    const char* ln = buf->text.lines[line];
    size_t len = strlen(ln);
    if (col >= len) return len;
    size_t i = col;
    if (i + 1 < len && is_space((unsigned char)ln[i + 1])) {
        while (i < len && is_space((unsigned char)ln[i])) i++;
    } else {
        while (i < len && is_word_char((unsigned char)ln[i])) i++;
        i--;
    }
    return i;
}

static void move_page(editor_t* ed, int delta_pages) {
    if (!ed || !ed->current_buffer) return;
    int step = 20 * delta_pages;
    if (step == 0) step = delta_pages > 0 ? 20 : -20;
    buffer_t* buf = ed->current_buffer;
    long nl = (long)buf->text.line_count;
    long cur = (long)buf->cursor.cursor_line + step;
    if (cur < 0) cur = 0;
    if (cur >= nl) cur = nl - 1;
    buf->cursor.cursor_line = (size_t)cur;
    size_t len = buffer_line_length(buf, buf->cursor.cursor_line);
    if (buf->cursor.cursor_col > len) buf->cursor.cursor_col = len;
}

static void goto_line(editor_t* ed, size_t target) {
    if (!ed || !ed->current_buffer) return;
    buffer_t* buf = ed->current_buffer;
    if (target >= buf->text.line_count) target = buf->text.line_count - 1;
    buf->cursor.cursor_line = target;
    size_t len = buffer_line_length(buf, target);
    if (buf->cursor.cursor_col > len) buf->cursor.cursor_col = len;
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

static char s_yank[QICTO_MAX_LINE_LEN];
static size_t s_yank_len = 0;
static char s_last_search[256] = {0};

static void yank_text(const char* s, size_t n) {
    if (n >= sizeof(s_yank)) n = sizeof(s_yank) - 1;
    memcpy(s_yank, s, n);
    s_yank[n] = '\0';
    s_yank_len = n;
}

void input_handle_normal(editor_t* ed, qkey_t key) {
    if (!ed || !ed->current_buffer) return;
    buffer_t* buf = ed->current_buffer;

    if (ed->pending_len > 0) {
        if (ed->pending_len == 1 && ed->pending[0] == 'g' && key == 'g') {
            ed->pending_len = 0;
            goto_line(ed, 0);
            editor_redraw(ed);
            return;
        }
        ed->pending_len = 0;
    }

    switch (key) {
        case 'i':
            editor_set_mode(ed, QICTO_MODE_INSERT);
            break;
        case 'h': move_cursor_left(ed); break;
        case 'j': move_cursor_down(ed); break;
        case 'k': move_cursor_up(ed); break;
        case 'l': move_cursor_right(ed); break;
        case 'w':
            buf->cursor.cursor_col = word_right(ed, buf->cursor.cursor_line,
                                                buf->cursor.cursor_col);
            break;
        case 'b':
            buf->cursor.cursor_col = word_left(ed, buf->cursor.cursor_line,
                                               buf->cursor.cursor_col);
            break;
        case 'e':
            buf->cursor.cursor_col = word_end(ed, buf->cursor.cursor_line,
                                              buf->cursor.cursor_col);
            break;
        case '0':
            buf->cursor.cursor_col = 0;
            break;
        case '$':
            buf->cursor.cursor_col = buffer_line_length(buf, buf->cursor.cursor_line);
            break;
        case '^':
            buf->cursor.cursor_col = 0;
            {
                const char* ln = buf->text.lines[buf->cursor.cursor_line];
                size_t i = 0;
                while (ln && ln[i] && (ln[i] == ' ' || ln[i] == '\t')) i++;
                buf->cursor.cursor_col = i;
            }
            break;
        case 'g':
            ed->pending[0] = 'g';
            ed->pending_len = 1;
            return;
        case 'G':
            goto_line(ed, buf->text.line_count > 0 ? buf->text.line_count - 1 : 0);
            break;
        case 'x':
            if (buf->cursor.cursor_col < buffer_line_length(buf, buf->cursor.cursor_line)) {
                yank_text(buf->text.lines[buf->cursor.cursor_line] + buf->cursor.cursor_col, 1);
                buffer_remove_char(buf, buf->cursor.cursor_line, buf->cursor.cursor_col);
                buffer_validate_cursor(buf);
                buf->render_valid = false;
            }
            break;
        case 'X':
            if (buf->cursor.cursor_col > 0) {
                yank_text(buf->text.lines[buf->cursor.cursor_line] + buf->cursor.cursor_col - 1, 1);
                buffer_remove_char(buf, buf->cursor.cursor_line, buf->cursor.cursor_col - 1);
                buf->cursor.cursor_col--;
                buf->render_valid = false;
            }
            break;
        case 'd':
            if (ed->pending_len == 0 || (ed->pending_len == 1 && ed->pending[0] == 'd')) {
                if (ed->pending_len == 0) {
                    ed->pending[0] = 'd';
                    ed->pending_len = 1;
                    return;
                }
                ed->pending_len = 0;
                if (buf->text.line_count > 1) {
                    yank_text(buf->text.lines[buf->cursor.cursor_line],
                              buffer_line_length(buf, buf->cursor.cursor_line));
                    yank_text(s_yank, s_yank_len);
                    s_yank[s_yank_len] = '\n';
                    s_yank_len++;
                    s_yank[s_yank_len] = '\0';
                    if (buf->cursor.cursor_line < buf->text.line_count - 1) {
                        buffer_join_lines(buf, buf->cursor.cursor_line);
                    } else {
                        buffer_remove_range(buf, buf->cursor.cursor_line, 0,
                                            buffer_line_length(buf, buf->cursor.cursor_line));
                        if (buf->cursor.cursor_line + 1 < buf->text.line_count) {
                        }
                    }
                } else {
                    buffer_remove_range(buf, 0, 0, buffer_line_length(buf, 0));
                }
                buf->render_valid = false;
                break;
            }
            ed->pending_len = 0;
            break;
        case 'a':
            move_cursor_right(ed);
            editor_set_mode(ed, QICTO_MODE_INSERT);
            break;
        case 'A':
            buf->cursor.cursor_col = buffer_line_length(buf, buf->cursor.cursor_line);
            editor_set_mode(ed, QICTO_MODE_INSERT);
            break;
        case 'o': {
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
        case 'n':
            if (ed->current_buffer) {
                if (s_last_search[0]) {
                    if (find_substr(ed, s_last_search, 0) == 0) {
                        editor_set_status(ed, "found: %s", s_last_search);
                    } else {
                        editor_set_status(ed, "not found: %s", s_last_search);
                    }
                } else {
                    editor_set_status(ed, "no previous search");
                }
            }
            break;
        case 'N':
            if (ed->current_buffer) {
                if (s_last_search[0]) {
                    if (find_substr(ed, s_last_search, 1) == 0) {
                        editor_set_status(ed, "found: %s", s_last_search);
                    } else {
                        editor_set_status(ed, "not found: %s", s_last_search);
                    }
                } else {
                    editor_set_status(ed, "no previous search");
                }
            }
            break;
        case 'u':
            if (ed->current_buffer) {
                if (buffer_undo(ed->current_buffer) == 0) {
                    editor_set_status(ed, "undo");
                } else {
                    editor_set_status(ed, "nothing to undo");
                }
            }
            break;
        case 'U':
            if (ed->current_buffer) {
                editor_set_status(ed, "redo not implemented");
            }
            break;
        case 'q':
            editor_quit(ed);
            break;
        case 'B':
            editor_cycle_buffer(ed, -1);
            break;
        case 0x0E:
            editor_cycle_buffer(ed, 1);
            break;
        case 0x10:
            editor_cycle_buffer(ed, -1);
            break;
        case 'p':
            if (s_yank_len > 0) {
                bool ends_nl = (s_yank[s_yank_len - 1] == '\n');
                size_t paste_len = ends_nl ? s_yank_len - 1 : s_yank_len;
                if (paste_len > 0) {
                    char tmp[QICTO_MAX_LINE_LEN];
                    if (paste_len >= sizeof(tmp)) paste_len = sizeof(tmp) - 1;
                    memcpy(tmp, s_yank, paste_len);
                    tmp[paste_len] = '\0';
                    buffer_insert_text(buf, buf->cursor.cursor_line,
                                       buf->cursor.cursor_col, tmp);
                    buf->cursor.cursor_col += paste_len;
                }
                if (ends_nl) {
                    buffer_split_line(buf, buf->cursor.cursor_line,
                                      buf->cursor.cursor_col);
                    buf->cursor.cursor_line++;
                    buf->cursor.cursor_col = 0;
                }
                buf->render_valid = false;
            }
            break;
        case QICTO_KEY_PAGE_UP:
            move_page(ed, -1);
            break;
        case QICTO_KEY_PAGE_DOWN:
            move_page(ed, 1);
            break;
        case QICTO_KEY_HOME:
            buf->cursor.cursor_col = 0;
            break;
        case QICTO_KEY_END:
            buf->cursor.cursor_col = buffer_line_length(buf, buf->cursor.cursor_line);
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
    if (key == QICTO_KEY_LEFT || key == QICTO_KEY_RIGHT ||
        key == QICTO_KEY_UP   || key == QICTO_KEY_DOWN ||
        key == QICTO_KEY_HOME || key == QICTO_KEY_END) {
        if (key == QICTO_KEY_LEFT)  move_cursor_left(ed);
        if (key == QICTO_KEY_RIGHT) move_cursor_right(ed);
        if (key == QICTO_KEY_UP)    move_cursor_up(ed);
        if (key == QICTO_KEY_DOWN)  move_cursor_down(ed);
        if (key == QICTO_KEY_HOME && ed->current_buffer) ed->current_buffer->cursor.cursor_col = 0;
        if (key == QICTO_KEY_END && ed->current_buffer) {
            ed->current_buffer->cursor.cursor_col =
                buffer_line_length(ed->current_buffer, ed->current_buffer->cursor.cursor_line);
        }
        return;
    }
    handle_insert_char(ed, key);
    if (ed->mods && ed->current_buffer) {
        mod_registry_on_buffer_changed(ed->mods, ed, ed->current_buffer);
    }
}

static void delete_selection(editor_t* ed) {
    if (!ed || !ed->current_buffer) return;
    buffer_t* buf = ed->current_buffer;
    if (!buf->cursor.has_selection) return;
    size_t al = buf->cursor.sel_anchor_line;
    size_t ac = buf->cursor.sel_anchor_col;
    size_t cl = buf->cursor.cursor_line;
    size_t cc = buf->cursor.cursor_col;
    if (al > cl || (al == cl && ac > cc)) {
        size_t t;
        t = al; al = cl; cl = t;
        t = ac; ac = cc; cc = t;
    }
    if (al == cl) {
        yank_text(buf->text.lines[al] + ac, cc - ac);
    } else {
        size_t first_len = buffer_line_length(buf, al) - ac;
        size_t yi = 0;
        if (first_len >= sizeof(s_yank)) first_len = sizeof(s_yank) - 1;
        memcpy(s_yank + yi, buf->text.lines[al] + ac, first_len);
        yi += first_len;
        for (size_t i = al + 1; i < cl && yi + 1 < sizeof(s_yank); i++) {
            s_yank[yi++] = '\n';
            size_t llen = buffer_line_length(buf, i);
            if (llen >= sizeof(s_yank) - yi) llen = sizeof(s_yank) - yi - 1;
            memcpy(s_yank + yi, buf->text.lines[i], llen);
            yi += llen;
        }
        if (yi + 1 < sizeof(s_yank)) s_yank[yi++] = '\n';
        size_t last_len = cc;
        if (last_len >= sizeof(s_yank) - yi) last_len = sizeof(s_yank) - yi - 1;
        memcpy(s_yank + yi, buf->text.lines[cl], last_len);
        yi += last_len;
        s_yank[yi] = '\0';
        s_yank_len = yi;
    }
    if (al == cl) {
        buffer_remove_range(buf, al, ac, cc - ac);
    } else {
        buffer_remove_range(buf, al, ac,
                            buffer_line_length(buf, al) - ac);
        for (size_t i = al + 1; i < cl; ) {
            buffer_join_lines(buf, al);
            cl--;
        }
        buffer_join_lines(buf, al);
        buffer_remove_range(buf, al, 0, cc);
    }
    buf->cursor.cursor_line = al;
    buf->cursor.cursor_col = ac;
    buf->cursor.has_selection = false;
    buf->render_valid = false;
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
        case 'w':
            buf->cursor.cursor_col = word_right(ed, buf->cursor.cursor_line,
                                                buf->cursor.cursor_col);
            break;
        case 'b':
            buf->cursor.cursor_col = word_left(ed, buf->cursor.cursor_line,
                                               buf->cursor.cursor_col);
            break;
        case '0':
            buf->cursor.cursor_col = 0;
            break;
        case '$':
            buf->cursor.cursor_col = buffer_line_length(buf, buf->cursor.cursor_line);
            break;
        case 'v':
            ed->mode = QICTO_MODE_NORMAL;
            buf->cursor.has_selection = false;
            return;
        case 'x':
        case 'd':
            delete_selection(ed);
            ed->mode = QICTO_MODE_NORMAL;
            return;
        case 'y':
            {
                size_t al = buf->cursor.sel_anchor_line;
                size_t ac = buf->cursor.sel_anchor_col;
                size_t cl = buf->cursor.cursor_line;
                size_t cc = buf->cursor.cursor_col;
                if (al > cl || (al == cl && ac > cc)) {
                    size_t t;
                    t = al; al = cl; cl = t;
                    t = ac; ac = cc; cc = t;
                }
                if (al == cl) {
                    yank_text(buf->text.lines[al] + ac, cc - ac);
                } else {
                    size_t first_len = buffer_line_length(buf, al) - ac;
                    size_t yi = 0;
                    if (first_len >= sizeof(s_yank)) first_len = sizeof(s_yank) - 1;
                    memcpy(s_yank + yi, buf->text.lines[al] + ac, first_len);
                    yi += first_len;
                    for (size_t i = al + 1; i < cl && yi + 1 < sizeof(s_yank); i++) {
                        s_yank[yi++] = '\n';
                        size_t llen = buffer_line_length(buf, i);
                        if (llen >= sizeof(s_yank) - yi) llen = sizeof(s_yank) - yi - 1;
                        memcpy(s_yank + yi, buf->text.lines[i], llen);
                        yi += llen;
                    }
                    if (yi + 1 < sizeof(s_yank)) s_yank[yi++] = '\n';
                    size_t last_len = cc;
                    if (last_len >= sizeof(s_yank) - yi) last_len = sizeof(s_yank) - yi - 1;
                    memcpy(s_yank + yi, buf->text.lines[cl], last_len);
                    yi += last_len;
                    s_yank[yi] = '\0';
                    s_yank_len = yi;
                }
                editor_set_status(ed, "yanked %zu bytes", s_yank_len);
            }
            ed->mode = QICTO_MODE_NORMAL;
            buf->cursor.has_selection = false;
            return;
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
        bool was_search = (ed->mode == QICTO_MODE_SEARCH);
        if (was_search && cmd[0]) {
            strncpy(s_last_search, cmd, sizeof(s_last_search) - 1);
            s_last_search[sizeof(s_last_search) - 1] = '\0';
            if (find_substr(ed, cmd, 0) == 0) {
                editor_set_status(ed, "found: %s", cmd);
            } else {
                editor_set_status(ed, "not found: %s", cmd);
            }
        } else if (cmd[0]) {
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

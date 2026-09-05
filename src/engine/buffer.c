#include "buffer.h"
#include "utils/strings.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <cwalk.h>
#include <utf8proc.h>

static uint32_t _next_buf_id = 1;

/* Allocate a buffer with a single empty line. Caller must free the returned
 * string when done. Thin wrapper used in the few spots where we need a
 * heap-allocated empty string without going through the utils module. */
static char* qicto_strdup_empty(void) {
    char* r = malloc(1);
    if (r) r[0] = '\0';
    return r;
}

static void buffer_ensure_capacity(buffer_t* buf, size_t needed) {
    while (buf->text.capacity < needed) {
        size_t newcap = buf->text.capacity ? buf->text.capacity * 2 : 16;
        while (newcap < needed) newcap *= 2;
        buf->text.lines = realloc(buf->text.lines, newcap * sizeof(char*));
        assert(buf->text.lines);
        buf->text.capacity = newcap;
    }
}

static void buffer_detect_syntax(buffer_t* buf, const char* filename) {
    if (!buf || !filename) return;
    const char* ext = strrchr(filename, '.');
    if (!ext) {
        buf->syntax = QICTO_SYNTAX_UNKNOWN;
        return;
    }
    if (strcasecmp(ext, ".c") == 0 || strcasecmp(ext, ".h") == 0)
        buf->syntax = QICTO_SYNTAX_C;
    else if (strcasecmp(ext, ".cpp") == 0 || strcasecmp(ext, ".cc") == 0 || strcasecmp(ext, ".hpp") == 0)
        buf->syntax = QICTO_SYNTAX_CPP;
    else if (strcasecmp(ext, ".py") == 0)
        buf->syntax = QICTO_SYNTAX_PYTHON;
    else if (strcasecmp(ext, ".js") == 0 || strcasecmp(ext, ".ts") == 0 || strcasecmp(ext, ".jsx") == 0 || strcasecmp(ext, ".tsx") == 0)
        buf->syntax = QICTO_SYNTAX_JS;
    else if (strcasecmp(ext, ".go") == 0)
        buf->syntax = QICTO_SYNTAX_GO;
    else if (strcasecmp(ext, ".rs") == 0)
        buf->syntax = QICTO_SYNTAX_RUST;
    else if (strcasecmp(ext, ".html") == 0 || strcasecmp(ext, ".htm") == 0)
        buf->syntax = QICTO_SYNTAX_HTML;
    else if (strcasecmp(ext, ".css") == 0)
        buf->syntax = QICTO_SYNTAX_CSS;
    else if (strcasecmp(ext, ".json") == 0)
        buf->syntax = QICTO_SYNTAX_JSON;
    else if (strcasecmp(ext, ".yaml") == 0 || strcasecmp(ext, ".yml") == 0)
        buf->syntax = QICTO_SYNTAX_YAML;
    else if (strcasecmp(ext, ".toml") == 0)
        buf->syntax = QICTO_SYNTAX_TOML;
    else if (strcasecmp(ext, ".md") == 0)
        buf->syntax = QICTO_SYNTAX_MARKDOWN;
    else if (strcasecmp(ext, ".sh") == 0 || strcasecmp(ext, ".bash") == 0)
        buf->syntax = QICTO_SYNTAX_SHELL;
    else if (strcasecmp(ext, ".dockerfile") == 0 || strncasecmp(ext, ".dockerfile", 12) == 0)
        buf->syntax = QICTO_SYNTAX_DOCKERFILE;
    else
        buf->syntax = QICTO_SYNTAX_UNKNOWN;
}

buffer_t* buffer_new(const char* filename) {
    buffer_t* buf = calloc(1, sizeof(buffer_t));
    if (!buf) return NULL;

    buf->id = _next_buf_id++;
    buf->cursor.cursor_line = 0;
    buf->cursor.cursor_col = 0;
    buf->cursor.sel_anchor_line = 0;
    buf->cursor.sel_anchor_col = 0;
    buf->cursor.has_selection = false;

    if (filename) {
        strncpy(buf->filename, filename, QICTO_MAX_PATH_LEN - 1);
        buf->filename[QICTO_MAX_PATH_LEN - 1] = '\0';
    }

    buffer_ensure_capacity(buf, 1);
    buf->text.lines[0] = qicto_strdup_empty();
    buf->text.line_count = 1;
    buf->btype = QICTO_BUFTYPE_TEXT;
    buf->dirty = false;
    buf->readonly = false;
    buf->render_valid = false;
    buf->render_lines = NULL;
    buf->render_line_count = 0;

    buffer_detect_syntax(buf, filename);

    return buf;
}

static void free_render_lines(buffer_t* buf) {
    if (!buf->render_lines) return;
    for (size_t i = 0; i < buf->render_line_count; i++) {
        if (buf->render_lines[i].cells) {
            free(buf->render_lines[i].cells);
        }
    }
    free(buf->render_lines);
    buf->render_lines = NULL;
    buf->render_line_count = 0;
}

void buffer_free(buffer_t* buf) {
    if (!buf) return;
    free_render_lines(buf);
    if (buf->text.lines) {
        for (size_t i = 0; i < buf->text.line_count; i++) {
            if (buf->text.lines[i]) free(buf->text.lines[i]);
        }
        free(buf->text.lines);
    }
    free(buf);
}

int buffer_load_file(buffer_t* buf, const char* filename) {
    if (!buf || !filename) return -1;

    FILE* f = fopen(filename, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* data = malloc(fsize + 1);
    if (!data) { fclose(f); return -1; }

    size_t rd = fread(data, 1, fsize, f);
    fclose(f);
    data[rd] = '\0';

    for (size_t i = 0; i < buf->text.line_count; i++) {
        free(buf->text.lines[i]);
    }
    buf->text.line_count = 0;

    if (rd == 0) {
        buffer_ensure_capacity(buf, 1);
        buf->text.lines[0] = qicto_strdup_empty();
        buf->text.line_count = 1;
        free(data);
        return 0;
    }

    size_t line_idx = 0;
    char* p = data;
    char* line_start = p;
    buf->text.line_count = 0;

    while (line_idx < buf->text.capacity && *p) {
        if (*p == '\n') {
            *p = '\0';
            size_t len = p - line_start;
            if (len > 0 && line_start[len - 1] == '\r') {
                line_start[len - 1] = '\0';
                len--;
            }
            buffer_ensure_capacity(buf, line_idx + 1);
            buf->text.lines[line_idx++] = qstr_ndup(line_start, len);
            line_start = p + 1;
        }
        p++;
    }

    buffer_ensure_capacity(buf, line_idx + 1);
    size_t remaining = p - line_start;
    if (remaining > 0 || line_idx == 0) {
        buf->text.lines[line_idx++] = qstr_ndup(line_start, remaining);
    }
    buf->text.line_count = line_idx;

    free(data);
    buf->dirty = false;

    buffer_detect_syntax(buf, filename);

    const char* base;
    size_t base_len;
    cwk_path_get_basename(filename, &base, &base_len);
    if (base) {
        strncpy(buf->display_name, base, QICTO_MAX_NAME_LEN - 1);
    }
    if (buf->display_name[0] == '\0' && filename) {
        strncpy(buf->display_name, filename, QICTO_MAX_NAME_LEN - 1);
    }

    buf->render_valid = false;
    return 0;
}

int buffer_save(buffer_t* buf, const char* filename) {
    if (!buf || !filename) return -1;

    FILE* f = fopen(filename, "wb");
    if (!f) return -1;

    for (size_t i = 0; i < buf->text.line_count; i++) {
        if (buf->text.lines[i]) {
            fputs(buf->text.lines[i], f);
        }
        if (i < buf->text.line_count - 1) {
            fputc('\n', f);
        }
    }

    fclose(f);
    buf->dirty = false;
    buf->last_save = time(NULL);
    buf->render_valid = false;
    return 0;
}

void buffer_insert_char(buffer_t* buf, size_t line, size_t col, char c) {
    if (!buf || line >= buf->text.line_count) return;

    char* ln = buf->text.lines[line];
    size_t len = strlen(ln);

    if (col > len) col = len;

    char* new_line = malloc(len + 2);
    memcpy(new_line, ln, col);
    new_line[col] = c;
    memcpy(new_line + col + 1, ln + col, len - col + 1);
    free(ln);
    buf->text.lines[line] = new_line;

    buf->dirty = true;
    buf->render_valid = false;
}

void buffer_insert_text(buffer_t* buf, size_t line, size_t col, const char* text) {
    if (!buf || !text || line >= buf->text.line_count) return;

    char* ln = buf->text.lines[line];
    size_t len = strlen(ln);

    if (col > len) col = len;

    size_t text_len = strlen(text);
    char* new_line = malloc(len + text_len + 1);
    memcpy(new_line, ln, col);
    memcpy(new_line + col, text, text_len);
    memcpy(new_line + col + text_len, ln + col, len - col + 1);
    free(ln);
    buf->text.lines[line] = new_line;

    buf->dirty = true;
    buf->render_valid = false;
}

void buffer_remove_char(buffer_t* buf, size_t line, size_t col) {
    if (!buf || line >= buf->text.line_count) return;

    char* ln = buf->text.lines[line];
    size_t len = strlen(ln);

    if (col >= len) return;

    memmove(ln + col, ln + col + 1, len - col);
    buf->dirty = true;
    buf->render_valid = false;
}

void buffer_remove_range(buffer_t* buf, size_t line, size_t col, size_t rlen) {
    if (!buf || line >= buf->text.line_count) return;

    char* ln = buf->text.lines[line];
    size_t len = strlen(ln);

    if (col >= len) return;
    if (col + rlen > len) rlen = len - col;

    memmove(ln + col, ln + col + rlen, len - col - rlen + 1);
    buf->dirty = true;
    buf->render_valid = false;
}

void buffer_split_line(buffer_t* buf, size_t line, size_t col) {
    if (!buf || line >= buf->text.line_count) return;

    buffer_ensure_capacity(buf, buf->text.line_count + 1);

    char* ln = buf->text.lines[line];
    size_t len = strlen(ln);
    if (col > len) col = len;

    char* right_part = qstr_dup(ln + col);
    ln[col] = '\0';

    memmove(buf->text.lines + line + 2, buf->text.lines + line + 1,
            (buf->text.line_count - line) * sizeof(char*));
    buf->text.lines[line + 1] = right_part;
    buf->text.line_count++;

    buf->dirty = true;
    buf->render_valid = false;
}

void buffer_join_lines(buffer_t* buf, size_t line) {
    if (!buf || line + 1 >= buf->text.line_count) return;

    char* ln = buf->text.lines[line];
    char* next = buf->text.lines[line + 1];
    size_t len1 = strlen(ln);
    size_t len2 = strlen(next);

    ln = realloc(ln, len1 + len2 + 1);
    memcpy(ln + len1, next, len2 + 1);
    buf->text.lines[line] = ln;
    free(next);

    memmove(buf->text.lines + line + 1, buf->text.lines + line + 2,
            (buf->text.line_count - line - 1) * sizeof(char*));
    buf->text.line_count--;

    buf->dirty = true;
    buf->render_valid = false;
}

size_t buffer_line_length(buffer_t* buf, size_t line) {
    if (!buf || line >= buf->text.line_count) return 0;
    return strlen(buf->text.lines[line]);
}

const char* buffer_get_line(buffer_t* buf, size_t line) {
    if (!buf || line >= buf->text.line_count) return NULL;
    return buf->text.lines[line];
}


void buffer_update_render(buffer_t* buf) {
    if (!buf || buf->render_valid) return;
    if (!buf->render_lines) return;

    free_render_lines(buf);

    buf->render_line_count = buf->text.line_count;
    buf->render_lines = calloc(buf->render_line_count, sizeof(qicto_render_line_t));
    if (!buf->render_lines) return;

    for (size_t i = 0; i < buf->text.line_count; i++) {
        const char* ln = buf->text.lines[i];
        if (!ln) continue;

        size_t utf8_len = strlen(ln);
        utf8proc_uint8_t* src = (utf8proc_uint8_t*)ln;
        utf8proc_uint8_t* dst = NULL;
        utf8proc_ssize_t out_len = utf8proc_map(src, utf8_len, &dst,
            (utf8proc_option_t)(UTF8PROC_STRIPCC | UTF8PROC_STRIPMARK));

        if (out_len > 0 && dst) {
            size_t n = out_len;
            qicto_render_line_t* rl = &buf->render_lines[i];
            rl->count = n;
            rl->capacity = n;
            rl->cells = calloc(n > 0 ? n : 1, sizeof(qicto_cell_t));

            for (size_t j = 0; j < n; j++) {
                rl->cells[j].cp = dst[j];
                rl->cells[j].syntax_group = 0;
                rl->cells[j].style_mask = 0;
            }
            free(dst);
        } else {
            buf->render_lines[i].cells = NULL;
            buf->render_lines[i].count = 0;
            buf->render_lines[i].capacity = 0;
        }
    }

    buf->render_valid = true;
}

void buffer_set_cursor(buffer_t* buf, size_t line, size_t col) {
    if (!buf) return;
    if (line >= buf->text.line_count) line = buf->text.line_count - 1;

    size_t linelen = strlen(buf->text.lines[line]);
    if (col > linelen) col = linelen;

    buf->cursor.cursor_line = line;
    buf->cursor.cursor_col = col;
    buf->cursor.cursor_byte = col;
}

void buffer_validate_cursor(buffer_t* buf) {
    if (!buf) return;
    if (buf->cursor.cursor_line >= buf->text.line_count) {
        buf->cursor.cursor_line = buf->text.line_count - 1;
    }
    size_t linelen = strlen(buf->text.lines[buf->cursor.cursor_line]);
    if (buf->cursor.cursor_col > linelen) {
        buf->cursor.cursor_col = linelen;
    }
}

const char* buffer_display_name(buffer_t* buf) {
    if (!buf) return "(null)";
    return buf->display_name[0] ? buf->display_name : (buf->filename[0] ? buf->filename : "[No Name]");
}

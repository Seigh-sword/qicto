#include "session.h"
#include "buffer.h"
#include "editor.h"
#include "fileops.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <unistd.h>
#define MKDIR(p) mkdir((p), 0755)
#endif

static int mkdir_p(const char* path) {
    if (!path || !*path) return -1;
    char tmp[QICTO_MAX_PATH_LEN];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t n = strlen(tmp);
    if (n == 0) return -1;
    if (tmp[n-1] == '/') tmp[n-1] = '\0';
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (MKDIR(tmp) != 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    if (MKDIR(tmp) != 0 && errno != EEXIST) return -1;
    return 0;
}

static void json_escape(FILE* f, const char* s) {
    if (!s) return;
    while (*s) {
        unsigned char c = (unsigned char)*s++;
        switch (c) {
            case '"':  fputs("\\\"", f); break;
            case '\\': fputs("\\\\", f); break;
            case '\b': fputs("\\b", f); break;
            case '\f': fputs("\\f", f); break;
            case '\n': fputs("\\n", f); break;
            case '\r': fputs("\\r", f); break;
            case '\t': fputs("\\t", f); break;
            default:
                if (c < 0x20) {
                    fprintf(f, "\\u%04x", c);
                } else {
                    fputc(c, f);
                }
        }
    }
}

char* session_find_project_dir(const char* start_dir) {
    if (!start_dir || !*start_dir) return NULL;
    char cur[QICTO_MAX_PATH_LEN];
    snprintf(cur, sizeof(cur), "%s", start_dir);
    for (int i = 0; i < 32; i++) {
        char probe[QICTO_MAX_PATH_LEN];
        snprintf(probe, sizeof(probe), "%s/.qicto/project.json", cur);
        FILE* fp = fopen(probe, "r");
        if (fp) {
            fclose(fp);
            char* out = strdup(cur);
            return out;
        }
        char* slash = strrchr(cur, '/');
#ifdef _WIN32
        char* bslash = strrchr(cur, '\\');
        if (!slash || (bslash && bslash > slash)) slash = bslash;
#endif
        if (!slash) break;
        *slash = '\0';
    }
    return NULL;
}

int session_save(editor_t* ed, const char* project_dir) {
    if (!ed || !project_dir || !*project_dir) return -1;
    char qicto_dir[QICTO_MAX_PATH_LEN];
    snprintf(qicto_dir, sizeof(qicto_dir), "%s/.qicto", project_dir);
    if (mkdir_p(qicto_dir) != 0) return -1;

    char path[QICTO_MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/project.json", qicto_dir);

    char tmppath[QICTO_MAX_PATH_LEN];
    snprintf(tmppath, sizeof(tmppath), "%s.tmp", path);

    FILE* f = fopen(tmppath, "w");
    if (!f) return -1;

    fprintf(f, "{\n");
    fprintf(f, "  \"version\": %d,\n", QICTO_SESSION_VERSION);
    fprintf(f, "  \"active\": \"");
    if (ed->current_buffer && ed->current_buffer->filename[0]) {
        json_escape(f, ed->current_buffer->filename);
    }
    fprintf(f, "\",\n");
    fprintf(f, "  \"buffers\": [\n");

    buffer_t* buf = ed->buffers;
    bool first = true;
    while (buf) {
        if (!first) fprintf(f, ",\n");
        first = false;
        fprintf(f, "    {\"path\": \"");
        json_escape(f, buf->filename);
        fprintf(f, "\", \"line\": %zu, \"col\": %zu, \"top\": %zu, \"col_off\": %zu, \"dirty\": %s}",
                buf->cursor.cursor_line,
                buf->cursor.cursor_col,
                ed->top_line,
                ed->col_offset,
                buf->dirty ? "true" : "false");
        buf = buf->next;
    }

    fprintf(f, "\n  ]\n");
    fprintf(f, "}\n");
    fclose(f);

#ifdef _WIN32
    _unlink(path);
#endif
    if (rename(tmppath, path) != 0) {
        unlink(tmppath);
        return -1;
    }
    return 0;
}

/* Tiny JSON reader sufficient for our session format. Skips whitespace,
 * expects {"key": <string|number|bool>, ...}. Returns 0 on success. */
static int jskip_ws(FILE* f) {
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (!isspace(c)) { ungetc(c, f); return c; }
    }
    return -1;
}
static int jexpect(FILE* f, char want) {
    int c;
    while ((c = fgetc(f)) != EOF && isspace(c)) {}
    if (c != want) return -1;
    return 0;
}
static int jread_string(FILE* f, char* out, size_t cap) {
    int c;
    while ((c = fgetc(f)) != EOF && isspace(c)) {}
    if (c != '"') return -1;
    size_t i = 0;
    while ((c = fgetc(f)) != EOF && c != '"') {
        if (c == '\\') {
            int esc = fgetc(f);
            if (esc == EOF) return -1;
            switch (esc) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '\\': c = '\\'; break;
                case '"': c = '"'; break;
                case '/': c = '/'; break;
                default: c = esc; break;
            }
        }
        if (i + 1 < cap) out[i++] = (char)c;
    }
    if (i < cap) out[i] = '\0';
    return 0;
}
static int jread_number(FILE* f, long* out) {
    char buf[32];
    size_t i = 0;
    int c;
    while ((c = fgetc(f)) != EOF && i + 1 < sizeof(buf)) {
        if (c == ',' || c == '}' || c == ']' || isspace(c)) {
            ungetc(c, f);
            break;
        }
        buf[i++] = (char)c;
    }
    buf[i] = '\0';
    char* endp = NULL;
    long v = strtol(buf, &endp, 10);
    if (endp == buf) return -1;
    *out = v;
    return 0;
}
static int jread_bool(FILE* f, bool* out) {
    char buf[8] = {0};
    size_t i = 0;
    int c;
    while ((c = fgetc(f)) != EOF && i + 1 < sizeof(buf)) {
        if (c == ',' || c == '}' || c == ']' || isspace(c)) {
            ungetc(c, f);
            break;
        }
        buf[i++] = (char)c;
    }
    buf[i] = '\0';
    if (strcmp(buf, "true") == 0)  { *out = true;  return 0; }
    if (strcmp(buf, "false") == 0) { *out = false; return 0; }
    return -1;
}

int session_load(editor_t* ed, const char* project_dir) {
    if (!ed || !project_dir || !*project_dir) return -1;
    char path[QICTO_MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/.qicto/project.json", project_dir);
    FILE* f = fopen(path, "r");
    if (!f) return -1;

    if (jexpect(f, '{') != 0) { fclose(f); return -1; }

    char active_path[QICTO_MAX_PATH_LEN] = {0};
    bool have_active = false;
    long version = 0;

    while (1) {
        int c = jskip_ws(f);
        if (c == -1) break;
        if (c == '}') break;
        char key[64];
        if (jread_string(f, key, sizeof(key)) != 0) break;
        if (jexpect(f, ':') != 0) break;

        if (strcmp(key, "version") == 0) {
            if (jread_number(f, &version) != 0) break;
        } else if (strcmp(key, "active") == 0) {
            if (jread_string(f, active_path, sizeof(active_path)) != 0) break;
            have_active = true;
        } else if (strcmp(key, "buffers") == 0) {
            if (jexpect(f, '[') != 0) break;
            while (1) {
                int bc = jskip_ws(f);
                if (bc == -1 || bc == ']') break;
                if (bc != '{') break;
                char bpath[QICTO_MAX_PATH_LEN] = {0};
                long line = 0, col = 0, top = 0, col_off = 0;
                bool dirty = false;
                while (1) {
                    int kc = jskip_ws(f);
                    if (kc == -1 || kc == '}') break;
                    char bkey[64];
                    if (jread_string(f, bkey, sizeof(bkey)) != 0) break;
                    if (jexpect(f, ':') != 0) break;
                    if (strcmp(bkey, "path") == 0) {
                        if (jread_string(f, bpath, sizeof(bpath)) != 0) break;
                    } else if (strcmp(bkey, "line") == 0) {
                        if (jread_number(f, &line) != 0) break;
                    } else if (strcmp(bkey, "col") == 0) {
                        if (jread_number(f, &col) != 0) break;
                    } else if (strcmp(bkey, "top") == 0) {
                        if (jread_number(f, &top) != 0) break;
                    } else if (strcmp(bkey, "col_off") == 0) {
                        if (jread_number(f, &col_off) != 0) break;
                    } else if (strcmp(bkey, "dirty") == 0) {
                        if (jread_bool(f, &dirty) != 0) break;
                    }
                    int sep = jskip_ws(f);
                    if (sep == ',') continue;
                    if (sep == '}') break;
                }
                if (bpath[0]) {
                    buffer_t* nb = editor_open_file(ed, bpath);
                    if (nb) {
                        nb->cursor.cursor_line = (size_t)line;
                        nb->cursor.cursor_col = (size_t)col;
                        nb->dirty = dirty;
                    }
                }
                int after = jskip_ws(f);
                if (after == ',') continue;
                if (after == ']' || after == -1) break;
            }
        }

        int sep = jskip_ws(f);
        if (sep == ',') continue;
        if (sep == '}') break;
    }

    fclose(f);

    if (have_active && active_path[0]) {
        buffer_t* b = ed->buffers;
        while (b) {
            if (strcmp(b->filename, active_path) == 0) {
                ed->current_buffer = b;
                break;
            }
            b = b->next;
        }
    }
    return 0;
}

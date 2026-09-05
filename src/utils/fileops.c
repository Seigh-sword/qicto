#define _POSIX_C_SOURCE 200809L
#include "fileops.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>
#include <cwalk.h>

int fileops_read_text(const char* path, char** out_content, size_t* out_size) {
    if (!path || !out_content || !out_size) return -1;
    *out_content = NULL;
    *out_size = 0;

    FILE* f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return -1; }

    char* data = malloc(sz + 1);
    if (!data) { fclose(f); return -1; }

    size_t rd = fread(data, 1, sz, f);
    fclose(f);
    data[rd] = '\0';

    *out_content = data;
    *out_size = rd;
    return 0;
}

int fileops_write_text(const char* path, const char* content, size_t size) {
    if (!path || !content) return -1;
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    size_t wr = fwrite(content, 1, size, f);
    fclose(f);
    return (wr == size) ? 0 : -1;
}

int fileops_read_lines(const char* path, char*** out_lines, size_t* out_count) {
    if (!path || !out_lines || !out_count) return -1;
    *out_lines = NULL;
    *out_count = 0;

    char* content = NULL;
    size_t size = 0;
    if (fileops_read_text(path, &content, &size) != 0) return -1;

    size_t line_count = 0;
    size_t capacity = 16;
    char** lines = malloc(capacity * sizeof(char*));

    char* p = content;
    char* line_start = p;
    while (*p) {
        if (*p == '\n') {
            *p = '\0';
            if (p > line_start && p[-1] == '\r') p[-1] = '\0';
            if (line_count >= capacity) {
                capacity *= 2;
                lines = realloc(lines, capacity * sizeof(char*));
            }
            lines[line_count++] = strdup(line_start);
            line_start = p + 1;
        }
        p++;
    }

    size_t remaining = p - line_start;
    if (remaining > 0 || line_count == 0) {
        if (line_count >= capacity) {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(char*));
        }
        lines[line_count++] = strdup(line_start);
    }

    free(content);
    *out_lines = lines;
    *out_count = line_count;
    return 0;
}

qicto_syntax_t fileops_detect_syntax(const char* filename) {
    if (!filename) return QICTO_SYNTAX_UNKNOWN;
    const char* ext = strrchr(filename, '.');
    if (!ext) return QICTO_SYNTAX_UNKNOWN;

    if (strcasecmp(ext, ".c") == 0 || strcasecmp(ext, ".h") == 0)
        return QICTO_SYNTAX_C;
    else if (strcasecmp(ext, ".cpp") == 0 || strcasecmp(ext, ".cc") == 0 ||
             strcasecmp(ext, ".cxx") == 0 || strcasecmp(ext, ".hpp") == 0 ||
             strcasecmp(ext, ".hxx") == 0)
        return QICTO_SYNTAX_CPP;
    else if (strcasecmp(ext, ".py") == 0)
        return QICTO_SYNTAX_PYTHON;
    else if (strcasecmp(ext, ".js") == 0 || strcasecmp(ext, ".mjs") == 0)
        return QICTO_SYNTAX_JS;
    else if (strcasecmp(ext, ".jsx") == 0 || strcasecmp(ext, ".tsx") == 0)
        return QICTO_SYNTAX_JS;
    else if (strcasecmp(ext, ".ts") == 0)
        return QICTO_SYNTAX_JS;
    else if (strcasecmp(ext, ".go") == 0)
        return QICTO_SYNTAX_GO;
    else if (strcasecmp(ext, ".rs") == 0)
        return QICTO_SYNTAX_RUST;
    else if (strcasecmp(ext, ".html") == 0 || strcasecmp(ext, ".htm") == 0)
        return QICTO_SYNTAX_HTML;
    else if (strcasecmp(ext, ".css") == 0)
        return QICTO_SYNTAX_CSS;
    else if (strcasecmp(ext, ".json") == 0)
        return QICTO_SYNTAX_JSON;
    else if (strcasecmp(ext, ".yaml") == 0 || strcasecmp(ext, ".yml") == 0)
        return QICTO_SYNTAX_YAML;
    else if (strcasecmp(ext, ".toml") == 0)
        return QICTO_SYNTAX_TOML;
    else if (strcasecmp(ext, ".md") == 0)
        return QICTO_SYNTAX_MARKDOWN;
    else if (strcasecmp(ext, ".sh") == 0 || strcasecmp(ext, ".bash") == 0 ||
             strcasecmp(ext, ".zsh") == 0)
        return QICTO_SYNTAX_SHELL;
    else if (strcasecmp(ext, ".dockerfile") == 0)
        return QICTO_SYNTAX_DOCKERFILE;
    else if (strcasecmp(ext, ".txt") == 0)
        return QICTO_SYNTAX_UNKNOWN;

    return QICTO_SYNTAX_UNKNOWN;
}

int fileops_is_binary(const char* path) {
    if (!path) return 0;
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    unsigned char buf[4096];
    size_t rd = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    for (size_t i = 0; i < rd; i++) {
        if (buf[i] == 0) return 1;
    }
    return 0;
}

int fileops_backup(const char* path) {
    if (!path) return -1;
    char backup[QICTO_MAX_PATH_LEN];
    snprintf(backup, sizeof(backup), "%s~", path);
    char* content = NULL;
    size_t size = 0;
    if (fileops_read_text(path, &content, &size) != 0) return -1;
    int rc = fileops_write_text(backup, content, size);
    free(content);
    return rc;
}

void fileops_free_lines(char** lines, size_t count) {
    if (!lines) return;
    for (size_t i = 0; i < count; i++) {
        free(lines[i]);
    }
    free(lines);
}

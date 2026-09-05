#include "strings.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <utf8proc.h>

char* qstr_dup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* r = malloc(len + 1);
    if (r) memcpy(r, s, len + 1);
    return r;
}

char* qstr_ndup(const char* s, size_t n) {
    if (!s) return NULL;
    size_t len = strlen(s);
    if (n > len) n = len;
    char* r = malloc(n + 1);
    if (r) {
        memcpy(r, s, n);
        r[n] = '\0';
    }
    return r;
}

char* qstr_concat(const char* a, const char* b) {
    if (!a || !b) return NULL;
    size_t la = strlen(a);
    size_t lb = strlen(b);
    char* r = malloc(la + lb + 1);
    if (r) {
        memcpy(r, a, la);
        memcpy(r + la, b, lb + 1);
    }
    return r;
}

char* qstr_trim(char* s) {
    if (!s) return NULL;
    s = qstr_ltrim(s);
    s = qstr_rtrim(s);
    return s;
}

char* qstr_ltrim(char* s) {
    if (!s) return NULL;
    while (isspace((unsigned char)*s)) {
        memmove(s, s + 1, strlen(s));
    }
    return s;
}

char* qstr_rtrim(char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
    return s;
}

char** qstr_split(const char* s, char delim, int* count) {
    if (!s || !count) return NULL;
    *count = 0;

    int cap = 8;
    char** result = malloc(cap * sizeof(char*));
    if (!result) return NULL;

    const char* start = s;
    while (*s) {
        if (*s == delim) {
            size_t len = s - start;
            if (len > 0) {
                if (*count >= cap) {
                    cap *= 2;
                    result = realloc(result, cap * sizeof(char*));
                }
                result[*count] = qstr_ndup(start, len);
                (*count)++;
            }
            start = s + 1;
        }
        s++;
    }

    size_t len = s - start;
    if (len > 0) {
        if (*count >= cap) {
            cap *= 2;
            result = realloc(result, cap * sizeof(char*));
        }
        result[*count] = qstr_ndup(start, len);
        (*count)++;
    }

    return result;
}

void qstr_free_split(char** arr, int count) {
    if (!arr) return;
    for (int i = 0; i < count; i++) {
        free(arr[i]);
    }
    free(arr);
}

int qstr_starts_with(const char* s, const char* prefix) {
    if (!s || !prefix) return 0;
    size_t plen = strlen(prefix);
    return (strncmp(s, prefix, plen) == 0) ? 1 : 0;
}

int qstr_ends_with(const char* s, const char* suffix) {
    if (!s || !suffix) return 0;
    size_t slen = strlen(s);
    size_t flen = strlen(suffix);
    return (slen >= flen && strcmp(s + slen - flen, suffix) == 0) ? 1 : 0;
}

size_t qstr_display_width(const char* s) {
    if (!s) return 0;
    utf8proc_uint8_t* dst = NULL;
    utf8proc_ssize_t len = utf8proc_map((const utf8proc_uint8_t*)s, (utf8proc_ssize_t)strlen(s), &dst,
        (utf8proc_option_t)UTF8PROC_STRIPCC);
    if (len < 0 || !dst) return strlen(s);

    size_t width = 0;
    for (utf8proc_ssize_t i = 0; i < len; i++) {
        int w = utf8proc_charwidth(dst[i]);
        width += (w >= 0) ? (size_t)w : 0;
    }
    free(dst);
    return width;
}

char* qstr_to_lower(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* r = malloc(len + 1);
    if (r) {
        for (size_t i = 0; i <= len; i++) {
            r[i] = tolower((unsigned char)s[i]);
        }
    }
    return r;
}

char* qstr_to_upper(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* r = malloc(len + 1);
    if (r) {
        for (size_t i = 0; i <= len; i++) {
            r[i] = toupper((unsigned char)s[i]);
        }
    }
    return r;
}

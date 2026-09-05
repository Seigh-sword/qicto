#include "syntax_mod.h"
#include "buffer.h"
#include "editor.h"
#include "module.h"
#include <tree_sitter/api.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

TSLanguage* tree_sitter_c(void);
TSLanguage* tree_sitter_cpp(void);

static struct {
    const char* keyword;
    uint8_t group;
} c_keywords[] = {
    {"if", 1}, {"else", 1}, {"for", 1}, {"while", 1}, {"do", 1},
    {"switch", 1}, {"case", 1}, {"default", 1}, {"break", 1}, {"continue", 1},
    {"return", 1}, {"goto", 1}, {"typedef", 1}, {"struct", 1}, {"union", 1},
    {"enum", 1}, {"sizeof", 1}, {"static", 1}, {"const", 1}, {"volatile", 1},
    {"register", 1}, {"auto", 1}, {"extern", 1}, {"inline", 1}, {"void", 1},
    {"char", 1}, {"short", 1}, {"int", 1}, {"long", 1}, {"float", 1},
    {"double", 1}, {"signed", 1}, {"unsigned", 1}, {"bool", 1}, {"true", 1},
    {"false", 1}, {"NULL", 4}, {"nullptr", 4},
};

static int is_c_keyword(const char* s, size_t len) {
    for (size_t i = 0; i < sizeof(c_keywords)/sizeof(c_keywords[0]); i++) {
        if (strlen(c_keywords[i].keyword) == len &&
            strncmp(c_keywords[i].keyword, s, len) == 0) {
            return c_keywords[i].group;
        }
    }
    return 0;
}

static int is_word_char(char c) {
    return (isalnum((unsigned char)c) || c == '_' || c == '.');
}

static void highlight_line(const char* line, qicto_render_line_t* rl, qicto_syntax_t syntax) {
    if (!line || !rl || !rl->cells) return;
    if (rl->count == 0) return;

    size_t i = 0;
    size_t col = 0;

    while (i < rl->count) {
        uint32_t cp = rl->cells[i].cp;

        if (cp == '/' && i + 1 < rl->count) {
            if (rl->cells[i + 1].cp == '/') {
                for (size_t j = i; j < rl->count; j++) {
                    rl->cells[j].syntax_group = 2;
                }
                return;
            } else if (rl->cells[i + 1].cp == '*') {
                for (size_t j = i; j < rl->count; j++) {
                    rl->cells[j].syntax_group = 2;
                }
                return;
            }
        }

        if (cp == '"') {
            for (size_t j = i; j < rl->count; j++) {
                rl->cells[j].syntax_group = 3;
                if (rl->cells[j].cp == '"' && j != i) {
                    i = j;
                    break;
                }
            }
            i++;
            continue;
        }

        if (cp == '\'') {
            for (size_t j = i; j < rl->count; j++) {
                rl->cells[j].syntax_group = 3;
                if (rl->cells[j].cp == '\'' && j != i) {
                    i = j;
                    break;
                }
            }
            i++;
            continue;
        }

        if ((cp >= '0' && cp <= '9')) {
            for (size_t j = i; j < rl->count; j++) {
                if (!is_word_char((char)(rl->cells[j].cp & 0xFF))) {
                    break;
                }
                rl->cells[j].syntax_group = 4;
            }
            i++;
            while (i < rl->count && is_word_char((char)(rl->cells[i].cp & 0xFF))) i++;
            continue;
        }

        if (isalpha((unsigned char)cp) || cp == '_') {
            size_t start = i;
            while (i < rl->count && is_word_char((char)(rl->cells[i].cp & 0xFF))) {
                i++;
            }
            size_t word_len = i - start;
            char word[64] = {0};
            for (size_t j = 0; j < word_len && j < 63; j++) {
                word[j] = (char)(rl->cells[start + j].cp & 0xFF);
            }
            int group = 0;
            if (syntax == QICTO_SYNTAX_C || syntax == QICTO_SYNTAX_CPP) {
                group = is_c_keyword(word, word_len);
            }
            for (size_t j = start; j < start + word_len; j++) {
                rl->cells[j].syntax_group = (uint8_t)group;
            }
            continue;
        }

        i++;
        col++;
    }
}

static qicto_cmd_result_t syntax_init(editor_t* ed) {
    (void)ed;
    return QICTO_CMD_SUCCESS;
}

static void syntax_cleanup(editor_t* ed) {
    (void)ed;
}

static void syntax_on_buffer_opened(editor_t* ed, buffer_t* buf) {
    (void)ed;
    if (!buf) return;
    buf->render_valid = false;
    buffer_update_render(buf);
}

static void syntax_on_buffer_changed(editor_t* ed, buffer_t* buf) {
    (void)ed;
    if (!buf || !buf->render_lines) return;

    for (size_t i = 0; i < buf->render_line_count; i++) {
        const char* text = buffer_get_line(buf, i);
        if (!text) continue;
        highlight_line(text, &buf->render_lines[i], buf->syntax);
    }
}

static void syntax_on_render(editor_t* ed, void* ncp) {
    (void)ed;
    (void)ncp;
}

static qicto_cmd_result_t syntax_on_command(editor_t* ed, const char* cmd, char** out) {
    (void)ed; (void)cmd; (void)out;
    return QICTO_CMD_UNKNOWN;
}

static const char syntax_mod_name[] = "syntax";
static const char syntax_mod_version[] = "0.1.0";

static qicto_mod_api_t s_syntax_api = {
    .name = syntax_mod_name,
    .version = syntax_mod_version,
    .description = "Keyword-based syntax highlighting",
    .init = syntax_init,
    .cleanup = syntax_cleanup,
    .on_render = syntax_on_render,
    .on_key = NULL,
    .on_buffer_opened = syntax_on_buffer_opened,
    .on_buffer_changed = syntax_on_buffer_changed,
    .on_mode_change = NULL,
    .on_config_reload = NULL,
    .on_command = syntax_on_command,
};

const qicto_mod_api_t* syntax_mod_get_api(void) {
    return &s_syntax_api;
}

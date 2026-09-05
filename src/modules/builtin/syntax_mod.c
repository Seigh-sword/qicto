#include "syntax_mod.h"
#include "buffer.h"
#include "editor.h"
#include "module.h"
#include <tree_sitter/api.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

TSLanguage* tree_sitter_c(void);
TSLanguage* tree_sitter_cpp(void);

typedef struct {
    TSParser* parser;
    TSQueryCursor* cursor;
    TSLanguage* lang;
} ts_state_t;

static ts_state_t s_state_c = {0};
static ts_state_t s_state_cpp = {0};

static TSLanguage* lang_for(qicto_syntax_t syntax) {
    switch (syntax) {
        case QICTO_SYNTAX_C:   return tree_sitter_c();
        case QICTO_SYNTAX_CPP: return tree_sitter_cpp();
        default: return NULL;
    }
}

static ts_state_t* state_for(qicto_syntax_t syntax) {
    switch (syntax) {
        case QICTO_SYNTAX_C:   return &s_state_c;
        case QICTO_SYNTAX_CPP: return &s_state_cpp;
        default: return NULL;
    }
}

static uint8_t map_capture(const char* name) {
    if (!name) return 0;
    if (strcmp(name, "keyword") == 0)              return 1;
    if (strcmp(name, "comment") == 0)              return 2;
    if (strcmp(name, "string") == 0)               return 3;
    if (strcmp(name, "number") == 0)               return 4;
    if (strcmp(name, "type") == 0)                 return 5;
    if (strcmp(name, "type_identifier") == 0)      return 5;
    if (strcmp(name, "function") == 0)             return 6;
    if (strcmp(name, "function.declaration") == 0) return 6;
    if (strcmp(name, "function.call") == 0)        return 6;
    if (strcmp(name, "variable") == 0)             return 7;
    if (strcmp(name, "operator") == 0)             return 8;
    if (strcmp(name, "punctuation") == 0)          return 9;
    if (strcmp(name, "preproc") == 0)              return 10;
    if (strcmp(name, "constant") == 0)             return 11;
    if (strcmp(name, "constant.builtin") == 0)     return 11;
    if (strcmp(name, "builtin") == 0)              return 12;
    return 0;
}

static void highlight_with_treesitter(buffer_t* buf, ts_state_t* st) {
    if (!st || !st->parser || !st->lang) return;
    if (!buf || buf->text.line_count == 0) return;

    size_t total = 0;
    for (size_t i = 0; i < buf->text.line_count; i++) {
        total += strlen(buf->text.lines[i]) + 1;
    }
    char* src = malloc(total + 1);
    if (!src) return;
    char* p = src;
    for (size_t i = 0; i < buf->text.line_count; i++) {
        size_t n = strlen(buf->text.lines[i]);
        memcpy(p, buf->text.lines[i], n);
        p += n;
        *p++ = '\n';
    }
    *p = '\0';

    TSTree* tree = ts_parser_parse_string(st->parser, NULL, src, (uint32_t)total);
    if (!tree) { free(src); return; }

    TSNode root = ts_tree_root_node(tree);
    TSQueryCursor* cur = st->cursor;
    if (!cur) {
        cur = ts_query_cursor_new();
        st->cursor = cur;
    }
    ts_query_cursor_reset(cur, root);

    TSTreeCursor tc = ts_tree_cursor_new(root);
    TSNode node = ts_tree_cursor_current_node(&tc);
    while (!ts_node_is_null(node)) {
        const char* type = ts_node_type(node);
        uint8_t grp = 0;
        if (strcmp(type, "primitive_type") == 0) grp = 1;
        else if (strcmp(type, "type_identifier") == 0) grp = 5;
        else if (strcmp(type, "field_identifier") == 0) grp = 7;
        else if (strcmp(type, "identifier") == 0) grp = 0;
        else if (strcmp(type, "call_expression") == 0 ||
                 strcmp(type, "call") == 0) grp = 6;
        else if (strcmp(type, "function_declarator") == 0) grp = 6;
        else if (strcmp(type, "string_literal") == 0) grp = 3;
        else if (strcmp(type, "char_literal") == 0) grp = 3;
        else if (strcmp(type, "number_literal") == 0) grp = 4;
        else if (strcmp(type, "comment") == 0) grp = 2;
        else if (strcmp(type, "preproc_include") == 0 ||
                 strcmp(type, "preproc_def") == 0 ||
                 strcmp(type, "preproc_call") == 0) grp = 10;
        else if (strcmp(type, "null") == 0 ||
                 strcmp(type, "true") == 0 ||
                 strcmp(type, "false") == 0) grp = 11;
        else if (strcmp(type, "operator") == 0) grp = 8;

        if (grp != 0) {
            uint32_t sb = ts_node_start_byte(node);
            uint32_t eb = ts_node_end_byte(node);
            TSPoint sp = ts_node_start_point(node);
            uint32_t cur_line = sp.row;
            uint32_t cur_col = sp.column;
            for (uint32_t b = sb; b < eb; b++) {
                if (cur_line >= buf->render_line_count) break;
                if (src[b] == '\n') {
                    cur_line++;
                    cur_col = 0;
                    continue;
                }
                qicto_render_line_t* rl = &buf->render_lines[cur_line];
                if (cur_col < rl->count) {
                    rl->cells[cur_col].syntax_group = grp;
                }
                cur_col++;
            }
        }

        if (ts_tree_cursor_goto_first_child(&tc)) {
            node = ts_tree_cursor_current_node(&tc);
        } else {
            while (true) {
                if (ts_tree_cursor_goto_next_sibling(&tc)) {
                    node = ts_tree_cursor_current_node(&tc);
                    break;
                }
                if (!ts_tree_cursor_goto_parent(&tc)) {
                    node = ts_node_null();
                    break;
                }
            }
        }
    }
    ts_tree_cursor_delete(&tc);
    ts_tree_delete(tree);
    free(src);
}

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
    {"double", 1}, {"signed", 1}, {"unsigned", 1}, {"bool", 1}, {"true", 11},
    {"false", 11}, {"NULL", 11}, {"nullptr", 11},
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

static int is_word_char_local(uint32_t cp) {
    if (cp == '_') return 1;
    return (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') ||
           (cp >= '0' && cp <= '9');
}

static void highlight_line_fallback(buffer_t* buf, size_t i) {
    if (!buf || i >= buf->render_line_count) return;
    qicto_render_line_t* rl = &buf->render_lines[i];
    const char* text = buf->text.lines[i];
    if (!text || !rl->cells) return;

    size_t pos = 0;
    while (pos < rl->count) {
        uint32_t cp = rl->cells[pos].cp;

        if (cp == '/' && pos + 1 < rl->count && rl->cells[pos + 1].cp == '/') {
            for (size_t j = pos; j < rl->count; j++)
                rl->cells[j].syntax_group = 2;
            return;
        }
        if (cp == '/' && pos + 1 < rl->count && rl->cells[pos + 1].cp == '*') {
            for (size_t j = pos; j < rl->count; j++)
                rl->cells[j].syntax_group = 2;
            return;
        }
        if (cp == '"') {
            rl->cells[pos].syntax_group = 3;
            pos++;
            while (pos < rl->count && rl->cells[pos].cp != '"') {
                rl->cells[pos].syntax_group = 3;
                pos++;
            }
            if (pos < rl->count) rl->cells[pos].syntax_group = 3;
            pos++;
            continue;
        }
        if (cp == '\'') {
            rl->cells[pos].syntax_group = 3;
            pos++;
            while (pos < rl->count && rl->cells[pos].cp != '\'') {
                rl->cells[pos].syntax_group = 3;
                pos++;
            }
            if (pos < rl->count) rl->cells[pos].syntax_group = 3;
            pos++;
            continue;
        }
        if (cp >= '0' && cp <= '9') {
            size_t start = pos;
            while (pos < rl->count && is_word_char_local(rl->cells[pos].cp)) pos++;
            for (size_t j = start; j < pos; j++)
                rl->cells[j].syntax_group = 4;
            continue;
        }
        if (cp == '_' || (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z')) {
            size_t start = pos;
            while (pos < rl->count && is_word_char_local(rl->cells[pos].cp)) pos++;
            size_t wlen = pos - start;
            char word[64] = {0};
            for (size_t j = 0; j < wlen && j < 63; j++)
                word[j] = (char)(rl->cells[start + j].cp & 0x7F);
            int g = 0;
            if (buf->syntax == QICTO_SYNTAX_C || buf->syntax == QICTO_SYNTAX_CPP)
                g = is_c_keyword(word, wlen);
            for (size_t j = start; j < pos; j++)
                rl->cells[j].syntax_group = (uint8_t)g;
            continue;
        }
        pos++;
    }
}

static qicto_cmd_result_t syntax_init(editor_t* ed) {
    (void)ed;
    if (!s_state_c.parser) {
        s_state_c.parser = ts_parser_new();
        if (s_state_c.parser) ts_parser_set_language(s_state_c.parser, tree_sitter_c());
        s_state_c.lang = tree_sitter_c();
    }
    if (!s_state_cpp.parser) {
        s_state_cpp.parser = ts_parser_new();
        if (s_state_cpp.parser) ts_parser_set_language(s_state_cpp.parser, tree_sitter_cpp());
        s_state_cpp.lang = tree_sitter_cpp();
    }
    return QICTO_CMD_SUCCESS;
}

static void syntax_cleanup(editor_t* ed) {
    (void)ed;
    if (s_state_c.cursor)   { ts_query_cursor_delete(s_state_c.cursor);   s_state_c.cursor = NULL; }
    if (s_state_cpp.cursor) { ts_query_cursor_delete(s_state_cpp.cursor); s_state_cpp.cursor = NULL; }
    if (s_state_c.parser)   { ts_parser_delete(s_state_c.parser);   s_state_c.parser = NULL; }
    if (s_state_cpp.parser) { ts_parser_delete(s_state_cpp.parser); s_state_cpp.parser = NULL; }
}

static void syntax_on_buffer_opened(editor_t* ed, buffer_t* buf) {
    (void)ed;
    if (!buf) return;
    buf->render_valid = false;
    buffer_update_render(buf);
}

static void syntax_on_buffer_changed(editor_t* ed, buffer_t* buf) {
    (void)ed;
    if (!buf) return;

    if (buf->render_lines) {
        for (size_t i = 0; i < buf->render_line_count; i++) {
            qicto_render_line_t* rl = &buf->render_lines[i];
            if (rl->cells) {
                for (size_t j = 0; j < rl->count; j++)
                    rl->cells[j].syntax_group = 0;
            }
        }
    }

    ts_state_t* st = state_for(buf->syntax);
    if (st && st->parser) {
        highlight_with_treesitter(buf, st);
        return;
    }
    for (size_t i = 0; i < buf->render_line_count; i++) {
        highlight_line_fallback(buf, i);
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
    .description = "Tree-sitter syntax highlighting for C/C++, keyword fallback otherwise",
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

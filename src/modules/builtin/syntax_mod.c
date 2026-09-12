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
#include <stdbool.h>

TSLanguage* tree_sitter_c(void);
TSLanguage* tree_sitter_cpp(void);
TSLanguage* tree_sitter_python(void);
TSLanguage* tree_sitter_bash(void);

typedef struct {
    TSParser* parser;
    TSQuery* query;
    TSQueryCursor* cursor;
    TSLanguage* lang;
} ts_state_t;

static ts_state_t s_state_c      = {0};
static ts_state_t s_state_cpp    = {0};
static ts_state_t s_state_python = {0};
static ts_state_t s_state_bash   = {0};

static const char* c_query =
    "(string_literal) @string\n"
    "(system_lib_string) @string\n"
    "(char_literal) @string\n"
    "(number_literal) @number\n"
    "(comment) @comment\n"
    "(preproc_include) @preproc\n"
    "(preproc_def) @preproc\n"
    "(preproc_call) @preproc\n"
    "(preproc_ifdef) @preproc\n"
    "(primitive_type) @type\n"
    "(type_identifier) @type\n"
    "(storage_class_specifier) @keyword\n"
    "(null) @constant\n"
    "(true) @constant\n"
    "(false) @constant\n"
    "(call_expression function: (identifier) @function)\n"
    "(call_expression function: (field_expression field: (field_identifier) @function))\n"
    "(function_declarator declarator: (identifier) @function)\n"
    "(pointer_declarator declarator: (identifier) @function)\n";

static const char* cpp_query =
    "(string_literal) @string\n"
    "(raw_string_literal) @string\n"
    "(number_literal) @number\n"
    "(comment) @comment\n"
    "(preproc_include) @preproc\n"
    "(preproc_def) @preproc\n"
    "(primitive_type) @type\n"
    "(type_identifier) @type\n"
    "(nullptr) @constant\n"
    "(true) @constant\n"
    "(false) @constant\n"
    "(call_expression function: (identifier) @function)\n"
    "(call_expression function: (field_expression field: (field_identifier) @function))\n"
    "(function_declarator declarator: (identifier) @function)\n";

static const char* python_query =
    "(string) @string\n"
    "(integer) @number\n"
    "(float) @number\n"
    "(comment) @comment\n"
    "(true) @constant\n"
    "(false) @constant\n"
    "(none) @constant\n"
    "(function_definition name: (identifier) @function)\n"
    "(class_definition name: (identifier) @type)\n"
    "(call function: (identifier) @function)\n"
    "(call function: (attribute attribute: (identifier) @function))\n"
    "(import_statement) @preproc\n"
    "(import_from_statement) @preproc\n";

static const char* bash_query =
    "(string) @string\n"
    "(number) @number\n"
    "(comment) @comment\n"
    "(variable_name) @variable\n"
    "(function_definition name: (word) @function)\n"
    "(command name: (word) @function)\n";

static TSLanguage* lang_for(qicto_syntax_t syntax) {
    switch (syntax) {
        case QICTO_SYNTAX_C:      return tree_sitter_c();
        case QICTO_SYNTAX_CPP:    return tree_sitter_cpp();
        case QICTO_SYNTAX_PYTHON: return tree_sitter_python();
        case QICTO_SYNTAX_SHELL:  return tree_sitter_bash();
        default: return NULL;
    }
}

static ts_state_t* state_for(qicto_syntax_t syntax) {
    switch (syntax) {
        case QICTO_SYNTAX_C:      return &s_state_c;
        case QICTO_SYNTAX_CPP:    return &s_state_cpp;
        case QICTO_SYNTAX_PYTHON: return &s_state_python;
        case QICTO_SYNTAX_SHELL:  return &s_state_bash;
        default: return NULL;
    }
}

static const char* query_for(qicto_syntax_t syntax) {
    switch (syntax) {
        case QICTO_SYNTAX_C:      return c_query;
        case QICTO_SYNTAX_CPP:    return cpp_query;
        case QICTO_SYNTAX_PYTHON: return python_query;
        case QICTO_SYNTAX_SHELL:  return bash_query;
        default: return NULL;
    }
}

static uint8_t map_capture(const char* name) {
    if (!name) return 0;
    if (strcmp(name, "keyword")  == 0) return QICTO_HL_KEYWORD;
    if (strcmp(name, "comment")  == 0) return QICTO_HL_COMMENT;
    if (strcmp(name, "string")   == 0) return QICTO_HL_STRING;
    if (strcmp(name, "number")   == 0) return QICTO_HL_NUMBER;
    if (strcmp(name, "type")     == 0) return QICTO_HL_TYPE;
    if (strcmp(name, "function") == 0) return QICTO_HL_FUNCTION;
    if (strcmp(name, "variable") == 0) return QICTO_HL_VARIABLE;
    if (strcmp(name, "operator") == 0) return QICTO_HL_OPERATOR;
    if (strcmp(name, "punctuation") == 0) return QICTO_HL_PUNCTUATION;
    if (strcmp(name, "preproc")  == 0) return QICTO_HL_PREPROC;
    if (strcmp(name, "constant") == 0) return QICTO_HL_CONSTANT;
    if (strcmp(name, "builtin")  == 0) return QICTO_HL_BUILTIN;
    return QICTO_HL_NONE;
}

static bool compile_query(ts_state_t* st, const char* src) {
    if (!st || !src) return false;
    uint32_t error_offset = 0;
    TSQueryError error_type = 0;
    TSQuery* q = ts_query_new(st->lang, src, (uint32_t)strlen(src),
                              &error_offset, &error_type);
    if (!q) {
        return false;
    }
    if (st->query) ts_query_delete(st->query);
    st->query = q;
    return true;
}

static void highlight_with_query(buffer_t* buf, ts_state_t* st) {
    if (!st || !st->parser || !st->lang || !st->query || !st->cursor) return;
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

    ts_query_cursor_reset(st->cursor, ts_tree_root_node(tree));

    TSQueryMatch match;
    while (ts_query_cursor_next_match(st->cursor, &match)) {
        for (uint16_t i = 0; i < match.capture_count; i++) {
            TSNode node = match.captures[i].node;
            uint32_t cname_len = 0;
            const char* cname = ts_query_capture_name_for_id(st->query,
                                                              match.captures[i].index,
                                                              &cname_len);
            uint8_t grp = map_capture(cname);
            if (grp == QICTO_HL_NONE) continue;

            uint32_t sb = ts_node_start_byte(node);
            uint32_t eb = ts_node_end_byte(node);
            TSPoint sp = ts_node_start_point(node);
            uint32_t cur_line = sp.row;
            uint32_t cur_col = sp.column;
            for (uint32_t b = sb; b < eb && b < total; b++) {
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
    }

    ts_tree_delete(tree);
    free(src);
}

typedef struct {
    const char* keyword;
    uint8_t group;
} kw_entry_t;

static const kw_entry_t c_keywords[] = {
    {"if", 1}, {"else", 1}, {"for", 1}, {"while", 1}, {"do", 1},
    {"switch", 1}, {"case", 1}, {"default", 1}, {"break", 1}, {"continue", 1},
    {"return", 1}, {"goto", 1}, {"typedef", 1}, {"struct", 1}, {"union", 1},
    {"enum", 1}, {"sizeof", 1}, {"static", 1}, {"const", 1}, {"volatile", 1},
    {"register", 1}, {"auto", 1}, {"extern", 1}, {"inline", 1}, {"void", 1},
    {"char", 1}, {"short", 1}, {"int", 1}, {"long", 1}, {"float", 1},
    {"double", 1}, {"signed", 1}, {"unsigned", 1}, {"bool", 1}, {"true", 11},
    {"false", 11}, {"NULL", 11}, {"nullptr", 11},
};

static const kw_entry_t py_keywords[] = {
    {"and", 1}, {"as", 1}, {"assert", 1}, {"async", 1}, {"await", 1},
    {"break", 1}, {"class", 1}, {"continue", 1}, {"def", 1}, {"del", 1},
    {"elif", 1}, {"else", 1}, {"except", 1}, {"finally", 1}, {"for", 1},
    {"from", 1}, {"global", 1}, {"if", 1}, {"import", 1}, {"in", 1},
    {"is", 1}, {"lambda", 1}, {"nonlocal", 1}, {"not", 1}, {"or", 1},
    {"pass", 1}, {"raise", 1}, {"return", 1}, {"try", 1}, {"while", 1},
    {"with", 1}, {"yield", 1}, {"True", 11}, {"False", 11}, {"None", 11},
};

static const kw_entry_t sh_keywords[] = {
    {"if", 1}, {"then", 1}, {"else", 1}, {"elif", 1}, {"fi", 1},
    {"for", 1}, {"in", 1}, {"do", 1}, {"done", 1}, {"while", 1},
    {"case", 1}, {"esac", 1}, {"function", 1}, {"return", 1}, {"local", 1},
    {"export", 1}, {"source", 1}, {"echo", 1}, {"cd", 1}, {"set", 1},
    {"unset", 1}, {"true", 11}, {"false", 11},
};

static int lookup_keyword(const char* s, size_t len, const kw_entry_t* tbl, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (strlen(tbl[i].keyword) == len && strncmp(tbl[i].keyword, s, len) == 0) {
            return tbl[i].group;
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

    const kw_entry_t* kw_tbl = NULL;
    size_t kw_n = 0;
    switch (buf->syntax) {
        case QICTO_SYNTAX_C:
        case QICTO_SYNTAX_CPP:
            kw_tbl = c_keywords; kw_n = sizeof(c_keywords)/sizeof(c_keywords[0]); break;
        case QICTO_SYNTAX_PYTHON:
            kw_tbl = py_keywords; kw_n = sizeof(py_keywords)/sizeof(py_keywords[0]); break;
        case QICTO_SYNTAX_SHELL:
            kw_tbl = sh_keywords; kw_n = sizeof(sh_keywords)/sizeof(sh_keywords[0]); break;
        default: break;
    }

    size_t pos = 0;
    while (pos < rl->count) {
        uint32_t cp = rl->cells[pos].cp;

        if (cp == '#' && (buf->syntax == QICTO_SYNTAX_PYTHON ||
                          buf->syntax == QICTO_SYNTAX_SHELL)) {
            for (size_t j = pos; j < rl->count; j++)
                rl->cells[j].syntax_group = QICTO_HL_COMMENT;
            return;
        }
        if (cp == '/' && pos + 1 < rl->count && rl->cells[pos + 1].cp == '/') {
            for (size_t j = pos; j < rl->count; j++)
                rl->cells[j].syntax_group = QICTO_HL_COMMENT;
            return;
        }
        if (cp == '/' && pos + 1 < rl->count && rl->cells[pos + 1].cp == '*') {
            for (size_t j = pos; j < rl->count; j++)
                rl->cells[j].syntax_group = QICTO_HL_COMMENT;
            return;
        }
        if (cp == '"' || cp == '\'') {
            uint32_t open = cp;
            rl->cells[pos].syntax_group = QICTO_HL_STRING;
            pos++;
            while (pos < rl->count && rl->cells[pos].cp != open) {
                rl->cells[pos].syntax_group = QICTO_HL_STRING;
                pos++;
            }
            if (pos < rl->count) rl->cells[pos].syntax_group = QICTO_HL_STRING;
            pos++;
            continue;
        }
        if (cp >= '0' && cp <= '9') {
            size_t start = pos;
            while (pos < rl->count && is_word_char_local(rl->cells[pos].cp)) pos++;
            for (size_t j = start; j < pos; j++)
                rl->cells[j].syntax_group = QICTO_HL_NUMBER;
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
            if (kw_tbl) g = lookup_keyword(word, wlen, kw_tbl, kw_n);
            for (size_t j = start; j < pos; j++)
                rl->cells[j].syntax_group = (uint8_t)g;
            continue;
        }
        pos++;
    }
}

static qicto_cmd_result_t syntax_init(editor_t* ed) {
    (void)ed;

    static const struct {
        ts_state_t* st;
        TSLanguage* (*lang_fn)(void);
    } allocs[] = {
        { &s_state_c,      tree_sitter_c      },
        { &s_state_cpp,    tree_sitter_cpp    },
        { &s_state_python, tree_sitter_python },
        { &s_state_bash,   tree_sitter_bash   },
    };

    for (size_t i = 0; i < sizeof(allocs)/sizeof(allocs[0]); i++) {
        ts_state_t* st = allocs[i].st;
        if (!st->parser) {
            st->parser = ts_parser_new();
            if (st->parser) ts_parser_set_language(st->parser, allocs[i].lang_fn());
            st->lang = allocs[i].lang_fn();
        }
        if (st->parser && st->lang) {
            qicto_syntax_t s;
            if (st == &s_state_c)            s = QICTO_SYNTAX_C;
            else if (st == &s_state_cpp)     s = QICTO_SYNTAX_CPP;
            else if (st == &s_state_python)  s = QICTO_SYNTAX_PYTHON;
            else                             s = QICTO_SYNTAX_SHELL;
            if (!compile_query(st, query_for(s))) {
                st->query = NULL;
            }
            st->cursor = ts_query_cursor_new();
        }
    }
    return QICTO_CMD_SUCCESS;
}

static void syntax_cleanup(editor_t* ed) {
    (void)ed;
    static ts_state_t* all[] = { &s_state_c, &s_state_cpp, &s_state_python, &s_state_bash };
    for (size_t i = 0; i < sizeof(all)/sizeof(all[0]); i++) {
        if (all[i]->cursor) { ts_query_cursor_delete(all[i]->cursor); all[i]->cursor = NULL; }
        if (all[i]->query)  { ts_query_delete(all[i]->query);         all[i]->query  = NULL; }
        if (all[i]->parser) { ts_parser_delete(all[i]->parser);       all[i]->parser = NULL; }
    }
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
    if (st && st->parser && st->query && st->cursor) {
        highlight_with_query(buf, st);
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
static const char syntax_mod_version[] = "0.2.0";

static qicto_mod_api_t s_syntax_api = {
    .name = syntax_mod_name,
    .version = syntax_mod_version,
    .description = "Tree-sitter syntax highlighting for C/C++/Python/Bash, keyword fallback otherwise",
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

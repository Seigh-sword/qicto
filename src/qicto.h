#ifndef QICTO_H
#define QICTO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define QICTO_VERSION_MAJOR 0
#define QICTO_VERSION_MINOR 1
#define QICTO_VERSION_PATCH 0
#define QICTO_VERSION_STRING "0.1.0"

#define QICTO_MAX_PATH_LEN 4096
#define QICTO_MAX_LINE_LEN 100000
#define QICTO_MAX_NAME_LEN 256
#define QICTO_MAX_MOD_NAME 64
#define QICTO_MAX_CMD_LEN 1024

typedef enum {
    QICTO_MODE_NORMAL = 0,
    QICTO_MODE_INSERT,
    QICTO_MODE_VISUAL,
    QICTO_MODE_COMMAND,
    QICTO_MODE_SEARCH,
    QICTO_MODE_SELECT,
    QICTO_MODE_COUNT
} qicto_mode_t;

typedef enum {
    QICTO_CMD_SUCCESS = 0,
    QICTO_CMD_UNKNOWN = 1,
    QICTO_CMD_ERROR = 2,
    QICTO_CMD_CANCEL = 3,
} qicto_cmd_result_t;

typedef uint32_t qkey_t;

enum {
    QICTO_KEY_NONE = 0,
    QICTO_KEY_BACKSPACE = 0x7f,
    QICTO_KEY_ENTER = 0x0a,
    QICTO_KEY_TAB = 0x09,
    QICTO_KEY_ESC = 0x1b,
    QICTO_KEY_UP = 0x100,
    QICTO_KEY_DOWN,
    QICTO_KEY_LEFT,
    QICTO_KEY_RIGHT,
    QICTO_KEY_HOME,
    QICTO_KEY_END,
    QICTO_KEY_PAGE_UP,
    QICTO_KEY_PAGE_DOWN,
    QICTO_KEY_INSERT,
    QICTO_KEY_DELETE,
    QICTO_KEY_F1, QICTO_KEY_F2, QICTO_KEY_F3, QICTO_KEY_F4,
    QICTO_KEY_F5, QICTO_KEY_F6, QICTO_KEY_F7, QICTO_KEY_F8,
    QICTO_KEY_F9, QICTO_KEY_F10, QICTO_KEY_F11, QICTO_KEY_F12,
};

typedef enum {
    QICTO_BUFTYPE_UNKNOWN = 0,
    QICTO_BUFTYPE_TEXT,
    QICTO_BUFTYPE_TERMINAL,
    QICTO_BUFTYPE_QUICKFIX,
    QICTO_BUFTYPE_HELP,
} qicto_buftype_t;

typedef enum {
    QICTO_SYNTAX_UNKNOWN = 0,
    QICTO_SYNTAX_C,
    QICTO_SYNTAX_CPP,
    QICTO_SYNTAX_PYTHON,
    QICTO_SYNTAX_JS,
    QICTO_SYNTAX_GO,
    QICTO_SYNTAX_RUST,
    QICTO_SYNTAX_HTML,
    QICTO_SYNTAX_CSS,
    QICTO_SYNTAX_JSON,
    QICTO_SYNTAX_YAML,
    QICTO_SYNTAX_TOML,
    QICTO_SYNTAX_MARKDOWN,
    QICTO_SYNTAX_SHELL,
    QICTO_SYNTAX_DOCKERFILE,
    QICTO_SYNTAX_COUNT
} qicto_syntax_t;

typedef struct {
    uint32_t cp;
    uint8_t syntax_group;
    uint8_t style_mask;
} qicto_cell_t;

typedef struct {
    qicto_cell_t* cells;
    size_t count;
    size_t capacity;
    bool valid;
} qicto_render_line_t;

typedef struct {
    size_t cursor_line;
    size_t cursor_col;
    size_t cursor_byte;
    size_t sel_anchor_line;
    size_t sel_anchor_col;
    size_t sel_cursor_line;
    size_t sel_cursor_col;
    bool has_selection;
} qicto_cursor_t;

typedef struct {
    char** lines;
    size_t line_count;
    size_t capacity;
} qicto_text_lines_t;

typedef struct buffer_t {
    char filename[QICTO_MAX_PATH_LEN];
    char display_name[QICTO_MAX_NAME_LEN];
    qicto_buftype_t btype;
    bool dirty;
    bool readonly;
    time_t last_mtime;
    time_t last_save;

    qicto_text_lines_t text;

    qicto_cursor_t cursor;

    qicto_syntax_t syntax;
    qicto_render_line_t* render_lines;
    size_t render_line_count;
    bool render_valid;
    int id;
    struct buffer_t* next;
    struct buffer_t* prev;
} buffer_t;

typedef struct {
    char key[32];
    char value[256];
} qicto_setting_t;

typedef struct {
    qicto_setting_t* settings;
    size_t count;
    size_t capacity;

    int tab_width;
    bool expand_tabs;
    bool show_line_numbers;
    bool show_whitespace;
    bool show_minimap;
    bool auto_indent;
    bool auto_complete;
    bool word_wrap;
    int scroll_offset;
    char font_family[QICTO_MAX_NAME_LEN];
    int font_size;
    qicto_syntax_t default_syntax;
    char theme[QICTO_MAX_NAME_LEN];
    char mods_dir[QICTO_MAX_PATH_LEN];
    char keymap[QICTO_MAX_NAME_LEN];
    char project_dir[QICTO_MAX_PATH_LEN];
} config_t;

typedef struct qicto_window_t {
    int x, y, width, height;
    buffer_t* buffer;
    int id;
    struct qicto_window_t* next;
    struct qicto_window_t* prev;
} qicto_window_t;

typedef struct {
    qicto_window_t* active;
    qicto_window_t* windows;
    size_t window_count;
    int split_mode;
} qicto_layout_t;

typedef struct command_registry_t command_registry_t;
typedef struct mod_registry_t mod_registry_t;

typedef struct qicto_editor_t {
    qicto_mode_t mode;
    qicto_cursor_t cursor;
    buffer_t* current_buffer;
    buffer_t* buffers;
    size_t buffer_count;
    config_t* config;
    qicto_layout_t layout;
    command_registry_t* commands;
    mod_registry_t* mods;
    char statusmsg[512];
    time_t statusmsg_time;
    char cmd_buffer[QICTO_MAX_CMD_LEN];
    size_t cmd_cursor;
    bool quit_requested;
    bool force_quit;
} editor_t;

typedef struct {
    void* dl_handle;
    char name[QICTO_MAX_MOD_NAME];
    char version[QICTO_MAX_NAME_LEN];
    int refcount;
    bool active;
    void (*on_init)(editor_t* ed);
    void (*on_cleanup)(editor_t* ed);
    void (*on_render)(editor_t* ed, void* ncp);
    qkey_t (*on_key)(editor_t* ed, qkey_t key);
    void (*on_buffer_opened)(editor_t* ed, buffer_t* buf);
    void (*on_buffer_changed)(editor_t* ed, buffer_t* buf);
    void (*on_mode_change)(editor_t* ed, qicto_mode_t from, qicto_mode_t to);
    void (*on_config_reload)(editor_t* ed, config_t* cfg);
    void (*on_command)(editor_t* ed, const char* cmd, char** out);
    const char* (*get_name)(void);
    const char* (*get_version)(void);
    void* reserved[8];
} qicto_mod_t;

typedef struct {
    void* ctx;
    const char* name;
    const char* version;
    const char* description;
    qicto_cmd_result_t (*init)(editor_t* ed);
    void (*cleanup)(editor_t* ed);
    void (*on_render)(editor_t* ed, void* ncp);
    qkey_t (*on_key)(editor_t* ed, qkey_t key);
    void (*on_buffer_opened)(editor_t* ed, buffer_t* buf);
    void (*on_buffer_changed)(editor_t* ed, buffer_t* buf);
    void (*on_mode_change)(editor_t* ed, qicto_mode_t from, qicto_mode_t to);
    void (*on_config_reload)(editor_t* ed, config_t* cfg);
    qicto_cmd_result_t (*on_command)(editor_t* ed, const char* cmd, char** out);
} qicto_mod_api_t;

#define QICTO_MOD_API_VERSION 1

#endif

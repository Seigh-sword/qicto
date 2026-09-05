#ifndef QICTO_TUI_H
#define QICTO_TUI_H

#include "qicto.h"
#include <notcurses/notcurses.h>

typedef struct {
    struct notcurses* nc;
    struct ncplane* stdplane;
    int width;
    int height;
    bool initialized;
    qicto_mode_t pending_mode;
} tui_state_t;

typedef struct {
    struct ncplane* plane;
    int x, y, width, height;
} tui_window_t;

tui_state_t* tui_init(void);
void tui_deinit(tui_state_t* tui);
int tui_render(tui_state_t* tui, editor_t* ed);
qkey_t tui_read_key(tui_state_t* tui);
void tui_handle_key(tui_state_t* tui, editor_t* ed, qkey_t key);
void tui_enter_command_mode(editor_t* ed);
void tui_refresh_size(tui_state_t* tui);
void tui_flush(tui_state_t* tui);

#endif

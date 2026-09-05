#ifndef QICTO_EDITOR_H
#define QICTO_EDITOR_H

#include "qicto.h"

editor_t* editor_create(void);
void editor_destroy(editor_t* ed);
void editor_set_mode(editor_t* ed, qicto_mode_t mode);
buffer_t* editor_open_file(editor_t* ed, const char* filename);
buffer_t* editor_current_buffer(editor_t* ed);
void editor_cycle_buffer(editor_t* ed, int direction);
void editor_close_buffer(editor_t* ed, buffer_t* buf);
void editor_run(editor_t* ed);
void editor_quit(editor_t* ed);
void editor_redraw(editor_t* ed);
void editor_set_status(editor_t* ed, const char* fmt, ...);
void editor_clear_status(editor_t* ed);
bool editor_status_active(editor_t* ed);
config_t* editor_config(editor_t* ed);
void editor_scroll_to_cursor(editor_t* ed, int viewport_height, int viewport_width);

#endif

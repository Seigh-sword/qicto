#ifndef QICTO_BUFFER_H
#define QICTO_BUFFER_H

#include "qicto.h"

buffer_t* buffer_new(const char* filename);
void buffer_free(buffer_t* buf);
int buffer_load_file(buffer_t* buf, const char* filename);
int buffer_save(buffer_t* buf, const char* filename);
void buffer_insert_char(buffer_t* buf, size_t line, size_t col, char c);
void buffer_insert_text(buffer_t* buf, size_t line, size_t col, const char* text);
void buffer_remove_char(buffer_t* buf, size_t line, size_t col);
void buffer_remove_range(buffer_t* buf, size_t line, size_t col, size_t len);
void buffer_split_line(buffer_t* buf, size_t line, size_t col);
void buffer_join_lines(buffer_t* buf, size_t line);
size_t buffer_line_length(buffer_t* buf, size_t line);
const char* buffer_get_line(buffer_t* buf, size_t line);
void buffer_update_render(buffer_t* buf);
void buffer_set_cursor(buffer_t* buf, size_t line, size_t col);
void buffer_validate_cursor(buffer_t* buf);
const char* buffer_display_name(buffer_t* buf);
int buffer_undo(buffer_t* buf);
void buffer_undo_clear(buffer_t* buf);

#endif

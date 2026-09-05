#ifndef QICTO_RENDERER_H
#define QICTO_RENDERER_H

#include "qicto.h"
#include <notcurses/notcurses.h>

void renderer_init(struct ncplane* plane);
void renderer_render_buffer(struct ncplane* plane, buffer_t* buf, int x, int y, int width, int height, bool show_line_numbers);
void renderer_render_statusbar(struct ncplane* plane, editor_t* ed, int x, int y, int width);
void renderer_render_cmdline(struct ncplane* plane, editor_t* ed, int x, int y, int width);
void renderer_render_mode(struct ncplane* plane, editor_t* ed, int x, int y);
void renderer_clear_area(struct ncplane* plane, int x, int y, int width, int height);
void renderer_set_style(struct ncplane* plane, int fg_r, int fg_g, int fg_b, int bg_r, int bg_g, int bg_b);

typedef struct {
    uint32_t fg_color;
    uint32_t bg_color;
    uint8_t style;
    uint8_t syntax_group;
} render_attr_t;

#endif

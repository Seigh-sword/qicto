#include "renderer.h"
#include "buffer.h"
#include "editor.h"
#include "platform.h"
#include <notcurses/notcurses.h>
#include <stdio.h>
#include <string.h>

static int s_renderer_initialized = 0;

void renderer_init(struct ncplane* plane) {
    (void)plane;
    s_renderer_initialized = 1;
}

void renderer_clear_area(struct ncplane* plane, int x, int y, int width, int height) {
    if (!plane || width <= 0 || height <= 0) return;
    for (int i = 0; i < height; i++) {
        ncplane_cursor_move_yx(plane, y + i, x);
        ncplane_erase(plane);
    }
}

void renderer_set_style(struct ncplane* plane, int fg_r, int fg_g, int fg_b,
                        int bg_r, int bg_g, int bg_b) {
    if (!plane) return;
    ncplane_set_fg_rgb8(plane, (unsigned)fg_r, (unsigned)fg_g, (unsigned)fg_b);
    ncplane_set_bg_rgb8(plane, (unsigned)bg_r, (unsigned)bg_g, (unsigned)bg_b);
}

void renderer_render_buffer(struct ncplane* plane, buffer_t* buf,
                            int x, int y, int width, int height, bool show_line_numbers) {
    if (!plane || !buf) return;

    int x_offset = x;
    if (show_line_numbers) {
        int max_digits = 1;
        int n = (int)buf->text.line_count;
        while (n >= 10) { max_digits++; n /= 10; }
        x_offset += max_digits + 2;
    }

    ncplane_set_fg_rgb8(plane, 0xdf, 0xdf, 0xdf);

    for (int i = 0; i < height; i++) {
        size_t line_idx = (size_t)(i);
        if (line_idx >= buf->text.line_count) {
            ncplane_cursor_move_yx(plane, y + i, x_offset);
            continue;
        }
        const char* text = buffer_get_line(buf, line_idx);
        if (!text) continue;

        ncplane_cursor_move_yx(plane, y + i, x_offset);
        ncplane_putstr(plane, text);
    }
}

void renderer_render_statusbar(struct ncplane* plane, editor_t* ed, int x, int y, int width) {
    if (!plane || !ed || width <= 0) return;

    buffer_t* buf = ed->current_buffer;

    ncplane_set_bg_rgb8(plane, 0x2d, 0x2a, 0x27);
    ncplane_set_fg_rgb8(plane, 0xe6, 0xe6, 0xe6);

    ncplane_cursor_move_yx(plane, y, x);
    for (int i = 0; i < width; i++) {
        ncplane_putchar(plane, ' ');
    }

    ncplane_cursor_move_yx(plane, y, x);

    const char* mode_str = "NORMAL";
    uint8_t mode_r = 0xe6, mode_g = 0xe6, mode_b = 0xe6;
    if (ed->mode == QICTO_MODE_INSERT) {
        mode_str = "INSERT";
        mode_r = 0x50; mode_g = 0xfa; mode_b = 0x77;
    } else if (ed->mode == QICTO_MODE_VISUAL || ed->mode == QICTO_MODE_SELECT) {
        mode_str = "VISUAL";
        mode_r = 0xff; mode_g = 0xbd; mode_b = 0x65;
    } else if (ed->mode == QICTO_MODE_COMMAND) {
        mode_str = "COMMAND";
        mode_r = 0x8b; mode_g = 0xae; mode_b = 0xf2;
    } else if (ed->mode == QICTO_MODE_SEARCH) {
        mode_str = "SEARCH";
        mode_r = 0xff; mode_g = 0x79; mode_b = 0xc6;
    }

    ncplane_set_fg_rgb8(plane, mode_r, mode_g, mode_b);
    ncplane_printf(plane, " %s ", mode_str);

    char bufinfo[256] = {0};
    if (buf) {
        const char* name = buffer_display_name(buf);
        ncplane_set_fg_rgb8(plane, 0xe6, 0xe6, 0xe6);
        ncplane_printf(plane, " %s ", name);

        if (buf->dirty) {
            ncplane_set_fg_rgb8(plane, 0xf3, 0x8b, 0xa8);
            ncplane_putchar(plane, '*');
            ncplane_set_fg_rgb8(plane, 0xe6, 0xe6, 0xe6);
        }

        char dims[64];
        snprintf(dims, sizeof(dims), " [%lux%lu]", (unsigned long)buf->text.line_count,
                 (unsigned long)(buf->text.line_count > 0 ? strlen(buf->text.lines[0]) : 0));
        ncplane_putstr(plane, dims);
    }

    if (ed->statusmsg[0] && editor_status_active(ed)) {
        ncplane_cursor_move_yx(plane, y, x + width - 30);
        ncplane_set_fg_rgb8(plane, 0x68, 0x70, 0x76);
        char msgbuf[64];
        snprintf(msgbuf, sizeof(msgbuf), "%.30s", ed->statusmsg);
        ncplane_putstr(plane, msgbuf);
    }

    ncplane_set_fg_rgb8(plane, 0xdf, 0xdf, 0xdf);
    ncplane_set_bg_rgb8(plane, 0x1e, 0x1e, 0x2e);
}

void renderer_render_cmdline(struct ncplane* plane, editor_t* ed, int x, int y, int width) {
    if (!plane || !ed || width <= 0) return;

    ncplane_set_bg_rgb8(plane, 0x1e, 0x1e, 0x2e);
    ncplane_set_fg_rgb8(plane, 0xff, 0xff, 0xff);

    ncplane_cursor_move_yx(plane, y, x);
    for (int i = 0; i < width; i++) ncplane_putchar(plane, ' ');

    ncplane_cursor_move_yx(plane, y, x);

    char prefix[8] = {0};
    if (ed->mode == QICTO_MODE_COMMAND) {
        prefix[0] = ':';
    } else if (ed->mode == QICTO_MODE_SEARCH) {
        prefix[0] = '/';
    }

    if (prefix[0]) {
        ncplane_set_fg_rgb8(plane, 0xff, 0xbd, 0x65);
        ncplane_putchar(plane, prefix[0]);
        ncplane_set_fg_rgb8(plane, 0xff, 0xff, 0xff);
    }

    ncplane_putstr(plane, ed->cmd_buffer);
}

void renderer_render_mode(struct ncplane* plane, editor_t* ed, int x, int y) {
    if (!plane || !ed) return;
    const char* mode_str = "NORMAL";
    if (ed->mode == QICTO_MODE_INSERT) mode_str = "INSERT";
    else if (ed->mode == QICTO_MODE_VISUAL || ed->mode == QICTO_MODE_SELECT) mode_str = "VISUAL";
    else if (ed->mode == QICTO_MODE_COMMAND) mode_str = "COMMAND";
    else if (ed->mode == QICTO_MODE_SEARCH) mode_str = "SEARCH";
    ncplane_cursor_move_yx(plane, y, x);
    ncplane_set_fg_rgb8(plane, 0x8b, 0xae, 0xf2);
    ncplane_printf(plane, "[%s]", mode_str);
}

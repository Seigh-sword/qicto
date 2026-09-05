#include "tui.h"
#include "renderer.h"
#include "input.h"
#include "layout.h"
#include "command.h"
#include "module.h"
#include "config.h"
#include "buffer.h"
#include "editor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

tui_state_t* tui_init(void) {
    tui_state_t* tui = calloc(1, sizeof(tui_state_t));
    if (!tui) return NULL;

    struct notcurses_options opts = {0};
    opts.flags = NCOPTION_NO_QUIT_SIGHANDLERS;
    opts.loglevel = NCLOGLEVEL_SILENT;

    tui->nc = notcurses_init(&opts, NULL);
    if (!tui->nc) {
        free(tui);
        return NULL;
    }

    tui->stdplane = notcurses_stdplane(tui->nc);
    if (!tui->stdplane) {
        notcurses_stop(tui->nc);
        free(tui);
        return NULL;
    }

    tui->width = ncplane_dim_x(tui->stdplane);
    tui->height = ncplane_dim_y(tui->stdplane);
    tui->initialized = true;
    tui->pending_mode = QICTO_MODE_NORMAL;

    return tui;
}

void tui_deinit(tui_state_t* tui) {
    if (!tui) return;
    if (tui->nc) {
        notcurses_stop(tui->nc);
    }
    free(tui);
}

void tui_refresh_size(tui_state_t* tui) {
    if (!tui || !tui->stdplane) return;
    tui->width = ncplane_dim_x(tui->stdplane);
    tui->height = ncplane_dim_y(tui->stdplane);
}

void tui_flush(tui_state_t* tui) {
    if (!tui || !tui->nc) return;
    notcurses_render(tui->nc);
}

static void tui_clear(tui_state_t* tui) {
    if (!tui || !tui->stdplane) return;
    ncplane_erase(tui->stdplane);
}

int tui_render(tui_state_t* tui, editor_t* ed) {
    if (!tui || !ed || !tui->stdplane) return 0;
    tui_refresh_size(tui);

    tui_clear(tui);

    buffer_t* buf = ed->current_buffer;
    if (!buf) {
        ncplane_putstr(tui->stdplane, "[no buffer]");
        notcurses_render(tui->nc);
        return 0;
    }

    if (!buf->render_valid) {
        buffer_update_render(buf);
    }

    int content_height = tui->height - 2;
    int line_num_width = 0;
    if (ed->config && ed->config->show_line_numbers) {
        int max_digits = 1;
        int n = (int)buf->text.line_count;
        while (n >= 10) { max_digits++; n /= 10; }
        line_num_width = max_digits + 2;
    }

    int cursor_screen_y = 0;
    int cursor_screen_x = 0;

    for (int i = 0; i < content_height && (size_t)i < buf->text.line_count; i++) {
        size_t line_idx = (size_t)i;
        const char* text = buffer_get_line(buf, line_idx);
        if (!text) continue;

        int screen_y = i;

        if (line_num_width > 0) {
            ncplane_cursor_move_yx(tui->stdplane, screen_y, 0);
            ncplane_set_styles(tui->stdplane, NCSTYLE_UNDERLINE);
            ncplane_printf(tui->stdplane, "%*zu ", line_num_width - 2, line_idx + 1);
            ncplane_set_styles(tui->stdplane, NCSTYLE_NONE);
        }

        int x_offset = line_num_width;
        ncplane_cursor_move_yx(tui->stdplane, screen_y, x_offset);

        if (line_idx == buf->cursor.cursor_line) {
            cursor_screen_y = screen_y;
            cursor_screen_x = x_offset + (int)buf->cursor.cursor_col;
        }

        ncplane_set_fg_rgb8(tui->stdplane, 0xc5, 0xc8, 0xc6);
        ncplane_putstr(tui->stdplane, text);
        ncplane_set_fg_rgb8(tui->stdplane, 0xdf, 0xdf, 0xdf);
    }

    renderer_render_statusbar(tui->stdplane, ed, 0, tui->height - 2, tui->width);
    renderer_render_cmdline(tui->stdplane, ed, 0, tui->height - 1, tui->width);

    if (ed->mods) {
        mod_registry_render_all(ed->mods, ed, (void*)tui->stdplane);
    }

    ncplane_cursor_move_yx(tui->stdplane, cursor_screen_y, cursor_screen_x);
    notcurses_render(tui->nc);
    return 0;
}

qkey_t tui_read_key(tui_state_t* tui) {
    if (!tui || !tui->nc) return QICTO_KEY_NONE;

    struct ncinput input = {0};
    int rc = notcurses_get(tui->nc, 0, &input);
    if (rc < 0) {
        notcurses_stop(tui->nc);
        exit(1);
    }
    if (rc == 0) return QICTO_KEY_NONE;

    return input_map_nckey(&input, rc);
}

void tui_handle_key(tui_state_t* tui, editor_t* ed, qkey_t key) {
    if (!tui || !ed) return;

    if (ed->mode == QICTO_MODE_COMMAND) {
        input_handle_command(ed, key);
        tui_render(tui, ed);
        return;
    }

    if (ed->mods) {
        qkey_t new_key = mod_registry_dispatch_key(ed->mods, ed, key);
        if (new_key == 0) return;
        key = new_key;
    }

    switch (ed->mode) {
        case QICTO_MODE_NORMAL:
            input_handle_normal(ed, key);
            break;
        case QICTO_MODE_INSERT:
            input_handle_insert(ed, key);
            break;
        case QICTO_MODE_VISUAL:
            input_handle_visual(ed, key);
            break;
        default:
            break;
    }

    tui_render(tui, ed);
}

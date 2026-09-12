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

static int compute_line_num_width(buffer_t* buf) {
    int w = 0;
    int max_digits = 1;
    int n = (int)buf->text.line_count;
    while (n >= 10) { max_digits++; n /= 10; }
    w = max_digits + 2;
    if (w < 3) w = 3;
    return w;
}

static void draw_pane(struct ncplane* plane, editor_t* ed, buffer_t* buf,
                      int x, int y, int width, int height, bool is_active,
                      size_t* scroll_top_out, size_t* scroll_col_out) {
    if (!buf || width <= 0 || height <= 0) return;
    if (!buf->render_valid) {
        buffer_update_render(buf);
    }

    int line_num_width = 0;
    if (ed->config && ed->config->show_line_numbers) {
        line_num_width = compute_line_num_width(buf);
        if (line_num_width >= width) line_num_width = width / 4;
    }

    buffer_t* saved_buf = ed->current_buffer;
    size_t saved_top_line = ed->top_line;
    size_t saved_col_off  = ed->col_offset;

    ed->current_buffer = buf;
    ed->top_line = scroll_top_out ? *scroll_top_out : 0;
    ed->col_offset = scroll_col_out ? *scroll_col_out : 0;

    editor_scroll_to_cursor(ed, height, width - line_num_width);
    if (ed->top_line > 0 && ed->top_line >= buf->text.line_count) {
        ed->top_line = buf->text.line_count > 0 ? buf->text.line_count - 1 : 0;
    }

    if (line_num_width > 0) {
        for (int i = 0; i < height; i++) {
            size_t line_idx = ed->top_line + (size_t)i;
            if (line_idx >= buf->text.line_count) break;
            char linestr[32];
            snprintf(linestr, sizeof(linestr), "%*lu ",
                     line_num_width - 2, (unsigned long)(line_idx + 1));
            ncplane_cursor_move_yx(plane, y + i, x);
            ncplane_set_styles(plane, NCSTYLE_UNDERLINE);
            ncplane_putstr(plane, linestr);
            ncplane_set_styles(plane, NCSTYLE_NONE);
        }
    }

    renderer_render_buffer(plane, buf,
                           x + line_num_width, y,
                           width - line_num_width, height,
                           false);

    if (is_active && buf->cursor.has_selection) {
        size_t al = buf->cursor.sel_anchor_line;
        size_t ac = buf->cursor.sel_anchor_col;
        size_t cl = buf->cursor.sel_cursor_line;
        size_t cc = buf->cursor.sel_cursor_col;
        if (al > cl || (al == cl && ac > cc)) {
            size_t t;
            t = al; al = cl; cl = t;
            t = ac; ac = cc; cc = t;
        }
        ncplane_set_bg_rgb8(plane, 0x35, 0x32, 0x2f);
        for (size_t l = al; l <= cl && (int)(l - ed->top_line) < height; l++) {
            if (l < ed->top_line) continue;
            int sy = y + (int)(l - ed->top_line);
            int sx = x + line_num_width;
            int ex = x + width - 1;
            if (l == al) sx = x + line_num_width + (int)ac - (int)ed->col_offset;
            if (l == cl) {
                int e = x + line_num_width + (int)cc - (int)ed->col_offset;
                if (e < sx) e = sx;
                ex = e;
            }
            if (sx < x) sx = x;
            if (ex >= x + width) ex = x + width - 1;
            ncplane_cursor_move_yx(plane, sy, sx);
            for (int xx = sx; xx <= ex; xx++) ncplane_putchar(plane, ' ');
        }
        ncplane_set_bg_rgb8(plane, 0x1e, 0x1e, 0x2e);
        renderer_render_buffer(plane, buf,
                               x + line_num_width, y,
                               width - line_num_width, height,
                               false);
    }

    if (scroll_top_out) *scroll_top_out = ed->top_line;
    if (scroll_col_out) *scroll_col_out = ed->col_offset;

    ed->current_buffer = saved_buf;
    ed->top_line = saved_top_line;
    ed->col_offset = saved_col_off;
}

static int walk_leaves(qicto_window_t* w,
                       int (*cb)(qicto_window_t*, void*), void* ud) {
    if (!w) return 0;
    if (!w->child) {
        return cb(w, ud);
    }
    int r = walk_leaves(w->child, cb, ud);
    if (r) return r;
    return walk_leaves(w->sibling, cb, ud);
}

typedef struct {
    struct ncplane* plane;
    editor_t* ed;
    int* cursor_x_out;
    int* cursor_y_out;
    int found_active;
} leaf_draw_ud_t;

static int draw_leaf_cb(qicto_window_t* leaf, void* udv) {
    leaf_draw_ud_t* ud = (leaf_draw_ud_t*)udv;
    buffer_t* b = leaf->buffer;
    if (!b) b = ud->ed->current_buffer;
    bool active = (leaf == ud->ed->layout.active) || (leaf == ud->ed->layout.root && !ud->ed->layout.root->child);
    draw_pane(ud->plane, ud->ed, b, leaf->x, leaf->y, leaf->width, leaf->height,
              active, &leaf->top_line, &leaf->col_offset);
    if (active && b) {
        size_t cur_line = b->cursor.cursor_line;
        if (cur_line >= leaf->top_line &&
            (int)(cur_line - leaf->top_line) < leaf->height) {
            *ud->cursor_y_out = leaf->y + (int)(cur_line - leaf->top_line);
        } else {
            *ud->cursor_y_out = leaf->y;
        }
        int lnw = 0;
        if (ud->ed->config && ud->ed->config->show_line_numbers) {
            lnw = compute_line_num_width(b);
            if (lnw >= leaf->width) lnw = leaf->width / 4;
        }
        int ccol = (int)b->cursor.cursor_col - (int)leaf->col_offset;
        if (ccol < 0) ccol = 0;
        *ud->cursor_x_out = leaf->x + lnw + ccol;
    }
    return 0;
}

static void draw_separators(struct ncplane* plane, qicto_window_t* root) {
    if (!root || !root->child) return;
    int y0 = root->y;
    int x0 = root->x;
    int w0 = root->width;
    int h0 = root->height;
    if (root->split == 1) {
        int mid_y = root->child->y + root->child->height;
        if (mid_y < y0 + h0) {
            for (int xi = x0; xi < x0 + w0; xi++) {
                ncplane_cursor_move_yx(plane, mid_y, xi);
                ncplane_putstr(plane, "─");
            }
        }
    } else if (root->split == 2) {
        int mid_x = root->child->x + root->child->width;
        if (mid_x < x0 + w0) {
            for (int yi = y0; yi < y0 + h0; yi++) {
                ncplane_cursor_move_yx(plane, yi, mid_x);
                ncplane_putstr(plane, "│");
            }
        }
    }
    draw_separators(plane, root->child);
    draw_separators(plane, root->sibling);
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

    int status_h = 2;
    int content_height = tui->height - status_h;
    if (content_height < 1) content_height = 1;

    qicto_window_t* root = ed->layout.root ? ed->layout.root : NULL;
    int cursor_screen_x = 0;
    int cursor_screen_y = 0;

    if (root) {
        qicto_layout_resize(root, 0, 0, tui->width, content_height);
        leaf_draw_ud_t ud = {
            .plane = tui->stdplane,
            .ed = ed,
            .cursor_x_out = &cursor_screen_x,
            .cursor_y_out = &cursor_screen_y,
            .found_active = 0,
        };
        walk_leaves(root, draw_leaf_cb, &ud);
        draw_separators(tui->stdplane, root);
    } else {
        draw_pane(tui->stdplane, ed, buf,
                  0, 0, tui->width, content_height, true,
                  &ed->top_line, &ed->col_offset);
        if (buf->cursor.cursor_line >= ed->top_line &&
            (int)(buf->cursor.cursor_line - ed->top_line) < content_height) {
            cursor_screen_y = (int)(buf->cursor.cursor_line - ed->top_line);
        }
        int lnw = 0;
        if (ed->config && ed->config->show_line_numbers) {
            lnw = compute_line_num_width(buf);
            if (lnw >= tui->width) lnw = tui->width / 4;
        }
        int ccol = (int)buf->cursor.cursor_col - (int)ed->col_offset;
        if (ccol < 0) ccol = 0;
        cursor_screen_x = lnw + ccol;
    }

    renderer_render_statusbar(tui->stdplane, ed, 0, tui->height - 2, tui->width);
    renderer_render_cmdline(tui->stdplane, ed, 0, tui->height - 1, tui->width);

    if (ed->mods) {
        mod_registry_render_all(ed->mods, ed, (void*)tui->stdplane);
    }

    if (cursor_screen_x < 0) cursor_screen_x = 0;
    if (cursor_screen_x >= tui->width) cursor_screen_x = tui->width - 1;
    if (cursor_screen_y < 0) cursor_screen_y = 0;
    if (cursor_screen_y >= tui->height - 1) cursor_screen_y = tui->height - 2;
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

    if (ed->mode == QICTO_MODE_COMMAND || ed->mode == QICTO_MODE_SEARCH) {
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

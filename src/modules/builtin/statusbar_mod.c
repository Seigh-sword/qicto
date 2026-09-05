#include "statusbar_mod.h"
#include "editor.h"
#include "buffer.h"
#include "renderer.h"
#include "module.h"
#include <notcurses/notcurses.h>
#include <stdio.h>
#include <string.h>

static qicto_cmd_result_t statusbar_init(editor_t* ed) {
    (void)ed;
    return QICTO_CMD_SUCCESS;
}

static void statusbar_cleanup(editor_t* ed) {
    (void)ed;
}

static void statusbar_on_render(editor_t* ed, void* ncp) {
    if (!ed || !ncp) return;
    struct ncplane* plane = (struct ncplane*)ncp;

    int cols = ncplane_dim_x(plane);

    buffer_t* buf = ed->current_buffer;
    const char* name = buffer_display_name(buf);
    bool dirty = buf ? buf->dirty : false;

    ncplane_set_bg_rgb8(plane, 0x2d, 0x2a, 0x27);
    ncplane_set_fg_rgb8(plane, 0xe6, 0xe6, 0xe6);

    ncplane_cursor_move_yx(plane, 0, 0);
    ncplane_printf(plane, " %s", name ? name : "[No Name]");
    if (dirty) {
        ncplane_set_fg_rgb8(plane, 0xf3, 0x8b, 0xa8);
        ncplane_putchar(plane, '*');
        ncplane_set_fg_rgb8(plane, 0xe6, 0xe6, 0xe6);
    }

    if (buf) {
        ncplane_printf(plane, " [%zux%zu]", buf->text.line_count,
                       buf->text.line_count > 0 ? strlen(buf->text.lines[0]) : 0);
    }

    ncplane_cursor_move_yx(plane, 0, cols - 20);
    const char* mode_str = "NORMAL";
    uint8_t mr = 0xe6, mg = 0xe6, mb = 0xe6;
    if (ed->mode == QICTO_MODE_INSERT) {
        mode_str = "INSERT";
        mr = 0x50; mg = 0xfa; mb = 0x77;
    } else if (ed->mode == QICTO_MODE_VISUAL || ed->mode == QICTO_MODE_SELECT) {
        mode_str = "VISUAL";
        mr = 0xff; mg = 0xbd; mb = 0x65;
    }
    ncplane_set_fg_rgb8(plane, mr, mg, mb);
    ncplane_printf(plane, "[%s]", mode_str);
    ncplane_set_fg_rgb8(plane, 0xe6, 0xe6, 0xe6);
}

static void statusbar_on_mode_change(editor_t* ed, qicto_mode_t from, qicto_mode_t to) {
    (void)ed; (void)from; (void)to;
}

static qicto_cmd_result_t statusbar_command(editor_t* ed, const char* cmd, char** out) {
    if (!ed || !cmd) return QICTO_CMD_UNKNOWN;
    if (strcmp(cmd, "mode") == 0) {
        const char* mode_str = "normal";
        if (ed->mode == QICTO_MODE_INSERT) mode_str = "insert";
        else if (ed->mode == QICTO_MODE_VISUAL || ed->mode == QICTO_MODE_SELECT) mode_str = "visual";
        else if (ed->mode == QICTO_MODE_COMMAND) mode_str = "command";
        if (out) *out = malloc(1); // empty response, just success
        return QICTO_CMD_SUCCESS;
    }
    return QICTO_CMD_UNKNOWN;
}

static const char statusbar_mod_name[] = "statusbar";
static const char statusbar_mod_version[] = "0.1.0";

static qicto_mod_api_t s_statusbar_api = {
    .name = statusbar_mod_name,
    .version = statusbar_mod_version,
    .description = "Status bar module",
    .init = statusbar_init,
    .cleanup = statusbar_cleanup,
    .on_render = statusbar_on_render,
    .on_key = NULL,
    .on_buffer_opened = NULL,
    .on_buffer_changed = NULL,
    .on_mode_change = statusbar_on_mode_change,
    .on_config_reload = NULL,
    .on_command = statusbar_command,
};

const qicto_mod_api_t* statusbar_mod_get_api(void) {
    return &s_statusbar_api;
}

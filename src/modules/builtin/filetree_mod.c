#include "filetree_mod.h"
#include "editor.h"
#include "module.h"
#include "platform.h"
#include <notcurses/notcurses.h>
#include <cwalk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char root_path[QICTO_MAX_PATH_LEN];
    char** entries;
    int entry_count;
    int selected;
    bool visible;
    int width;
} filetree_state_t;

static filetree_state_t s_tree = {0};

static qicto_cmd_result_t filetree_init(editor_t* ed) {
    (void)ed;
    memset(&s_tree, 0, sizeof(s_tree));
    s_tree.width = 30;
    s_tree.visible = false;
    return QICTO_CMD_SUCCESS;
}

static void filetree_cleanup(editor_t* ed) {
    (void)ed;
    if (s_tree.entries) {
        platform_free_dir_listing(s_tree.entries, s_tree.entry_count);
        s_tree.entries = NULL;
        s_tree.entry_count = 0;
    }
}

static void filetree_load_dir(const char* path) {
    if (s_tree.entries) {
        platform_free_dir_listing(s_tree.entries, s_tree.entry_count);
    }
    strncpy(s_tree.root_path, path, sizeof(s_tree.root_path) - 1);
    s_tree.entries = platform_list_dir(path, &s_tree.entry_count);
    s_tree.selected = 0;
}

static void filetree_on_render(editor_t* ed, void* ncp) {
    if (!ed || !ncp || !s_tree.visible || !s_tree.entries) return;
    struct ncplane* plane = (struct ncplane*)ncp;

    int cols = ncplane_dim_x(plane);
    int rows = ncplane_dim_y(plane);
    int ft_width = s_tree.width;
    if (ft_width > cols / 3) ft_width = cols / 3;

    ncplane_set_bg_rgb8(plane, 0x1e, 0x1e, 0x2e);
    ncplane_set_fg_rgb8(plane, 0xcd, 0xcd, 0xcd);

    ncplane_cursor_move_yx(plane, 0, 0);
    for (int i = 0; i < ft_width; i++) ncplane_putchar(plane, ' ');

    ncplane_cursor_move_yx(plane, 1, 0);
    for (int i = 0; i < rows - 1 && i < s_tree.entry_count; i++) {
        if (i == s_tree.selected) {
            ncplane_set_bg_rgb8(plane, 0x33, 0x33, 0x55);
        }
        ncplane_cursor_move_yx(plane, i + 1, 0);
        ncplane_printf(plane, " %s", s_tree.entries[i]);
        ncplane_set_bg_rgb8(plane, 0x1e, 0x1e, 0x2e);
    }
}

static qkey_t filetree_on_key(editor_t* ed, qkey_t key) {
    if (!s_tree.visible) return key;

    switch (key) {
        case 'j':
            s_tree.selected++;
            if (s_tree.selected >= s_tree.entry_count) s_tree.selected = s_tree.entry_count - 1;
            if (s_tree.selected < 0) s_tree.selected = 0;
            break;
        case 'k':
            s_tree.selected--;
            if (s_tree.selected < 0) s_tree.selected = 0;
            break;
        case QICTO_KEY_ENTER:
            if (s_tree.entries && s_tree.entry_count > 0 && s_tree.selected >= 0) {
                char full_path[4096];
                cwk_path_join(s_tree.root_path, s_tree.entries[s_tree.selected],
                              full_path, sizeof(full_path));
                if (platform_is_dir(full_path)) {
                    filetree_load_dir(full_path);
                } else {
                    editor_open_file(ed, full_path);
                    s_tree.visible = false;
                }
            }
            break;
        case QICTO_KEY_ESC:
            s_tree.visible = false;
            break;
        case '\t':
            s_tree.visible = !s_tree.visible;
            break;
    }
    return key;
}

static void filetree_on_buffer_opened(editor_t* ed, buffer_t* buf) {
    (void)ed; (void)buf;
}

static qicto_cmd_result_t filetree_command(editor_t* ed, const char* cmd, char** out) {
    if (!ed || !cmd) return QICTO_CMD_UNKNOWN;

    if (strcmp(cmd, "filetree") == 0 || strcmp(cmd, "tree") == 0) {
        s_tree.visible = !s_tree.visible;
        if (s_tree.visible && !s_tree.entries) {
            const char* dir = ".";
            if (ed->config && ed->config->project_dir[0]) {
                dir = ed->config->project_dir;
            }
            filetree_load_dir(dir);
        }
        return QICTO_CMD_SUCCESS;
    }
    return QICTO_CMD_UNKNOWN;
}

static const char filetree_mod_name[] = "filetree";
static const char filetree_mod_version[] = "0.1.0";

static qicto_mod_api_t s_filetree_api = {
    .name = filetree_mod_name,
    .version = filetree_mod_version,
    .description = "File tree sidebar",
    .init = filetree_init,
    .cleanup = filetree_cleanup,
    .on_render = filetree_on_render,
    .on_key = filetree_on_key,
    .on_buffer_opened = filetree_on_buffer_opened,
    .on_buffer_changed = NULL,
    .on_mode_change = NULL,
    .on_config_reload = NULL,
    .on_command = filetree_command,
};

const qicto_mod_api_t* filetree_mod_get_api(void) {
    return &s_filetree_api;
}

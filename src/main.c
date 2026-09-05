#include "qicto.h"
#include "engine/editor.h"
#include "engine/module.h"
#include "engine/config.h"
#include "engine/command.h"
#include "engine/buffer.h"
#include "ui/tui.h"
#include "platform/platform.h"
#include "modules/builtin/mod_builtins.h"

#include <cargs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static cag_option cli_options[] = {
    {.identifier = 'h', .access_name = "help", .access_letters = "h", .description = "Display this help text"},
    {.identifier = 'v', .access_name = "version", .access_letters = "v", .description = "Display version information"},
    {.identifier = 'c', .access_name = "config", .access_letters = "c", .value_name = "FILE", .description = "Path to config file"},
    {.identifier = 'd', .access_name = "dir", .access_letters = "d", .value_name = "DIR", .description = "Project directory to edit"},
    {.identifier = 0, .access_name = NULL, .access_letters = NULL, .value_name = NULL, .description = NULL}
};

static void print_help(const char* prog) {
    printf("qicto %s - A fast, modular TUI text editor\n\n", QICTO_VERSION_STRING);
    printf("Usage: %s [options] <file>...\n\n", prog);
    printf("Options:\n");
    printf("  -h, --help        Show this help message\n");
    printf("  -v, --version     Show version\n");
    printf("  -c, --config      Path to config file (default: ~/.config/qicto/config.ini)\n");
    printf("  -d, --dir         Project directory to edit (default: current directory)\n");
    printf("\nKeybindings:\n");
    printf("  i        Enter insert mode\n");
    printf("  Esc      Return to normal mode\n");
    printf("  h/j/k/l  Move cursor left/down/up/right\n");
    printf("  :        Enter command mode\n");
    printf("  /        Enter search mode\n");
    printf("  w        Save file\n");
    printf("  q        Quit (if no unsaved changes)\n");
    printf("  b/B      Next/prev buffer\n");
}

int main(int argc, char* argv[]) {
    cag_option_context ctx;
    cag_option_prepare(&ctx, cli_options, CAG_ARRAY_SIZE(cli_options) - 1, argc, argv);

    const char* config_file = NULL;
    const char* project_dir = NULL;
    int file_start = 1;

    while (cag_option_fetch(&ctx)) {
        char identifier = cag_option_get(&ctx);
        const char* value = cag_option_get_value(&ctx);

        switch (identifier) {
            case 'h':
                print_help(argv[0]);
                return 0;
            case 'v':
                printf("qicto %s\n", QICTO_VERSION_STRING);
                return 0;
            case 'c':
                config_file = value;
                break;
            case 'd':
                project_dir = value;
                break;
        }
    }

    file_start = cag_option_get_index(&ctx);

    platform_init();

    editor_t* ed = editor_create();
    if (!ed) {
        fprintf(stderr, "qicto: failed to create editor\n");
        return 1;
    }

    if (config_file) {
        config_load(ed->config, config_file);
    } else {
        char* config_dir = platform_config_dir();
        if (config_dir) {
            char config_path[4096];
            snprintf(config_path, sizeof(config_path), "%s/qicto.ini", config_dir);
            config_load(ed->config, config_path);
            free(config_dir);
        }
    }

    ed->mods = mod_registry_create();
    mod_builtins_register(ed);

    if (project_dir) {
        strncpy(ed->config->project_dir, project_dir, sizeof(ed->config->project_dir) - 1);
    }

    mod_registry_load_dir(ed->mods, ed, ed->config->mods_dir);
    mod_registry_init_all(ed->mods, ed);

    if (argc - file_start > 0) {
        for (int i = file_start; i < argc; i++) {
            if (platform_is_dir(argv[i])) {
                editor_set_status(ed, "opening directory: %s", argv[i]);
            } else {
                editor_open_file(ed, argv[i]);
            }
        }
    } else {
        ed->current_buffer = buffer_new(NULL);
        ed->buffers = ed->current_buffer;
        ed->buffer_count = 1;
    }

    tui_state_t* tui = tui_init();
    if (!tui) {
        fprintf(stderr, "qicto: failed to initialize TUI (try a different terminal)\n");
        editor_destroy(ed);
        platform_deinit();
        return 1;
    }

    editor_redraw(ed);
    tui_render(tui, ed);

    while (!ed->quit_requested) {
        qkey_t key = tui_read_key(tui);
        if (key == QICTO_KEY_NONE) continue;

        if (key == QICTO_KEY_ESC) {
            if (ed->mode == QICTO_MODE_NORMAL) {
                editor_set_status(ed, "");
                tui_render(tui, ed);
            } else {
                tui_handle_key(tui, ed, key);
            }
            continue;
        }

        tui_handle_key(tui, ed, key);
    }

    tui_deinit(tui);
    editor_destroy(ed);
    platform_deinit();

    return 0;
}

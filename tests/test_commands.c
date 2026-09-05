#include "test.h"
#include "../src/engine/command.h"
#include "../src/engine/config.h"
#include "../src/engine/editor.h"
#include "../src/utils/strings.h"

TEST_BEGIN(test_commands_register_find) {
    command_registry_t* cmds = commands_create();
    ASSERT(cmds != NULL, "commands registry should not be NULL");

    int rc = commands_register(cmds, "quit", cmd_version, "Quit");
    ASSERT(rc == 0, "register should succeed");
    ASSERT(cmds->count == 1, "should have 1 command");

    command_entry_t* found = commands_find(cmds, "quit");
    ASSERT(found != NULL, "should find 'quit' command");
    ASSERT_STR_EQ(found->name, "quit", "name should match");

    command_entry_t* notfound = commands_find(cmds, "nonexistent");
    ASSERT(notfound == NULL, "should not find nonexistent command");

    commands_destroy(cmds);
}

TEST_BEGIN(test_commands_execute) {
    command_registry_t* cmds = commands_create();
    commands_register(cmds, "version", cmd_version, "Show version");

    editor_t* ed = editor_create();
    char* out = NULL;

    qicto_cmd_result_t rc = commands_execute(cmds, ed, "version", &out);
    ASSERT(rc == QICTO_CMD_SUCCESS, "version command should succeed");

    if (out) {
        ASSERT(qstr_starts_with(out, "qicto") == 1 || strstr(out, "qicto") != NULL,
               "output should contain 'qicto'");
        free(out);
    }

    commands_destroy(cmds);
    editor_destroy(ed);
}

TEST_BEGIN(test_config_set_get) {
    config_t* cfg = config_create();
    ASSERT(cfg != NULL, "config should not be NULL");

    config_set(cfg, "test_key", "test_value");
    const char* val = config_get(cfg, "test_key");
    ASSERT(val != NULL, "should find config value");
    ASSERT_STR_EQ(val, "test_value", "value should match");

    config_set(cfg, "test_key", "updated");
    val = config_get(cfg, "test_key");
    ASSERT_STR_EQ(val, "updated", "value should be updated");

    config_destroy(cfg);
}

TEST_BEGIN(test_config_defaults) {
    config_t* cfg = config_create();
    ASSERT(cfg->tab_width == 4, "default tab width should be 4");
    ASSERT(cfg->expand_tabs == true, "should expand tabs");
    ASSERT(cfg->show_line_numbers == true, "should show line numbers");
    ASSERT(cfg->auto_indent == true, "should auto indent");
    config_destroy(cfg);
}

TEST_BEGIN(test_config_save_load) {
    char tmppath[] = "test_config.ini";
    config_t* cfg = config_create();
    config_set(cfg, "editor:test", "hello");
    int rc = config_save(cfg, tmppath);
    ASSERT(rc == 0, "should save config");
    config_destroy(cfg);

    config_t* cfg2 = config_create();
    rc = config_load(cfg2, tmppath);
    ASSERT(rc == 0, "should load config");
    const char* val = config_get(cfg2, "editor:tab_width");
    ASSERT(val != NULL, "should find tab_width after load");
    config_destroy(cfg2);
    remove(tmppath);
}

int main(void) {
    RUN_TEST(test_commands_register_find);
    RUN_TEST(test_commands_execute);
    RUN_TEST(test_config_set_get);
    RUN_TEST(test_config_defaults);
    RUN_TEST(test_config_save_load);
    TEST_SUMMARY();
}

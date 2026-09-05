#include "test.h"
#include "../src/engine/editor.h"

TEST_BEGIN(test_editor_create) {
    editor_t* ed = editor_create();
    ASSERT(ed != NULL, "editor should not be NULL");
    ASSERT(ed->mode == QICTO_MODE_NORMAL, "should start in normal mode");
    ASSERT(ed->current_buffer == NULL, "no current buffer initially");
    ASSERT(ed->buffer_count == 0, "no buffers initially");
    ASSERT(ed->config != NULL, "config should exist");
    ASSERT(ed->config->tab_width == 4, "default tab width should be 4");
    ASSERT(ed->config->expand_tabs == true, "should expand tabs by default");
    editor_destroy(ed);
}

TEST_BEGIN(test_editor_open_file) {
    char tmppath[] = "test_editor.tmp";
    FILE* f = fopen(tmppath, "w");
    ASSERT(f != NULL, "should create temp file");
    fprintf(f, "test content\nline 2\n");
    fclose(f);

    editor_t* ed = editor_create();
    buffer_t* buf = editor_open_file(ed, tmppath);
    ASSERT(buf != NULL, "should open file");
    ASSERT(ed->current_buffer == buf, "current buffer should be set");
    ASSERT(buf->text.line_count == 2, "should have 2 lines");
    ASSERT(ed->buffer_count == 1, "should have 1 buffer");
    editor_destroy(ed);
    remove(tmppath);
}

TEST_BEGIN(test_editor_cycle_buffer) {
    char tmp1[] = "test_cycle_1.tmp";
    char tmp2[] = "test_cycle_2.tmp";

    FILE* f1 = fopen(tmp1, "w"); fprintf(f1, "file 1"); fclose(f1);
    FILE* f2 = fopen(tmp2, "w"); fprintf(f2, "file 2"); fclose(f2);

    editor_t* ed = editor_create();
    editor_open_file(ed, tmp1);
    editor_open_file(ed, tmp2);
    ASSERT(ed->buffer_count == 2, "should have 2 buffers");

    buffer_t* buf1 = ed->current_buffer;
    editor_cycle_buffer(ed, 1);
    if (ed->current_buffer == buf1) {
        editor_cycle_buffer(ed, 1);
    }
    ASSERT(ed->current_buffer != buf1, "should cycle to different buffer");

    editor_cycle_buffer(ed, -1);
    ASSERT(ed->current_buffer == buf1, "should cycle back to original buffer");

    editor_destroy(ed);
    remove(tmp1);
    remove(tmp2);
}

TEST_BEGIN(test_editor_set_mode) {
    editor_t* ed = editor_create();
    editor_set_mode(ed, QICTO_MODE_INSERT);
    ASSERT(ed->mode == QICTO_MODE_INSERT, "should be in insert mode");
    editor_set_mode(ed, QICTO_MODE_NORMAL);
    ASSERT(ed->mode == QICTO_MODE_NORMAL, "should be back in normal mode");
    editor_destroy(ed);
}

int main(void) {
    RUN_TEST(test_editor_create);
    RUN_TEST(test_editor_open_file);
    RUN_TEST(test_editor_cycle_buffer);
    RUN_TEST(test_editor_set_mode);
    TEST_SUMMARY();
}

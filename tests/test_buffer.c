#include "test.h"
#include "../src/engine/buffer.h"

TEST_BEGIN(test_buffer_create) {
    buffer_t* buf = buffer_new(NULL);
    ASSERT(buf != NULL, "buffer should not be NULL");
    ASSERT(buf->text.line_count == 1, "buffer should have 1 line");
    ASSERT(buf->dirty == false, "buffer should not be dirty");
    ASSERT(buf->syntax == QICTO_SYNTAX_UNKNOWN, "syntax should be unknown");
    buffer_free(buf);
}

TEST_BEGIN(test_buffer_insert_char) {
    buffer_t* buf = buffer_new(NULL);
    buffer_insert_char(buf, 0, 0, 'h');
    buffer_insert_char(buf, 0, 1, 'i');
    const char* line = buffer_get_line(buf, 0);
    ASSERT_STR_EQ(line, "hi", "buffer should contain 'hi'");
    buffer_free(buf);
}

TEST_BEGIN(test_buffer_insert_text) {
    buffer_t* buf = buffer_new(NULL);
    buffer_insert_text(buf, 0, 0, "Hello, World!");
    const char* line = buffer_get_line(buf, 0);
    ASSERT_STR_EQ(line, "Hello, World!", "buffer should contain 'Hello, World!'");
    buffer_free(buf);
}

TEST_BEGIN(test_buffer_remove_char) {
    buffer_t* buf = buffer_new(NULL);
    buffer_insert_text(buf, 0, 0, "Hello");
    buffer_remove_char(buf, 0, 2);
    const char* line = buffer_get_line(buf, 0);
    ASSERT_STR_EQ(line, "Helo", "buffer should contain 'Helo'");
    buffer_free(buf);
}

TEST_BEGIN(test_buffer_split_join) {
    buffer_t* buf = buffer_new(NULL);
    buffer_insert_text(buf, 0, 0, "Hello World");
    buffer_split_line(buf, 0, 5);
    ASSERT(buf->text.line_count == 2, "should have 2 lines after split");
    const char* line0 = buffer_get_line(buf, 0);
    const char* line1 = buffer_get_line(buf, 1);
    ASSERT_STR_EQ(line0, "Hello", "first line should be 'Hello'");
    ASSERT_STR_EQ(line1, " World", "second line should be ' World'");
    buffer_join_lines(buf, 0);
    ASSERT(buf->text.line_count == 1, "should have 1 line after join");
    buffer_free(buf);
}

TEST_BEGIN(test_buffer_set_cursor) {
    buffer_t* buf = buffer_new(NULL);
    buffer_insert_text(buf, 0, 0, "Hello World");
    buffer_set_cursor(buf, 0, 5);
    ASSERT_EQ(buf->cursor.cursor_line, 0, "line should be 0");
    ASSERT_EQ(buf->cursor.cursor_col, 5, "col should be 5");
    buffer_set_cursor(buf, 0, 100);
    ASSERT_EQ(buf->cursor.cursor_col, 11, "col should be clamped to line length");
    buffer_free(buf);
}

TEST_BEGIN(test_buffer_load_save) {
    char tmppath[] = "test_buffer.tmp";
    FILE* f = fopen(tmppath, "w");
    ASSERT(f != NULL, "should be able to create temp file");
    fprintf(f, "line one\nline two\nline three");
    fclose(f);

    buffer_t* buf = buffer_new(tmppath);
    int rc = buffer_load_file(buf, tmppath);
    ASSERT(rc == 0, "should load file successfully");
    ASSERT(buf->text.line_count == 3, "should have 3 lines");
    ASSERT_STR_EQ(buffer_get_line(buf, 0), "line one", "first line");
    ASSERT_STR_EQ(buffer_get_line(buf, 1), "line two", "second line");
    ASSERT_STR_EQ(buffer_get_line(buf, 2), "line three", "third line");
    buf->dirty = true;

    rc = buffer_save(buf, tmppath);
    ASSERT(rc == 0, "should save file successfully");
    ASSERT(buf->dirty == false, "buffer should not be dirty after save");

    buffer_free(buf);
    remove(tmppath);
}

TEST_BEGIN(test_buffer_detect_syntax) {
    buffer_t* buf = buffer_new("test.py");
    ASSERT(buf->syntax == QICTO_SYNTAX_PYTHON, "should detect Python");
    buffer_free(buf);

    buf = buffer_new("main.c");
    ASSERT(buf->syntax == QICTO_SYNTAX_C, "should detect C");
    buffer_free(buf);

    buf = buffer_new("main.cpp");
    ASSERT(buf->syntax == QICTO_SYNTAX_CPP, "should detect C++");
    buffer_free(buf);

    buf = buffer_new("index.js");
    ASSERT(buf->syntax == QICTO_SYNTAX_JS, "should detect JS");
    buffer_free(buf);

    buf = buffer_new("config.json");
    ASSERT(buf->syntax == QICTO_SYNTAX_JSON, "should detect JSON");
    buffer_free(buf);
}

int main(void) {
    RUN_TEST(test_buffer_create);
    RUN_TEST(test_buffer_insert_char);
    RUN_TEST(test_buffer_insert_text);
    RUN_TEST(test_buffer_remove_char);
    RUN_TEST(test_buffer_split_join);
    RUN_TEST(test_buffer_set_cursor);
    RUN_TEST(test_buffer_load_save);
    RUN_TEST(test_buffer_detect_syntax);
    TEST_SUMMARY();
}

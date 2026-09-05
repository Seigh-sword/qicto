#include "test.h"
#include "../src/ui/layout.h"

TEST_BEGIN(test_layout_create) {
    qicto_window_t* root = qicto_layout_create();
    ASSERT(root != NULL, "root should not be NULL");
    ASSERT(root->child == NULL, "root should be a leaf initially");
    ASSERT(root->split == 0, "split direction should be 0");
    qicto_layout_destroy(root);
}

TEST_BEGIN(test_layout_split) {
    qicto_window_t* root = qicto_layout_create();
    qicto_window_t* right = qicto_layout_split(root, 2);
    ASSERT(right != NULL, "split should return new pane");
    ASSERT(root->child != NULL, "root should now have a child");
    ASSERT(root->sibling == right, "root's sibling should be the new pane");
    ASSERT(root->split == 2, "split direction should be vertical");

    ASSERT(qicto_layout_split(root, 2) == NULL,
           "cannot split a non-leaf");

    qicto_window_t* bottom = qicto_layout_split(right, 1);
    ASSERT(bottom != NULL, "split of right pane should succeed");
    ASSERT(right->split == 1, "right is now split horizontally");

    qicto_layout_destroy(root);
}

TEST_BEGIN(test_layout_resize) {
    qicto_window_t* root = qicto_layout_create();
    qicto_layout_resize(root, 0, 0, 80, 24);
    ASSERT_EQ(root->width, 80, "width should be 80");
    ASSERT_EQ(root->height, 24, "height should be 24");

    qicto_window_t* right = qicto_layout_split(root, 2);
    qicto_layout_resize(root, 0, 0, 100, 50);
    ASSERT_EQ(root->width, 50, "left half should be 50");
    ASSERT_EQ(root->height, 50, "left half should be 50");
    ASSERT_EQ(right->width, 50, "right half should be 50");
    ASSERT_EQ(right->height, 50, "right half should be 50");

    qicto_window_t* bottom = qicto_layout_split(right, 1);
    qicto_layout_resize(root, 0, 0, 100, 50);
    ASSERT_EQ(right->width, 50, "right still 50 wide");
    ASSERT_EQ(right->height, 25, "right top half 25");
    ASSERT_EQ(bottom->height, 25, "right bottom half 25");
    qicto_layout_destroy(root);
}

TEST_BEGIN(test_layout_close) {
    qicto_window_t* root = qicto_layout_create();
    qicto_window_t* right = qicto_layout_split(root, 2);
    qicto_window_t* survivor = qicto_layout_close(right);
    ASSERT(survivor != NULL, "close should return surviving pane");
    ASSERT(survivor->parent == NULL, "survivor should be new root");
    ASSERT(qicto_layout_close(survivor) == NULL, "cannot close last window");
    qicto_layout_destroy(survivor);
}

int main(void) {
    RUN_TEST(test_layout_create);
    RUN_TEST(test_layout_split);
    RUN_TEST(test_layout_resize);
    RUN_TEST(test_layout_close);
    TEST_SUMMARY();
}

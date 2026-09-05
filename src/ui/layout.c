#include "layout.h"
#include "editor.h"
#include "buffer.h"
#include <stdlib.h>
#include <string.h>

qicto_window_t* qicto_layout_create(void) {
    qicto_window_t* w = calloc(1, sizeof(qicto_window_t));
    if (!w) return NULL;
    w->id = 1;
    return w;
}

void qicto_layout_destroy(qicto_window_t* w) {
    if (!w) return;
    if (w->child) {
        qicto_layout_destroy(w->child);
        qicto_layout_destroy(w->sibling);
    }
    free(w);
}

qicto_window_t* qicto_layout_split(qicto_window_t* leaf, int direction) {
    if (!leaf || leaf->child) return NULL;
    if (direction != 1 && direction != 2) return NULL;
    qicto_window_t* sib = calloc(1, sizeof(qicto_window_t));
    if (!sib) return NULL;
    qicto_window_t* first = calloc(1, sizeof(qicto_window_t));
    if (!first) { free(sib); return NULL; }
    *first = *leaf;
    first->child = NULL;
    first->sibling = sib;
    first->split = 0;
    first->parent = leaf;
    sib->id = leaf->id + 1;
    sib->parent = leaf;
    sib->buffer = leaf->buffer;
    sib->sibling = first;
    sib->split = 0;
    leaf->child = first;
    leaf->sibling = sib;
    leaf->split = direction;
    leaf->buffer = NULL;
    return sib;
}

qicto_window_t* qicto_layout_close(qicto_window_t* leaf) {
    if (!leaf || leaf->child) return NULL;
    qicto_window_t* parent = leaf->parent;
    if (!parent) return NULL;
    qicto_window_t* survivor = leaf->sibling;
    if (parent->parent) {
        if (parent->parent->child == parent) parent->parent->child = survivor;
        if (parent->parent->sibling == parent) parent->parent->sibling = survivor;
        survivor->parent = parent->parent;
    } else {
        survivor->parent = NULL;
    }
    survivor->split = 0;
    free(parent->child);
    free(parent);
    free(leaf);
    return survivor;
}

qicto_window_t* qicto_layout_root(qicto_window_t* w) {
    if (!w) return NULL;
    while (w->parent) w = w->parent;
    return w;
}

void qicto_layout_resize(qicto_window_t* root, int x, int y, int width, int height) {
    if (!root || width <= 0 || height <= 0) return;
    if (!root->child) {
        root->x = x;
        root->y = y;
        root->width = width;
        root->height = height;
        return;
    }
    if (root->split == 1) {
        int half = height / 2;
        qicto_layout_resize(root->child, x, y, width, half);
        qicto_layout_resize(root->sibling, x, y + half, width, height - half);
    } else {
        int half = width / 2;
        qicto_layout_resize(root->child, x, y, half, height);
        qicto_layout_resize(root->sibling, x + half, y, width - half, height);
    }
}

size_t qicto_layout_leaf_count(qicto_layout_t* l) {
    (void)l;
    return 0;
}

qicto_layout_engine_t* layout_create(void) {
    qicto_layout_engine_t* lc = calloc(1, sizeof(*lc));
    if (!lc) return NULL;
    lc->root = qicto_layout_create();
    if (!lc->root) { free(lc); return NULL; }
    lc->active = lc->root;
    lc->pane_count = 1;
    lc->status_height = 2;
    lc->cmd_height = 1;
    lc->line_num_width = 4;
    return lc;
}

void layout_destroy(qicto_layout_engine_t* lc) {
    if (!lc) return;
    qicto_layout_destroy(lc->root);
    free(lc);
}

void layout_resize(qicto_layout_engine_t* lc, int width, int height) {
    if (!lc || !lc->root) return;
    if (lc->line_num_width < 3) lc->line_num_width = 3;
    int content_height = height - lc->status_height - lc->cmd_height;
    if (content_height < 1) content_height = 1;
    qicto_layout_resize(lc->root, 0, 0, width, content_height);
}

qicto_pane_t* layout_get_active(qicto_layout_engine_t* lc) {
    static qicto_pane_t s_pane;
    if (!lc || !lc->active) return NULL;
    s_pane.x = lc->active->x;
    s_pane.y = lc->active->y;
    s_pane.width = lc->active->width;
    s_pane.height = lc->active->height;
    s_pane.buffer = lc->active->buffer;
    s_pane.id = lc->active->id;
    s_pane.next = NULL;
    s_pane.prev = NULL;
    return &s_pane;
}

void layout_split_vertical(qicto_layout_engine_t* lc) {
    if (!lc || !lc->active || lc->active->child) return;
    qicto_window_t* sib = qicto_layout_split(lc->active, 2);
    if (sib) {
        lc->active = lc->active->child;
        lc->pane_count++;
    }
}

void layout_split_horizontal(qicto_layout_engine_t* lc) {
    if (!lc || !lc->active || lc->active->child) return;
    qicto_window_t* sib = qicto_layout_split(lc->active, 1);
    if (sib) {
        lc->active = lc->active->child;
        lc->pane_count++;
    }
}

void layout_close_pane(qicto_layout_engine_t* lc) {
    if (!lc || !lc->active || !lc->active->parent) return;
    qicto_window_t* survivor = qicto_layout_close(lc->active);
    if (survivor) {
        if (survivor->parent == NULL) {
            lc->root = survivor;
        }
        lc->active = survivor;
        if (lc->pane_count > 0) lc->pane_count--;
    }
}

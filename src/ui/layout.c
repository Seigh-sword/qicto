#include "layout.h"
#include "editor.h"
#include "buffer.h"
#include <stdlib.h>
#include <string.h>

qicto_layout_engine_t* layout_create(void) {
    qicto_layout_engine_t* lc = calloc(1, sizeof(qicto_layout_engine_t));
    if (!lc) return NULL;

    lc->status_height = 2;
    lc->cmd_height = 1;
    lc->line_num_width = 4;

    lc->panes = calloc(1, sizeof(qicto_pane_t));
    if (!lc->panes) {
        free(lc);
        return NULL;
    }
    lc->root = lc->panes;
    lc->active = lc->root;
    lc->pane_count = 1;

    return lc;
}

void layout_destroy(qicto_layout_engine_t* lc) {
    if (!lc) return;
    free(lc->panes);
    free(lc);
}

void layout_resize(qicto_layout_engine_t* lc, int width, int height) {
    if (!lc || !lc->root) return;

    if (lc->line_num_width < 3) lc->line_num_width = 3;

    int content_height = height - lc->status_height - lc->cmd_height;
    if (content_height < 1) content_height = 1;

    lc->root->x = 0;
    lc->root->y = 0;
    lc->root->width = width;
    lc->root->height = content_height;
}

qicto_pane_t* layout_get_active(qicto_layout_engine_t* lc) {
    return lc ? lc->active : NULL;
}

static void pane_split_vertical(qicto_pane_t* pane) {
    int half_w = pane->width / 2;
    pane->x = 0;
    pane->width = half_w;

    qicto_pane_t* new_pane = calloc(1, sizeof(qicto_pane_t));
    new_pane->x = half_w;
    new_pane->y = pane->y;
    new_pane->width = pane->width - half_w;
    new_pane->height = pane->height;
    new_pane->id = pane->id + 1000;
    new_pane->buffer = pane->buffer;
}

static void pane_split_horizontal(qicto_pane_t* pane) {
    int half_h = pane->height / 2;
    pane->height = half_h;

    qicto_pane_t* new_pane = calloc(1, sizeof(qicto_pane_t));
    new_pane->x = pane->x;
    new_pane->y = pane->y + half_h;
    new_pane->width = pane->width;
    new_pane->height = pane->height - half_h;
    new_pane->id = pane->id + 2000;
    new_pane->buffer = pane->buffer;
}

void layout_split_vertical(qicto_layout_engine_t* lc) {
    if (!lc || !lc->root) return;
    pane_split_vertical(lc->root);
}

void layout_split_horizontal(qicto_layout_engine_t* lc) {
    if (!lc || !lc->root) return;
    pane_split_horizontal(lc->root);
}

void layout_close_pane(qicto_layout_engine_t* lc) {
    if (!lc || !lc->root) return;
}

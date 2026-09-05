#ifndef QICTO_LAYOUT_H
#define QICTO_LAYOUT_H

#include "qicto.h"

typedef struct qicto_pane_t {
    int x, y, width, height;
    buffer_t* buffer;
    int id;
    struct qicto_pane_t* next;
    struct qicto_pane_t* prev;
} qicto_pane_t;

typedef struct {
    qicto_pane_t* root;
    qicto_pane_t* active;
    qicto_pane_t* panes;
    int pane_count;
    int active_status_height;
    int active_cmd_height;
    int line_num_width;
    int status_height;
    int cmd_height;
} qicto_layout_engine_t;

qicto_layout_engine_t* layout_create(void);
void layout_destroy(qicto_layout_engine_t* lc);
void layout_resize(qicto_layout_engine_t* lc, int width, int height);
qicto_pane_t* layout_get_active(qicto_layout_engine_t* lc);
void layout_split_vertical(qicto_layout_engine_t* lc);
void layout_split_horizontal(qicto_layout_engine_t* lc);
void layout_close_pane(qicto_layout_engine_t* lc);

#endif

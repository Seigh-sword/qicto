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

/* New tree-based layout. qicto_window_t is defined in qicto.h and is
 * the primary type. The old qicto_layout_engine_t compat shim is
 * retained so existing callers don't break, but new code should use
 * qicto_layout_* directly. */
typedef struct {
    qicto_window_t* root;
    qicto_window_t* active;
    size_t pane_count;
    int active_status_height;
    int active_cmd_height;
    int line_num_width;
    int status_height;
    int cmd_height;
} qicto_layout_engine_t;

qicto_window_t* qicto_layout_create(void);
void qicto_layout_destroy(qicto_window_t* root);
qicto_window_t* qicto_layout_split(qicto_window_t* leaf, int direction);
qicto_window_t* qicto_layout_close(qicto_window_t* leaf);
qicto_window_t* qicto_layout_root(qicto_window_t* w);
void qicto_layout_resize(qicto_window_t* root, int x, int y, int width, int height);
size_t qicto_layout_leaf_count(qicto_layout_t* layout);

/* Old API (shim). */
qicto_layout_engine_t* layout_create(void);
void layout_destroy(qicto_layout_engine_t* lc);
void layout_resize(qicto_layout_engine_t* lc, int width, int height);
qicto_pane_t* layout_get_active(qicto_layout_engine_t* lc);
void layout_split_vertical(qicto_layout_engine_t* lc);
void layout_split_horizontal(qicto_layout_engine_t* lc);
void layout_close_pane(qicto_layout_engine_t* lc);

#endif

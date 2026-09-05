#ifndef QICTO_DYNARRAY_H
#define QICTO_DYNARRAY_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define DYNARRAY_INIT_CAP 16

#define dynarray_push(arr, count, capacity, item, type) \
    do { \
        if ((count) >= (capacity)) { \
            size_t _newcap = (capacity) ? (capacity) * 2 : DYNARRAY_INIT_CAP; \
            type* _tmp = realloc((arr), _newcap * sizeof(type)); \
            if (!_tmp) break; \
            (arr) = _tmp; \
            (capacity) = _newcap; \
        } \
        (arr)[(count)++] = (item); \
    } while (0)

#define dynarray_free(arr) \
    do { \
        free(arr); \
        arr = NULL; \
    } while (0)

#endif

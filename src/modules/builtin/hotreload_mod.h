#ifndef QICTO_HOTRELOAD_MOD_H
#define QICTO_HOTRELOAD_MOD_H

#include "qicto.h"
#include "module.h"

const qicto_mod_api_t* hotreload_mod_get_api(void);
void hotreload_tick(editor_t* ed);

#endif

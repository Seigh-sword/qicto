#ifndef QICTO_INPUT_H
#define QICTO_INPUT_H

#include "qicto.h"
#include <notcurses/notcurses.h>

qkey_t input_map_nckey(struct ncinput* input, int nc_key);
qkey_t input_map_char(uint32_t codepoint);
int input_is_modifier(qkey_t key);
const char* key_name(qkey_t key);

void input_handle_normal(editor_t* ed, qkey_t key);
void input_handle_insert(editor_t* ed, qkey_t key);
void input_handle_visual(editor_t* ed, qkey_t key);
void input_handle_command(editor_t* ed, qkey_t key);

#endif

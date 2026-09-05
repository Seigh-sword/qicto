#ifndef QICTO_FILEOPS_H
#define QICTO_FILEOPS_H

#include <stddef.h>
#include "qicto.h"

int fileops_read_text(const char* path, char** out_content, size_t* out_size);
int fileops_write_text(const char* path, const char* content, size_t size);
int fileops_read_lines(const char* path, char*** out_lines, size_t* out_count);
qicto_syntax_t fileops_detect_syntax(const char* filename);
int fileops_is_binary(const char* path);
int fileops_backup(const char* path);
void fileops_free_lines(char** lines, size_t count);

#endif

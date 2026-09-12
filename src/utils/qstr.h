#ifndef QICTO_STRINGS_H
#define QICTO_STRINGS_H

#include <stddef.h>

char* qstr_dup(const char* s);
char* qstr_ndup(const char* s, size_t n);
char* qstr_concat(const char* a, const char* b);
char* qstr_trim(char* s);
char* qstr_ltrim(char* s);
char* qstr_rtrim(char* s);
char** qstr_split(const char* s, char delim, int* count);
void qstr_free_split(char** arr, int count);
int qstr_starts_with(const char* s, const char* prefix);
int qstr_ends_with(const char* s, const char* suffix);
size_t qstr_display_width(const char* s);
char* qstr_to_lower(const char* s);
char* qstr_to_upper(const char* s);

#endif

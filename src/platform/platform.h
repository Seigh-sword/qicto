#ifndef QICTO_PLATFORM_H
#define QICTO_PLATFORM_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t total_physical_memory;
    uint64_t available_physical_memory;
    int cpu_count;
    char hostname[256];
    char username[256];
} qicto_sysinfo_t;

typedef struct {
    char filename[512];
    uint64_t size;
    int is_directory;
    int is_hidden;
    uint64_t modified_time;
} qicto_dir_entry_t;

int platform_init(void);
void platform_deinit(void);
void platform_get_sysinfo(qicto_sysinfo_t* info);
int platform_file_exists(const char* path);
int64_t platform_file_size(const char* path);
time_t platform_file_mtime(const char* path);
int platform_file_read(const char* path, char** out_data, size_t* out_size);
int platform_file_write(const char* path, const char* data, size_t size);
int platform_mkdir(const char* path);
int platform_is_dir(const char* path);
char** platform_list_dir(const char* path, int* count);
void platform_free_dir_listing(char** list, int count);
char* platform_resolve_path(const char* path);
char* platform_home_dir(void);
char* platform_config_dir(void);
int platform_dl_open(const char* path);
void platform_dl_close(int handle);
void* platform_dl_sym(int handle, const char* name);
char* platform_dl_error(void);

#endif

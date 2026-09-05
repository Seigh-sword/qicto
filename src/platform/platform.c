#include "platform.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <fileapi.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <pwd.h>
#endif

int platform_init(void) {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) return -1;
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
    return 0;
}

void platform_deinit(void) {
}

void platform_get_sysinfo(qicto_sysinfo_t* info) {
    if (!info) return;
    memset(info, 0, sizeof(qicto_sysinfo_t));
#ifdef _WIN32
    SYSTEM_INFO si = {0};
    GetSystemInfo(&si);
    info->cpu_count = (int)si.dwNumberOfProcessors;
    MEMORYSTATUSEX mem = {0};
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);
    info->total_physical_memory = mem.ullTotalPhys;
    info->available_physical_memory = mem.ullAvailPhys;
    DWORD len = sizeof(info->hostname);
    GetComputerNameA(info->hostname, &len);
    len = sizeof(info->username);
    GetUserNameA(info->username, &len);
#else
    info->cpu_count = (int)sysconf(_SC_NPROCESSORS_ONLN);
    FILE* f = fopen("/proc/meminfo", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            unsigned long val;
            if (sscanf(line, "MemTotal: %lu", &val) == 1)
                info->total_physical_memory = val * 1024;
            else if (sscanf(line, "MemAvailable: %lu", &val) == 1)
                info->available_physical_memory = val * 1024;
        }
        fclose(f);
    }
    gethostname(info->hostname, sizeof(info->hostname));
    struct passwd* pw = getpwuid(getuid());
    if (pw) strncpy(info->username, pw->pw_name, sizeof(info->username) - 1);
#endif
}

int platform_file_exists(const char* path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) ? 1 : 0;
#else
    struct stat st;
    return (stat(path, &st) == 0) ? 1 : 0;
#endif
}

int64_t platform_file_size(const char* path) {
#ifdef _WIN32
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    LARGE_INTEGER size;
    GetFileSizeEx(h, &size);
    CloseHandle(h);
    return (int64_t)size.QuadPart;
#else
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (int64_t)st.st_size;
#endif
}

time_t platform_file_mtime(const char* path) {
#ifdef _WIN32
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    FILETIME ft;
    GetFileTime(h, NULL, NULL, &ft);
    CloseHandle(h);
    uint64_t ns = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    return (time_t)(ns / 10000000ULL - 11644473600ULL);
#else
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return st.st_mtime;
#endif
}

int platform_file_read(const char* path, char** out_data, size_t* out_size) {
    if (!path || !out_data || !out_size) return -1;
    *out_data = NULL;
    *out_size = 0;

    FILE* f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz < 0) { fclose(f); return -1; }

    char* data = malloc(sz + 1);
    if (!data) { fclose(f); return -1; }

    size_t rd = fread(data, 1, sz, f);
    fclose(f);
    data[rd] = '\0';

    *out_data = data;
    *out_size = rd;
    return 0;
}

int platform_file_write(const char* path, const char* data, size_t size) {
    if (!path || !data) return -1;
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    size_t wr = fwrite(data, 1, size, f);
    fclose(f);
    return (wr == size) ? 0 : -1;
}

int platform_mkdir(const char* path) {
#ifdef _WIN32
    return _mkdir(path);
#else
    return mkdir(path, 0755);
#endif
}

int platform_is_dir(const char* path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) return 0;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
#else
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode) ? 1 : 0;
#endif
}

char** platform_list_dir(const char* path, int* count) {
    *count = 0;
#ifdef _WIN32
    char search[4096];
    snprintf(search, sizeof(search), "%s\\*", path);
    WIN32_FIND_DATAA fdata;
    HANDLE h = FindFirstFileA(search, &fdata);
    if (h == INVALID_HANDLE_VALUE) return NULL;
    int cap = 16;
    char** list = malloc(cap * sizeof(char*));
    int n = 0;
    do {
        if (strcmp(fdata.cFileName, ".") == 0 || strcmp(fdata.cFileName, "..") == 0)
            continue;
        if (n >= cap) { cap *= 2; list = realloc(list, cap * sizeof(char*)); }
        list[n++] = _strdup(fdata.cFileName);
    } while (FindNextFileA(h, &fdata));
    FindClose(h);
    *count = n;
    return list;
#else
    DIR* d = opendir(path);
    if (!d) return NULL;
    int cap = 16;
    char** list = malloc(cap * sizeof(char*));
    int n = 0;
    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        if (n >= cap) { cap *= 2; list = realloc(list, cap * sizeof(char*)); }
        list[n++] = strdup(ent->d_name);
    }
    closedir(d);
    *count = n;
    return list;
#endif
}

void platform_free_dir_listing(char** list, int count) {
    if (!list) return;
    for (int i = 0; i < count; i++) {
        free(list[i]);
    }
    free(list);
}

char* platform_resolve_path(const char* path) {
#ifdef _WIN32
    char* resolved = malloc(4096);
    if (!resolved) return NULL;
    DWORD len = GetFullPathNameA(path, 4096, resolved, NULL);
    if (len == 0 || len > 4096) { free(resolved); return NULL; }
    return resolved;
#else
    return realpath(path, NULL);
#endif
}

char* platform_home_dir(void) {
#ifdef _WIN32
    const char* userprofile = getenv("USERPROFILE");
    if (userprofile) return strdup(userprofile);
    const char* homedrive = getenv("HOMEDRIVE");
    const char* homepath = getenv("HOMEPATH");
    if (homedrive && homepath) {
        char* result = malloc(strlen(homedrive) + strlen(homepath) + 2);
        sprintf(result, "%s%s", homedrive, homepath);
        return result;
    }
    return NULL;
#else
    const char* home = getenv("HOME");
    if (home) return strdup(home);
    struct passwd* pw = getpwuid(getuid());
    if (pw) return strdup(pw->pw_dir);
    return NULL;
#endif
}

char* platform_config_dir(void) {
#ifdef _WIN32
    const char* appdata = getenv("APPDATA");
    if (appdata) {
        char* result = malloc(strlen(appdata) + strlen("\\qicto") + 1);
        sprintf(result, "%s\\qicto", appdata);
        return result;
    }
    return strdup(".");
#else
    const char* xdg = getenv("XDG_CONFIG_HOME");
    if (xdg) {
        char* result = malloc(strlen(xdg) + strlen("/qicto") + 1);
        sprintf(result, "%s/qicto", xdg);
        return result;
    }
    char* home = platform_home_dir();
    if (home) {
        char* result = malloc(strlen(home) + strlen("/.config/qicto") + 1);
        sprintf(result, "%s/.config/qicto", home);
        free(home);
        return result;
    }
    return strdup(".qicto");
#endif
}

int platform_dl_open(const char* path) {
#ifdef _WIN32
    HMODULE h = LoadLibraryA(path);
    return (int)(intptr_t)h;
#else
    return (int)(intptr_t)dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

void platform_dl_close(int handle) {
#ifdef _WIN32
    if (handle) FreeLibrary((HMODULE)(intptr_t)handle);
#else
    if (handle) dlclose((void*)(intptr_t)handle);
#endif
}

void* platform_dl_sym(int handle, const char* name) {
#ifdef _WIN32
    if (!handle) return NULL;
    return (void*)GetProcAddress((HMODULE)(intptr_t)handle, name);
#else
    if (!handle) return NULL;
    return dlsym((void*)(intptr_t)handle, name);
#endif
}

char* platform_dl_error(void) {
#ifdef _WIN32
    static char err[256];
    DWORD code = GetLastError();
    if (code == 0) return NULL;
    snprintf(err, sizeof(err), "error %lu", code);
    return err;
#else
    return dlerror();
#endif
}

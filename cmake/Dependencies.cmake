include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# uthash for, hash tables, headers etc...
FetchContent_Declare(
    uthash
    GIT_REPOSITORY https://github.com/troydhanson/uthash.git
    GIT_TAG        v2.3.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(uthash)

# cwalk, for cross compabilities
FetchContent_Declare(
    cwalk
    GIT_REPOSITORY https://github.com/likle/cwalk.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(cwalk)

# inih for ini parsing
FetchContent_Declare(
    inih
    GIT_REPOSITORY https://github.com/benhoyt/inih.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
set(INIH_BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
set(INIH_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Populate(inih)
add_library(inih STATIC ${inih_SOURCE_DIR}/ini.c)
target_include_directories(inih PUBLIC ${inih_SOURCE_DIR})

# cargs for CLI arguments managment
FetchContent_Declare(
    cargs
    GIT_REPOSITORY https://github.com/likle/cargs.git
    GIT_TAG        v1.0.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(cargs)

# utf8proc for unicodes and utf8 encodings...
FetchContent_Declare(
    utf8proc
    GIT_REPOSITORY https://github.com/JuliaStrings/utf8proc.git
    GIT_TAG        v2.11.0
    GIT_SHALLOW    TRUE
)
set(UTF8PROC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(UTF8PROC_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(utf8proc)
# pcre2, the regex engine
FetchContent_Declare(
    pcre2
    GIT_REPOSITORY https://github.com/PCRE2Project/pcre2.git
    GIT_TAG        pcre2-10.43
    GIT_SHALLOW    TRUE
)
set(PCRE2_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PCRE2_SUPPORT_LIBREADLINE OFF CACHE BOOL "" FORCE)
set(PCRE2_SUPPORT_LIBZ OFF CACHE BOOL "" FORCE)
set(PCRE2_SUPPORT_BZIP2 OFF CACHE BOOL "" FORCE)
set(PCRE2_SUPPORT_LIBLZMA OFF CACHE BOOL "" FORCE)
set(PCRE2_BUILD_PCRE2GREP OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(pcre2)

# tree-sitter's main engine
FetchContent_Declare(
    tree_sitter
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
    GIT_TAG        v0.24.0
    GIT_SHALLOW    TRUE
)
set(TREE_SITTER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tree_sitter)

# tree-sitter for c
FetchContent_Declare(
    tree_sitter_c
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-c.git
    GIT_TAG        v0.21.0
    GIT_SHALLOW    TRUE
)
FetchContent_Populate(tree_sitter_c)

# tree-sitter for c++
FetchContent_Declare(
    tree_sitter_cpp
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-cpp.git
    GIT_TAG        v0.22.0
    GIT_SHALLOW    TRUE
)
FetchContent_Populate(tree_sitter_cpp)

# for config files json 
FetchContent_Declare(
    json_c
    GIT_REPOSITORY https://github.com/json-c/json-c.git
    GIT_TAG        json-c-0.17-20230812
    GIT_SHALLOW    TRUE
)
set(JSON_C_SHARED OFF CACHE BOOL "" FORCE)
set(JSON_C_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(JSON_C_WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JSON_C_WITH_TOOLS OFF CACHE BOOL "" FORCE)
set(JSON_C_VISIBILITY hidden)
FetchContent_MakeAvailable(json_c)
if(TARGET json-c_static AND NOT TARGET json-c)
    add_library(json-c ALIAS json-c_static)
endif()

# libuv for file managment, and fs control
FetchContent_Declare(
    libuv
    GIT_REPOSITORY https://github.com/libuv/libuv.git
    GIT_TAG        v1.48.0
    GIT_SHALLOW    TRUE
)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(LIBUV_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LIBUV_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(LIBUV_BUILD_FUZZERS OFF CACHE BOOL "" FORCE)
set(LIBUV_INSTALL OFF CACHE BOOL "" FORCE)
set(LIBUV_MAINTAINER_MODE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libuv)

# the... notcurses for TUI rendering
FetchContent_Declare(
    notcurses
    GIT_REPOSITORY https://github.com/dankamongmen/notcurses.git
    GIT_TAG        v3.0.10
    GIT_SHALLOW    TRUE
)
set(NOTCURSES_PREFER_NCURSESW OFF CACHE BOOL "" FORCE)
set(NOTCURSES_USE_DOXYGEN OFF CACHE BOOL "" FORCE)
set(NOTCURSES_INSTALL OFF CACHE BOOL "" FORCE)
set(USE_MULTIMEDIA "none" CACHE STRING "" FORCE)
set(USE_DEFLATE OFF CACHE BOOL "" FORCE)
set(USE_PANDOC OFF CACHE BOOL "" FORCE)
set(NOTCURSES_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NOTCURSES_USE_DOXYGEN OFF CACHE BOOL "" FORCE)
set(USE_POC OFF CACHE BOOL "" FORCE)
set(BUILD_EXECUTABLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(notcurses)
set_target_properties(notcurses-core PROPERTIES C_EXTENSIONS ON)
set_target_properties(notcurses-core-static PROPERTIES C_EXTENSIONS ON)
if(WIN32)
    target_include_directories(notcurses-core PUBLIC /usr/include)
    target_include_directories(notcurses-core-static PUBLIC /usr/include)
endif()
# Make notcurses include dirs PUBLIC so consumers can find notcurses/notcurses.h
get_target_property(NC_INC_DIR notcurses-core INTERFACE_INCLUDE_DIRECTORIES)
if(NOT NC_INC_DIR)
    set_target_properties(notcurses-core PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${notcurses_SOURCE_DIR}/include;${notcurses_BINARY_DIR}/include;")
    set_target_properties(notcurses-core-static PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${notcurses_SOURCE_DIR}/include;${notcurses_BINARY_DIR}/include;")
endif()
if(TARGET notcurses_static AND NOT TARGET notcurses)
    add_library(notcurses ALIAS notcurses_static)
endif()

if(MINGW)
    set(NOTCURSES_COMPAT "${notcurses_SOURCE_DIR}/src/compat/compat.h")
    if(EXISTS "${NOTCURSES_COMPAT}")
        execute_process(
            COMMAND powershell -Command "
                $p = '${NOTCURSES_COMPAT}';
                if (-not (Select-String -Path $p -Pattern 'struct termios' -Quiet)) {
                    (Get-Content $p) -replace '#ifdef  __MINGW32__', '#ifdef  __MINGW32__\nstruct termios { unsigned int c_iflag; unsigned int c_oflag; unsigned int c_cflag; unsigned int c_lflag; unsigned char c_cc[32]; unsigned int c_ispeed; unsigned int c_ospeed; };' | Set-Content $p
                }
            "
            RESULT_VARIABLE _patch_rc
        )
    endif()
endif()

# the git integration for qicto
FetchContent_Declare(
    libgit2
    GIT_REPOSITORY https://github.com/libgit2/libgit2.git
    GIT_TAG        v1.8.1
    GIT_SHALLOW    TRUE
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CURL_ENABLED OFF CACHE BOOL "" FORCE)
set(GIT_THREADSAFE_ASSERTS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(LIBGIT2_DOCS OFF CACHE BOOL "" FORCE)
set(LIBGIT2_USE_SSH OFF CACHE BOOL "" FORCE)
set(LIBGIT2_USE_HTTPS OFF CACHE BOOL "" FORCE)
set(LIBGIT2_USE_OPENSSL OFF CACHE BOOL "" FORCE)
set(LIBGIT2_USE_WINHTTP ON CACHE BOOL "" FORCE)
set(LIBGIT2_BUILD_LIBSSH2_TEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libgit2)
if(TARGET libgit2 AND NOT TARGET git2)
    add_library(git2 ALIAS libgit2)
endif()

# tree-sitter's grammar static libs 
enable_language(C CXX)

add_library(tree_sitter_c STATIC
    ${tree_sitter_c_SOURCE_DIR}/src/parser.c
)
target_include_directories(tree_sitter_c PUBLIC
    ${tree_sitter_SOURCE_DIR}/include
)
if(MSVC)
    target_compile_options(tree_sitter_c PRIVATE /wd4996)
endif()

add_library(tree_sitter_cpp STATIC
    ${tree_sitter_cpp_SOURCE_DIR}/src/parser.c
    ${tree_sitter_cpp_SOURCE_DIR}/src/scanner.c
)
target_include_directories(tree_sitter_cpp PUBLIC
    ${tree_sitter_SOURCE_DIR}/include
)
if(MSVC)
    target_compile_options(tree_sitter_cpp PRIVATE /wd4996)
endif()

QICTO PROJECT — AGENT REFERENCE DOCUMENT
==========================================

PROJECT IDENTITY
----------------
Name:        QICTO
Version:     0.1.0
Description: A fast, modular TUI text editor written in C11 with C++ extensions
License:     Apache License 2.0
Author:      Seigh-sword
Year:        2026
Homepage:    https://github.com/Seigh-sword/qicto.git
Working Dir: C:\Users\abhij\projects\qicto

CORE PHILOSOPHY
---------------
QICTO is designed as a vim-inspired, modal TUI editor with a heavy emphasis on modularity.
Every feature beyond core text editing lives in a module. The editor core does NOT contain
feature-specific logic — it delegates to the module registry.

Key design principles:
- Modal editing (NORMAL, INSERT, VISUAL, SELECT, COMMAND, SEARCH)
- Buffer-centric architecture (multiple files open simultaneously as buffers)
- Module-driven extensibility (builtin + dynamically loaded modules)
- TUI rendering via notcurses (NOT ncurses, NOT curses)
- Cross-platform: Windows (MSYS2 ucrt64 / MinGW), Linux, macOS
- Static linking preferred for deployment

DIRECTORY STRUCTURE
-------------------
qicto/
├── .gitignore
├── .kilo/
│   └── .gitignore
├── AGENT.md                    ← THIS FILE
├── LICENSE
├── CMakeLists.txt              ← Root build file (128 lines)
├── README.md
├── cmake/
│   └── Dependencies.cmake      ← All FetchContent deps, 212 lines
├── src/
│   ├── main.c                  ← Entry point, CLI parsing via cargs
│   ├── qicto.h                 ← Master header (types, enums, structs)
│   ├── core/
│   │   ├── buffer.c / .h       ← Text buffer management, UTF-8 aware
│   │   ├── editor.c / .h       ← Editor state, mode management
│   │   ├── command.c / .h      ← Command registry & builtin commands
│   │   ├── config.c / .h       ← INI config loading/saving
│   │   ├── module.c / .h       ← Module system (load/unload/registry)
│   │   └── mod_load.c          ← Dynamic module loading implementation
│   ├── ui/
│   │   ├── tui.c / .h          ← notcurses init/deinit/render loop
│   │   ├── renderer.c / .h     ← ncplane drawing primitives
│   │   ├── input.c / .h        ← Keyboard input mapping (notcurses → qicto keys)
│   │   └── layout.c / .h       ← Window layout engine
│   ├── utils/
│   │   ├── strings.c / .h      ← String utilities, UTF-8 display width
│   │   ├── fileops.c / .h      ← File I/O wrappers
│   │   └── dynarray.c / .h     ← Dynamic array implementation
│   ├── platform/
│   │   ├── platform.c / .h     ← OS abstraction layer (Win/Linux/macOS)
│   │   └── platform_windows.c  ← Windows-specific platform code
│   └── modules/builtin/
│       ├── mod_builtins.c / .h ← Builtin module registration
│       ├── syntax_mod.c / .h   ← Keyword-based syntax highlighting
│       ├── statusbar_mod.c / .h← Status bar module
│       └── filetree_mod.c / .h ← File tree sidebar module
├── tests/
│   ├── CMakeLists.txt
│   ├── test.h                  ← Test framework macros
│   ├── test_buffer.c
│   ├── test_editor.c
│   └── test_commands.c
└── build/                      ← CMake build directory (gitignored)
    ├── bin/                    ← Executables output
    ├── _deps/                  ← FetchContent downloaded sources
    └── ...

SOURCE FILE INVENTORY (src/)
-----------------------------
Core:
  buffer.c      Text buffer with linked list, cursor, selection, render lines
  editor.c      Editor state machine, mode switching, buffer list mgmt
  command.c     Command registry, builtin commands (quit, write, edit, help, etc.)
  config.c      INI parsing via inih, default config loading
  module.c      Module registry, builtin module registration
  mod_load.c    dlopen/dlsym wrapper for dynamic modules

UI:
  tui.c         notcurses lifecycle: init/deinit/refresh/render loop
  renderer.c    Drawing: statusbar, cmdline, mode indicator, buffer text
  input.c       ncinput → qicto key mapping, insert/normal/visual/command handlers
  layout.c      Window splitting and layout calculations

Utils:
  strings.c     String dup, concat, trim, split, display width (utf8proc)
  fileops.c     File read/write/mkdir/list via platform abstraction
  dynarray.c    Generic dynamic array (void* based)

Platform:
  platform.c    Cross-platform API (file ops, dlopen, dir listing, sysinfo)
  platform_windows.c Windows implementations

Modules:
  syntax_mod.c  Simple keyword-based syntax highlighting (C/C++/etc.)
  statusbar_mod.c Status bar rendering via module callback
  filetree_mod.c File tree sidebar with cwalk path joining

Tests:
  test_buffer.c    Buffer unit tests
  test_editor.c    Editor unit tests
  test_commands.c  Command system tests

BUILD SYSTEM
------------
Build Tool:   CMake 3.21+ with Ninja (default on Windows MSYS2)
Compiler:     GCC 16.2.0 (x86_64-w64-mingw32) on Windows
Standard:     C11 (CMAKE_C_STANDARD 11, CMAKE_C_EXTENSIONS OFF)
Languages:    C + C++ (tree-sitter-cpp requires C++)

CMake Targets:
  qicto_core    STATIC library containing all editor logic
  qicto         EXECUTABLE (links qicto_core)
  test_buffer   EXECUTABLE (unit test)
  test_editor   EXECUTABLE (unit test)
  test_commands EXECUTABLE (unit test)

CMake Options:
  QICTO_BUILD_TESTS       ON  — Build unit tests
  QICTO_BUILD_MODS_DIR    ON  — External module loading support
  QICTO_ENABLE_ASAN       OFF — AddressSanitizer

Build Commands:
  Configure:  cmake -B build -G Ninja
  Build:      cmake --build build
  Build all:  cmake --build build --target qicto test_buffer test_editor test_commands
  Clean:      cmake --build build --target clean
  Reconfigure: rm -rf build && cmake -B build -G Ninja

DEPENDENCIES (via FetchContent in cmake/Dependencies.cmake)
------------------------------------------------------------
All dependencies are fetched automatically by CMake FetchContent.

1. uthash        v2.3.0     https://github.com/troydhanson/uthash.git
                 Header-only hash table library. No link target created.
                 Used in: module.c (hash tables for mod/command registries)

2. cwalk         master     https://github.com/likle/cwalk.git
                 Cross-platform path manipulation library.
                 Target: cwalk (static)

3. inih          master     https://github.com/benhoyt/inih.git
                 INI file parser. Uses Meson natively — manually built via add_library.
                 Target: inih (static, built from ini.c)
                 Include: <ini.h>

4. cargs         v1.0.3     https://github.com/likle/cargs.git
                 CLI argument parser. API: cag_option, cag_option_prepare, cag_option_fetch.
                 Target: cargs (static)
                 Header: <cargs.h>

5. utf8proc      v2.11.0    https://github.com/JuliaStrings/utf8proc.git
                 Unicode/UTF-8 processing library.
                 Target: utf8proc (static)
                 Key API: utf8proc_map(), utf8proc_encode_char(), utf8proc_charwidth()
                 Types: utf8proc_int32_t, utf8proc_uint8_t, utf8proc_ssize_t

6. pcre2         pcre2-10.43 https://github.com/PCRE2Project/pcre2.git
                 Regex engine. 8-bit library only.
                 Target: pcre2-8 (static)
                 Tests/Bench/Fuzz/Grep all disabled.

7. tree-sitter   v0.24.0    https://github.com/tree-sitter/tree-sitter.git
                 Incremental parsing engine (header-only in v0.24+).
                 No static lib produced. Includes only.

8. tree-sitter-c v0.21.0    https://github.com/tree-sitter/tree-sitter-c.git
                 C grammar parser for tree-sitter.
                 Target: tree_sitter_c (static, from parser.c)

9. tree-sitter-cpp v0.22.0  https://github.com/tree-sitter/tree-sitter-cpp.git
                 C++ grammar parser for tree-sitter.
                 Target: tree_sitter_cpp (static, from parser.c + scanner.c)

10. json-c       json-c-0.17-20230812 https://github.com/json-c/json-c.git
                  JSON library.
                  Target: json-c (static ALIAS from json-c_static)

11. libuv        v1.48.0    https://github.com/libuv/libuv.git
                  Event loop / async I/O.
                  Target: uv (shared libuv.dll on Windows)

12. notcurses    v3.0.10    https://github.com/dankamongmen/notcurses.git
                  TUI rendering library (successor to ncurses/ncurses).
                  THIS IS THE CORRECT REPO — not "notncurses".
                  Targets: notcurses-core-static (static), notcurses (shared DLL)
                  CRITICAL: v3.0.x API is DIFFERENT from older versions.
                  See NOTCURSES V3.0 API CHANGES section below.
                  Multimedia: none (USE_MULTIMEDIA=none)
                  POC programs: disabled (USE_POC=OFF, BUILD_EXECUTABLES=OFF)

13. libgit2      v1.8.1     https://github.com/libgit2/libgit2.git
                  Git library.
                  Target: git2 (ALIAS from libgit2)
                  Windows: HTTPS via WinHTTP (LIBGIT2_USE_WINHTTP=ON)
                  Tests: disabled (BUILD_TESTS=OFF, BUILD_TESTING=OFF)

DEPENDENCY URL CORRECTIONS (LESSONS LEARNED)
---------------------------------------------
The following wrong URLs were fixed during initial setup:
  troydh/uthash          → troydhanson/uthash
  brinkqiang/inih        → benhoyt/inih
  dankamongdrones/notcurses → dankamongmen/notcurses
  cargs/cargs            → likle/cargs
  utf8proc v2.9.0        → v2.11.0
  cwalk GIT_TAG main     → GIT_TAG master (cwalk uses "master" branch)

NOTCURSES V3.0 API CHANGES (CRITICAL)
--------------------------------------
notcurses v3.0.10 has a DIFFERENT API than v2.x. The following mappings are required:

OLD API (v2.x)                    → NEW API (v3.0.x)
---------------------------------------------------------------------------
notcurses_get_stdplane(nc)        → notcurses_stdplane(nc)
ncplane_cursor_move(n, y, x)      → ncplane_cursor_move_yx(n, y, x)
ncplane_putc(n, c)                → ncplane_putchar(n, c)    [takes char, not nccell*]
ncplane_set_styles(n, a, b, ...)  → ncplane_set_styles(n, stylebits) [single bitmask]
ncplane_set_fg_rgb(n, r,g,b)      → ncplane_set_fg_rgb8(n, r, g, b)
ncplane_set_bg_rgb(n, r,g,b)      → ncplane_set_bg_rgb8(n, r, g, b)

Style constants:
  NCSTYLE_NONE      → 0
  NCSTYLE_BOLD      → 0x0002u
  NCSTYLE_UNDERLINE → 0x0008u
  NCSTYLE_ITALIC    → 0x0010u
  NCSTYLE_STRUCK    → 0x0001u
  NCSTYLE_UNDERCURL → 0x0004u
  NCSTYLE_MASK      → 0xffffu

notcurses_options struct:
  opts.flags        → NCOPTION_* bitmask (NCOPTION_NO_QUIT_SIGHANDLERS, etc.)
  opts.log_level    → opts.loglevel (ncloglevel_e enum)
  NCPREVENT_*       → REMOVED. Use NCOPTION_* flags instead.

Key constants:
  NCKEY_PGDN        → NCKEY_PGDOWN
  NCKEY_F1          → NCKEY_F01 (all F-keys are zero-padded: F01-F12)
  NCKEY_F2          → NCKEY_F02, etc.

UTF8PROC V2.11.0 API
---------------------
The v2.11.0 API uses these exact signatures:
  utf8proc_map(const utf8proc_uint8_t *str, utf8proc_ssize_t strlen,
               utf8proc_uint8_t **dstptr, utf8proc_option_t options)
               → returns utf8proc_ssize_t (not int32_t len param)

  utf8proc_encode_char(utf8proc_int32_t codepoint, utf8proc_uint8_t *dst)
               → returns utf8proc_ssize_t

  utf8proc_charwidth(utf8proc_int32_t cp) → returns int

Types:
  utf8proc_int32_t  → int32_t
  utf8proc_uint8_t  → uint8_t (unsigned char)
  utf8proc_ssize_t  → ssize_t

MODULE SYSTEM ARCHITECTURE
---------------------------
Modules are the primary extension mechanism. There are two types:

1. BUILTIN modules — compiled into qicto_core, registered in mod_builtins_register()
   - syntax_mod   — keyword-based syntax highlighting
   - statusbar_mod — status bar rendering callback
   - filetree_mod  — file tree sidebar

2. DYNAMIC modules — loaded at runtime from shared libraries (.so/.dll)
   - Must export: qicto_mod_api_t* mod_get_api(void)
   - Loaded from: ed->config->mods_dir (default: platform_config_dir()/mods)

Module API (qicto_mod_api_t):
  .name, .version, .description
  .init(editor_t*) → qicto_cmd_result_t
  .cleanup(editor_t*)
  .on_render(editor_t*, void* ncp) — called after main render
  .on_key(editor_t*, qkey_t) → qkey_t (return 0 to consume key)
  .on_buffer_opened(editor_t*, buffer_t*)
  .on_buffer_changed(editor_t*, buffer_t*)
  .on_mode_change(editor_t*, qicto_mode_t from, qicto_mode_t to)
  .on_config_reload(editor_t*, config_t*)
  .on_command(editor_t*, const char* cmd, char** out) → qicto_cmd_result_t

MODULE CALLBACK CONTRACT
------------------------
- .init must return QICTO_CMD_SUCCESS
- .on_render receives the ncplane pointer as void* ncp — cast to struct ncplane*
- .on_key returning 0 means the key was consumed; returning the key passes it through
- Module state should be stored in a static/global struct inside the module .c file
- Never free editor-owned memory in cleanup unless you allocated it

COMMAND SYSTEM
--------------
Commands are registered via commands_register(cmds, name, fn, help).
Builtin commands: quit, q, write, w, edit, e, buffer, ls, help, version, lsmods
Command format: "name args" — args are everything after the first space.
Execute from command mode by typing ":commandname args" or "/searchterm".

CONFIG SYSTEM
-------------
Config files are INI format parsed by inih.
Default path: platform_config_dir()/qicto.ini
Load order:
  1. Builtin defaults (config_load_builtin_defaults)
  2. User config file (if exists)
Config keys are simple string key/value pairs.
Special keys: mods_dir, project_dir, tab_width, expand_tabs, show_line_numbers, etc.

EDITOR STATE MACHINE
--------------------
Modes: NORMAL → INSERT → NORMAL (via Esc)
                  → VISUAL (via v)
                  → SELECT  (via V)
                  → COMMAND (via :)
                  → SEARCH  (via /)

Key bindings (NORMAL mode):
  h/j/k/l  — move cursor
  i        — enter INSERT mode
  a        — append after cursor (INSERT mode)
  o/O      — open line below/above (INSERT mode)
  x/X      — delete character
  v/V      — enter VISUAL/SELECT mode
  :        — enter COMMAND mode
  /        — enter SEARCH mode
  w        — save buffer
  q        — quit (if clean) or warn
  b/B      — next/prev buffer

RENDER PIPELINE
---------------
1. tui_clear() — erase the standard plane
2. For each buffer line:
   - Set foreground color (ncplane_set_fg_rgb8)
   - Move cursor (ncplane_cursor_move_yx)
   - Print text (ncplane_putstr)
3. Render statusbar module callback
4. Render cmdline
5. Position cursor at current buffer position
6. notcurses_render()

BUFFER INTERNALS
----------------
- buffer_t is a doubly-linked list node (next/prev)
- text is stored as char** lines (qicto_text_lines_t)
- Cursor: cursor_line (size_t), cursor_col (size_t), cursor_byte (size_t)
- Selection: sel_anchor_*, sel_cursor_*, has_selection
- Render: render_lines is an array of qicto_render_line_t (cells with cp, syntax_group, style_mask)
- render_valid flag — set false when buffer changes, true after buffer_update_render()
- dirty flag — set true when buffer is modified

PLATFORM ABSTRACTION
--------------------
platform.h defines a clean OS abstraction layer.
Windows: platform.c + platform_windows.c
Linux:   platform.c + platform_linux.c (if exists)
macOS:   platform.c + platform_macos.c (if exists)

Functions:
  platform_init/deinit
  platform_file_exists/size/mtime/read/write
  platform_mkdir
  platform_is_dir
  platform_list_dir / platform_free_dir_listing
  platform_resolve_path / platform_home_dir / platform_config_dir
  platform_dl_open/close/sym/error

CURRENT BUILD STATUS (as of latest session)
-------------------------------------------
The project compiles successfully on Windows MSYS2 ucrt64 with GCC 16.2.0.

Previously fixed issues:
  ✓ Dependency URLs corrected in cmake/Dependencies.cmake
  ✓ inih manually built via add_library (Meson-native project)
  ✓ notcurses v3.0.x API migration (cursor_move → cursor_move_yx, putc → putchar, etc.)
  ✓ NCKEY_F1-F12 renamed to NCKEY_F01-NCKEY_F12 in input.c
  ✓ utf8proc v2.11.0 API fixed (utf8proc_map signature, types)
  ✓ strndep shim added for Windows (qicto_strndup in buffer.c)
  ✓ cwk_path_join added include for cwalk.h in filetree_mod.c
  ✓ Module init return types fixed (void → qicto_cmd_result_t)
  ✓ cargs v1.0.3 API migrated to cag_option API
  ✓ libgit2 tests disabled (BUILD_TESTS=OFF, not LIBGIT2_TESTS)
  ✓ notcurses POC programs disabled (USE_POC=OFF, BUILD_EXECUTABLES=OFF)
  ✓ notcurses include directories propagated to qicto executable target
  ✓ tree_sitter removed from direct link (header-only, propagated via tree_sitter_c/cpp)
  ✓ uthash removed from link (header-only)
  ✓ libgit2 alias git2 created for CMake target

Build output:
  bin/qicto.exe  — main executable
  libqicto_core.a — static library

STYLE CONVENTIONS
-----------------
- C11 standard, no compiler extensions (CMAKE_C_EXTENSIONS OFF)
- 4-space indentation (no tabs)
- snake_case for functions and variables
- PascalCase for types (buffer_t, editor_t, qicto_mode_t)
- Prefix convention: qicto_ for public API, internal functions static
- Header guards: QICTO_<FILENAME>_H
- All public types in qicto.h
- Module files: <name>_mod.c / <name>_mod.h
- No trailing whitespace
- No emojis in code or documentation unless explicitly requested
- Comments in code are minimal; prefer self-documenting names

NAMING CONVENTIONS
------------------
Files:
  core/*.c/h     — Core editor logic
  ui/*.c/h       — TUI rendering and input
  utils/*.c/h    — Utility functions
  platform/*.c/h — OS abstraction
  modules/*/     — Builtin modules

Functions:
  qicto_*        — Public API functions
  buffer_*       — Buffer operations
  editor_*       — Editor operations
  input_*        — Input handling
  renderer_*     — Rendering operations
  platform_*     — Platform abstraction
  mod_*          — Module system
  cmd_*          — Builtin commands
  config_*       — Configuration
  qstr_*         — String utilities
  qicto_strndup  — Internal shims (qicto_ prefix to avoid conflicts)

Types:
  qicto_*_t      — Enums and typedef structs
  buffer_t, editor_t, config_t, etc.

GIT WORKFLOW
------------
- Main branch: master
- No commit hooks configured
- Build artifacts are gitignored
- Tests are run via CTest (ctest --output-on-failure)

KNOWN ISSUES / TODOS
--------------------
1. tree_sitter target is header-only (v0.24+), but qicto_core links it transitively
   through tree_sitter_c/cpp. The linker tries to find -ltree_sitter on Windows.
   Workaround: tree_sitter_c and tree_sitter_cpp already have PUBLIC link to tree_sitter,
   but the qicto_core target still propagates it. May need to use PRIVATE for tree_sitter
   in those targets, or ensure tree_sitter creates a proper INTERFACE target.

2. libgit2 build is large and slow. Consider pre-built binaries or system package.

3. notcurses on Windows requires termios.h from /usr/include (MSYS2 compat shim).
   The struct termios definition is patched in the build directory compat.h.

4. Tests are minimal. test_buffer, test_editor, test_commands exist but coverage is low.

5. UTF-8 grapheme cluster support is incomplete (buffer.c has a custom function
   that conflicts with utf8proc library).

6. The editor is currently single-window. Multi-window splits (layout.c) are stubbed
   but not fully integrated with the render loop.

7. Syntax highlighting is keyword-based only, not full tree-sitter integration.
   The tree-sitter grammars are compiled but not used in rendering yet.

8. Dynamic module loading is implemented but only used for builtins currently.
   External module directory scanning is in mod_registry_load_dir().

HOW TO ADD A NEW BUILTIN MODULE
--------------------------------
1. Create src/modules/builtin/my_mod.c and src/modules/builtin/my_mod.h
2. Implement the qicto_mod_api_t interface:
   static qicto_mod_api_t s_my_api = {
       .name = "my_mod",
       .version = "0.1.0",
       .description = "Description",
       .init = my_init,          // MUST return qicto_cmd_result_t
       .cleanup = my_cleanup,
       .on_render = my_on_render, // receives ncplane* as void*
       .on_key = my_on_key,       // return 0 to consume key
       ...
   };
3. In mod_builtins.c, add: #include "my_mod.h"
4. In mod_builtins_register(), add: register_builtin(ed, my_mod_get_api());
5. Add src/modules/builtin/my_mod.c to QICTO_CORE_SOURCES in CMakeLists.txt

HOW TO ADD A NEW COMMAND
------------------------
1. In src/core/command.c, implement: qicto_cmd_result_t cmd_mycommand(editor_t* ed, const char* args, char** out)
2. In editor_create() (editor.c), register it:
   commands_register(ed->commands, "mycmd", cmd_mycommand, "My command help text");
3. Users invoke it as :mycmd args in COMMAND mode.

WINDOWS-SPECIFIC NOTES
-----------------------
- Build requires MSYS2 ucrt64 environment
- pacman packages needed: mingw-w64-ucrt-x86_64-libunistring
- termios.h is provided by MSYS2 ncurses (C:\msys64\ucrt64\include)
- struct termios is missing on MinGW; compat.h is patched in build/_deps/notcurses-build/src/compat/
- C_EXTENSIONS must be ON for notcurses-core and notcurses-core-static targets
- Link libraries: iphlpapi, ws2_32, secur32, user32, advapi32, etc.
- Dynamic modules use .dll extension
- strndup is not available; qicto_strndup shim is used in buffer.c

LINUX-SPECIFIC NOTES
--------------------
- Requires: libnotcurses-dev, libutf8proc-dev, libpcre2-dev, etc. OR use CMake FetchContent
- Link: dl
- Build: cmake -B build && cmake --build build
- Run: ./build/bin/qicto

MACOS-SPECIFIC NOTES
--------------------
- Requires Homebrew: brew install notcurses utf8proc pcre2 libuv json-c cwalk inih
- Or use FetchContent (same as Linux)
- Link: dl
- Build: cmake -B build && cmake --build build

TESTING
-------
Test framework: Custom lightweight framework in tests/test.h
Tests use ASSERT, ASSERT_EQ, ASSERT_STR_EQ macros.
Run all tests: ctest --output-on-failure
Run specific test: ctest -R test_buffer --output-on-failure

CI/CD EXPECTATIONS
------------------
- Build should succeed with -Werror on Linux/macOS
- Windows builds use MSYS2 ucrt64 with GCC or Clang
- No external dependencies at runtime if statically linked
- Binary distribution: single qicto.exe with bundled DLLs

SECURITY & SAFETY
-----------------
- Never commit secrets, API keys, or credentials
- Never log sensitive data (passwords, tokens)
- All file paths should be validated before use
- Buffer overflows: use QICTO_MAX_* constants for all buffers
- UTF-8 input must be validated before processing
- Module loading should validate the ELF/PE magic before dlopen

DEBUGGING
---------
- Enable ASAN: cmake -B build -DQICTO_ENABLE_ASAN=ON
- Debug build: cmake -B build -DCMAKE_BUILD_TYPE=Debug
- Print notcurses errors: set NCPREVENT_NONE or use notcurses_options.loglevel
- Enable libgit2 debug: GIT_TRACE=1, GIT_TRACE_PERFORMANCE=1
- Tree-sitter debug: TS_LOG_LEVEL=debug

CONTRIBUTING WORKFLOW
---------------------
1. Read this AGENT.md fully before touching any code
2. Follow the existing code style exactly (snake_case, 4 spaces, no extensions)
3. All new public types go in src/qicto.h
4. All new module callbacks must return qicto_cmd_result_t for .init
5. notcurses v3.0 API must be used (see API mapping table above)
6. utf8proc v2.11.0 types must be used exactly as defined
7. Run cmake --build build and fix ALL warnings before considering done
8. Run ctest --output-on-failure and ensure all tests pass
9. Update this AGENT.md if you add new patterns, APIs, or conventions
10. Never add comments to code unless explicitly requested by the user

IMPORTANT REMINDERS FOR AI AGENTS
----------------------------------
- This project is C11 with no compiler extensions
- CMAKE_C_EXTENSIONS is OFF — do not enable it
- notcurses v3.0.x is the TUI library — NOT ncurses, NOT notncurses
- The correct repo is https://github.com/dankamongmen/notcurses
- tree-sitter v0.24+ is header-only for the core library
- inih must be manually built (Meson-native, not CMake)
- All module .init functions return qicto_cmd_result_t (NOT void)
- strndup does not exist on Windows MinGW — use qicto_strndup
- cwalk uses cwk_path_get_basename (NOT cwk_path_basename)
- config.c includes <ini.h> (NOT <inih.h>)
- command.h and module.h use named struct tags (NOT anonymous typedefs)

LAST UPDATED
------------
2026-09-05 by Seigh-sword

CHANGELOG
---------
0.1.0 (2026-09-05)
  - Initial project scaffold with CMake + FetchContent build system
  - Core editor architecture: buffer, editor, command, config, module systems
  - TUI rendering via notcurses v3.0.10 with Windows MinGW compat shim
  - Tree-sitter v0.24+ integration (header-only core, C/C++ grammars)
  - libgit2 v1.8.1 integration for Git operations
  - UTF-8 processing via utf8proc v2.11.0
  - INI config parsing via inih (manual CMake target)
  - CLI parsing via cargs v1.0.3 (cag_option API)
  - Cross-platform path handling via cwalk
  - Regex support via pcre2 v10.43
  - JSON support via json-c v0.17
  - Async I/O via libuv v1.48.0
  - Windows MSYS2 ucrt64 / MinGW build support
  - Linux and macOS build support via FetchContent
  - Unit tests: test_buffer, test_editor, test_commands
  - Project documentation: AGENT.md, README.md, LICENSE (Apache 2.0)

ARCHITECTURE DEEP DIVE
----------------------
Buffer System:
  buffer_t uses a doubly-linked list for multi-buffer management.
  Each buffer stores text as char** lines (qicto_text_lines_t).
  The render pipeline uses qicto_render_line_t arrays with utf8proc
  for grapheme cluster awareness. dirty flag tracks modifications.

Editor State:
  editor_t holds mode, cursor, current_buffer, buffer list, config,
  layout, command registry, and module registry. Mode transitions
  are handled in input.c via notcurses key events.

Command System:
  commands_create() allocates a registry with dynamic array growth.
  commands_register() copies name and stores function pointer.
  commands_find() does exact match then prefix match.
  commands_execute() tokenizes input and dispatches.

Module System:
  mod_registry_t tracks loaded modules. Builtins are registered at
  editor_create() time. Dynamic modules are loaded via platform_dl_open()
  and must export qicto_mod_api_t* mod_get_api(void).

Rendering:
  tui.c manages notcurses lifecycle. renderer.c draws each plane.
  notcurses v3.0 uses ncplane_set_fg_rgb8() and ncplane_cursor_move_yx().
  Standard plane is obtained via notcurses_stdplane().

Platform Layer:
  platform.c provides OS abstraction. Windows uses platform_windows.c.
  Dynamic loading uses LoadLibrary/FreeLibrary on Windows,
  dlopen/dlsym on Unix. Path handling uses cwalk for consistency.

TESTING STRATEGY
----------------
- test_buffer: buffer creation, insert, remove, split, join, cursor, load/save, syntax detection
- test_editor: editor lifecycle, buffer management, mode switching
- test_commands: command registration, execution, config load/save
- Tests link against qicto_core static library
- Run via CTest with --output-on-failure for verbose results

BUILD ARTIFACTS
---------------
Release build:
  build/bin/qicto.exe          — Main executable
  build/libqicto_core.a        — Static library
  build/tests/test_*.exe       — Unit test executables
  build/_deps/                 — FetchContent downloaded sources

Debug build:
  cmake -B build -DCMAKE_BUILD_TYPE=Debug
  build/bin/qicto.exe (with debug symbols)

ASAN build:
  cmake -B build -DQICTO_ENABLE_ASAN=ON
  Requires: AddressSanitizer runtime

PERFORMANCE NOTES
-----------------
- Static linking preferred for Windows deployment
- notcurses-core-static provides static TUI library
- libuv is shared (DLL) on Windows due to complex dependencies
- Tree-sitter grammars are compiled into static libs
- utf8proc is static for Unicode processing
- inih is static for INI parsing

DEBUGGING TIPS
--------------
1. If notcurses fails to compile on Windows, check compat.h struct termios shim
2. If tree_sitter linker errors occur, ensure tree_sitter_c/cpp don't link tree_sitter PUBLIC
3. If libgit2 fails, ensure BUILD_TESTS=OFF and LIBGIT2_USE_WINHTTP=ON on Windows
4. If utf8proc types mismatch, use utf8proc_int32_t, utf8proc_uint8_t, utf8proc_ssize_t
5. If cwalk functions missing, check cwk_path_get_basename (not cwk_path_basename)
6. If config.c fails to find ini.h, ensure inih include dir is set correctly

CI/CD CONFIGURATION
-------------------
Recommended GitHub Actions workflow:
  - Build matrix: Windows (MSYS2), Ubuntu, macOS
  - Cache: CMake dependencies in _deps/
  - Steps: configure, build, test, package
  - Artifacts: qicto.exe / qicto binary per platform
  - Do NOT use FETCHCONTENT_FULLY_DISCONNECTED in CI

PACKAGING
---------
Windows:
  - Build with -DCMAKE_BUILD_TYPE=Release
  - Bundle libuv.dll with qicto.exe
  - Use NSIS or Inno Setup for installer

Linux:
  - Build static binary with -DBUILD_SHARED_LIBS=OFF
  - Package as .deb/.rpm or AppImage

macOS:
  - Build static or dynamic
  - Package as .app bundle or Homebrew formula

ROADMAP
-------
- [ ] Multi-window split support (layout.c integration)
- [ ] Full tree-sitter syntax highlighting integration
- [ ] LSP client module for language intelligence
- [ ] Git integration UI (diff, blame, log)
- [ ] File tree with Git status indicators
- [ ] Fuzzy finder for file/buffer switching
- [ ] Macro recording and playback
- [ ] Plugin system with hot-reload
- [ ] Session management and .qicto/project files
- [ ] Terminal multiplexer integration (tmux/screen)

GLOSSARY
--------
  TUI     — Text User Interface (terminal UI)
  ncplane — notcurses plane (drawing surface)
  buffer  — Text buffer with linked list management
  module  — Plugin that extends editor functionality
  qkey_t  — QICTO key type (uint32_t)
  NCOPTION — notcurses option flag
  UTF8PROC — Unicode normalization library
  cwalk   — Cross-platform path library
  cargs   — CLI argument parsing library
  inih    — INI file parsing library
  pcre2   — Perl-compatible regular expressions
  libuv   — Async I/O library
  libgit2 — Git library
  notcurses — TUI rendering library (ncurses successor)
  tree-sitter — Incremental parsing system

REFERENCE — KEY FILES
---------------------
  src/qicto.h           — Master header (239 lines)
  src/engine/buffer.c   — Buffer management (405 lines)
  src/engine/editor.c   — Editor state (estimated 200+ lines)
  src/engine/command.c  — Command system (191 lines)
  src/engine/config.c   — INI config (includes <ini.h>)
  src/ui/tui.c          — notcurses init/render loop
  src/ui/renderer.c     — Drawing primitives
  src/ui/input.c        — Key mapping (NCKEY_F01-NCKEY_F12)
  cmake/Dependencies.cmake — All 13+ dependencies (212 lines)
  CMakeLists.txt        — Root build (128 lines)

REFERENCE — BUILD VARIABLES
---------------------------
  CMAKE_C_STANDARD     11
  CMAKE_C_EXTENSIONS   OFF
  CMAKE_BUILD_TYPE     Release (default)
  QICTO_BUILD_TESTS    ON
  QICTO_BUILD_MODS_DIR ON
  QICTO_ENABLE_ASAN    OFF

REFERENCE — IMPORTANT CMake TARGETS
------------------------------------
  qicto_core            STATIC library (all editor logic)
  qicto                 EXECUTABLE (links qicto_core)
  tree_sitter_c         STATIC (C grammar parser)
  tree_sitter_cpp       STATIC (C++ grammar parser)
  notcurses-core        SHARED (notcurses core)
  notcurses-core-static STATIC (notcurses core)
  notcurses             ALIAS (shared or static)
  git2                  ALIAS for libgit2
  inih                  STATIC (manually built from ini.c)
  utf8proc              STATIC
  pcre2-8               STATIC
  json-c                STATIC (ALIAS from json-c_static)
  uv                    SHARED (libuv.dll on Windows)
  cwalk                 STATIC
  cargs                 STATIC

REFERENCE — COMPILER FLAGS
---------------------------
  C Standard:           -std=c11
  Optimization:         -O3 (Release), -O2 (RelWithDebInfo), -O0 (Debug)
  Warnings:             -Wall -Wextra -Wshadow -Wvla -Wstrict-aliasing=2
  MinGW compat:         -fno-signed-zeros -fno-trapping-math -fassociative-math
  ASAN:                 -fsanitize=address -fno-omit-frame-pointer

REFERENCE — LINKER FLAGS
------------------------
  Windows:              -liphlpapi -lwsock32 -lsecur32 -luser32 -ladvapi32
  Linux:                -ldl -lpthread
  notcurses core:       -ltinfo -lncursesw
  notcurses ffi:        -lstdc++ -lm

KNOWN WORKAROUNDS
-----------------
1. notcurses struct termios on MinGW:
   Patch build/_deps/notcurses-src/src/compat/compat.h to add:
     struct termios { unsigned int c_iflag; ... unsigned int c_ospeed; };
   This is now automated in cmake/Dependencies.cmake for MINGW builds.

2. tree_sitter missing library:
   tree-sitter v0.24+ is header-only. Do NOT link tree_sitter target.
   Use tree_sitter_c and tree_sitter_cpp which embed parser.c.

3. strndep missing on MinGW:
   Use qicto_strndup() shim defined in buffer.c.

4. FETCHCONTENT_FULLY_DISCONNECTED:
   Ensure this is OFF in CMakeLists.txt for CI to fetch dependencies.

MAINTENANCE NOTES
-----------------
- When upgrading notcurses, check compat.h for termios changes
- When upgrading tree-sitter, verify grammar versions match parser.c
- When upgrading libgit2, check BUILD_TESTS variable name (BUILD_TESTS vs BUILD_TESTING)
- When upgrading utf8proc, verify type names (utf8proc_int32_t, etc.)
- When upgrading cargs, check API migration to cag_option if needed
- Always run ctest --output-on-failure after dependency upgrades
- Update this AGENT.md when adding new dependencies or patterns

DETAILED API REFERENCE — BUFFER
--------------------------------
buffer_new(const char* filename)
  Creates a new buffer. If filename is provided, detects syntax from extension.
  Returns buffer_t* or NULL on allocation failure.

buffer_free(buffer_t* buf)
  Frees buffer and all associated memory (lines, render_lines, buffer struct).

buffer_load_file(buffer_t* buf, const char* filename)
  Loads file content into buffer. Replaces existing lines.
  Detects syntax from filename extension. Sets display_name via cwk_path_get_basename.

buffer_save(buffer_t* buf, const char* filename)
  Saves buffer content to file. Writes lines with newline separators.
  Clears dirty flag on success.

buffer_insert_char(buffer_t* buf, size_t line, size_t col, char c)
  Inserts character at line/col. Reallocates line buffer.
  Clamps col to line length. Sets dirty and invalidates render.

buffer_insert_text(buffer_t* buf, size_t line, size_t col, const char* text)
  Inserts null-terminated text at line/col.

buffer_remove_char(buffer_t* buf, size_t line, size_t col)
  Removes character at line/col using memmove. No-op if col >= line length.

buffer_remove_range(buffer_t* buf, size_t line, size_t col, size_t rlen)
  Removes rlen characters starting at line/col. Clamps to line length.

buffer_split_line(buffer_t* buf, size_t line, size_t col)
  Splits line at col into two lines. Right part becomes new line at line+1.

buffer_join_lines(buffer_t* buf, size_t line)
  Joins line with line+1. Frees line+1 and decrements line_count.

buffer_line_length(buffer_t* buf, size_t line)
  Returns strlen of line at index.

buffer_get_line(buffer_t* buf, size_t line)
  Returns const char* to line at index.

buffer_update_render(buffer_t* buf)
  Recomputes render_lines using utf8proc_map for grapheme clusters.
  Only runs if render_valid is false.

buffer_set_cursor(buffer_t* buf, size_t line, size_t col)
  Sets cursor position. Clamps col to line length.

buffer_validate_cursor(buffer_t* buf)
  Clamps cursor to valid range (line < line_count, col <= line length).

buffer_display_name(buffer_t* buf)
  Returns display_name if set, else filename, else "[No Name]".

DETAILED API REFERENCE — EDITOR
--------------------------------
editor_create(void)
  Allocates editor_t, initializes mode, cursor, config, command registry,
  module registry. Registers builtin commands and builtin modules.

editor_destroy(editor_t* ed)
  Frees editor, buffers, config, command registry, module registry.

editor_set_mode(editor_t* ed, qicto_mode_t mode)
  Changes editor mode. Triggers on_mode_change callbacks in modules.

editor_open_file(editor_t* ed, const char* filename)
  Creates new buffer, loads file, adds to buffer list, sets as current.

editor_current_buffer(editor_t* ed)
  Returns ed->current_buffer.

editor_cycle_buffer(editor_t* ed, int direction)
  Moves to next/prev buffer in linked list.

editor_close_buffer(editor_t* ed, buffer_t* buf)
  Removes buffer from list, frees it. Adjusts current_buffer if needed.

editor_run(editor_t* ed)
  Main event loop: input → update → render.

editor_quit(editor_t* ed)
  Sets quit flag. Checks for unsaved buffers.

editor_redraw(editor_t* ed)
  Forces full screen redraw.

editor_set_status(editor_t* ed, const char* fmt, ...)
  Sets status message with printf-style formatting.

editor_config(editor_t* ed)
  Returns ed->config.

DETAILED API REFERENCE — COMMANDS
---------------------------------
commands_create(void)
  Allocates command_registry_t with initial capacity 0.

commands_destroy(command_registry_t* cmds)
  Frees entries array and registry struct.

commands_register(command_registry_t* cmds, const char* name, command_fn fn, const char* help)
  Adds command to registry. Grows array if needed. Returns 0 on success, -1 on error.

commands_find(command_registry_t* cmds, const char* name)
  Exact match first, then prefix match. Returns command_entry_t* or NULL.

commands_execute(command_registry_t* cmds, editor_t* ed, const char* input, char** out)
  Parses "name args", finds command, calls fn(ed, args, out).
  Returns QICTO_CMD_SUCCESS, QICTO_CMD_UNKNOWN, or QICTO_CMD_ERROR.

commands_list(command_registry_t* cmds, char*** names, size_t* count)
  Returns array of command names. Caller must free array (not strings).

Builtin Commands:
  cmd_quit     — Quit editor (warns if dirty)
  cmd_write    — Save current buffer
  cmd_edit     — Open file
  cmd_buffer   — List buffers
  cmd_help     — Show command help
  cmd_version  — Show version string
  cmd_lsmods   — List loaded modules

DETAILED API REFERENCE — CONFIG
--------------------------------
config_create(void)
  Allocates config_t with defaults.

config_destroy(config_t* cfg)
  Frees settings array and config struct.

config_set(config_t* cfg, const char* key, const char* value)
  Sets config key/value. Overwrites existing.

config_get(config_t* cfg, const char* key)
  Returns value for key, or NULL if not found.

config_save(config_t* cfg, const char* filename)
  Saves config to INI file via inih.

config_load(config_t* cfg, const char* filename)
  Loads config from INI file via inih.

Default Keys:
  tab_width       = 4
  expand_tabs     = true
  show_line_numbers = true
  auto_indent     = true
  mods_dir        = <platform_config_dir>/mods
  project_dir     = ""
  theme           = "default"

DETAILED API REFERENCE — MODULES
---------------------------------
mod_registry_t tracks loaded modules. Each module has:
  void* dl_handle          — Dynamic library handle (NULL for builtins)
  char name[64]            — Module name
  char version[256]        — Module version
  int refcount             — Reference count
  bool active              — Is module active
  Callbacks: on_init, on_cleanup, on_render, on_key, on_buffer_opened,
             on_buffer_changed, on_mode_change, on_config_reload, on_command
  get_name(), get_version() — Query functions

Module Lifecycle:
  1. mod_get_api() returns qicto_mod_api_t*
  2. .init(editor_t*) called during registration
  3. Callbacks invoked during editor lifecycle
  4. .cleanup(editor_t*) called on unload or editor destroy

Builtin Modules:
  syntax_mod    — Keyword-based syntax highlighting
  statusbar_mod — Status bar rendering
  filetree_mod  — File tree sidebar

DETAILED API REFERENCE — PLATFORM
----------------------------------
platform_init(void) / platform_deinit(void)
  Global platform initialization and cleanup.

platform_file_exists(const char* path) → bool
platform_file_size(const char* path) → int64_t
platform_file_mtime(const char* path) → time_t
platform_file_read(const char* path, char** out, size_t* len) → int
platform_file_write(const char* path, const char* data, size_t len) → int
platform_mkdir(const char* path) → int
platform_is_dir(const char* path) → bool
platform_list_dir(const char* path, char*** entries, size_t* count) → int
platform_free_dir_listing(char** entries, size_t count)
platform_resolve_path(const char* path, char* out, size_t outlen) → int
platform_home_dir(char* out, size_t outlen) → int
platform_config_dir(char* out, size_t outlen) → int
platform_dl_open(const char* path) → void*
platform_dl_close(void* handle) → int
platform_dl_sym(void* handle, const char* name) → void*
platform_dl_error(char* out, size_t outlen) → int

Windows Implementation (platform_windows.c):
  Uses GetFileAttributesEx, CreateFile, ReadFile/WriteFile
  dlopen → LoadLibrary, dlsym → GetProcAddress, dlclose → FreeLibrary
  Config dir: %APPDATA%/qicto
  Home dir: %USERPROFILE%

DETAILED API REFERENCE — INPUT
-------------------------------
notcurses input events are translated to qicto key events in input.c.
Key mapping uses NCKEY_* constants from notcurses v3.0.

Key Categories:
  Navigation:  h/j/k/l, arrow keys, Home/End, PageUp/PageDown
  Editing:     x/X, i/a/o/O, Backspace, Delete
  Selection:   v (visual), V (select line)
  Mode:        Esc (normal), : (command), / (search)
  Buffer:      b/B (next/prev buffer), w (save), q (quit)
  Mouse:       Click to position cursor (if enabled)

Input Processing:
  1. notcurses_get() blocks for input
  2. Translate ncinput to qkey_t
  3. Dispatch based on current mode
  4. Handle module on_key callbacks (if any consume the key)
  5. Update editor state
  6. Trigger render

DETAILED API REFERENCE — RENDERING
-----------------------------------
Rendering is done via notcurses ncplane API.
Standard plane is obtained via notcurses_stdplane().

Drawing Operations:
  ncplane_cursor_move_yx(nc, y, x)     — Move cursor
  ncplane_set_fg_rgb8(nc, r, g, b)     — Set foreground color
  ncplane_set_bg_rgb8(nc, r, g, b)     — Set background color
  ncplane_putchar(nc, c)               — Print character
  ncplane_putstr(nc, str)              — Print string
  ncplane_putegc(nc, str, bytes)       — Print extended grapheme cluster

Style Constants (NCSTYLE_*):
  NCSTYLE_NONE      = 0
  NCSTYLE_BOLD      = 0x0002u
  NCSTYLE_UNDERLINE = 0x0008u
  NCSTYLE_ITALIC    = 0x0010u
  NCSTYLE_STRUCK    = 0x0001u
  NCSTYLE_UNDERCURL = 0x0004u
  NCSTYLE_MASK      = 0xffffu

Render Pipeline:
  1. tui_clear() — erase standard plane
  2. For each visible line in current buffer:
     a. Set FG color based on syntax highlighting
     b. Move cursor to line start
     c. Print text with ncplane_putstr
  3. Render statusbar module callback
  4. Render command line
  5. Position cursor at buffer cursor location
  6. notcurses_render() — flush to terminal

DETAILED API REFERENCE — STRINGS
---------------------------------
qstr_dup(const char* s) → char*
  Duplicates string via malloc. Returns NULL on failure.

qstr_ndup(const char* s, size_t n) → char*
  Duplicates up to n characters.

qstr_concat(const char* a, const char* b) → char*
  Concatenates two strings. Caller must free.

qstr_trim(char* s) → char*
  Trims whitespace in-place. Returns s.

qstr_ltrim(char* s) → char*
  Trims leading whitespace in-place.

qstr_rtrim(char* s) → char*
  Trims trailing whitespace in-place.

qstr_split(const char* s, char delim, int* count) → char**
  Splits string by delimiter. Returns array of strings.

qstr_free_split(char** arr, int count)
  Frees array returned by qstr_split.

qstr_starts_with(const char* s, const char* prefix) → int
  Returns 1 if s starts with prefix, 0 otherwise.

qstr_ends_with(const char* s, const char* suffix) → int
  Returns 1 if s ends with suffix, 0 otherwise.

qstr_display_width(const char* s) → size_t
  Returns display width of UTF-8 string using utf8proc_charwidth.

qstr_to_lower(char* s) → char*
  Converts string to lowercase in-place.

qstr_to_upper(char* s) → char*
  Converts string to uppercase in-place.

DETAILED API REFERENCE — FILE OPS
----------------------------------
platform_file_exists(const char* path) → bool
platform_file_size(const char* path) → int64_t
platform_file_mtime(const char* path) → time_t
platform_file_read(const char* path, char** out, size_t* len) → int
platform_file_write(const char* path, const char* data, size_t len) → int
platform_mkdir(const char* path) → int
platform_is_dir(const char* path) → bool
platform_list_dir(const char* path, char*** entries, size_t* count) → int
platform_free_dir_listing(char** entries, size_t count)
platform_resolve_path(const char* path, char* out, size_t outlen) → int
platform_home_dir(char* out, size_t outlen) → int
platform_config_dir(char* out, size_t outlen) → int

DETAILED API REFERENCE — DYNAMIC ARRAY
---------------------------------------
dynarray_t is a generic dynamic array (void* based).
Used for command entries, module entries, etc.

dynarray_init(dynarray_t* arr, size_t elem_size)
dynarray_free(dynarray_t* arr)
dynarray_push(dynarray_t* arr, const void* elem) → int
dynarray_pop(dynarray_t* arr, void* elem) → int
dynarray_get(dynarray_t* arr, size_t index) → void*
dynarray_set(dynarray_t* arr, size_t index, const void* elem) → int
dynarray_count(dynarray_t* arr) → size_t
dynarray_clear(dynarray_t* arr)

CONTRIBUTOR CHECKLIST
---------------------
Before submitting a PR:
  [ ] Code compiles with -Wall -Wextra -Werror (Linux/macOS)
  [ ] All tests pass: ctest --output-on-failure
  [ ] New public types added to src/qicto.h
  [ ] New module callbacks return qicto_cmd_result_t for .init
  [ ] notcurses v3.0 API used (ncplane_cursor_move_yx, etc.)
  [ ] utf8proc v2.11.0 types used correctly
  [ ] No trailing whitespace in any files
  [ ] AGENT.md updated if new patterns/APIs added
  [ ] CMakeLists.txt updated if new source files added
  [ ] Windows compatibility verified (MSYS2 ucrt64)

REPORTING BUGS
--------------
Include:
  1. OS and compiler version (e.g., Windows 11 + GCC 16.2.0)
  2. Build type (Release/Debug/ASAN)
  3. Full build log (cmake --build build)
  4. Test output (ctest --output-on-failure)
  5. Steps to reproduce
  6. Expected vs actual behavior

LICENSE
-------
Copyright 2026 Seigh-sword
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

REFERENCES
----------
- notcurses:     https://github.com/dankamongmen/notcurses
- tree-sitter:   https://github.com/tree-sitter/tree-sitter
- utf8proc:      https://github.com/JuliaStrings/utf8proc
- pcre2:         https://github.com/PCRE2Project/pcre2
- libuv:         https://github.com/libuv/libuv
- libgit2:       https://github.com/libgit2/libgit2
- json-c:        https://github.com/json-c/json-c
- cwalk:         https://github.com/likle/cwalk
- cargs:         https://github.com/likle/cargs
- inih:          https://github.com/benhoyt/inih
- uthash:        https://github.com/troydhanson/uthash

APPENDIX — CMAKE TARGET DEPENDENCY GRAPH
-----------------------------------------
qicto (EXECUTABLE)
  └── qicto_core (STATIC)
        ├── notcurses-core (SHARED) / notcurses-core-static (STATIC)
        ├── tree_sitter_c (STATIC) — embeds parser.c
        ├── tree_sitter_cpp (STATIC) — embeds parser.c + scanner.c
        ├── json-c (STATIC)
        ├── uv (SHARED)
        ├── pcre2-8 (STATIC)
        ├── utf8proc (STATIC)
        ├── cargs (STATIC)
        ├── inih (STATIC)
        ├── cwalk (STATIC)
        └── git2 (ALIAS for libgit2, STATIC)

test_buffer (EXECUTABLE)
  └── qicto_core (STATIC)

test_editor (EXECUTABLE)
  └── qicto_core (STATIC)

test_commands (EXECUTABLE)
  └── qicto_core (STATIC)

APPENDIX — FETCHCONTENT DEPENDENCY MATRIX
------------------------------------------
Dependency           Version   Type        Link     Include
-----------------------------------------------------------------
uthash               v2.3.0    Header-only None     PUBLIC (from subbuild)
cwalk                master    Static      STATIC   PUBLIC
inih                 master    Static      STATIC   PUBLIC
cargs                v1.0.3    Static      STATIC   PUBLIC
utf8proc             v2.11.0   Static      STATIC   PUBLIC
pcre2                pcre2-10.43 Static    STATIC   PUBLIC
tree-sitter          v0.24.0   Header-only None     PUBLIC (includes only)
tree-sitter-c        v0.21.0   Static      STATIC   PUBLIC
tree-sitter-cpp      v0.22.0   Static      STATIC   PUBLIC
json-c               json-c-0.17-20230812 Static STATIC PUBLIC
libuv                v1.48.0   Shared      SHARED   PUBLIC
notcurses            v3.0.10   Shared/Static SHARED/STATIC PUBLIC
libgit2              v1.8.1    Static      STATIC   PUBLIC (ALIAS: git2)

APPENDIX — WINDOWS BUILD REQUIREMENTS
--------------------------------------
- MSYS2 ucrt64 environment (C:\msys64\ucrt64)
- Packages: mingw-w64-ucrt-x86_64-gcc, mingw-w64-ucrt-x86_64-cmake,
            mingw-w64-ucrt-x86_64-ninja, mingw-w64-ucrt-x86_64-libunistring
- PowerShell 5.1+ for build scripts
- Git for Windows (for FetchContent)
- No Visual Studio required (uses MinGW GCC)

APPENDIX — LINUX BUILD REQUIREMENTS
-------------------------------------
- GCC 9+ or Clang 10+
- CMake 3.21+
- Ninja (optional but recommended)
- libunistring-dev
- libncursesw-dev (for notcurses if not using FetchContent)
- Or use FetchContent for all dependencies

APPENDIX — MACOS BUILD REQUIREMENTS
-------------------------------------
- Xcode Command Line Tools
- CMake 3.21+ (brew install cmake)
- Ninja (brew install ninja)
- Homebrew dependencies (optional if using FetchContent):
    brew install notcurses utf8proc pcre2 libuv json-c cwalk inih

APPENDIX — TROUBLESHOOTING CHECKLIST
-------------------------------------
Build fails with "cannot find -ltree_sitter":
  → Remove tree_sitter from target_link_libraries. It's header-only.

Build fails with "struct termios incomplete":
  → Patch notcurses compat.h with MinGW struct termios shim.

Build fails with "strndup undeclared":
  → Use qicto_strndup shim (already in buffer.c).

Build fails with "cwk_path_get_basename undeclared":
  → Ensure cwalk.h is included (already in filetree_mod.c).

Config fails with "target 'tree_sitter' is not defined":
  → Tree-sitter v0.24+ has no root CMakeLists.txt. Don't link it.

Tests fail with "buffer should have 1 line":
  → Check buffer->text.line_count (not buf->line_count).

Tests fail with "register should succeed":
  → commands_register requires non-NULL function pointer.

Runtime crash in notcurses:
  → Ensure C_EXTENSIONS ON for notcurses-core targets.
  → Check terminal supports UTF-8 and true color.

APPENDIX — CODE METRICS
------------------------
  Total source files:     ~20 .c files
  Total header files:     ~20 .h files
  Total test files:       3 .c files
  Lines of C code:        ~3000+ (estimated)
  Lines of CMake:         ~340 (CMakeLists.txt + Dependencies.cmake)
  Dependencies:           13+ via FetchContent
  Build targets:          8+ (qicto, qicto_core, 3 tests, deps)
  Supported platforms:    3 (Windows, Linux, macOS)

APPENDIX — FUTURE MIGRATION NOTES
----------------------------------
- notcurses v4.0 may change API again — monitor upstream
- tree-sitter v0.25+ may change grammar loading mechanism
- utf8proc v3.0 may change type names or API signatures
- libgit2 v1.9+ may change option names or add new features
- CMake 3.30+ may deprecate FetchContent_Populate in favor of FetchContent_MakeAvailable
- C23 adoption: consider migrating from C11 when compilers stabilize

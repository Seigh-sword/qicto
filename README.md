# QICTO

A fast, modular TUI text editor written in C11.

![Version](https://img.shields.io/badge/version-0.1.0-blue)
![License](https://img.shields.io/badge/license-Apache_2.0-green)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)

## What is QICTO?

**QICTO** is a vim-inspired, modal terminal text editor designed for **speed and extensibility**. Every feature beyond core text editing lives in a `module`, making the editor `lightweight` at its core and `infinitely expandable`.

## Key Features

- **Modal Editing** — NORMAL, INSERT, VISUAL, SELECT, COMMAND, and SEARCH modes
- **Buffer-Centric** — Multiple files open simultaneously as buffers
- **Modular Architecture** — Builtin + dynamically loaded modules
- **TUI Rendering** — Powered by notcurses v3.0.x for modern terminal graphics
- **Cross-Platform** — Windows (MSYS2/MinGW), Linux, macOS
- **UTF-8 Aware** — Full Unicode support via utf8proc
- **Syntax Highlighting** — Keyword-based highlighting with tree-sitter grammars compiled in
- **Git Integration** — Built on libgit2 for version control features
- **Static Linking** — Single binary deployment preferred

## Quick Start

### Prerequisites

- CMake 3.21 or later
- Ninja build system
- C11 compiler (GCC 16+ or Clang 16+)
- C++ compiler (for tree-sitter grammars)

### Build

```bash
# to configure the project
cmake -B build -G Ninja

# building command is...
cmake --build build

# finally for running...
./build/bin/qicto
```

### Windows (MSYS2 ucrt64)

```bash
# to install dependencies run these...
pacman -S mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
         mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-libunistring

# To build run these...
cmake -B build -G Ninja
cmake --build build
```

## Keybindings

| Key | Action |
|-----|--------|
| `i` | Enter INSERT mode |
| `Esc` | Return to NORMAL mode |
| `h/j/k/l` | Move cursor left/down/up/right |
| `:` | Enter COMMAND mode |
| `/` | Enter SEARCH mode |
| `w` | Save file |
| `q` | Quit (if no unsaved changes) |
| `b/B` | Next/previous buffer |

## Commands

Type `:` in COMMAND mode to execute:

- `:quit` or `:q` — Quit the editor
- `:write` or `:w` — Save current buffer
- `:edit <file>` or `:e <file>` — Open a file
- `:buffer` or `:ls` — List buffers
- `:help` — Show help
- `:version` — Show version
- `:lsmods` — List loaded modules

## Configuration

QICTO uses INI configuration files. The default config path is:

- Linux: `~/.config/qicto/config.ini`
- Windows: `%APPDATA%\qicto\config.ini`
- macOS: `~/.config/qicto/config.ini`

### Example Config

```ini
tab_width = 4
expand_tabs = false
show_line_numbers = true
show_whitespace = false
auto_indent = true
word_wrap = false
default_syntax = c
theme = default
mods_dir = ~/.config/qicto/mods
project_dir = .
```

## Module System

QICTO's functionality is organized into modules. Builtin modules are compiled into the editor, and external modules can be loaded dynamically from shared libraries.

### Builtin Modules

- **syntax** — Keyword-based syntax highlighting for C, C++, Python, JS, Go, Rust, HTML, CSS, JSON, YAML, TOML, Markdown, Shell, Dockerfile
- **statusbar** — Status bar rendering with mode indicator and buffer info
- **filetree** — File tree sidebar for project navigation

### Writing a Module

A module is a shared library that exports a single function:

```c
#include "qicto.h"

static qicto_cmd_result_t my_init(editor_t* ed) {
    (void)ed;
    return QICTO_CMD_SUCCESS;
}

static qicto_mod_api_t s_my_api = {
    .name = "my_module",
    .version = "0.1.0",
    .description = "My awesome module",
    .init = my_init,
    .cleanup = NULL,
    .on_render = NULL,
    .on_key = NULL,
    .on_buffer_opened = NULL,
    .on_buffer_changed = NULL,
    .on_mode_change = NULL,
    .on_config_reload = NULL,
    .on_command = NULL,
};

const qicto_mod_api_t* mod_get_api(void) {
    return &s_my_api;
}
```

Place the compiled `.dll`/`.so` in your mods directory.

## Architecture

```
qicto/
├── src/
│   ├── main.c              Entry point, CLI parsing
│   ├── qicto.h             Master header (all public types)
│   ├── core/               Editor core logic
│   │   ├── buffer.c        Text buffer management
│   │   ├── editor.c        Editor state machine
│   │   ├── command.c       Command registry
│   │   ├── config.c        INI config loading
│   │   ├── module.c        Module system
│   │   └── mod_load.c      Dynamic module loading
│   ├── ui/                 Terminal UI
│   │   ├── tui.c           notcurses lifecycle
│   │   ├── renderer.c      Drawing primitives
│   │   ├── input.c         Keyboard handling
│   │   └── layout.c        Window layout
│   ├── utils/              Utilities
│   │   ├── strings.c       String & UTF-8 utilities
│   │   ├── fileops.c       File I/O wrappers
│   │   └── dynarray.c      Dynamic arrays
│   ├── platform/           OS abstraction
│   │   ├── platform.c      Cross-platform API
│   │   └── platform_*.c    OS-specific implementations
│   └── modules/builtin/    Builtin modules
│       ├── syntax_mod.c
│       ├── statusbar_mod.c
│       └── filetree_mod.c
├── tests/                  Unit tests
└── cmake/                  Build configuration
```

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| notcurses | v3.0.10 | TUI rendering |
| tree-sitter | v0.24.0 | Incremental parsing engine |
| tree-sitter-c | v0.21.0 | C grammar |
| tree-sitter-cpp | v0.22.0 | C++ grammar |
| utf8proc | v2.11.0 | Unicode/UTF-8 processing |
| pcre2 | 10.43 | Regular expressions |
| libuv | v1.48.0 | Async I/O / event loop |
| json-c | 0.17 | JSON parsing |
| inih | master | INI config parsing |
| cargs | v1.0.3 | CLI argument parsing |
| cwalk | master | Cross-platform path handling |
| uthash | v2.3.0 | Hash tables |
| libgit2 | v1.8.1 | Git integration |

All dependencies are fetched automatically via CMake FetchContent.

## Building from Source

### Dependencies

- CMake >= 3.21
- Ninja
- C11 compiler (GCC or Clang)
- C++ compiler (for tree-sitter grammars)
- MSYS2 ucrt64 on Windows (recommended)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/Seigh-sword/qicto.git
cd qicto

# Configure
cmake -B build -G Ninja

# Build
cmake --build build

# Run tests
ctest --output-on-failure

# Install (optional)
cmake --install build
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `QICTO_BUILD_TESTS` | ON | Build unit tests |
| `QICTO_BUILD_MODS_DIR` | ON | Enable external module loading |
| `QICTO_ENABLE_ASAN` | OFF | Enable AddressSanitizer |
| `CMAKE_BUILD_TYPE` | Release | Debug or Release |

## Contributing

1. Read `AGENT.md` — it contains the complete project context
2. Fork the repository
3. Create a feature branch
4. Make your changes following the style guide
5. Run `cmake --build build` and fix all warnings
6. Run `ctest --output-on-failure` and ensure all tests pass
7. Submit a pull request

## License

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

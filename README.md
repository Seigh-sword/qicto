# QICTO

A fast, modular TUI text editor written in C11.

![Version](https://img.shields.io/badge/version-0.1.0-blue)
![License](https://img.shields.io/badge/license-Apache_2.0-green)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)

## What is QICTO?

QICTO is a modern, modal terminal code editor. It borrows the modal flow of vim but
aims for the speed and feel of a code editor. Every feature beyond core text
editing lives in a `module`, so the core stays small and the surface grows without
touching it.

## Highlights

- **Modal editing** — NORMAL, INSERT, VISUAL, COMMAND, and SEARCH modes
- **Buffers** — Multiple files open at once; cycle with `B`, `Ctrl+N`, `Ctrl+P`
- **Tree-sitter highlighting** — Real grammar-driven coloring for C and C++,
  with a keyword-based fallback for everything else
- **Undo** — 64-step linear history with `:undo` or `u`
- **Search** — `/text` to search forward, `n`/`N` for next/prev, last pattern remembered
- **Word motions** — `w`/`b`/`e` and `0`/`$`/`^`/`gg`/`G`, plus `PgUp`/`PgDn`
- **Visual mode** — `v` character-wise, `V` line-wise, then `x`/`d` to delete, `y` to yank
- **Yank/paste** — Single register; `x`, `dd`, and visual-mode ops all populate it; `p` pastes
- **Scrolling** — Cursor is kept on screen automatically, the renderer draws
  the line-number gutter and any per-cell syntax colors
- **Splits** — A real binary-tree layout (`qicto_layout_t`); nodes can be split,
  closed, and resized. The editor still draws the active leaf for now, the
  splitting primitives are ready for a multi-pane UI
- **Modules** — Builtin and dynamically loaded; `syntax`, `statusbar`, `filetree`
  ship in the binary
- **UTF-8 aware** — All text handled through utf8proc, so multi-byte
  characters and display width are correct

## Quick Start

### Prerequisites

- CMake 3.21 or later
- Ninja
- A C11 compiler (GCC 12+ or Clang 14+ work; GCC 16+ recommended for tree-sitter)
- A C++ compiler (the tree-sitter grammars are C++)

### Build

```bash
cmake -B build -G Ninja
cmake --build build
./build/bin/qicto
```

### Windows (MSYS2 ucrt64)

```bash
pacman -S mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja \
          mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-libunistring

cmake -B build -G Ninja
cmake --build build
```

## Keybindings

### NORMAL mode

| Key | Action |
|-----|--------|
| `i` / `a` / `A` | Enter INSERT mode (at cursor / after / end of line) |
| `o` / `O` | Open new line below / above and enter INSERT |
| `v` / `V` | Enter VISUAL mode (char-wise / line-wise) |
| `Esc` | Return to NORMAL; clears the status message |
| `h` `j` `k` `l` | Move left / down / up / right |
| `w` / `b` / `e` | Next word / previous word / end of word |
| `0` / `$` / `^` | Start / end / first non-blank of line |
| `gg` / `G` | First line / last line |
| `PgUp` / `PgDn` | Scroll one page up / down |
| `Home` / `End` | Start / end of line |
| `x` / `X` | Delete char under / before cursor (yanks it) |
| `dd` | Delete current line (yanks it) |
| `p` | Paste the yank register after the cursor |
| `u` | Undo last edit |
| `n` / `N` | Repeat last search forward / backward |
| `q` | Quit (refuses if buffers are dirty) |
| `:` | Enter COMMAND mode |
| `/` | Enter SEARCH mode |
| `B` | Previous buffer |
| `Ctrl+N` / `Ctrl+P` | Next / previous buffer |

### INSERT mode

| Key | Action |
|-----|--------|
| Printable keys | Insert character at cursor |
| `Enter` | Split line |
| `Tab` | Insert spaces (per `tab_width`) or a literal `\t` |
| `Backspace` / `Del` | Delete char before / under cursor |
| `Esc` | Return to NORMAL |

### VISUAL mode

| Key | Action |
|-----|--------|
| `h` `j` `k` `l` / `w` `b` / `0` `$` | Extend selection |
| `v` | Return to NORMAL (keep cursor) |
| `x` / `d` | Delete the selection (yanks it) |
| `y` | Yank the selection, return to NORMAL |

### COMMAND mode

Type `:` to enter. Press `Enter` to run, `Esc` to cancel.

| Command | Action |
|---------|--------|
| `:q` / `:quit` | Quit (refuses if dirty) |
| `:q!` / `:quit!` | Force quit |
| `:w` / `:write` [path] | Save current buffer |
| `:e` / `:edit` \<path\> | Open a file |
| `:b` / `:buffer` [N] | Switch to buffer N (omit N to list) |
| `:ls` | List buffers |
| `:undo` / `:u` | Undo last edit |
| `:search` \<text\> | Search forward (same as `/`) |
| `:help` | Show all commands with their help text |
| `:version` | Show editor version |
| `:lsmods` | List loaded modules |

## Configuration

QICTO uses INI config files. Default path:

- Linux: `~/.config/qicto/qicto.ini`
- Windows: `%APPDATA%\qicto\qicto.ini`
- macOS: `~/.config/qicto/qicto.ini`

Pass an explicit file with `qicto -c path/to/config.ini`.

### Example Config

```ini
tab_width = 4
expand_tabs = true
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

QICTO's functionality lives in modules. Builtin modules are linked into the
binary; external modules are shared libraries that live in `mods_dir` and are
loaded on startup.

### Builtin Modules

- **syntax** — Tree-sitter highlighting for C/C++, keyword fallback otherwise
- **statusbar** — Mode indicator and buffer info on the status line
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

Drop the compiled `.dll` (Windows) or `.so` (Linux/macOS) into your `mods_dir`.

## Architecture

```
qicto/
├── src/
│   ├── main.c              Entry point, CLI parsing, main loop
│   ├── qicto.h             Master header (all public types + the
│   │                       QICTO_HL_* highlight groups)
│   ├── engine/
│   │   ├── buffer.c        Text buffer (line array, cursor, undo ring)
│   │   ├── editor.c        Editor state machine
│   │   ├── command.c       Command registry and dispatch
│   │   ├── config.c        INI config loading
│   │   ├── module.c        Module system
│   │   └── mod_load.c      Dynamic module loading
│   ├── ui/
│   │   ├── tui.c           notcurses lifecycle and main render
│   │   ├── renderer.c      Drawing primitives, syntax color map
│   │   ├── input.c         Keyboard mapping, modal handlers
│   │   └── layout.c        Binary-tree split/close/resize
│   ├── utils/
│   │   ├── strings.c       String + UTF-8 utilities
│   │   ├── fileops.c       File I/O wrappers
│   │   └── dynarray.c      Dynamic arrays
│   ├── platform/
│   │   ├── platform.c      Cross-platform API
│   │   └── platform_*.c    OS-specific implementations
│   └── modules/builtin/
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
- A C11 compiler (GCC 12+ or Clang 14+; GCC 16+ recommended for tree-sitter)
- A C++ compiler (for the tree-sitter grammars)
- MSYS2 ucrt64 on Windows (recommended)

### Build Steps

```bash
git clone https://github.com/Seigh-sword/qicto.git
cd qicto

cmake -B build -G Ninja
cmake --build build

ctest --output-on-failure

cmake --install build   # optional
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

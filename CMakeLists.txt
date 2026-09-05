cmake_minimum_required(VERSION 3.21)

project(qicto
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "A fast, modular TUI text editor"
    HOMEPAGE_URL "https://github.com/Seigh-sword/qicto.git"
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(QICTO_BUILD_TESTS "Build unit tests" ON)
option(QICTO_BUILD_MODS_DIR "Build with external mod loading support" ON)
option(QICTO_ENABLE_ASAN "Enable AddressSanitizer" OFF)

if(QICTO_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

include(GNUInstallDirs)
include(cmake/Dependencies.cmake)

set(QICTO_CORE_SOURCES
    src/engine/buffer.c
    src/engine/editor.c
    src/engine/module.c
    src/engine/mod_load.c
    src/engine/config.c
    src/engine/command.c
    src/ui/tui.c
    src/ui/renderer.c
    src/ui/input.c
    src/ui/layout.c
    src/utils/strings.c
    src/utils/fileops.c
    src/utils/dynarray.c
    src/platform/platform.c
    src/modules/builtin/syntax_mod.c
    src/modules/builtin/statusbar_mod.c
    src/modules/builtin/filetree_mod.c
    src/modules/builtin/mod_builtins.c
)

add_library(qicto_core STATIC ${QICTO_CORE_SOURCES})

target_include_directories(qicto_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
        ${CMAKE_CURRENT_SOURCE_DIR}/src/modules
)

target_link_libraries(qicto_core
    PUBLIC
        notcurses
        tree_sitter_c
        tree_sitter_cpp
        json-c
        uv
        pcre2-8
        utf8proc
        cargs
        inih
        cwalk
        git2
        ${CMAKE_DL_LIBS}
)

if(WIN32)
    target_include_directories(qicto_core PUBLIC
        ${CMAKE_SOURCE_DIR}/build/_deps/notcurses-src/include
        ${CMAKE_SOURCE_DIR}/build/_deps/notncurses-build/include
    )
endif()

if(WIN32)
    target_compile_definitions(qicto_core PRIVATE QICTO_PLATFORM_WINDOWS)
    target_link_libraries(qicto_core PRIVATE iphlpapi)
endif()

if(UNIX AND NOT APPLE)
    target_compile_definitions(qicto_core PRIVATE QICTO_PLATFORM_LINUX)
    target_link_libraries(qicto_core PRIVATE dl)
endif()

add_executable(qicto src/main.c)
target_link_libraries(qicto PRIVATE qicto_core)
set_target_properties(qicto PROPERTIES
    OUTPUT_NAME "qicto"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(QICTO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS qicto qicto_core
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qicto
    FILES_MATCHING PATTERN "*.h"
)

message(STATUS "")
message(STATUS "qicto ${PROJECT_VERSION}")
message(STATUS "  Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:     ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Platform:     ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Tests:        ${QICTO_BUILD_TESTS}")
message(STATUS "  Dependencies: 13+ libraries via FetchContent")
message(STATUS "")

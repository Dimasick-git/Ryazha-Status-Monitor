# This CMAKE is only for local tests of smd_parser, you won't compile homebrew with it
cmake_minimum_required(VERSION 3.16)
project(smd_parser CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(SMD_SANITIZE    "Build with -fsanitize=address,undefined"     OFF)
option(SMD_BUILD_TESTS "Build the test suite"                        ON)
# Enabling SMD_DEBUG_HISTORY exposes the debug-only HistoryUpdate /
# HistoryClean callbacks to consumers (see smd_parser.hpp). The history
# ring buffer is always maintained internally; this flag only controls
# whether the host receives a callback for each push/clear.
option(SMD_DEBUG_HISTORY "Define DEBUG so HistoryUpdate/HistoryClean are visible" OFF)

if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra)
endif()

if(SMD_DEBUG_HISTORY)
    add_compile_definitions(DEBUG)
endif()

if(SMD_SANITIZE)
    if(MSVC)
        message(FATAL_ERROR "SMD_SANITIZE is GCC/Clang only")
    endif()
    set(SAN_FLAGS
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all
        -g
    )
    add_compile_options(${SAN_FLAGS})
    add_link_options(${SAN_FLAGS})
endif()

# ---------------------------------------------------------------------------
# tinyexpr (vendored; Marek's modified copy)
# ---------------------------------------------------------------------------
add_library(tinyexpr STATIC lib/tinyexpr/tinyexpr.c)
target_include_directories(tinyexpr PUBLIC lib/tinyexpr)

# ---------------------------------------------------------------------------
# smd_parser
# ---------------------------------------------------------------------------
add_library(smd_parser STATIC source/smd_parser.cpp)
target_include_directories(smd_parser PUBLIC include)
target_link_libraries(smd_parser PUBLIC tinyexpr)
if(UNIX)
    target_link_libraries(smd_parser PUBLIC m)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(SMD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

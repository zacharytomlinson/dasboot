# Repository Guidelines

## Project Structure & Module Organization

- `CMakeLists.txt`: build configuration (C++20).
- `main.cpp`: current application entry point.
- `firmware/`: AVR firmware/bootloader code (ATmega4808 bring-up, SPI drivers).
- `cmake-build-debug/`: CLion-generated build output (do not commit). Prefer a local `build/` directory for CLI builds.

As the project grows, prefer:
- `src/` for implementation (`.cpp`)
- `include/` for public headers (`.hpp`)
- `tests/` for test code

## Build, Test, and Development Commands

Toolchain note: this repo targets CMake 3.16+.

Firmware builds require `avr-gcc` and `avr-objcopy` (plus `ninja` or `make`).

Configure an out-of-source build:
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`

Build the executable:
- `cmake --build build`

Run locally:
- `./build/dasboot`

Tests are not wired up yet. If/when tests are added via CTest (`enable_testing()` + `add_test()`), run:
- `ctest --test-dir build`

Build the AVR firmware:
- `cmake -S firmware -B build-fw -G Ninja -DCMAKE_TOOLCHAIN_FILE=firmware/cmake/avr-gcc-toolchain.cmake`
- `cmake --build build-fw`

## Coding Style & Naming Conventions

- Formatting: follow `.clang-format` (4 spaces, LLVM-based). Run: `clang-format -i main.cpp` (or target files).
- C++ standard: keep code compatible with C++20 (`set(CMAKE_CXX_STANDARD 20)`).
- Naming (recommended): `PascalCase` for types, `lower_snake_case` for variables/functions, `lower_snake_case.cpp` for file names.

## Testing Guidelines

- Place tests in `tests/` and name files `*_test.cpp` (e.g., `tests/parser_test.cpp`).
- Keep tests deterministic and runnable from the command line (no IDE-only configuration).

## Commit & Pull Request Guidelines

This directory currently has no Git history checked in. If you initialize Git, use Conventional Commits:
- `feat: …`, `fix: …`, `refactor: …`, `test: …`, `docs: …`

Pull requests should include:
- a short description of the change and rationale
- how to build/run/test (commands + expected output when relevant)
- screenshots or console output for user-visible behavior changes

## Configuration Tips

- Don’t commit generated files/directories: `build/`, `cmake-build-*/`, `.idea/`, `*.user`, `compile_commands.json` (unless intentionally tracked).

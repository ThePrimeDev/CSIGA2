# Counter-Strike Intelligent Game Assistant 2 (CSIGA2)

This is a minimal Dear ImGui + SDL3 + OpenGL2 overlay scaffold for Counter-Strike Intelligent Game Assistant 2.

## Prerequisites

- CMake 3.16+
- A C++17 compiler
- SDL3 source placed at `external/SDL`
- OpenGL development package
- X11 development packages (for clickthrough on X11)
- Dear ImGui source placed at `external/imgui`

## Setup Dear ImGui

Clone the Dear ImGui repository into `external/imgui` so that `external/imgui/imgui.h` exists.

## Setup SDL3

Clone the SDL repository into `external/SDL` so that `external/SDL/CMakeLists.txt` exists.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/CSIGA2
```

## Notes

- The window is borderless, always on top, and transparent.
- Clickthrough is enabled by default on X11; press `Insert` to toggle it so you can interact with the UI.
- Clickthrough relies on the X11 Shape extension and requires a compositor for transparency.

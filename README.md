# FifthOS

FifthOS is an ESP32-hosted Forth system with a browser REPL, a portable graphics layer, and a retained-mode embedded GUI runtime aimed at small touch displays.

This repository currently targets the Waveshare `ESP32-S3-AMOLED-1.91` board with a `240x536` RM67162 AMOLED panel and FT3168 touch controller. The active build path is PlatformIO only.

## What FifthOS Provides

- A live Forth/Fifth interpreter running on ESP32-S3
- A browser-based REPL served directly from the device
- A display-backend abstraction that hides the AMOLED driver
- Graphics primitives exposed as Forth words
- A retained GUI tree with styles, widgets, events, and swipeable screens
- A touch-aware embedded UI model suitable for instrumentation and control

## Repository Layout

- `src/`: C/C++ implementation
- `include/`: public headers and boot-loaded Fifth vocabulary
- `boards/`: local PlatformIO board definition
- `docs/`: project documentation
- `platformio.ini`: active build configuration

## Quick Start

1. Build with `pio run -e esp32-s3-amoled-191`.
2. Upload with `pio run -e esp32-s3-amoled-191 -t upload`.
3. If no saved Wi-Fi exists, connect to the setup AP shown on the device.
4. Open the displayed setup URL or REPL URL in a browser.
5. Choose a Wi-Fi network on the setup page, or keep using the device through its own local AP.

FifthOS now boots and runs without any preconfigured Wi-Fi. If no saved network is available, it starts its own setup AP and keeps the REPL available locally.

## Documentation Set

- [docs/README.md](docs/README.md): documentation map and maintenance policy
- [docs/architecture.md](docs/architecture.md): detailed architecture and subsystem design
- [docs/user-guide.md](docs/user-guide.md): installation, build, upload, REPL, GUI, and day-to-day usage
- [docs/forth-reference.md](docs/forth-reference.md): FifthOS-specific words, constants, stack effects, and examples
- [docs/app-tutorial.md](docs/app-tutorial.md): step-by-step tutorial for building a retained GUI app with the current high-level Fifth vocabulary

## Documentation Policy

The documentation in `docs/` is intended to track the current firmware, not an aspirational design.

Any change to FifthOS-specific words or user-visible behavior should update the relevant documents in the same change, especially when modifying:

- [src/primitives.cpp](src/primitives.cpp)
- [src/dictionary.cpp](src/dictionary.cpp)
- [include/gui_boot.h](include/gui_boot.h)
- [include/gui_runtime.h](include/gui_runtime.h)
- [include/gfx_backend.h](include/gfx_backend.h)

If the code and docs disagree, fix the docs or the code immediately rather than letting them drift.

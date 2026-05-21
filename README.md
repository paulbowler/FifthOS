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

1. Copy [include/credentials_template.h](/Users/paulbowler/Documents/Projects/FifthOS/include/credentials_template.h:1) to `include/credentials.h`.
2. Edit `ssid` and `pass` in `include/credentials.h`.
3. Build with `pio run -e esp32-s3-amoled-191`.
4. Upload with `pio run -e esp32-s3-amoled-191 -t upload`.
5. Open `http://fifthos.local/` in a browser once the board joins Wi-Fi.

## Documentation Set

- [docs/README.md](/Users/paulbowler/Documents/Projects/FifthOS/docs/README.md:1): documentation map and maintenance policy
- [docs/architecture.md](/Users/paulbowler/Documents/Projects/FifthOS/docs/architecture.md:1): detailed architecture and subsystem design
- [docs/user-guide.md](/Users/paulbowler/Documents/Projects/FifthOS/docs/user-guide.md:1): installation, build, upload, REPL, GUI, and day-to-day usage
- [docs/forth-reference.md](/Users/paulbowler/Documents/Projects/FifthOS/docs/forth-reference.md:1): FifthOS-specific words, constants, stack effects, and examples

## Documentation Policy

The documentation in `docs/` is intended to track the current firmware, not an aspirational design.

Any change to FifthOS-specific words or user-visible behavior should update the relevant documents in the same change, especially when modifying:

- [src/primitives.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/primitives.cpp:1)
- [src/dictionary.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/dictionary.cpp:1)
- [include/gui_boot.h](/Users/paulbowler/Documents/Projects/FifthOS/include/gui_boot.h:1)
- [include/gui_runtime.h](/Users/paulbowler/Documents/Projects/FifthOS/include/gui_runtime.h:1)
- [include/gfx_backend.h](/Users/paulbowler/Documents/Projects/FifthOS/include/gfx_backend.h:1)

If the code and docs disagree, fix the docs or the code immediately rather than letting them drift.

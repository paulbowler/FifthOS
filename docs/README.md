# FifthOS Documentation

This directory is the canonical documentation set for the current FifthOS firmware in this repository.

## Documents

- [architecture.md](architecture.md): subsystem design, boot flow, runtime model, and GUI layering
- [user-guide.md](user-guide.md): installation, build, flashing, REPL use, GUI behavior, and troubleshooting
- [forth-reference.md](forth-reference.md): FifthOS-specific words, stack effects, constants, and examples
- [app-tutorial.md](app-tutorial.md): practical tutorial for building a complete watch-style app with the retained GUI vocabulary

## Scope

These documents describe:

- The active PlatformIO-based FifthOS build
- The current ESP32-S3 AMOLED target
- The FifthOS-specific extensions beyond the inherited eForth core
- The retained GUI stack and boot-loaded GUI vocabulary

They do not attempt to fully re-document every inherited core eForth word from upstream. The reference focuses on the FifthOS additions and the parts of the system a user or maintainer must understand to work effectively in this repository.

## Maintenance Rule

Treat documentation updates as part of the feature or bug fix, not as follow-up work.

Update this documentation set whenever a change affects:

- a public FifthOS word
- a stack effect
- a constant name or value
- boot behavior
- display, touch, or GUI behavior
- build, flash, or configuration steps
- repository structure

In practice, changes under `src/primitives.cpp`, `src/dictionary.cpp`, `include/gui_boot.h`, `include/gui_runtime.h`, `include/gfx_backend.h`, `src/main.cpp`, and `include/index_html.h` usually require a documentation review.

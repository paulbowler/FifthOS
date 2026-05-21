# FifthOS Architecture

## Overview

FifthOS combines four major pieces:

1. An ESP32-hosted eForth-derived interpreter and VM
2. A browser-served REPL transport over Wi-Fi
3. A portable display abstraction implemented in C/C++
4. A retained-mode GUI runtime exposed upward as Fifth words

The design intent is to keep board-specific and performance-sensitive work in a small C/C++ layer while moving the interesting UI composition and experimentation surface into Fifth itself.

## High-Level Data Flow

At boot:

1. `setup()` initializes the VM state.
2. Wi-Fi and mDNS come up through the network layer.
3. The dictionary is built, registering native primitives and constants.
4. The graphics and touch runtime is initialized.
5. The boot-loaded Fifth GUI vocabulary from [include/gui_boot.h](/Users/paulbowler/Documents/Projects/FifthOS/include/gui_boot.h:1) is evaluated in phases.
6. The retained demo app is built and drawn.
7. The HTTP server accepts browser REPL commands.

At runtime:

1. Browser input is posted to the device as a command string.
2. FifthOS evaluates that string inside the live interpreter.
3. Output is returned to the browser REPL transcript.
4. The GUI runtime polls touch, generates events, marks nodes dirty, and redraws the active app only when required.

## Subsystem Layout

### 1. VM and Dictionary

Core files:

- [include/vm.h](/Users/paulbowler/Documents/Projects/FifthOS/include/vm.h:1)
- [src/vm.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/vm.cpp:1)
- [include/dictionary.h](/Users/paulbowler/Documents/Projects/FifthOS/include/dictionary.h:1)
- [src/dictionary.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/dictionary.cpp:1)
- [src/primitives.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/primitives.cpp:1)
- [include/forth_runtime.h](/Users/paulbowler/Documents/Projects/FifthOS/include/forth_runtime.h:1)
- [src/forth_runtime.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/forth_runtime.cpp:1)

Responsibilities:

- Maintain the data stack, return stack, dictionary, and code space
- Register native words and constants
- Execute compiled words and evaluate incoming text buffers
- Bridge HTTP and GUI callbacks back into the interpreter

The VM remains the system center. The GUI layer is not a separate framework bolted on the side; it is another vocabulary and runtime surface available to the interpreter.

### 2. Network and REPL

Core files:

- [src/network.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/network.cpp:1)
- [src/http.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/http.cpp:1)
- [include/index_html.h](/Users/paulbowler/Documents/Projects/FifthOS/include/index_html.h:1)
- [src/main.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/main.cpp:1)

Responsibilities:

- Join Wi-Fi using `include/credentials.h`
- Publish an mDNS hostname from `deviceName`
- Serve the browser REPL UI
- Accept REPL commands over HTTP and return textual output

The browser REPL is only a transport and terminal surface. The interpreter remains live on-device.

### 3. Display Backend

Core files:

- [include/display.h](/Users/paulbowler/Documents/Projects/FifthOS/include/display.h:1)
- [src/display.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/display.cpp:1)
- [include/gfx_backend.h](/Users/paulbowler/Documents/Projects/FifthOS/include/gfx_backend.h:1)
- [src/gfx_backend.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/gfx_backend.cpp:1)

Responsibilities:

- Initialize the RM67162 AMOLED through `Arduino_GFX`
- Hide all board-specific display details behind `gfx_*` functions
- Report logical screen dimensions and rotation
- Provide basic drawing primitives used by both REPL graphics words and retained widgets

Design constraints:

- Higher layers do not call `Arduino_GFX` directly
- `gfx_init()` is idempotent because boot vocabulary may invoke it after C-side setup
- Rotation is centralized so touch mapping and drawing agree on the same logical screen

### 4. Touch and GUI Runtime

Core files:

- [include/gui_runtime.h](/Users/paulbowler/Documents/Projects/FifthOS/include/gui_runtime.h:1)
- [src/gui_runtime.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/gui_runtime.cpp:1)

Responsibilities:

- Poll the FT3168 touch controller over I2C
- Convert raw touch points into logical screen coordinates
- Maintain style, node, and app pools
- Hit-test the retained tree
- Dispatch touch, swipe, and timer events
- Redraw the active app when dirty

Important implementation choices:

- Fixed-size pools are used for styles, nodes, and apps
- Nodes are organized as parent / first-child / next-sibling
- Dirty propagation is explicit and deterministic
- The runtime favors composition through node kind, style, and callbacks over class hierarchies

### 5. Boot-Loaded GUI Vocabulary

Core file:

- [include/gui_boot.h](/Users/paulbowler/Documents/Projects/FifthOS/include/gui_boot.h:1)

Responsibilities:

- Define Fifth-level colors, helper words, constants, and aliases
- Build demo data and styles
- Build the retained demo screens and app manager
- Expose higher-level convenience words such as `SCREEN.NEW`, `STYLE.MAKE`, `GUI.STATUS`, and `TOUCH.DEBUG`

This file is the main bridge between the native runtime and the user-level GUI vocabulary. It is intentionally plain Fifth so the higher-level system remains editable from the language side.

## GUI Layering

The graphics and GUI stack is layered as follows.

### Layer 0: Backend

Native `gfx_*` functions wrap the AMOLED driver. This layer knows about the board and the concrete display implementation.

### Layer 1: Primitive Fifth Words

Words such as `PIXEL`, `LINE`, `TEXT`, `STYLE.NEW`, `NODE.NEW`, and `APP.DRAW` expose backend and runtime operations to Fifth.

### Layer 2: Immediate-Mode Helpers

Words such as `CENTER-TEXT`, `STATUS-BAR`, `PANEL`, and `PROGRESS-BAR` provide convenience drawing logic without introducing retained state.

### Layer 3: Geometry and Style Conventions

Nodes use `x y w h` bounds. Styles carry foreground, background, border, padding, and font scale. These records remain deliberately small and integer-based.

### Layer 4: Retained Node Tree

Each node stores:

- parent linkage
- sibling / child linkage
- bounds
- flags
- style handle
- widget kind
- draw / event callbacks
- optional text, bind address, and chart data pointers

### Layer 5: Events

The runtime generates:

- touch down
- touch move
- touch up
- touch tap
- swipe left
- swipe right
- timer
- draw

Events bubble from the deepest hit node toward the root.

### Layer 6: App and Screen Manager

An app owns an ordered list of retained screens. Left and right swipes switch the active screen. The current implementation switches instantly rather than animating.

### Layer 7: User Vocabulary

The intended end state is a Fifth-native UI vocabulary where screens, panels, labels, gauges, and controls are built from words rather than hard-coded C++ logic. The current demo in `gui_boot.h` is the first concrete step in that direction.

## Boot Sequence

The GUI bootstrap is evaluated in phases to make failures diagnosable on-device:

1. helpers
2. state
3. callbacks
4. screens
5. build app

If a phase fails, startup can render a boot status message to the display before the system enters normal operation.

## Memory Model

The GUI runtime currently uses fixed pool sizes compiled into [src/gui_runtime.cpp](/Users/paulbowler/Documents/Projects/FifthOS/src/gui_runtime.cpp:1):

- styles: 16
- nodes: 64
- apps: 4

This keeps allocation deterministic and simple, which is appropriate for embedded control UIs. If the GUI vocabulary expands materially, these pool sizes should be revisited alongside documentation and examples.

## Current Widget Set

Built-in retained widgets currently include:

- label
- panel
- button
- status bar
- value display
- vertical gauge
- line chart
- alarm indicator

The system does not yet attempt to provide desktop-style windowing, layout engines, or a heavyweight theme system.

## Current Limitations

- The browser REPL and on-device GUI are both live, but there is no persistence layer for user programs yet
- The GUI runtime uses fixed pools rather than dynamic growth
- The current demo layout is hand-positioned
- Event handling is intentionally simple
- `PIN`, `TONE`, `DUTY`, `FREQ`, and `sendPacket` remain placeholders in the current native primitive table

## Design Rules Going Forward

When expanding FifthOS:

- Keep display hardware details behind `gfx_*`
- Prefer exposing capability to Fifth rather than hard-coding policy in C++
- Add new public words to the reference immediately
- Keep retained GUI structures simple and deterministic
- Avoid introducing heavyweight GUI frameworks that bypass the Fifth vocabulary

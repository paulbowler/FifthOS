# FifthOS Architecture

## Overview

FifthOS combines four major pieces:

1. An ESP32-hosted eForth-derived interpreter and VM
2. A browser-served REPL transport over Wi-Fi
3. A small native data/runtime service layer for time and scheduled callbacks
4. A portable display abstraction implemented in C/C++
5. A retained-mode GUI runtime exposed upward as Fifth words

The design intent is to keep board-specific and performance-sensitive work in a small C/C++ layer while moving the interesting UI composition and experimentation surface into Fifth itself.

## High-Level Data Flow

At boot:

1. `setup()` initializes the VM state.
2. Wi-Fi and mDNS come up through the network layer.
3. The dictionary is built, registering native primitives and constants.
4. The data runtime initializes time sync and the task scheduler.
5. The graphics and touch runtime is initialized.
6. The boot-loaded Fifth GUI vocabulary from [include/gui_boot.h](../include/gui_boot.h) is evaluated in phases.
7. The retained demo app is built and drawn.
8. The HTTP server accepts browser REPL commands.

At runtime:

1. Browser input is posted to the device as a command string.
2. FifthOS evaluates that string inside the live interpreter.
3. Output is returned to the browser REPL transcript.
4. The GUI runtime polls touch, generates events, marks nodes dirty, and redraws the active app only when required.

## Subsystem Layout

### 1. VM and Dictionary

Core files:

- [include/vm.h](../include/vm.h)
- [src/vm.cpp](../src/vm.cpp)
- [include/dictionary.h](../include/dictionary.h)
- [src/dictionary.cpp](../src/dictionary.cpp)
- [src/primitives.cpp](../src/primitives.cpp)
- [include/forth_runtime.h](../include/forth_runtime.h)
- [src/forth_runtime.cpp](../src/forth_runtime.cpp)

Responsibilities:

- Maintain the data stack, return stack, dictionary, and code space
- Register native words and constants
- Execute compiled words and evaluate incoming text buffers
- Bridge HTTP and GUI callbacks back into the interpreter
- Support compiler-scoped read-only input locals using `{ a b -- }` syntax for colon definitions

The VM remains the system center. The GUI layer is not a separate framework bolted on the side; it is another vocabulary and runtime surface available to the interpreter.

### 2. Network and REPL

Core files:

- [src/network.cpp](../src/network.cpp)
- [src/http.cpp](../src/http.cpp)
- [include/index_html.h](../include/index_html.h)
- [src/main.cpp](../src/main.cpp)

Responsibilities:

- Reconnect to saved Wi-Fi credentials from flash when available
- Start a local setup AP when no saved Wi-Fi exists or connection fails
- Expose a setup page for scan, connect, forget, and WPS actions
- Perform Wi-Fi scans on demand instead of continuously in the background
- Publish an mDNS hostname from `deviceName`
- Serve the browser REPL UI
- Accept REPL commands over HTTP and return textual output

The browser REPL is only a transport and terminal surface. The interpreter remains live on-device.

### 3. Data Runtime

Core files:

- [include/data_runtime.h](../include/data_runtime.h)
- [src/data_runtime.cpp](../src/data_runtime.cpp)

Responsibilities:

- Start SNTP/NTP time synchronization once station Wi-Fi is available
- Expose current wall-clock time and uptime as primitive services
- Maintain a small fixed pool of scheduled Fifth callback tasks
- Execute periodic or one-shot Fifth callbacks outside the GUI tree

This layer intentionally stays generic. It does not know about the watch UI; it only provides time and scheduling services that higher-level Fifth words can consume.

### 4. Display Backend

Core files:

- [include/display.h](../include/display.h)
- [src/display.cpp](../src/display.cpp)
- [include/gfx_backend.h](../include/gfx_backend.h)
- [src/gfx_backend.cpp](../src/gfx_backend.cpp)

Responsibilities:

- Initialize the RM67162 AMOLED through `Arduino_GFX`
- Hide all board-specific display details behind `gfx_*` functions
- Report logical screen dimensions and rotation
- Provide basic drawing primitives used by both REPL graphics words and retained widgets

Design constraints:

- Higher layers do not call `Arduino_GFX` directly
- `gfx_init()` is idempotent because boot vocabulary may invoke it after C-side setup
- Rotation is centralized so touch mapping and drawing agree on the same logical screen

### 5. Touch and GUI Runtime

Core files:

- [include/gui_runtime.h](../include/gui_runtime.h)
- [src/gui_runtime.cpp](../src/gui_runtime.cpp)

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
- Dirty marking is local to the node being updated; redraw walks dirty subtrees instead of forcing whole-screen invalidation
- The runtime favors composition through node kind, style, and callbacks over class hierarchies

### 6. Boot-Loaded GUI Vocabulary

Core file:

- [include/gui_boot.h](../include/gui_boot.h)

Responsibilities:

- Define Fifth-level colors, helper words, constants, and aliases
- Build app state, styles, generic UI shell words, and showcase screens
- Build the retained app manager and a watch-style top navigation bar
- Expose higher-level convenience words such as `SCREEN.NEW`, `STYLE.MAKE`, `FACE.LABEL`, `FACE.BOX`, `FACE.RULE`, and `TOUCH.DEBUG`
- Assemble the live clock face from separate retained `HH`, `MM`, `SS`, and info-line nodes so periodic updates repaint only the region that changed

This file is the main bridge between the native runtime and the user-level GUI vocabulary. It is intentionally plain Fifth so the higher-level system remains editable from the language side.

The boot vocabulary is currently in a transitional state:

- many generic helpers now use read-only locals for clarity
- some hot draw and scheduler paths still use the older `G0..G9` scratch globals deliberately
- the app bootstrap path also remains stack-based because it proved more stable than a broad locals conversion

That split is intentional. Locals are being adopted incrementally rather than forced into the most stateful runtime paths all at once.

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

An app owns an ordered list of retained screens. The current demo uses a top icon bar for direct navigation while the runtime still supports left and right swipe events. Screen switching is instant.

### Layer 7: User Vocabulary

The current boot vocabulary is deliberately layered:

- foundation words and constants
- generic UI shell and layout words
- app state and callbacks
- top-level screen builders

This keeps the distinction clear between low-level capability, reusable composition, and application-specific words.

## Locals Strategy

FifthOS now supports compiler-scoped read-only input locals:

```forth
: ADD2 { a b -- } a b + ;
```

This feature is intended to replace much of the old scratch-register style in GUI helper words, but not every word has been migrated yet.

Current guidance:

- use locals for pure helper words and screen composition words
- keep stack order explicit in event-heavy code
- be conservative in hot draw loops and scheduler paths

The current calendar month-grid renderer is a good example of this policy. Title/state helpers use locals cleanly, while the per-cell draw loop still uses the simpler scratch-register form because it has proven to be the most brittle path during migration.

## Boot Sequence

The GUI bootstrap is evaluated in phases to make failures diagnosable on-device:

1. foundation
2. ui kit
3. app state
4. screens
5. bootstrap

If a phase fails, startup can render a boot status message to the display before the system enters normal operation.

## Memory Model

The GUI runtime currently uses fixed pool sizes compiled into [src/gui_runtime.cpp](../src/gui_runtime.cpp):

- styles: 24
- nodes: 128
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
- 1-bit 16x16 icon widget

The system does not yet attempt to provide desktop-style windowing, layout engines, or a heavyweight theme system.

## Current Limitations

- The browser REPL and on-device GUI are both live, but there is no persistence layer for user programs yet
- The GUI runtime uses fixed pools rather than dynamic growth
- The current demo layout is hand-positioned in a watch-face style rather than using a general layout engine
- Event handling is intentionally simple
- `PIN`, `TONE`, `DUTY`, `FREQ`, and `sendPacket` remain placeholders in the current native primitive table

## Design Rules Going Forward

When expanding FifthOS:

- Keep display hardware details behind `gfx_*`
- Prefer exposing capability to Fifth rather than hard-coding policy in C++
- Add new public words to the reference immediately
- Keep retained GUI structures simple and deterministic
- Avoid introducing heavyweight GUI frameworks that bypass the Fifth vocabulary

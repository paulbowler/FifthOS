# FifthOS User Guide

## What FifthOS Is

FifthOS is a live Forth system running on an ESP32-S3 board. You interact with it through a browser REPL, while the board can also render its own retained GUI on the local AMOLED display.

The same live interpreter drives:

- normal REPL evaluation
- low-level graphics
- retained GUI construction
- touch event handlers

## Hardware Target

The active target in this repository is the Waveshare `ESP32-S3-AMOLED-1.91` board:

- ESP32-S3
- RM67162 `240x536` AMOLED display
- FT3168 touch controller
- USB serial / JTAG

## First-Time Setup

### 1. Create Wi-Fi Credentials

Copy [include/credentials_template.h](/Users/paulbowler/Documents/Projects/FifthOS/include/credentials_template.h:1) to `include/credentials.h`.

Edit:

- `ssid`
- `pass`

`include/credentials.h` is ignored by git and should remain local.

### 2. Build

```bash
pio run -e esp32-s3-amoled-191
```

### 3. Upload

```bash
pio run -e esp32-s3-amoled-191 -t upload
```

### 4. Monitor Serial Output

```bash
pio device monitor -b 115200
```

The serial output is useful for:

- Wi-Fi join confirmation
- IP address reporting
- boot diagnostics
- GUI bootstrap failures

## Connecting To The REPL

Once the board joins Wi-Fi, open:

```text
http://fifthos.local/
```

The browser UI is an inline terminal-style REPL. Commands are entered directly into the transcript and results appear inline.

## What Happens At Boot

Normal boot sequence:

1. VM initializes
2. Wi-Fi and HTTP server initialize
3. Fifth dictionary is built
4. Graphics and touch runtime initialize
5. GUI boot phases run
6. Demo app is built and drawn

If the GUI bootstrap fails, the display may show a boot error with the failing phase and the unknown word or other interpreter output.

## Everyday REPL Usage

Enter ordinary Forth/Fifth commands directly in the browser.

Examples:

```forth
1 2 + .
```

```forth
SCREEN.W . SCREEN.H .
```

```forth
TOUCH.DEBUG
```

Use the browser REPL for:

- trying graphics words
- inspecting node state
- rebuilding demo screens
- experimenting with styles and widget composition

## Display Graphics From The REPL

### Clear The Screen

```forth
BLACK CLS
```

### Draw A Pixel

```forth
120 200 GREEN PIXEL
```

### Draw A Line

```forth
0 0 SCREEN.W SCREEN.H RED LINE
```

### Draw A Rectangle

```forth
20 40 100 60 WHITE RECT
```

### Draw Filled Content

```forth
20 40 100 60 BLUE FILL-RECT
```

### Draw Text

```forth
40 80 $" FifthOS" COUNT WHITE TEXT
```

## Using The Touch System

### Poll The Latest Touch State

```forth
TOUCH.UPDATE .
```

This returns a flag indicating whether the touch state changed during polling.

### Print Touch Diagnostics

```forth
TOUCH.DEBUG
```

This prints:

- touch event type
- x
- y
- dx
- dy

### Swipe Behavior

The retained demo app responds to left and right swipes by switching between screens.

Current demo screens:

- telemetry
- alarms
- system

## Rebuilding The Demo GUI

The GUI demo is assembled from words defined in the boot vocabulary.

The top-level builder is:

```forth
FIFTHOS.GUI
```

This rebuilds styles, screens, and the app manager, then draws the active app.

## GUI Concepts For Users

You will work with four main concepts:

- styles
- nodes
- widgets
- apps and screens

### Styles

Styles define foreground, background, border, padding, and font scale.

### Nodes

Nodes are retained objects with bounds, flags, style, and optional callbacks or data bindings.

### Widgets

Built-in widgets are convenience constructors for common node kinds such as labels and gauges.

### Apps And Screens

An app is a screen manager. A screen is just a retained node acting as a root for a subtree.

## Minimal Retained GUI Session

The following pattern is typical:

1. Create a style
2. Create a screen node
3. Add widgets to the screen
4. Create an app
5. Add the screen to the app
6. Show and draw it

The exact words are listed in the reference guide:

- [forth-reference.md](/Users/paulbowler/Documents/Projects/FifthOS/docs/forth-reference.md:1)

## Browser REPL And On-Device GUI

The browser REPL and the local GUI are complementary:

- use the browser for typing, inspecting, and rapid experimentation
- use the device screen for the target embedded user experience

The GUI does not replace the REPL. FifthOS is designed around keeping both active.

## Troubleshooting

### The Board Does Not Join Wi-Fi

Check:

- `include/credentials.h`
- SSID spelling
- password
- serial output

### The Browser Page Does Not Load

Check:

- that the board has an IP address
- that `http://fifthos.local/` resolves on your network
- serial output for boot failures

### The GUI Screen Is Blank

Check:

- boot diagnostics on the device display
- serial output for bootstrap errors
- that `FIFTHOS.GUI` runs without an unknown-word error

### Touch Does Not Respond

Use:

```forth
TOUCH.DEBUG
```

If no values change, the problem is likely below the Fifth vocabulary, in touch polling or coordinate mapping.

### The GUI Flickers

The retained runtime redraws only when the active app is dirty. If flicker returns in future changes, inspect:

- dirty propagation
- timer handlers
- unconditional redraw paths

## Where To Look Next

For system structure, read [architecture.md](/Users/paulbowler/Documents/Projects/FifthOS/docs/architecture.md:1).

For the word-level interface, read [forth-reference.md](/Users/paulbowler/Documents/Projects/FifthOS/docs/forth-reference.md:1).

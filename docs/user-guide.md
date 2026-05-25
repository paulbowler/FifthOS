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

### 1. Build

```bash
pio run -e esp32-s3-amoled-191
```

### 2. Upload

```bash
pio run -e esp32-s3-amoled-191 -t upload
```

### 3. First Boot and Wi-Fi Setup

FifthOS no longer depends on compile-time Wi-Fi credentials. The board always boots locally.

If no saved Wi-Fi network is available, FifthOS starts a setup access point and shows:

- AP name
- Wi-Fi password
- REPL URL
- Wi-Fi setup URL

Join that AP from a phone or computer, then open the setup URL in a browser. You can then:

- scan for nearby Wi-Fi networks
- connect with SSID and password
- start WPS pairing
- forget the saved Wi-Fi network

Saved credentials are stored in flash and reused on later boots.

### 4. Monitor Serial Output

```bash
pio device monitor -b 115200
```

The serial output is useful for:

- Wi-Fi join confirmation
- setup AP credentials and URLs
- IP address reporting
- boot diagnostics
- GUI bootstrap failures

## Connecting To The REPL

If the board is connected to your normal Wi-Fi, open:

```text
http://fifthos.local/
```

If the board is running its own setup AP, open the REPL URL shown on the device, typically:

```text
http://192.168.4.1/
```

The browser UI is an inline terminal-style REPL. Commands are entered directly into the transcript and results appear inline.

## What Happens At Boot

Normal boot sequence:

1. VM initializes
2. Wi-Fi manager initializes and either reconnects or starts the setup AP
3. Fifth dictionary is built
4. Graphics and touch runtime initialize
5. GUI boot phases run
6. Demo app is built and drawn or the Wi-Fi setup screen is shown

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

```forth
TIME.SYNC? .
TIME.HMS . . .
WIFI.STATUS
```

Use the browser REPL for:

- trying graphics words
- inspecting node state
- rebuilding demo screens
- experimenting with styles and widget composition
- managing Wi-Fi from the live interpreter
- checking time sync and scheduled task behavior

### Wi-Fi Control From Fifth

The current Wi-Fi control words are:

```forth
WIFI.STATUS
WIFI.SCAN
WIFI.CONNECTED? .
WIFI.AP? .
WIFI.SAVED? .
```

Connect to a network manually:

```forth
$" MySSID" COUNT $" secretpass" COUNT WIFI.CONNECT .
```

Start WPS pairing:

```forth
WIFI.WPS .
```

Forget the saved network:

```forth
WIFI.FORGET .
```

### Time And Task Control From Fifth

The current time and scheduling words are:

```forth
MILLIS .
TIME.SYNC? .
TIME.UNIX .
TIME.HMS . . .
TIME.YMD . . .
TIME.WDAY .
```

The current scheduler words are:

```forth
TASK.NEW
TASK.EVERY
TASK.ONCE
TASK.START
TASK.STOP
TASK.ACTIVE?
```

Typical scheduling pattern:

```forth
TASK.NEW CONSTANT T0
' SOME-CALLBACK CONSTANT SOME-CALLBACK.XT
T0 1000 SOME-CALLBACK.XT TASK.EVERY
T0 TASK.START
```

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

### Navigation Behavior

The current showcase app uses a top icon bar for direct navigation between screens.

The runtime also supports touch gestures and can emit left and right swipe events, but the current watch-style demo is centered on tap navigation through the icon row.

Current showcase screens:

- clock
- calendar
- alarms
- weather

## Rebuilding The Demo GUI

The GUI demo is assembled from words defined in the boot vocabulary.

The top-level builder is:

```forth
FIFTHOS.GUI
```

This rebuilds styles, the generic UI shell, the showcase screens, and the app manager, then draws the active app.

The current demo is intentionally styled like a digital watch face:

- one dominant datum per screen
- compact supporting labels and metrics
- a top icon bar for screen switching
- minimal framing lines rather than large filled panels

The current calendar screen uses a mixed implementation on purpose:

- calendar title/state helpers use the newer locals syntax
- the per-day month-grid draw path still uses the older scratch-register form

That is not accidental. It is the current safe boundary for the locals migration.

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

Built-in widgets are convenience constructors for common node kinds such as labels, gauges, alarm indicators, and 1-bit icons.

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

- [forth-reference.md](forth-reference.md)

For a worked end-to-end example using the current high-level vocabulary, read:

- [app-tutorial.md](app-tutorial.md)

## Browser REPL And On-Device GUI

The browser REPL and the local GUI are complementary:

- use the browser for typing, inspecting, and rapid experimentation
- use the device screen for the target embedded user experience

## Locals In FifthOS

FifthOS now supports read-only input locals in colon definitions:

```forth
: ADD2 { a b -- } a b + ;
```

This is intended to reduce dependence on global scratch variables such as `G0..G9` in GUI and application words.

Current limitations:

- locals are currently input-only
- writable locals such as `TO name` are not implemented yet
- some event, scheduler, and hot draw words still intentionally use the older scratch-register style

Use locals first in pure helper words and screen composition words. Migrate stateful runtime words more cautiously.

The GUI does not replace the REPL. FifthOS is designed around keeping both active.

## Troubleshooting

### The Board Does Not Join Wi-Fi

Check:

- whether the board started its own setup AP instead
- serial output for the fallback AP credentials and URLs
- SSID spelling and password if you entered them manually
- whether WPS is actually enabled on the router if you are using WPS

### The Browser Page Does Not Load

Check:

- that the board has an IP address or setup AP address
- that `http://fifthos.local/` resolves on your network when station Wi-Fi is connected
- whether you should be using the setup AP URL instead
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

- whether one node update is unnecessarily dirtied as a larger subtree
- timer handlers
- any path that still calls a forced full redraw
- network setup/status transitions rather than the steady-state app loop

The live clock face is intentionally split into separate `HH`, `MM`, and `SS` widgets so the seconds tick does not repaint the full time row.

## Where To Look Next

For system structure, read [architecture.md](architecture.md).

For the word-level interface, read [forth-reference.md](forth-reference.md).

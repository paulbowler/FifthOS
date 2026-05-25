# FifthOS Forth Reference

This document covers FifthOS-specific words and constants added by this repository. It does not attempt to fully document the inherited upstream eForth core.

Stack effects are written in the form:

```text
( before -- after )
```

## Notes On Address Arguments

Words that accept `addr len` for text expect the address to refer to Fifth string storage in the VM data area, as produced by patterns such as:

```forth
$" Hello" COUNT
```

Words that accept a `value-addr` or `points-addr` expect a cell address in the Fifth data space.

## Locals

FifthOS now supports read-only input locals in colon definitions:

```forth
: ADD2 { a b -- } a b + ;
```

This feature is compiler-scoped and currently supports:

- input locals only
- read-only access only
- use inside ordinary colon definitions

It does not yet provide writable locals such as `TO name`.

The GUI boot vocabulary uses a mixed style at present:

- many helper and composition words have been migrated to locals
- some scheduler, event, bootstrap, and hot draw words still deliberately use `G0..G9`

That split reflects the current safe migration boundary, not a documentation omission.

## Color Constants

Boot-loaded in [include/gui_boot.h](../include/gui_boot.h):

- `BLACK`
- `WHITE`
- `RED`
- `GREEN`
- `BLUE`
- `ORANGE`
- `GRAY`
- `DARK-GRAY`

These are 16-bit RGB565 values.

## Platform And Utility Extensions

### `sendPacket`

```text
( ... -- ... )
```

Declared but currently a stub in the native primitive table.

### `POKE`

```text
( value addr -- )
```

Writes a machine word to a raw address.

### `PEEK`

```text
( addr -- value )
```

Reads a machine word from a raw address.

### `ADC`

```text
( pin -- value )
```

Reads an analog value using `analogRead`.

### `PIN`

```text
( ... -- ... )
```

Declared in the dictionary but currently not implemented in the native primitive table.

### `TONE`

```text
( ... -- ... )
```

Declared in the dictionary but currently not implemented in the native primitive table.

### `DUTY`

```text
( ... -- ... )
```

Declared in the dictionary but currently not implemented in the native primitive table.

### `FREQ`

```text
( ... -- ... )
```

Declared in the dictionary but currently not implemented in the native primitive table.

## Wi-Fi Words

### `WIFI.STATUS`

```text
( -- )
```

Prints the current Wi-Fi state, including saved/station/AP flags, current SSID or AP details, and the REPL and setup URLs.

### `WIFI.SCAN`

```text
( -- )
```

Triggers a scan immediately and prints the visible networks with RSSI and security state.

### `WIFI.CONNECT`

```text
( ssid-addr ssid-len pass-addr pass-len -- flag )
```

Starts a station connection using the supplied SSID and password and saves that network for later boots. Returns non-zero if the request was accepted.

Typical usage:

```forth
$" MySSID" COUNT $" secretpass" COUNT WIFI.CONNECT .
```

### `WIFI.WPS`

```text
( -- flag )
```

Starts WPS push-button pairing. Returns non-zero if WPS was started.

### `WIFI.FORGET`

```text
( -- flag )
```

Clears the saved Wi-Fi credentials from flash. Returns non-zero on success.

### `WIFI.CONNECTED?`

```text
( -- flag )
```

Returns non-zero when the board is connected to an external Wi-Fi network.

### `WIFI.AP?`

```text
( -- flag )
```

Returns non-zero when the local setup AP is active.

### `WIFI.SAVED?`

```text
( -- flag )
```

Returns non-zero when saved Wi-Fi credentials exist in flash.

## Time And Task Words

### `MILLIS`

```text
( -- ms )
```

Returns the current millisecond uptime from the ESP32 runtime.

### `TIME.SYNC?`

```text
( -- flag )
```

Returns non-zero when the system clock has been synchronized to a valid wall-clock time.

### `TIME.UNIX`

```text
( -- unix-seconds )
```

Returns the current Unix timestamp. Returns `0` when time has not been synchronized yet.

### `TIME.HMS`

```text
( -- h m s )
```

Returns local time fields for hour, minute, and second.

### `TIME.YMD`

```text
( -- y m d )
```

Returns local date fields for year, month, and day.

### `TIME.WDAY`

```text
( -- wday )
```

Returns the local weekday as `0..6`, where `0` is Sunday.

### `TASK.NEW`

```text
( -- task )
```

Allocates a task slot from the fixed scheduler pool. Returns `0` if no slots remain.

### `TASK.EVERY`

```text
( task ms xt -- )
```

Configures `task` to run `xt` repeatedly every `ms` milliseconds.

### `TASK.ONCE`

```text
( task ms xt -- )
```

Configures `task` to run `xt` once after `ms` milliseconds.

### `TASK.START`

```text
( task -- )
```

Starts a configured task.

### `TASK.STOP`

```text
( task -- )
```

Stops a configured task.

### `TASK.ACTIVE?`

```text
( task -- flag )
```

Returns non-zero when the task is currently active.

## Graphics Backend Words

### `GFX.INIT`

```text
( -- )
```

Initializes the display backend. Safe to call more than once.

### `SCREEN.W`

```text
( -- w )
```

Returns the logical screen width in the current rotation.

### `SCREEN.H`

```text
( -- h )
```

Returns the logical screen height in the current rotation.

### `CLS`

```text
( color -- )
```

Clears the full display to `color`.

### `PIXEL`

```text
( x y color -- )
```

Draws one pixel.

### `LINE`

```text
( x1 y1 x2 y2 color -- )
```

Draws a line.

### `RECT`

```text
( x y w h color -- )
```

Draws an outline rectangle.

### `FILL-RECT`

```text
( x y w h color -- )
```

Draws a filled rectangle.

### `TEXT`

```text
( x y addr len color -- )
```

Draws a text span.

### `TEXT-W`

```text
( addr len -- width )
```

Returns the rendered text width in pixels for the current text scale.

### `ROTATION`

```text
( rotation -- )
```

Sets display rotation. Valid values are `0..3`.

### `BITMAP1`

```text
( x y w h scale color addr -- )
```

Draws a 1-bit bitmap from row-mask data stored in Fifth data space.

## Touch And Event Words

### `TOUCH.UPDATE`

```text
( -- flag )
```

Polls touch hardware and updates the current event state. Returns non-zero if the state changed.

### `TOUCH.TYPE`

```text
( -- type )
```

Returns the current event type.

### `TOUCH.X`

```text
( -- x )
```

Returns the current touch x coordinate.

### `TOUCH.Y`

```text
( -- y )
```

Returns the current touch y coordinate.

### `TOUCH.DX`

```text
( -- dx )
```

Returns delta x for the current event.

### `TOUCH.DY`

```text
( -- dy )
```

Returns delta y for the current event.

### `TOUCH.TICK`

```text
( -- tick )
```

Returns the event timestamp in milliseconds.

### `TOUCH.TARGET`

```text
( -- node )
```

Returns the current event target node, or `0`.

### `SWIPE.DETECT`

```text
( -- type|0 )
```

Returns the current swipe event type or `0`.

### Event Constants

- `TOUCH.DOWN`
- `TOUCH.MOVE`
- `TOUCH.UP`
- `TOUCH.TAP`
- `TOUCH.SWIPE.LEFT`
- `TOUCH.SWIPE.RIGHT`
- `TIMER`
- `DRAW`

### `EVENT.DISPATCH`

```text
( node event -- )
```

Dispatches an event starting at `node`.

### `EVENT.BUBBLE`

```text
( node event -- )
```

Currently equivalent to `EVENT.DISPATCH`.

## Style Words

### `STYLE.NEW`

```text
( -- style )
```

Allocates a new style handle from the fixed style pool.

### `STYLE.SET`

```text
( fg bg border padding font style -- )
```

Assigns all standard style fields.

### `STYLE.FIELD@`

```text
( style field -- value )
```

Reads one style field.

### `STYLE.FIELD!`

```text
( value style field -- )
```

Writes one style field.

### Style Field Constants

- `STYLE>FG`
- `STYLE>BG`
- `STYLE>BORDER`
- `STYLE>PADDING`
- `STYLE>FONT`

### Helper Word

### `STYLE.MAKE`

```text
( fg bg border padding font -- style )
```

Boot-loaded helper that allocates a style, assigns all fields, and returns the style handle.

## Node Words

### `NODE.NEW`

```text
( -- node )
```

Allocates a new retained node.

### `NODE.ADD`

```text
( parent child -- )
```

Adds `child` to `parent`.

### `NODE.REMOVE`

```text
( child -- )
```

Detaches `child` from its parent.

### `NODE.DIRTY`

```text
( node -- )
```

Marks `node` dirty. The runtime redraw pass then walks dirty subtrees from the active screen root.

### `NODE.DRAW`

```text
( node -- )
```

Draws one node.

### `NODE.DRAW-TREE`

```text
( node -- )
```

Draws the full subtree rooted at `node`.

### `NODE.HIT`

```text
( node x y -- hit|0 )
```

Returns the deepest touchable visible node at `x y`, or `0`.

### `NODE.SCREEN-XY`

```text
( node -- x y )
```

Returns the absolute screen coordinates of the node origin.

### `NODE.SET-BOUNDS`

```text
( x y w h node -- )
```

Assigns the node bounds.

### `NODE.VISIBLE?`

```text
( node -- flag )
```

Returns non-zero if the node is visible.

### `NODE.TOUCHABLE?`

```text
( node -- flag )
```

Returns non-zero if the node is touchable.

### `NODE.FIELD@`

```text
( node field -- value )
```

Reads one node field.

### `NODE.FIELD!`

```text
( value node field -- )
```

Writes one node field and marks the node dirty.

### Node Field Constants

- `NODE>PARENT`
- `NODE>FIRST`
- `NODE>NEXT`
- `NODE>X`
- `NODE>Y`
- `NODE>W`
- `NODE>H`
- `NODE>FLAGS`
- `NODE>DRAW`
- `NODE>EVENT`
- `NODE>STYLE`
- `NODE>USER`
- `NODE>KIND`
- `NODE>TEXT`
- `NODE>TEXTLEN`
- `NODE>BIND`
- `NODE>MIN`
- `NODE>MAX`
- `NODE>POINTS`
- `NODE>COUNT`

### Node Flag Constants

- `NODE.VISIBLE`
- `NODE.DIRTY.FLAG`
- `NODE.TOUCHABLE`
- `NODE.FOCUSABLE`

## Screen And App Words

### `SCREEN.NEW`

```text
( style -- screen )
```

Boot-loaded helper that creates a full-screen root node using the current `SCREEN.W` and `SCREEN.H`.

### `NODE.DRAW!`

```text
( xt node -- )
```

Boot-loaded helper that stores a draw callback execution token into a node.

### `NODE.EVENT!`

```text
( xt node -- )
```

Boot-loaded helper that stores an event callback execution token into a node.

### `NODE.STYLE!`

```text
( style node -- )
```

Boot-loaded helper that stores a style handle into a node.

### `APP.NEW`

```text
( -- app )
```

Allocates a new app manager.

### `APP.ADD-SCREEN`

```text
( app screen -- )
```

Appends a screen to an app.

### `APP.NEXT`

```text
( app -- )
```

Shows the next screen, wrapping around.

### `APP.PREV`

```text
( app -- )
```

Shows the previous screen, wrapping around.

### `APP.SHOW`

```text
( app index -- )
```

Shows the screen at `index`.

### `APP.ACTIVE`

```text
( app -- screen|0 )
```

Returns the active screen node for the app.

### `APP.DRAW`

```text
( app -- )
```

Draws the active screen for the app.

## Built-In Widget Constructor Words

These are native retained-widget constructors. Each returns a node handle.

### `W.PANEL`

```text
( parent x y w h style -- node )
```

### `W.LABEL`

```text
( parent x y w h style addr len -- node )
```

### `W.BUTTON`

```text
( parent x y w h style addr len xt -- node )
```

`xt` is the event handler execution token.

### `W.STATUS`

```text
( parent x y w h style addr len -- node )
```

### `W.VALUE`

```text
( parent x y w h style addr len value-addr -- node )
```

Displays a label plus the numeric value stored at `value-addr`.

### `W.GAUGE`

```text
( parent x y w h style value-addr min max -- node )
```

Creates a vertical bar gauge.

### `W.CHART`

```text
( parent x y w h style points-addr count min max -- node )
```

Creates a simple line chart.

### `W.ALARM`

```text
( parent x y w h style addr len value-addr -- node )
```

Displays alarm text using `value-addr` to determine active state.

### `W.ICON16`

```text
( parent x y style bitmap-addr scale -- node )
```

Creates a retained 16x16 1-bit icon widget.

## Boot Vocabulary Widget Aliases

The following words currently alias the native widget constructors directly:

- `GUI.PANEL`
- `GUI.LABEL`
- `GUI.BUTTON`
- `GUI.STATUS`
- `GUI.VALUE`
- `GUI.GAUGE`
- `GUI.CHART`
- `GUI.ALARM`
- `GUI.ICON16`
- `ICON.1BIT`

## Generic UI Shell Words

These words are boot-loaded in the current showcase app and sit above the primitive widget constructors.

### `STYLE.FG`

```text
( style -- color )
```

### `STYLE.BG`

```text
( style -- color )
```

### `STYLE.BORDER`

```text
( style -- color )
```

### `DRAW.NODE`

```text
( parent x y w h style xt -- node )
```

Creates a retained node with explicit bounds, style, and draw callback.

### `FACE.X`

```text
( -- x )
```

Returns the left edge of the content area.

### `FACE.W`

```text
( -- w )
```

Returns the content width after outer margins.

### `FACE.H`

```text
( -- h )
```

Returns the full logical screen height.

### `CONTENT.Y`

```text
( -- y )
```

Constant marking the top edge of the main watch-face content area below the icon row.

### `CONTENT.H`

```text
( -- h )
```

Returns the content height below `CONTENT.Y`.

### `HALF.W`

```text
( -- w )
```

Returns the width of one half-width content region.

### `RIGHT.X`

```text
( -- x )
```

Returns the relative x offset for the right-hand half content region.

### `FACE.PANEL`

```text
( screen x y w h style -- node )
```

Creates a retained panel using content-relative coordinates.

### `FACE.LABEL`

```text
( screen x y w h style addr len -- node )
```

Creates a label using content-relative coordinates.

### `FACE.BUTTON`

```text
( screen x y w h style addr len xt -- node )
```

Creates a button using content-relative coordinates.

### `FACE.VALUE`

```text
( screen x y w h style addr len value-addr -- node )
```

Creates a value display using content-relative coordinates.

### `FACE.GAUGE`

```text
( screen x y w h style value-addr min max -- node )
```

Creates a vertical gauge using content-relative coordinates.

### `FACE.CHART`

```text
( screen x y w h style points-addr count min max -- node )
```

Creates a chart using content-relative coordinates.

### `FACE.ALARM`

```text
( screen x y w h style addr len value-addr -- node )
```

Creates an alarm indicator using content-relative coordinates.

### `FACE.ICON`

```text
( screen x y style bitmap-addr scale -- node )
```

Creates a retained icon using content-relative coordinates.

### `SCREEN.TITLE`

```text
( screen addr len -- )
```

Currently a no-op in the watch-style demo shell.

### `SCREEN.SUBTITLE`

```text
( screen addr len -- )
```

Currently a no-op in the watch-style demo shell.

### `NAV.STYLE`

```text
( index selected -- style )
```

Returns the active or idle navigation style for a navigation item.

### `BUILD.SHELL`

```text
( screen selected title-addr title-len sub-addr sub-len -- )
```

Currently a placeholder for shared shell setup in the boot vocabulary.

### `FACE.RULE`

```text
( screen y -- )
```

Creates a thin full-width accent rule at relative y.

### `FACE.BOX`

```text
( screen x y w h border -- node )
```

Creates a black-filled, border-outlined panel.

### `FACE.INVERT`

```text
( screen x y w h addr len fg -- node )
```

Creates an inverted panel with black text on a colored fill.

### Icon Asset Helpers

The current boot vocabulary includes these bitmap assets and draw helpers:

- `ICON.RAIN.DATA`
- `ICON.CLOCK.DATA`
- `ICON.CALENDAR.DATA`
- `ICON.ALARM.DATA`
- `ICON.RAIN`
- `ICON.CLOCK`
- `ICON.CALENDAR`
- `ICON.ALARM`

## Immediate-Mode Helper Words

Defined in [include/gui_boot.h](../include/gui_boot.h).

### `CENTER-TEXT`

```text
( x y w addr len color -- )
```

Draws centered text inside a width.

### `RIGHT-TEXT`

```text
( x y w addr len color -- )
```

Draws right-aligned text inside a width.

### `BORDER`

```text
( x y w h color -- )
```

Alias of `RECT`.

### `PANEL`

```text
( x y w h fill border -- )
```

Fills a rectangle and then outlines it.

### `LABEL`

```text
( x y addr len color -- )
```

Alias of `TEXT`.

### `STATUS-BAR`

```text
( x y w h addr len fg bg -- )
```

Draws a filled bar and centered label.

### `PROGRESS-BAR`

```text
( x y w h value max fg bg -- )
```

Draws a simple filled horizontal progress bar.

### `HLINE`

```text
( x y w color -- )
```

Draws a horizontal line.

### `VLINE`

```text
( x y h color -- )
```

Draws a vertical line.

### `CLEAR-REGION`

```text
( x y w h color -- )
```

Alias of `FILL-RECT`.

### `INSET-RECT`

```text
( x y w h inset -- x' y' w' h' )
```

Returns inset bounds.

## Demo And Diagnostic Words

### `TOUCH.DEBUG`

```text
( -- )
```

Polls touch state and prints event diagnostics.

### `NAV.TAP?`

```text
( event target tick x y -- flag )
```

Returns non-zero when the incoming callback event is a tap.

### `SHOW.CLOCK`

```text
( event target tick x y -- )
```

Shows the clock screen on tap.

### `SHOW.CALENDAR`

```text
( event target tick x y -- )
```

Shows the calendar screen on tap.

### `SHOW.ALARMS`

```text
( event target tick x y -- )
```

Shows the alarms screen on tap.

### `SHOW.WEATHER`

```text
( event target tick x y -- )
```

Shows the weather screen on tap.

### `TOGGLE.ALARM`

```text
( event target tick x y -- )
```

Demo callback that toggles the alarm state.

### `DEMO.TICK`

```text
( node event x y dx -- )
```

Demo callback that advances dummy data for the showcase clock and weather screens on timer events.

Note: `DEMO.TICK` remains on the older scratch-register style and is one of the words not yet migrated to locals.

### `BUILD.STYLES`

```text
( -- )
```

Builds the current watch-face demo style set, including monochrome navigation icon styles and the blue content palette.

### `ADD.NAV`

```text
( screen selected -- )
```

Placeholder retained for compatibility with older boot scripts.

### `ADD.NAV.CLOCK`

```text
( screen -- )
```

Builds the top icon row with the clock icon selected.

### `ADD.NAV.CALENDAR`

```text
( screen -- )
```

Builds the top icon row with the calendar icon selected.

### `ADD.NAV.ALARMS`

```text
( screen -- )
```

Builds the top icon row with the alarms icon selected.

### `ADD.NAV.WEATHER`

```text
( screen -- )
```

Builds the top icon row with the weather icon selected.

### `ADD.WEATHER.BADGE`

```text
( screen -- )
```

Adds the small weather icon badge used on the clock screen.

### `BUILD.CLOCK`

```text
( -- )
```

Builds the current clock watch-face screen. The live time row is split into independent retained `HH`, `MM`, and `SS` nodes plus a separate info line, so normal ticking updates only the region that changed.

### `BUILD.CALENDAR`

```text
( -- )
```

Builds the current calendar watch-face screen.

Important implementation note: the calendar screen currently uses locals for title/state helpers, but the per-day month-grid renderer still uses the simpler scratch-register path. This is intentional and reflects the current safe locals migration boundary.

### `BUILD.ALARMS`

```text
( -- )
```

Builds the current alarms watch-face screen.

### `BUILD.WEATHER`

```text
( -- )
```

Builds the current weather watch-face screen.

### `BUILD.APP`

```text
( -- )
```

Creates the showcase app and attaches the clock, calendar, alarms, and weather screens.

Note: the app bootstrap words remain stack-based today. A broader locals conversion of the app assembly path was attempted and then rolled back because it destabilized active-screen selection.

### `FIFTHOS.GUI`

```text
( -- )
```

Builds and draws the complete showcase app shell and screens.

## Example Session

```forth
BLACK CLS
40 40 $" FifthOS" COUNT WHITE TEXT
STYLE.NEW CONSTANT S0
WHITE BLACK GRAY 4 1 S0 STYLE.SET
S0 SCREEN.NEW CONSTANT ROOT
ROOT 0 0 SCREEN.W 28 S0 $" Demo" COUNT GUI.STATUS DROP
APP.NEW CONSTANT APP0
APP0 ROOT APP.ADD-SCREEN
APP0 0 APP.SHOW
APP0 APP.DRAW
```

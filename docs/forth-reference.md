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

## Color Constants

Boot-loaded in [include/gui_boot.h](/Users/paulbowler/Documents/Projects/FifthOS/include/gui_boot.h:1):

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

Marks `node` and its ancestors dirty.

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

## Immediate-Mode Helper Words

Defined in [include/gui_boot.h](/Users/paulbowler/Documents/Projects/FifthOS/include/gui_boot.h:1).

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

### `TOGGLE.ALARM`

```text
( node event x y dx -- )
```

Demo callback that toggles the alarm state.

### `DEMO.TICK`

```text
( node event x y dx -- )
```

Demo callback that updates telemetry values on timer events.

### `BUILD.STYLES`

```text
( -- )
```

Builds the demo style set.

### `BUILD.MAIN`

```text
( -- )
```

Builds the telemetry screen.

### `BUILD.ALARMS`

```text
( -- )
```

Builds the alarms screen.

### `BUILD.SYSTEM`

```text
( -- )
```

Builds the system screen.

### `BUILD.APP`

```text
( -- )
```

Creates the demo app and attaches the demo screens.

### `FIFTHOS.GUI`

```text
( -- )
```

Builds and draws the complete demo app.

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

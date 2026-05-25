# FifthOS App Tutorial

This tutorial builds a complete retained GUI app in FifthOS using the current high-level vocabulary from [include/gui_boot.h](../include/gui_boot.h).

The goal is not to clone the built-in demo exactly. The goal is to show the intended layering:

1. Define colors, styles, and shared layout helpers
2. Define small reusable assets such as icons
3. Build screen constructors from high-level retained words
4. Assemble screens into an app
5. Draw the app and switch screens

## Prerequisites

The tutorial assumes the firmware is already running and the boot vocabulary is loaded.

Useful checks:

```forth
SCREEN.W . SCREEN.H .
```

```forth
TOUCH.DEBUG
```

If the built-in demo is currently active and you want to rebuild it after experimenting:

```forth
FIFTHOS.GUI
```

## Vocabulary Layers

The current high-level GUI work is best thought of in four layers.

### 1. Primitive and runtime words

Examples:

- `SCREEN.W`
- `SCREEN.H`
- `TEXT`
- `BITMAP1`
- `STYLE.NEW`
- `NODE.NEW`
- `APP.NEW`

These are the low-level surfaces exposed from C/C++.

### 2. Generic retained helpers

Examples:

- `SCREEN.NEW`
- `STYLE.MAKE`
- `FACE.PANEL`
- `FACE.LABEL`
- `FACE.BUTTON`
- `FACE.VALUE`
- `FACE.GAUGE`
- `FACE.CHART`
- `FACE.ALARM`
- `FACE.ICON`

These are the words you should usually build application screens with.

### 3. Layout helpers

Examples:

- `FACE.W`
- `FACE.H`
- `CONTENT.Y`
- `CONTENT.H`
- `HALF.W`
- `RIGHT.X`
- `FACE.RULE`
- `FACE.BOX`

These keep screen definitions readable and consistent.

### 4. Application words

Examples:

- `BUILD.HOME`
- `BUILD.SENSORS`
- `BUILD.ALERTS`
- `BUILD.MYAPP`

These should describe your actual app, not generic UI capability.

## Step 1: Define App State

Create a few variables and simple data buffers. Keep the data model small and explicit.

```forth
VARIABLE APP.DEMO
VARIABLE SCREEN.HOME
VARIABLE SCREEN.SENSORS
VARIABLE SCREEN.ALERTS

VARIABLE BATTERY.PCT
VARIABLE TEMP.C
VARIABLE HUMIDITY.PCT
VARIABLE ALARM.STATE

78 BATTERY.PCT !
21 TEMP.C !
67 HUMIDITY.PCT !
1 ALARM.STATE !

CREATE TEMP.TREND 8 CELLS ALLOT
18 TEMP.TREND !
19 TEMP.TREND CELL+ !
20 TEMP.TREND CELL+ CELL+ !
21 TEMP.TREND 3 CELLS + !
22 TEMP.TREND 4 CELLS + !
23 TEMP.TREND 5 CELLS + !
22 TEMP.TREND 6 CELLS + !
21 TEMP.TREND 7 CELLS + !
```

## Step 2: Define Styles

Use `STYLE.MAKE` for almost all app-level work. It keeps the screen builders compact.

```forth
VARIABLE S.ROOT
VARIABLE S.TITLE
VARIABLE S.SUB
VARIABLE S.CARD
VARIABLE S.HERO
VARIABLE S.MUTED
VARIABLE S.ACCENT
VARIABLE S.BUTTON
VARIABLE S.ALERT
VARIABLE S.NAV.IDLE
VARIABLE S.NAV.ACTIVE

: APP.STYLES
  ICE BLACK BLACK 0 1 STYLE.MAKE S.ROOT !
  ICE BLACK BLACK 0 2 STYLE.MAKE S.TITLE !
  ICE-DIM BLACK BLACK 0 1 STYLE.MAKE S.SUB !
  ICE-DIM BLACK ICE-DIM 2 1 STYLE.MAKE S.CARD !
  ICE BLACK BLACK 0 5 STYLE.MAKE S.HERO !
  ICE-LOW BLACK BLACK 0 1 STYLE.MAKE S.MUTED !
  ICE BLACK ICE 2 1 STYLE.MAKE S.ACCENT !
  BLACK ICE ICE 2 1 STYLE.MAKE S.BUTTON !
  ORANGE BLACK ORANGE 2 1 STYLE.MAKE S.ALERT !
  GRAY BLACK BLACK 0 1 STYLE.MAKE S.NAV.IDLE !
  WHITE BLACK BLACK 0 1 STYLE.MAKE S.NAV.ACTIVE ! ;
```

## Step 3: Define Icons

Icons are stored as 1-bit row masks. Keep them data-only and separate from screen builders.

```forth
CREATE ICON.HOME.DATA
HEX
0180 , 03C0 , 07E0 , 0FF0 ,
1FF8 , 39CC , 718E , 6186 ,
7FFE , 7FFE , 6186 , 6186 ,
6186 , 0000 , 0000 , 0000 ,
DECIMAL

CREATE ICON.SENSOR.DATA
HEX
03C0 , 0420 , 0810 , 0810 ,
0990 , 0990 , 0810 , 0810 ,
0990 , 0990 , 0810 , 0810 ,
0420 , 03C0 , 0000 , 0000 ,
DECIMAL

CREATE ICON.ALERT.DATA
HEX
0180 , 03C0 , 07E0 , 07E0 ,
07E0 , 07E0 , 07E0 , 03C0 ,
03C0 , 03C0 , 03C0 , 0000 ,
03C0 , 0000 , 0000 , 0000 ,
DECIMAL
```

To draw one immediately for testing:

```forth
20 20 16 16 2 WHITE ICON.HOME.DATA ICON.1BIT
```

To use one as a retained widget:

```forth
SCREEN.HOME @ 96 6 S.NAV.ACTIVE @ ICON.HOME.DATA 2 FACE.ICON DROP
```

## Step 4: Define Shared Shell Helpers

These are app-specific helpers that sit above the generic `FACE.*` layer.

```forth
: APP.RULE
  FACE.RULE ;

: APP.METRIC
  G7 ! G6 ! G5 ! G4 ! G3 ! G2 ! G1 ! G0 !
  G0 @ G1 @ G2 @ G3 @ G4 @ FACE.BOX DROP
  G0 @ G1 @ 8 +  G2 @ 10 STYLE.MUTED @ G5 @ G6 @ FACE.LABEL DROP
  G0 @ G1 @ 18 + G2 @ 16 STYLE.TITLE @ G7 @ G3 @ FACE.LABEL DROP ;
```

The stack effect for `APP.METRIC` is:

```text
( screen x y w h label-addr label-len value-addr value-len -- )
```

That is the kind of word you should create for your own app. It is application-friendly, but still generic enough to reuse inside multiple screens.

## Step 5: Define Navigation Builders

The current built-in demo uses separate words for each selected top-nav state. That pattern is simple and effective.

```forth
: APP.NAV.HOME
  G0 !
  G0 @ 96  6 S.NAV.ACTIVE @ ICON.HOME.DATA   2 FACE.ICON DROP
  G0 @ 166 6 S.NAV.IDLE   @ ICON.SENSOR.DATA 2 FACE.ICON DROP
  G0 @ 236 6 S.NAV.IDLE   @ ICON.ALERT.DATA  2 FACE.ICON DROP ;

: APP.NAV.SENSORS
  G0 !
  G0 @ 96  6 S.NAV.IDLE   @ ICON.HOME.DATA   2 FACE.ICON DROP
  G0 @ 166 6 S.NAV.ACTIVE @ ICON.SENSOR.DATA 2 FACE.ICON DROP
  G0 @ 236 6 S.NAV.IDLE   @ ICON.ALERT.DATA  2 FACE.ICON DROP ;

: APP.NAV.ALERTS
  G0 !
  G0 @ 96  6 S.NAV.IDLE   @ ICON.HOME.DATA   2 FACE.ICON DROP
  G0 @ 166 6 S.NAV.IDLE   @ ICON.SENSOR.DATA 2 FACE.ICON DROP
  G0 @ 236 6 S.NAV.ACTIVE @ ICON.ALERT.DATA  2 FACE.ICON DROP ;
```

## Step 6: Define Event Helpers

Keep event logic small. Let screens handle taps and let widgets do the rest.

```forth
: NAV.TAP?
  G4 ! G3 ! G2 ! G1 ! G0 !
  G1 @ TOUCH.TAP = ;

: APP.NAV.TAP
  NAV.TAP? IF
    G3 @ 40 < IF
      G2 @ 96  < 0= G2 @ 128 < AND IF APP.DEMO @ 0 APP.SHOW THEN
      G2 @ 166 < 0= G2 @ 198 < AND IF APP.DEMO @ 1 APP.SHOW THEN
      G2 @ 236 < 0= G2 @ 268 < AND IF APP.DEMO @ 2 APP.SHOW THEN
    THEN
  THEN ;
' APP.NAV.TAP CONSTANT APP.NAV.TAP.XT
```

This shows the pattern for root-level screen events:

- event filtering
- coordinate testing
- app-level screen switching

For new app code, prefer explicit locals-based event signatures where practical:

```forth
: APP.NAV.TAP { event target tick x y -- }
  event TOUCH.TAP = IF
    y 40 < IF
      ...
    THEN
  THEN ;
```

The FifthOS demo still keeps a few hot event and draw paths on the older scratch-register style where that has proven safer during incremental migration.

## Step 7: Build Screens

This is where the watch-style layout matters. Each screen should have:

- one dominant reading
- one or two supporting labels
- a small number of subordinate metrics

### Home Screen

```forth
: BUILD.HOME
  S.ROOT @ SCREEN.NEW DUP SCREEN.HOME !
  APP.NAV.TAP.XT SCREEN.HOME @ NODE.EVENT!
  SCREEN.HOME @ APP.NAV.HOME
  SCREEN.HOME @ 40 APP.RULE
  SCREEN.HOME @ 8 54 76 12 S.MUTED @ $" STATUS" COUNT FACE.LABEL DROP
  SCREEN.HOME @ 8 74 220 72 S.HERO @ $" 10:58" COUNT FACE.LABEL DROP
  SCREEN.HOME @ 296 58 132 14 S.TITLE @ $" CLEAR" COUNT FACE.LABEL DROP
  SCREEN.HOME @ 296 78 132 12 S.SUB @ $" BATT 78%" COUNT FACE.LABEL DROP
  SCREEN.HOME @ 296 96 132 12 S.SUB @ $" ALARM 06:30" COUNT FACE.LABEL DROP
  SCREEN.HOME @ 0 178 FACE.W 1 S.ACCENT @ FACE.PANEL DROP
  SCREEN.HOME @ 0 188 144 34 S.CARD @ FACE.BOX DROP
  SCREEN.HOME @ 10 194 90 10 S.MUTED @ $" TEMP" COUNT FACE.LABEL DROP
  SCREEN.HOME @ 12 204 80 16 S.TITLE @ $" 21C" COUNT FACE.LABEL DROP
  SCREEN.HOME @ 156 188 144 34 S.CARD @ FACE.BOX DROP
  SCREEN.HOME @ 166 194 90 10 S.MUTED @ $" HUMID" COUNT FACE.LABEL DROP
  SCREEN.HOME @ 168 204 80 16 S.TITLE @ $" 67" COUNT FACE.LABEL DROP
  SCREEN.HOME @ 312 188 144 34 S.CARD @ FACE.BOX DROP
  SCREEN.HOME @ 322 194 90 10 S.MUTED @ $" BATT" COUNT FACE.LABEL DROP
  SCREEN.HOME @ 324 204 80 16 S.TITLE @ $" 78" COUNT FACE.LABEL DROP ;
```

### Sensors Screen

This screen shows `FACE.GAUGE` and `FACE.CHART`.

```forth
: BUILD.SENSORS
  S.ROOT @ SCREEN.NEW DUP SCREEN.SENSORS !
  APP.NAV.TAP.XT SCREEN.SENSORS @ NODE.EVENT!
  SCREEN.SENSORS @ APP.NAV.SENSORS
  SCREEN.SENSORS @ 40 APP.RULE
  SCREEN.SENSORS @ 8 54 90 12 S.MUTED @ $" SENSORS" COUNT FACE.LABEL DROP
  SCREEN.SENSORS @ 8 74 120 60 S.HERO @ $" 21C" COUNT FACE.LABEL DROP
  SCREEN.SENSORS @ 152 58 64 10 S.MUTED @ $" HUMID" COUNT FACE.LABEL DROP
  SCREEN.SENSORS @ 152 76 36 58 S.ACCENT @ HUMIDITY.PCT 0 100 FACE.GAUGE DROP
  SCREEN.SENSORS @ 220 58 120 10 S.MUTED @ $" TREND" COUNT FACE.LABEL DROP
  SCREEN.SENSORS @ 220 76 210 66 S.ACCENT @ TEMP.TREND 8 0 30 FACE.CHART DROP
  SCREEN.SENSORS @ 0 178 FACE.W 1 S.ACCENT @ FACE.PANEL DROP
  SCREEN.SENSORS @ 0 188 216 34 S.CARD @ FACE.BOX DROP
  SCREEN.SENSORS @ 12 194 84 10 S.MUTED @ $" CURRENT" COUNT FACE.LABEL DROP
  SCREEN.SENSORS @ 12 204 120 14 S.TITLE @ $" TEMP 21C" COUNT FACE.LABEL DROP
  SCREEN.SENSORS @ 240 188 216 34 S.CARD @ FACE.BOX DROP
  SCREEN.SENSORS @ 252 194 84 10 S.MUTED @ $" STATUS" COUNT FACE.LABEL DROP
  SCREEN.SENSORS @ 252 204 120 14 S.TITLE @ $" STABLE" COUNT FACE.LABEL DROP ;
```

### Alerts Screen

This screen shows `FACE.ALARM` and `FACE.BUTTON`.

```forth
: TOGGLE.ALARM
  NAV.TAP? IF
    ALARM.STATE @ 0= IF 1 ELSE 0 THEN ALARM.STATE !
    SCREEN.ALERTS @ NODE.DIRTY
  THEN ;
' TOGGLE.ALARM CONSTANT TOGGLE.ALARM.XT

: BUILD.ALERTS
  S.ROOT @ SCREEN.NEW DUP SCREEN.ALERTS !
  APP.NAV.TAP.XT SCREEN.ALERTS @ NODE.EVENT!
  SCREEN.ALERTS @ APP.NAV.ALERTS
  SCREEN.ALERTS @ 40 APP.RULE
  SCREEN.ALERTS @ 8 54 72 12 S.MUTED @ $" ALERT" COUNT FACE.LABEL DROP
  SCREEN.ALERTS @ 8 74 190 72 S.HERO @ $" 06:30" COUNT FACE.LABEL DROP
  SCREEN.ALERTS @ 312 60 120 30 S.ALERT @ $" ARMED" COUNT ALARM.STATE FACE.ALARM DROP
  SCREEN.ALERTS @ 0 166 216 24 S.CARD @ FACE.BOX DROP
  SCREEN.ALERTS @ 12 172 160 12 S.TITLE @ $" BEDROOM BUZZER" COUNT FACE.LABEL DROP
  SCREEN.ALERTS @ 240 166 216 24 S.CARD @ FACE.BOX DROP
  SCREEN.ALERTS @ 252 172 150 12 S.TITLE @ $" EXIT REMINDER" COUNT FACE.LABEL DROP
  SCREEN.ALERTS @ 120 198 216 24 S.BUTTON @ $" TOGGLE ARM" COUNT TOGGLE.ALARM.XT FACE.BUTTON DROP ;
```

## Step 8: Assemble The App

```forth
: BUILD.MYAPP
  APP.NEW DUP APP.DEMO !
  DUP SCREEN.HOME    @ APP.ADD-SCREEN
  DUP SCREEN.SENSORS @ APP.ADD-SCREEN
  DUP SCREEN.ALERTS  @ APP.ADD-SCREEN
  0 APP.SHOW ;
```

## Step 9: Boot Or Rebuild

```forth
: MYAPP
  GFX.INIT
  APP.STYLES
  BUILD.HOME
  BUILD.SENSORS
  BUILD.ALERTS
  BUILD.MYAPP
  APP.DEMO @ APP.DRAW ;
```

Run it:

```forth
MYAPP
```

## What This Tutorial Used

This tutorial intentionally exercised the current high-level retained vocabulary:

- `STYLE.MAKE`
- `SCREEN.NEW`
- `NODE.EVENT!`
- `FACE.PANEL`
- `FACE.LABEL`
- `FACE.BUTTON`
- `FACE.GAUGE`
- `FACE.CHART`
- `FACE.ALARM`
- `FACE.ICON`
- `FACE.RULE`
- `FACE.BOX`
- `APP.NEW`
- `APP.ADD-SCREEN`
- `APP.SHOW`
- `APP.DRAW`
- `ICON.1BIT`

It also used the immediate-mode icon and text concepts indirectly through the retained widgets.

## Immediate-Mode Playground

If you want to test the immediate helper words before building retained nodes:

```forth
BLACK CLS
10 10 220 24 $" FifthOS" COUNT ICE CENTER-TEXT
12 40 216 1 ICE HLINE
20 60 180 40 BLACK ICE PANEL
20 72 $" TEST PANEL" COUNT ICE LABEL
20 120 180 16 55 100 ICE BLACK PROGRESS-BAR
```

That is useful for checking colors and proportions quickly before committing them to a retained screen builder.

## Recommended Pattern For Real Apps

When building a real FifthOS app:

1. Put constants, variables, and data buffers first.
2. Put styles next.
3. Prefer locals for pure helper and composition words.
4. Be conservative with locals in bootstrapping, scheduler, and hot draw-loop code until those paths are explicitly proven.
3. Put icon assets after that.
4. Put shared shell or metric builders next.
5. Put event handlers after that.
6. Put screen constructors last.
7. Put final app assembly in one small top-level word.

That keeps the separation of concerns clear:

- primitive capability at the bottom
- generic UI composition in the middle
- application words at the top

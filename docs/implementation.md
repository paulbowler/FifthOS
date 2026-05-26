# FifthOS Implementation Notes

This document is for implementers: language authors, VM/runtime engineers, and Forth maintainers who want to understand how FifthOS works internally, why it was built this way, and where the current design boundaries still are.

It complements the higher-level [architecture.md](architecture.md) and the user-facing [forth-reference.md](forth-reference.md). This file focuses on internal representation, execution, compilation, and design trade-offs.

## Goals And Non-Goals

FifthOS is not trying to be:

- a full ANS Forth implementation
- a hosted desktop Forth
- an optimizing compiler
- a traditional block-oriented development environment

FifthOS is trying to be:

- a small live language for embedded hardware
- interactive over USB serial and browser REPL
- easy to extend with native words
- able to host a retained GUI and device runtime from within the language
- simple enough that the implementation remains understandable and hackable

The system therefore prefers small, explicit machinery over abstraction purity.

## Lineage

The core interpreter and dictionary structure are inherited from a small eForth-style system. FifthOS extends that base with:

- an ESP32 runtime embedding
- Wi-Fi and HTTP REPL transport
- graphics, touch, and retained GUI primitives
- a small time/task service layer
- compiler-scoped locals

The implementation still carries the shape of the inherited system:

- direct-threaded execution
- bytecode-dispatched primitives
- dictionary-building code written partly in C++ and partly in bootstrapped Forth
- colon words compiled into linear code space

That lineage explains some design decisions that would look unusual in a greenfield Forth.

## Execution Model

### Core State

The VM state is intentionally global and simple.

From [include/vm.h](../include/vm.h):

- `stack[256]`: data stack backing store
- `rack[256]`: return stack backing store
- `data[16000]`: main cell-addressed data/code space
- `cData`: byte view over the same storage
- `S`: data stack index
- `R`: return stack index
- `P`: bytecode program counter for the current primitive stream
- `IP`: instruction pointer for threaded colon execution
- `WP`: working pointer to the current word body
- `top`: cached top-of-stack cell

The cached `top` register is central. Many primitives operate on `top` directly and use macros in [include/vm.h](../include/vm.h):

```c
#define vmPop top = stack[(unsigned char)S--]
#define vmPush stack[(unsigned char)++S] = top; top =
```

This is a classic small-Forth performance trade-off:

- faster primitive code for top-of-stack operations
- more care required when reasoning about stack transitions

### Direct-Threaded Outer Execution

The lowest execution loop is in [src/util.cpp](../src/util.cpp):

```c++
void evaluate()
{
    while (true) {
        bytecode = (unsigned char)cData[P++];
        if (bytecode) {
            primitives[bytecode]();
        } else {
            break;
        }
    }
}
```

So FifthOS executes a byte stream, not a tokenized AST or a native code image. Each byte indexes the `primitives[]` table declared in [include/primitives.h](../include/primitives.h).

This is a hybrid model:

- colon definitions are threaded sequences in code space
- primitive execution is bytecode-dispatched through a C++ table

This is much closer to classic embedded eForth than to systems like Gforth, which use more aggressive threading and host-level performance techniques.

## Dictionary Representation

### Dictionary Construction

The dictionary is built in [src/dictionary.cpp](../src/dictionary.cpp) by `initDictionary()`. The native side emits:

- user variables such as `>IN`, `#TIB`, `CP`, `LAST`, `CONTEXT`
- primitive words such as `DROP`, `DUP`, `@`, `!`
- FifthOS native extensions such as `GFX.INIT`, `WIFI.STATUS`, `TASK.NEW`
- compiler words such as `:`, `;`, `COMPILE`, `$COMPILE`

The dictionary is not loaded from an image file. It is reconstructed at boot by C++.

This is a deliberate trade-off:

- simpler firmware build and portability
- easier native extension development
- longer boot path than a prebuilt dictionary image

### Headers And Bodies

The implementation uses helper macros/functions such as `HEADER()`, `CODE()`, `COLON()`, and `LABEL()` inside `initDictionary()`.

Conceptually:

- `HEADER` emits the dictionary header and links the word into the current vocabulary chain
- `CODE` emits a code body containing primitive bytecodes and inline cells
- `COLON` emits a colon definition body

Important consequence: many “core Forth words” are themselves defined inside `initDictionary()` using Forth-level composition on top of a smaller primitive substrate.

That keeps the primitive set smaller, but means language semantics are split across:

- C++ primitive behavior
- bootstrapped colon definitions in `initDictionary()`

## Memory Model

FifthOS uses one shared memory region for dictionary and user data:

- `data[]` is cell-oriented
- `cData` is the byte-oriented overlay

This gives the implementation a compact, inspectable model, but it also means:

- code and data are intentionally close together
- low-level words like `HERE`, `,`, `ALLOT`, `C!`, `!`, `MOVE`, `FILL` matter directly to system integrity
- malformed compiler extensions can corrupt live code space quickly

This is an accepted Forth trade-off rather than an accident.

## Text Evaluation

Browser and serial REPL input both eventually flow through [src/forth_runtime.cpp](../src/forth_runtime.cpp):

```c++
void forth_eval_buffer(const char* text, size_t len)
```

The implementation writes input into a fixed region:

- `FORTH_EVAL_ADDR = 0x8000`
- `FORTH_EVAL_MAX = 0x7000`

Then it sets:

- `>IN`
- `#TIB`
- `'TIB`

and jumps into `EVAL`.

This is intentionally crude but predictable:

- no dynamic string object
- no parser object graph
- the interpreter sees the same `TIB`-style input shape regardless of source

## Compiler Model

### Interpretation vs Compilation

The inherited model uses an execution token stored in `'EVAL` to choose between:

- interpreter behavior
- compile behavior

`[` switches to interpret mode. `]` switches to compile mode. `:` and `;` are built on top of that.

Key words in [src/dictionary.cpp](../src/dictionary.cpp):

- `$INTERPRET`
- `$COMPILE`
- `COMPILE`
- `]`
- `[`
- `:`
- `;`

This is a classic minimal Forth compiler design: parsing and code generation are tightly interleaved and happen against the live dictionary.

### Defining Words

`CREATE`, `VARIABLE`, and `CONSTANT` are still defined in Forth terms inside `initDictionary()`:

- `CODE`
- `CREATE`
- `VARIABLE`
- `CONSTANT`

This matters because earlier bugs in FifthOS were not display bugs at all; they turned out to be compiler-path issues in these defining words and the underlying storage/align path.

One concrete example from the project history:

- `CODE` originally aligned the caller's data stack value instead of aligning `HERE`/`CP`
- runtime-defined constants such as `FFFF CONSTANT WHITE` were therefore corrupted
- GUI colors failed as a symptom of a compiler/layout bug

That is an instructive example of the FifthOS design philosophy: keep the system small, but accept that compiler/runtime boundaries are exposed and must be understood.

## Primitive Dispatch

Primitive IDs are declared in [include/primitives.h](../include/primitives.h). The current table has 157 entries.

Categories include:

- VM and arithmetic core
- raw memory access
- graphics backend
- touch and GUI runtime
- Wi-Fi and network control
- time and task runtime
- locals support

This is a very explicit design. FifthOS does not hide native capabilities behind a large C++ object model. It exposes them as primitive words as directly as practical.

Trade-off:

- easy to add capabilities
- primitive namespace grows quickly
- more pressure on documentation and disciplined layering

## Colon Calls And Return Stack Use

The call path is in [src/primitives.cpp](../src/primitives.cpp):

- `dolist()` enters a colon definition
- `exitt()` returns from one

`dolist()`:

- increments `forthCallDepth`
- initializes the locals frame slot for that depth
- pushes `IP` onto the return stack
- loads `IP = WP`

`exitt()`:

- tears down any active locals frame for that depth
- restores `IP` from the return stack
- decrements `forthCallDepth`

This is the point where FifthOS differs from many textbook descriptions: locals are now tied into call depth and return-stack discipline.

## Locals Design

### Stable Feature: Input Locals

FifthOS now supports compiler-scoped input locals:

```forth
: ADD2 { a b -- } a b + ;
```

These are stable and used throughout the GUI vocabulary.

Implementation pieces:

- compile-time state in [include/globals.h](../include/globals.h)
- parser/compiler support in [src/primitives.cpp](../src/primitives.cpp)
- dispatch words registered in [src/dictionary.cpp](../src/dictionary.cpp):
  - `LOCALS.ENTER`
  - `LOCAL.GET`
  - `{`
  - `COMPILE.LOCALS`

The current strategy is compiler-scoped name shadowing:

- while compiling a word with locals active, names declared in `{ ... }` are looked up before the ordinary dictionary
- matching names compile `LOCAL.GET <index>`
- the compiler state is cleared at `;`

### Experimental Feature: Scratch Locals And `TO`

FifthOS also now has an experimental form:

```forth
: T1 { a b | sum -- } a b + TO sum sum . ;
```

This adds:

- scratch locals after `|`
- `LOCAL.SET`
- `TO local` compilation support

The mechanism works in simple cases, but it is not yet considered stable for hot draw loops or more complex mutable-local patterns.

This is an important design boundary:

- input locals are production-ready in FifthOS
- writable scratch locals are not yet mature enough to eliminate every manual scratch path

### Why Locals Were Added Incrementally

The original GUI vocabulary used anonymous scratch globals like `G0..G9`.

That choice was pragmatic:

- the inherited core had no locals
- GUI words are argument-heavy
- pure stack shuffling was reducing readability and slowing debugging

Locals were introduced later to improve vocabulary quality without rewriting the VM wholesale.

The migration has been incremental because the hot paths expose the real design constraints:

- app bootstrap is more brittle than helper words
- scheduler/event code is more brittle than static layout code
- the calendar day-cell draw loop is the hottest remaining edge case

## The Remaining Scratch Exception

Generic `G0..G9` scratch globals are gone from the boot vocabulary.

The one remaining exception is the calendar day-cell renderer in [include/gui_boot.h](../include/gui_boot.h), which still uses explicit named scratch variables:

- `CAL.DRAW.DAY#`
- `CAL.DRAW.ADDR`
- `CAL.DRAW.LEN`
- `CAL.DRAW.COL`
- `CAL.DRAW.ROW`
- `CAL.DRAW.X`
- `CAL.DRAW.Y`

This is not stylistic inconsistency. It is a deliberate containment of the one path where current writable locals are not yet robust enough.

The accepted trade-off is:

- remove generic scratch globals everywhere practical
- keep one narrowly scoped manual scratch frame where the runtime semantics still need work

## Comparison To Other Forth Implementations

These are broad design comparisons, not claims of full semantic equivalence.

### Versus Gforth

Gforth is a much more feature-rich hosted system with:

- strong ANS coverage
- mature locals
- more aggressive implementation techniques
- desktop-oriented development workflow

FifthOS differs by choosing:

- a much smaller VM surface
- embedded-first constraints
- direct exposure of device primitives
- deliberately incomplete standard coverage in exchange for hardware focus

If Gforth optimizes for language completeness and host power, FifthOS optimizes for inspectable embedded integration.

### Versus Mecrisp-Stellaris

Mecrisp-Stellaris is a closer philosophical neighbor:

- microcontroller resident
- interactive
- small and practical

But Mecrisp typically leans more toward compact native-code execution and a traditional MCU-Forth style, whereas FifthOS currently remains closer to:

- bytecode-dispatched primitives
- boot-built dictionary
- a strong mixed-language split between C++ runtime and Forth GUI vocabulary

FifthOS also places much more emphasis on:

- network REPL transport
- retained GUI runtime
- touch and app layering

### Versus FlashForth / Tiny Embedded Forths

Many small embedded Forths optimize for:

- serial console use only
- minimum memory
- tiny core vocabularies

FifthOS accepts additional complexity for:

- Wi-Fi provisioning
- browser-served REPL
- retained GUI nodes and styles
- periodic tasks and time services

In other words, FifthOS is not trying to be the smallest embedded Forth. It is trying to be a small but usable embedded application language.

## Design Decisions And Accepted Trade-Offs

### 1. Shared Code/Data Space

Accepted because:

- it keeps the model simple
- it matches inherited eForth assumptions
- it is easy to inspect at runtime

Cost:

- compiler mistakes are dangerous
- memory protection is minimal

### 2. Bytecode Primitive Dispatch

Accepted because:

- simple to port
- simple to extend
- easy to reason about when debugging

Cost:

- slower than more optimized threading/native strategies
- more runtime overhead per primitive

### 3. Boot-Built Dictionary

Accepted because:

- native extension work is straightforward
- no separate image-generation toolchain is required

Cost:

- boot sequence is longer and more fragile than loading a frozen image
- dictionary bugs surface at startup

### 4. Forth-Level GUI Composition

Accepted because:

- it keeps the interesting UI vocabulary in the language
- allows live iteration from the REPL
- maintains backend portability

Cost:

- some GUI words are structurally more complex than a C++ widget API would be
- language/runtime weaknesses show up quickly in application code

### 5. Incremental Locals Migration

Accepted because:

- large-scale compiler/runtime rewrites would be riskier than staged improvement
- helper words delivered most of the readability benefit early

Cost:

- temporary mixed style in the boot vocabulary
- one remaining manual scratch-frame word

### 6. Fixed Pools In GUI And Tasks

Accepted because:

- deterministic memory use matters on this target
- the retained GUI does not need unbounded growth

Cost:

- hard limits require revisiting as the system grows
- no dynamic elasticity

## Known Technical Gaps

These are the important unresolved areas.

### Locals Runtime Maturity

Input locals are stable.

Scratch locals plus `TO` are not yet mature enough for the hottest renderer paths. The likely long-term fix is a better locals frame model, probably RAM-frame-backed rather than tightly coupled to the current return-stack discipline.

### Standard Coverage

FifthOS is not currently trying to be a full standards-oriented Forth. That is acceptable for the device mission, but it means portability of arbitrary third-party Forth code is limited.

### Error Recovery And Diagnostics

Compiler/runtime diagnostics are still closer to classic Forth than to modern language tooling:

- unknown word reporting is basic
- stack effect mistakes can still surface indirectly
- bytecode-level and dictionary-level debugging is still mostly manual

### Memory Safety

Low-level words such as `!`, `C!`, `POKE`, `MOVE`, and compiler-space mutation remain intentionally powerful. That keeps the system flexible, but it also means the environment is easy to damage from the REPL.

### Concurrency Model

FifthOS is effectively single-threaded from the language point of view. Timers, GUI polling, HTTP requests, and user evaluation all rendezvous through one live interpreter. This keeps mental load down, but it limits scaling to more complex asynchronous semantics.

## Work Still To Do

The most important implementation work remaining is:

1. Mature writable locals enough to remove the final calendar scratch-frame exception.
2. Decide whether locals should continue evolving on the current return-stack-backed scheme or move to a dedicated RAM-frame design.
3. Improve compiler/runtime diagnostics for defining words and locals.
4. Continue tightening the separation between stable user-facing language features and experimental implementation surfaces.
5. Revisit fixed pool sizes and internal limits as the GUI vocabulary grows.
6. Decide how much ANS-style compatibility FifthOS should target versus staying explicitly device-focused.

## Practical Reading Order For Implementers

If you want to understand the implementation efficiently, read in this order:

1. [include/vm.h](../include/vm.h)
2. [src/util.cpp](../src/util.cpp)
3. [src/primitives.cpp](../src/primitives.cpp)
4. [src/dictionary.cpp](../src/dictionary.cpp)
5. [src/forth_runtime.cpp](../src/forth_runtime.cpp)
6. [include/gui_boot.h](../include/gui_boot.h)
7. [src/gui_runtime.cpp](../src/gui_runtime.cpp)

That path moves from VM mechanics to compiler behavior, then into the higher-level language that exercises those mechanisms most aggressively.

## Final Position

FifthOS is best understood as a practical embedded language system rather than a pure Forth implementation exercise.

Its strongest qualities come from that choice:

- live interaction
- simple extension path
- direct hardware relevance
- high-level app construction in the language itself

Its rough edges also come from that choice:

- mixed inherited and evolved semantics
- incomplete standardization
- exposed compiler/runtime boundaries
- a few still-maturing core features such as writable scratch locals

For language implementers, that makes FifthOS interesting precisely because it is not polished into abstraction. The engineering constraints remain visible in the design.

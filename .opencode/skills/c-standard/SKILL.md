---
name: c-standard
description: The C coding standard for the bare-meshcore pure-C firmware. Use when writing, editing, or reviewing any C code (.c/.h) in this repo, including Doxygen documentation compliance, the no-blank-line-inside-function rule, files covered, and the audit method. Trigger on keywords like "C standard", "style", "documentation", "Doxygen", "doc compliance", "blank line", "code audit", or when starting any new function, struct, macro, or file.
---

# C Standard (bare-meshcore)

This is the coding standard for the pure-C firmware in this repo. Every
function, global, macro, `typedef`, struct, and member must comply. Vendored
libraries and auto-generated files are **excluded** (see Scope).

## Scope: what must comply

Comply (all C code you own):

- Every `*.c` and `*.h` in this repo that you write or edit, including `main/`,
  `core/`, `drv/`, `app/`, `protocol/`, and `crypto/` (everything except the
  exclusions below).

Excluded (do not touch):

- `crypto/ed25519/` (vendored orlp/ed25519 — keep as-is, compile as C)
- Auto-generated files (DO-NOT-EDIT, e.g. generated config/systables)

## Rule 1: No blank lines inside a function or method body

There must be **zero** blank lines within any function body.

- A function body is the `{ ... }` that follows a signature — i.e. a `{` whose
  preceding non-whitespace character is `)`. In C this covers function
  definitions AND the control-flow blocks inside them (`if/for/while/switch`):
  all of those braces are inside a function body, so blank lines are forbidden
  at any depth within the body.
- Blank lines are allowed **only** to separate functions or types from each
  other (outside bodies). Struct/union/enum member declarations are **not**
  function bodies — blank lines may separate them.
- A "blank line" is a line that is whitespace-only (including a line of indent
  spaces). Lines that contain code, a `//` comment, or a `/** ... */` block are
  NOT blank and are untouched.

Examples:

```c
// WRONG: blank lines (line 2 and 4) inside the body
static void sx1262_reset(void) {
    sx1262_wait_busy();
                      // <- blank, remove
    sx1262_write_cmd(...);
                      // <- blank, remove
}
```

```c
// CORRECT: no blank lines inside the body
static void sx1262_reset(void) {
    sx1262_wait_busy();
    sx1262_write_cmd(...);
}
```

### Comments inside bodies

- Keep comment lines in place; only remove the genuinely blank lines.
- Guard a multi-line `/* ... */` comment can span blank-looking comment lines:
  those are comment content, not blank lines — leave them. Only whitespace-only
  lines count.

## Rule 2: Doxygen documentation compliance (100%)

Every entity in scope must carry Doxygen documentation.

### File header

Every file starts with this exact structure:

```c
/**
 * FILE: sx1262.c
 *
 * DESCRIPTION:
 * One or two lines describing what the file does.
 *
 * BRIEF:
 * Short one-liner.
 *
 * AUTHOR: Kevin Thomas
 * DATE: <Month> <Year>
 */
```

### Functions

Every function (including file-local `static` helpers) gets a doc block
immediately above it with `@brief` and, when applicable, `@param`/`@return`:

```c
/**
 * @brief Clears the LED array and pushes the update.
 *
 * @return None
 */
static void neopixel_clear(void) {
    neopixel_fill(0x000000);
    neopixel_show();
}
```

Use `@param None` / `@return None` when there are none meaningful. If a function
takes parameters, name every one:

```c
/**
 * @brief Sets the LoRa modulation parameters on the SX1262.
 *
 * @param freq The center frequency in Hz.
 * @param bw The bandwidth from ::sx1262_bandwidth_t.
 * @return True on success, false on timeout.
 */
bool sx1262_set_params(uint32_t freq, uint8_t bw);
```

### Globals, `#define`s, `typedef`s, struct members, and constants

Each gets a `@brief` block:

```c
/**
 * @brief Hardware pin connected to the NeoPixel DIN line.
 */
#define NEOPIXEL_PIN 10

/**
 * @brief Number of LEDs in the badge strip.
 */
#define NEOPIXEL_COUNT 80
```

For struct members (public and private), each member declaration is preceded by
a `@brief` block:

```c
/**
 * @brief SX1262 driver instance state.
 */
typedef struct {
    /**
     * @brief Current operating mode.
     */
    sx1262_mode_t mode;
    /**
     * @brief Last recorded RSSI in dBm.
     */
    int16_t last_rssi;
} Sx1262Radio;
```

### Locals

Simple loop iteration locals need no annotation. Named/explicit locals carry a
`@brief Declaration of X.` block immediately above (no blank line between the
comment and the declaration, and no blank line between consecutive blocks):

```c
static void neopixel_process_drive_frame(void) {
    for (uint8_t y = 0; y < kDriveRowCount; y++) {
        /**
         * @brief Declaration of start.
         */
        uint8_t start = kDriveRowStart[y];
        /**
         * @brief Declaration of len.
         */
        uint8_t len = kDriveRowLength[y];
    }
}
```

> Note: the `@brief Declaration of <name>.` blocks are a repo convention and
> MUST be preserved when present. Blank lines inside function bodies are the
> only thing removed — never delete a Doxygen comment block.

## Rule 3: Naming and structure conventions

- Files: `snake_case` (`sx1262.c`, `packet.c`), matching the module.
- Constants/`#define`s: `k` + PascalCase (`kDriveRowCount`) or `UPPER_SNAKE`
  (`NEOPIXEL_PIN`), whichever the surrounding file uses.
- Types (`typedef` names): PascalCase (`Sx1262Radio`, `MeshPacket`,
  `OuroborosMac`).
- Functions: `snake_case`, prefixed with the module name (`sx1262_init`,
  `mesh_packet_parse`, `neopixel_show`).
- Struct members: `snake_case`.
- File-local (static) symbols: `_snake_case` (leading underscore) for variables
  and helpers that are private to the `.c` file (`_drive_offset`); non-static
  globals are avoided entirely in favor of module-owned instances.
- `#include` guards or `#pragma once` at the top of headers.

## Rule 4: Pitfalls to avoid

- Never include secrets or credentials in plaintext readably in the binary —
  use the ouroboros MAC / obfuscated-secret pattern, NOT literal strings.
- Never use leading-underscore+capital identifiers (`_Foo`) — reserved.
- Don't touch vendored `crypto/ed25519/` or auto-generated files.
- Don't add `//` editor comments; keep the Doxygen style consistent (`/** */`).
- Use `<stdint.h>` fixed-width types; never rely on `int` width.
- No implicit narrowing conversions that truncate values.
- No dynamic memory allocation except during setup/begin (MeshCore principle);
  keep stack usage bounded for the badge's task stack buckets.

## Auditing: how to verify compliance

Run the audit scanner (committed in this repo) to find blank lines inside
function bodies:

```bash
python3 scripts/audit_blank_lines.py
```

The scanner reports `path: (count) [line numbers]` for whitespace-only lines
sitting inside a function body (correctly ignoring comments, strings, and
struct/union/enum member declarations). No output means the "no blank line in
function body" rule holds across your owned `.c`/`.h` files (excluding
`crypto/ed25519/` and auto-generated files).

After any edit, rebuild. Bare-metal ESP-IDF project:

```bash
source $HOME/esp/esp-idf/export.sh
idf.py build
```

## References

- Bias toward the existing conformant C files in this repo as live examples of
  correct style.
- The standard enforces consistent C documentation, formatting, and a strict no-blank-line-inside-functions rule across all firmware modules.
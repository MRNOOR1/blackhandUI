/*
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║  main.c — BlackHand OS  ·  Entry Point & UI Engine                     ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 *
 *  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  HOW TO READ THESE COMMENTS
 *  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  Every C and Notcurses concept is explained the first time it appears,
 *  then used without repetition ater that.  Read this file top-to-bottom
 *  once before building any new screen — after that you will have everything
 *  you need to write any view independently.
 *
 *  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  ARCHITECTURE OVERVIEW
 *  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *
 *  ┌─────────────────────────────────────────────────────────────┐
 *  │  main()                                                     │
 *  │    │                                                        │
 *  │    ├── hardware_init()          read battery/signal         │
 *  │    ├── notcurses_init()         start terminal graphics     │
 *  │    ├── create_phone_plane()     our drawing canvas          │
 *  │    │                                                        │
 *  │    └── LOOP ─────────────────────────────────────────────  │
 *  │            │                                                │
 *  │            ├── draw_frame()     border + status bar        │
 *  │            ├── screen_*_draw()  active screen content      │
 *  │            ├── notcurses_render() push to terminal         │
 *  │            └── handle input → update screen_id            │
 *  └─────────────────────────────────────────────────────────────┘
 *
 *  TO ADD A NEW SCREEN — complete checklist:
 *  ─────────────────────────────────────────
 *    1.  Add   SCREEN_CALLS       to the screen_id enum in ui.h
 *    2.  Add   "CALLS"            to the screen_name switch in main()
 *    3.  Create screen_calls.c    with screen_calls_draw() and
 *                                      screen_calls_input()
 *    4.  Declare both functions   in ui.h
 *    5.  Add a draw case          to the draw switch in the main loop
 *    6.  Add an input case        to the input switch in the main loop
 *    7.  Route a key to it        in whichever screen_*_input() navigates there
 *
 *  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  RECOMMENDED DISPLAY DIMENSIONS  (HyperPixel 4.0  480 × 800 portrait)
 *  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *
 *  Font: Iosevka Term  ss08 variant  (thin, razor-precise, luxury feel)
 *  Install: build from https://github.com/be5invis/Iosevka
 *  Or use pre-built: apt install fonts-iosevka  (may not include ss08)
 *
 *  Set in /etc/default/console-setup:
 *    FONTFACE="Iosevka Term"
 *    FONTSIZE="12x20"         ← approx 40 cols × 40 rows on HyperPixel 4.0
 *
 *  Alternative: Departure Mono  — bitmap-inspired, unique rhythm, very
 *  distinctive.  https://departuremono.com/  (free, open source)
 *
 *  Vertu Signature S had a 240×320 display (3:4 portrait ratio, very tall
 *  and narrow).  To match that feel:
 *
 *    #define PHONE_COLS  36    // ~90 % of a 40-col terminal
 *    #define PHONE_ROWS  38    // ~95 % of a 40-row terminal
 *
 *  This leaves a 2-cell margin around the frame — enough for the DEV_LABEL
 *  corner text without crowding the border.  The 36:38 ratio is close to
 *  the Signature S's tall-narrow silhouette.
 *
 *  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  CONTENT AREA — where your screen functions draw
 *  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *
 *  Row 0          ┏━━━━━━━━━━━━━━━━━━┓  top border
 *  Row 1          ┃ 09:41    ▰▰▰▱ ●●┃  status bar   (STATUS_ROW = 1)
 *  Row 2          ┣━━━━ HOME ━━━━━━━┫  separator
 *  Rows 3..35     ┃                  ┃  CONTENT AREA  ← screens draw here
 *  Row 36         ┗━━━━━━━━━━━━━━━━━━┛  bottom border
 *
 *  Content area:
 *    First row  = 3                  (CONTENT_ROW_START in config.h)
 *    Last row   = PHONE_ROWS - 2     (last interior row above bottom border)
 *    First col  = 2                  (one inside the border + one margin)
 *    Last col   = PHONE_COLS - 3     (one inside border + one margin)
 *    Inner width = PHONE_COLS - 4    (usable columns per row)
 */


/* ══════════════════════════════════════════════════════════════════════════
 *  SECTION 1: INCLUDES
 *
 *  C CONCEPT: #include
 *  ────────────────────
 *  #include copies the contents of another file into this one at compile
 *  time.  Think of it as "paste this code here before compiling."
 *
 *  Two forms:
 *    #include <file.h>   — system/library headers, searched in system paths
 *    #include "file.h"   — your own headers, searched from current directory
 *
 *  Headers (.h files) contain DECLARATIONS — they tell the compiler what
 *  functions and types exist without providing their full implementation.
 *  The compiler needs declarations before it can compile calls to those
 *  functions.  The actual implementations live in .c files.
 * ══════════════════════════════════════════════════════════════════════════ */

#include <locale.h>
/*  Provides setlocale().  Must be called before any Unicode output so the
 *  C runtime uses the correct character encoding (UTF-8 on the Pi).
 *  Without it, multi-byte characters like ▰ ● ┏ display as garbage.        */

#include <notcurses/notcurses.h>
/*  The entire Notcurses API.  Key types you will use in every screen:
 *
 *  struct notcurses *nc
 *    The engine — one per program.  Created by notcurses_init(),
 *    destroyed by notcurses_stop().  Manages the terminal, all planes,
 *    the render pipeline, and input.
 *
 *  struct ncplane *plane
 *    A rectangular drawing surface — our canvas.  Like a transparent layer
 *    in Photoshop.  Multiple planes exist simultaneously and composite on
 *    render.  The standard plane (stdplane) covers the whole terminal.
 *    Every screen_*_draw() function receives the phone plane and draws on it.
 *
 *  nccell
 *    One character cell: glyph + fg colour + bg colour + style flags.
 *    Used directly when working with box-drawing borders.
 *
 *  ncinput
 *    One keyboard or mouse event.  Fields: .id (key code), .evtype,
 *    .modifiers (shift/ctrl/alt), .y and .x (mouse position).             */

#include <stdint.h>
/*  Fixed-width integer types.  Notcurses uses these for colours and keys:
 *
 *  uint32_t  unsigned 32-bit (0 – 4,294,967,295)
 *            Used for 0xRRGGBB colours and Unicode key codes.
 *
 *  uint64_t  unsigned 64-bit
 *            Used for Notcurses "channels" (packed fg+bg colour pair).
 *
 *  uint8_t   unsigned 8-bit (0 – 255)
 *            Useful for individual R, G, B components when doing colour math.
 *
 *  WHY NOT JUST USE 'int'?
 *  Plain 'int' is platform-dependent — 16, 32, or 64 bits depending on
 *  CPU and compiler.  uint32_t is always exactly 32 bits, which Notcurses
 *  requires for its packed colour encoding.                                  */

#include <stdio.h>
/*  Standard I/O.  Functions used here:
 *    fprintf(stderr, "msg\n")           print errors to stderr
 *    snprintf(buffer, size, fmt, ...)   format a string into a char array   */

#include <string.h>
/*  String functions.  Used here:
 *    strlen(str)          count bytes in str (not counting '\0')
 *    strcat(dest, src)    append src to dest (dest must have room!)          */

#include <stdbool.h>
/*  Provides 'bool', 'true' (= 1), and 'false' (= 0).
 *  Without this header you'd write int charging = 1 instead of
 *  bool charging = true — less readable and more error-prone.               */

#include "ui.h"
/*  Our screen system.  Contains:
 *    typedef enum { SCREEN_HOME, SCREEN_SETTINGS, … } screen_id;
 *    Declarations for screen_*_draw() and screen_*_input() functions.
 *
 *  An enum is a set of named integer constants.  The compiler assigns
 *  sequential values starting from 0 unless you specify otherwise.
 *  Using enum names instead of raw integers makes switch statements
 *  self-documenting and lets the compiler warn about unhandled cases.       */

#include "config.h"
/*  All magic numbers in one place.  Add to this file whenever you need
 *  a new constant — never hardcode numbers directly in .c files.
 *
 *  Key constants defined here:
 *    PHONE_ROWS, PHONE_COLS          frame dimensions
 *    STATUS_ROW          = 1         status bar row
 *    STATUS_BATTERY_COL  = 2         battery glyphs start column
 *    STATUS_BATTERY_PCT_COL          percentage label column
 *    CONTENT_ROW_START   = 3         first row screens may draw on
 *    FRAME_MIN_ROWS, FRAME_MIN_COLS  safety limits before drawing
 *    COL_BG              0x080808    background (near-black)
 *    COL_BORDER          0x242424    border colour
 *    COL_SEPARATOR       0x1C1C1C    separator line colour
 *    COL_GHOST_ON        0xE0E0E0    active glyph, off-white
 *    COL_GHOST_OFF       0x1E1E1E    inactive glyph, near-invisible
 *    COL_GHOST_PCT       0x2C2C2C    percentage/dim text
 *    COL_GHOST_LOW       0x7F1D1D    low battery warning, deep red
 *    COL_PLACEHOLDER     0x333333    placeholder text
 *    COL_HINT            0x2A2A2A    key-hint text
 *    COL_DEV_LABEL       0x1A1A1A    corner dev tag                        */

#include "platform/hardware.h"
#include "services/settings_service.h"
/*  Battery and cellular abstraction layer.  Currently stubs; later wraps
 *  real I2C reads (battery gauge) and UART/AT commands (cellular modem).
 *
 *  Types provided:
 *    battery_status_t  { int percent; bool charging; }
 *    cellular_status_t { int signal_bars; bool connected; char carrier[32]; }
 *
 *  Functions provided:
 *    void hardware_init()
 *    void hardware_cleanup()
 *    battery_status_t  hardware_get_battery()
 *    cellular_status_t hardware_get_cellular()                               */


/* ══════════════════════════════════════════════════════════════════════════
 *  SECTION 2: PRIMITIVE DRAWING HELPERS
 *
 *  These are the building blocks every screen reuses.  Understand these
 *  five functions and you can draw anything in any screen without help.
 *
 *  C CONCEPT: static functions
 *  ────────────────────────────
 *  'static' before a function = "private to this .c file."
 *  No other file can call it.  This prevents name collisions between files
 *  and signals that this is internal plumbing, not a public API.
 *
 *  C CONCEPT: function parameters and pointers
 *  ─────────────────────────────────────────────
 *  Parameters are copies of the values passed in (pass-by-value) EXCEPT
 *  for pointers.  A pointer holds a MEMORY ADDRESS, not the data itself.
 *
 *  Analogy: a pointer is like a street address.  The address (4 bytes)
 *  is cheap to copy.  What lives at that address (the house = the plane
 *  struct) is NOT copied — both caller and function access the same data.
 *
 *  'struct ncplane *n'  — n is a pointer to an ncplane.  Passing n gives
 *  the function direct access to the plane so it can draw on it.
 *
 *  'const char *text'   — pointer to a read-only string.  'const' prevents
 *  accidental modification.  String literals ("HOME") live in read-only
 *  memory and must always be accessed through const char *.
 * ══════════════════════════════════════════════════════════════════════════ */

/* ──────────────────────────────────────────────────────────────────────────
 *  ghost_set()  —  Set foreground colour, lock background to COL_BG
 *
 *  The single most-called function in the codebase.  Every draw operation
 *  calls this (or ghost_text which calls it internally) before placing text.
 *
 *  NOTCURSES: ncplane_set_fg_rgb(plane, colour)
 *  ─────────────────────────────────────────────
 *  Sets the foreground (glyph/text) colour for ALL subsequent draws on
 *  'plane' until changed again.  Colour is 0xRRGGBB — three 8-bit channels
 *  packed into the lower 24 bits of a uint32_t:
 *    bits 23-16 = Red   (0x00–0xFF)
 *    bits 15-8  = Green (0x00–0xFF)
 *    bits 7-0   = Blue  (0x00–0xFF)
 *
 *  NOTCURSES: ncplane_set_bg_rgb(plane, colour)
 *  ─────────────────────────────────────────────
 *  Same as above for background — the rectangle of colour behind each glyph.
 *
 *  IMPORTANT: Colours are STICKY — they remain set until explicitly changed.
 *  Never assume the colour is what you set earlier; other drawing calls
 *  may have changed it.  Always set colour immediately before drawing.
 * ────────────────────────────────────────────────────────────────────────── */
static void ghost_set(struct ncplane *n, uint32_t fg) {
    ncplane_set_fg_rgb(n, fg);
    ncplane_set_bg_rgb(n, COL_BG);
}

/* ──────────────────────────────────────────────────────────────────────────
 *  ghost_text()  —  Set colour and draw a string at (row, col)
 *
 *  Use this for every piece of text your screens draw.
 *
 *  NOTCURSES: ncplane_putstr_yx(plane, row, col, string)
 *  ──────────────────────────────────────────────────────
 *  Draws 'string' starting at cell (row, col) using the currently-set
 *  colours.  The cursor advances right after each glyph.
 *
 *  "_yx" ordering: Y (row) comes first, X (col) second.  Notcurses
 *  consistently uses (row, col) ordering throughout its API, never (x, y).
 *  Row 0 = top of plane.  Col 0 = left edge of plane.
 *
 *  UNICODE NOTE:
 *  C strings are byte arrays.  ▰ is 3 UTF-8 bytes, ● is 3 bytes, etc.
 *  Notcurses handles UTF-8 parsing internally — you just pass the string.
 *  Each Unicode character typically occupies 1 terminal column (some CJK
 *  characters occupy 2).  Column arithmetic uses terminal columns, not bytes.
 *
 *  USAGE EXAMPLES:
 *    ghost_text(p, 4, 2, COL_GHOST_ON,  "BATTERY");
 *    ghost_text(p, 4, 12, COL_GHOST_PCT, "▰▰▰▱  75%");
 *    ghost_text(p, 6, 2, COL_GHOST_LOW, "LOW SIGNAL");
 * ────────────────────────────────────────────────────────────────────────── */
static void ghost_text(struct ncplane *n, int row, int col,
                       uint32_t colour, const char *text) {
    ghost_set(n, colour);
    ncplane_putstr_yx(n, row, col, text);
}

/* ──────────────────────────────────────────────────────────────────────────
 *  ghost_hline()  —  Draw a horizontal run of one repeated glyph
 *
 *  Use for content separators, progress indicators, or decorative rules
 *  within a screen.  Not for the status separator (draw_frame handles that).
 *
 *  C CONCEPT: for loops
 *  ─────────────────────
 *  for (initialiser; condition; step) { body }
 *
 *  Execution order:
 *    1.  initialiser runs once before the loop starts
 *    2.  condition is checked before each iteration — exit when false
 *    3.  body executes
 *    4.  step executes
 *    5.  back to 2
 *
 *  'i++' is shorthand for i = i + 1.
 *  'i < length' means the loop runs for i = 0, 1, 2, … length-1.
 *  Total iterations = length.
 *
 *  USAGE:
 *    ghost_hline(p, 10, 2, 32, "─", COL_SEPARATOR);
 *    // draws 32 thin-line glyphs starting at row 10, col 2
 * ────────────────────────────────────────────────────────────────────────── */
static void ghost_hline(struct ncplane *n, int row, int col,
                        int length, const char *glyph, uint32_t colour) {
    ghost_set(n, colour);
    for (int i = 0; i < length; i++) {
        ncplane_putstr_yx(n, row, col + i, glyph);
    }
}

/* ──────────────────────────────────────────────────────────────────────────
 *  ghost_fill_rect()  —  Fill a rectangle with a single character
 *
 *  Use to:
 *    • Clear a sub-region before redrawing it (fill with ' ', COL_BG, COL_BG)
 *    • Draw a highlight bar for a selected menu item
 *    • Paint a coloured background block
 *
 *  C CONCEPT: ncplane_putchar_yx vs ncplane_putstr_yx
 *  ───────────────────────────────────────────────────
 *  putchar_yx draws a single ASCII character (one byte, value 0-127).
 *  putstr_yx  draws a null-terminated string, handling multi-byte UTF-8.
 *  For spaces and simple ASCII: use putchar_yx — it's marginally faster.
 *  For any Unicode glyph (▰, ●, ┃, etc.): always use putstr_yx.
 *
 *  USAGE:
 *    // Clear a 4-row × 28-col region of content
 *    ghost_fill_rect(p, 5, 2, 4, 28, ' ', COL_BG, COL_BG);
 *
 *    // Highlight bar for selected menu item (dark bg, bright text)
 *    ghost_fill_rect(p, 7, 1, 1, PHONE_COLS-2, ' ', COL_BG, 0x1C1C1C);
 * ────────────────────────────────────────────────────────────────────────── */
static void ghost_fill_rect(struct ncplane *n,
                             int row, int col, int h, int w,
                             char ch, uint32_t fg, uint32_t bg) {
    ncplane_set_fg_rgb(n, fg);
    ncplane_set_bg_rgb(n, bg);
    for (int r = 0; r < h; r++) {
        for (int c = 0; c < w; c++) {
            ncplane_putchar_yx(n, row + r, col + c, ch);
        }
    }
}

/* ──────────────────────────────────────────────────────────────────────────
 *  ghost_label_value()  —  Draw a dim label + brighter value on one row
 *
 *  This is the most common layout pattern across all screens:
 *
 *    col 2           col 14
 *    │               │
 *    SIGNAL          ●●●○
 *    BATTERY         ▰▰▰▱  75%
 *    CARRIER         BH-NET
 *    NETWORK         LTE
 *
 *  Align all value_col arguments to the same column for a clean two-column
 *  layout.  Define VALUE_COL = 14 (or whatever fits) in config.h and use
 *  it everywhere for consistency.
 *
 *  USAGE:
 *    ghost_label_value(p, 5, 2, VALUE_COL, "SIGNAL",  "●●●○");
 *    ghost_label_value(p, 6, 2, VALUE_COL, "BATTERY", "▰▰▰▱  75%");
 *    ghost_label_value(p, 7, 2, VALUE_COL, "CARRIER", carrier_name);
 * ────────────────────────────────────────────────────────────────────────── */
static void ghost_label_value(struct ncplane *n,
                               int row, int label_col, int value_col,
                               const char *label, const char *value) {
    ghost_text(n, row, label_col, COL_GHOST_PCT, label);   /* dim */
    ghost_text(n, row, value_col, COL_GHOST_ON,  value);   /* bright */
}


/* ══════════════════════════════════════════════════════════════════════════
 *  SECTION 3: PHONE PLANE CREATION
 * ══════════════════════════════════════════════════════════════════════════ */

/* ──────────────────────────────────────────────────────────────────────────
 *  create_phone_plane()  —  Create the centred phone canvas
 *
 *  RETURNS: pointer to the new plane, or NULL on failure.
 *  Always check for NULL — using a NULL pointer crashes the program.
 *
 *  NOTCURSES: plane hierarchy
 *  ───────────────────────────
 *  Planes form a tree.  Each child plane is positioned RELATIVE to its
 *  parent.  Moving the parent moves all children with it.  Destroying
 *  the parent destroys all children too.
 *
 *  stdplane (full terminal)
 *    └── phone plane (centred child)
 *
 *  NOTCURSES: struct ncplane_options
 *  ────────────────────────────────────
 *  Configuration for ncplane_create():
 *    .y     top-left row, relative to parent
 *    .x     top-left column, relative to parent
 *    .rows  height in terminal rows
 *    .cols  width in terminal columns
 *    .name  debug name (appears in notcurses diagnostics, not on screen)
 *
 *  NOTCURSES: ncplane_dim_yx(plane, *rows, *cols)
 *  ─────────────────────────────────────────────────
 *  Reads the plane dimensions into variables via pointers.  '&' gets the
 *  address of a variable:
 *    &term_rows  is the memory address where term_rows lives
 *  The function writes the dimensions to those addresses.  After the call,
 *  term_rows and term_cols hold the terminal's current dimensions.
 *  This is "pass by pointer" — C's way for a function to "return" multiple
 *  values (since a function has only one actual return slot).
 *
 *  C CONCEPT: struct designated initializers
 *  ───────────────────────────────────────────
 *  { .field = value }  syntax sets only the named fields.
 *  All other fields are zero-initialised.  This is safer than positional
 *  initializers because adding a new field to the struct later won't
 *  silently shift your values.
 * ────────────────────────────────────────────────────────────────────────── */
static struct ncplane *create_phone_plane(struct ncplane *std) {
    unsigned term_rows, term_cols;
    ncplane_dim_yx(std, &term_rows, &term_cols);

    /*
     * CENTERING FORMULA: start = (total - object) / 2
     *
     * Integer division truncates toward zero, so on odd remainders the
     * object sits one cell toward top-left of perfect centre — invisible.
     *
     * We use signed int because subtraction could go negative if the
     * terminal is smaller than PHONE_ROWS / PHONE_COLS.  Clamping to 0
     * places the phone at the top-left in that case rather than crashing.
     */
    int start_y = ((int)term_rows - PHONE_ROWS) / 2;
    int start_x = ((int)term_cols - PHONE_COLS) / 2;
    if (start_y < 0) start_y = 0;
    if (start_x < 0) start_x = 0;

    struct ncplane_options opts = {
        .y    = start_y,
        .x    = start_x,
        .rows = PHONE_ROWS,
        .cols = PHONE_COLS,
        .name = "phone",
    };

    /*
     * ncplane_create(parent, options)
     * ────────────────────────────────
     * Allocates a new plane as a child of parent.
     * Returns NULL if allocation fails.
     * The returned pointer is the handle for all subsequent drawing.
     */
    return ncplane_create(std, &opts);
}


/* ══════════════════════════════════════════════════════════════════════════
 *  SECTION 4: STATUS BAR — GHOST ORIGINAL (E1)
 *
 *  Status bar occupies rows 0 (border) and 1 (content).
 *  Three functions:
 *    draw_battery()      left side  — battery glyphs + percentage
 *    draw_signal()       right side — signal circles, dynamically anchored
 *    draw_status_bar()   coordinator — reads hardware, calls both
 * ══════════════════════════════════════════════════════════════════════════ */

/* ──────────────────────────────────────────────────────────────────────────
 *  draw_battery()
 *
 *  Output examples:
 *    Normal    75%  →  ▰▰▰▱  75%
 *    Charging  60%  →  ▰▰▰▱  ⚡60%
 *    Low       12%  →  ▰▱▱▱  12%   (whole widget blinks)
 *    Empty      0%  →  ▱▱▱▱   0%
 *
 *  Glyphs:
 *    ▰  U+25B0  BLACK LOWER RIGHT TRIANGLE  — filled segment
 *    ▱  U+25B1  WHITE LOWER RIGHT TRIANGLE  — hollow segment
 *    ⚡  U+26A1  HIGH VOLTAGE SIGN           — charging
 *
 *  C CONCEPT: the modulo operator  %
 *  ───────────────────────────────────
 *  a % b  gives the REMAINDER after dividing a by b.
 *    10 % 3  = 1   (10 = 3×3 + 1)
 *     7 % 4  = 3   ( 7 = 4×1 + 3)
 *
 *  tick % 10  cycles 0,1,2,3,4,5,6,7,8,9,0,1,2,…
 *  (tick % 10) < 5  is true for exactly half of all ticks → 50% blink duty.
 *
 *  C CONCEPT: the ternary operator  ? :
 *  ──────────────────────────────────────
 *  condition ? value_if_true : value_if_false
 *  Produces a value — a compact if/else that can be used inside expressions.
 *    int max = (a > b) ? a : b;
 *  We use it to pick "▰" or "▱" per segment, and to pick a colour.
 *
 *  C CONCEPT: snprintf — safe string formatting
 *  ──────────────────────────────────────────────
 *  snprintf(buffer, max_bytes, format_string, values…)
 *  Like printf() but writes into a char array instead of the terminal.
 *  The 'n' limits output to max_bytes bytes (including '\0') — safe.
 *
 *  Format specifiers:
 *    %d    decimal integer
 *    %s    string (char *)
 *    %f    float  (use %.2f for 2 decimal places)
 *    %%    literal percent sign
 *
 *  Buffer sizing:
 *    "⚡100%\0" = 3 (⚡ = 3 UTF-8 bytes) + 4 ("100%") + 1 ('\0') = 8 bytes.
 *    char label[16] gives comfortable headroom.
 *
 *  C CONCEPT: char arrays (C strings)
 *  ────────────────────────────────────
 *  C has no built-in string type.  A string is a char array terminated
 *  by '\0' (the null byte).  "Hello" is stored as:
 *    ['H','e','l','l','o','\0']   — 6 bytes for 5 visible characters.
 *  Always allocate at least strlen + 1 bytes to accommodate '\0'.
 * ────────────────────────────────────────────────────────────────────────── */
static void draw_battery(struct ncplane *phone, int percent,
                         bool charging, int tick) {

    /* Map 0-100% → 0-4 filled segments using ceiling-like division */
    int segs = (percent + 24) / 25;
    if (segs > 4) segs = 4;
    if (segs < 0) segs = 0;

    /*
     * LOW BATTERY BLINK
     * tick % 10 cycles 0–9.  Ticks 0-4: visible.  Ticks 5-9: hidden.
     * We overwrite with spaces rather than ncplane_erase() because erase()
     * clears the ENTIRE plane (wiping clock and signal too).
     * 15 spaces covers the longest possible battery string comfortably.
     */
    if (percent < 15 && !charging) {
        if ((tick % 10) >= 5) {
            ghost_set(phone, COL_BG);
            ncplane_putstr_yx(phone, STATUS_ROW, STATUS_BATTERY_COL,
                              "               ");
            return;
        }
    }

    /* Four glyphs, one per iteration, drawn individually for per-glyph colour control */
    for (int i = 0; i < 4; i++) {
        ghost_set(phone, i < segs ? COL_GHOST_ON : COL_GHOST_OFF);
        ncplane_putstr_yx(phone, STATUS_ROW,
                          STATUS_BATTERY_COL + i,
                          i < segs ? "▰" : "▱");
    }

    /* Percentage label */
    char label[16];
    if (charging) {
        ghost_set(phone, COL_GHOST_ON);
        snprintf(label, sizeof(label), "⚡%d%%", percent);
    } else if (percent < 15) {
        ghost_set(phone, COL_GHOST_LOW);
        snprintf(label, sizeof(label), " %d%%", percent);
    } else {
        ghost_set(phone, COL_GHOST_PCT);
        snprintf(label, sizeof(label), " %d%%", percent);
    }
    ncplane_putstr_yx(phone, STATUS_ROW, STATUS_BATTERY_PCT_COL, label);
}

/* ──────────────────────────────────────────────────────────────────────────
 *  draw_signal()
 *
 *  Output:
 *    4 bars    →  ●●●●   (all filled, right-anchored)
 *    3 bars    →  ●●●○
 *    No signal →  ✕○○○   (✕ pulses between two dark greys)
 *
 *  Glyphs:
 *    ●  U+25CF  BLACK CIRCLE      — active bar
 *    ○  U+25CB  WHITE CIRCLE      — inactive bar
 *    ✕  U+2715  MULTIPLICATION X  — no-signal marker
 *
 *  RIGHT-ANCHORED POSITIONING
 *  ───────────────────────────
 *  Position is computed dynamically from the plane width at draw-time:
 *    sig_col = cols - 6
 *  This keeps signal flush against the right border regardless of PHONE_COLS
 *  or terminal resize.  NCKEY_RESIZE causes a redraw; the next call to
 *  draw_signal() picks up the new cols automatically.
 *
 *  Column layout (right edge):
 *    cols-7  →  ✕ prefix  (only when disconnected)
 *    cols-6  →  circle 0
 *    cols-5  →  circle 1
 *    cols-4  →  circle 2
 *    cols-3  →  circle 3
 *    cols-2  →  gap before border
 *    cols-1  →  right border ┃
 *
 *  C CONCEPT: casting  (int)unsigned_value
 *  ─────────────────────────────────────────
 *  cols is unsigned (plane width is never negative).  We subtract from it
 *  to find sig_col, which could theoretically go negative on a tiny terminal.
 *  Casting to int first makes the arithmetic signed so negative results are
 *  handled correctly (and we then guard with 'if (sig_col < 1) return').
 * ────────────────────────────────────────────────────────────────────────── */
static void draw_signal(struct ncplane *phone, int bars,
                        bool connected, int tick) {

    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    int sig_col    = (int)cols - 6;
    int prefix_col = (int)cols - 7;

    if (sig_col < 1) return;   /* plane too narrow — bail silently */

    if (!connected) {
        /*
         * Pulse ✕ between two barely-different dark greys.
         * The difference (0x242424 vs 0x383838) is intentionally subtle —
         * just enough to read as "scanning", not alarming.
         */
        uint32_t x_color = ((tick % 8) < 4) ? 0x242424 : 0x383838;
        ghost_set(phone, x_color);
        ncplane_putstr_yx(phone, STATUS_ROW, prefix_col, "✕");

        ghost_set(phone, COL_GHOST_OFF);
        for (int i = 0; i < 4; i++)
            ncplane_putstr_yx(phone, STATUS_ROW, sig_col + i, "○");
        return;
    }

    /* Erase prefix column — clears any leftover ✕ from disconnected state */
    ghost_set(phone, COL_BG);
    ncplane_putstr_yx(phone, STATUS_ROW, prefix_col, " ");

    for (int i = 0; i < 4; i++) {
        ghost_set(phone, i < bars ? COL_GHOST_ON : COL_GHOST_OFF);
        ncplane_putstr_yx(phone, STATUS_ROW, sig_col + i,
                          i < bars ? "●" : "○");
    }
}

/* ──────────────────────────────────────────────────────────────────────────
 *  draw_status_bar()  —  Read hardware state and draw both indicators
 *
 *  C CONCEPT: returning structs by value
 *  ──────────────────────────────────────
 *  hardware_get_battery() returns a battery_status_t struct.
 *  The ENTIRE struct is copied into the local variable 'batt'.
 *  For small structs (a few int/bool fields) this is fine — copying a
 *  handful of bytes is fast.  For large structs, pass a pointer instead.
 *
 *  Struct field access: batt.percent, batt.charging
 *  The '.' operator reads a named field from a struct variable.
 *  If you had a pointer to a struct: ptr->percent  (arrow operator).
 * ────────────────────────────────────────────────────────────────────────── */
static void draw_status_bar(struct ncplane *phone, int tick) {
    battery_status_t  batt = hardware_get_battery();
    cellular_status_t cell = hardware_get_cellular();
    draw_battery(phone, batt.percent,     batt.charging,  tick);
    draw_signal (phone, cell.signal_bars, cell.connected, tick);
}


/* ══════════════════════════════════════════════════════════════════════════
 *  SECTION 5: FRAME
 *
 *  draw_frame() is called at the start of EVERY frame before the active
 *  screen draws.  It owns the background, border, status bar, and separator.
 *  After it returns, screens draw into the content area (rows 3 onward).
 * ══════════════════════════════════════════════════════════════════════════ */

/* ──────────────────────────────────────────────────────────────────────────
 *  draw_frame()
 *
 *  PARAMETERS:
 *    phone        the phone plane
 *    tick         frame counter, forwarded to status bar for animation
 *    screen_name  short uppercase ASCII label centred in the separator
 *
 *  SEPARATOR FORMAT:
 *    ┣━━━━━ HOME ━━━━━━┫
 *    Centred by splitting remaining ━ cells into left_fill and right_fill.
 *
 *  NOTCURSES: ncplane_erase()
 *  ────────────────────────────
 *  Resets every cell in the plane to: space glyph, default colours, no style.
 *  MUST be called at the start of each frame so last frame's content doesn't
 *  bleed through where the new frame draws nothing.
 *
 *  NOTCURSES: double buffering
 *  ────────────────────────────
 *  All putstr / putchar calls write to an INTERNAL BUFFER, not the terminal.
 *  Nothing is visible until notcurses_render() is called.  This eliminates
 *  flickering — the terminal receives only the final composed frame.
 *
 *  NOTCURSES: border drawing with nccells
 *  ────────────────────────────────────────
 *  Step 1: Declare 6 nccell variables, one per box element:
 *            ul (upper-left corner)  ur (upper-right)  ll (lower-left)
 *            lr (lower-right)        hl (horizontal)   vl (vertical)
 *          Always initialise with NCCELL_TRIVIAL_INITIALIZER before use.
 *
 *  Step 2: Build a uint64_t 'channels' encoding the border colours.
 *          A channel packs fg + bg colours into one 64-bit integer.
 *          Manipulate with macros, never raw bit operations:
 *            ncchannels_set_fg_rgb(&channels, 0xRRGGBB)
 *            ncchannels_set_bg_rgb(&channels, 0xRRGGBB)
 *            ncchannels_set_bg_default(&channels)   ← transparent bg
 *
 *  Step 3: Load the 6 cells with box-drawing characters:
 *            nccells_heavy_box(plane, styles, channels, &ul, &ur, …)
 *          Alternatives:
 *            nccells_light_box()    ┌┐└┘─│   thin lines
 *            nccells_double_box()   ╔╗╚╝═║   double lines
 *            nccells_rounded_box()  ╭╮╰╯─│   rounded corners
 *
 *  Step 4: Draw the box:
 *            ncplane_cursor_move_yx(plane, 0, 0)   ← position cursor
 *            ncplane_box(plane, &ul,&ur,&ll,&lr,&hl,&vl, rows-1, cols-1, 0)
 *            'rows-1, cols-1' = bottom-right corner of the box.
 *            '0' = ctlword: draw all sides, no interior fill.
 *
 *  Step 5: Release cell memory:
 *            nccell_release(plane, &ul)  … for all 6 cells.
 *          nccells_heavy_box may allocate heap memory for multi-byte glyphs.
 *          Skipping release = memory leak.  Always release.
 *
 *  NOTCURSES: ncplane_set_bg_default()
 *  ─────────────────────────────────────
 *  Makes the background of subsequent cells TRANSPARENT — the cell shows
 *  whatever is on planes below it (in this case the terminal background).
 *  Use this for border cells so they don't show COL_BG as a coloured halo
 *  outside the phone frame against the terminal's own background.
 * ────────────────────────────────────────────────────────────────────────── */
static void draw_frame(struct ncplane *phone, int tick,
                       const char *screen_name) {

    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    ncplane_erase(phone);
    if (rows < (unsigned)FRAME_MIN_ROWS || cols < (unsigned)FRAME_MIN_COLS)
        return;

    /* ── Background fill — interior only, leave border cells transparent ── */
    ghost_fill_rect(phone, 1, 1, (int)rows - 2, (int)cols - 2, ' ', COL_BG, COL_BG);

    /* ── Heavy-line border ─────────────────────────────────────────────── */
    nccell ul = NCCELL_TRIVIAL_INITIALIZER;
    nccell ur = NCCELL_TRIVIAL_INITIALIZER;
    nccell ll = NCCELL_TRIVIAL_INITIALIZER;
    nccell lr = NCCELL_TRIVIAL_INITIALIZER;
    nccell hl = NCCELL_TRIVIAL_INITIALIZER;
    nccell vl = NCCELL_TRIVIAL_INITIALIZER;

    uint64_t channels = 0;
    ncchannels_set_bg_default(&channels);
    ncchannels_set_fg_rgb(&channels, COL_BORDER);
    nccells_heavy_box(phone, 0, channels, &ul, &ur, &ll, &lr, &hl, &vl);

    ncplane_cursor_move_yx(phone, 0, 0);
    ncplane_box(phone, &ul, &ur, &ll, &lr, &hl, &vl, rows - 1, cols - 1, 0);

    nccell_release(phone, &ul);
    nccell_release(phone, &ur);
    nccell_release(phone, &ll);
    nccell_release(phone, &lr);
    nccell_release(phone, &hl);
    nccell_release(phone, &vl);

    /* ── Status bar ────────────────────────────────────────────────────── */
    draw_status_bar(phone, tick);

    /* ── Centred screen-name separator ────────────────────────────────── */
    /*
     * CENTERING ALGORITHM
     * ────────────────────
     * inner      = cols - 2          ← interior columns (between T-junctions)
     * name_len   = strlen(name)      ← byte count (= column count for ASCII)
     * padded     = name_len + 2      ← name with one space on each side
     * left_fill  = (inner-padded)/2  ← ━ cells left of the name
     * right_fill = remainder         ← ━ cells right of the name
     *
     * Clamped to 0 so extremely long names don't produce negative loops.
     *
     * NOTE: strlen() counts BYTES not Unicode codepoints.  For ASCII screen
     * names ("HOME", "SETTINGS") bytes = columns.  If you ever use a
     * non-ASCII name you will need mbstowcs() or similar to count columns.
     */
    int inner      = (int)cols - 2;
    int name_len   = (int)strlen(screen_name);
    int padded     = name_len + 2;
    int left_fill  = (inner - padded) / 2;
    int right_fill = inner - padded - left_fill;
    if (left_fill  < 0) left_fill  = 0;
    if (right_fill < 0) right_fill = 0;

    /* Left T-junction — transparent bg so it blends with terminal border */
    ncplane_set_fg_rgb(phone, COL_SEPARATOR);
    ncplane_set_bg_default(phone);
    ncplane_putstr_yx(phone, 2, 0, "┣");

    /* Left ━ fill */
    ncplane_set_bg_rgb(phone, COL_BG);
    for (int x = 0; x < left_fill; x++)
        ncplane_putstr_yx(phone, 2, 1 + x, "━");

    /* Space + name + space */
    ncplane_putstr_yx(phone, 2, 1 + left_fill, " ");
    ncplane_set_fg_rgb(phone, COL_GHOST_PCT);
    ncplane_putstr_yx(phone, 2, 1 + left_fill + 1, screen_name);
    ncplane_set_fg_rgb(phone, COL_SEPARATOR);
    ncplane_putstr_yx(phone, 2, 1 + left_fill + 1 + name_len, " ");

    /* Right ━ fill */
    int right_start = 1 + left_fill + 1 + name_len + 1;
    for (int x = 0; x < right_fill; x++)
        ncplane_putstr_yx(phone, 2, right_start + x, "━");

    /* Right T-junction */
    ncplane_set_bg_default(phone);
    ncplane_putstr_yx(phone, 2, (int)cols - 1, "┫");
}


/* ══════════════════════════════════════════════════════════════════════════
 *  SECTION 6: MAIN
 *
 *  C CONCEPT: int main(void)
 *  ──────────────────────────
 *  The OS calls main() to start the program.
 *  Return 0 = success.  Return non-zero = error.
 *  Check with:  echo $?  in the shell after the program exits.
 *
 *  C CONCEPT: the event loop pattern
 *  ─────────────────────────────────────
 *  All interactive terminal programs follow this structure:
 *    1.  Initialise resources
 *    2.  LOOP until quit:
 *          a.  Draw current state to buffer
 *          b.  Render buffer to terminal
 *          c.  Block waiting for input  (CPU idle here — no busy-wait)
 *          d.  Update state based on input
 *    3.  Clean up resources
 *
 *  The 'tick' counter increments every frame and drives all animations.
 *  At 10fps, tick advances 10 per second.  tick % N cycles every N/10 sec.
 * ══════════════════════════════════════════════════════════════════════════ */

int main(void) {

    /* ── Locale — MUST be first, before any Unicode output ─────────────── */
    /*
     * setlocale(LC_ALL, "")
     * ──────────────────────
     * Sets the program's locale to the system default (read from the LANG
     * or LC_ALL environment variable).  On Raspbian this is typically
     * en_GB.UTF-8 or en_US.UTF-8 — both enable correct UTF-8 output.
     * LC_ALL affects all categories: encoding, numbers, dates, collation.
     */
    setlocale(LC_ALL, "");

    /* ── Hardware ───────────────────────────────────────────────────────── */
    hardware_init();
    settings_service_init();

    /* ── Notcurses initialisation ───────────────────────────────────────── */
    /*
     * struct notcurses_options
     * ─────────────────────────
     * Configuration for notcurses_init().  Unset fields default to 0.
     *
     * NCOPTION_SUPPRESS_BANNERS
     *   Prevents Notcurses printing "notcurses vX.Y.Z" on startup.
     *   Always use this in production so the terminal looks clean.
     *
     * Other useful flags to know:
     *   NCOPTION_NO_ALTERNATE_SCREEN  keep terminal scrollback (good during dev)
     *   NCOPTION_NO_WINCH_SIGHANDLER  don't auto-handle resize signals
     *   NCOPTION_NO_QUIT_SIGHANDLERS  don't auto-handle Ctrl-C
     */
    struct notcurses_options nc_opts = {
        .flags = NCOPTION_SUPPRESS_BANNERS,
    };

    /*
     * notcurses_init(options, output_stream)
     * ────────────────────────────────────────
     * output_stream = NULL means stdout (the terminal).
     *
     * What this does internally:
     *   • Queries terminal capabilities (colour depth, Unicode, box chars)
     *   • Enters the "alternate screen"  (your scrollback is preserved)
     *   • Enables raw input mode  (no line buffering, no echo)
     *   • Creates the standard plane covering the full terminal
     *
     * Returns NULL on failure.  Always check before using 'nc'.
     */
    struct notcurses *nc = notcurses_init(&nc_opts, NULL);
    if (!nc) {
        fprintf(stderr, "Notcurses init failed\n");
        return 1;
    }

    /*
     * notcurses_stdplane(nc)
     * ───────────────────────
     * Returns the standard plane — always exists after successful init,
     * covers the full terminal, is the root of the plane tree.
     * Never returns NULL.  Use as parent for our phone plane.
     */
    struct ncplane *std = notcurses_stdplane(nc);

    /* Small dev label on the std plane (visible outside the phone frame) */
    ncplane_set_fg_rgb(std, COL_DEV_LABEL);
    ncplane_putstr_yx(std, 0, 2, TEXT_DEV_LABEL);

    /* ── Phone plane ────────────────────────────────────────────────────── */
    struct ncplane *phone = create_phone_plane(std);
    if (!phone) {
        notcurses_stop(nc);
        return 1;
    }

    /* ── State ──────────────────────────────────────────────────────────── */
    /*
     * current_screen tracks which view is active.
     * Changing this in the input handler is the entire navigation system.
     * The enum type (screen_id) means the compiler will warn if you assign
     * a value that doesn't exist in the enum.
     */
    screen_id current_screen = SCREEN_HOME;
    screen_id previous_screen = (screen_id)-1;  /* force first-frame transition */

    /*
     * tick drives all animation.  Increments each frame.
     * int holds up to ~2.1 billion — won't overflow for years.
     * All animation uses tick % N so it loops forever.
     */
    int tick = 0;

    /* ── Event loop ─────────────────────────────────────────────────────── */
    /*
     * C CONCEPT: while (1)  —  infinite loop
     * ────────────────────────────────────────
     * 1 is always truthy in C, so this runs forever until a 'break' or
     * 'return' statement exits it.
     */
    while (1) {

        /* ── SCREEN TRANSITION ─────────────────────────────────────────── */
        /*
         * Track screen changes.  Currently used only for the screen_name
         * label in the separator.  When a screen needs init/cleanup,
         * add it here:
         *   if (previous == SCREEN_X) screen_x_destroy();
         *   if (current  == SCREEN_X) screen_x_init(phone);
         */
        previous_screen = current_screen;

        /* RESOLVE SCREEN NAME
         * ────────────────────
         * Map the current screen enum to the label shown in the separator.
         *
         * C CONCEPT: switch / case
         * ─────────────────────────
         * switch (expr) compares expr to each 'case' value and jumps to
         * the first match.  Execution runs until 'break' exits the switch.
         * Without 'break', execution falls through into the next case
         * (sometimes intentional, almost always a bug).
         * 'default' handles any value not matched by an explicit case —
         * always include it as a safety net.
         *
         * The compiler can generate a jump table for switch, making it O(1)
         * regardless of the number of cases.  An if/else chain is O(n).
         *
         * C CONCEPT: const char *
         * ────────────────────────
         * screen_name is a pointer to a string literal.  String literals
         * live in read-only memory — you can read them, never modify them.
         * 'const' enforces this at compile time.
         *
         * HOW TO ADD A NEW SCREEN NAME:
         *   case SCREEN_CALLS:   screen_name = "CALLS";   break;
         */
        const char *screen_name;
        switch (current_screen) {
            case SCREEN_HOME:     screen_name = "HOME";     break;
            case SCREEN_SETTINGS: screen_name = "SETTINGS"; break;
            case SCREEN_CALLS:    screen_name = "CALLS";    break;
            case SCREEN_MESSAGES: screen_name = "MESSAGES"; break;
            case SCREEN_CONTACTS: screen_name = "CONTACTS"; break;
            case SCREEN_MP3:      screen_name = "MP3";      break;
            case SCREEN_VOICE_MEMO: screen_name = "VOICE";  break;
            case SCREEN_NOTES:    screen_name = "NOTES";    break;
            default:              screen_name = "";           break;
        }

        /* ── DRAW PHASE ──────────────────────────────────────────────────── */
        /*
         * draw_frame() MUST come first — it calls ncplane_erase() which
         * clears the plane.  Screen draw functions then paint on top of the
         * cleared frame.  Never call ncplane_erase() in a screen function.
         *
         * PATTERN for all screen_*_draw() functions:
         * ────────────────────────────────────────────
         *   void screen_foo_draw(struct ncplane *phone) {
         *       // The content area starts at row CONTENT_ROW_START (= 3).
         *       // Left margin is col 2 (one inside border + one gap).
         *       // Right limit is PHONE_COLS - 3.
         *
         *       // Title
         *       ghost_text(phone, 3, 2, COL_GHOST_ON, "TITLE TEXT");
         *
         *       // Separator under title (optional)
         *       ghost_hline(phone, 4, 2, PHONE_COLS-4, "─", COL_SEPARATOR);
         *
         *       // Data rows using the label/value pattern
         *       ghost_label_value(phone, 5, 2, VALUE_COL, "LABEL", "value");
         *       ghost_label_value(phone, 6, 2, VALUE_COL, "LABEL2","value2");
         *
         *       // Key hint at bottom
         *       ghost_text(phone, PHONE_ROWS-3, 2, COL_HINT, "[↑↓] Scroll");
         *   }
         */
        draw_frame(phone, tick, screen_name);
        tick++;

        switch (current_screen) {
            case SCREEN_HOME:
                screen_home_draw(phone);
                break;
            case SCREEN_SETTINGS:
                screen_settings_draw(phone);
                break;
            case SCREEN_CALLS:
                screen_calls_draw(phone);
                break;
            case SCREEN_MESSAGES:
                screen_messages_draw(phone);
                break;
            case SCREEN_CONTACTS:
                screen_contacts_draw(phone);
                break;
            case SCREEN_MP3:
                screen_mp3_draw(phone);
                break;
            case SCREEN_VOICE_MEMO:
                screen_voice_memo_draw(phone);
                break;
            case SCREEN_NOTES:
                screen_notes_draw(phone);
                break;
            /*
             * HOW TO ADD A NEW SCREEN DRAW:
             *   case SCREEN_CALLS:
             *       screen_calls_draw(phone);
             *       break;
             */
            default:
                ghost_text(phone, 4, 3, COL_PLACEHOLDER, TEXT_COMING_SOON);
                ghost_text(phone, 6, 3, COL_HINT,        TEXT_GO_HOME);
                break;
        }

        /* ── RENDER PHASE ────────────────────────────────────────────────── */
        /*
         * notcurses_render(nc)
         * ─────────────────────
         * Composites all planes from bottom (stdplane) to top (phone plane),
         * diffs the result against the last rendered frame, and sends only
         * the terminal escape codes needed to update changed cells.
         *
         * This diff-and-patch approach is critical on slow links (UART, USB
         * serial to a Pi) — it minimises the bytes sent to the terminal.
         *
         * ALWAYS call this after all drawing for the frame is complete.
         */
        notcurses_render(nc);

        /* ── INPUT PHASE ─────────────────────────────────────────────────── */
        /*
         * notcurses_get_blocking(nc, &ni)
         * ─────────────────────────────────
         * Blocks until an input event arrives.  Returns the key code.
         * The CPU is completely idle during the wait — no busy-looping.
         * ni (ncinput) is filled with the full event details.
         *
         * KEY CODE REFERENCE:
         * ────────────────────
         * Regular characters:  'a' 'A' '1' ' ' '\n'  etc. (ASCII codepoints)
         * Arrow keys:          NCKEY_UP  NCKEY_DOWN  NCKEY_LEFT  NCKEY_RIGHT
         * Enter / Backspace:   NCKEY_ENTER  NCKEY_BACKSPACE
         * Escape:              NCKEY_ESC
         * Function keys:       NCKEY_F01 … NCKEY_F12
         * Terminal resized:    NCKEY_RESIZE   ← sent when window size changes
         *
         * Modifier check example:
         *   if (ni.modifiers & NCKEY_MOD_CTRL && key == 'c') { … }
         *
         * PHYSICAL KEYPAD (your hardware buttons):
         * Map button GPIO events to key codes in hardware.c, then handle
         * those codes in the switch below.  A typical mapping:
         *   UP button    → emit NCKEY_UP   (or 'k')
         *   DOWN button  → emit NCKEY_DOWN (or 'j')
         *   SELECT       → emit NCKEY_ENTER
         *   BACK/ESC     → emit NCKEY_ESC  (or 'b')
         */
        ncinput ni;
        uint32_t key = notcurses_get_blocking(nc, &ni);

        /* GLOBAL KEYS — handled before routing to screen
         *
         * C CONCEPT: continue
         * ────────────────────
         * Inside a loop, 'continue' skips the rest of the current iteration
         * and jumps back to the top of the loop (the while condition).
         * Use it when you've handled the input and want to immediately redraw.
         *
         * C CONCEPT: break (inside a loop)
         * ─────────────────────────────────
         * 'break' exits the nearest enclosing loop.
         * Here it exits the while(1) loop, falling through to cleanup.
         */
        if (key == NCKEY_RESIZE) { continue; }  /* redraw at new size */
        if (key == 'q' || key == 'Q') { break; } /* quit */
        if (key == 'h' || key == 'H') {
            current_screen = SCREEN_HOME;
            continue;
        }

        /* SCREEN-SPECIFIC INPUT ROUTING
         *
         * Each screen_*_input() receives the key and returns the new
         * screen_id.  If the screen doesn't handle the key, it returns
         * its own screen_id unchanged (a no-op navigation).
         *
         * PATTERN for all screen_*_input() functions:
         * ────────────────────────────────────────────
         *   screen_id screen_foo_input(uint32_t key) {
         *       switch (key) {
         *           case NCKEY_UP:
         *               // move selection up
         *               return SCREEN_FOO;   // stay on this screen
         *           case NCKEY_DOWN:
         *               // move selection down
         *               return SCREEN_FOO;
         *           case NCKEY_ENTER:
         *               // confirm selection → navigate somewhere
         *               return SCREEN_BAR;
         *           case NCKEY_ESC:
         *               return SCREEN_HOME;  // back to home
         *           default:
         *               return SCREEN_FOO;   // unhandled → stay
         *       }
         *   }
         *
         * NOTE: screen_*_input() should NOT draw anything.  Drawing
         * happens only in screen_*_draw(), called at the top of the loop.
         * Input functions only update state; the next loop iteration draws.
         *
         * HOW TO ADD INPUT FOR A NEW SCREEN:
         *   case SCREEN_CALLS:
         *       current_screen = screen_calls_input(key);
         *       break;
         */
        switch (current_screen) {
            case SCREEN_HOME:
                current_screen = screen_home_input(key);
                break;
            case SCREEN_SETTINGS:
                current_screen = screen_settings_input(key);
                break;
            case SCREEN_CALLS:
                current_screen = screen_calls_input(key);
                break;
            case SCREEN_MESSAGES:
                current_screen = screen_messages_input(key);
                break;
            case SCREEN_CONTACTS:
                current_screen = screen_contacts_input(key);
                break;
            case SCREEN_MP3:
                current_screen = screen_mp3_input(key);
                break;
            case SCREEN_VOICE_MEMO:
                current_screen = screen_voice_memo_input(key);
                break;
            case SCREEN_NOTES:
                current_screen = screen_notes_input(key);
                break;
            default:
                break;
        }
    }

    /* ── Cleanup — REVERSE order of creation ────────────────────────────── */
    /*
     * C CONCEPT: manual resource management
     * ──────────────────────────────────────
     * C has no garbage collector.  Every resource you acquire must be
     * explicitly released, in reverse order of acquisition.
     *
     * ncplane_destroy(plane)
     *   Frees this plane and all its children.
     *   After this, the 'phone' pointer is dangling — never use it again.
     *
     * notcurses_stop(nc)
     *   Destroys all remaining planes (including stdplane).
     *   Restores terminal to normal mode (exits raw input).
     *   Returns to normal screen (scrollback reappears).
     *   Frees all internal Notcurses memory.
     *   If you skip this after a crash, run 'reset' in the terminal to fix it.
     *
     * hardware_cleanup()
     *   Closes any I2C/UART file descriptors opened by hardware_init().
     */
    ncplane_destroy(phone);
    notcurses_stop(nc);
    settings_service_shutdown();
    hardware_cleanup();

    return 0;
}

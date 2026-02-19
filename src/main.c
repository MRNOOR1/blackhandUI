/*
 * ============================================================================
 *  main.c — BlackHand UI Entry Point
 * ============================================================================
 *
 *  WHAT IS THIS FILE?
 *  ------------------
 *  This is the main entry point for the BlackHand phone-style terminal UI.
 *  When you run the program, execution starts here at the main() function.
 *  This file handles:
 *    - Initializing the Notcurses library (our terminal graphics engine)
 *    - Creating the "phone" display area centered on screen
 *    - Drawing the border, status bar (battery/signal), and screen content
 *    - Processing keyboard input in an event loop
 *    - Cleaning up resources when the program exits
 *
 *
 *  WHAT IS NOTCURSES?
 *  ------------------
 *  Notcurses is a modern C library for creating rich terminal user interfaces.
 *  Think of it as a way to draw graphics in your terminal using text characters.
 *
 *  Key concepts:
 *    - PLANES: Rectangular drawing surfaces (like layers in Photoshop)
 *    - CELLS:  Individual character positions with color and style
 *    - RENDER: Pushing your drawings to the actual terminal display
 *
 *  Documentation: https://notcurses.com/
 *  GitHub: https://github.com/dankamongmen/notcurses
 *
 *
 *  C CONCEPTS YOU'LL LEARN IN THIS FILE:
 *  -------------------------------------
 *  1. #include directives - importing code from other files
 *  2. Functions - reusable blocks of code
 *  3. Pointers (*) - variables that store memory addresses
 *  4. Structs - grouping related data together
 *  5. The main() function - where every C program starts
 *  6. while loops - repeating code until a condition is false
 *  7. switch/case - choosing between multiple options
 *
 *
 *  HOW TO MODIFY THIS CODE:
 *  ------------------------
 *  - To change colors: Edit config.h (all colors defined there)
 *  - To resize the phone: Change PHONE_ROWS/PHONE_COLS in config.h
 *  - To add a new screen: See the switch statement in the main loop
 *  - To change status bar position: Edit STATUS_ROW in config.h
 *
 * ============================================================================
 */

/* ═══════════════════════════════════════════════════════════════════════════
 *  SECTION 1: HEADER INCLUDES
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *  In C, we use #include to import code from other files. This is similar to
 *  "import" in Python or "require" in JavaScript.
 *
 *  Two types of includes:
 *    #include <file.h>  - System/library headers (in system directories)
 *    #include "file.h"  - Our own project headers (in our source directory)
 *
 *  Headers (.h files) contain DECLARATIONS - they tell the compiler what
 *  functions and types exist. The actual code is in .c files.
 */

/*
 * <locale.h> - C Standard Library
 *
 * PURPOSE: Allows setting the program's "locale" (language/region settings).
 * WHY WE NEED IT: We call setlocale() so the terminal correctly displays
 *                 Unicode characters like our box-drawing symbols (┏━┓).
 *
 * KEY FUNCTION:
 *   setlocale(LC_ALL, "") - Use the user's system locale settings
 *
 * EXAMPLE:
 *   setlocale(LC_ALL, "");           // Use system default (recommended)
 *   setlocale(LC_ALL, "en_US.UTF-8"); // Force US English UTF-8
 */
#include <locale.h>

/*
 * <notcurses/notcurses.h> - Notcurses Library (Third-Party)
 *
 * PURPOSE: The main header for the Notcurses terminal graphics library.
 * WHY WE NEED IT: Provides ALL Notcurses types and functions.
 *
 * KEY TYPES:
 *   struct notcurses     - The main library context (like a "game engine")
 *   struct ncplane       - A drawing surface (like a canvas or layer)
 *   struct ncplane_options - Configuration for creating new planes
 *   nccell               - A single character cell with color and style
 *   ncinput              - Keyboard/mouse input data
 *
 * KEY FUNCTIONS:
 *   notcurses_init()       - Start the library
 *   notcurses_stop()       - Shut down the library
 *   notcurses_render()     - Display everything to the terminal
 *   notcurses_get_blocking() - Wait for keyboard input
 *   ncplane_putstr_yx()    - Draw text at a specific position
 *   ncplane_set_fg_rgb()   - Set text (foreground) color
 *   ncplane_set_bg_rgb()   - Set background color
 */
#include <notcurses/notcurses.h>

/*
 * <stdint.h> - C Standard Library
 *
 * PURPOSE: Provides fixed-width integer types.
 * WHY WE NEED IT: Notcurses uses uint32_t for colors and key codes.
 *
 * KEY TYPES:
 *   uint32_t - Unsigned 32-bit integer (0 to 4,294,967,295)
 *   int32_t  - Signed 32-bit integer (-2 billion to +2 billion)
 *   uint8_t  - Unsigned 8-bit integer (0 to 255, good for bytes)
 *
 * WHY "UNSIGNED"?
 *   - "signed" integers can be negative (-5, -100, etc.)
 *   - "unsigned" integers are always >= 0 (useful for counts, sizes)
 *
 * EXAMPLE:
 *   uint32_t color = 0xFF0000;  // Red in RGB (no negative colors!)
 *   int32_t temperature = -40;  // Can be negative
 */
#include <stdint.h>

/*
 * <stdio.h> - C Standard Library (Standard Input/Output)
 *
 * PURPOSE: Basic input/output operations (printing, reading).
 * WHY WE NEED IT: We use fprintf(stderr, ...) to print error messages.
 *
 * KEY FUNCTIONS:
 *   printf(format, ...)         - Print to standard output (stdout)
 *   fprintf(file, format, ...)  - Print to a specific file/stream
 *   snprintf(buf, size, format, ...) - Print to a string buffer (safe)
 *
 * KEY STREAMS:
 *   stdout - Standard output (normal program output)
 *   stderr - Standard error (error messages, separate from stdout)
 *
 * EXAMPLE:
 *   printf("Hello %s!\n", "World");      // Output: Hello World!
 *   fprintf(stderr, "Error: %d\n", 42);  // Prints error to stderr
 */
#include <stdio.h>

/*
 * <string.h> - C Standard Library
 *
 * PURPOSE: String manipulation functions.
 * WHY WE NEED IT: We use strlen() to measure text length for alignment.
 *
 * KEY FUNCTIONS:
 *   strlen(str)              - Get length of string (not including \0)
 *   strcpy(dest, src)        - Copy string (UNSAFE - use strncpy instead)
 *   strncpy(dest, src, n)    - Copy at most n characters (SAFER)
 *   strcmp(a, b)             - Compare strings (0 if equal)
 *   strcat(dest, src)        - Append src to dest (UNSAFE)
 *
 * C STRINGS EXPLAINED:
 *   In C, strings are arrays of characters ending with '\0' (null byte).
 *   "Hello" is actually stored as: ['H','e','l','l','o','\0']
 *
 * EXAMPLE:
 *   const char *name = "Alice";
 *   size_t len = strlen(name);  // len = 5 (doesn't count '\0')
 */
#include <string.h>

/*
 * <stdbool.h> - C Standard Library (C99+)
 *
 * PURPOSE: Provides the 'bool' type with 'true' and 'false' values.
 * WHY WE NEED IT: Makes code more readable than using 0/1 for booleans.
 *
 * WHAT IT DEFINES:
 *   bool  - A boolean type (actually just an int underneath)
 *   true  - The value 1
 *   false - The value 0
 *
 * EXAMPLE:
 *   bool is_charging = true;
 *   if (is_charging) {
 *       printf("Battery is charging\n");
 *   }
 *
 * NOTE: Before C99, programmers used int with 0/1, or defined their own.
 */
#include <stdbool.h>

/*
 * "ui.h" - Our Project Header
 *
 * PURPOSE: Declares screen-related functions and types.
 * CONTAINS:
 *   - screen_id enum (SCREEN_HOME, SCREEN_SETTINGS, etc.)
 *   - screen_home_draw() declaration
 *   - screen_settings_draw() declaration
 *   - screen_home_input() declaration
 *
 * WHY SEPARATE HEADER?
 *   Headers let us DECLARE functions in one place and DEFINE them elsewhere.
 *   This way, main.c can call screen_home_draw() even though the actual
 *   code is in screen_home.c. The compiler sees the declaration in ui.h
 *   and trusts that the definition exists somewhere.
 */
#include "ui.h"

/*
 * "config.h" - Our Project Header
 *
 * PURPOSE: Central configuration file for the entire UI.
 * CONTAINS:
 *   - PHONE_ROWS, PHONE_COLS (phone dimensions)
 *   - COL_* (all color values as 0xRRGGBB hex)
 *   - Layout constants (row positions, column offsets)
 *   - Text strings (labels, button hints)
 *
 * WHY A SEPARATE CONFIG FILE?
 *   Putting all settings in one place makes it easy to:
 *   - Change the color theme by editing one file
 *   - Resize the UI without hunting through code
 *   - Translate text labels to another language
 */
#include "config.h"

/*
 * "hardware.h" - Our Project Header
 *
 * PURPOSE: Hardware abstraction layer for battery and cellular status.
 * CONTAINS:
 *   - battery_status_t struct (percent, charging)
 *   - cellular_status_t struct (signal_bars, connected, carrier)
 *   - hardware_get_battery() - Get current battery status
 *   - hardware_get_cellular() - Get current signal status
 *   - hardware_init() / hardware_cleanup() - Setup/teardown
 *
 * WHY ABSTRACT HARDWARE?
 *   By defining an interface here, we can:
 *   - Use fake/simulated values during development
 *   - Later replace with real hardware reads (I2C, UART, GPIO)
 *   - The UI code doesn't need to change - only hardware.c
 */
#include "hardware.h"


/* ═══════════════════════════════════════════════════════════════════════════
 *  SECTION 2: HELPER FUNCTIONS
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *  C CONCEPT: FUNCTIONS
 *  --------------------
 *  A function is a reusable block of code. Format:
 *
 *    return_type function_name(parameters) {
 *        // code here
 *        return value;  // if return_type is not void
 *    }
 *
 *  EXAMPLE:
 *    int add(int a, int b) {
 *        return a + b;
 *    }
 *    int result = add(3, 4);  // result = 7
 *
 *
 *  C CONCEPT: STATIC FUNCTIONS
 *  ---------------------------
 *  The 'static' keyword before a function means "private to this file."
 *  Other .c files cannot call this function. It's like making a function
 *  "private" in object-oriented languages.
 *
 *  WHY USE STATIC?
 *  - Prevents name collisions (two files can have static functions with same name)
 *  - Signals to readers "this is internal, not part of the public API"
 *  - May help the compiler optimize
 */

/* ───────────────────────────────────────────────────────────────────────────
 *  create_phone_plane() - Create the centered phone display area
 * ───────────────────────────────────────────────────────────────────────────
 *
 *  WHAT IT DOES:
 *  Creates a new "plane" (drawing surface) centered on the terminal.
 *  This plane represents our phone screen where we draw everything.
 *
 *  NOTCURSES CONCEPT: PLANES
 *  -------------------------
 *  A "plane" is a rectangular grid of character cells that you draw on.
 *  Think of planes like transparent layers in Photoshop:
 *    - You can have multiple planes stacked on top of each other
 *    - Each plane can be positioned anywhere on the terminal
 *    - When you render, all planes are composited together
 *
 *  The "standard plane" (stdplane) covers the entire terminal and always
 *  exists. We create a CHILD plane on top of it for our phone UI.
 *
 *  C CONCEPT: POINTERS
 *  -------------------
 *  "struct ncplane *std" means "std is a pointer to an ncplane."
 *
 *  A pointer holds a MEMORY ADDRESS, not the actual data.
 *  Think of it like a street address vs. the actual house:
 *    - The address (pointer) tells you WHERE the house is
 *    - The house (data) is the actual thing at that location
 *
 *  Why use pointers?
 *    - Efficiency: Pass the address instead of copying large data
 *    - Modification: Functions can modify data at that address
 *    - Dynamic memory: Create data whose size isn't known at compile time
 *
 *  SYNTAX:
 *    int x = 42;       // x is an integer with value 42
 *    int *p = &x;      // p is a pointer holding the address of x
 *    *p = 100;         // Dereference p, change x to 100
 *    printf("%d", x);  // Prints 100
 *
 *
 *  PARAMETERS:
 *    std - Pointer to the standard plane (the full terminal)
 *
 *  RETURNS:
 *    Pointer to the new phone plane, or NULL if creation failed
 *
 *  HOW TO MODIFY:
 *    - Change phone size: Edit PHONE_ROWS/PHONE_COLS in config.h
 *    - Change position: Modify the start_y/start_x calculation
 *    - Add a plane name: Change the .name field for debugging
 */
static struct ncplane *create_phone_plane(struct ncplane *std) {
    /*
     * Declare variables to hold the terminal dimensions.
     * "unsigned" means non-negative (can't have negative rows/columns).
     */
    unsigned term_rows, term_cols;

    /*
     * ncplane_dim_yx() - Get the dimensions of a plane
     *
     * NOTCURSES FUNCTION:
     *   void ncplane_dim_yx(const struct ncplane *n,
     *                       unsigned *rows, unsigned *cols);
     *
     * The "&" operator gets the ADDRESS of a variable. This lets the
     * function write values INTO our variables (called "pass by reference").
     *
     * After this call:
     *   term_rows = height of terminal (e.g., 40)
     *   term_cols = width of terminal (e.g., 120)
     */
    ncplane_dim_yx(std, &term_rows, &term_cols);

    /*
     * Calculate the position to center the phone on screen.
     *
     * CENTERING FORMULA:
     *   position = (total_size - object_size) / 2
     *
     * EXAMPLE: Terminal is 120 columns, phone is 50 columns
     *   start_x = (120 - 50) / 2 = 35
     *   So the phone starts at column 35, leaving 35 columns on each side.
     *
     * We use "int" (signed) because the subtraction could go negative
     * if the terminal is smaller than the phone.
     */
    int start_y = (term_rows - PHONE_ROWS) / 2;  /* Vertical position */
    int start_x = (term_cols - PHONE_COLS) / 2;  /* Horizontal position */

    /*
     * SAFETY CHECK: Clamp negative values to 0.
     * If terminal is smaller than phone, position at top-left corner
     * instead of using a negative (invalid) position.
     */
    if (start_y < 0) start_y = 0;
    if (start_x < 0) start_x = 0;

    /*
     * Create an options struct to configure the new plane.
     *
     * C CONCEPT: STRUCT INITIALIZATION
     * ---------------------------------
     * This syntax { .field = value } is called "designated initializer."
     * It lets you set specific fields by name. Unmentioned fields are
     * automatically set to 0/NULL.
     *
     * ALTERNATIVE SYNTAX (positional, harder to read):
     *   struct ncplane_options opts = { start_y, start_x, PHONE_ROWS, ... };
     *
     * NOTCURSES: ncplane_options FIELDS
     * ---------------------------------
     *   .y     - Row position relative to parent plane
     *   .x     - Column position relative to parent plane
     *   .rows  - Height of the plane in rows
     *   .cols  - Width of the plane in columns
     *   .name  - Debug name (shows up in notcurses diagnostics)
     *   .userptr - Custom data pointer (we don't use this)
     *   .flags - Special flags (0 = defaults)
     */
    struct ncplane_options opts = {
        .y    = start_y,
        .x    = start_x,
        .rows = PHONE_ROWS,
        .cols = PHONE_COLS,
        .name = "phone",
    };

    /*
     * ncplane_create() - Create a new child plane
     *
     * NOTCURSES FUNCTION:
     *   struct ncplane *ncplane_create(struct ncplane *parent,
     *                                  const struct ncplane_options *opts);
     *
     * Creates a new plane as a child of 'parent'. The child's position
     * is relative to the parent (so y=5 means 5 rows from parent's top).
     *
     * RETURNS:
     *   - Pointer to the new plane on success
     *   - NULL on failure (always check!)
     *
     * MEMORY NOTE:
     *   Notcurses owns this memory. We must call ncplane_destroy()
     *   when done, otherwise it's cleaned up when we call notcurses_stop().
     */
    return ncplane_create(std, &opts);
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  STATUS BAR — GHOST ORIGINAL (E1)
 *  Black Hand OS · status_bar.c
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *  DESIGN LANGUAGE
 *  ───────────────
 *  Everything that is OFF is nearly invisible — very dark grey, not truly
 *  black, so the glyph structure is still legible up close but vanishes at
 *  arm's length.  Everything ON is a clean off-white (#E0E0E0), never pure
 *  white, which would be harsh against the black background.
 *
 *  BATTERY   ▰▰▰▱  75%
 *    ▰  (U+25B0)  BLACK LOWER RIGHT TRIANGLE — "filled" segment
 *    ▱  (U+25B1)  WHITE LOWER RIGHT TRIANGLE — "hollow" segment
 *    Percentage is printed in a near-invisible dark grey so it takes
 *    no visual priority over the glyph bar itself.
 *    Below 15%: percentage shifts to a deep red.
 *
 *  SIGNAL    ●●●○
 *    ●  (U+25CF)  BLACK CIRCLE — active bar
 *    ○  (U+25CB)  WHITE CIRCLE — inactive bar
 *    No-signal: all four circles shown hollow + a single dim "✕" prefix.
 *
 *  COLOUR PALETTE (24-bit RGB)
 *  ───────────────────────────
 *  Add these to config.h:
 *
 *    #define COL_GHOST_ON       0xE0E0E0   // active glyph — off-white
 *    #define COL_GHOST_OFF      0x1E1E1E   // inactive glyph — near-invisible
 *    #define COL_GHOST_PCT      0x2C2C2C   // percentage text — very dark
 *    #define COL_GHOST_LOW      0x7F1D1D   // low battery percentage — deep red
 *    #define COL_BG             0x080808   // background
 *
 *  ANIMATION
 *  ─────────
 *  Both functions accept `tick` — an int your render loop increments each
 *  frame.  Used for:
 *    • Low battery blink  (below 15%, not charging)
 *    • No-signal pulse    (the ✕ fades in/out)
 *
 *  RENDER LOOP EXAMPLE
 *  ───────────────────
 *    int tick = 0;
 *    while (running) {
 *        draw_battery(phone, battery_pct, charging, tick);
 *        draw_signal (phone, signal_bars, connected,  tick);
 *        notcurses_render(nc);
 *        usleep(100000);  // 100 ms → 10 fps
 *        tick++;
 *    }
 */


/* ───────────────────────────────────────────────────────────────────────────
 *  INTERNAL HELPERS
 * ───────────────────────────────────────────────────────────────────────────
 *
 *  ghost_set() is a tiny convenience wrapper that sets fg+bg together.
 *  We call it constantly, so collapsing it cuts visual noise in the
 *  drawing functions below.
 *
 *  C CONCEPT: STATIC FUNCTIONS
 *  ────────────────────────────
 *  `static` here means the function is private to this translation unit
 *  (this .c file).  It won't be visible to other files — keeps our
 *  namespace clean.
 */
static void ghost_set(struct ncplane *n, uint32_t fg) {
    ncplane_set_fg_rgb(n, fg);
    ncplane_set_bg_rgb(n, COL_BG);
}


/* ───────────────────────────────────────────────────────────────────────────
 *  draw_battery()
 * ───────────────────────────────────────────────────────────────────────────
 *
 *  OUTPUT EXAMPLES
 *  ───────────────
 *    Normal   100% →  ▰▰▰▰  100%
 *    Normal    75% →  ▰▰▰▱   75%
 *    Normal    50% →  ▰▰▱▱   50%
 *    Normal    25% →  ▰▱▱▱   25%
 *    Low       12% →  ▰▱▱▱   12%   ← percentage blinks red
 *    Charging  60% →  ▰▰▰▱  ⚡60%
 *
 *  PARAMETERS
 *  ──────────
 *    phone     ncplane to draw on
 *    percent   battery level 0–100
 *    charging  true while plugged in
 *    tick      frame counter from render loop
 */
static void draw_battery(struct ncplane *phone, int percent,
                         bool charging, int tick) {

    /* ── SEGMENTS ──────────────────────────────────────────────────────── */
    /*
     * Map 0–100% onto 0–4 filled segments.
     *
     * FORMULA: (percent + 24) / 25
     *   0–24%   → 0 segments  (we clamp up to 1 so bar is never all-hollow
     *                           unless percent == 0, which looks intentional)
     *   25–49%  → 1 segment
     *   50–74%  → 2 segments
     *   75–99%  → 3 segments
     *   100%    → 4 segments
     *
     * We DON'T clamp 0 up to 1 deliberately — if the battery reads 0%,
     * all four glyphs are hollow, which is a valid and readable state.
     */
    int segs = (percent + 24) / 25;
    if (segs > 4) segs = 4;
    if (segs < 0) segs = 0;

    /* ── LOW BATTERY BLINK ─────────────────────────────────────────────── */
    /*
     * Below 15% and not charging: blink the whole widget.
     *
     * tick % 10 cycles 0→9.
     *   0–4  = widget visible
     *   5–9  = widget hidden (spaces printed over it)
     *
     * We return early on the "hidden" phase so nothing is drawn.
     * On the "visible" phase we fall through to normal drawing.
     *
     * WHY SPACES AND NOT ncplane_erase?
     * ncplane_erase() clears the ENTIRE plane, which would wipe the
     * clock and signal too.  Printing spaces over the exact columns
     * erases only what we own.
     *
     * HOW MANY SPACES?
     * "▰▰▰▱  12%" is about 10 visible columns.  15 spaces is safe.
     */
    if (percent < 15 && !charging) {
        if ((tick % 10) >= 5) {
            ghost_set(phone, COL_BG);
            ncplane_putstr_yx(phone, STATUS_ROW, STATUS_BATTERY_COL,
                              "               ");   /* 15 spaces */
            return;
        }
    }

    /* ── DRAW THE FOUR GLYPHS ──────────────────────────────────────────── */
    /*
     * We draw each glyph individually rather than building a string first.
     * This lets us set fg colour per-glyph in the future if needed
     * (e.g. a gradient effect), and avoids the multi-byte bookkeeping
     * of a manually assembled UTF-8 string.
     *
     * ncplane_putstr_yx() advances the cursor after each call, so
     * successive calls at the same (row, col+offset) aren't needed —
     * we can just call ncplane_putstr() for glyphs 1–3 after positioning
     * for glyph 0.  Using _yx for each is explicit and safer.
     *
     * Each glyph (▰ or ▱) is 3 UTF-8 bytes and occupies 1 terminal column.
     */
    for (int i = 0; i < 4; i++) {
        ghost_set(phone, i < segs ? COL_GHOST_ON : COL_GHOST_OFF);
        ncplane_putstr_yx(phone, STATUS_ROW,
                          STATUS_BATTERY_COL + i,
                          i < segs ? "▰" : "▱");
    }

    /* ── PERCENTAGE LABEL ──────────────────────────────────────────────── */
    /*
     * Printed two columns after the last glyph (col+4 = one space gap,
     * col+5 onward = digits).  STATUS_BATTERY_PCT_COL should be defined
     * as STATUS_BATTERY_COL + 5 in config.h.
     *
     * Colour rules:
     *   < 15% and not charging  → COL_GHOST_LOW  (deep red)
     *   charging                → COL_GHOST_ON   (normal, ⚡ prefix)
     *   otherwise               → COL_GHOST_PCT  (near-invisible dark)
     *
     * C CONCEPT: char arrays and snprintf
     * ─────────────────────────────────────
     * We need to build "⚡75%" or " 75%" as a string.
     * ⚡ (U+26A1) is 3 UTF-8 bytes.
     * "⚡100%" = 3 + 4 + 1(NUL) = 8 bytes.
     * "  100%" = 1 + 4 + 1(NUL) = 7 bytes.
     * char label[16] gives comfortable margin.
     */
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


/* ───────────────────────────────────────────────────────────────────────────
 *  draw_signal()
 * ───────────────────────────────────────────────────────────────────────────
 *
 *  OUTPUT EXAMPLES
 *  ───────────────
 *    4 bars   →  ●●●●
 *    3 bars   →  ●●●○
 *    1 bar    →  ●○○○
 *    No signal→  ✕○○○   ← ✕ pulses dim/invisible
 *
 *  NO-SIGNAL STATE
 *  ───────────────
 *  We don't replace the circles with text.  Instead the four circles all
 *  go hollow and a ✕ (U+2715) appears in the column just before them,
 *  pulsing between COL_GHOST_OFF and a slightly brighter dim grey
 *  so it draws the eye without being alarming.
 *
 *  PARAMETERS
 *  ──────────
 *    phone      ncplane to draw on
 *    bars       signal strength 0–4
 *    connected  true if registered to any network
 *    tick       frame counter from render loop
 */
static void draw_signal(struct ncplane *phone, int bars,
                        bool connected, int tick) {

    /* ── NO SIGNAL ─────────────────────────────────────────────────────── */
    if (!connected) {
        /*
         * Pulse the ✕ between two shades of dark grey.
         *
         * tick % 8:  0-3 = dimmer shade, 4-7 = slightly brighter
         * This is a very subtle pulse — enough to signal "scanning"
         * without being distracting.
         *
         * STATUS_SIGNAL_PREFIX_COL = STATUS_SIGNAL_COL - 1
         * (one column left of where the circles start)
         */
        uint32_t x_color = ((tick % 8) < 4) ? 0x242424 : 0x383838;
        ghost_set(phone, x_color);
        ncplane_putstr_yx(phone, STATUS_ROW, STATUS_SIGNAL_PREFIX_COL, "✕");

        /* All four circles hollow */
        ghost_set(phone, COL_GHOST_OFF);
        for (int i = 0; i < 4; i++) {
            ncplane_putstr_yx(phone, STATUS_ROW,
                              STATUS_SIGNAL_COL + i, "○");
        }
        return;
    }

    /* ── CONNECTED ─────────────────────────────────────────────────────── */
    /*
     * Erase the prefix column in case we were previously disconnected
     * and a ✕ is sitting there.
     */
    ghost_set(phone, COL_BG);
    ncplane_putstr_yx(phone, STATUS_ROW, STATUS_SIGNAL_PREFIX_COL, " ");

    /*
     * Draw four circles: ● for active (i < bars), ○ for inactive.
     *
     * Both ● (U+25CF) and ○ (U+25CB) are 3-byte UTF-8 sequences that
     * occupy exactly 1 terminal column each, so column arithmetic is
     * straightforward: STATUS_SIGNAL_COL + i.
     */
    for (int i = 0; i < 4; i++) {
        ghost_set(phone, i < bars ? COL_GHOST_ON : COL_GHOST_OFF);
        ncplane_putstr_yx(phone, STATUS_ROW,
                          STATUS_SIGNAL_COL + i,
                          i < bars ? "●" : "○");
    }
}
/* ───────────────────────────────────────────────────────────────────────────
 *  draw_status_bar() - Draw the complete status bar (battery + signal)
 * ───────────────────────────────────────────────────────────────────────────
 *
 *  WHAT IT DOES:
 *  Reads current hardware status and draws both indicators.
 *  This is a "convenience function" that combines multiple operations.
 *
 *  DESIGN PATTERN: FACADE
 *  ----------------------
 *  This function hides the complexity of getting hardware data and
 *  drawing multiple elements. Callers just call draw_status_bar()
 *  without worrying about the details.
 */
static void draw_status_bar(struct ncplane *phone, int tick) {
    /*
     * Get current hardware status.
     *
     * These functions return STRUCTS (not pointers). The entire struct
     * is COPIED into our local variables. This is fine for small structs
     * but inefficient for large ones (which would use pointers instead).
     *
     * See hardware.h for struct definitions.
     */
    battery_status_t  batt = hardware_get_battery();
    cellular_status_t cell = hardware_get_cellular();

    /* Draw both indicators */
    draw_battery(phone, batt.percent, batt.charging, tick);
    draw_signal(phone, cell.signal_bars, cell.connected, tick);
}


/* ───────────────────────────────────────────────────────────────────────────
 *  draw_frame() - Draw the phone border, status bar, and separators
 * ───────────────────────────────────────────────────────────────────────────
 *
 *  WHAT IT DOES:
 *  Draws the static "chrome" of the phone UI:
 *    1. Fills the background with COL_BG
 *    2. Draws a heavy Unicode border around the edge
 *    3. Draws the status bar (battery/signal)
 *    4. Draws separator lines (below status bar, above footer)
 *    5. Draws the footer text ("[q]Quit")
 *
 *  CALLED: Every frame, before drawing the active screen's content
 *
 *  NOTCURSES CONCEPT: DOUBLE BUFFERING
 *  -----------------------------------
 *  All our drawing goes to an off-screen buffer. Nothing appears on the
 *  terminal until we call notcurses_render(). This prevents flicker and
 *  allows the library to optimize updates (only redraw changed parts).
 *
 *  HOW TO MODIFY:
 *    - Change border style: Use nccells_light_box, nccells_double_box, etc.
 *    - Remove footer: Delete the footer drawing code at the bottom
 *    - Add a header title: Add ncplane_putstr_yx() after the border
 */
static void draw_frame(struct ncplane *phone, int tick) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    /*
     * ncplane_erase() - Clear the entire plane
     *
     * NOTCURSES FUNCTION:
     *   void ncplane_erase(struct ncplane *n);
     *
     * Resets all cells to blank (space character, default colors).
     * We do this first so previous frame's content doesn't bleed through.
     *
     * NOTE: erase() uses the plane's "base cell" colors, not the current
     * fg/bg colors. That's why we also do the fill loop below.
     */
    ncplane_erase(phone);

    /* Skip drawing if plane is too small (prevents garbled output) */
    if (rows < FRAME_MIN_ROWS || cols < FRAME_MIN_COLS) return;

    /*
     * STEP 1: Fill the interior with background color
     * ------------------------------------------------
     * We only fill the cells INSIDE the border (rows 1..rows-2,
     * cols 1..cols-2) with COL_BG.  The border cells (row 0, last row,
     * col 0, last col) are left at the default so they don't show a
     * coloured halo against the terminal background.
     *
     * ncplane_putchar_yx(plane, row, col, char)
     *   Draws a single character at the specified position.
     *   Uses the currently-set fg/bg colors.
     */
    ncplane_set_bg_rgb(phone, COL_BG);
    for (unsigned y = 1; y < rows - 1; y++) {
        for (unsigned x = 1; x < cols - 1; x++) {
            ncplane_putchar_yx(phone, y, x, ' ');
        }
    }

    /*
     * STEP 2: Draw the border
     * -----------------------
     * Notcurses provides functions to draw boxes using Unicode box-drawing
     * characters. We need to:
     *   1. Create 6 cells for the box parts (corners + edges)
     *   2. Load them with the box-drawing characters
     *   3. Draw the box
     *   4. Release the cells (free any internal memory)
     *
     * NOTCURSES: nccell
     * -----------------
     * An nccell represents one character cell with:
     *   - The character/glyph (can be multi-byte Unicode)
     *   - Foreground color
     *   - Background color
     *   - Style attributes (bold, italic, etc.)
     *
     * NCCELL_TRIVIAL_INITIALIZER
     *   Macro that zeros out a cell. Always initialize cells before use!
     */
    ncplane_set_fg_rgb(phone, COL_BORDER);

    nccell ul = NCCELL_TRIVIAL_INITIALIZER;  /* Upper-left corner:  ┏ */
    nccell ur = NCCELL_TRIVIAL_INITIALIZER;  /* Upper-right corner: ┓ */
    nccell ll = NCCELL_TRIVIAL_INITIALIZER;  /* Lower-left corner:  ┗ */
    nccell lr = NCCELL_TRIVIAL_INITIALIZER;  /* Lower-right corner: ┛ */
    nccell hl = NCCELL_TRIVIAL_INITIALIZER;  /* Horizontal line:    ━ */
    nccell vl = NCCELL_TRIVIAL_INITIALIZER;  /* Vertical line:      ┃ */

    /*
     * NOTCURSES: Channels
     * -------------------
     * A "channel" is a 64-bit value encoding foreground and background colors.
     * We create a channel with our colors so the border cells use them.
     *
     * ncchannels_set_fg_rgb(&channels, color) - Set foreground in channel
     * ncchannels_set_bg_rgb(&channels, color) - Set background in channel
     *
     * WHY? When nccells_heavy_box loads cells, it uses the channel to set
     * their colors. If channels=0, cells get default (transparent) colors.
     */
    uint64_t channels = 0;
    ncchannels_set_bg_default(&channels);
    ncchannels_set_fg_rgb(&channels, COL_BORDER);

    /*
     * nccells_heavy_box() - Load cells with heavy (thick) box characters
     *
     * NOTCURSES FUNCTION:
     *   int nccells_heavy_box(struct ncplane *n, uint32_t styles,
     *                         uint64_t channels, nccell *ul, nccell *ur,
     *                         nccell *ll, nccell *lr, nccell *hl, nccell *vl);
     *
     * ALTERNATIVES:
     *   nccells_light_box()   - Thin lines: ┌┐└┘─│
     *   nccells_double_box()  - Double lines: ╔╗╚╝═║
     *   nccells_rounded_box() - Rounded corners: ╭╮╰╯─│
     */
    nccells_heavy_box(phone, 0, channels, &ul, &ur, &ll, &lr, &hl, &vl);

    /*
     * ncplane_box() - Draw a box on the plane
     *
     * NOTCURSES FUNCTION:
     *   int ncplane_box(struct ncplane *n, const nccell *ul, const nccell *ur,
     *                   const nccell *ll, const nccell *lr, const nccell *hline,
     *                   const nccell *vline, unsigned ystop, unsigned xstop,
     *                   unsigned ctlword);
     *
     * Draws from the current cursor position to (ystop, xstop).
     * ctlword = 0 means don't fill the interior.
     *
     * We first move the cursor to (0, 0) - the top-left corner.
     */
    ncplane_cursor_move_yx(phone, 0, 0);
    ncplane_box(phone, &ul, &ur, &ll, &lr, &hl, &vl, rows - 1, cols - 1, 0);

    /*
     * nccell_release() - Free any memory used by a cell
     *
     * Some cells allocate internal storage for multi-byte characters.
     * Always release cells that were loaded with nccells_*_box() or nccell_load().
     *
     * C CONCEPT: RESOURCE MANAGEMENT
     * -------------------------------
     * C has no garbage collector. If you allocate memory, YOU must free it.
     * This is called "manual memory management." Forgetting to free memory
     * causes "memory leaks" - your program gradually uses more and more RAM.
     */
    nccell_release(phone, &ul);
    nccell_release(phone, &ur);
    nccell_release(phone, &ll);
    nccell_release(phone, &lr);
    nccell_release(phone, &hl);
    nccell_release(phone, &vl);

    /*
     * STEP 3: Draw the status bar
     */
    draw_status_bar(phone, tick);

    /*
     * STEP 4: Draw separator lines
     * ----------------------------
     * We draw horizontal lines to separate the status bar from content,
     * and content from the footer.
     *
     * Line characters used:
     *   ┣ - Left T-junction (connects to left border)
     *   ━ - Horizontal line
     *   ┫ - Right T-junction (connects to right border)
     */
    ncplane_set_fg_rgb(phone, COL_SEPARATOR);

    /* Top separator (below status bar, at row 2) */
    /* T-junctions sit on the border edge (col 0 / cols-1), so use
       default bg so they don't leak COL_BG outside the border. */
    ncplane_set_bg_default(phone);
    ncplane_putstr_yx(phone, 2, 0, "┣");
    ncplane_putstr_yx(phone, 2, cols - 1, "┫");

    /* Interior separator segments use COL_BG */
    ncplane_set_bg_rgb(phone, COL_BG);
    for (unsigned x = 1; x < cols - 1; x++) {
        ncplane_putstr_yx(phone, 2, x, "━");
    }

    /* Bottom separator (above footer, at second-to-last row) */
   // ncplane_putstr_yx(phone, rows - 2, 0, "┣");
   // for (unsigned x = 1; x < cols - 1; x++) {
   //     ncplane_putstr_yx(phone, rows - 2, x, "━");
   // }
  //  ncplane_putstr_yx(phone, rows - 2, cols - 1, "┫");

    /*
     * STEP 5: Draw footer text
     */
    //ncplane_set_fg_rgb(phone, COL_FOOTER_TEXT);
   // ncplane_putstr_yx(phone, rows - 1, 1, TEXT_FOOTER);
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  SECTION 3: MAIN FUNCTION
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *  C CONCEPT: THE main() FUNCTION
 *  ------------------------------
 *  Every C program must have exactly one main() function. This is where
 *  the operating system starts running your code.
 *
 *  SIGNATURES:
 *    int main(void)                 - No command-line arguments
 *    int main(int argc, char **argv) - With command-line arguments
 *
 *  RETURN VALUE:
 *    - 0 means "success" (program ran correctly)
 *    - Non-zero means "error" (the number can indicate which error)
 *
 *  The shell can check this with: echo $?
 */

int main(void) {
    /*
     * STEP 1: INITIALIZE LOCALE
     * -------------------------
     * setlocale() tells the C library to use the user's language/region
     * settings. This is REQUIRED for Unicode characters to display correctly.
     *
     * LC_ALL affects all locale categories (numbers, dates, characters, etc.)
     * "" means "use whatever the environment specifies"
     *
     * Without this call, Unicode box-drawing characters would show as garbage!
     */
    setlocale(LC_ALL, "");

    /*
     * STEP 2: INITIALIZE HARDWARE
     * ---------------------------
     * Set up the hardware abstraction layer. For now this does nothing
     * (stub implementation), but when connected to real hardware it would
     * open I2C/UART connections to battery gauge and cellular modem.
     */
    hardware_init();

    /*
     * STEP 3: INITIALIZE NOTCURSES
     * ----------------------------
     * Configure and start the Notcurses library.
     *
     * NCOPTION_SUPPRESS_BANNERS prevents Notcurses from printing
     * startup/shutdown messages (version info, terminal capabilities).
     */
    struct notcurses_options opts = {
        .flags = NCOPTION_SUPPRESS_BANNERS,
    };

    /*
     * notcurses_init() - Start the library
     *
     * NOTCURSES FUNCTION:
     *   struct notcurses *notcurses_init(const struct notcurses_options *opts,
     *                                    FILE *fp);
     *
     * Parameters:
     *   opts - Configuration options (pass NULL for defaults)
     *   fp   - Output stream (NULL = stdout, i.e., the terminal)
     *
     * What it does:
     *   - Queries terminal for capabilities (colors, size, etc.)
     *   - Switches to "alternate screen" (preserves your scrollback)
     *   - Enables raw input mode (immediate key events)
     *   - Creates the standard plane covering the whole terminal
     *
     * RETURNS:
     *   - Pointer to the notcurses context on success
     *   - NULL on failure
     *
     * IMPORTANT: Always check if this returns NULL!
     */
    struct notcurses *nc = notcurses_init(&opts, NULL);

    /*
     * C CONCEPT: ERROR HANDLING
     * -------------------------
     * C doesn't have exceptions. Functions indicate errors by:
     *   - Returning NULL (for pointers)
     *   - Returning negative numbers (for integers)
     *   - Setting a global 'errno' variable
     *
     * You MUST check return values! Using a NULL pointer causes a
     * "segmentation fault" (crash).
     */
    if (!nc) {
        /*
         * Print error to stderr (standard error stream).
         * stderr is separate from stdout so error messages don't mix
         * with normal output (important when output is redirected to a file).
         */
        fprintf(stderr, "Notcurses init failed\n");
        return 1;  /* Return non-zero to indicate error */
    }

    /*
     * Get the standard plane (covers entire terminal).
     *
     * notcurses_stdplane() - Get the standard plane
     *
     * The standard plane always exists after init; never NULL.
     * We use it as the parent for our phone plane.
     */
    struct ncplane *std = notcurses_stdplane(nc);

    /*
     * Draw a dev-mode label in the corner (outside the phone).
     * This is on the standard plane, visible behind the phone.
     */
    ncplane_set_fg_rgb(std, COL_DEV_LABEL);
    ncplane_putstr_yx(std, 0, 2, TEXT_DEV_LABEL);

    /*
     * STEP 4: CREATE THE PHONE PLANE
     * -------------------------------
     */
    struct ncplane *phone = create_phone_plane(std);
    if (!phone) {
        notcurses_stop(nc);  /* Clean up before exiting */
        return 1;
    }

    /*
     * STEP 5: INITIALIZE SCREEN STATE
     * --------------------------------
     * Track which screen we're currently showing.
     * See ui.h for the screen_id enum definition.
     */
    screen_id current_screen = SCREEN_HOME;

    /*
     * Frame counter for animations (low battery blink, no-signal pulse).
     * Incremented each frame; used by draw_battery() and draw_signal().
     */
    int tick = 0;

    /*
     * STEP 6: MAIN EVENT LOOP
     * -----------------------
     * This is the heart of any interactive application.
     *
     * C CONCEPT: while(1) INFINITE LOOP
     * ----------------------------------
     * "while (1)" loops forever because 1 is always "true."
     * We exit with "break" (jump out of loop) or "return" (exit function).
     *
     * THE LOOP PATTERN:
     *   1. Draw current state to screen
     *   2. Render (push to terminal)
     *   3. Wait for user input
     *   4. Update state based on input
     *   5. Repeat
     *
     * This is also called a "game loop" or "event loop."
     */
    while (1) {
        /*
         * DRAW PHASE
         * ----------
         * First draw the frame (border, status bar, footer).
         * Then draw the active screen's content.
         */
        draw_frame(phone, tick);
        tick++;

        /*
         * C CONCEPT: SWITCH STATEMENT
         * ---------------------------
         * Switch compares a value against multiple cases.
         * It's like a series of if/else if/else, but more efficient
         * and cleaner for multiple options.
         *
         * SYNTAX:
         *   switch (value) {
         *       case OPTION_A:
         *           // code for option A
         *           break;  // IMPORTANT: prevents "fall through"
         *       case OPTION_B:
         *           // code for option B
         *           break;
         *       default:
         *           // code if no case matches
         *           break;
         *   }
         *
         * WARNING: Without "break", execution continues into the next case!
         * This "fall through" is sometimes intentional but usually a bug.
         */
        switch (current_screen) {
            case SCREEN_HOME:
                screen_home_draw(phone);
                break;

            case SCREEN_SETTINGS:
                screen_settings_draw(phone);
                break;

            default:
                /* Placeholder for unimplemented screens */
                ncplane_set_fg_rgb(phone, COL_PLACEHOLDER);
                ncplane_set_bg_rgb(phone, COL_BG);
                ncplane_putstr_yx(phone, 4, 3, TEXT_COMING_SOON);
                ncplane_set_fg_rgb(phone, COL_HINT);
                ncplane_putstr_yx(phone, 6, 3, TEXT_GO_HOME);
                break;
        }

        /*
         * RENDER PHASE
         * ------------
         * Push all our drawing to the actual terminal.
         *
         * notcurses_render() composites all planes together and sends
         * the minimal set of escape codes needed to update the display.
         * This is efficient - only changed cells are redrawn.
         */
        notcurses_render(nc);

        /*
         * INPUT PHASE
         * -----------
         * Wait for user to press a key.
         *
         * ncinput is a struct that holds input event details:
         *   - id: The key code or Unicode codepoint
         *   - y, x: Mouse position (if applicable)
         *   - evtype: Event type (press, release, repeat)
         *   - modifiers: Shift, Ctrl, Alt state
         *
         * notcurses_get_blocking() waits (blocks) until input arrives.
         * The CPU is idle during this wait - no busy loop.
         */
        ncinput ni;
        uint32_t key = notcurses_get_blocking(nc, &ni);

        /*
         * HANDLE GLOBAL KEYS
         * ------------------
         * These keys work on any screen.
         *
         * C CONCEPT: continue
         * -------------------
         * "continue" skips the rest of the loop body and jumps back to
         * the beginning of the while loop. Useful for "handle this and
         * move on" situations.
         */

        /* NCKEY_RESIZE is sent when the terminal window is resized */
        if (key == NCKEY_RESIZE) {
            continue;  /* Just redraw with new size */
        }

        /* Quit: 'q' or 'Q' exits the program */
        if (key == 'q' || key == 'Q') {
            break;  /* Exit the while loop */
        }

        /* Home: 'h' or 'H' returns to home screen from anywhere */
        if (key == 'h' || key == 'H') {
            current_screen = SCREEN_HOME;
            continue;
        }

        /*
         * ROUTE INPUT TO ACTIVE SCREEN
         * ----------------------------
         * Each screen can have its own input handler that processes
         * keys and potentially changes the current screen.
         */
        switch (current_screen) {
            case SCREEN_HOME:
                current_screen = screen_home_input(key);
                break;
            default:
                /* Other screens don't handle input yet */
                break;
        }
    }

    /*
     * STEP 7: CLEANUP
     * ---------------
     * Free resources in REVERSE order of creation.
     * This is a common C pattern - things that depend on other things
     * should be cleaned up first.
     *
     * Order:
     *   1. phone plane (child) - created last
     *   2. notcurses context (parent) - created first
     *   3. hardware interfaces
     */
    ncplane_destroy(phone);

    /*
     * notcurses_stop() - Shut down the library
     *
     * This:
     *   - Destroys all remaining planes
     *   - Restores terminal to normal mode
     *   - Frees all internal memory
     *
     * If you skip this (e.g., due to crash), your terminal will be broken.
     * You can fix it by running "reset" in the terminal.
     */
    notcurses_stop(nc);

    hardware_cleanup();

    return 0;  /* Success! */
}

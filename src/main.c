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
#include "frame_renderer.h"
#include "draw_utils.h"
#include "services/theme_service.h"
#include "services/notes_service.h"


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
    theme_service_init();
    notes_service_init();

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
    notes_service_shutdown();
    settings_service_shutdown();
    hardware_cleanup();

    return 0;
}

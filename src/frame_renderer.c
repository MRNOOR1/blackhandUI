#include "frame_renderer.h"
#include "draw_utils.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "platform/hardware.h"
#include "services/theme_service.h"


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
void draw_battery(struct ncplane *phone, int percent,
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
            ghost_set(phone, theme_bg());
            ncplane_putstr_yx(phone, STATUS_ROW, STATUS_BATTERY_COL,
                              "               ");
            return;
        }
    }

    /* Four glyphs, one per iteration, drawn individually for per-glyph colour control */
    for (int i = 0; i < 4; i++) {
        ghost_set(phone, i < segs ? theme_text_primary() : theme_text_muted());
        ncplane_putstr_yx(phone, STATUS_ROW,
                          STATUS_BATTERY_COL + i,
                          i < segs ? "▰" : "▱");
    }

    /* Percentage label */
    char label[16];
    if (charging) {
        ghost_set(phone, theme_text_primary());
        snprintf(label, sizeof(label), "⚡%d%%", percent);
    } else if (percent < 15) {
        ghost_set(phone, COL_GHOST_LOW);
        snprintf(label, sizeof(label), " %d%%", percent);
    } else {
        ghost_set(phone, theme_text_muted());
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
void draw_signal(struct ncplane *phone, int bars,
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

        ghost_set(phone, theme_text_muted());
        for (int i = 0; i < 4; i++)
            ncplane_putstr_yx(phone, STATUS_ROW, sig_col + i, "○");
        return;
    }

    /* Erase prefix column — clears any leftover ✕ from disconnected state */
    ghost_set(phone, theme_bg());
    ncplane_putstr_yx(phone, STATUS_ROW, prefix_col, " ");

    for (int i = 0; i < 4; i++) {
        ghost_set(phone, i < bars ? theme_text_primary() : theme_text_muted());
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
void draw_status_bar(struct ncplane *phone, int tick) {
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
void draw_frame(struct ncplane *phone, int tick,
                       const char *screen_name) {

    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    ncplane_erase(phone);
    if (rows < (unsigned)FRAME_MIN_ROWS || cols < (unsigned)FRAME_MIN_COLS)
        return;

    /* ── Background fill — interior only, leave border cells transparent ── */
    ghost_fill_rect(phone, 1, 1, (int)rows - 2, (int)cols - 2, ' ', theme_bg(), theme_bg());

    /* ── Heavy-line border ─────────────────────────────────────────────── */
    nccell ul = NCCELL_TRIVIAL_INITIALIZER;
    nccell ur = NCCELL_TRIVIAL_INITIALIZER;
    nccell ll = NCCELL_TRIVIAL_INITIALIZER;
    nccell lr = NCCELL_TRIVIAL_INITIALIZER;
    nccell hl = NCCELL_TRIVIAL_INITIALIZER;
    nccell vl = NCCELL_TRIVIAL_INITIALIZER;

    uint64_t channels = 0;
    ncchannels_set_bg_rgb(&channels, theme_bg());
    ncchannels_set_fg_rgb(&channels, theme_border());
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
    ncplane_set_fg_rgb(phone, theme_border());
    ncplane_set_bg_rgb(phone, theme_bg());
    ncplane_putstr_yx(phone, 2, 0, "┣");

    /* Left ━ fill */
    ncplane_set_fg_rgb(phone, theme_border());
    ncplane_set_bg_rgb(phone, theme_bg());
    for (int x = 0; x < left_fill; x++)
        ncplane_putstr_yx(phone, 2, 1 + x, "━");

    /* Space + name + space */
    ncplane_putstr_yx(phone, 2, 1 + left_fill, " ");
    ncplane_set_fg_rgb(phone, theme_text_muted());
    ncplane_putstr_yx(phone, 2, 1 + left_fill + 1, screen_name);
    ncplane_set_fg_rgb(phone, theme_border());
    ncplane_putstr_yx(phone, 2, 1 + left_fill + 1 + name_len, " ");

    /* Right ━ fill */
    int right_start = 1 + left_fill + 1 + name_len + 1;
    ncplane_set_fg_rgb(phone, theme_border());
    for (int x = 0; x < right_fill; x++)
        ncplane_putstr_yx(phone, 2, right_start + x, "━");

    /* Right T-junction */
    ncplane_set_fg_rgb(phone, theme_border());
    ncplane_set_bg_rgb(phone, theme_bg());
    ncplane_putstr_yx(phone, 2, (int)cols - 1, "┫");
}

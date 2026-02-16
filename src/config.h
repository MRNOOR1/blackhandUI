#ifndef BLACKHAND_CONFIG_H
#define BLACKHAND_CONFIG_H

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  BLACKHAND UI — Master Config
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//
//  Change values here to restyle the entire UI.
//  All dimensions, colours, spacing, and text
//  are defined in one place.
//
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// ── Phone Dimensions ────────────────────────────
// Width and height of the phone plane in terminal
// cells. Increase for a larger phone, decrease for
// a tighter display.
#define PHONE_COLS          50
#define PHONE_ROWS          15

// ── Colour Palette ──────────────────────────────
// All colours are 24-bit RGB hex values (0xRRGGBB).
// Swap any value to instantly retheme the UI.

// Background
#define COL_BG              0x111111    // Main background fill

// Border / Frame
#define COL_BORDER          0x8b9a8c    // Box-drawing border lines
#define COL_SEPARATOR       0x555555    // Header & footer separator lines

// Header
#define COL_HEADER_TEXT     0xd8dad3    // "BH" brand text in header

// Footer
#define COL_FOOTER_TEXT     0xa5a58d    // "[q]Quit" hint text

// Menu (Home Screen)
#define COL_MENU_NORMAL     0xc9ada7    // Unselected menu item
#define COL_MENU_SELECTED   0xffffff    // Highlighted / selected item

// Placeholder screens
#define COL_PLACEHOLDER     0x555555    // "Coming soon..." text
#define COL_HINT            0xa5a58d    // "[h] go Home" hint text

// Settings screen
#define COL_SETTINGS_HEADER 0xf2e9e4   // "Settings" heading
#define COL_SETTINGS_TEXT   0xc9ada7   // Settings detail lines

// Dev mode label (outside the phone)
#define COL_DEV_LABEL       0x444444    // Dim dev-mode indicator

// ── Layout — Frame ──────────────────────────────
// Minimum phone plane size before we bail out of
// drawing. Prevents garbled output in tiny terminals.
#define FRAME_MIN_ROWS      3
#define FRAME_MIN_COLS      10

// ── Layout — Home Screen ────────────────────────
#define HOME_CONTENT_START_ROW  3   // First row of menu items
#define HOME_CONTENT_COL        2   // Left column offset
#define HOME_ROW_SPACING        1   // Rows between items (1=dense, 2=airy)
#define HOME_MIN_ROWS           6   // Min rows before "Too small"
#define HOME_MIN_COLS           20  // Min cols before "Too small"

// ── Layout — Settings Screen ────────────────────
#define SETTINGS_HEADER_ROW     3   // Row for "Settings" heading
#define SETTINGS_CONTENT_COL    2   // Left column offset
#define SETTINGS_FIRST_ROW      5   // First detail row
#define SETTINGS_MIN_ROWS       6
#define SETTINGS_MIN_COLS       20

// ── Text / Labels ───────────────────────────────
// Change these to localise or rebrand.
#define TEXT_BRAND           " BH "
#define TEXT_FOOTER          " [q]Quit "
#define TEXT_DEV_LABEL       "[ Dev — 50x15 phone screen ]"
#define TEXT_COMING_SOON     "Coming soon..."
#define TEXT_GO_HOME         "[h] go Home"
#define TEXT_TOO_SMALL       "Too small"
#define TEXT_SCREEN_TOO_SMALL "Screen too small"

// ── Selection Indicator ─────────────────────────
// The character shown next to the selected menu item.
// "\u25b8" = ▸ (right-pointing triangle)
#define MENU_CURSOR          "\u25b8 "
#define MENU_CURSOR_BLANK    "  "

#endif

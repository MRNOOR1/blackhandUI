#include <notcurses/notcurses.h>
#include <stdint.h>
#include <string.h>

#include "ui.h"

// ── Menu item definition ────────────────────────
// Each menu item has a label (what the user sees)
// and a target screen (where Enter takes them).
//
// Using a struct array instead of loose strings means
// each item carries its own behavior. When you add
// features later (notification counts, icons), you
// add fields to this struct — the drawing loop
// automatically picks them up.
typedef struct {
    const char *label;        // Display text
    screen_id   target;       // Screen to navigate to on Enter
} menu_item;

// ── The menu items ──────────────────────────────
// 'static' means these are only visible inside this
// file. 'const' means they won't be modified at runtime.
// This array is your single source of truth for the
// home screen menu.
static const menu_item items[] = {
    { "Calls",         SCREEN_CALLS      },
    { "Messages",      SCREEN_MESSAGES   },
    { "Settings", SCREEN_VOICE_TEXT },
    { "MP3 Player",    SCREEN_MP3        },
    { "Voice Memos",   SCREEN_VOICE_MEMO },
    { "Notes",         SCREEN_NOTES      },
};

// Calculate array length at compile time.
// sizeof(items) = total bytes of the array
// sizeof(items[0]) = bytes of one element
// Dividing gives the count. This way, adding or
// removing items from the array above automatically
// updates the count — no magic numbers to maintain.
static const int item_count = sizeof(items) / sizeof(items[0]);

// ── Selection state ─────────────────────────────
// 'static' inside a function means the variable
// persists across function calls — it's NOT reset
// each time the function runs. This is how the
// selected item "remembers" where it was between
// draws.
//
// This is similar to a class instance variable in
// Python (self.selected = 0), but in C we use
// file-scoped static variables since there are no
// classes.
static int selected = 0;

// ── Draw function ───────────────────────────────
// This is called every frame by main.c's loop.
// It draws the current state of the home screen
// onto the phone plane. It does NOT handle input —
// that's screen_home_input()'s job.
void screen_home_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    // Guard: don't draw if the plane is too small.
    // On your 50x15 phone, rows=15 and cols=50,
    // so this check passes comfortably.
    if (rows < 6 || cols < 20) {
        ncplane_putstr_yx(phone, 2, 2, "Too small");
        return;
    }

    // ── Layout constants ────────────────────────
    // These define where content sits inside the frame.
    // Row 0 = header, Row 1 = separator, so content
    // starts at row 2. The left border is column 0,
    // so content starts at column 2 (1 for border +
    // 1 for padding).
    const int content_start_row = 3;
    const int content_col       = 2;
    const int row_spacing        = 1;  // Rows between items.
                                       // 1 = dense list
                                       // 2 = breathing room

    // ── Draw each menu item ─────────────────────
    // This is the core pattern:
    //   for each item:
    //     if selected → set highlight colors
    //     else        → set normal colors
    //     print the item
    for (int i = 0; i < item_count; i++) {

        // Calculate which row this item lands on.
        // Item 0 → row 3
        // Item 1 → row 5 (with spacing=2)
        // Item 2 → row 7
        // etc.
        int row = content_start_row + (i * row_spacing);

        // Safety: don't draw past the content area.
        // rows-2 is the footer separator row.
        if (row >= (int)rows - 2) break;

        uint32_t fg = 0xc9ada7;
        if (i == selected) {
            fg = 0xffffff;
        }

        ncplane_set_fg_rgb(phone, fg);
        ncplane_set_bg_rgb(phone, 0x111111);
        if (i == selected) {
            ncplane_putstr_yx(phone, row, content_col, "\u25b8 ");
        } 
        
        else {
            ncplane_putstr_yx(phone, row, content_col, "  ");
        }
        ncplane_putstr_yx(phone, row, content_col + 2, items[i].label);
    }
}

// ── Input handler ───────────────────────────────
// Called by main.c when a key is pressed while the
// home screen is active. Returns which screen should
// be shown next.
//
// The pattern:
//   UP    → move selection up (with clamping)
//   DOWN  → move selection down (with clamping)
//   ENTER → navigate to the selected item's target
//   other → ignore (return current screen)
//
// "Clamping" means preventing the index from going
// below 0 or above item_count-1. Without this,
// pressing UP on the first item would set selected
// to -1, which would either skip all items in the
// draw loop or (worse) access memory before the
// array — a buffer underread bug.
screen_id screen_home_input(uint32_t key) {
    switch (key) {

        case NCKEY_UP:
            // Only move up if we're not already at
            // the first item. This is the "clamp"
            // — selected can never go below 0.
            if (selected > 0) {
                selected--;
            }
            // Return HOME because we're still on
            // the home screen — we just moved the
            // cursor.
            return SCREEN_HOME;

        case NCKEY_DOWN:
            // Only move down if we're not at the
            // last item. item_count-1 is the index
            // of the last element (0-indexed).
            if (selected < item_count - 1) {
                selected++;
            }
            return SCREEN_HOME;

        case NCKEY_ENTER:
        case '\n':
            // Navigate to whatever screen this menu
            // item points to. The menu_item struct
            // carries the target, so we don't need
            // a switch/if-else chain here.
            //
            // If you add a 7th menu item, it just
            // works — no input code changes needed.
            return items[selected].target;

        default:
            // Any other key: do nothing, stay on
            // home screen.
            return SCREEN_HOME;
    }
}

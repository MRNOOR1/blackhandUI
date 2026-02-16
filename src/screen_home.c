#include <notcurses/notcurses.h>
#include <stdint.h>
#include <string.h>

#include "ui.h"
#include "config.h"

// ── Menu item definition ────────────────────────
typedef struct {
    const char *label;
    screen_id   target;
} menu_item;

// ── The menu items ──────────────────────────────
static const menu_item items[] = {
    { "Calls",         SCREEN_CALLS      },
    { "Messages",      SCREEN_MESSAGES   },
    { "Settings",      SCREEN_VOICE_TEXT  },
    { "MP3 Player",    SCREEN_MP3        },
    { "Voice Memos",   SCREEN_VOICE_MEMO },
    { "Notes",         SCREEN_NOTES      },
};

static const int item_count = sizeof(items) / sizeof(items[0]);

// ── Selection state ─────────────────────────────
static int selected = 0;

// ── Draw function ───────────────────────────────
void screen_home_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    if (rows < HOME_MIN_ROWS || cols < HOME_MIN_COLS) {
        ncplane_putstr_yx(phone, 2, 2, TEXT_TOO_SMALL);
        return;
    }

    for (int i = 0; i < item_count; i++) {
        int row = HOME_CONTENT_START_ROW + (i * HOME_ROW_SPACING);

        if (row >= (int)rows - 2) break;

        uint32_t fg = COL_MENU_NORMAL;
        if (i == selected) {
            fg = COL_MENU_SELECTED;
        }

        ncplane_set_fg_rgb(phone, fg);
        ncplane_set_bg_rgb(phone, COL_BG);
        if (i == selected) {
            ncplane_putstr_yx(phone, row, HOME_CONTENT_COL, MENU_CURSOR);
        } else {
            ncplane_putstr_yx(phone, row, HOME_CONTENT_COL, MENU_CURSOR_BLANK);
        }
        ncplane_putstr_yx(phone, row, HOME_CONTENT_COL + 2, items[i].label);
    }
}

// ── Input handler ───────────────────────────────
screen_id screen_home_input(uint32_t key) {
    switch (key) {
        case NCKEY_UP:
            if (selected > 0) {
                selected--;
            }
            return SCREEN_HOME;

        case NCKEY_DOWN:
            if (selected < item_count - 1) {
                selected++;
            }
            return SCREEN_HOME;

        case NCKEY_ENTER:
        case '\n':
            return items[selected].target;

        default:
            return SCREEN_HOME;
    }
}

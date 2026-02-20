
#include <notcurses/notcurses.h>
#include <stdint.h>

#include "ui.h"
#include "config.h"

#include "services/settings_service.h"

/* ── Settings items ────────────────────────────────────────────────────── */

static int selected = 0;

/* ── Draw ──────────────────────────────────────────────────────────────── */

void screen_settings_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    if (rows < SETTINGS_MIN_ROWS || cols < SETTINGS_MIN_COLS) {
        ncplane_putstr_yx(phone, 2, 2, TEXT_SCREEN_TOO_SMALL);
        return;
    }

    int item_count = settings_service_count();

    for (int i = 0; i < item_count; i++) {
        int row = SETTINGS_FIRST_ROW + i;
        if (row >= (int)rows - 1) break;

        uint32_t fg = (i == selected) ? COL_MENU_SELECTED : COL_SETTINGS_TEXT;
        const char *cursor = (i == selected) ? MENU_CURSOR : MENU_CURSOR_BLANK;
        const char *check  = settings_service_enabled(i) ? "☑ " : "☐ ";

        ncplane_set_fg_rgb(phone, fg);
        ncplane_set_bg_rgb(phone, COL_BG);
        ncplane_putstr_yx(phone, row, SETTINGS_CONTENT_COL, cursor);
        ncplane_putstr_yx(phone, row, SETTINGS_CONTENT_COL + 2, check);
        ncplane_putstr_yx(phone, row, SETTINGS_CONTENT_COL + 4, settings_service_label(i));
    }
}

/* ── Input ─────────────────────────────────────────────────────────────── */

screen_id screen_settings_input(uint32_t key) {
    int item_count = settings_service_count();

    switch (key) {
        case NCKEY_UP:
            if (selected > 0) selected--;
            return SCREEN_SETTINGS;
        case NCKEY_DOWN:
            if (selected < item_count - 1) selected++;
            return SCREEN_SETTINGS;
        case NCKEY_ENTER:
        case '\n':
            settings_service_toggle(selected);
            return SCREEN_SETTINGS;
        case NCKEY_ESC:
        case 'b':
        case 'B':
            return SCREEN_HOME;
        default:
            return SCREEN_SETTINGS;
    }
}

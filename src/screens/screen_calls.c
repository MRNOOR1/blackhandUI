#include <notcurses/notcurses.h>
#include <stdint.h>

#include "config.h"
#include "ui.h"

static const char *k_call_log[] = {
    "Noura  2m ago",
    "Mom    yesterday",
    "Unknown missed",
};

void screen_calls_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);
    if (rows < 8 || cols < 20) {
        ncplane_putstr_yx(phone, 3, 2, TEXT_SCREEN_TOO_SMALL);
        return;
    }

    ncplane_set_bg_rgb(phone, COL_BG);
    ncplane_set_fg_rgb(phone, COL_GHOST_PCT);
    ncplane_putstr_yx(phone, 3, 2, "Recent Calls");

    ncplane_set_fg_rgb(phone, COL_MENU_NORMAL);
    for (int i = 0; i < 3; i++) {
        ncplane_putstr_yx(phone, 5 + i, 2, k_call_log[i]);
    }

    ncplane_set_fg_rgb(phone, COL_HINT);
    ncplane_putstr_yx(phone, (int)rows - 2, 2, "[b] Back");
}

screen_id screen_calls_input(uint32_t key) {
    switch (key) {
        case NCKEY_ESC:
        case 'b':
        case 'B':
            return SCREEN_HOME;
        default:
            return SCREEN_CALLS;
    }
}

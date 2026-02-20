#include <notcurses/notcurses.h>
#include <stdint.h>

#include "config.h"
#include "ui.h"

void screen_messages_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);
    if (rows < 8 || cols < 20) {
        ncplane_putstr_yx(phone, 3, 2, TEXT_SCREEN_TOO_SMALL);
        return;
    }

    ncplane_set_bg_rgb(phone, COL_BG);
    ncplane_set_fg_rgb(phone, COL_GHOST_PCT);
    ncplane_putstr_yx(phone, 3, 2, "Messages");

    ncplane_set_fg_rgb(phone, COL_PLACEHOLDER);
    ncplane_putstr_yx(phone, 5, 2, "No messages yet");
    ncplane_putstr_yx(phone, 6, 2, "(cell modem later)");

    ncplane_set_fg_rgb(phone, COL_HINT);
    ncplane_putstr_yx(phone, (int)rows - 2, 2, "[b] Back");
}

screen_id screen_messages_input(uint32_t key) {
    switch (key) {
        case NCKEY_ESC:
        case 'b':
        case 'B':
            return SCREEN_HOME;
        default:
            return SCREEN_MESSAGES;
    }
}

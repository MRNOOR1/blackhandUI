#include <notcurses/notcurses.h>

#include "ui.h"
#include "config.h"

void screen_settings_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    if (rows < SETTINGS_MIN_ROWS || cols < SETTINGS_MIN_COLS) {
        ncplane_putstr_yx(phone, 2, 2, TEXT_SCREEN_TOO_SMALL);
        return;
    }

    ncplane_set_fg_rgb(phone, COL_SETTINGS_HEADER);
    ncplane_putstr_yx(phone, SETTINGS_HEADER_ROW, SETTINGS_CONTENT_COL, "Settings");

    ncplane_set_fg_rgb(phone, COL_SETTINGS_TEXT);
    ncplane_putstr_yx(phone, SETTINGS_FIRST_ROW,     SETTINGS_CONTENT_COL, "Display: Auto");
    ncplane_putstr_yx(phone, SETTINGS_FIRST_ROW + 1, SETTINGS_CONTENT_COL, "Audio: Beep");
    ncplane_putstr_yx(phone, SETTINGS_FIRST_ROW + 2, SETTINGS_CONTENT_COL, "Storage: 12 GB free");
}

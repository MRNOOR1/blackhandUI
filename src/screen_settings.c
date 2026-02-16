#include <notcurses/notcurses.h>

#include "ui.h"

void screen_settings_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    if (rows < 6 || cols < 20) {
        ncplane_putstr_yx(phone, 2, 2, "Screen too small");
        return;
    }

    ncplane_set_fg_rgb(phone, 0xf2e9e4);
    ncplane_putstr_yx(phone, 3, 2, "Settings");

    ncplane_set_fg_rgb(phone, 0xc9ada7);
    ncplane_putstr_yx(phone, 5, 2, "Display: Auto");
    ncplane_putstr_yx(phone, 6, 2, "Audio: Beep");
    ncplane_putstr_yx(phone, 7, 2, "Storage: 12 GB free");
}

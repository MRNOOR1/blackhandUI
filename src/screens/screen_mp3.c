#include <notcurses/notcurses.h>
#include <stdint.h>

#include "config.h"
#include "ui.h"

/*
 * screen_mp3.c
 *
 * Purpose:
 * - UI for MP3 library and now-playing state.
 *
 * What you should implement next:
 * - A two-mode screen (library view + now-playing view).
 * - Track selection with Up/Down.
 * - Enter to play selected track.
 * - Space to toggle play/pause.
 * - Progress rendering based on service state.
 *
 * Coupling/cohesion rule:
 * - No audio decoding logic here.
 * - Call mp3_service APIs for list/playback state.
 */

void screen_mp3_draw(struct ncplane *phone) {
    ncplane_set_bg_rgb(phone, COL_BG);
    ncplane_set_fg_rgb(phone, COL_PLACEHOLDER);
    ncplane_putstr_yx(phone, 4, 2, "MP3 screen TODO");
    ncplane_set_fg_rgb(phone, COL_HINT);
    ncplane_putstr_yx(phone, 6, 2, "[Enter] Select  [b] Back");
}

screen_id screen_mp3_input(uint32_t key) {
    switch (key) {
        case NCKEY_ESC:
        case 'b':
        case 'B':
            return SCREEN_HOME;
        default:
            return SCREEN_MP3;
    }
}

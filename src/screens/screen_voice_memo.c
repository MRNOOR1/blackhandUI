#include <notcurses/notcurses.h>
#include <stdint.h>

#include "config.h"
#include "ui.h"

/*
 * screen_voice_memo.c
 *
 * Purpose:
 * - UI for recording and listing voice memos.
 *
 * What you should implement next:
 * - Two states: idle/list vs recording.
 * - Start/stop recording action on Enter.
 * - Timer display while recording.
 * - Playback and delete actions for saved memos.
 *
 * Coupling/cohesion rule:
 * - Keep ALSA/audio details out of this file.
 * - Use a dedicated voice_memo_service module.
 */

void screen_voice_memo_draw(struct ncplane *phone) {
    ncplane_set_bg_rgb(phone, COL_BG);
    ncplane_set_fg_rgb(phone, COL_PLACEHOLDER);
    ncplane_putstr_yx(phone, 4, 2, "Voice Memo screen TODO");
    ncplane_set_fg_rgb(phone, COL_HINT);
    ncplane_putstr_yx(phone, 6, 2, "[Enter] Record  [b] Back");
}

screen_id screen_voice_memo_input(uint32_t key) {
    switch (key) {
        case NCKEY_ESC:
        case 'b':
        case 'B':
            return SCREEN_HOME;
        default:
            return SCREEN_VOICE_MEMO;
    }
}

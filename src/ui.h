#ifndef BLACKHAND_UI_H
#define BLACKHAND_UI_H

#include <notcurses/notcurses.h>
#include <stdint.h>

enum screen_id {
    SCREEN_HOME = 0,
    SCREEN_SETTINGS = 1,
    SCREEN_CALLS = 2,
    SCREEN_MESSAGES = 3,
    SCREEN_VOICE_TEXT = 4,
    SCREEN_MP3 = 5,
    SCREEN_VOICE_MEMO = 6,
    SCREEN_NOTES = 7
};

typedef enum screen_id screen_id;

void screen_home_draw(struct ncplane *phone);
screen_id screen_home_input(uint32_t key);
void screen_settings_draw(struct ncplane *phone);

#endif

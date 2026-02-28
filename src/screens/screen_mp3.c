#include <notcurses/notcurses.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "ui.h"
#include "services/mp3_service.h"
#include "services/theme_service.h"

typedef enum {
    MP3_MODE_LIBRARY,
    MP3_MODE_NOW_PLAYING,
} mp3_mode_t;

static mp3_mode_t mode = MP3_MODE_LIBRARY;
static int selected = 0;

static void draw_visualizer(struct ncplane *phone, int row, int col, int width) {
    if (width < 8) return;

    unsigned char levels[MP3_VIZ_BINS] = {0};
    size_t bins = mp3_service_get_visualizer(levels, MP3_VIZ_BINS);
    if (bins == 0) return;

    int max_bars = width - 1;
    int bars = (int)bins;
    if (bars > max_bars) bars = max_bars;

    for (int i = 0; i < bars; i++) {
        int level = levels[i];

        const char *glyph = "\xE2\x96\x81";          /* ▁ */
        if (level >= 2) glyph = "\xE2\x96\x82";      /* ▂ */
        if (level >= 4) glyph = "\xE2\x96\x84";      /* ▄ */
        if (level >= 6) glyph = "\xE2\x96\x86";      /* ▆ */
        if (level >= 8) glyph = "\xE2\x96\x88";      /* █ */

        ncplane_set_fg_rgb(phone, (i % 2) ? theme_text_muted() : theme_text_primary());
        ncplane_set_bg_rgb(phone, theme_bg());
        ncplane_putstr_yx(phone, row, col + i, glyph);
    }
}

static void draw_library(struct ncplane *phone, unsigned rows, unsigned cols) {
    size_t count = mp3_service_count();

    if (count == 0) {
        ncplane_set_fg_rgb(phone, theme_text_muted());
        ncplane_set_bg_rgb(phone, theme_bg());
        ncplane_putstr_yx(phone, 4, 2, "No MP3 files found");
        ncplane_putstr_yx(phone, 6, 2, "Place files in ./Music");
        ncplane_putstr_yx(phone, (int)rows - 2, 2, "[b] Back");
        return;
    }

    if (selected < 0) selected = 0;
    if (selected >= (int)count) selected = (int)count - 1;

    int max_rows = (int)rows - 5;
    if (max_rows < 1) max_rows = 1;

    for (int i = 0; i < (int)count && i < max_rows; i++) {
        const AudioFile *track = mp3_service_get((size_t)i);
        if (!track) continue;

        int row = 3 + i;
        const char *cursor = (i == selected) ? MENU_CURSOR : MENU_CURSOR_BLANK;
        uint32_t fg = (i == selected) ? theme_text_primary() : theme_text_muted();

        ncplane_set_fg_rgb(phone, fg);
        ncplane_set_bg_rgb(phone, theme_bg());
        ncplane_putstr_yx(phone, row, 2, cursor);

        char line[256];
        snprintf(line, sizeof(line), "%s - %s", track->author ? track->author : "Unknown", track->title ? track->title : "Unknown");
        line[(int)cols - 6 > 0 ? (int)cols - 6 : 0] = '\0';
        ncplane_putstr_yx(phone, row, 4, line);
    }

    ncplane_set_fg_rgb(phone, theme_text_muted());
    ncplane_set_bg_rgb(phone, theme_bg());
    ncplane_putstr_yx(phone, (int)rows - 2, 2, "[Enter] Play  [b] Back");
}

static void draw_now_playing(struct ncplane *phone, unsigned rows, unsigned cols) {
    int current = mp3_service_get_current_index();
    if (current < 0) {
        mode = MP3_MODE_LIBRARY;
        return;
    }

    const AudioFile *track = mp3_service_get((size_t)current);
    if (!track) {
        mode = MP3_MODE_LIBRARY;
        return;
    }

    playback_state st = mp3_service_get_state();
    unsigned elapsed = mp3_service_get_elapsed();

    const char *state_text = (st == PLAYING) ? "PLAYING" : (st == PAUSED ? "PAUSED" : "STOPPED");

    ncplane_set_fg_rgb(phone, theme_text_primary());
    ncplane_set_bg_rgb(phone, theme_bg());
    ncplane_putstr_yx(phone, 4, 2, "Now Playing");

    char line1[256];
    char line2[256];
    char line3[64];
    snprintf(line1, sizeof(line1), "Title: %s", track->title ? track->title : "Unknown");
    snprintf(line2, sizeof(line2), "Artist: %s", track->author ? track->author : "Unknown");
    snprintf(line3, sizeof(line3), "%s  %u sec", state_text, elapsed);

    line1[(int)cols - 4 > 0 ? (int)cols - 4 : 0] = '\0';
    line2[(int)cols - 4 > 0 ? (int)cols - 4 : 0] = '\0';

    ncplane_putstr_yx(phone, 6, 2, line1);
    ncplane_putstr_yx(phone, 7, 2, line2);
    ncplane_set_fg_rgb(phone, theme_text_muted());
    ncplane_putstr_yx(phone, 9, 2, line3);

    draw_visualizer(phone, 11, 2, (int)cols - 4);

    ncplane_putstr_yx(phone, (int)rows - 2, 2, "[space] Play/Pause  [b] Back");
}

void screen_mp3_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    if (mode == MP3_MODE_LIBRARY) {
        draw_library(phone, rows, cols);
    } else {
        draw_now_playing(phone, rows, cols);
    }
}

screen_id screen_mp3_input(uint32_t key) {
    size_t count = mp3_service_count();

    if (mode == MP3_MODE_LIBRARY) {
        switch (key) {
            case NCKEY_UP:
                if (selected > 0) selected--;
                return SCREEN_MP3;
            case NCKEY_DOWN:
                if (selected < (int)count - 1) selected++;
                return SCREEN_MP3;
            case NCKEY_ENTER:
            case '\n':
                if (count > 0 && mp3_service_play((size_t)selected) == 0) {
                    mode = MP3_MODE_NOW_PLAYING;
                }
                return SCREEN_MP3;
            case NCKEY_ESC:
            case 'b':
            case 'B':
                return SCREEN_HOME;
            default:
                return SCREEN_MP3;
        }
    }

    switch (key) {
        case ' ':
            if (mp3_service_get_state() == PLAYING) {
                mp3_service_pause();
            } else if (mp3_service_get_state() == PAUSED) {
                mp3_service_resume();
            } else if (count > 0) {
                mp3_service_play((size_t)selected);
            }
            return SCREEN_MP3;
        case NCKEY_ESC:
        case 'b':
        case 'B':
            mode = MP3_MODE_LIBRARY;
            return SCREEN_MP3;
        default:
            return SCREEN_MP3;
    }
}

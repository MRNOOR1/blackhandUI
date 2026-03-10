#include <notcurses/notcurses.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "draw_utils.h"
#include "ui.h"
#include "services/mp3_service.h"
#include "services/theme_service.h"

typedef enum {
    MP3_MODE_LIBRARY,
    MP3_MODE_NOW_PLAYING,
} mp3_mode_t;

static mp3_mode_t mode = MP3_MODE_LIBRARY;
static int selected = 0;
static int list_scroll = 0;

static void put_clipped(struct ncplane *p, int row, int col, int max_w, const char *text) {
    if (!text || max_w <= 0) return;
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", text);
    if ((int)strlen(buf) > max_w) {
        if (max_w > 3) { buf[max_w-3]='.'; buf[max_w-2]='.'; buf[max_w-1]='.'; }
        buf[max_w] = '\0';
    }
    ncplane_putstr_yx(p, row, col, buf);
}

/* ── Library mode ──────────────────────────────────────────────────────── */
static void draw_library(struct ncplane *phone, unsigned rows, unsigned cols) {
    int footer = (int)rows - FOOTER_ROW_OFFSET;
    int width = INNER_WIDTH(cols);

    size_t count = mp3_service_count();

    /* title */
    ncplane_set_fg_rgb(phone, theme_text_primary());
    ncplane_set_bg_rgb(phone, theme_bg());
    ncplane_putstr_yx(phone, CONTENT_START_ROW, CONTENT_COL, "MUSIC");

    /* separator */
    ncplane_set_fg_rgb(phone, theme_text_muted());
    for (int x = 0; x < width && CONTENT_COL + x < (int)cols - 1; x++)
        ncplane_putstr_yx(phone, CONTENT_START_ROW + 1, CONTENT_COL + x, "\u2500");

    if (count == 0) {
        int mid = (CONTENT_START_ROW + 2 + footer) / 2;
        ncplane_set_fg_rgb(phone, theme_text_muted());
        ncplane_putstr_yx(phone, mid - 1, CONTENT_COL, "\u266B  \u266A  \u266B");
        ncplane_putstr_yx(phone, mid + 1, CONTENT_COL, "No MP3 files found");
        ncplane_putstr_yx(phone, mid + 2, CONTENT_COL, "Place files in ./Music/");
        ghost_softkeys(phone, "[Back]", "[Play]");
        return;
    }

    if (selected < 0) selected = 0;
    if (selected >= (int)count) selected = (int)count - 1;

    int list_start = CONTENT_START_ROW + 3;
    int max_rows = footer - list_start;
    if (max_rows < 1) max_rows = 1;

    if (selected < list_scroll) list_scroll = selected;
    if (selected >= list_scroll + max_rows) list_scroll = selected - max_rows + 1;
    if (list_scroll < 0) list_scroll = 0;

    for (int i = 0; i < max_rows; i++) {
        int index = list_scroll + i;
        if (index >= (int)count) break;

        const AudioFile *track = mp3_service_get((size_t)index);
        if (!track) continue;

        int row = list_start + i;
        int sel = (index == selected);

        ncplane_set_fg_rgb(phone, sel ? theme_text_primary() : theme_text_muted());
        ncplane_set_bg_rgb(phone, theme_bg());
        ncplane_putstr_yx(phone, row, CONTENT_COL, sel ? MENU_CURSOR : MENU_CURSOR_BLANK);

        /* note icon for selected */
        if (sel) {
            char line[256];
            snprintf(line, sizeof(line), "\u266A %s - %s",
                     track->author ? track->author : "?",
                     track->title ? track->title : "?");
            put_clipped(phone, row, CONTENT_COL + 2, width - 2, line);
        } else {
            char line[256];
            snprintf(line, sizeof(line), "%s - %s",
                     track->author ? track->author : "?",
                     track->title ? track->title : "?");
            put_clipped(phone, row, CONTENT_COL + 2, width - 2, line);
        }
    }

    ghost_softkeys(phone, "[Back]", "[Play]");
}

/* ── Now Playing mode (the GUI-feel screen) ────────────────────────────── */
static void draw_now_playing(struct ncplane *phone, unsigned rows, unsigned cols) {
    int current = mp3_service_get_current_index();
    if (current < 0) { mode = MP3_MODE_LIBRARY; return; }

    const AudioFile *track = mp3_service_get((size_t)current);
    if (!track) { mode = MP3_MODE_LIBRARY; return; }

    playback_state st = mp3_service_get_state();
    unsigned elapsed = mp3_service_get_elapsed();
    int footer = (int)rows - FOOTER_ROW_OFFSET;
    int width = INNER_WIDTH(cols);

    ncplane_set_fg_rgb(phone, theme_text_primary());
    ncplane_set_bg_rgb(phone, theme_bg());
    ncplane_putstr_yx(phone, CONTENT_START_ROW, CONTENT_COL, "MUSIC");

    ncplane_set_fg_rgb(phone, theme_text_muted());
    for (int x = 0; x < width && CONTENT_COL + x < (int)cols - 1; x++) {
        ncplane_putstr_yx(phone, CONTENT_START_ROW + 1, CONTENT_COL + x, "\u2500");
    }

    int info_row = CONTENT_START_ROW + 3;

    const char *state = (st == PLAYING) ? "PLAYBACK" : ((st == PAUSED) ? "PAUSED" : "IDLE");
    const char *icon = (st == PLAYING) ? "▶" : ((st == PAUSED) ? "▌▌" : "■");
    char status_line[128];
    unsigned mins = elapsed / 60;
    unsigned secs = elapsed % 60;
    snprintf(status_line, sizeof(status_line), "MODE: %s  %s  %02u:%02u", state, icon, mins, secs);
    ghost_text(phone, info_row, CONTENT_COL, theme_text_primary(), status_line);

    ncplane_set_fg_rgb(phone, theme_text_primary());
    ncplane_set_bg_rgb(phone, theme_bg());
    put_clipped(phone, info_row + 2, CONTENT_COL, width,
                track->title ? track->title : "Unknown");

    ncplane_set_fg_rgb(phone, theme_text_muted());
    put_clipped(phone, info_row + 3, CONTENT_COL, width,
                track->author ? track->author : "Unknown");

    /* ── progress bar ── */
    int bar_row = info_row + 5;
    int bar_width = width - 2;
    if (bar_width > 0 && bar_row < footer) {
        int meter_width = bar_width - 5;
        if (meter_width < 1) meter_width = 1;

        /* elapsed indicator (simple fill based on time, wraps every 4 min) */
        int fill = (int)(elapsed % 240) * meter_width / 240;
        if (fill > meter_width) fill = meter_width;

        ncplane_set_fg_rgb(phone, theme_text_primary());
        ncplane_set_bg_rgb(phone, theme_bg());
        ncplane_putstr_yx(phone, bar_row, CONTENT_COL, "PLAY ");
        for (int x = 0; x < meter_width; x++) {
            ncplane_putstr_yx(phone, bar_row, CONTENT_COL + 1 + x,
                              (x < fill) ? "█" : "░");
        }
    }

    int meter_row = bar_row + 2;
    if (meter_row < footer - 1) {
        unsigned char levels[16] = {0};
        size_t bins = mp3_service_get_visualizer(levels, 16);
        if (bins == 0) bins = 16;
        char meter[64];
        int p = 0;
        p += snprintf(meter + p, sizeof(meter) - (size_t)p, "AUDIO ");
        for (int i = 0; i < 10 && p < (int)sizeof(meter) - 2; i++) {
            meter[p++] = (levels[i % bins] >= 4) ? '|' : '.';
        }
        meter[p] = '\0';
        ghost_text(phone, meter_row, CONTENT_COL, theme_text_muted(), meter);
    }

    ghost_softkeys(phone, "[Back]", "[Play/Pause]");
}

void screen_mp3_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    if (mode == MP3_MODE_LIBRARY)
        draw_library(phone, rows, cols);
    else
        draw_now_playing(phone, rows, cols);
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
            case 'e':
            case 'E':
                if (count > 0 && mp3_service_play((size_t)selected) == 0)
                    mode = MP3_MODE_NOW_PLAYING;
                return SCREEN_MP3;
            case 'q':
            case 'Q':
                return SCREEN_HOME;
            default:
                return SCREEN_MP3;
        }
    }

    /* now playing input */
    switch (key) {
        case NCKEY_ENTER:
        case '\n':
        case 'e':
        case 'E':
            if (mp3_service_get_state() == PLAYING)
                mp3_service_pause();
            else if (mp3_service_get_state() == PAUSED)
                mp3_service_resume();
            else if (count > 0)
                mp3_service_play((size_t)selected);
            return SCREEN_MP3;
        case 'q':
        case 'Q':
            mode = MP3_MODE_LIBRARY;
            return SCREEN_MP3;
        default:
            return SCREEN_MP3;
    }
}

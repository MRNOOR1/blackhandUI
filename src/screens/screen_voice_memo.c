#include <notcurses/notcurses.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "draw_utils.h"
#include "ui.h"
#include "services/theme_service.h"
#include "services/voice_memo_service.h"

static int selected = 0;
static int list_scroll = 0;
static int anim_tick = 0;
static int delete_prompt = 0;
static int delete_yes = 0;

static void draw_delete_popup(struct ncplane *phone, unsigned rows, unsigned cols) {
    (void)rows;
    (void)cols;
    ghost_confirm_popup(phone, "ARE YOU SURE?", delete_yes);
}

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

static void format_time_ms(int ms, char *buf, size_t buf_size) {
    int sec = ms / 1000;
    int min = sec / 60;
    sec %= 60;
    snprintf(buf, buf_size, "%02d:%02d", min, sec);
}

/* ── Animated waveform for recording ───────────────────────────────────── */
static void draw_recording_wave(struct ncplane *phone, int row, int col,
                                 int width, int tick) {
    static const char *wave_chars[] = {
        "\u2581", "\u2582", "\u2583", "\u2584",
        "\u2585", "\u2586", "\u2587", "\u2588"
    };

    uint32_t fg = ((tick / 2) % 2 == 0) ? theme_border() : theme_text_primary();

    for (int i = 0; i < width; i++) {
        /* simulate random-ish waveform using tick + position */
        int wave = ((tick * 7 + i * 13 + i * i) % 16);
        int idx = wave / 2;
        if (idx > 7) idx = 7;
        if (idx < 0) idx = 0;

        ncplane_set_fg_rgb(phone, fg);
        ncplane_set_bg_rgb(phone, theme_bg());
        ncplane_putstr_yx(phone, row, col + i, wave_chars[idx]);
    }
}

/* ── Animated playback waveform ────────────────────────────────────────── */
static void draw_playback_wave(struct ncplane *phone, int row, int col,
                                int width, int tick, int total_ms, int elapsed_ms) {
    if (width <= 0) return;

    float progress = 0.0f;
    if (total_ms > 0) progress = (float)elapsed_ms / (float)total_ms;
    if (progress > 1.0f) progress = 1.0f;
    int fill = (int)(progress * (float)width);

    uint32_t played_col = theme_border();
    uint32_t remain_col = theme_text_muted();

    static const char *bars[] = { "\u2581", "\u2582", "\u2583", "\u2584",
                                  "\u2585", "\u2584", "\u2583", "\u2582" };

    for (int i = 0; i < width; i++) {
        int pattern = ((i * 5 + tick) % 8);
        if (pattern < 0) pattern = 0;

        ncplane_set_fg_rgb(phone, (i < fill) ? played_col : remain_col);
        ncplane_set_bg_rgb(phone, theme_bg());
        ncplane_putstr_yx(phone, row, col + i, bars[pattern % 8]);
    }
}

/* ── Recording screen ──────────────────────────────────────────────────── */
static void draw_recording(struct ncplane *phone, unsigned rows, unsigned cols) {
    int footer = (int)rows - FOOTER_ROW_OFFSET;
    int width = INNER_WIDTH(cols);
    int elapsed = voice_memo_service_elapsed_ms();
    char tbuf[16];
    format_time_ms(elapsed, tbuf, sizeof(tbuf));

    /* pulsing record dot */
    int dot_visible = ((anim_tick / 4) % 2 == 0);
    ncplane_set_fg_rgb(phone, theme_border());
    ncplane_set_bg_rgb(phone, theme_bg());
    ncplane_putstr_yx(phone, CONTENT_START_ROW, CONTENT_COL,
                      dot_visible ? "MODE: RECORDING  ●" : "MODE: RECORDING");

    /* time display, large */
    ncplane_set_fg_rgb(phone, theme_text_primary());
    ncplane_putstr_yx(phone, CONTENT_START_ROW + 2, CONTENT_COL, "STATE: CAPTURE");
    ncplane_putstr_yx(phone, CONTENT_START_ROW + 3, CONTENT_COL, tbuf);

    int bar_row = CONTENT_START_ROW + 6;
    int bar_w = width > 16 ? 16 : width;
    int fill = (elapsed / 1000) % (bar_w + 1);
    if (bar_row < footer) {
        for (int i = 0; i < bar_w; i++) {
            ncplane_putstr_yx(phone, bar_row, CONTENT_COL + i, (i < fill) ? "█" : "░");
        }
    }

    ghost_softkeys(phone, "[Back]", "[Stop]");
}

/* ── Idle list mode ────────────────────────────────────────────────────── */
static void draw_list(struct ncplane *phone, unsigned rows, unsigned cols) {
    int footer = (int)rows - FOOTER_ROW_OFFSET;
    int width = INNER_WIDTH(cols);

    VMState st = voice_memo_service_state();
    int elapsed = voice_memo_service_elapsed_ms();
    int total = voice_memo_service_total_ms();

    /* title */
    ncplane_set_fg_rgb(phone, theme_text_primary());
    ncplane_set_bg_rgb(phone, theme_bg());
    ncplane_putstr_yx(phone, CONTENT_START_ROW, CONTENT_COL, "VOICE MEMO");

    /* separator */
    ncplane_set_fg_rgb(phone, theme_text_muted());
    for (int x = 0; x < width && CONTENT_COL + x < (int)cols - 1; x++)
        ncplane_putstr_yx(phone, CONTENT_START_ROW + 1, CONTENT_COL + x, "\u2500");

    size_t count = 0;
    const VoiceMemo **list = voice_memo_service_list_all(&count);

    if (count == 0) {
        int mid = (CONTENT_START_ROW + 2 + footer) / 2;
        ncplane_set_fg_rgb(phone, theme_text_muted());
        ncplane_putstr_yx(phone, mid, CONTENT_COL, "No voice memos yet");
        ncplane_putstr_yx(phone, mid + 1, CONTENT_COL, "Right soft key starts recording");
        ghost_softkeys(phone, "[Back]", "[Record]");
        return;
    }

    if (selected < 0) selected = 0;
    if (count > 0 && selected >= (int)count) selected = (int)count - 1;

    int list_start = CONTENT_START_ROW + 3;
    int max_rows = footer - list_start - 3; /* reserve space for playback bar */
    if (max_rows < 1) max_rows = 1;

    if (selected < list_scroll) list_scroll = selected;
    if (selected >= list_scroll + max_rows) list_scroll = selected - max_rows + 1;
    if (list_scroll < 0) list_scroll = 0;

    for (int i = 0; i < max_rows; i++) {
        int index = list_scroll + i;
        if (index >= (int)count) break;

        const VoiceMemo *m = list[index];
        int row = list_start + i;
        int sel = (index == selected);

        ncplane_set_fg_rgb(phone, sel ? theme_text_primary() : theme_text_muted());
        ncplane_set_bg_rgb(phone, theme_bg());
        ncplane_putstr_yx(phone, row, CONTENT_COL, sel ? MENU_CURSOR : MENU_CURSOR_BLANK);

        char dbuf[16];
        format_time_ms(m->duration_ms, dbuf, sizeof(dbuf));

        char line[256];
        snprintf(line, sizeof(line), "%s  %s", m->filename ? m->filename : "?", dbuf);
        put_clipped(phone, row, CONTENT_COL + 2, width - 2, line);
    }

    /* playback indicator with animated waveform */
    if (st == VM_PLAYING || st == VM_PAUSED) {
        const VoiceMemo *cur = voice_memo_service_current();
        int play_row = footer - 2;
        if (play_row > list_start + max_rows) {
            char tbuf[16];
            format_time_ms(elapsed, tbuf, sizeof(tbuf));

            const char *icon = (st == VM_PLAYING) ? "▶" : "▌▌";
            char status[128];
            snprintf(status, sizeof(status), "STATE: %s  %s %s %s",
                     (st == VM_PLAYING) ? "PLAYBACK" : "PAUSED",
                     icon, cur ? cur->filename : "", tbuf);
            ncplane_set_fg_rgb(phone, theme_text_primary());
            ncplane_set_bg_rgb(phone, theme_bg());
            put_clipped(phone, play_row, CONTENT_COL, width, status);

            if (play_row + 1 < footer) {
                int meter_w = width > 16 ? 16 : width;
                int fill = (total > 0) ? (elapsed * meter_w / total) : 0;
                if (fill > meter_w) fill = meter_w;
                for (int i = 0; i < meter_w; i++) {
                    ncplane_putstr_yx(phone, play_row + 1, CONTENT_COL + i, (i < fill) ? "█" : "░");
                }
            }
        }
    }

    if (st == VM_PLAYING) {
        ghost_softkeys(phone, "[Back]", "[Pause]");
    } else if (st == VM_PAUSED) {
        ghost_softkeys(phone, "[Back]", "[Resume]");
    } else {
        ghost_softkeys(phone, "[Back]", "[Play/Rec]");
    }

    if (delete_prompt && st != VM_RECORDING) draw_delete_popup(phone, rows, cols);
}

void screen_voice_memo_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);
    anim_tick++;

    VMState st = voice_memo_service_state();

    if (st == VM_RECORDING) {
        draw_recording(phone, rows, cols);
    } else {
        draw_list(phone, rows, cols);
    }

}

screen_id screen_voice_memo_input(uint32_t key) {
    size_t count = 0;
    const VoiceMemo **list = voice_memo_service_list_all(&count);
    VMState st = voice_memo_service_state();

    if (delete_prompt && st != VM_RECORDING) {
        switch (key) {
            case NCKEY_LEFT:
            case NCKEY_UP:
                delete_yes = 0;
                return SCREEN_VOICE_MEMO;
            case NCKEY_RIGHT:
            case NCKEY_DOWN:
                delete_yes = 1;
                return SCREEN_VOICE_MEMO;
            case NCKEY_ENTER:
            case '\n':
            case 'e':
            case 'E':
                if (delete_yes && count > 0 && list && selected < (int)count) {
                    voice_memo_service_delete(list[selected]->filename);
                    voice_memo_service_list_all(&count);
                    if (selected >= (int)count && selected > 0) selected--;
                }
                delete_prompt = 0;
                delete_yes = 0;
                return SCREEN_VOICE_MEMO;
            case 'q':
            case 'Q':
                delete_prompt = 0;
                delete_yes = 0;
                return SCREEN_VOICE_MEMO;
            default:
                return SCREEN_VOICE_MEMO;
        }
    }

    /* ── normal input ── */
    switch (key) {
        case NCKEY_UP:
            if (selected > 0) selected--;
            return SCREEN_VOICE_MEMO;
        case NCKEY_DOWN:
            if (selected < (int)count - 1) selected++;
            return SCREEN_VOICE_MEMO;
        case NCKEY_ENTER:
        case '\n':
        case NCKEY_RIGHT:
        case 'e':
        case 'E':
            if (st == VM_RECORDING) {
                voice_memo_service_record_stop(NULL);
                selected = 0;
                list_scroll = 0;
            } else if (st == VM_IDLE) {
                if (count > 0 && list) {
                    voice_memo_service_play_start(list[selected]->filename);
                } else {
                    voice_memo_service_record_start();
                }
            } else if (st == VM_PLAYING) {
                voice_memo_service_play_pause();
            } else if (st == VM_PAUSED) {
                voice_memo_service_play_resume();
            }
            return SCREEN_VOICE_MEMO;
        case NCKEY_LEFT:
            if (count > 0 && list && st != VM_RECORDING) {
                delete_prompt = 1;
                delete_yes = 0;
            }
            return SCREEN_VOICE_MEMO;
        case 'q':
        case 'Q':
            delete_prompt = 0;
            delete_yes = 0;
            if (st == VM_RECORDING)
                voice_memo_service_record_stop(NULL);
            else if (st == VM_PLAYING || st == VM_PAUSED)
                voice_memo_service_play_stop();
            return SCREEN_HOME;
        default:
            return SCREEN_VOICE_MEMO;
    }
}

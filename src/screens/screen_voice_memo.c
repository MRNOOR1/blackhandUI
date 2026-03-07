#include <notcurses/notcurses.h>
#include <stdint.h>
#include <stdio.h>

#include "config.h"
#include "ui.h"
#include "services/theme_service.h"
#include "services/voice_memo_service.h"

static int selected = 0;

static int safe_trunc_index(int cols, int padding, int buf_size) {
    int idx = cols - padding;
    if (idx < 0) return 0;
    if (idx >= buf_size) return buf_size - 1;
    return idx;
}

static void format_time_ms(int ms, char *buf, size_t buf_size) {
    int sec = ms / 1000;
    int min = sec / 60;
    sec %= 60;
    snprintf(buf, buf_size, "%02d:%02d", min, sec);
}

void screen_voice_memo_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    VMState st = voice_memo_service_state();
    int elapsed = voice_memo_service_elapsed_ms();

    char tbuf[16];
    format_time_ms(elapsed, tbuf, sizeof(tbuf));

    ncplane_set_fg_rgb(phone, theme_text_primary());
    ncplane_set_bg_rgb(phone, theme_bg());

    if (st == VM_RECORDING) {
        ncplane_putstr_yx(phone, 4, 2, "Recording...");
        char line[64];
        snprintf(line, sizeof(line), "Elapsed: %s", tbuf);
        ncplane_putstr_yx(phone, 6, 2, line);
        ncplane_set_fg_rgb(phone, theme_text_muted());
        ncplane_putstr_yx(phone, (int)rows - 2, 2, "[r] Stop  [b] Back");
        return;
    }

    size_t count = 0;
    const VoiceMemo **list = voice_memo_service_list_all(&count);
    if (!list && count > 0) {
        ncplane_set_fg_rgb(phone, theme_text_muted());
        ncplane_putstr_yx(phone, 4, 2, "Voice memo list unavailable");
        return;
    }

    if (selected < 0) selected = 0;
    if (count > 0 && selected >= (int)count) selected = (int)count - 1;

    if (count == 0) {
        ncplane_set_fg_rgb(phone, theme_text_muted());
        ncplane_putstr_yx(phone, 4, 2, "No voice memos yet");
        ncplane_putstr_yx(phone, 6, 2, "[r] Record  [b] Back");
        return;
    }

    int max_rows = (int)rows - 6;
    if (max_rows < 1) max_rows = 1;

    for (int i = 0; i < (int)count && i < max_rows; i++) {
        const VoiceMemo *m = list[i];
        int row = 3 + i;
        const char *cursor = (i == selected) ? MENU_CURSOR : MENU_CURSOR_BLANK;
        uint32_t fg = (i == selected) ? theme_text_primary() : theme_text_muted();
        ncplane_set_fg_rgb(phone, fg);
        ncplane_putstr_yx(phone, row, 2, cursor);

        char dbuf[16];
        format_time_ms(m->duration_ms, dbuf, sizeof(dbuf));

        char line[256];
        snprintf(line, sizeof(line), "%s  (%s)", m->filename, dbuf);
        line[safe_trunc_index((int)cols, 6, (int)sizeof(line))] = '\0';
        ncplane_putstr_yx(phone, row, 4, line);
    }

    ncplane_set_fg_rgb(phone, theme_text_muted());
    if (st == VM_PLAYING || st == VM_PAUSED) {
        const VoiceMemo *c = voice_memo_service_current();
        char sbuf[96];
        snprintf(sbuf, sizeof(sbuf), "%s: %s [%s]", (st == VM_PLAYING) ? "Playing" : "Paused", c ? c->filename : "", tbuf);
        sbuf[safe_trunc_index((int)cols, 4, (int)sizeof(sbuf))] = '\0';
        ncplane_putstr_yx(phone, (int)rows - 3, 2, sbuf);
    }
    ncplane_putstr_yx(phone, (int)rows - 2, 2, "[r] Rec  [Enter] Play  [space] Pause/Resume  [d] Del  [b] Back");
}

screen_id screen_voice_memo_input(uint32_t key) {
    size_t count = 0;
    const VoiceMemo **list = voice_memo_service_list_all(&count);
    VMState st = voice_memo_service_state();

    switch (key) {
        case NCKEY_UP:
            if (selected > 0) selected--;
            return SCREEN_VOICE_MEMO;
        case NCKEY_DOWN:
            if (selected < (int)count - 1) selected++;
            return SCREEN_VOICE_MEMO;
        case 'r':
        case 'R':
            if (st == VM_RECORDING) {
                voice_memo_service_record_stop(NULL);
                selected = 0;
            } else if (st == VM_IDLE) {
                voice_memo_service_record_start();
            }
            return SCREEN_VOICE_MEMO;
        case NCKEY_ENTER:
        case '\n':
            if (count > 0) {
                voice_memo_service_play_start(list[selected]->filename);
            }
            return SCREEN_VOICE_MEMO;
        case ' ':
            if (st == VM_PLAYING) {
                voice_memo_service_play_pause();
            } else if (st == VM_PAUSED) {
                voice_memo_service_play_resume();
            }
            return SCREEN_VOICE_MEMO;
        case 's':
        case 'S':
            if (st == VM_PLAYING || st == VM_PAUSED) {
                voice_memo_service_play_stop();
            }
            return SCREEN_VOICE_MEMO;
        case 'd':
        case 'D':
            if (count > 0 && st != VM_RECORDING) {
                voice_memo_service_delete(list[selected]->filename);
                if (selected >= (int)count - 1 && selected > 0) selected--;
            }
            return SCREEN_VOICE_MEMO;
        case NCKEY_ESC:
        case 'b':
        case 'B':
            if (st == VM_RECORDING) {
                voice_memo_service_record_stop(NULL);
            } else if (st == VM_PLAYING || st == VM_PAUSED) {
                voice_memo_service_play_stop();
            }
            return SCREEN_HOME;
        default:
            return SCREEN_VOICE_MEMO;
    }
}

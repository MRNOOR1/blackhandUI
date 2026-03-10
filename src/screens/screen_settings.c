#include <notcurses/notcurses.h>
#include <stdint.h>

#include "config.h"
#include "draw_utils.h"
#include "ui.h"
#include "services/settings_service.h"
#include "services/theme_service.h"
#include "services/contacts_service.h"
#include "services/notes_service.h"
#include "services/voice_memo_service.h"
#include "services/alarm_service.h"
#include "services/comm_service.h"

typedef enum {
    ROW_NIGHT_MODE,
    ROW_BLUETOOTH,
    ROW_HAND_WHITE,
    ROW_THEME,
    ROW_WIPE_SHUT,
} settings_row_id;

static int s_selected = 0;
static int s_wipe_prompt = 0;
static int s_wipe_yes = 0;

static int row_count(void) {
    return theme_service_is_dark() ? 4 : 5;
}

static settings_row_id row_id_from_index(int index) {
    if (index <= 0) return ROW_NIGHT_MODE;
    if (index == 1) return ROW_BLUETOOTH;
    if (index == 2) return ROW_HAND_WHITE;
    if (!theme_service_is_dark()) {
        if (index == 3) return ROW_THEME;
        return ROW_WIPE_SHUT;
    }
    return ROW_WIPE_SHUT;
}

static void wipe_phone_data(void) {
    size_t count = 0;

    const Contact **contacts = contact_service_list_all(&count);
    while (count > 0 && contacts && contacts[0] && contacts[0]->id) {
        contact_service_delete(contacts[0]->id);
        contacts = contact_service_list_all(&count);
    }

    Note **notes = notes_service_list_all(&count);
    while (count > 0 && notes && notes[0]) {
        notes_service_delete_note(notes[0]);
        notes = notes_service_list_all(&count);
    }

    const VoiceMemo **memos = voice_memo_service_list_all(&count);
    while (count > 0 && memos && memos[0] && memos[0]->filename) {
        voice_memo_service_delete(memos[0]->filename);
        memos = voice_memo_service_list_all(&count);
    }

    const Alarm **alarms = alarm_service_list_all(&count);
    while (count > 0 && alarms && alarms[0] && alarms[0]->id) {
        alarm_service_delete(alarms[0]->id);
        alarms = alarm_service_list_all(&count);
    }

    comm_service_reset();
    settings_service_reset_defaults();
    theme_service_sync_from_settings();
}

void screen_settings_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);
    int footer = (int)rows - FOOTER_ROW_OFFSET;
    int width = INNER_WIDTH(cols);

    ncplane_set_fg_rgb(phone, theme_text_primary());
    ncplane_set_bg_rgb(phone, theme_bg());
    ncplane_putstr_yx(phone, CONTENT_START_ROW, CONTENT_COL, "SETTINGS");

    ncplane_set_fg_rgb(phone, theme_text_muted());
    for (int x = 0; x < width && CONTENT_COL + x < (int)cols - 1; x++) {
        ncplane_putstr_yx(phone, CONTENT_START_ROW + 1, CONTENT_COL + x, "\u2500");
    }

    ghost_text(phone, CONTENT_START_ROW + 2, CONTENT_COL, theme_text_muted(), "MODE: CONFIG");

    int total = row_count();
    if (s_selected < 0) s_selected = 0;
    if (s_selected >= total) s_selected = total - 1;

    for (int i = 0; i < total; i++) {
        int row = CONTENT_START_ROW + 4 + i;
        if (row >= footer) break;

        const char *cursor = (i == s_selected) ? MENU_CURSOR : MENU_CURSOR_BLANK;
        ncplane_set_fg_rgb(phone, (i == s_selected) ? theme_text_primary() : theme_text_muted());
        ncplane_set_bg_rgb(phone, theme_bg());
        ncplane_putstr_yx(phone, row, CONTENT_COL, cursor);

        switch (row_id_from_index(i)) {
            case ROW_NIGHT_MODE:
                ncplane_putstr_yx(phone, row, CONTENT_COL + 2,
                                  settings_service_get_bool("night_mode") ? "[ON]  NIGHT MODE" : "[OFF] NIGHT MODE");
                break;
            case ROW_BLUETOOTH:
                ncplane_putstr_yx(phone, row, CONTENT_COL + 2,
                                  settings_service_get_bool("bluetooth") ? "[ON]  BLUETOOTH" : "[OFF] BLUETOOTH");
                break;
            case ROW_HAND_WHITE:
                ncplane_putstr_yx(phone, row, CONTENT_COL + 2,
                                  settings_service_get_bool("hand_white") ? "[ON]  HAND WHITE" : "[OFF] HAND WHITE");
                break;
            case ROW_THEME:
                ncplane_putstr_yx(phone, row, CONTENT_COL + 2, "[OPEN] THEME");
                break;
            case ROW_WIPE_SHUT:
                ncplane_putstr_yx(phone, row, CONTENT_COL + 2, "[RUN] WIPE SHUT");
                break;
        }
    }

    ghost_softkeys(phone, "[Back]", "[Select]");
    if (s_wipe_prompt) {
        ghost_confirm_popup(phone, "WIPE SHUT NOW?", s_wipe_yes);
    }
}

screen_id screen_settings_input(uint32_t key) {
    int total = row_count();

    if (s_wipe_prompt) {
        switch (key) {
            case NCKEY_LEFT:
            case NCKEY_UP:
                s_wipe_yes = 0;
                return SCREEN_SETTINGS;
            case NCKEY_RIGHT:
            case NCKEY_DOWN:
                s_wipe_yes = 1;
                return SCREEN_SETTINGS;
            case NCKEY_ENTER:
            case '\n':
            case 'e':
            case 'E':
                if (s_wipe_yes) {
                    wipe_phone_data();
                    s_wipe_prompt = 0;
                    s_wipe_yes = 0;
                    s_selected = 0;
                    return SCREEN_HOME;
                }
                s_wipe_prompt = 0;
                s_wipe_yes = 0;
                return SCREEN_SETTINGS;
            case 'q':
            case 'Q':
                s_wipe_prompt = 0;
                s_wipe_yes = 0;
                return SCREEN_SETTINGS;
            default:
                return SCREEN_SETTINGS;
        }
    }

    switch (key) {
        case NCKEY_UP:
            if (s_selected > 0) s_selected--;
            return SCREEN_SETTINGS;
        case NCKEY_DOWN:
            if (s_selected < total - 1) s_selected++;
            return SCREEN_SETTINGS;
        case NCKEY_ENTER:
        case '\n':
        case 'e':
        case 'E': {
            settings_row_id id = row_id_from_index(s_selected);
            if (id == ROW_NIGHT_MODE) {
                settings_service_toggle_by_key("night_mode");
                theme_service_sync_from_settings();
                if (s_selected >= row_count()) s_selected = row_count() - 1;
                return SCREEN_SETTINGS;
            }
            if (id == ROW_BLUETOOTH) {
                return SCREEN_BLUETOOTH;
            }
            if (id == ROW_HAND_WHITE) {
                settings_service_toggle_by_key("hand_white");
                theme_service_sync_from_settings();
                return SCREEN_SETTINGS;
            }
            if (id == ROW_THEME) {
                return SCREEN_THEME;
            }
            s_wipe_prompt = 1;
            s_wipe_yes = 0;
            return SCREEN_SETTINGS;
        }
        case 'q':
        case 'Q':
            return SCREEN_HOME;
        default:
            return SCREEN_SETTINGS;
    }
}

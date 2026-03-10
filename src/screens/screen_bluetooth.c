#include <notcurses/notcurses.h>
#include <stdint.h>
#include <stdio.h>

#include "config.h"
#include "draw_utils.h"
#include "services/settings_service.h"
#include "services/theme_service.h"
#include "ui.h"

typedef struct {
    const char *name;
    int connected;
} bt_device;

static bt_device s_devices[] = {
    { "HEADSET-A1", 0 },
    { "SPEAKER-X", 0 },
    { "KEYBOARD-M", 0 },
};
static int s_selected = 0;

void screen_bluetooth_draw(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);
    int footer = (int)rows - FOOTER_ROW_OFFSET;
    int width = INNER_WIDTH(cols);

    ncplane_set_fg_rgb(phone, theme_text_primary());
    ncplane_set_bg_rgb(phone, theme_bg());
    ncplane_putstr_yx(phone, CONTENT_START_ROW, CONTENT_COL, "BLUETOOTH");

    ncplane_set_fg_rgb(phone, theme_text_muted());
    for (int x = 0; x < width && CONTENT_COL + x < (int)cols - 1; x++) {
        ncplane_putstr_yx(phone, CONTENT_START_ROW + 1, CONTENT_COL + x, "\u2500");
    }

    int enabled = settings_service_get_bool("bluetooth") ? 1 : 0;
    ghost_text(phone, CONTENT_START_ROW + 2, CONTENT_COL, theme_text_muted(),
               enabled ? "STATE: ON" : "STATE: OFF");

    int count = (int)(sizeof(s_devices) / sizeof(s_devices[0]));
    if (s_selected < 0) s_selected = 0;
    if (s_selected >= count) s_selected = count - 1;

    for (int i = 0; i < count; i++) {
        int row = CONTENT_START_ROW + 4 + i;
        if (row >= footer) break;
        char line[64];
        snprintf(line, sizeof(line), "%s %-12s %s",
                 (i == s_selected) ? MENU_CURSOR : MENU_CURSOR_BLANK,
                 s_devices[i].name,
                 s_devices[i].connected ? "CONNECTED" : "READY");
        ncplane_set_fg_rgb(phone, (i == s_selected) ? theme_text_primary() : theme_text_muted());
        ncplane_set_bg_rgb(phone, theme_bg());
        ncplane_putstr_yx(phone, row, CONTENT_COL, line);
    }

    ghost_softkeys(phone, "[Back]", enabled ? "[Connect]" : "[Enable]");
}

screen_id screen_bluetooth_input(uint32_t key) {
    int count = (int)(sizeof(s_devices) / sizeof(s_devices[0]));
    int enabled = settings_service_get_bool("bluetooth") ? 1 : 0;

    switch (key) {
        case NCKEY_UP:
            if (s_selected > 0) s_selected--;
            return SCREEN_BLUETOOTH;
        case NCKEY_DOWN:
            if (s_selected < count - 1) s_selected++;
            return SCREEN_BLUETOOTH;
        case NCKEY_ENTER:
        case '\n':
        case 'e':
        case 'E':
            if (!enabled) {
                settings_service_toggle_by_key("bluetooth");
                theme_service_sync_from_settings();
                return SCREEN_BLUETOOTH;
            }
            for (int i = 0; i < count; i++) s_devices[i].connected = 0;
            s_devices[s_selected].connected = 1;
            return SCREEN_BLUETOOTH;
        case 'q':
        case 'Q':
            return SCREEN_SETTINGS;
        default:
            return SCREEN_BLUETOOTH;
    }
}

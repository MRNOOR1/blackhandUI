#include <notcurses/notcurses.h>
#include <stdint.h>

#include "config.h"
#include "ui.h"

/*
 * screen_contacts.c
 *
 * Purpose:
 * - UI layer for viewing and navigating contacts.
 *
 * What you should implement next:
 * - A contacts list model (name + phone number).
 * - Selection with Up/Down keys.
 * - Open-contact action on Enter.
 * - Search/filter later (optional).
 *
 * Coupling/cohesion rule:
 * - This file should only handle drawing + input routing.
 * - Store and query contacts through a service module (not directly here).
 */

void screen_contacts_draw(struct ncplane *phone) {
    ncplane_set_bg_rgb(phone, COL_BG);
    ncplane_set_fg_rgb(phone, COL_PLACEHOLDER);
    ncplane_putstr_yx(phone, 4, 2, "Contacts screen TODO");
    ncplane_set_fg_rgb(phone, COL_HINT);
    ncplane_putstr_yx(phone, 6, 2, "[b] Back");
}

screen_id screen_contacts_input(uint32_t key) {
    switch (key) {
        case NCKEY_ESC:
        case 'b':
        case 'B':
            return SCREEN_HOME;
        default:
            return SCREEN_CONTACTS;
    }
}

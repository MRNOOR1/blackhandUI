#include <notcurses/notcurses.h>
#include <stdint.h>

#include "config.h"
#include "ui.h"

/*
 * screen_notes.c
 *
 * Purpose:
 * - UI for notes list and note editing.
 *
 * What you should implement next:
 * - Notes list selection (Up/Down).
 * - Create note action.
 * - Open note for edit.
 * - Save/delete note actions.
 *
 * Coupling/cohesion rule:
 * - Do not store note data in static screen variables.
 * - Use notes_service for CRUD and persistence.
 */

void screen_notes_draw(struct ncplane *phone) {
    ncplane_set_bg_rgb(phone, COL_BG);
    ncplane_set_fg_rgb(phone, COL_PLACEHOLDER);
    ncplane_putstr_yx(phone, 4, 2, "Notes screen TODO");
    ncplane_set_fg_rgb(phone, COL_HINT);
    ncplane_putstr_yx(phone, 6, 2, "[Enter] Open  [b] Back");
}

screen_id screen_notes_input(uint32_t key) {
    switch (key) {
        case NCKEY_ESC:
        case 'b':
        case 'B':
            return SCREEN_HOME;
        default:
            return SCREEN_NOTES;
    }
}

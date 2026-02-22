#include <notcurses/notcurses.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "ui.h"
#include "services/notes_service.h"
#include "services/theme_service.h"
#include "draw_utils.h"

/*
 * screen_notes.c
 *
 * Notes screen with two modes:
 *   LIST mode  — browse notes, create/delete
 *   VIEW mode  — read a single note's content
 *
 * Coupling/cohesion:
 *   All note data comes from notes_service.
 *   This file only handles drawing + input routing.
 */

/* ── Screen modes ─────────────────────────────────────────────────────── */
typedef enum {
    NOTES_MODE_LIST,
    NOTES_MODE_VIEW,
} notes_mode_t;

/* ── Screen state (static = private to this file) ─────────────────────── */
static notes_mode_t mode = NOTES_MODE_LIST;
static int selected = 0;
static int scroll_offset = 0;  /* for content scrolling in view mode */

/* ── Layout constants ─────────────────────────────────────────────────── */
#define NOTES_START_ROW     3
#define NOTES_COL           2
#define NOTES_HINT_ROW_OFFSET 2  /* rows from bottom for hints */

/* ── LIST mode draw ───────────────────────────────────────────────────── */
static void draw_list(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    size_t count = notes_service_note_count();

    /* Empty state */
    if (count == 0) {
        ghost_text(phone, NOTES_START_ROW, NOTES_COL,
                   theme_text_muted(), "No notes yet");
        ghost_text(phone, NOTES_START_ROW + 2, NOTES_COL,
                   theme_text_muted(), "[n] New  [b] Back");
        return;
    }

    /* Clamp selected index */
    if (selected < 0) selected = 0;
    if (selected >= (int)count) selected = (int)count - 1;

    Note **notes = notes_service_list_all(NULL);

    /* How many rows available for note items */
    int max_visible = (int)rows - NOTES_START_ROW - NOTES_HINT_ROW_OFFSET;
    if (max_visible < 1) max_visible = 1;

    for (int i = 0; i < (int)count && i < max_visible; i++) {
        int row = NOTES_START_ROW + i;
        if (row >= (int)rows - NOTES_HINT_ROW_OFFSET) break;

        Note *n = notes[i];
        uint32_t fg = (i == selected) ? theme_text_primary() : theme_text_muted();
        const char *cursor = (i == selected) ? MENU_CURSOR : MENU_CURSOR_BLANK;

        ncplane_set_fg_rgb(phone, fg);
        ncplane_set_bg_rgb(phone, theme_bg());
        ncplane_putstr_yx(phone, row, NOTES_COL, cursor);

        /* Truncate title to fit within border */
        int title_max = (int)cols - NOTES_COL - 4;  /* 2 cursor + 2 border */
        char title_buf[128];
        if (n->title) {
            strncpy(title_buf, n->title, sizeof(title_buf) - 1);
            title_buf[sizeof(title_buf) - 1] = '\0';
            if ((int)strlen(title_buf) > title_max && title_max > 3) {
                title_buf[title_max - 3] = '.';
                title_buf[title_max - 2] = '.';
                title_buf[title_max - 1] = '.';
                title_buf[title_max] = '\0';
            }
        } else {
            strncpy(title_buf, "Untitled", sizeof(title_buf));
        }

        ncplane_putstr_yx(phone, row, NOTES_COL + 2, title_buf);
    }

    /* Hints at bottom */
    ghost_text(phone, (int)rows - 2, NOTES_COL,
               theme_text_muted(), "[Enter]Open [n]New [d]Del [b]Back");
}

/* ── VIEW mode draw ───────────────────────────────────────────────────── */
static void draw_view(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    size_t count = notes_service_note_count();
    if (selected < 0 || selected >= (int)count) {
        mode = NOTES_MODE_LIST;
        return;
    }

    Note **notes = notes_service_list_all(NULL);
    Note *n = notes[selected];

    /* Title */
    ghost_text(phone, NOTES_START_ROW, NOTES_COL,
               theme_text_primary(), n->title ? n->title : "Untitled");

    /* Date */
    ghost_text(phone, NOTES_START_ROW + 1, NOTES_COL,
               theme_text_muted(), n->created_at ? n->created_at : "");

    /* Content area — show lines with scroll offset */
    int content_start = NOTES_START_ROW + 3;
    int max_lines = (int)rows - content_start - NOTES_HINT_ROW_OFFSET;
    if (max_lines < 1) max_lines = 1;

    int content_width = (int)cols - NOTES_COL - 2;  /* inside borders */
    if (content_width < 1) content_width = 1;

    if (n->content && strlen(n->content) > 0) {
        /* Walk through content line by line */
        const char *ptr = n->content;
        int line_num = 0;
        int drawn = 0;

        while (*ptr && drawn < max_lines) {
            /* Find end of current line */
            const char *eol = strchr(ptr, '\n');
            int line_len = eol ? (int)(eol - ptr) : (int)strlen(ptr);

            if (line_num >= scroll_offset) {
                /* Draw this line, truncated to fit */
                char line_buf[256];
                int copy_len = line_len;
                if (copy_len > content_width) copy_len = content_width;
                if (copy_len > (int)sizeof(line_buf) - 1)
                    copy_len = (int)sizeof(line_buf) - 1;

                memcpy(line_buf, ptr, copy_len);
                line_buf[copy_len] = '\0';

                ghost_text(phone, content_start + drawn, NOTES_COL,
                           theme_text_primary(), line_buf);
                drawn++;
            }

            line_num++;
            if (eol)
                ptr = eol + 1;
            else
                break;
        }
    } else {
        ghost_text(phone, content_start, NOTES_COL,
                   theme_text_muted(), "(empty)");
    }

    /* Hints */
    ghost_text(phone, (int)rows - 2, NOTES_COL,
               theme_text_muted(), "[b]Back to list");
}

/* ── Public draw ──────────────────────────────────────────────────────── */
void screen_notes_draw(struct ncplane *phone) {
    switch (mode) {
        case NOTES_MODE_LIST: draw_list(phone); break;
        case NOTES_MODE_VIEW: draw_view(phone); break;
    }
}

/* ── Public input ─────────────────────────────────────────────────────── */
screen_id screen_notes_input(uint32_t key) {
    size_t count = notes_service_note_count();

    if (mode == NOTES_MODE_LIST) {
        switch (key) {
            case NCKEY_UP:
                if (selected > 0) selected--;
                return SCREEN_NOTES;

            case NCKEY_DOWN:
                if (selected < (int)count - 1) selected++;
                return SCREEN_NOTES;

            case NCKEY_ENTER:
            case '\n':
                if (count > 0) {
                    mode = NOTES_MODE_VIEW;
                    scroll_offset = 0;
                }
                return SCREEN_NOTES;

            case 'n':
            case 'N':
                notes_service_create("New Note", "");
                selected = 0;  /* new note is at front */
                return SCREEN_NOTES;

            case 'd':
            case 'D':
                if (count > 0) {
                    Note **notes = notes_service_list_all(NULL);
                    notes_service_delete_note(notes[selected]);
                    count = notes_service_note_count();
                    if (selected >= (int)count && selected > 0)
                        selected--;
                }
                return SCREEN_NOTES;

            case NCKEY_ESC:
            case 'b':
            case 'B':
                return SCREEN_HOME;

            default:
                return SCREEN_NOTES;
        }
    }

    /* VIEW mode input */
    switch (key) {
        case NCKEY_UP:
            if (scroll_offset > 0) scroll_offset--;
            return SCREEN_NOTES;

        case NCKEY_DOWN:
            scroll_offset++;
            return SCREEN_NOTES;

        case NCKEY_ESC:
        case 'b':
        case 'B':
            mode = NOTES_MODE_LIST;
            scroll_offset = 0;
            return SCREEN_NOTES;

        default:
            return SCREEN_NOTES;
    }
}

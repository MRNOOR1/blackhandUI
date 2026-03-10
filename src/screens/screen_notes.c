#include <notcurses/notcurses.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "draw_utils.h"
#include "ui.h"
#include "services/notes_service.h"
#include "services/theme_service.h"

typedef enum {
    NOTES_MODE_LIST,
    NOTES_MODE_VIEW,
    NOTES_MODE_EDIT,
} notes_mode_t;

static notes_mode_t mode = NOTES_MODE_LIST;
static int selected = 0;
static int list_scroll = 0;
static int scroll_offset = 0;

static int delete_prompt = 0;
static int delete_choice_yes = 0;
static int delete_target = -1;

static int edit_is_new = 1;
static int edit_index = -1;
static int edit_field = 0; /* 0 title, 1 content */
static char edit_title[128];
static char edit_body[2048];

static void put_clipped(struct ncplane *p, int row, int col, int max_w, const char *text) {
    if (!text || max_w <= 0) return;
    char buf[512];
    snprintf(buf, sizeof(buf), "%s", text);
    if ((int)strlen(buf) > max_w) {
        if (max_w > 3) { buf[max_w - 3] = '.'; buf[max_w - 2] = '.'; buf[max_w - 1] = '.'; }
        buf[max_w] = '\0';
    }
    ncplane_putstr_yx(p, row, col, buf);
}

static void draw_delete_popup(struct ncplane *phone, unsigned rows, unsigned cols) {
    (void)rows;
    (void)cols;
    ghost_confirm_popup(phone, "ARE YOU SURE?", delete_choice_yes);
}

static void begin_new_note_edit(void) {
    edit_is_new = 1;
    edit_index = -1;
    edit_field = 0;
    snprintf(edit_title, sizeof(edit_title), "");
    snprintf(edit_body, sizeof(edit_body), "");
    mode = NOTES_MODE_EDIT;
}

static void begin_edit_existing(int index) {
    size_t count = notes_service_note_count();
    if (index < 0 || index >= (int)count) return;
    Note **notes = notes_service_list_all(NULL);
    if (!notes || !notes[index]) return;

    edit_is_new = 0;
    edit_index = index;
    edit_field = 0;
    snprintf(edit_title, sizeof(edit_title), "%s", notes[index]->title ? notes[index]->title : "");
    snprintf(edit_body, sizeof(edit_body), "%s", notes[index]->content ? notes[index]->content : "");
    mode = NOTES_MODE_EDIT;
}

static void save_edit(void) {
    const char *title = (edit_title[0] == '\0') ? "Untitled" : edit_title;
    const char *body = edit_body;

    if (edit_is_new) {
        notes_service_create(title, body);
        selected = 0;
        list_scroll = 0;
    } else {
        size_t count = notes_service_note_count();
        if (edit_index >= 0 && edit_index < (int)count) {
            Note **notes = notes_service_list_all(NULL);
            if (notes && notes[edit_index]) {
                Note *n = notes[edit_index];
                free(n->title);
                free(n->content);
                n->title = strdup(title);
                n->content = strdup(body);
                notes_service_update_note(n);
                selected = 0;
                list_scroll = 0;
            }
        }
    }

    mode = NOTES_MODE_LIST;
}

static void draw_list(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);
    int footer = (int)rows - FOOTER_ROW_OFFSET;
    int width = INNER_WIDTH(cols);
    if (width < 10) return;

    ncplane_set_fg_rgb(phone, theme_text_primary());
    ncplane_set_bg_rgb(phone, theme_bg());
    ncplane_putstr_yx(phone, CONTENT_START_ROW, CONTENT_COL, "NOTES");

    ncplane_set_fg_rgb(phone, theme_text_muted());
    for (int x = 0; x < width && CONTENT_COL + x < (int)cols - 1; x++) {
        ncplane_putstr_yx(phone, CONTENT_START_ROW + 1, CONTENT_COL + x, "\u2500");
    }

    size_t count = notes_service_note_count();
    if (count == 0) {
        int mid = (CONTENT_START_ROW + 2 + footer) / 2;
        ghost_text(phone, mid, CONTENT_COL, theme_text_muted(), "No notes yet");
        ghost_text(phone, mid + 1, CONTENT_COL, theme_text_muted(), "Right soft key creates note");
        ghost_softkeys(phone, "[Back]", "[New]");
        if (delete_prompt) draw_delete_popup(phone, rows, cols);
        return;
    }

    if (selected < 0) selected = 0;
    if (selected >= (int)count) selected = (int)count - 1;

    Note **notes = notes_service_list_all(NULL);
    if (!notes) return;

    int list_start = CONTENT_START_ROW + 3;
    int max_visible = footer - list_start;
    if (max_visible < 1) max_visible = 1;

    if (selected < list_scroll) list_scroll = selected;
    if (selected >= list_scroll + max_visible) list_scroll = selected - max_visible + 1;
    if (list_scroll < 0) list_scroll = 0;

    for (int i = 0; i < max_visible; i++) {
        int idx = list_scroll + i;
        if (idx >= (int)count) break;
        int row = list_start + i;
        int sel = (idx == selected);

        ncplane_set_fg_rgb(phone, sel ? theme_text_primary() : theme_text_muted());
        ncplane_set_bg_rgb(phone, theme_bg());
        ncplane_putstr_yx(phone, row, CONTENT_COL, sel ? MENU_CURSOR : MENU_CURSOR_BLANK);
        put_clipped(phone, row, CONTENT_COL + 2, width - 2, notes[idx]->title ? notes[idx]->title : "Untitled");
    }

    ghost_text(phone, footer - 1, CONTENT_COL, theme_text_muted(), "Left:Delete  Enter:Open/Edit");
    ghost_softkeys(phone, "[Back]", "[New]");
    if (delete_prompt) draw_delete_popup(phone, rows, cols);
}

static void draw_view(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);
    int footer = (int)rows - FOOTER_ROW_OFFSET;
    int width = INNER_WIDTH(cols);
    if (width < 10) return;

    size_t count = notes_service_note_count();
    if (selected < 0 || selected >= (int)count) { mode = NOTES_MODE_LIST; return; }
    Note **notes = notes_service_list_all(NULL);
    if (!notes || !notes[selected]) { mode = NOTES_MODE_LIST; return; }
    Note *n = notes[selected];

    ncplane_set_fg_rgb(phone, theme_text_primary());
    ncplane_set_bg_rgb(phone, theme_bg());
    put_clipped(phone, CONTENT_START_ROW, CONTENT_COL, width, n->title ? n->title : "Untitled");

    ncplane_set_fg_rgb(phone, theme_text_muted());
    put_clipped(phone, CONTENT_START_ROW + 1, CONTENT_COL, width, n->created_at ? n->created_at : "");

    for (int x = 0; x < width && CONTENT_COL + x < (int)cols - 1; x++) {
        ncplane_putstr_yx(phone, CONTENT_START_ROW + 2, CONTENT_COL + x, "\u2500");
    }

    int content_start = CONTENT_START_ROW + 3;
    int max_lines = footer - content_start;
    if (max_lines < 1) max_lines = 1;

    if (n->content && n->content[0] != '\0') {
        const char *ptr = n->content;
        int line_num = 0;
        int drawn = 0;
        while (*ptr && drawn < max_lines) {
            const char *eol = strchr(ptr, '\n');
            int line_len = eol ? (int)(eol - ptr) : (int)strlen(ptr);
            if (line_num >= scroll_offset) {
                char line[256];
                int copy_len = line_len;
                if (copy_len > width) copy_len = width;
                if (copy_len > 255) copy_len = 255;
                memcpy(line, ptr, copy_len);
                line[copy_len] = '\0';
                ncplane_set_fg_rgb(phone, theme_text_primary());
                ncplane_putstr_yx(phone, content_start + drawn, CONTENT_COL, line);
                drawn++;
            }
            line_num++;
            if (eol) ptr = eol + 1; else break;
        }
    } else {
        ghost_text(phone, content_start, CONTENT_COL, theme_text_muted(), "(empty)");
    }

    ghost_softkeys(phone, "[Back]", "[Edit]");
}

static void draw_edit(struct ncplane *phone) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);
    int footer = (int)rows - FOOTER_ROW_OFFSET;
    int width = INNER_WIDTH(cols);

    ncplane_set_fg_rgb(phone, theme_text_primary());
    ncplane_set_bg_rgb(phone, theme_bg());
    ncplane_putstr_yx(phone, CONTENT_START_ROW, CONTENT_COL, edit_is_new ? "New Note" : "Edit Note");

    ncplane_set_fg_rgb(phone, theme_text_muted());
    for (int x = 0; x < width && CONTENT_COL + x < (int)cols - 1; x++) {
        ncplane_putstr_yx(phone, CONTENT_START_ROW + 1, CONTENT_COL + x, "\u2500");
    }

    ncplane_set_fg_rgb(phone, edit_field == 0 ? theme_text_primary() : theme_text_muted());
    ncplane_putstr_yx(phone, CONTENT_START_ROW + 3, CONTENT_COL, "Title:");
    put_clipped(phone, CONTENT_START_ROW + 4, CONTENT_COL, width, edit_title);

    ncplane_set_fg_rgb(phone, edit_field == 1 ? theme_text_primary() : theme_text_muted());
    ncplane_putstr_yx(phone, CONTENT_START_ROW + 6, CONTENT_COL, "Content:");

    int body_row = CONTENT_START_ROW + 7;
    int body_max = footer - body_row - 1;
    if (body_max < 1) body_max = 1;

    const char *ptr = edit_body;
    for (int i = 0; i < body_max; i++) {
        if (!ptr || *ptr == '\0') break;
        const char *eol = strchr(ptr, '\n');
        int len = eol ? (int)(eol - ptr) : (int)strlen(ptr);
        char line[256];
        int copy_len = len;
        if (copy_len > width) copy_len = width;
        if (copy_len > 255) copy_len = 255;
        memcpy(line, ptr, copy_len);
        line[copy_len] = '\0';
        ncplane_set_fg_rgb(phone, theme_text_primary());
        ncplane_putstr_yx(phone, body_row + i, CONTENT_COL, line);
        if (!eol) break;
        ptr = eol + 1;
    }

    ghost_text(phone, footer - 1, CONTENT_COL, theme_text_muted(), "Up/Down:Field  Enter:Next/Newline");
    ghost_softkeys(phone, "[Q Cancel]", "[E Save]");
}

void screen_notes_draw(struct ncplane *phone) {
    switch (mode) {
        case NOTES_MODE_LIST: draw_list(phone); break;
        case NOTES_MODE_VIEW: draw_view(phone); break;
        case NOTES_MODE_EDIT: draw_edit(phone); break;
    }
}

screen_id screen_notes_input(uint32_t key) {
    size_t count = notes_service_note_count();

    if (mode == NOTES_MODE_LIST) {
        if (delete_prompt) {
            switch (key) {
                case NCKEY_LEFT:
                case NCKEY_UP:
                    delete_choice_yes = 0;
                    return SCREEN_NOTES;
                case NCKEY_RIGHT:
                case NCKEY_DOWN:
                    delete_choice_yes = 1;
                    return SCREEN_NOTES;
                case NCKEY_ENTER:
                case '\n':
            case 'e':
            case 'E':
                if (delete_choice_yes && delete_target >= 0) {
                        Note **notes = notes_service_list_all(NULL);
                        if (notes && delete_target < (int)count) {
                            notes_service_delete_note(notes[delete_target]);
                        }
                        count = notes_service_note_count();
                        if (selected >= (int)count && selected > 0) selected--;
                    }
                    delete_prompt = 0;
                    delete_target = -1;
                    delete_choice_yes = 0;
                    return SCREEN_NOTES;
            case 'q':
            case 'Q':
                delete_prompt = 0;
                    delete_target = -1;
                    delete_choice_yes = 0;
                    return SCREEN_NOTES;
                default:
                    return SCREEN_NOTES;
            }
        }

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
                    begin_edit_existing(selected);
                } else {
                    begin_new_note_edit();
                }
                return SCREEN_NOTES;
            case NCKEY_RIGHT:
            case 'e':
            case 'E':
                begin_new_note_edit();
                return SCREEN_NOTES;
            case NCKEY_LEFT:
                if (count > 0) {
                    delete_prompt = 1;
                    delete_choice_yes = 0;
                    delete_target = selected;
                }
                return SCREEN_NOTES;
            case 'q':
            case 'Q':
                return SCREEN_HOME;
            default:
                return SCREEN_NOTES;
        }
    }

    if (mode == NOTES_MODE_VIEW) {
        switch (key) {
            case NCKEY_UP:
                if (scroll_offset > 0) scroll_offset--;
                return SCREEN_NOTES;
            case NCKEY_DOWN:
                scroll_offset++;
                return SCREEN_NOTES;
            case 'e':
            case 'E':
            case NCKEY_ENTER:
            case '\n':
                begin_edit_existing(selected);
                return SCREEN_NOTES;
            case 'q':
            case 'Q':
                mode = NOTES_MODE_LIST;
                scroll_offset = 0;
                return SCREEN_NOTES;
            default:
                return SCREEN_NOTES;
        }
    }

    switch (key) {
        case NCKEY_UP:
            if (edit_field > 0) edit_field--;
            return SCREEN_NOTES;
        case NCKEY_DOWN:
            if (edit_field < 1) edit_field++;
            return SCREEN_NOTES;
        case NCKEY_TAB:
            edit_field = 1 - edit_field;
            return SCREEN_NOTES;
        case 'E':
            save_edit();
            return SCREEN_NOTES;
        case 'Q':
            mode = NOTES_MODE_LIST;
            return SCREEN_NOTES;
        case NCKEY_ENTER:
        case '\n':
            if (edit_field == 0) {
                edit_field = 1;
            } else {
                size_t len = strlen(edit_body);
                if (len + 1 < sizeof(edit_body)) {
                    edit_body[len] = '\n';
                    edit_body[len + 1] = '\0';
                }
            }
            return SCREEN_NOTES;
        case NCKEY_BACKSPACE:
        case 127:
            if (edit_field == 0) {
                size_t len = strlen(edit_title);
                if (len > 0) edit_title[len - 1] = '\0';
            } else {
                size_t len = strlen(edit_body);
                if (len > 0) edit_body[len - 1] = '\0';
            }
            return SCREEN_NOTES;
        case NCKEY_RIGHT:
            if (edit_field == 1) {
                size_t len = strlen(edit_body);
                if (len + 1 < sizeof(edit_body)) {
                    edit_body[len] = '\n';
                    edit_body[len + 1] = '\0';
                }
            }
            return SCREEN_NOTES;
        default:
            if (key >= 32 && key <= 126) {
                if (edit_field == 0) {
                    size_t len = strlen(edit_title);
                    if (len + 1 < sizeof(edit_title)) {
                        edit_title[len] = (char)key;
                        edit_title[len + 1] = '\0';
                    }
                } else {
                    size_t len = strlen(edit_body);
                    if (len + 1 < sizeof(edit_body)) {
                        edit_body[len] = (char)key;
                        edit_body[len + 1] = '\0';
                    }
                }
            }
            return SCREEN_NOTES;
    }
}

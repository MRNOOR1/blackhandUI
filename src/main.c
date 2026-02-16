#include <locale.h>
#include <notcurses/notcurses.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ui.h"
#include "config.h"

// ── Create the phone plane ──────────────────────
static struct ncplane *create_phone_plane(struct ncplane *std) {
    unsigned term_rows, term_cols;
    ncplane_dim_yx(std, &term_rows, &term_cols);

    int start_y = (term_rows - PHONE_ROWS) / 2;
    int start_x = (term_cols - PHONE_COLS) / 2;
    if (start_y < 0) start_y = 0;
    if (start_x < 0) start_x = 0;

    struct ncplane_options opts = {
        .y = start_y,
        .x = start_x,
        .rows = PHONE_ROWS,
        .cols = PHONE_COLS,
        .name = "phone",
    };

    return ncplane_create(std, &opts);
}

// ── Draw the frame ──────────────────────────────
static void draw_frame(struct ncplane *phone, const char *title) {
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    ncplane_erase(phone);

    if (rows < FRAME_MIN_ROWS || cols < FRAME_MIN_COLS) return;

    // Fill background
    ncplane_set_bg_rgb(phone, COL_BG);
    for (unsigned y = 0; y < rows; y++) {
        for (unsigned x = 0; x < cols; x++) {
            ncplane_putchar_yx(phone, y, x, ' ');
        }
    }

    // Border
    ncplane_set_fg_rgb(phone, COL_BORDER);
    nccell ul = NCCELL_TRIVIAL_INITIALIZER;
    nccell ur = NCCELL_TRIVIAL_INITIALIZER;
    nccell ll = NCCELL_TRIVIAL_INITIALIZER;
    nccell lr = NCCELL_TRIVIAL_INITIALIZER;
    nccell hl = NCCELL_TRIVIAL_INITIALIZER;
    nccell vl = NCCELL_TRIVIAL_INITIALIZER;
    nccells_heavy_box(phone, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl);

    ncplane_cursor_move_yx(phone, 0, 0);
    ncplane_box(phone, &ul, &ur, &ll, &lr, &hl, &vl,
                rows - 1, cols - 1, 0);

    nccell_release(phone, &ul);
    nccell_release(phone, &ur);
    nccell_release(phone, &ll);
    nccell_release(phone, &lr);
    nccell_release(phone, &hl);
    nccell_release(phone, &vl);

    // Header
    ncplane_set_fg_rgb(phone, COL_HEADER_TEXT);
    ncplane_putstr_yx(phone, 0, 1, TEXT_BRAND);

    if (title) {
        int title_x = cols - (int)strlen(title) - 2;
        if (title_x > 5) {
            ncplane_putstr_yx(phone, 0, title_x, title);
        }
    }

    // Header separator
    ncplane_set_fg_rgb(phone, COL_SEPARATOR);
    ncplane_putstr_yx(phone, 1, 0, "┣");
    nccell sep = NCCELL_TRIVIAL_INITIALIZER;
    nccell_load(phone, &sep, "━");
    nccell_set_fg_rgb(&sep, COL_SEPARATOR);
    ncplane_cursor_move_yx(phone, 1, 1);
    ncplane_hline(phone, &sep, cols - 2);
    nccell_release(phone, &sep);
    ncplane_putstr_yx(phone, 1, cols - 1, "┫");

    // Footer separator
    ncplane_putstr_yx(phone, rows - 2, 0, "┣");
    nccell fsep = NCCELL_TRIVIAL_INITIALIZER;
    nccell_load(phone, &fsep, "━");
    nccell_set_fg_rgb(&fsep, COL_SEPARATOR);
    ncplane_cursor_move_yx(phone, rows - 2, 1);
    ncplane_hline(phone, &fsep, cols - 2);
    nccell_release(phone, &fsep);
    ncplane_putstr_yx(phone, rows - 2, cols - 1, "┫");

    // Footer
    ncplane_set_fg_rgb(phone, COL_FOOTER_TEXT);
    ncplane_putstr_yx(phone, rows - 1, 1, TEXT_FOOTER);
}

// ── Main ────────────────────────────────────────
int main(void) {
    setlocale(LC_ALL, "");

    struct notcurses_options opts = {
        .flags = NCOPTION_SUPPRESS_BANNERS,
    };

    struct notcurses *nc = notcurses_init(&opts, NULL);
    if (!nc) {
        fprintf(stderr, "Notcurses init failed\n");
        return 1;
    }

    struct ncplane *std = notcurses_stdplane(nc);

    // Dev mode label
    ncplane_set_fg_rgb(std, COL_DEV_LABEL);
    ncplane_putstr_yx(std, 0, 2, TEXT_DEV_LABEL);

    struct ncplane *phone = create_phone_plane(std);
    if (!phone) {
        notcurses_stop(nc);
        return 1;
    }

    // ── Active screen state ─────────────────────
    screen_id current_screen = SCREEN_HOME;

    // ── Main loop ───────────────────────────────
    while (1) {

        // ── 1 & 2: Draw ────────────────────────
        const char *title = NULL;
        switch (current_screen) {
            case SCREEN_HOME:     title = "Home"; break;
            case SCREEN_SETTINGS: title = "Settings"; break;
            case SCREEN_CALLS:    title = "Calls"; break;
            case SCREEN_MESSAGES: title = "Messages"; break;
            default:              title = ""; break;
        }

        draw_frame(phone, title);

        switch (current_screen) {
            case SCREEN_HOME:
                screen_home_draw(phone);
                break;
            case SCREEN_SETTINGS:
                screen_settings_draw(phone);
                break;
            default:
                ncplane_set_fg_rgb(phone, COL_PLACEHOLDER);
                ncplane_putstr_yx(phone, 4, 3, TEXT_COMING_SOON);
                ncplane_set_fg_rgb(phone, COL_HINT);
                ncplane_putstr_yx(phone, 6, 3, TEXT_GO_HOME);
                break;
        }

        // ── 3: Render ──────────────────────────
        notcurses_render(nc);

        // ── 4: Wait for input ──────────────────
        ncinput ni;
        uint32_t key = notcurses_get_blocking(nc, &ni);

        // ── Global keys (work on any screen) ───
        if (key == NCKEY_RESIZE) continue;
        if (key == 'q' || key == 'Q') break;
        if (key == 'h' || key == 'H') {
            current_screen = SCREEN_HOME;
            continue;
        }

        // ── 5 & 6: Route to screen handler ─────
        switch (current_screen) {
            case SCREEN_HOME:
                current_screen = screen_home_input(key);
                break;
            default:
                break;
        }
    }

    ncplane_destroy(phone);
    notcurses_stop(nc);
    return 0;
}

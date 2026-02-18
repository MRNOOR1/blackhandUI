/*
 * ============================================================================
 *  screen_settings.c — Settings Screen
 * ============================================================================
 *
 *  WHAT IS THIS FILE?
 *  ------------------
 *  This file implements the Settings screen. Currently it displays static
 *  placeholder information, but it's a template for adding real settings.
 *
 *  This is a simple screen - it demonstrates:
 *    - Basic screen structure (draw function only, no input handling yet)
 *    - Drawing text at specific positions
 *    - Using configuration constants for layout
 *
 *
 *  C CONCEPTS DEMONSTRATED:
 *  ------------------------
 *  1. Function definitions - implementing declared functions
 *  2. Early returns - exiting a function before the end
 *  3. Using preprocessor constants from headers
 *
 *
 *  HOW TO MODIFY:
 *  --------------
 *  - Add more settings items: Add more ncplane_putstr_yx() calls
 *  - Make settings interactive: Create screen_settings_input() function
 *  - Change colors: Edit COL_SETTINGS_* in config.h
 *  - Change layout: Edit SETTINGS_* constants in config.h
 *
 *
 *  HOW TO ADD INPUT HANDLING:
 *  --------------------------
 *  1. Add a static variable for selection (like screen_home.c)
 *  2. Create screen_settings_input(uint32_t key) function
 *  3. Declare it in ui.h
 *  4. Call it from the switch in main.c
 *
 * ============================================================================
 */

/* ═══════════════════════════════════════════════════════════════════════════
 *  HEADER INCLUDES
 * ═══════════════════════════════════════════════════════════════════════════
 */

/*
 * <notcurses/notcurses.h> - Notcurses Library
 *
 * Provides:
 *   - struct ncplane type for the drawing surface
 *   - ncplane_dim_yx() to get plane dimensions
 *   - ncplane_set_fg_rgb() / ncplane_set_bg_rgb() for colors
 *   - ncplane_putstr_yx() for drawing text
 */
#include <notcurses/notcurses.h>

/*
 * "ui.h" - Our UI Definitions
 *
 * Provides:
 *   - Function declarations (so main.c knows about screen_settings_draw)
 *   - screen_id enum (though we don't use it in this simple screen)
 */
#include "ui.h"

/*
 * "config.h" - Configuration Constants
 *
 * Provides:
 *   - SETTINGS_* layout constants (rows, columns for positioning)
 *   - COL_SETTINGS_* color values
 *   - TEXT_SCREEN_TOO_SMALL error message
 *   - COL_BG background color
 */
#include "config.h"


/* ═══════════════════════════════════════════════════════════════════════════
 *  DRAW FUNCTION
 * ═══════════════════════════════════════════════════════════════════════════
 */

/*
 * screen_settings_draw() - Draw the settings screen
 *
 * WHAT IT DOES:
 * Displays a simple settings screen with a header and some placeholder
 * settings items. This is a static display - no interactive elements yet.
 *
 * PARAMETERS:
 *   phone - Pointer to the ncplane to draw on
 *           This is the phone's drawing surface, passed from main.c
 *
 * CALLED BY:
 *   The main event loop in main.c when current_screen == SCREEN_SETTINGS
 *
 * OUTPUT LAYOUT:
 *   Row 3: "Settings" (header)
 *   Row 5: "Display: Auto"
 *   Row 6: "Audio: Beep"
 *   Row 7: "Storage: 12 GB free"
 *
 *
 * C CONCEPT: VOID RETURN TYPE
 * ---------------------------
 * 'void' means the function doesn't return any value.
 * Compare to screen_home_input() which returns a screen_id.
 *
 * EXAMPLE:
 *   void say_hello(const char *name) {
 *       printf("Hello, %s!\n", name);
 *       // No return statement needed (or use "return;" to exit early)
 *   }
 */
void screen_settings_draw(struct ncplane *phone) {
    /*
     * Get the plane dimensions.
     *
     * We need to know the size to:
     *   1. Check if the plane is too small to draw
     *   2. Potentially calculate positions based on size
     *
     * NOTCURSES: ncplane_dim_yx(plane, &rows, &cols)
     *   Writes the plane's height into 'rows' and width into 'cols'.
     *   We pass addresses (&) so the function can modify our variables.
     */
    unsigned rows, cols;
    ncplane_dim_yx(phone, &rows, &cols);

    /*
     * Safety check: Don't try to draw on a tiny plane.
     *
     * SETTINGS_MIN_ROWS and SETTINGS_MIN_COLS are defined in config.h.
     * If the terminal window is smaller than these values, drawing would
     * look garbled, so we show an error message instead.
     *
     * C CONCEPT: EARLY RETURN
     * -----------------------
     * Using 'return;' in a void function exits immediately.
     * This is cleaner than wrapping all the drawing code in an else block.
     *
     * PATTERN:
     *   if (bad_condition) {
     *       handle_error();
     *       return;
     *   }
     *   // Normal code here (not indented inside an else)
     */
    if (rows < SETTINGS_MIN_ROWS || cols < SETTINGS_MIN_COLS) {
        ncplane_putstr_yx(phone, 2, 2, TEXT_SCREEN_TOO_SMALL);
        return;
    }

    /*
     * Draw the "Settings" header.
     *
     * NOTCURSES: ncplane_set_fg_rgb(plane, color)
     *   Sets the foreground (text) color for subsequent drawing operations.
     *   The color stays set until you change it again.
     *
     *   Color format: 0xRRGGBB (24-bit RGB)
     *     - 0xFF0000 = bright red
     *     - 0x00FF00 = bright green
     *     - 0x0000FF = bright blue
     *     - 0xFFFFFF = white
     *     - 0x000000 = black
     *
     * COL_SETTINGS_HEADER (defined in config.h) is a light cream color.
     */
    ncplane_set_fg_rgb(phone, COL_SETTINGS_HEADER);
    ncplane_set_bg_rgb(phone, COL_BG);

    /*
     * NOTCURSES: ncplane_putstr_yx(plane, row, col, string)
     *   Draws a string starting at the specified position.
     *
     *   Parameters:
     *     plane  - Where to draw
     *     row    - Y coordinate (0 = top)
     *     col    - X coordinate (0 = left)
     *     string - The text to draw
     *
     *   The function returns the number of columns advanced, which we ignore.
     *
     * SETTINGS_HEADER_ROW = 3 (from config.h)
     * SETTINGS_CONTENT_COL = 2 (from config.h)
     */
    ncplane_putstr_yx(phone, SETTINGS_HEADER_ROW, SETTINGS_CONTENT_COL, "Settings");

    /*
     * Draw the settings items.
     *
     * These are placeholder items showing the format. In a real app,
     * these would be read from configuration and be editable.
     *
     * COL_SETTINGS_TEXT is a softer color than the header for visual hierarchy.
     */
    ncplane_set_fg_rgb(phone, COL_SETTINGS_TEXT);

    /*
     * SETTINGS_FIRST_ROW = 5 (from config.h)
     * Each subsequent item is one row down (+1, +2, etc.)
     */
    ncplane_putstr_yx(phone, SETTINGS_FIRST_ROW,     SETTINGS_CONTENT_COL, "Display: Auto");
    ncplane_putstr_yx(phone, SETTINGS_FIRST_ROW + 1, SETTINGS_CONTENT_COL, "Audio: Beep");
    ncplane_putstr_yx(phone, SETTINGS_FIRST_ROW + 2, SETTINGS_CONTENT_COL, "Storage: 12 GB free");

    /*
     * EXERCISE FOR THE READER:
     * ------------------------
     * To make this screen interactive like the home screen:
     *
     * 1. Add static variables:
     *      static int selected_setting = 0;
     *      static const char *settings_labels[] = {"Display", "Audio", "Storage"};
     *
     * 2. Create an input handler:
     *      screen_id screen_settings_input(uint32_t key) {
     *          switch (key) {
     *              case NCKEY_UP: ... break;
     *              case NCKEY_DOWN: ... break;
     *              case NCKEY_ENTER: // Toggle setting
     *              ...
     *          }
     *          return SCREEN_SETTINGS;
     *      }
     *
     * 3. Declare it in ui.h:
     *      screen_id screen_settings_input(uint32_t key);
     *
     * 4. Add it to the switch in main.c:
     *      case SCREEN_SETTINGS:
     *          current_screen = screen_settings_input(key);
     *          break;
     */
}

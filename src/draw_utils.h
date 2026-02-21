#ifndef DRAW_UTILS_H
#define DRAW_UTILS_H

#include <stdint.h>
#include <notcurses/notcurses.h>

void ghost_set(struct ncplane *n, uint32_t fg);
void ghost_text(struct ncplane *n, int row, int col, uint32_t colour, const char *text);
void ghost_hline(struct ncplane *n, int row, int col, int length, const char *glyph, uint32_t colour);
void ghost_fill_rect(struct ncplane *n, int row, int col, int h, int w, char ch, uint32_t fg, uint32_t bg);
void ghost_label_value(struct ncplane *n, int row, int label_col, int value_col, const char *label, const char *value);

#endif

#ifndef DRAWING_H
#define DRAWING_H

#include <nds.h>

// Bitmap primitives only; each screen supplies its own colors and layout.
void draw_rounded_box(u16* screen, int x, int y, int width, int height,
                      int radius, u16 fill, u16 border);
void print_text(const char* text, int x, int y, u16* screen, u16 color, int scale);
void print_text_fit(const char* text, int x, int y, int max_chars,
                    u16* screen, u16 color, int scale);
void print_string_embedded(const char* text, int x, int y, u16* screen);

// Launches the signature capture interface. 
// Returns 1 if successfully saved, 0 if canceled or failed.
int show_drawing_capture(u16* top_vram, u16* bottom_vram, const char* signature_path);

#endif

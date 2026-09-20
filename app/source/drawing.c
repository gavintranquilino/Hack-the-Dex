#include "drawing.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define CANVAS_WIDTH 256
#define CANVAS_HEIGHT 192
#define WHITE (RGB15(31, 31, 31) | BIT(15))
// Clip every primitive to the physical DSi bitmap, including rounded corners.
static void rounded_fill(u16* screen, int x, int y, int w, int h, int r, u16 color) {
    if (w <= 0 || h <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 0) r = 0;
    for (int dy = 0; dy < h; dy++) {
        int py = y + dy;
        if (py < 0 || py >= 192) continue;
        for (int dx = 0; dx < w; dx++) {
            int px = x + dx;
            if (px < 0 || px >= 256) continue;
            int cx = dx < r ? r - 1 - dx : (dx >= w - r ? dx - (w - r) : 0);
            int cy = dy < r ? r - 1 - dy : (dy >= h - r ? dy - (h - r) : 0);
            if (cx * cx + cy * cy <= r * r)
                screen[py * 256 + px] = color | BIT(15);
        }
    }
}

void draw_rounded_box(u16* screen, int x, int y, int width, int height,
                      int radius, u16 fill, u16 border) {
    rounded_fill(screen, x, y, width, height, radius, border);
    rounded_fill(screen, x + 2, y + 2, width - 4, height - 4,
                 radius - 2, fill);
}

static const u16 brush_colors[] = {
    RGB15(31, 0, 0) | BIT(15),   // Red
    RGB15(0, 31, 0) | BIT(15),   // Green
    RGB15(0, 0, 31) | BIT(15),   // Blue
    BIT(15),                     // Black
    RGB15(31, 0, 31) | BIT(15),  // Purple
    RGB15(31, 16, 0) | BIT(15),  // Orange
    RGB15(31, 31, 0) | BIT(15)   // Yellow
};

static const u16 ERASER = RGB15(31, 31, 31) | BIT(15);   // White
static const char* color_names[] = {
    "RED", "GREEN", "BLUE", "BLACK", "PURPLE", "ORANGE", "YELLOW"
};
#define NUM_COLORS 7

static const int brush_sizes[] = { 1, 2, 3 }; // Small (1x), Medium (2x), Large (3x)
static const char* size_names[] = { "SMALL", "MEDIUM", "LARGE" };
#define NUM_SIZES 3

static int last_x = -1;
static int last_y = -1;

static void draw_pixel(int x, int y, u16 color, int size, u16* canvas) {
    int offset = size / 2;
    for (int dy = -offset; dy < size - offset; dy++) {
        for (int dx = -offset; dx < size - offset; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 8 && px < CANVAS_WIDTH - 8 && py >= 8 && py < CANVAS_HEIGHT - 8) {
                canvas[py * CANVAS_WIDTH + px] = color;
            }
        }
    }
}

static void draw_line(int x0, int y0, int x1, int y1, u16 color, int size, u16* canvas) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx = dx >= 0 ? 1 : -1;
    int sy = dy >= 0 ? 1 : -1;
    int err = (dx >= 0 ? dx : -dx) - (dy >= 0 ? dy : -dy);

    while (true) {
        draw_pixel(x0, y0, color, size, canvas);
        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;
        if (e2 > -(dy >= 0 ? dy : -dy)) {
            err -= (dy >= 0 ? dy : -dy);
            x0 += sx;
        }
        if (e2 < (dx >= 0 ? dx : -dx)) {
            err += (dx >= 0 ? dx : -dx);
            y0 += sy;
        }
    }
}

static void clear_canvas(u16* canvas) {
    for (int i = 0; i < CANVAS_WIDTH * CANVAS_HEIGHT; i++) {
        canvas[i] = WHITE;
    }
    last_x = -1;
    last_y = -1;
}

static void put_u16(unsigned char *p, uint16_t value) {
    p[0] = value & 0xff;
    p[1] = (value >> 8) & 0xff;
}

static void put_u32(unsigned char *p, uint32_t value) {
    p[0] = value & 0xff;
    p[1] = (value >> 8) & 0xff;
    p[2] = (value >> 16) & 0xff;
    p[3] = (value >> 24) & 0xff;
}

static bool save_bmp(const char *path, u16* canvas) {
    FILE *file = fopen(path, "wb");
    if (!file) return false;

    const uint32_t data_size = CANVAS_WIDTH * CANVAS_HEIGHT * 3;
    unsigned char header[54];
    memset(header, 0, sizeof(header));

    header[0] = 'B';
    header[1] = 'M';
    put_u32(&header[2], 54 + data_size);
    put_u32(&header[10], 54);
    put_u32(&header[14], 40);
    put_u32(&header[18], CANVAS_WIDTH);
    put_u32(&header[22], CANVAS_HEIGHT);
    put_u16(&header[26], 1);
    put_u16(&header[28], 24);
    put_u32(&header[34], data_size);

    if (fwrite(header, 1, sizeof(header), file) != sizeof(header)) {
        fclose(file);
        return false;
    }

    for (int y = CANVAS_HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            u16 p = canvas[y * CANVAS_WIDTH + x];
            unsigned char r = ((p >> 0) & 0x1f) << 3;
            unsigned char g = ((p >> 5) & 0x1f) << 3;
            unsigned char b = ((p >> 10) & 0x1f) << 3;

            fputc(b, file);
            fputc(g, file);
            fputc(r, file);
        }
    }

    bool ok = !ferror(file);
    if (fclose(file) != 0) ok = false;
    return ok;
}

static void draw_top_ui(u16* screen, int color_idx, int size_idx, int eraser_active) {
    dmaFillHalfWords(RGB15(31, 30, 25) | BIT(15), screen, CANVAS_WIDTH * CANVAS_HEIGHT * 2);
    print_text("MAKE YOUR MARK", 12, 10, screen, RGB15(4, 5, 8), 2);
    print_string_embedded("01 SCAN > 02 PHOTO > 03 DRAW", 12, 32, screen);
    draw_rounded_box(screen, 8, 47, 240, 81, 9, RGB15(31, 31, 30), RGB15(4, 5, 8));
    print_string_embedded("INK", 18, 58, screen);
    print_string_embedded(eraser_active ? "ERASER ON" : color_names[color_idx], 54, 58, screen);
    for (int i = 0; i < NUM_COLORS; i++) {
        int x = 18 + i * 32;
        draw_rounded_box(screen, x, 72, 26, 23, 6, brush_colors[i], RGB15(4, 5, 8));
        if (i == color_idx && !eraser_active) print_string_embedded("^", x + 10, 97, screen);
    }
    print_string_embedded("SIZE", 18, 114, screen);
    print_string_embedded(size_names[size_idx], 54, 114, screen);
    print_string_embedded("X ERASER", 174, 114, screen);
    print_string_embedded("LEFT/RIGHT INK   UP/DOWN SIZE", 12, 139, screen);
    draw_rounded_box(screen, 8, 156, 150, 25, 7, RGB15(4, 5, 8), RGB15(4, 5, 8));
    draw_rounded_box(screen, 8, 153, 150, 25, 7, RGB15(31, 26, 3), RGB15(4, 5, 8));
    print_string_embedded("A SAVE DRAWING", 41, 162, screen);
    draw_rounded_box(screen, 166, 153, 82, 25, 7, RGB15(31, 31, 30), RGB15(4, 5, 8));
    print_string_embedded("B CLEAR", 186, 162, screen);
    print_string_embedded("DRAW INSIDE THE FRAME BELOW", 47, 183, screen);
}

static void clear_drawing(u16* canvas, u16* screen) {
    clear_canvas(canvas);
    dmaFillHalfWords(RGB15(31, 30, 25) | BIT(15), screen, CANVAS_WIDTH * CANVAS_HEIGHT * 2);
    draw_rounded_box(screen, 4, 4, 248, 184, 8, WHITE, RGB15(4, 5, 8));
}

int show_drawing_capture(u16* top_vram, u16* bottom_vram, const char* signature_path) {
    int color_idx = 3; // Default Black
    int size_idx = 1;  // Default Medium (2x)
    int eraser_active = 0;
    
    // Artwork has its own buffer so the decorative frame never enters the BMP.
    u16* canvas = malloc(CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(u16));
    if (!canvas) return 0;
    clear_drawing(canvas, bottom_vram);
    draw_top_ui(top_vram, color_idx, size_idx, eraser_active);

    while (1) {
        swiWaitForVBlank();
        scanKeys();
        
        int keys_down = keysDown();
        int keys_held = keysHeld();

        if (keys_down & KEY_B) {
            clear_drawing(canvas, bottom_vram);
        }

        if (keys_down & KEY_X) {
            eraser_active = !eraser_active;
            draw_top_ui(top_vram, color_idx, size_idx, eraser_active);
        }

        if (keys_down & KEY_RIGHT) {
            if (eraser_active == 0) {
                color_idx = (color_idx + 1) % NUM_COLORS;
                draw_top_ui(top_vram, color_idx, size_idx, eraser_active);
            }
        }

        if (keys_down & KEY_LEFT) {
            if (eraser_active == 0) {
                color_idx = (color_idx - 1 + NUM_COLORS) % NUM_COLORS;
                draw_top_ui(top_vram, color_idx, size_idx, eraser_active);
            }
        }

        if (keys_down & KEY_UP) {
            if (size_idx < NUM_SIZES - 1) {
                size_idx++;
                draw_top_ui(top_vram, color_idx, size_idx, eraser_active);
            }
        }

        if (keys_down & KEY_DOWN) {
            if (size_idx > 0) {
                size_idx--;
                draw_top_ui(top_vram, color_idx, size_idx, eraser_active);
            }
        }

        if (keys_down & KEY_A) {
            if (save_bmp(signature_path, canvas)) {
                free(canvas);
                return 1;
            }
            draw_rounded_box(top_vram, 8, 153, 240, 25, 7, RGB15(31, 31, 30), RGB15(23, 4, 6));
            print_text("SAVE FAILED. A TO RETRY", 26, 162, top_vram, RGB15(23, 4, 6), 1);
        }

        if (keys_held & KEY_TOUCH) {
            touchPosition touch;
            touchRead(&touch);

            if (touch.px >= 8 && touch.py >= 8 && touch.px < CANVAS_WIDTH - 8 && touch.py < CANVAS_HEIGHT - 8) {

                u16 current_color;

                if (eraser_active == 0) {
                    current_color = brush_colors[color_idx];
                } else {
                    current_color = ERASER;
                }
                int current_size = brush_sizes[size_idx];

                if (last_x >= 0 && last_y >= 0) {
                    draw_line(last_x, last_y, touch.px, touch.py, current_color, current_size, canvas);
                    draw_line(last_x, last_y, touch.px, touch.py, current_color, current_size, bottom_vram);
                } else {
                    draw_pixel(touch.px, touch.py, current_color, current_size, canvas);
                    draw_pixel(touch.px, touch.py, current_color, current_size, bottom_vram);
                }

                last_x = touch.px;
                last_y = touch.py;
            } else {
                last_x = last_y = -1;
            }
        } else {
            last_x = -1;
            last_y = -1;
        }
    }
    return 0;
}
#include "drawing.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define CANVAS_WIDTH 256
#define CANVAS_HEIGHT 192
#define WHITE (RGB15(31, 31, 31) | BIT(15))
#define GB_BG_COLOR RGB15(17, 21, 1)
#define GB_TEXT_COLOR RGB15(1, 7, 1)

extern void print_string_embedded(const char* str, int x, int y, u16* offscreen);

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
static const char* eraser[] = {"OFF", "ON"};
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
            if (px >= 0 && px < CANVAS_WIDTH && py >= 0 && py < CANVAS_HEIGHT) {
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

    fclose(file);
    return true;
}

static void draw_top_ui(u16* top_vram, int color_idx, int size_idx, int eraser_active) {
    for (int i = 0; i < CANVAS_WIDTH * CANVAS_HEIGHT; i++) {
        top_vram[i] = GB_BG_COLOR | BIT(15);
    }

    const char* title_msg = "ADD YOUR DRAWING";
    int title_x = (256 - (strlen(title_msg) * 6)) / 2;
    print_string_embedded(title_msg, title_x, 12, top_vram);

    print_string_embedded("USE STYLUS TO DRAW", 12, 45, top_vram);
    print_string_embedded("A = CONFIRM", 12, 70, top_vram);
    print_string_embedded("B = CLEAR", 12, 90, top_vram);

    char size_buf[64];
    snprintf(size_buf, sizeof(size_buf), "UP/DOWN = BRUSH SIZE (%s)", size_names[size_idx]);
    print_string_embedded(size_buf, 12, 110, top_vram);
    
    char color_buf[64];
    snprintf(color_buf, sizeof(color_buf), "LEFT/RIGHT = CHANGE COLOR (%s)", color_names[color_idx]);
    print_string_embedded(color_buf, 12, 130, top_vram);

    char eraser_buf[64];
    snprintf(eraser_buf, sizeof(eraser_buf), "X = TOGGLE ERASER (%s)", eraser[eraser_active]);
    print_string_embedded(eraser_buf, 12, 150, top_vram);

    const char* dex_msg = "HACK THE DEX";
    int dex_x = 256 - (strlen(dex_msg) * 6) - 3; 
    int dex_y = 192 - 8 - 3;              
    print_string_embedded(dex_msg, dex_x, dex_y, top_vram);
}

int show_drawing_capture(u16* top_vram, u16* bottom_vram, const char* signature_path) {
    int color_idx = 3; // Default Black
    int size_idx = 1;  // Default Medium (2x)
    int eraser_active = 0;
    
    clear_canvas(bottom_vram);
    draw_top_ui(top_vram, color_idx, size_idx, eraser_active);

    while (1) {
        swiWaitForVBlank();
        scanKeys();
        
        int keys_down = keysDown();
        int keys_held = keysHeld();

        if (keys_down & KEY_B) {
            clear_canvas(bottom_vram);
        }

        if (keys_down & KEY_X) {
            eraser_active != eraser_active;
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
            size_idx = (size_idx + 1) % NUM_SIZES;
            draw_top_ui(top_vram, color_idx, size_idx, eraser_active);
        }

        if (keys_down & KEY_DOWN) {
            size_idx = (size_idx - 1 + NUM_SIZES) % NUM_SIZES;
            draw_top_ui(top_vram, color_idx, size_idx, eraser_active);
        }

        if (keys_down & KEY_A) {
            if (save_bmp(signature_path, bottom_vram)) {
                return 1;
            }
        }

        if (keys_held & KEY_TOUCH) {
            touchPosition touch;
            touchRead(&touch);

            if (touch.px > 0 && touch.py > 0 && touch.px < CANVAS_WIDTH && touch.py < CANVAS_HEIGHT) {

                u16 current_color;

                if (eraser_active == 0) {
                    current_color = brush_colors[color_idx];
                } else {
                    current_color = ERASER;
                }
                int current_size = brush_sizes[size_idx];

                if (last_x >= 0 && last_y >= 0) {
                    draw_line(last_x, last_y, touch.px, touch.py, current_color, current_size, bottom_vram);
                } else {
                    draw_pixel(touch.px, touch.py, current_color, current_size, bottom_vram);
                }

                last_x = touch.px;
                last_y = touch.py;
            }
        } else {
            last_x = -1;
            last_y = -1;
        }
    }
    return 0;
}
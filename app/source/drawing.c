#include "drawing.h"
#include "styles.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define CANVAS_WIDTH 256
#define CANVAS_HEIGHT 192
#define WHITE UI_COLOR_BACKGROUND

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
#ifdef EMU_NO_SD
    // Bypassing file I/O completely on emulator to prevent DLDI write crashes.
    return true;
#else
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
#endif
}

static void draw_top_ui(u16* screen, int color_idx, int size_idx, int eraser_active) {
    ui_begin_screen(screen);
    ui_draw_text(screen, &(UiText){ .value = "MAKE YOUR MARK", .x = 12, .y = 10, .color = UI_COLOR_HEADING, .scale = 2 });
    ui_draw_text(screen, &(UiText){ .value = "01 SCAN > 02 PHOTO > 03 DRAW", .x = 12, .y = 32, .color = UI_COLOR_INK, .scale = 1 });
    ui_draw_card(screen, &(UiCard){ .bounds = { 8, 47, 240, 81 }, .radius = 9, .surface = UI_SURFACE_CARD });
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
    ui_draw_button(screen, &(UiButton){
        .card = { .bounds = { 8, 153, 150, 25 }, .radius = 7, .surface = UI_SURFACE_SUCCESS },
        .label = { .value = "A SAVE DRAWING", .x = 41, .y = 162, .color = UI_COLOR_INK, .scale = 1 }
    });
    ui_draw_button(screen, &(UiButton){
        .card = { .bounds = { 166, 153, 82, 25 }, .radius = 7, .surface = UI_SURFACE_SELECTED },
        .label = { .value = "B CLEAR", .x = 186, .y = 162, .color = UI_COLOR_INK, .scale = 1 }
    });
    print_string_embedded("DRAW INSIDE THE FRAME BELOW", 47, 183, screen);
}

static void clear_drawing(u16* canvas, u16* screen) {
    clear_canvas(canvas);
    dmaFillHalfWords(UI_COLOR_FRAME, screen, CANVAS_WIDTH * CANVAS_HEIGHT * 2);
    draw_rounded_box(screen, 4, 4, 248, 184, 8, WHITE, UI_COLOR_INK);
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
            draw_rounded_box(top_vram, 8, 153, 240, 25, 7, RGB15(31, 31, 31), RGB15(23, 4, 6));
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
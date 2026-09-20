#include "profile_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "drawing.h"

static void draw_scaled_bmp_box(const char* path, u16* offscreen, int bx, int by, int bw, int bh) {
    draw_rounded_box(offscreen, bx - 3, by - 3, bw + 6, bh + 6, 8,
                     RGB15(31, 31, 31), RGB15(4, 5, 8));
    print_string_embedded("NO IMAGE", bx + (bw - 48) / 2, by + bh / 2, offscreen);

    if (!path || strlen(path) == 0) return;
    FILE* file = fopen(path, "rb");
    if (!file) return;

    fseek(file, 54, SEEK_SET);
    u8* file_data = (u8*)malloc(256 * 192 * 3);
    if (!file_data) {
        fclose(file);
        return;
    }
    
    if (fread(file_data, 1, 256 * 192 * 3, file) != 256 * 192 * 3) {
        fclose(file);
        free(file_data);
        return;
    }
    fclose(file);

    for (int dy = 0; dy < bh; dy++) {
        for (int dx = 0; dx < bw; dx++) {
            int cx = dx < 5 ? 4 - dx : (dx >= bw - 5 ? dx - (bw - 5) : 0);
            int cy = dy < 5 ? 4 - dy : (dy >= bh - 5 ? dy - (bh - 5) : 0);
            if (cx * cx + cy * cy > 25) continue;
            int sx = (dx * 256) / bw;
            int sy_bmp = 191 - ((dy * 192) / bh);
            
            int ptr = (sy_bmp * 256 + sx) * 3;
            u8 b = file_data[ptr];
            u8 g = file_data[ptr + 1];
            u8 r = file_data[ptr + 2];

            int px = bx + dx;
            int py = by + dy;
            if (px >= 0 && px < 256 && py >= 0 && py < 192) {
                offscreen[py * 256 + px] = RGB15(r >> 3, g >> 3, b >> 3) | BIT(15);
            }
        }
    }
    free(file_data);
}

// Full-width rows wrap long handles instead of painting over the portrait.
static void social_row(u16* screen, int y, const char* label, const char* value, u16 brand) {
    draw_rounded_box(screen, 8, y, 240, 32, 6, RGB15(31, 31, 31), RGB15(4, 5, 8));
    print_text(label, 18, y + 4, screen, brand, 1);
    if (!value[0]) {
        print_text("Not shared", 18, y + 16, screen, RGB15(12, 13, 14), 1);
        return;
    }
    char line[37];
    snprintf(line, sizeof(line), "%.36s", value);
    print_string_embedded(line, 18, y + 14, screen);
    if (strlen(value) > 36) print_string_embedded(value + 36, 18, y + 22, screen);
}

void show_profile_view(DexUser* user, u16* top_vram) {
    videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    int bg3_sub = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* bottom_vram = bgGetGfxPtr(bg3_sub);
    u16* top = malloc(256 * 192 * sizeof(u16));
    u16* bottom = malloc(256 * 192 * sizeof(u16));
    if (!top || !bottom) { free(top); free(bottom); return; }
    for (int i = 0; i < 256 * 192; i++) top[i] = bottom[i] = RGB15(31, 31, 31) | BIT(15);

    draw_rounded_box(top, 0, 0, 256, 192, 10, RGB15(31, 31, 31), RGB15(28, 14, 15));
    draw_rounded_box(bottom, 0, 0, 256, 192, 10, RGB15(31, 31, 31), RGB15(28, 14, 15));
    draw_rounded_box(top, 8, 7, 240, 27, 8, RGB15(31, 21, 21), RGB15(4, 5, 8));
    print_text("FRIEND CARD", 62, 14, top, RGB15(4, 5, 8), 2);
    if (strlen(user->name) <= 18) {
        print_text(user->name, 16, 44, top, RGB15(4, 5, 8), 2);
    } else {
        char name[37];
        snprintf(name, sizeof(name), "%.36s", user->name);
        print_string_embedded(name, 16, 43, top);
        if (strlen(user->name) > 36) print_string_embedded(user->name + 36, 16, 54, top);
    }
    print_text_fit(user->pronouns, 16, 68, 36, top, RGB15(12, 13, 14), 1);
    draw_scaled_bmp_box(user->photo_path, top, 16, 86, 104, 78);
    draw_scaled_bmp_box(user->signature_path, top, 136, 86, 104, 78);
    print_string_embedded("PORTRAIT", 44, 175, top);
    print_string_embedded("THEIR MARK", 158, 175, top);

    print_text("STAY CONNECTED", 12, 10, bottom, RGB15(21, 5, 7), 2);
    // Dark brand shades keep small text legible on the white cards.
    social_row(bottom, 32, "DISCORD", user->discord, RGB15(11, 12, 29));
    social_row(bottom, 66, "TWITTER", user->twitter, RGB15(0, 13, 21));
    social_row(bottom, 100, "INSTAGRAM", user->instagram, RGB15(23, 4, 10));
    social_row(bottom, 134, "LINKEDIN", user->linkedin, RGB15(1, 12, 22));
    draw_rounded_box(bottom, 8, 171, 240, 19, 6, RGB15(22, 27, 31), RGB15(4, 5, 8));
    print_string_embedded("B  BACK TO YOUR DEX", 74, 177, bottom);
    dmaCopy(top, top_vram, 256 * 192 * sizeof(u16));
    dmaCopy(bottom, bottom_vram, 256 * 192 * sizeof(u16));
    free(top);
    free(bottom);
    while (1) {
        swiWaitForVBlank();
        scanKeys();
        if (keysDown() & KEY_B) break;
        if (keysDown() & KEY_TOUCH) {
            touchPosition touch;
            touchRead(&touch);
            if (touch.px >= 8 && touch.px < 248 && touch.py >= 171 && touch.py < 190) break;
        }
    }
}

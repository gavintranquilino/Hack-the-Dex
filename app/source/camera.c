#include <nds.h>
#include "camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAMERA_WIDTH 256
#define CAMERA_HEIGHT 192
#define GB_BG_COLOR RGB15(17, 21, 1)
#define GB_TEXT_COLOR RGB15(1, 7, 1)

extern void print_string_embedded(const char* str, int x, int y, u16* offscreen);

static int save_picture(const char* path, const u16* vram_data) {
#ifdef EMU
    const char* save_path = "sd:/hackthedex/emulator/photo.bmp";
#else
    const char* save_path = path;
#endif

    FILE* file = fopen(save_path, "wb");
    if (!file) return 0;

    u8 header[54] = {
        0x42, 0x4D, 0x36, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x40, 0x02, 0x00
    };
    fwrite(header, 1, sizeof(header), file);

    for (int y = CAMERA_HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < CAMERA_WIDTH; x++) {
            u16 pixel = vram_data[y * CAMERA_WIDTH + x];
            fputc(((pixel >> 10) & 0x1F) << 3, file);
            fputc(((pixel >> 5) & 0x1F) << 3, file);
            fputc((pixel & 0x1F) << 3, file);
        }
    }

    fclose(file);
    return 1;
}

static void draw_controls(u16* vram, const char* first, const char* second) {
    dmaFillHalfWords(GB_BG_COLOR | BIT(15), vram, CAMERA_WIDTH * CAMERA_HEIGHT * 2);
    print_string_embedded(first, 12, 55, vram);
    print_string_embedded(second, 12, 75, vram);
}

int show_camera_capture(u16* top_vram, u16* bottom_vram, const char* photo_path) {
    videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);

    draw_controls(bottom_vram, "A = TAKE PICTURE", "X = SWITCH CAMERA");

    cameraInit();
    int active_camera = CAMERA_OUTER;
    cameraSelect(active_camera);

    int saved = 0;

    // Direct hardware loop - zero mallocs, zero memcpy bottlenecks
    while (!saved) {
        swiWaitForVBlank();
        
        // 1. NDMA streams the camera feed directly into top_vram
        cameraStartTransfer(top_vram, MCUREG_APT_SEQ_CMD_PREVIEW, 1);

        // 2. Stamp the text directly onto top_vram over the new video frame
        const char* title_msg = "TAKE A PHOTO";
        int title_len = strlen(title_msg);
        int title_x = (256 - (title_len * 6)) / 2;
        print_string_embedded(title_msg, title_x, 12, top_vram);

        const char* dex_msg = "HACK THE DEX";
        int dex_len = strlen(dex_msg);
        int dex_x = 256 - (dex_len * 6) - 3; 
        int dex_y = 192 - 8 - 3;              
        print_string_embedded(dex_msg, dex_x, dex_y, top_vram);

        scanKeys();
        int keys = keysDown();

        if (keys & KEY_X) {
            active_camera = active_camera == CAMERA_OUTER ? CAMERA_INNER : CAMERA_OUTER;
            cameraSelect(active_camera);
        }

        if (keys & KEY_A) {
            draw_controls(bottom_vram, "CONFIRM?", "A = YES   B = NO");

            // Entering this loop stops cameraStartTransfer from being called, 
            // naturally freezing the final frame on the top screen.
            while (1) {
                swiWaitForVBlank();
                scanKeys();
                int confirm_keys = keysDown();

                if (confirm_keys & KEY_A) {
                    save_picture(photo_path, top_vram);
                    saved = 1; // Breaks the outer loop to return to main.c
                    break; 
                }

                if (confirm_keys & KEY_B) {
                    draw_controls(bottom_vram, "A = TAKE PICTURE", "X = SWITCH CAMERA");
                    break; // Breaks the inner loop to resume the live feed
                }
            }
        }
    }

    cameraDeinit();
    return 1;
}
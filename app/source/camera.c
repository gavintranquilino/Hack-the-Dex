#include <nds.h>
#include "camera.h"
#include "drawing.h"
#include "styles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAMERA_WIDTH 256
#define CAMERA_HEIGHT 192
#define CAMERA_NDMA_CHANNEL 1

static int save_picture(const char* path, const u16* vram_data) {
#ifdef EMU_NO_SD
    // Bypassing file I/O completely on emulator to prevent DLDI write crashes.
    return 1; 
#else
    const char* save_path = path;
    FILE* file = fopen(save_path, "wb");
    if (!file) return 0;

    u8 header[54] = {
        0x42, 0x4D, 0x36, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x40, 0x02, 0x00
    };
    if (fwrite(header, 1, sizeof(header), file) != sizeof(header)) { fclose(file); return 0; }

    for (int y = CAMERA_HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < CAMERA_WIDTH; x++) {
            u16 pixel = vram_data[y * CAMERA_WIDTH + x];
            fputc(((pixel >> 10) & 0x1F) << 3, file);
            fputc(((pixel >> 5) & 0x1F) << 3, file);
            fputc((pixel & 0x1F) << 3, file);
        }
    }

    bool ok = !ferror(file);
    if (fclose(file) != 0) ok = false;
    return ok;
#endif
}

// Keep the live image untouched; all controls live on the touch screen.
static void draw_controls(u16* screen, bool review, bool failed, int active_camera) {
    ui_begin_screen(screen);
    ui_draw_page_header(screen, &(UiPageHeader){
        .title = { .value = review ? "LOOKING GOOD?" : "SAY HELLO!",
                   .x = 12, .y = 10, .color = UI_COLOR_HEADING, .scale = 2 },
        .hint = { .value = "01 PHOTO > 02 DRAW > 03 DETAILS",
                  .x = 12, .y = 32, .color = UI_COLOR_INK, .scale = 1 }
    });
    draw_rounded_box(screen, 8, 49, 240, 57, 9, RGB15(31, 31, 31), RGB15(4, 5, 8));
    print_string_embedded(review ? "YOUR NEW FRIEND'S PORTRAIT" : "FRAME YOUR FRIEND ON THE TOP SCREEN", 18, 61, screen);
    print_text(failed ? "SAVE FAILED. A TO RETRY" :
               (review ? "KEEP THIS SHOT OR TRY AGAIN" :
                (active_camera == CAMERA_OUTER ? "OUTER CAMERA  /  READY" : "INNER CAMERA  /  READY")),
               18, 83, screen, failed ? RGB15(23, 4, 6) : RGB15(3, 13, 10), 1);
    ui_draw_button(screen, &(UiButton){
        .card = { .bounds = { 8, 114, 240, 29 }, .radius = 8,
                  .surface = review ? UI_SURFACE_SUCCESS : UI_SURFACE_MUTED },
        .label = { .value = review ? "A  KEEP PHOTO" : "A  TAKE PHOTO",
                   .x = 50, .y = 122, .color = UI_COLOR_INK, .scale = 2 }
    });
    ui_draw_card(screen, &(UiCard){ .bounds = { 8, 152, 240, 24 }, .radius = 7, .surface = UI_SURFACE_HIGHLIGHT });
    print_string_embedded(review ? "B  RETAKE PHOTO" : "X  SWITCH CAMERA", review ? 86 : 80, 161, screen);
    print_string_embedded("MAKE A MEMORY. FILL YOUR DEX.", 44, 183, screen);
}

static void draw_camera_error(u16* screen, const char* message) {
    ui_begin_screen(screen);
    ui_draw_page_header(screen, &(UiPageHeader){
        .title = { .value = "CAMERA UNAVAILABLE", .x = 12, .y = 10,
                   .color = UI_COLOR_HEADING, .scale = 2 },
        .hint = { .value = "01 PHOTO > 02 DRAW > 03 DETAILS", .x = 12, .y = 32,
                  .color = UI_COLOR_INK, .scale = 1 }
    });
    draw_rounded_box(screen, 8, 52, 240, 74, 9, RGB15(31, 31, 31), RGB15(4, 5, 8));
    print_text_fit(message, 18, 72, 36, screen, RGB15(23, 4, 6), 1);
    print_string_embedded("B  BACK TO MENU", 75, 158, screen);
}

static int camera_input(bool review) {
    int keys = keysDown();
    if (keys & KEY_TOUCH) {
        touchPosition touch;
        touchRead(&touch);
        if (touch.px >= 8 && touch.px < 248) {
            if (touch.py >= 114 && touch.py < 143) keys |= KEY_A;
            if (touch.py >= 152 && touch.py < 176) keys |= review ? KEY_B : KEY_X;
        }
    }
    return keys;
}

int show_camera_capture(u16* top_vram, u16* bottom_vram, const char* photo_path) {
    videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);

    if (!isDSiMode()) {
        draw_camera_error(bottom_vram, "RUN THIS ROM IN DSI MODE");
        while (1) {
            swiWaitForVBlank();
            scanKeys();
            if (keysDown() & KEY_B) return 0;
        }
    }

    if (!cameraInit()) {
        draw_camera_error(bottom_vram, "DSI CAMERA INIT FAILED");
        while (1) {
            swiWaitForVBlank();
            scanKeys();
            if (keysDown() & KEY_B) return 0;
        }
    }

    int active_camera = CAMERA_OUTER;
    if (!cameraSelect(active_camera)) {
        cameraDeinit();
        draw_camera_error(bottom_vram, "CANNOT SELECT DSI CAMERA");
        while (1) {
            swiWaitForVBlank();
            scanKeys();
            if (keysDown() & KEY_B) return 0;
        }
    }
    draw_controls(bottom_vram, false, false, active_camera);

    int saved = 0;
    int transfer_pending = 0;
    int camera_failed = 0;

    while (!saved) {
        swiWaitForVBlank();

        // Only queue a preview after the previous camera/NDMA transfer ends.
        if (transfer_pending &&
            !(ndmaBusy(CAMERA_NDMA_CHANNEL) && cameraTransferActive())) {
            transfer_pending = 0;
        }
        if (!transfer_pending) {
            if (!cameraStartTransfer(top_vram, MCUREG_APT_SEQ_CMD_PREVIEW,
                                     CAMERA_NDMA_CHANNEL)) {
                camera_failed = 1;
                break;
            }
            transfer_pending = 1;
        }

        scanKeys();
        int keys = camera_input(false);

        if (keys & KEY_X) {
            REG_NDMA_CR(CAMERA_NDMA_CHANNEL) = 0;
            cameraStopTransfer();
            transfer_pending = 0;
            active_camera = active_camera == CAMERA_OUTER ? CAMERA_INNER : CAMERA_OUTER;
            if (!cameraSelect(active_camera)) {
                camera_failed = 1;
                break;
            }
            draw_controls(bottom_vram, false, false, active_camera);
        }

        if (keys & KEY_A) {
            REG_NDMA_CR(CAMERA_NDMA_CHANNEL) = 0;
            cameraStopTransfer();
            transfer_pending = 0;
            draw_controls(bottom_vram, true, false, active_camera);

            while (1) {
                swiWaitForVBlank();
                scanKeys();
                int confirm_keys = camera_input(true);

                if (confirm_keys & KEY_A) {
                    int save_success = save_picture(photo_path, top_vram);

                    if (save_success) {
                        saved = 1;
                        break;
                    }
                    draw_controls(bottom_vram, true, true, active_camera);
                }

                if (confirm_keys & KEY_B) {
                    draw_controls(bottom_vram, false, false, active_camera);
                    break; 
                }
            }
        }
    }

    REG_NDMA_CR(CAMERA_NDMA_CHANNEL) = 0;
    cameraStopTransfer();
    cameraDeinit();
    if (camera_failed) {
        draw_camera_error(bottom_vram, "CAMERA PREVIEW FAILED");
        while (1) {
            swiWaitForVBlank();
            scanKeys();
            if (keysDown() & KEY_B) return 0;
        }
    }
    return 1;
}

#include <nds.h>
#include <stdio.h>
#include <fat.h>

// ---------------------------------------------------------
// Save current camera frame as BMP
// ---------------------------------------------------------
void savePicture(u16* vramData) {

    if (!fatInitDefault()) {
        printf("Failed to access SD card!\n");

        while(1) {
            swiWaitForVBlank();
        }
        
    }

    printf("FAT Initialized successfully.\n");


    FILE* file = fopen("sd:/capture.bmp", "wb");

    if (!file) {
        printf("Failed to open capture.bmp for writing!\n");
        return;
    }

    printf("File opened successfully\n");

    // 54-byte BMP header
    // 256 x 192, 24-bit RGB
    u8 header[54] = {
        0x42, 0x4D,                         // BM
        0x36, 0x40, 0x02, 0x00,             // File size
        0x00, 0x00, 0x00, 0x00,             // Reserved
        0x36, 0x00, 0x00, 0x00,             // Pixel data offset

        0x28, 0x00, 0x00, 0x00,             // DIB header size

        0x00, 0x01, 0x00, 0x00,             // Width = 256
        0xC0, 0x00, 0x00, 0x00,             // Height = 192

        0x01, 0x00,                         // Color planes
        0x18, 0x00,                         // 24-bit

        0x00, 0x00, 0x00, 0x00,             // Compression
        0x00, 0x40, 0x02, 0x00,             // Image size

        0x00, 0x00, 0x00, 0x00,             // X pixels/meter
        0x00, 0x00, 0x00, 0x00,             // Y pixels/meter
        0x00, 0x00, 0x00, 0x00,             // Colors used
        0x00, 0x00, 0x00, 0x00              // Important colors
    };

    fwrite(header, 1, 54, file);

    // BMP is stored bottom-to-top
    for (int y = 191; y >= 0; y--) {

        for (int x = 0; x < 256; x++) {

            u16 pixel = vramData[y * 256 + x];

            u8 red   = ((pixel >> 0) & 0x1F) << 3;
            u8 green = ((pixel >> 5) & 0x1F) << 3;
            u8 blue  = ((pixel >> 10) & 0x1F) << 3;

            // BMP uses BGR order
            u8 bgr[3] = {
                blue,
                green,
                red
            };

            fwrite(bgr, 1, 3, file);
        }
    }

    fclose(file);

    printf("Picture saved to capture.bmp!\n");
}


// ---------------------------------------------------------
// Main
// ---------------------------------------------------------
int main(int argc, char* argv[]) {

    // -----------------------------------------------------
    // DSi check
    // -----------------------------------------------------
    if (!isDSiMode()) {

        consoleDemoInit();

        printf("ERROR: This app requires a DSi.\n");

        while (1) {
            swiWaitForVBlank();
        }
    }


    // -----------------------------------------------------
    // Top screen
    // -----------------------------------------------------
    videoSetMode(MODE_5_2D);

    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);

    int bg = bgInit(
        3,
        BgType_Bmp16,
        BgSize_B16_256x256,
        0,
        0
    );

    u16* cameraBuffer = bgGetGfxPtr(bg);


    // -----------------------------------------------------
    // Bottom screen
    // -----------------------------------------------------
    videoSetModeSub(MODE_0_2D);

    consoleDemoInit();


    // -----------------------------------------------------
    // Camera initialization
    // -----------------------------------------------------
    printf("DSi Camera Test\n");
    printf("----------------\n\n");

    printf("Initializing camera...\n");

    cameraInit();

    int activeCamera = CAMERA_OUTER;

    cameraSelect(activeCamera);

    printf("Camera ready.\n\n");

    printf("A = Switch Camera\n");
    printf("X = Take Picture\n");
    printf("START = Quit\n");


    // -----------------------------------------------------
    // Main loop
    // -----------------------------------------------------
    while (1) {

        // Wait for next frame
        swiWaitForVBlank();


        // -------------------------------------------------
        // Camera stream
        // -------------------------------------------------
        cameraStartTransfer(
            cameraBuffer,
            MCUREG_APT_SEQ_CMD_PREVIEW,
            1
        );


        // -------------------------------------------------
        // Read buttons
        // -------------------------------------------------
        scanKeys();

        int keys = keysDown();


        // -------------------------------------------------
        // A = switch camera
        // -------------------------------------------------
        if (keys & KEY_A) {

            if (activeCamera == CAMERA_OUTER) {
                activeCamera = CAMERA_INNER;
            }
            else {
                activeCamera = CAMERA_OUTER;
            }

            cameraSelect(activeCamera);

            printf(
                "Camera switched to: %s\n",
                (activeCamera == CAMERA_OUTER)
                    ? "OUTER"
                    : "INNER"
            );
        }


        // -------------------------------------------------
        // X = take picture
        // -------------------------------------------------
        if (keys & KEY_X) {

            printf("Taking picture...\n");

            savePicture(cameraBuffer);
        }


        // -------------------------------------------------
        // START = quit
        // -------------------------------------------------
        if (keys & KEY_START) {
            break;
        }
    }


    // -----------------------------------------------------
    // Shutdown
    // -----------------------------------------------------
    cameraDeinit();

    return 0;
}
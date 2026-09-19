#include <nds.h>
#include <fat.h>
#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------
// Configuration
// ---------------------------------------------------------
#define HOTSPOT_SSID "MyHotspotName"  // <-- CHANGE THIS TO YOUR HOTSPOT
#define TARGET_IP "192.168.4.1"       // <-- VERIFY YOUR PC'S HOTSPOT IP
#define TARGET_PORT 8080

// ---------------------------------------------------------
// Save current camera frame as BMP (SD Card Backup)
// ---------------------------------------------------------
void savePicture(u16* vramData) {
    if (!fatInitDefault()) {
        printf("Failed to access SD card!\n");
        return;
    }

    FILE* file = fopen("sd:/capture.bmp", "wb");
    if (!file) return;

    u8 header[54] = {
        0x42, 0x4D, 0x36, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00,
        0x28, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    fwrite(header, 1, 54, file);

    for (int y = 191; y >= 0; y--) {
        for (int x = 0; x < 256; x++) {
            u16 pixel = vramData[y * 256 + x];
            u8 bgr[3] = { 
                ((pixel >> 10) & 0x1F) << 3, 
                ((pixel >> 5) & 0x1F) << 3, 
                ((pixel >> 0) & 0x1F) << 3 
            };
            fwrite(bgr, 1, 3, file);
        }
    }
    fclose(file);
}

// ---------------------------------------------------------
// TCP Communication (Sends raw 8-bit grayscale array)
// ---------------------------------------------------------
void sendFrameOverTCP(const uint8_t* buffer, size_t size) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Failed to create socket.\n");
        return;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TARGET_PORT);
    inet_aton(TARGET_IP, &serv_addr.sin_addr);

    printf("Connecting to PC at %s:%d...\n", TARGET_IP, TARGET_PORT);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) {
        printf("Connected! Streaming 49KB...\n");

        size_t total_sent = 0;
        while (total_sent < size) {
            int sent = send(sock, buffer + total_sent, size - total_sent, 0);
            if (sent < 0) break;
            total_sent += sent;
        }

        if (total_sent == size) printf("Transmitted successfully!\n");
        else printf("Error during transmission.\n");
    } else {
        printf("TCP connection failed.\n");
    }

    close(sock);
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    if (!isDSiMode()) {
        consoleDemoInit();
        printf("ERROR: This app requires a DSi.\n");
        while (1) swiWaitForVBlank();
    }

    consoleDemoInit();
    printf("Initializing Wi-Fi...\n");

    if (!Wifi_InitDefault(INIT_ONLY | WIFI_ATTEMPT_DSI_MODE)) {
        printf("Wi-Fi init failed.\n");
        while (1) swiWaitForVBlank();
    }

    // -------------------------------------------------
    // Hardcoded Wi-Fi Connection
    // -------------------------------------------------
    printf("Scanning for %s...\n", HOTSPOT_SSID);
    Wifi_ScanMode();
    for (int i = 0; i < 180; i++) swiWaitForVBlank(); // Wait for scan
    
    int num_aps = Wifi_GetNumAP();
    Wifi_AccessPoint target_ap;
    bool found = false;

    for (int i = 0; i < num_aps; i++) {
        Wifi_AccessPoint ap;
        Wifi_GetAPData(i, &ap);
        
        char ssid[33] = {0};
        strncpy(ssid, ap.ssid, ap.ssid_len);
        
        if (strcmp(ssid, HOTSPOT_SSID) == 0) {
            target_ap = ap;
            found = true;
            break;
        }
    }

    if (!found) {
        printf("Could not find hotspot %s.\n", HOTSPOT_SSID);
        printf("Freezing...\n");
        while(1) swiWaitForVBlank();
    }

    printf("Found %s! Connecting...\n", HOTSPOT_SSID);
    Wifi_ConnectAP(&target_ap, WEPMODE_NONE, 0, 0); // Assuming open hotspot

    int status;
    while ((status = Wifi_AssocStatus()) == ASSOCSTATUS_SEARCHING ||
           status == ASSOCSTATUS_AUTHENTICATING ||
           status == ASSOCSTATUS_ASSOCIATING) {
        swiWaitForVBlank();
    }

    if (status != ASSOCSTATUS_ASSOCIATED) {
        printf("Failed to connect! Status: %d\n", status);
        while(1) swiWaitForVBlank();
    }

    printf("Connected successfully!\n\n");

    // -------------------------------------------------
    // Camera Setup
    // -------------------------------------------------
    videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
    int bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* cameraBuffer = bgGetGfxPtr(bg);

    cameraInit();
    int activeCamera = CAMERA_OUTER;
    cameraSelect(activeCamera);

    printf("--- Camera Ready ---\n");
    printf("A = Switch Camera\n");
    printf("X = Capture & Send\n");
    printf("START = Quit\n");

    uint8_t grayscaleBuffer[256 * 192];

    while (1) {
        swiWaitForVBlank();
        cameraStartTransfer(cameraBuffer, MCUREG_APT_SEQ_CMD_PREVIEW, 1);
        
        scanKeys();
        int keys = keysDown();

        if (keys & KEY_A) {
            activeCamera = (activeCamera == CAMERA_OUTER) ? CAMERA_INNER : CAMERA_OUTER;
            cameraSelect(activeCamera);
            printf("Switched camera.\n");
        }

        if (keys & KEY_X) {
            printf("\nCapturing...\n");
            savePicture(cameraBuffer); // Optional SD backup

            // Convert to flat 8-bit array
            for (int i = 0; i < (256 * 192); i++) {
                u16 pixel = cameraBuffer[i];
                u8 r = ((pixel >> 0) & 0x1F) << 3;
                u8 g = ((pixel >> 5) & 0x1F) << 3;
                u8 b = ((pixel >> 10) & 0x1F) << 3;
                grayscaleBuffer[i] = (r * 77 + g * 150 + b * 29) >> 8;
            }

            sendFrameOverTCP(grayscaleBuffer, sizeof(grayscaleBuffer));
            printf("Ready.\n");
        }

        if (keys & KEY_START) break;
    }

    cameraDeinit();
    Wifi_DisconnectAP();
    return 0;
}
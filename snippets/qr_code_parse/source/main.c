#include <nds.h>
#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>       
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------
// Configuration - UPDATE WITH YOUR NGROK URL & PORT
// ---------------------------------------------------------
#define TARGET_HOST "4.tcp.ngrok.io" // Remove "tcp://"
#define TARGET_PORT 15145            // Your current Ngrok port

// ---------------------------------------------------------
// TCP Communication
// ---------------------------------------------------------
void sendFrameOverTCP(const uint8_t* buffer, size_t size) {
    printf("Resolving DNS for %s...\n", TARGET_HOST);
    
    struct hostent *host = gethostbyname(TARGET_HOST);
    if (!host) {
        printf("DNS resolution failed! Check Wi-Fi.\n");
        return;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Failed to create socket.\n");
        return;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TARGET_PORT);
    
    memcpy(&serv_addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

    printf("Connecting to %s:%d...\n", TARGET_HOST, TARGET_PORT);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) {
        printf("Connected! Streaming payload...\n");

        size_t total_sent = 0;
        while (total_sent < size) {
            int sent = send(sock, buffer + total_sent, size - total_sent, 0);
            if (sent < 0) break;
            total_sent += sent;
        }

        if (total_sent == size) {
            printf("Data fired successfully!\n");
        } else {
            printf("Error during transmission.\n");
        }
    } else {
        printf("Connection refused by Ngrok.\n");
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
    
    // -------------------------------------------------
    // Auto-Connect using DSi Advanced Settings (WPA2)
    // -------------------------------------------------
    printf("Initializing Wi-Fi (DSi WPA2 Mode)...\n");

    if (!Wifi_InitDefault(INIT_ONLY | WIFI_ATTEMPT_DSI_MODE)) {
        printf("Wi-Fi init failed.\n");
        while (1) swiWaitForVBlank();
    }

    printf("Connecting to saved WPA2 network...\n");
    Wifi_AutoConnect(); 

    int status;
    while ((status = Wifi_AssocStatus()) == ASSOCSTATUS_SEARCHING ||
           status == ASSOCSTATUS_AUTHENTICATING ||
           status == ASSOCSTATUS_ASSOCIATING) {
        swiWaitForVBlank();
    }

    if (status != ASSOCSTATUS_ASSOCIATED) {
        printf("Failed to connect! Status: %d\n", status);
        printf("Check HTN credentials in Connections 4-6.\n");
        while(1) swiWaitForVBlank();
    }

    printf("Connected to HTN successfully!\n\n");

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

    printf("--- Scanner Ready ---\n");
    printf("A = Switch Camera\n");
    printf("X = Capture & Send via Ngrok\n");
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
            printf("\nProcessing Frame...\n");

            for (int i = 0; i < (256 * 192); i++) {
                u16 pixel = cameraBuffer[i];
                u8 r = ((pixel >> 0) & 0x1F) << 3;
                u8 g = ((pixel >> 5) & 0x1F) << 3;
                u8 b = ((pixel >> 10) & 0x1F) << 3;
                
                grayscaleBuffer[i] = (r * 77 + g * 150 + b * 29) >> 8;
            }

            sendFrameOverTCP(grayscaleBuffer, sizeof(grayscaleBuffer));
            printf("Ready for next scan.\n");
        }

        if (keys & KEY_START) {
            break;
        }
    }

    cameraDeinit();
    Wifi_DisconnectAP();
    return 0;
}
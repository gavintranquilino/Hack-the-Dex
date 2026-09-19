#include <nds.h>
#include <dswifi9.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// Update these with your Ngrok TCP tunnel address
#define TARGET_HOST "0.tcp.ngrok.io"
#define TARGET_PORT "12345"
#define WIFI_CONNECT_TIMEOUT_FRAMES (60 * 30)

static void wait_forever(void) {
    while (1) swiWaitForVBlank();
}

static int connect_to_wifi(void) {
    int status;
    int frames = 0;

    printf("Connecting using saved DSi settings...\n");

    do {
        status = Wifi_AssocStatus();
        swiWaitForVBlank();
        frames++;
    } while ((status == ASSOCSTATUS_SEARCHING ||
              status == ASSOCSTATUS_AUTHENTICATING ||
              status == ASSOCSTATUS_ASSOCIATING ||
              status == ASSOCSTATUS_ACQUIRINGDHCP) &&
             frames < WIFI_CONNECT_TIMEOUT_FRAMES);

    if (status != ASSOCSTATUS_ASSOCIATED) {
        printf("Wi-Fi failed: %d\n", status);
        printf("Check the DSi WFC settings.\n");
        return 0;
    }

    printf("Wi-Fi connected successfully!\n");
    return 1;
}

void sendFrameOverTCP(const uint8_t* buffer, size_t size) {
    struct addrinfo hints = {0};
    struct addrinfo *result = NULL;
    int socket_fd;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    printf("Resolving %s...\n", TARGET_HOST);
    
    // getaddrinfo handles the DNS lookup so Ngrok URLs work perfectly
    if (getaddrinfo(TARGET_HOST, TARGET_PORT, &hints, &result) != 0) {
        printf("DNS lookup failed.\n");
        return;
    }

    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd < 0) {
        printf("Could not create socket.\n");
        freeaddrinfo(result);
        return;
    }

    printf("Connecting to proxy...\n");
    if (connect(socket_fd, result->ai_addr, result->ai_addrlen) == 0) {
        printf("Connected! Sending frame...\n");
        
        size_t total_sent = 0;
        while (total_sent < size) {
            int sent = send(socket_fd, buffer + total_sent, size - total_sent, 0);
            if (sent < 0) break; // Error occurred
            total_sent += sent;
        }
        
        if (total_sent == size) {
            printf("Payload sent successfully!\n");
        } else {
            printf("Send failed mid-transfer.\n");
        }
    } else {
        printf("Connection refused.\n");
    }

    close(socket_fd);
    freeaddrinfo(result);
}

int main(int argc, char* argv[]) {
    // -----------------------------------------------------
    // Bottom Screen (Console)
    // -----------------------------------------------------
    videoSetModeSub(MODE_0_2D);
    consoleDemoInit();

    if (!isDSiMode()) {
        printf("ERROR: This app requires a DSi.\n");
        wait_forever();
    }

    // -----------------------------------------------------
    // Wi-Fi Initialization
    // -----------------------------------------------------
    printf("Initializing Wi-Fi core...\n");
    if (!Wifi_InitDefault(WFC_CONNECT | WIFI_ATTEMPT_DSI_MODE)) {
        printf("Wi-Fi init failed.\n");
        wait_forever();
    }

    if (!connect_to_wifi()) {
        wait_forever();
    }

    // -----------------------------------------------------
    // Top Screen (Camera Preview)
    // -----------------------------------------------------
    videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
    int bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* cameraBuffer = bgGetGfxPtr(bg);

    // -----------------------------------------------------
    // Camera Initialization
    // -----------------------------------------------------
    printf("\nInitializing camera...\n");
    cameraInit();
    cameraSelect(CAMERA_OUTER);
    printf("--- Scanner Ready ---\n");
    printf("X = Capture & Send Frame\n");
    printf("START = Quit\n");

    static uint8_t grayscaleBuffer[256 * 192];

    while (1) {
        swiWaitForVBlank();
        cameraStartTransfer(cameraBuffer, MCUREG_APT_SEQ_CMD_PREVIEW, 1);
        
        scanKeys();
        int keys = keysDown();

        if (keys & KEY_X) {
            printf("\nProcessing Frame...\n");
            
            // Extract Luminance (Grayscale) from 16-bit RGB frame
            for (int i = 0; i < (256 * 192); i++) {
                u16 pixel = cameraBuffer[i];
                
                u8 r = ((pixel >> 0) & 0x1F) << 3;
                u8 g = ((pixel >> 5) & 0x1F) << 3;
                u8 b = ((pixel >> 10) & 0x1F) << 3;

                // Integer math approximation for grayscale
                grayscaleBuffer[i] = (r * 77 + g * 150 + b * 29) >> 8;
            }

            // Ship the 49KB array over the network
            sendFrameOverTCP(grayscaleBuffer, sizeof(grayscaleBuffer));
            printf("Ready for next scan.\n");
        }

        if (keys & KEY_START) break;
    }

    cameraDeinit();
    Wifi_DisconnectAP();
    return 0;
}
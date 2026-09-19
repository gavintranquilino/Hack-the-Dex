#include <nds.h>
#include <dswifi9.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// Update these with your Ngrok TCP tunnel address
#define TARGET_HOST "2.tcp.ngrok.io"
#define TARGET_PORT "28248"
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

static int read_full_response(int socket_fd, char *buffer, size_t buffer_size) {
    uint32_t payload_len = 0;
    int header_read = recv(socket_fd, &payload_len, sizeof(payload_len), 0);
    if (header_read != sizeof(payload_len)) {
        printf("No JSON response header received.\n");
        return 0;
    }

    payload_len = ntohl(payload_len);
    if (payload_len >= buffer_size) {
        printf("JSON response too large for buffer: %u\n", payload_len);
        return 0;
    }

    size_t total = 0;
    while (total < payload_len) {
        int received = recv(socket_fd, buffer + total, payload_len - total, 0);
        if (received <= 0) {
            printf("Failed reading JSON response body.\n");
            return 0;
        }
        total += (size_t)received;
    }

    buffer[total] = '\0';
    printf("Server JSON: %s\n", buffer);
    return 1;
}

void sendFrameOverTCP(const uint8_t* buffer, size_t size) {
    struct addrinfo hints = {0};
    struct addrinfo *result = NULL;
    int socket_fd;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    printf("Resolving %s...\n", TARGET_HOST);
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
        printf("Connected! Streaming frame...\n");

        uint32_t network_size = htonl((uint32_t)size);
        if (send(socket_fd, &network_size, sizeof(network_size), 0) < 0) {
            printf("Failed to send frame length header.\n");
            close(socket_fd);
            freeaddrinfo(result);
            return;
        }

        size_t total_sent = 0;
        size_t chunk_size = 1024; // Send in safe 1KB blocks

        while (total_sent < size) {
            size_t remaining = size - total_sent;
            size_t to_send = (remaining < chunk_size) ? remaining : chunk_size;

            int sent = send(socket_fd, buffer + total_sent, to_send, 0);
            if (sent < 0) {
                printf("Send error at byte %u\n", (unsigned int)total_sent);
                break;
            }

            total_sent += sent;
            swiWaitForVBlank();
        }

        if (total_sent == size) {
            printf("Payload sent successfully!\n");
            char response[512];
            read_full_response(socket_fd, response, sizeof(response));
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
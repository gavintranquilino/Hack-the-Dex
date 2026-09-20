#include "network_connection.h"
#include "cJSON.h"
#include <dswifi9.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define GB_BG_COLOR   RGB15(17, 21, 1)
#define GB_TEXT_COLOR RGB15(1, 7, 1)
#define ROOT_DIR "sd:/hackthedex"

// Keep the endpoint and wire format in sync with qr_code_parse.
#define TARGET_HOST "2.tcp.ngrok.io"
#define TARGET_PORT "28248"
#define FRAME_PIXELS (256 * 192)
#define CAMERA_NDMA_CHANNEL 1
#define CAMERA_TIMEOUT_FRAMES (60 * 3)
#define WIFI_TIMEOUT_FRAMES (60 * 30)
// web/main.py can spend 40 seconds retrieving the profile.
#define NETWORK_TIMEOUT_FRAMES (60 * 60)
#define MAX_PROFILE_BYTES (16 * 1024)

#ifdef EMU
#define PROFILE_DIR ROOT_DIR "/emulator"
#endif

// Crisp 5x7 Embedded Font
static const u8 font5x7[64][8] = {
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32  
  {0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00}, // 33 !
  {0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00}, // 34 "
  {0x50,0xF8,0x50,0xF8,0x50,0x00,0x00,0x00}, // 35 #
  {0x20,0x78,0xA0,0x70,0x28,0xF0,0x20,0x00}, // 36 $
  {0xC0,0xC8,0x10,0x20,0x40,0x98,0x18,0x00}, // 37 %
  {0x40,0xA0,0x40,0xA8,0x90,0x68,0x00,0x00}, // 38 &
  {0x60,0x20,0x40,0x00,0x00,0x00,0x00,0x00}, // 39 '
  {0x10,0x20,0x40,0x40,0x40,0x20,0x10,0x00}, // 40 (
  {0x40,0x20,0x10,0x10,0x10,0x20,0x40,0x00}, // 41 )
  {0x00,0x20,0x70,0xF8,0x70,0x20,0x00,0x00}, // 42 *
  {0x00,0x20,0x20,0xF8,0x20,0x20,0x00,0x00}, // 43 +
  {0x00,0x00,0x00,0x00,0x00,0x60,0x20,0x40}, // 44 ,
  {0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00}, // 45 -
  {0x00,0x00,0x00,0x00,0x00,0x60,0x60,0x00}, // 46 .
  {0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00}, // 47 /
  {0x70,0x88,0x98,0xA8,0xC8,0x88,0x70,0x00}, // 48 0
  {0x20,0x60,0x20,0x20,0x20,0x20,0x70,0x00}, // 49 1
  {0x70,0x88,0x08,0x10,0x20,0x40,0xF8,0x00}, // 50 2
  {0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00}, // 51 3
  {0x10,0x30,0x50,0x90,0xF8,0x10,0x10,0x00}, // 52 4
  {0xF8,0x80,0xF0,0x08,0x08,0x88,0x70,0x00}, // 53 5
  {0x30,0x40,0x80,0xF0,0x88,0x88,0x70,0x00}, // 54 6
  {0xF8,0x08,0x10,0x20,0x40,0x40,0x40,0x00}, // 55 7
  {0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00}, // 56 8
  {0x70,0x88,0x88,0x78,0x08,0x10,0x60,0x00}, // 57 9
  {0x00,0x60,0x60,0x00,0x60,0x60,0x00,0x00}, // 58 :
  {0x00,0x60,0x60,0x00,0x60,0x20,0x40,0x00}, // 59 ;
  {0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00}, // 60 <
  {0x00,0x00,0xF8,0x00,0xF8,0x00,0x00,0x00}, // 61 =
  {0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00}, // 62 >
  {0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00}, // 63 ?
  {0x70,0x88,0x88,0xA8,0xB8,0x80,0x70,0x00}, // 64 @
  {0x70,0x88,0x88,0xF8,0x88,0x88,0x88,0x00}, // 65 A
  {0xF0,0x88,0x88,0xF0,0x88,0x88,0xF0,0x00}, // 66 B
  {0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00}, // 67 C
  {0xF0,0x88,0x88,0x88,0x88,0x88,0xF0,0x00}, // 68 D
  {0xF8,0x80,0x80,0xF0,0x80,0x80,0xF8,0x00}, // 69 E
  {0xF8,0x80,0x80,0xF0,0x80,0x80,0x80,0x00}, // 70 F
  {0x70,0x88,0x80,0xB8,0x88,0x88,0x70,0x00}, // 71 G
  {0x88,0x88,0x88,0xF8,0x88,0x88,0x88,0x00}, // 72 H
  {0x70,0x20,0x20,0x20,0x20,0x20,0x70,0x00}, // 73 I
  {0x08,0x08,0x08,0x08,0x88,0x88,0x70,0x00}, // 74 J
  {0x88,0x90,0xA0,0xC0,0xA0,0x90,0x88,0x00}, // 75 K
  {0x80,0x80,0x80,0x80,0x80,0x80,0xF8,0x00}, // 76 L
  {0x88,0xD8,0xA8,0x88,0x88,0x88,0x88,0x00}, // 77 M
  {0x88,0x88,0xC8,0xA8,0x98,0x88,0x88,0x00}, // 78 N
  {0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00}, // 79 O
  {0xF0,0x88,0x88,0xF0,0x80,0x80,0x80,0x00}, // 80 P
  {0x70,0x88,0x88,0x88,0xA8,0x90,0x68,0x00}, // 81 Q
  {0xF0,0x88,0x88,0xF0,0xA0,0x90,0x88,0x00}, // 82 R
  {0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00}, // 83 S
  {0xF8,0x20,0x20,0x20,0x20,0x20,0x20,0x00}, // 84 T
  {0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00}, // 85 U
  {0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00}, // 86 V
  {0x88,0x88,0x88,0xA8,0xA8,0xD8,0x88,0x00}, // 87 W
  {0x88,0x88,0x50,0x20,0x50,0x88,0x88,0x00}, // 88 X
  {0x88,0x88,0x50,0x20,0x20,0x20,0x20,0x00}, // 89 Y
  {0xF8,0x08,0x10,0x20,0x40,0x80,0xF8,0x00}, // 90 Z
  {0x60,0x40,0x40,0x40,0x40,0x40,0x60,0x00}, // 91 [
  {0x00,0x80,0x40,0x20,0x10,0x08,0x00,0x00}, // 92 
  {0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00}, // 93 ]
  {0x20,0x50,0x88,0x00,0x00,0x00,0x00,0x00}, // 94 ^
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF8}  // 95 _
};

static void draw_char_embedded(char c, int x, int y, u16* offscreen) {
    if (c < 32 || c > 95) c = 32; 
    int char_index = c - 32;

    for (int ty = 0; ty < 8; ty++) {
        u8 row = font5x7[char_index][ty];
        for (int tx = 0; tx < 5; tx++) { 
            if (row & (1 << (7 - tx))) {
                int px = x + tx;
                int py = y + ty;
                if (px >= 0 && px < 256 && py >= 0 && py < 192) {
                    offscreen[py * 256 + px] = GB_TEXT_COLOR | BIT(15);
                }
            }
        }
    }
}

static void print_string_embedded(const char* str, int x, int y, u16* offscreen) {
    if (!str) return;
    int curr_x = x;
    for (int i = 0; str[i] != '\0'; i++) {
        char c = toupper((unsigned char)str[i]);
        draw_char_embedded(c, curr_x, y, offscreen);
        curr_x += 6;
    }
}

typedef struct {
    u16 *top_vram;
    u16 *bottom_vram;
    bool camera_initialized;
    bool camera_ready;
    bool transfer_pending;
    bool transfer_is_capture;
    bool capture_requested;
    bool frame_ready;
    bool cancelled;
    int transfer_frames;
} Scanner;

static void scanner_status(Scanner *scanner, const char *message) {
    u16 *screen = scanner->bottom_vram;
    dmaFillHalfWords(GB_BG_COLOR | BIT(15), screen, FRAME_PIXELS * sizeof(u16));
    print_string_embedded("NETWORK SETUP", 88, 24, screen);
    print_string_embedded("ALIGN QR CODE ON TOP SCREEN", 50, 56, screen);
    print_string_embedded(message, 8, 88, screen);
    print_string_embedded("A: CAPTURE / RETRY", 76, 136, screen);
    print_string_embedded("B: CANCEL", 100, 160, screen);
}

static void stop_camera(Scanner *scanner) {
    if (scanner->camera_initialized) {
        // Abort DMA before returning VRAM ownership to the caller.
        REG_NDMA_CR(CAMERA_NDMA_CHANNEL) = 0;
        cameraStopTransfer();
        cameraDeinit();
    }
    scanner->camera_initialized = false;
    scanner->camera_ready = false;
    scanner->transfer_pending = false;
    scanner->transfer_is_capture = false;
    scanner->capture_requested = false;
    scanner->frame_ready = false;
}

static bool start_camera(Scanner *scanner) {
    if (!isDSiMode()) {
        scanner_status(scanner, "A DSI CAMERA IS REQUIRED");
        return false;
    }
    scanner->camera_initialized = cameraInit();
    if (!scanner->camera_initialized || !cameraSelect(CAMERA_OUTER)) {
        stop_camera(scanner);
        scanner_status(scanner, "CAMERA FAILED. A TO RETRY");
        return false;
    }
    scanner->camera_ready = true;
    scanner_status(scanner, "READY. PRESS A TO SCAN");
    return true;
}

static void update_preview(Scanner *scanner) {
    if (!scanner->camera_ready || scanner->cancelled)
        return;

    if (scanner->transfer_pending) {
        // Match the camera preview loop: NDMA completion is the frame
        // boundary, while the camera enable bit may remain set for preview.
        if (ndmaBusy(CAMERA_NDMA_CHANNEL) && cameraTransferActive()) {
            if (++scanner->transfer_frames >= CAMERA_TIMEOUT_FRAMES) {
                stop_camera(scanner);
                scanner_status(scanner, "CAMERA TIMED OUT. A TO RETRY");
            }
            return;
        }
        if (REG_CAM_CNT & CAM_CNT_TRANSFER_ERROR) {
            stop_camera(scanner);
            scanner_status(scanner, "CAMERA ERROR. A TO RETRY");
            return;
        }
        scanner->transfer_pending = false;
        if (scanner->transfer_is_capture) {
            scanner->transfer_is_capture = false;
            scanner->frame_ready = true;
        }
    }

    // Freeze only long enough to convert the completed capture to grayscale.
    if (!scanner->frame_ready) {
        if (!cameraStartTransfer(scanner->top_vram,
                                 MCUREG_APT_SEQ_CMD_PREVIEW,
                                 CAMERA_NDMA_CHANNEL)) {
            stop_camera(scanner);
            scanner_status(scanner, "CAMERA FAILED. A TO RETRY");
            return;
        }
        scanner->transfer_is_capture = scanner->capture_requested;
        scanner->capture_requested = false;
        scanner->transfer_pending = true;
        scanner->transfer_frames = 0;
    }
}

// Network waits keep servicing the preview and the cancel button. A presses
// during an outstanding request are deliberately ignored.
static bool scanner_wait(Scanner *scanner) {
    swiWaitForVBlank();
    scanKeys();
    if (keysHeld() & KEY_B)
        scanner->cancelled = true;
    update_preview(scanner);
    return !scanner->cancelled;
}

static bool connect_wifi(Scanner *scanner) {
    scanner_status(scanner, "CONNECTING TO WI-FI...");
    if (!Wifi_CheckInit() &&
        !Wifi_InitDefault(INIT_ONLY | WIFI_ATTEMPT_DSI_MODE)) {
        scanner_status(scanner, "WI-FI INIT FAILED. A TO RETRY");
        return false;
    }
    if (Wifi_AssocStatus() == ASSOCSTATUS_ASSOCIATED)
        return true;

    Wifi_AutoConnect();
    for (int frames = 0; frames < WIFI_TIMEOUT_FRAMES; frames++) {
        if (!scanner_wait(scanner))
            return false;
        int status = Wifi_AssocStatus();
        if (status == ASSOCSTATUS_ASSOCIATED)
            return true;
        if (status == ASSOCSTATUS_CANNOTCONNECT)
            break;
    }
    Wifi_DisconnectAP();
    scanner_status(scanner, "WI-FI FAILED. A TO RETRY");
    return false;
}

static bool socket_pending(void) {
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
}

static bool connect_socket(Scanner *scanner, int fd,
                           const struct addrinfo *address) {
    if (connect(fd, address->ai_addr, address->ai_addrlen) == 0)
        return true;
    if (errno != EINPROGRESS && errno != EALREADY && !socket_pending())
        return false;

    for (int frames = 0; frames < NETWORK_TIMEOUT_FRAMES; frames++) {
        if (!scanner_wait(scanner))
            return false;
        fd_set writable, errors;
        FD_ZERO(&writable);
        FD_ZERO(&errors);
        FD_SET(fd, &writable);
        FD_SET(fd, &errors);
        struct timeval timeout = {0, 0};
        int ready = select(fd + 1, NULL, &writable, &errors, &timeout);
        if (ready < 0) {
            if (errno == EINTR)
                continue;
            return false;
        }
        if (ready > 0) {
            int error = 0;
            socklen_t size = sizeof(error);
            return getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &size) == 0 &&
                   error == 0 && FD_ISSET(fd, &writable);
        }
    }
    return false;
}

// TCP can split even the four-byte length header across multiple reads/writes.
static bool transfer_exact(Scanner *scanner, int fd, void *buffer,
                           size_t length, bool sending) {
    size_t total = 0;
    int idle_frames = 0;
    while (total < length && idle_frames < NETWORK_TIMEOUT_FRAMES) {
        if (!scanner_wait(scanner))
            return false;
        size_t chunk = length - total;
        if (chunk > 1024)
            chunk = 1024;
        int count = sending ? send(fd, (u8 *)buffer + total, chunk, 0)
                            : recv(fd, (u8 *)buffer + total, chunk, 0);
        if (count > 0) {
            total += (size_t)count;
            idle_frames = 0;
        } else if (count == 0 || !socket_pending()) {
            return false;
        } else {
            idle_frames++;
        }
    }
    return total == length;
}

static bool valid_profile(const char *response, size_t length) {
    // Reject embedded NULs and trailing non-JSON data, without reserializing.
    if (memchr(response, '\0', length))
        return false;
    cJSON *profile = cJSON_ParseWithLengthOpts(response, length + 1, NULL, true);
    bool valid = cJSON_IsObject(profile) &&
                 !cJSON_GetObjectItemCaseSensitive(profile, "status");
    static const char *fields[] = {
        "name", "pronouns", "instagram", "twitter", "linkedin", "discord",
        "photo", "signature"
    };
    for (size_t i = 0; valid && i < sizeof(fields) / sizeof(fields[0]); i++) {
        valid = cJSON_IsString(cJSON_GetObjectItemCaseSensitive(profile, fields[i]));
    }
    if (valid) {
        const char *name = cJSON_GetObjectItemCaseSensitive(profile, "name")->valuestring;
        while (*name && isspace((unsigned char)*name))
            name++;
        // The server's extraction-failure fallback has an empty name.
        valid = *name != '\0';
    }
    cJSON_Delete(profile);
    return valid;
}

static bool save_profile(const char *json_path, const char *response, size_t length) {
    // Do not expose a partial profile to main.c, or truncate a previous profile.
    char temporary_path[520];
    int count = snprintf(temporary_path, sizeof(temporary_path), "%s.tmp", json_path);
    if (count < 0 || (size_t)count >= sizeof(temporary_path))
        return false;
    FILE *file = fopen(temporary_path, "wb");
    if (!file)
        return false;
    bool saved = fwrite(response, 1, length, file) == length;
    if (fclose(file) != 0)
        saved = false;
    if (saved && rename(temporary_path, json_path) == 0)
        return true;
    remove(temporary_path);
    return false;
}

static bool send_frame(Scanner *scanner, u8 *grayscale, const char *json_path) {
    if (!connect_wifi(scanner))
        return false;

    struct addrinfo hints = {0};
    struct addrinfo *address = NULL;
    int fd = -1;
    char *response = NULL;
    bool saved = false;
    const char *error = "NETWORK FAILED. A TO RETRY";
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    scanner_status(scanner, "RESOLVING SERVER...");
    // DSWiFi's resolver is synchronous; all socket I/O below is nonblocking.
    if (getaddrinfo(TARGET_HOST, TARGET_PORT, &hints, &address) != 0)
        goto done;
    if (!scanner_wait(scanner))
        goto done;
    fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (fd < 0)
        goto done;
    int nonblocking = 1;
    if (ioctl(fd, FIONBIO, &nonblocking) < 0)
        goto done;
    scanner_status(scanner, "CONNECTING TO SERVER...");
    if (!connect_socket(scanner, fd, address))
        goto done;

    scanner_status(scanner, "SENDING CAPTURE...");
    uint32_t length = htonl(FRAME_PIXELS);
    if (!transfer_exact(scanner, fd, &length, sizeof(length), true) ||
        !transfer_exact(scanner, fd, grayscale, FRAME_PIXELS, true))
        goto done;

    scanner_status(scanner, "WAITING FOR PROFILE...");
    if (!transfer_exact(scanner, fd, &length, sizeof(length), false))
        goto done;
    length = ntohl(length);
    error = "INVALID RESPONSE. A TO RETRY";
    if (length == 0 || length > MAX_PROFILE_BYTES)
        goto done;
    response = malloc(length + 1);
    if (!response) {
        error = "OUT OF MEMORY. A TO RETRY";
        goto done;
    }
    if (!transfer_exact(scanner, fd, response, length, false))
        goto done;
    response[length] = '\0';
    error = "NO VALID PROFILE. A TO RETRY";
    if (!valid_profile(response, length))
        goto done;
    if (!scanner_wait(scanner))
        goto done;
    scanner_status(scanner, "SAVING PROFILE...");
    saved = save_profile(json_path, response, length);
    error = "SAVE FAILED. A TO RETRY";

done:
    free(response);
    if (fd >= 0)
        close(fd);
    if (address)
        freeaddrinfo(address);
    if (!saved && !scanner->cancelled)
        scanner_status(scanner, error);
    return saved;
}

int show_network_connection_screen(u16* top_vram, u16* bottom_vram, const char* timestamp_str) {
    char json_path[512];
#ifdef EMU
    (void)timestamp_str;
    int path_length = snprintf(json_path, sizeof(json_path), "%s/profile.json", PROFILE_DIR);
#else
    int path_length = snprintf(json_path, sizeof(json_path), "%s/%s/profile.json",
                               ROOT_DIR, timestamp_str);
#endif
    Scanner scanner = {.top_vram = top_vram, .bottom_vram = bottom_vram};
    int result = 1;
    bool a_released = false;
    u8 *grayscale = malloc(FRAME_PIXELS);
    bool usable = grayscale && path_length >= 0 && (size_t)path_length < sizeof(json_path);

    dmaFillHalfWords(GB_BG_COLOR | BIT(15), top_vram, FRAME_PIXELS * sizeof(u16));
    if (usable)
        start_camera(&scanner);
    else
        scanner_status(&scanner, "SCANNER UNAVAILABLE. B TO CANCEL");

    while (!scanner.cancelled) {
        swiWaitForVBlank();
        scanKeys();
        int held = keysHeld();
        if (held & KEY_B) {
            scanner.cancelled = true;
            break;
        }
        if (!(held & KEY_A))
            a_released = true;
        if (usable && a_released && (keysDown() & KEY_A)) {
            a_released = false;
            if (scanner.camera_ready || start_camera(&scanner)) {
                scanner.capture_requested = true;
                scanner_status(&scanner, "CAPTURING FRAME...");
            }
        }
        update_preview(&scanner);
        if (!scanner.frame_ready)
            continue;

        // This is the reference scanner's RGB555 -> 8-bit luminance conversion.
        // No camera transfer can write top_vram until conversion is complete.
        for (int i = 0; i < FRAME_PIXELS; i++) {
            u16 pixel = top_vram[i];
            u8 r = ((pixel >> 0) & 0x1F) << 3;
            u8 g = ((pixel >> 5) & 0x1F) << 3;
            u8 b = ((pixel >> 10) & 0x1F) << 3;
            grayscale[i] = (r * 77 + g * 150 + b * 29) >> 8;
        }
        scanner.frame_ready = false;
        update_preview(&scanner);
        if (send_frame(&scanner, grayscale, json_path)) {
            result = 0;
            break;
        }
        // A held during the request cannot queue an automatic retry.
        a_released = false;
    }

    stop_camera(&scanner);
    if (Wifi_CheckInit())
        Wifi_DisconnectAP();
    free(grayscale);
    return result;
}

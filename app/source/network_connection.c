#include "network_connection.h"
#include "cJSON.h"
#include "drawing.h"
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
#include <stdarg.h>

#define ROOT_DIR "sd:/hackthedex"

// Keep the endpoint and wire format in sync with qr_code_parse.

extern int global_host_number;
extern int global_port_number;


// #define TARGET_HOST "2.tcp.ngrok.io"
// #define TARGET_PORT "28248"
#define FRAME_PIXELS (256 * 192)
#define CAMERA_NDMA_CHANNEL 1
#define CAMERA_TIMEOUT_FRAMES (60 * 3)
// Do not leave the scanner stuck on a stale DHCP/association state.
#define WIFI_TIMEOUT_FRAMES (60 * 1)
// web/main.py can spend 40 seconds retrieving the profile.
#define NETWORK_TIMEOUT_FRAMES (60 * 60)
#define MAX_PROFILE_BYTES (16 * 1024)

#ifdef EMU
#define PROFILE_DIR ROOT_DIR "/emulator"
#endif

typedef struct {
    u16 *top_vram;
    u16 *bottom_vram;
    bool busy;
    bool camera_initialized;
    bool camera_ready;
    bool transfer_pending;
    bool capture_requested;
    bool frame_ready;
    bool cancelled;
    bool select_requested;
    int transfer_frames;
    char log_lines[6][64];
    int log_count;
} Scanner;

static void draw_scanner_ui(Scanner* scanner, const char* message) {
    u16* screen = scanner->bottom_vram;
    dmaFillHalfWords(RGB15(31, 31, 31) | BIT(15), screen, FRAME_PIXELS * sizeof(u16));
    draw_rounded_box(screen, 0, 0, 256, 192, 10, RGB15(31, 31, 31), RGB15(28, 14, 15));
    print_text("FIND A FRIEND", 12, 10, screen, RGB15(21, 5, 7), 2);
    print_string_embedded("01 SCAN > 02 PHOTO > 03 DRAW", 12, 32, screen);
    draw_rounded_box(screen, 8, 48, 240, 99, 9, RGB15(31, 31, 31), RGB15(4, 5, 8));
    bool error = strstr(message, "FAILED") || strstr(message, "TIMEOUT") ||
                 strstr(message, "UNAVAILABLE") || strstr(message, "REQUIRED");
    u16 accent = error ? RGB15(23, 4, 6) : RGB15(3, 13, 10);
    print_text(error ? "! CHECK CONNECTION" : (scanner->busy ? "... LINKING" : "LINK STATUS"),
               18, 57, screen, accent, 1);
    char line[37];
    snprintf(line, sizeof(line), "%.36s", message);
    print_string_embedded(line, 18, 73, screen);
    if (strlen(message) > 36) print_string_embedded(message + 36, 18, 84, screen);
    int first = scanner->log_count > 4 ? scanner->log_count - 4 : 0;
    for (int i = first; i < scanner->log_count - 1; i++)
        print_text_fit(scanner->log_lines[i], 18, 104 + (i - first) * 12, 36,
                       screen, RGB15(12, 13, 14), 1);
    draw_rounded_box(screen, 8, 155, 150, 24, 7, RGB15(4, 5, 8), RGB15(4, 5, 8));
    draw_rounded_box(screen, 8, 152, 150, 24, 7,
                     scanner->busy ? RGB15(31, 29, 19) : RGB15(22, 27, 31), RGB15(4, 5, 8));
    print_string_embedded(scanner->busy ? "PLEASE WAIT..." : "A SCAN / RETRY", 44, 161, screen);
    draw_rounded_box(screen, 166, 152, 82, 24, 7, RGB15(31, 31, 31), RGB15(4, 5, 8));
    print_string_embedded("B BACK", 189, 161, screen);
    print_string_embedded("ALIGN QR CODE ON THE TOP SCREEN", 38, 183, screen);
}

static int scanner_keys(void) {
    int keys = keysDown();
    if (keys & KEY_TOUCH) {
        touchPosition touch;
        touchRead(&touch);
        if (touch.py >= 152 && touch.py < 176) {
            if (touch.px >= 8 && touch.px < 158) keys |= KEY_A;
            if (touch.px >= 166 && touch.px < 248) keys |= KEY_B;
        }
    }
    return keys;
}

static void scanner_status(Scanner *scanner, const char *message) {
    if (scanner->log_count < 6) {
        snprintf(scanner->log_lines[scanner->log_count],
                 sizeof(scanner->log_lines[scanner->log_count]), "%s", message);
        scanner->log_count++;
    } else {
        for (int i = 1; i < 6; i++)
            snprintf(scanner->log_lines[i - 1], sizeof(scanner->log_lines[i - 1]),
                     "%s", scanner->log_lines[i]);
        snprintf(scanner->log_lines[5], sizeof(scanner->log_lines[5]), "%s", message);
    }
    draw_scanner_ui(scanner, message);
}

static void scanner_statusf(Scanner *scanner, const char *format, ...) {
    char message[64];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    scanner_status(scanner, message);
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
    scanner_status(scanner, "CAMERA READY. PRESS A");
    return true;
}

static void restart_camera(Scanner *scanner, const char *reason) {
    stop_camera(scanner);
    scanner_status(scanner, reason);
    if (!scanner->cancelled)
        start_camera(scanner);
}

static void update_preview(Scanner *scanner) {
    if (!scanner->camera_ready || scanner->cancelled)
        return;

    if (scanner->transfer_pending) {
        if (scanner->capture_requested) {
            // The reference app captures the buffer produced by the previous
            // preview transfer. Waiting for NDMA here can strand the first
            // transfer on hardware, so stop it at the next VBlank instead.
            cameraStopTransfer();
            scanner->transfer_pending = false;
            scanner->capture_requested = false;
            scanner->frame_ready = true;
            scanner_status(scanner, "FRAME READY. SENDING");
            return;
        }

        // Match the QR app and continuously schedule preview transfers.
        if (ndmaBusy(CAMERA_NDMA_CHANNEL) && cameraTransferActive()) {
            if (++scanner->transfer_frames >= CAMERA_TIMEOUT_FRAMES) {
                restart_camera(scanner, "CAMERA TIMEOUT. RESETTING");
            }
            return;
        }
        scanner->transfer_pending = false;
    }

    // Freeze only long enough to convert the completed capture to grayscale.
    if (!scanner->frame_ready) {
        if (!cameraStartTransfer(scanner->top_vram,
                                 MCUREG_APT_SEQ_CMD_PREVIEW,
                                 CAMERA_NDMA_CHANNEL)) {
            restart_camera(scanner, "CAMERA START FAILED. RESETTING");
            return;
        }
        scanner->transfer_pending = true;
        scanner->transfer_frames = 0;
    }
}

// Network waits keep servicing the preview and the cancel button. A presses
// during an outstanding request are deliberately ignored.
static bool scanner_wait(Scanner *scanner) {
    swiWaitForVBlank();
    scanKeys();
    if (keysDown() & KEY_SELECT)
        scanner->select_requested = true;
    if ((keysHeld() | scanner_keys()) & KEY_B)
        scanner->cancelled = true;
    update_preview(scanner);
    return !scanner->cancelled && !scanner->select_requested;
}

static bool connect_wifi(Scanner *scanner) {
    scanner_status(scanner, "WI-FI TARGET SAVED SETTINGS");
    if (!Wifi_CheckInit() &&
        !Wifi_InitDefault(WFC_CONNECT | WIFI_ATTEMPT_DSI_MODE)) {
        scanner_statusf(scanner, "WI-FI INIT FAILED E=%d", errno);
        return false;
    }
    int status = Wifi_AssocStatus();
    scanner_statusf(scanner, "WI-FI STATUS %d", status);
    if (status == ASSOCSTATUS_CANNOTCONNECT)
        scanner_status(scanner, "STATUS 6. WAIT FOR WI-FI TIMEOUT");
    if (status == ASSOCSTATUS_ASSOCIATED) {
        scanner_status(scanner, "WI-FI ASSOCIATED");
        return true;
    }

    Wifi_AutoConnect();
    for (int frames = 0; frames < WIFI_TIMEOUT_FRAMES; frames++) {
        if (!scanner_wait(scanner))
            return false;
        status = Wifi_AssocStatus();
        if (status == ASSOCSTATUS_ASSOCIATED)
        {
            scanner_status(scanner, "WI-FI ASSOCIATED");
            return true;
        }
        if (status == ASSOCSTATUS_CANNOTCONNECT) {
            scanner_status(scanner, "STATUS 6. WAIT FOR WI-FI TIMEOUT");
            break;
        }
    }
    scanner_statusf(scanner, "WI-FI FAILED S=%d", status);
    return false;
}

static bool socket_pending(void) {
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
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
        } else if (count == 0) {
            scanner_statusf(scanner, "%s CLOSED AT %lu/%lu",
                            sending ? "SEND" : "RECV",
                            (unsigned long)total, (unsigned long)length);
            return false;
        } else if (!socket_pending()) {
            scanner_statusf(scanner, "%s FAILED AT %lu E=%d",
                            sending ? "SEND" : "RECV",
                            (unsigned long)total, errno);
            return false;
        } else {
            idle_frames++;
        }
    }
    if (total != length)
        scanner_statusf(scanner, "%s TIMEOUT AT %lu/%lu",
                        sending ? "SEND" : "RECV",
                        (unsigned long)total, (unsigned long)length);
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

static bool save_dummy_profile(const char *json_path, const char *timestamp_str) {
    char dummy_profile[512];
    int length = snprintf(
        dummy_profile, sizeof(dummy_profile),
        "{\n"
        "  \"name\": \"Who Knows %s\",\n"
        "  \"pronouns\": \"?\",\n"
        "  \"instagram\": \"@testuser\",\n"
        "  \"discord\": \"test\",\n"
        "  \"linkedin\": \"test\",\n"
        "  \"twitter\": \"the goat\",\n"
        "  \"photo\": \"photo.bmp\",\n"
        "  \"signature\": \"signature.bmp\"\n"
        "}\n",
        timestamp_str ? timestamp_str : "TEST");
    return length >= 0 && (size_t)length < sizeof(dummy_profile) &&
           save_profile(json_path, dummy_profile, (size_t)length);
}

static bool shutdown_wifi(Scanner *scanner) {
    if (!Wifi_CheckInit())
        return true;

    scanner_status(scanner, "STOPPING WI-FI...");
    Wifi_DisconnectAP();
    swiWaitForVBlank();
    Wifi_DisableWifi();
    swiWaitForVBlank();
    swiWaitForVBlank();
    // BlocksDS cannot deinitialize the Internet-mode lwIP/1wIP stack.
    // Leave DSWiFi initialized, but with the AP and radio disabled.
    scanner_status(scanner, "WI-FI RADIO STOPPED");
    return true;
}

static bool send_frame(Scanner *scanner, u8 *grayscale, const char *json_path) {
    // Network waits are blocking; leave the camera in a known idle state and
    // restart it after a failed request instead of timing out its old DMA.
    if (scanner->camera_ready) {
        stop_camera(scanner);
        scanner_status(scanner, "CAMERA STOPPED FOR NETWORK");
    }
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

    char target_host[64];
    char target_port[16];
    snprintf(target_host, sizeof(target_host), "%d.tcp.ngrok.io", global_host_number);
    snprintf(target_port, sizeof(target_port), "%d", global_port_number);


    scanner_statusf(scanner, "DNS %s:%s", target_host, target_port);
    if (getaddrinfo(target_host, target_port, &hints, &address) != 0) {
        scanner_statusf(scanner, "DNS FAILED E=%d", errno);
        goto done;
    }
    scanner_status(scanner, "DNS RESOLVED");
    if (!scanner_wait(scanner))
        goto done;
    fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (fd < 0) {
        scanner_statusf(scanner, "SOCKET FAILED E=%d", errno);
        goto done;
    }
    scanner_status(scanner, "TCP CONNECTING");
    if (connect(fd, address->ai_addr, address->ai_addrlen) != 0) {
        scanner_statusf(scanner, "TCP FAILED E=%d", errno);
        goto done;
    }
    scanner_status(scanner, "TCP CONNECTED");

    scanner_status(scanner, "SENDING CAPTURE...");
    uint32_t length = htonl(FRAME_PIXELS);
    if (!transfer_exact(scanner, fd, &length, sizeof(length), true))
        goto done;
    scanner_status(scanner, "HEADER SENT");
    if (!transfer_exact(scanner, fd, grayscale, FRAME_PIXELS, true))
        goto done;
    scanner_status(scanner, "FRAME SENT. WAITING");

    scanner_status(scanner, "WAITING FOR PROFILE...");
    if (!transfer_exact(scanner, fd, &length, sizeof(length), false))
        goto done;
    length = ntohl(length);
    scanner_statusf(scanner, "RESPONSE BYTES %lu", (unsigned long)length);
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
    scanner_status(scanner, "PROFILE RECEIVED");
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
    scanner.busy = true;
    bool usable = grayscale && path_length >= 0 && (size_t)path_length < sizeof(json_path);

    dmaFillHalfWords(RGB15(31, 31, 31) | BIT(15), top_vram, FRAME_PIXELS * sizeof(u16));
    if (usable) {
        bool wifi_initialized = Wifi_CheckInit();
        if (wifi_initialized) {
            scanner_status(&scanner, "RESETTING WI-FI...");
            shutdown_wifi(&scanner);
            wifi_initialized = Wifi_CheckInit();
        }
        if (!wifi_initialized) {
            if (!Wifi_InitDefault(WFC_CONNECT | WIFI_ATTEMPT_DSI_MODE))
                scanner_statusf(&scanner, "WI-FI INIT FAILED E=%d", errno);
            else
                wifi_initialized = true;
        } else {
            Wifi_EnableWifi();
            swiWaitForVBlank();
        }
        if (wifi_initialized && connect_wifi(&scanner))
            start_camera(&scanner);
    }
    if (usable && !scanner.camera_ready && !scanner.cancelled)
        scanner_status(&scanner, "WI-FI FAILED. A TO RETRY");
    if (!usable)
        scanner_status(&scanner, "SCANNER UNAVAILABLE. B TO CANCEL");

    scanner.busy = false;
    draw_scanner_ui(&scanner, scanner.log_lines[scanner.log_count - 1]);
    while (!scanner.cancelled) {
        swiWaitForVBlank();
        scanKeys();
        int held = keysHeld();
        int pressed = scanner_keys();
        if (keysDown() & KEY_SELECT) {
            scanner.select_requested = true;
            if (save_dummy_profile(json_path, timestamp_str))
                result = 0;
            break;
        }
        if ((held | pressed) & KEY_B) {
            scanner.cancelled = true;
            break;
        }
        if (!(held & KEY_A))
            a_released = true;
        if (usable && a_released && (pressed & KEY_A)) {
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
        scanner.busy = true;
        bool sent = send_frame(&scanner, grayscale, json_path);
        scanner.busy = false;
        if (sent) {
            result = 0;
            break;
        }
        if (scanner.select_requested) {
            if (save_dummy_profile(json_path, timestamp_str))
                result = 0;
            break;
        }
        if (usable && !scanner.camera_ready && !scanner.cancelled)
            start_camera(&scanner);
        // A held during the request cannot queue an automatic retry.
        a_released = false;
        if (!scanner.cancelled) draw_scanner_ui(&scanner, scanner.log_lines[scanner.log_count - 1]);
    }

    stop_camera(&scanner);
    shutdown_wifi(&scanner);
    free(grayscale);
    return result;
}

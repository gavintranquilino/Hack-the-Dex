#include <nds.h>
#include <dswifi9.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define TARGET_HOST "google.com"
#define TARGET_PORT "80"

static void wait_forever(void)
{
    while (1)
        swiWaitForVBlank();
}

static int connect_to_wifi(void)
{
    int status;

    printf("Connecting using saved DSi Wi-Fi settings...\n");
    Wifi_AutoConnect();

    do {
        status = Wifi_AssocStatus();
        swiWaitForVBlank();
    } while (status == ASSOCSTATUS_SEARCHING ||
             status == ASSOCSTATUS_AUTHENTICATING ||
             status == ASSOCSTATUS_ASSOCIATING ||
             status == ASSOCSTATUS_ACQUIRINGDHCP);

    if (status != ASSOCSTATUS_ASSOCIATED) {
        printf("Wi-Fi failed: %d\n", status);
        printf("Check the DSi WFC settings.\n");
        return 0;
    }

    printf("Wi-Fi connected.\n");
    return 1;
}

static void ping_host(void)
{
    struct addrinfo hints = {0};
    struct addrinfo *result = NULL;
    int socket_fd;
    u32 start_ticks;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    printf("Resolving %s...\n", TARGET_HOST);
    if (getaddrinfo(TARGET_HOST, TARGET_PORT, &hints, &result) != 0) {
        printf("DNS lookup failed.\n");
        return;
    }

    socket_fd = socket(result->ai_family, result->ai_socktype,
                       result->ai_protocol);
    if (socket_fd < 0) {
        printf("Could not create socket.\n");
        freeaddrinfo(result);
        return;
    }

    printf("Connecting to %s...\n", TARGET_HOST);
    start_ticks = cpuGetTiming();
    if (connect(socket_fd, result->ai_addr, result->ai_addrlen) == 0) {
        u32 elapsed_ms = timerTicks2msec(cpuGetTiming() - start_ticks);
        printf("Success: TCP connection in %lu ms.\n", elapsed_ms);
    } else {
        printf("Connection to %s failed.\n", TARGET_HOST);
    }

    close(socket_fd);
    freeaddrinfo(result);
}

int main(void)
{
    consoleDemoInit();

    if (!isDSiMode()) {
        printf("This app requires a DSi.\n");
        wait_forever();
    }

    printf("Initializing Wi-Fi...\n");
    if (!Wifi_InitDefault(INIT_ONLY | WIFI_ATTEMPT_DSI_MODE)) {
        printf("Wi-Fi init failed.\n");
        wait_forever();
    }

    if (connect_to_wifi())
        ping_host();

    printf("\nPress START to exit.\n");
    while (1) {
        scanKeys();
        if (keysDown() & KEY_START)
            break;
        swiWaitForVBlank();
    }

    Wifi_DisconnectAP();
    return 0;
}

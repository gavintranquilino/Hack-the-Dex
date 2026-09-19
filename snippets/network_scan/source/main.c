#include <nds.h>
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

#define TARGET_IP "192.168.4.1"
#define TARGET_PORT 8080

// ---------------------------------------------------------
// TCP Communication
// ---------------------------------------------------------

void sendDataOverTCP() {
    printf("Creating TCP socket...\n");

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        printf("Failed to create socket.\n");
        return;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TARGET_PORT);

    if (inet_aton(TARGET_IP, &serv_addr.sin_addr) == 0) {
        printf("Invalid target IP address.\n");
        close(sock);
        return;
    }

    printf("Connecting to %s:%d...\n",
           TARGET_IP,
           TARGET_PORT);

    if (connect(sock,
                (struct sockaddr *)&serv_addr,
                sizeof(serv_addr)) == 0) {

        const char* payload = "Hello from the DSi!\n";

        int result = send(sock,
                          payload,
                          strlen(payload),
                          0);

        if (result >= 0) {
            printf("Payload sent successfully!\n");
        } else {
            printf("Failed to send payload.\n");
        }

    } else {
        printf("TCP connection failed.\n");
    }

    close(sock);
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------

int main() {

    consoleDemoInit();

    printf("Initializing Wi-Fi (DSi mode)...\n");

    if (!Wifi_InitDefault(INIT_ONLY | WIFI_ATTEMPT_DSI_MODE)) {
        printf("Wi-Fi initialization failed.\n");
        while (1) {
            swiWaitForVBlank();
        }
    }

    Wifi_AccessPoint ap_list[32];

    int num_aps = 0;
    int selected_ap = 0;

    bool needs_redraw = true;

    while (1) {

        swiWaitForVBlank();

        scanKeys();
        int keys = keysDown();

        // -------------------------------------------------
        // Scan for networks
        // -------------------------------------------------

        if ((keys & KEY_X) || num_aps == 0) {

            printf("\x1b[2J");
            printf("Scanning for networks...\n");

            Wifi_ScanMode();

            // Wait for scan to complete.
            for (int i = 0; i < 180; i++) {
                swiWaitForVBlank();
            }

            num_aps = Wifi_GetNumAP();

            if (num_aps > 32) {
                num_aps = 32;
            }

            for (int i = 0; i < num_aps; i++) {
                Wifi_GetAPData(i, &ap_list[i]);
            }

            selected_ap = 0;
            needs_redraw = true;
        }

        // -------------------------------------------------
        // Navigation
        // -------------------------------------------------

        if (keys & KEY_UP) {

            if (selected_ap > 0) {
                selected_ap--;
            }

            needs_redraw = true;
        }

        if (keys & KEY_DOWN) {

            if (selected_ap < num_aps - 1) {
                selected_ap++;
            }

            needs_redraw = true;
        }

        // -------------------------------------------------
        // Connect to selected network
        // -------------------------------------------------

        if ((keys & KEY_A) && num_aps > 0) {

            printf("\x1b[2J");

            char ssid[33] = {0};

            strncpy(
                ssid,
                ap_list[selected_ap].ssid,
                ap_list[selected_ap].ssid_len
            );

            // Check compatibility.

            if (!(ap_list[selected_ap].flags &
                  WFLAG_APDATA_COMPATIBLE)) {

                printf("Error: %s is not a compatible network.\n",
                       ssid);

                printf("Press B to return.\n");

                while (1) {

                    scanKeys();

                    if (keysDown() & KEY_B) {
                        break;
                    }

                    swiWaitForVBlank();
                }

                needs_redraw = true;
                continue;
            }

            printf("Connecting to: %s\n", ssid);
            printf("Attempting as Open Network...\n");

            Wifi_ConnectAP(
                &ap_list[selected_ap],
                WEPMODE_NONE,
                0,
                0
            );

            int status;

            while (
                (status = Wifi_AssocStatus()) ==
                    ASSOCSTATUS_SEARCHING ||
                status == ASSOCSTATUS_AUTHENTICATING ||
                status == ASSOCSTATUS_ASSOCIATING
            ) {
                swiWaitForVBlank();
            }

            if (status == ASSOCSTATUS_ASSOCIATED) {

                printf("\nConnected!\n\n");

                sendDataOverTCP();

                printf("\nPress START to exit.\n");

                while (1) {

                    scanKeys();

                    if (keysDown() & KEY_START) {
                        break;
                    }

                    swiWaitForVBlank();
                }

                break;

            } else {

                printf("\nFailed to connect (Status: %d)\n",
                       status);

                printf("Press B to return to list.\n");

                while (1) {

                    scanKeys();

                    if (keysDown() & KEY_B) {
                        break;
                    }

                    swiWaitForVBlank();
                }

                needs_redraw = true;
            }
        }

        // -------------------------------------------------
        // Draw UI
        // -------------------------------------------------

        if (needs_redraw) {

            printf("\x1b[2J");

            printf("--- Hack the North Scanner ---\n");
            printf("X: Rescan | A: Connect\n\n");

            if (num_aps == 0) {

                printf("No networks found.\n");

            } else {

                for (int i = 0; i < num_aps; i++) {

                    char ssid[33] = {0};

                    strncpy(
                        ssid,
                        ap_list[i].ssid,
                        ap_list[i].ssid_len
                    );

                    bool is_compatible =
                        (ap_list[i].flags &
                         WFLAG_APDATA_COMPATIBLE);

                    if (i == selected_ap) {

                        printf(
                            "-> %s %s\n",
                            ssid,
                            is_compatible ? "" : "[!]"
                        );

                    } else {

                        printf(
                            "   %s %s\n",
                            ssid,
                            is_compatible ? "" : "[!]"
                        );
                    }
                }
            }

            needs_redraw = false;
        }
    }

    Wifi_DisconnectAP();

    return 0;
}
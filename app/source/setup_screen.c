#include "setup_screen.h"
#include <stdio.h>
#include <string.h>

#define GB_BG_COLOR   RGB15(17, 21, 1)
#define GB_TEXT_COLOR RGB15(1, 7, 1)

extern void print_string_embedded(const char* str, int x, int y, u16* offscreen);

static void draw_setup_ui(u16* top_vram, u16* bottom_vram, int active_mode, int cursor_pos, int host_val, int* port_digits) {
    // Clear both screens
    for (int i = 0; i < 256 * 192; i++) {
        top_vram[i] = GB_BG_COLOR | BIT(15);
        bottom_vram[i] = GB_BG_COLOR | BIT(15);
    }

    // --- TOP SCREEN ---
    const char* title = "HACK THE DEX";
    print_string_embedded(title, (256 - strlen(title)*6)/2, 20, top_vram);

    // Host Field
    print_string_embedded("HOST NUMBER:", 20, 70, top_vram);
    char host_char[2] = { '0' + host_val, '\0' };
    print_string_embedded(host_char, 110, 70, top_vram);
    
    if (active_mode == 0) {
        print_string_embedded("^", 110, 80, top_vram); // Draw cursor
    }

    // Port Field
    print_string_embedded("PORT:", 20, 110, top_vram);
    for (int i = 0; i < 5; i++) {
        char p_char[2] = { '0' + port_digits[i], '\0' };
        int px = 70 + (i * 12); // Space out the digits slightly
        print_string_embedded(p_char, px, 110, top_vram);
        
        if (active_mode == 1 && cursor_pos == i) {
            print_string_embedded("^", px, 120, top_vram); // Draw cursor
        }
    }

    // --- BOTTOM SCREEN ---
    print_string_embedded("NETWORK SETUP", 88, 12, bottom_vram);
    
    print_string_embedded("UP/DOWN    = CHANGE NUMBER", 12, 45, bottom_vram);
    print_string_embedded("LEFT/RIGHT = SELECT NUMBER", 12, 65, bottom_vram);
    
    print_string_embedded("Y = EDIT HOST NUMBER", 12, 95, bottom_vram);
    print_string_embedded("X = EDIT PORT", 12, 115, bottom_vram);
    
    print_string_embedded("A = CONFIRM", 12, 150, bottom_vram);
}

void show_setup_screen(u16* top_vram, u16* bottom_vram, int* out_host, int* out_port) {
    int active_mode = 1; // Start on Port (1 = Port, 0 = Host)
    int cursor_pos = 4;  // Start cursor on the last digit of the port
    
    int host_val = 0;
    int port_digits[5] = {1, 0, 0, 0, 0}; // Default to Port 10000

    draw_setup_ui(top_vram, bottom_vram, active_mode, cursor_pos, host_val, port_digits);

    while (1) {
        swiWaitForVBlank();
        scanKeys();
        int keys_down = keysDown();

        if (keys_down & KEY_Y) {
            active_mode = 0; // Switch to Host
            draw_setup_ui(top_vram, bottom_vram, active_mode, cursor_pos, host_val, port_digits);
        }
        if (keys_down & KEY_X) {
            active_mode = 1; // Switch to Port
            draw_setup_ui(top_vram, bottom_vram, active_mode, cursor_pos, host_val, port_digits);
        }

        if (keys_down & KEY_LEFT) {
            if (active_mode == 1) {
                cursor_pos = (cursor_pos - 1 + 5) % 5;
                draw_setup_ui(top_vram, bottom_vram, active_mode, cursor_pos, host_val, port_digits);
            }
        }
        
        if (keys_down & KEY_RIGHT) {
            if (active_mode == 1) {
                cursor_pos = (cursor_pos + 1) % 5;
                draw_setup_ui(top_vram, bottom_vram, active_mode, cursor_pos, host_val, port_digits);
            }
        }

        if (keys_down & KEY_UP) {
            if (active_mode == 0) {
                host_val = (host_val + 1) % 10;
            } else {
                port_digits[cursor_pos] = (port_digits[cursor_pos] + 1) % 10;
            }
            draw_setup_ui(top_vram, bottom_vram, active_mode, cursor_pos, host_val, port_digits);
        }
        
        if (keys_down & KEY_DOWN) {
            if (active_mode == 0) {
                host_val = (host_val - 1 + 10) % 10;
            } else {
                port_digits[cursor_pos] = (port_digits[cursor_pos] - 1 + 10) % 10;
            }
            draw_setup_ui(top_vram, bottom_vram, active_mode, cursor_pos, host_val, port_digits);
        }

        if (keys_down & KEY_A) {
            // 1. Save the host integer
            *out_host = host_val;
            
            // 2. Convert the 5 isolated digits back into a single integer
            *out_port = (port_digits[0] * 10000) + 
                        (port_digits[1] * 1000) + 
                        (port_digits[2] * 100) + 
                        (port_digits[3] * 10) + 
                        (port_digits[4] * 1);
            break;
        }
    }
}
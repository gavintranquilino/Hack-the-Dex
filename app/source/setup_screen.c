#include "setup_screen.h"
#include <stdio.h>
#include <string.h>

#include "drawing.h"

static void draw_setup_ui(u16* top, u16* bottom, int active_mode, int cursor_pos, int host_val, int* port_digits) {
    for (int i = 0; i < 256 * 192; i++) {
        top[i] = bottom[i] = RGB15(31, 30, 25) | BIT(15);
    }
    draw_rounded_box(top, 8, 11, 240, 44, 10, RGB15(4, 5, 8), RGB15(4, 5, 8));
    draw_rounded_box(top, 8, 8, 240, 44, 10, RGB15(31, 26, 3), RGB15(4, 5, 8));
    print_text("HACK THE DEX", 24, 18, top, RGB15(4, 5, 8), 2);
    print_string_embedded("YOUR NEXT FRIEND IS OUT THERE", 24, 38, top);
    draw_rounded_box(top, 8, 64, 240, 113, 10, RGB15(31, 31, 30), RGB15(4, 5, 8));
    print_text("LET'S CONNECT", 24, 79, top, RGB15(4, 5, 8), 2);
    print_string_embedded("1  SET YOUR HOST AND PORT", 24, 108, top);
    print_string_embedded("2  SCAN A FRIEND'S QR CODE", 24, 128, top);
    print_string_embedded("3  ADD A PHOTO + A DRAWING", 24, 148, top);

    print_text("LINK SETUP", 12, 10, bottom, RGB15(4, 5, 8), 2);
    print_string_embedded("X SWITCH FIELD   SELECT DEFAULTS", 12, 32, bottom);
    draw_rounded_box(bottom, 8, 48, 54, 61, 8, RGB15(31, 31, 30), RGB15(4, 5, 8));
    draw_rounded_box(bottom, 68, 48, 180, 61, 8, RGB15(31, 31, 30), RGB15(4, 5, 8));
    print_string_embedded("HOST", 23, 55, bottom);
    print_string_embedded("PORT", 80, 55, bottom);
    char digit[2] = {'0' + host_val, 0};
    draw_rounded_box(bottom, 19, 69, 32, 32, 6,
                     active_mode == 0 ? RGB15(31, 26, 3) : RGB15(29, 29, 27), RGB15(4, 5, 8));
    print_text(digit, 30, 78, bottom, RGB15(4, 5, 8), 2);
    for (int i = 0; i < 5; i++) {
        int x = 78 + i * 32;
        bool selected = active_mode == 1 && cursor_pos == i;
        draw_rounded_box(bottom, x, 69, 28, 32, 6,
                         selected ? RGB15(31, 26, 3) : RGB15(29, 29, 27), RGB15(4, 5, 8));
        digit[0] = '0' + port_digits[i];
        print_text(digit, x + 9, 78, bottom, RGB15(4, 5, 8), 2);
        if (selected) print_string_embedded("^", x + 11, 102, bottom);
    }
    if (active_mode == 0) print_string_embedded("^", 32, 102, bottom);
    print_string_embedded("UP/DOWN CHANGE   LEFT/RIGHT MOVE", 12, 119, bottom);
    draw_rounded_box(bottom, 8, 138, 240, 33, 9, RGB15(4, 5, 8), RGB15(4, 5, 8));
    draw_rounded_box(bottom, 8, 135, 240, 33, 9, RGB15(31, 26, 3), RGB15(4, 5, 8));
    print_text("A  LET'S GO!", 62, 145, bottom, RGB15(4, 5, 8), 2);
    print_string_embedded("TAP A DIGIT TO SELECT IT", 59, 180, bottom);
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

        if (keys_down & KEY_TOUCH) {
            touchPosition touch;
            touchRead(&touch);
            if (touch.py >= 69 && touch.py < 101) {
                if (touch.px >= 19 && touch.px < 51) active_mode = 0;
                if (touch.px >= 78 && touch.px < 234 && (touch.px - 78) % 32 < 28) {
                    active_mode = 1;
                    cursor_pos = (touch.px - 78) / 32;
                }
                draw_setup_ui(top_vram, bottom_vram, active_mode, cursor_pos, host_val, port_digits);
            }
            if (touch.px >= 8 && touch.px < 248 && touch.py >= 135 && touch.py < 168)
                keys_down |= KEY_A;
        }
        if (keys_down & KEY_X) {

            active_mode = !active_mode;
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

        if (keys_down & KEY_SELECT) {

            // DEFAULT VALUES
            *out_host = 2;
            *out_port = 28248;
            break;
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
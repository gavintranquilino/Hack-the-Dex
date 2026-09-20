#include "offline.h"
#include "drawing.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // for malloc/free

#define ROOT_DIR "sd:/hackthedex"
#define FIELD_COUNT 6
#define FIELD_LENGTH 64
#define KEYBOARD_ROWS 4

static const char* field_names[FIELD_COUNT] = {
    "NAME", "PRONOUNS", "DISCORD", "INSTAGRAM", "TWITTER", "LINKEDIN"
};

static const char* keyboard_lower[KEYBOARD_ROWS] = {
    "qwertyuiop",
    "asdfghjkl",
    "zxcvbnm.-/",
    "0123456789"
};

static const char* keyboard_upper[KEYBOARD_ROWS] = {
    "QWERTYUIOP",
    "ASDFGHJKL",
    "ZXCVBNM.-/",
    "0123456789"
};

static const char* keyboard_sym[KEYBOARD_ROWS] = {
    "!@#$%^&*()",
    "-+=[]{}|~`", // Now includes { } | ~ `
    ":;\"'<>,.?\\", // Now includes the backslash \ and quotes
    "0123456789"
};

static void draw_field(u16* screen, int index, int selected,
                       const char* value) {
    int y = 8 + index * 29;
    draw_rounded_box(screen, 8, y + 2, 240, 26, 6,
                     RGB15(4, 5, 8), RGB15(4, 5, 8));
    draw_rounded_box(screen, 8, y, 240, 26, 6,
                     selected ? RGB15(31, 21, 21) : RGB15(31, 31, 31),
                     RGB15(4, 5, 8));
    print_string_embedded(field_names[index], 16, y + 4, screen);
    print_text_fit(value[0] ? value : "_", 82, y + 4, 26,
                   screen, RGB15(4, 5, 8), 1);
}

static void draw_top(u16* vram, int selected, char values[FIELD_COUNT][FIELD_LENGTH]) {
    // Render to a backbuffer first to eliminate flickering
    u16* screen = malloc(256 * 192 * sizeof(u16));
    if (!screen) return;
    
    dmaFillHalfWords(RGB15(31, 31, 31) | BIT(15), screen, 256 * 192 * 2);
    draw_rounded_box(screen, 0, 0, 256, 192, 10,
                     RGB15(31, 31, 31), RGB15(28, 14, 15));
                     
    for (int i = 0; i < FIELD_COUNT; i++)
        draw_field(screen, i, i == selected, values[i]);
        
    dmaCopy(screen, vram, 256 * 192 * sizeof(u16));
    free(screen);
}

static void draw_key(u16* screen, int x, int y, int width, const char* label) {
    draw_rounded_box(screen, x + 1, y + 2, width, 26, 5,
                     RGB15(4, 5, 8), RGB15(4, 5, 8));
    draw_rounded_box(screen, x, y, width, 26, 5,
                     RGB15(31, 31, 31), RGB15(4, 5, 8));
    int text_x = x + (width - (int)strlen(label) * 6) / 2;
    print_string_embedded(label, text_x, y + 8, screen);
}

static void draw_bottom(u16* vram, int selected, int kb_mode) {
    // Render to a backbuffer first to eliminate flickering
    u16* screen = malloc(256 * 192 * sizeof(u16));
    if (!screen) return;

    dmaFillHalfWords(RGB15(31, 31, 31) | BIT(15), screen, 256 * 192 * 2);
    draw_rounded_box(screen, 0, 0, 256, 192, 10,
                     RGB15(31, 31, 31), RGB15(28, 14, 15));
    print_text("ENTER YOUR INFO", 12, 9, screen, RGB15(21, 5, 7), 2);
    
    // Adjusted spacing slightly so it doesn't clip off the edges of the 256px screen
    print_string_embedded("UP/DOWN=SELECT FIELD  A=DONE  B=BACK", 20, 31, screen);

    const char** current_rows;
    if (kb_mode == 2) current_rows = keyboard_sym;
    else if (kb_mode == 1) current_rows = keyboard_upper;
    else current_rows = keyboard_lower;

    for (int row = 0; row < KEYBOARD_ROWS; row++) {
        int count = (int)strlen(current_rows[row]);
        int width = row == 3 ? 21 : 23;
        int gap = row == 3 ? 3 : 2;
        int start_x = row == 3 ? 20 : 10;
        for (int column = 0; column < count; column++) {
            char label[2] = {current_rows[row][column], '\0'};
            draw_key(screen, start_x + column * (width + gap),
                     45 + row * 29, width, label);
        }
    }

    // New bottom row configuration with dynamic toggles
    draw_key(screen, 10, 162, 45, kb_mode == 1 ? "a-z" : "CAPS");
    draw_key(screen, 58, 162, 70, "SPACE");
    draw_key(screen, 131, 162, 45, kb_mode == 2 ? "ABC" : "SYM");
    draw_key(screen, 179, 162, 67, "DEL");
    
    dmaCopy(screen, vram, 256 * 192 * sizeof(u16));
    free(screen);
}

#define ESCAPED_LENGTH (FIELD_LENGTH * 2)

static void save_profile(const char values[FIELD_COUNT][FIELD_LENGTH],
                         const char* timestamp_str) {
    char path[512];
#ifdef EMU
    int path_length = snprintf(path, sizeof(path), "%s/emulator/profile.json", ROOT_DIR);
#else
    int path_length = snprintf(path, sizeof(path), "%s/%s/profile.json",
                               ROOT_DIR, timestamp_str ? timestamp_str : "offline");
#endif
    if (path_length < 0 || (size_t)path_length >= sizeof(path)) return;

    FILE* file = fopen(path, "wb");
    if (!file) return;

    // Sanitize strings for JSON: escape '\' and '"'
    char escaped_values[FIELD_COUNT][ESCAPED_LENGTH];
    for (int i = 0; i < FIELD_COUNT; i++) {
        const char* in = values[i];
        char* out = escaped_values[i];
        while (*in) {
            if (*in == '"' || *in == '\\') {
                *out++ = '\\'; // Add the escape character
            }
            *out++ = *in++;
        }
        *out = '\0';
    }

    fprintf(file,
            "{\n"
            "  \"name\": \"%s\",\n"
            "  \"pronouns\": \"%s\",\n"
            "  \"discord\": \"%s\",\n"
            "  \"instagram\": \"%s\",\n"
            "  \"twitter\": \"%s\",\n"
            "  \"linkedin\": \"%s\",\n"
            "  \"photo\": \"photo.bmp\",\n"
            "  \"signature\": \"signature.bmp\"\n"
            "}\n",
            escaped_values[0], escaped_values[1], escaped_values[2], 
            escaped_values[3], escaped_values[4], escaped_values[5]);
    fclose(file);
}

int show_offline_credentials_screen(u16* top_vram, u16* bottom_vram,
                                     const char* timestamp_str) {
    char values[FIELD_COUNT][FIELD_LENGTH] = {{0}};
    int selected = 0;
    int length = 0;
    int done = 0;
    int dirty = 1; // Force a draw on the first frame
    int kb_mode = 0; // 0 = Lowercase, 1 = Uppercase, 2 = Symbols

    while (!done) {
        swiWaitForVBlank();
        scanKeys();
        int keys_down = keysDown();
        int keys_repeat = keysDownRepeat();
        int touched = 0;
        int touch_x = 0;
        int touch_y = 0;

        if (keys_down & KEY_TOUCH) {
            touchPosition touch;
            touchRead(&touch);
            touched = 1;
            touch_x = touch.px;
            touch_y = touch.py;
        }

        // Hardware Button 'B' implementation (Cancel/Back)
        if (keys_down & KEY_B) {
            return 1; 
        }

        // Hardware Button 'A' implementation (Save/Done)
        if (keys_down & KEY_A) {
            save_profile(values, timestamp_str);
            return 0; 
        }

        // Cycle through options with the D-pad
        if (keys_repeat & KEY_UP) {
            selected--;
            if (selected < 0) selected = FIELD_COUNT - 1;
            length = (int)strlen(values[selected]);
            dirty = 1;
        }

        if (keys_repeat & KEY_DOWN) {
            selected++;
            if (selected >= FIELD_COUNT) selected = 0;
            length = (int)strlen(values[selected]);
            dirty = 1;
        }

        if (touched && touch_y >= 45 && touch_y < 161) {
            int row = (touch_y - 45) / 29;
            if (row >= 0 && row < KEYBOARD_ROWS) {
                int width = row == 3 ? 21 : 23;
                int gap = row == 3 ? 3 : 2;
                int start_x = row == 3 ? 20 : 10;
                int column = (touch_x - start_x) / (width + gap);
                int key_x = start_x + column * (width + gap);

                const char** current_rows;
                if (kb_mode == 2) current_rows = keyboard_sym;
                else if (kb_mode == 1) current_rows = keyboard_upper;
                else current_rows = keyboard_lower;

                if (column >= 0 && column < (int)strlen(current_rows[row]) &&
                    touch_x >= key_x && touch_x < key_x + width &&
                    length < FIELD_LENGTH - 1) {
                    values[selected][length++] = current_rows[row][column];
                    values[selected][length] = '\0';
                    dirty = 1;
                }
            }
        } else if (touched && touch_y >= 162 && touch_y < 188) {
            if (touch_x >= 10 && touch_x < 55) { // CAPS Lock / lowercase toggle
                if (kb_mode == 1) kb_mode = 0;
                else kb_mode = 1;
                dirty = 1;
            } else if (touch_x >= 58 && touch_x < 128) { // SPACE
                if (length < FIELD_LENGTH - 1) {
                    values[selected][length++] = ' ';
                    values[selected][length] = '\0';
                    dirty = 1;
                }
            } else if (touch_x >= 131 && touch_x < 176) { // SYM Toggle
                if (kb_mode == 2) kb_mode = 0; // Return to lower on toggle off
                else kb_mode = 2;
                dirty = 1;
            } else if (touch_x >= 179 && touch_x < 246) { // DEL
                if (length > 0) {
                    values[selected][--length] = '\0';
                    dirty = 1;
                }
            }
        }

        // Only redraw the screen when a change actually occurs (fixes flickering)
        if (dirty) {
            draw_top(top_vram, selected, values);
            draw_bottom(bottom_vram, selected, kb_mode);
            dirty = 0;
        }
    }
    
    return 1; 
}
#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <fat.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include "cJSON.h"
#include "profile_view.h"
#include "network_connection.h"
#include "camera.h"
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include "drawing.h"
#include "setup_screen.h"
#include "offline.h"


#define MENU_VISIBLE_ROWS 5

#define ROOT_DIR "sd:/hackthedex"

#ifdef EMU
#define PROFILE_DIR ROOT_DIR "/emulator"
#endif

#define MAX_USERS 50

DexUser users[MAX_USERS];
int num_users = 0;

// UI State
int selected_index = 0;
int scroll_offset = 0;
int prev_index = -1;

// Global Network Variables (Saved for duration of app)
int global_host_number = 0;
int global_port_number = 0;

// ---------------------------------------------------------
// Crisp 5x7 Embedded Font (ASCII 32 to 95)
// ---------------------------------------------------------
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

// 5 extra characters to complete the standard ASCII set: ` { | } ~
static const u8 special5x7[5][8] = {
    {0x40, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 96 `
    {0x30, 0x40, 0x40, 0x80, 0x40, 0x40, 0x30, 0x00}, // 123 {
    {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00}, // 124 |
    {0x60, 0x10, 0x10, 0x08, 0x10, 0x10, 0x60, 0x00}, // 125 }
    {0x00, 0x00, 0x48, 0x90, 0x00, 0x00, 0x00, 0x00}  // 126 ~
};


int delete_directory_recursive(const char* path) {
    DIR* dir = opendir(path);

    // If we can't open it, fail
    if (!dir) {
        return -1;
    }

    struct dirent* ent;

    while ((ent = readdir(dir)) != NULL) {
        // Ignore . and ..
        if (strcmp(ent->d_name, ".") == 0 ||
            strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        char full_path[512];
        snprintf(full_path, sizeof(full_path),
                 "%s/%s", path, ent->d_name);

        if (ent->d_type == DT_DIR) {
            // Recursively delete subdirectory
            if (delete_directory_recursive(full_path) != 0) {
                closedir(dir);
                return -1;
            }
        } else {
            // Delete file
            if (unlink(full_path) != 0) {
                closedir(dir);
                return -1;
            }
        }
    }

    closedir(dir);

    // Now the directory is empty
    return rmdir(path);
}

static bool ensure_directory(const char* path) {
    if (mkdir(path, 0777) == 0) {
        return true;
    }

    DIR* dir = opendir(path);
    if (!dir) {
        return false;
    }

    closedir(dir);
    return true;
}

// ---------------------------------------------------------
// Profile Parsing Logic
// ---------------------------------------------------------
void parse_profile(const char* path, DexUser* user) {
    FILE* f = fopen(path, "rb");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* data = malloc(length + 1);
    fread(data, 1, length, f);
    fclose(f);
    data[length] = '\0';

    cJSON* json = cJSON_Parse(data);
    if (json) {
        cJSON* item;

        item = cJSON_GetObjectItemCaseSensitive(json, "name");
        if (cJSON_IsString(item) && item->valuestring) 
            strncpy(user->name, item->valuestring, sizeof(user->name) - 1);

        item = cJSON_GetObjectItemCaseSensitive(json, "pronouns");
        if (cJSON_IsString(item) && item->valuestring) 
            strncpy(user->pronouns, item->valuestring, sizeof(user->pronouns) - 1);

        item = cJSON_GetObjectItemCaseSensitive(json, "discord");
        if (cJSON_IsString(item) && item->valuestring) 
            strncpy(user->discord, item->valuestring, sizeof(user->discord) - 1);

        item = cJSON_GetObjectItemCaseSensitive(json, "twitter");
        if (cJSON_IsString(item) && item->valuestring) 
            strncpy(user->twitter, item->valuestring, sizeof(user->twitter) - 1);

        item = cJSON_GetObjectItemCaseSensitive(json, "instagram");
        if (cJSON_IsString(item) && item->valuestring) 
            strncpy(user->instagram, item->valuestring, sizeof(user->instagram) - 1);

        item = cJSON_GetObjectItemCaseSensitive(json, "linkedin");
        if (cJSON_IsString(item) && item->valuestring) 
            strncpy(user->linkedin, item->valuestring, sizeof(user->linkedin) - 1);

        item = cJSON_GetObjectItemCaseSensitive(json, "photo");
        if (cJSON_IsString(item) && item->valuestring) 
            snprintf(user->photo_path, sizeof(user->photo_path), "%s/%.127s", user->dir_path, item->valuestring);

        item = cJSON_GetObjectItemCaseSensitive(json, "signature");
        if (cJSON_IsString(item) && item->valuestring) 
            snprintf(user->signature_path, sizeof(user->signature_path), "%s/%.127s", user->dir_path, item->valuestring);
        
        cJSON_Delete(json);
    }
    free(data);
}

void load_users() {
    num_users = 0;
    DIR* dir = opendir(ROOT_DIR);
    if (!dir) return;

    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL && num_users < MAX_USERS) {
        if (ent->d_type == DT_DIR && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            
            char profile_path[512];
            snprintf(profile_path, sizeof(profile_path), "%s/%.200s/profile.json", ROOT_DIR, ent->d_name);

            FILE* test = fopen(profile_path, "r");
            if (test) {
                fclose(test);
                
                memset(&users[num_users], 0, sizeof(DexUser)); 
                
                snprintf(users[num_users].dir_path, sizeof(users[num_users].dir_path), "%s/%.200s", ROOT_DIR, ent->d_name);
                strcpy(users[num_users].name, "Unknown User");
                
                parse_profile(profile_path, &users[num_users]);
                num_users++;
            }
        }
    }
    closedir(dir);
}

// ---------------------------------------------------------
// Standalone 5x7 Pixel-Perfect Text Renderer
// ---------------------------------------------------------
// Lowercase keeps names and social handles readable without changing their case.
static const u8 lowercase5x7[26][7] = {
    {0,0,0x70,0x08,0x78,0x88,0x78}, {0x80,0x80,0xB0,0xC8,0x88,0x88,0xF0},
    {0,0,0x70,0x88,0x80,0x88,0x70}, {0x08,0x08,0x68,0x98,0x88,0x88,0x78},
    {0,0,0x70,0x88,0xF8,0x80,0x70}, {0x30,0x48,0x40,0xE0,0x40,0x40,0x40},
    {0,0x78,0x88,0x88,0x78,0x08,0x70}, {0x80,0x80,0xB0,0xC8,0x88,0x88,0x88},
    {0x20,0,0x60,0x20,0x20,0x20,0x70}, {0x10,0,0x30,0x10,0x10,0x90,0x60},
    {0x80,0x80,0x90,0xA0,0xC0,0xA0,0x90}, {0x60,0x20,0x20,0x20,0x20,0x20,0x70},
    {0,0,0xD0,0xA8,0xA8,0xA8,0xA8}, {0,0,0xB0,0xC8,0x88,0x88,0x88},
    {0,0,0x70,0x88,0x88,0x88,0x70}, {0,0xF0,0x88,0x88,0xF0,0x80,0x80},
    {0,0x78,0x88,0x88,0x78,0x08,0x08}, {0,0,0xB0,0xC8,0x80,0x80,0x80},
    {0,0,0x78,0x80,0x70,0x08,0xF0}, {0x40,0x40,0xE0,0x40,0x40,0x48,0x30},
    {0,0,0x88,0x88,0x88,0x98,0x68}, {0,0,0x88,0x88,0x88,0x50,0x20},
    {0,0,0x88,0x88,0xA8,0xA8,0x50}, {0,0,0x88,0x50,0x20,0x50,0x88},
    {0,0x88,0x88,0x88,0x78,0x08,0x70}, {0,0,0xF8,0x10,0x20,0x40,0xF8}
};

void print_text(const char* text, int x, int y, u16* screen, u16 color, int scale) {
    if (!text || scale < 1 || scale > 2) return;
    for (int i = 0; text[i] && x < 256; i++, x += 6 * scale) {
        unsigned char c = (unsigned char)text[i];
        if (c < 32 || c > 126) c = '?';
        
        for (int ty = 0; ty < 8; ty++) {
            u8 row;
            if (c >= 'a' && c <= 'z') {
                row = (ty < 7) ? lowercase5x7[c - 'a'][ty] : 0;
            } else if (c <= 95) {
                row = font5x7[c - 32][ty];
            } else if (c == 96) {
                row = special5x7[0][ty]; // `
            } else if (c >= 123 && c <= 126) {
                row = special5x7[c - 122][ty]; // { | } ~
            } else {
                row = font5x7['?' - 32][ty];
            }

            for (int tx = 0; tx < 5; tx++) {
                if (!(row & (1 << (7 - tx)))) continue;
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        int px = x + tx * scale + sx, py = y + ty * scale + sy;
                        if (px >= 0 && px < 256 && py >= 0 && py < 192)
                            screen[py * 256 + px] = color | BIT(15);
                    }
                }
            }
        }
    }
}

void print_text_fit(const char* text, int x, int y, int max_chars,
                    u16* screen, u16 color, int scale) {
    if (!text || max_chars < 1) return;
    char fitted[65];
    if (max_chars > 64) max_chars = 64;
    snprintf(fitted, sizeof(fitted), "%.*s", max_chars, text);
    if ((int)strlen(text) > max_chars && max_chars >= 3)
        memcpy(fitted + max_chars - 3, "...", 3);
    print_text(fitted, x, y, screen, color, scale);
}

void print_string_embedded(const char* text, int x, int y, u16* screen) {
    print_text(text, x, y, screen, RGB15(4, 5, 8), 1);
}

// ---------------------------------------------------------
// Universal Frame & Hardware BMP Loader (Top Screen)
// ---------------------------------------------------------
void display_photo(const char* path, const char* name_str, u16* vram) {
    u16* screen = malloc(256 * 192 * sizeof(u16));
    if (!screen) return;
    for (int i = 0; i < 256 * 192; i++) screen[i] = RGB15(31, 31, 31) | BIT(15);
    draw_rounded_box(screen, 0, 0, 256, 192, 10, RGB15(31, 31, 31), RGB15(28, 14, 15));
    draw_rounded_box(screen, 8, 10, 240, 32, 9, RGB15(4, 5, 8), RGB15(4, 5, 8));
    draw_rounded_box(screen, 8, 7, 240, 32, 9, RGB15(31, 21, 21), RGB15(4, 5, 8));
    print_text("HACK THE DEX", 24, 16, screen, RGB15(4, 5, 8), 2);
    print_string_embedded("+", 226, 20, screen);
    draw_rounded_box(screen, 8, 49, 240, 132, 10, RGB15(4, 5, 8), RGB15(4, 5, 8));
    draw_rounded_box(screen, 8, 46, 240, 132, 10, RGB15(31, 31, 31), RGB15(4, 5, 8));

    if (name_str) {
        draw_rounded_box(screen, 56, 54, 144, 108, 8, RGB15(22, 27, 31), RGB15(22, 27, 31));
        print_string_embedded("NO PHOTO YET", 92, 102, screen);
        FILE* file = path && *path ? fopen(path, "rb") : NULL;
        if (file) {
            u8* data = malloc(256 * 192 * 3);
            fseek(file, 54, SEEK_SET);
            if (data && fread(data, 1, 256 * 192 * 3, file) == 256 * 192 * 3) {
                for (int y = 0; y < 108; y++) for (int x = 0; x < 144; x++) {
                    int cx = x < 7 ? 6 - x : (x >= 137 ? x - 137 : 0);
                    int cy = y < 7 ? 6 - y : (y >= 101 ? y - 101 : 0);
                    if (cx * cx + cy * cy > 49) continue;
                    int ptr = ((191 - y * 192 / 108) * 256 + x * 256 / 144) * 3;
                    screen[(54 + y) * 256 + 56 + x] =
                        RGB15(data[ptr + 2] >> 3, data[ptr + 1] >> 3, data[ptr] >> 3) | BIT(15);
                }
            }
            free(data);
            fclose(file);
        }
        int len = strlen(name_str);
        if (len > 36) len = 36;
        print_text_fit(name_str, (256 - len * 6) / 2, 166, 36, screen, RGB15(4, 5, 8), 1);
    } else {
        // A tiny original pixel spark gives the empty card a game-like focal point.
        draw_rounded_box(screen, 106, 57, 44, 36, 9, RGB15(23, 29, 23), RGB15(4, 5, 8));
        print_text("+", 122, 68, screen, RGB15(4, 5, 8), 2);
        print_text("A NEW FRIEND!", 50, 105, screen, RGB15(4, 5, 8), 2);
        print_string_embedded("SCAN. SNAP. MAKE YOUR MARK.", 50, 132, screen);
        print_string_embedded("A / TAP TO START", 80, 155, screen);
    }
    dmaCopy(screen, vram, 256 * 192 * sizeof(u16));
    free(screen);
}

void update_bottom_screen(u16* vram) {
    u16* screen = malloc(256 * 192 * sizeof(u16));
    if (!screen) return;
    for (int i = 0; i < 256 * 192; i++) screen[i] = RGB15(31, 31, 31) | BIT(15);
    draw_rounded_box(screen, 0, 0, 256, 192, 10, RGB15(31, 31, 31), RGB15(28, 14, 15));
    print_text("FRIEND DEX", 12, 9, screen, RGB15(21, 5, 7), 2);
    char count[16];
    snprintf(count, sizeof(count), "%02d FOUND", num_users);
    draw_rounded_box(screen, 178, 6, 70, 22, 7, RGB15(31, 29, 19), RGB15(4, 5, 8));
    print_string_embedded(count, 189, 14, screen);
    for (int i = 0; i < MENU_VISIBLE_ROWS; i++) {
        int idx = scroll_offset + i;
        if (idx >= num_users + 1) break;
        int y = 34 + i * 25;
        bool selected = idx == selected_index;
        draw_rounded_box(screen, 8, y + 2, 232, 22, 6, RGB15(4, 5, 8), RGB15(4, 5, 8));
        draw_rounded_box(screen, 8, y, 232, 22, 6,
                         selected ? RGB15(31, 21, 21) : RGB15(31, 31, 31), RGB15(4, 5, 8));
        print_string_embedded(selected ? ">" : (idx == 0 ? "+" : "*"), 17, y + 7, screen);
        print_text_fit(idx == 0 ? "ADD NEW FRIEND" : users[idx - 1].name,
                       32, y + 7, 31, screen, RGB15(4, 5, 8), 1);
    }
    if (num_users + 1 > MENU_VISIBLE_ROWS) {
        draw_rounded_box(screen, 244, 34, 5, 122, 2, RGB15(29, 29, 29), RGB15(29, 29, 29));
        int thumb_y = 34 + scroll_offset * 104 / (num_users + 1 - MENU_VISIBLE_ROWS);
        draw_rounded_box(screen, 244, thumb_y, 5, 18, 2, RGB15(4, 5, 8), RGB15(4, 5, 8));
    }
    print_string_embedded("UP/DOWN SELECT   A OPEN", 12, 164, screen);
    print_string_embedded("X DELETE    START EXIT", 12, 179, screen);
    dmaCopy(screen, vram, 256 * 192 * sizeof(u16));
    free(screen);
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    if (!fatInitDefault()) {
        while(1) swiWaitForVBlank();
    }

    // Clean initialization for both screens as Bitmaps (Mode 5)
    videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);

    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    int bg3_top = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* top_vram = bgGetGfxPtr(bg3_top);

    int bg3_sub = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* bottom_vram = bgGetGfxPtr(bg3_sub);

#if !defined(OFFLINE) && !defined(EMU)

    show_setup_screen(top_vram, bottom_vram, &global_host_number, &global_port_number);

#endif

    keysSetRepeat(25, 5); 
    load_users();
    int total_items = num_users + 1;

    while (1) {
        swiWaitForVBlank();
        scanKeys();
        
        int keys_repeat = keysDownRepeat();
        int keys_once = keysDown();

        if (keys_once & KEY_TOUCH) {
            touchPosition touch;
            touchRead(&touch);
            if (touch.px >= 8 && touch.px < 240 && touch.py >= 34 && touch.py < 159) {
                int row = (touch.py - 34) / 25;
                int tapped = scroll_offset + row;
                if ((touch.py - 34) % 25 < 22 && tapped < total_items) {
                    if (selected_index == tapped) keys_once |= KEY_A;
                    selected_index = tapped;
                }
            }
        }
        if (keys_repeat & KEY_UP) selected_index--;
        if (keys_repeat & KEY_DOWN) selected_index++;

        if (selected_index < 0) selected_index = total_items - 1;
        if (selected_index >= total_items) selected_index = 0;

        int max_scroll = total_items > MENU_VISIBLE_ROWS ? total_items - MENU_VISIBLE_ROWS : 0;
        if (scroll_offset > max_scroll) scroll_offset = max_scroll;
        if (selected_index < scroll_offset) scroll_offset = selected_index;
        else if (selected_index >= scroll_offset + MENU_VISIBLE_ROWS) scroll_offset = selected_index - MENU_VISIBLE_ROWS + 1;

        if (selected_index != prev_index) {
            if (selected_index > 0) {
                display_photo(users[selected_index - 1].photo_path, users[selected_index - 1].name, top_vram);
            } else {
                display_photo(NULL, NULL, top_vram); 
            }
            
            update_bottom_screen(bottom_vram);
            prev_index = selected_index;
        }

        if (keys_once & KEY_A) {
            if (selected_index == 0) {
                // 1. Generate current timestamp string (e.g., 20260919_173000)
                time_t rawtime;
                time(&rawtime);
                struct tm *info = localtime(&rawtime);
                char timestamp_str[32];
                strftime(timestamp_str, sizeof(timestamp_str), "%Y%m%d_%H%M%S", info);

                char photo_path[512];
                char signature_path[512]; // ADDED

#ifdef EMU
                // Pre-create sd:/hackthedex/emulator in the SD image. MelonDS does not
                // reliably support runtime directory creation through its SD backend.
                snprintf(photo_path, sizeof(photo_path), "%s/photo.bmp", PROFILE_DIR);
                snprintf(signature_path, sizeof(signature_path), "%s/signature.bmp", PROFILE_DIR); // ADDED
#else
                char dir_path[512];
                snprintf(dir_path, sizeof(dir_path), "%s/%s", ROOT_DIR, timestamp_str);

                if (!ensure_directory(ROOT_DIR) || !ensure_directory(dir_path)) {
                    continue;
                }
                snprintf(photo_path, sizeof(photo_path), "%s/photo.bmp", dir_path);
                snprintf(signature_path, sizeof(signature_path), "%s/signature.bmp", dir_path); // ADDED
#endif

                // 3. Launch the Network Connection Screen (UI only - JSON will be downloaded here later)
                int status;

#if defined(OFFLINE) || defined(EMU)

                status = show_offline_credentials_screen(top_vram, bottom_vram, timestamp_str);

#else

                status = show_network_connection_screen(top_vram, bottom_vram, timestamp_str);

#endif // end of offline determination


                if (status == 0) {
                    if (show_camera_capture(top_vram, bottom_vram, photo_path)) {
                        if (show_drawing_capture(top_vram, bottom_vram, signature_path)) {
                            load_users();
                            total_items = num_users + 1;
                        }
                    }
                }

                // --- RETURN RECOVERY ---
                videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
                bg3_sub = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
                bottom_vram = bgGetGfxPtr(bg3_sub);
                
                prev_index = -1;
            } else {
                // Launch the Detailed View Module
                show_profile_view(&users[selected_index - 1], top_vram);

                // --- RETURN RECOVERY ---
                // Re-assert Mode 5 bitmap on bottom screen (since show_profile_view uses it too)
                videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
                bg3_sub = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
                bottom_vram = bgGetGfxPtr(bg3_sub);
                
                // Force UI to redraw both screens
                prev_index = -1;
            }
        }

        if (keys_once & KEY_X) {
            if (selected_index > 0) {
                // Delete selected user folder
                if (delete_directory_recursive(users[selected_index - 1].dir_path) == 0) {

                    // Reload users after deletion
                    load_users();
                    total_items = num_users + 1;

                    // Keep selection valid
                    if (selected_index >= total_items) {
                        selected_index = total_items - 1;
                    }

                    if (selected_index < 0) {
                        selected_index = 0;
                    }

                    // Force the screens to redraw
                    prev_index = -1;
                }
            }
        }

        if (keys_once & KEY_START) {
            break;
        }
    }

    return 0;
}
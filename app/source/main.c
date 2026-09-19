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
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>


// GameBoy Theme Colors
#define GB_BG_COLOR   RGB15(17, 21, 1)
#define GB_TEXT_COLOR RGB15(1, 7, 1)

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
void draw_char_embedded(char c, int x, int y, u16* offscreen) {
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

void print_string_embedded(const char* str, int x, int y, u16* offscreen) {
    if (!str) return;
    int curr_x = x;
    for (int i = 0; str[i] != '\0'; i++) {
        char c = toupper((unsigned char)str[i]);
        draw_char_embedded(c, curr_x, y, offscreen);
        curr_x += 6; // 5px width + 1px spacing
    }
}

// ---------------------------------------------------------
// Universal Frame & Hardware BMP Loader (Top Screen)
// ---------------------------------------------------------
void display_photo(const char* path, const char* name_str, u16* vram) {
    u16* offscreen = (u16*)calloc(256 * 192, sizeof(u16));
    if (!offscreen) return;

    // Fill background
    for(int i = 0; i < 256*192; i++) offscreen[i] = GB_BG_COLOR | BIT(15);

    // 1. Draw Border
    for (int y = 23; y <= 168; y++) {
        for (int x = 15; x <= 240; x++) {
            if (y == 23 || y == 168 || x == 15 || x == 240) {
                offscreen[y * 256 + x] = GB_TEXT_COLOR | BIT(15); 
            }
        }
    }

    // 2. Load Photo
    if (path && strlen(path) > 0) {
        FILE* file = fopen(path, "rb");
        if (file) {
            fseek(file, 54, SEEK_SET); 
            u8* file_data = (u8*)malloc(256 * 192 * 3);
            if (file_data) {
                fread(file_data, 1, 256 * 192 * 3, file);
                fclose(file);
                
                int ptr = 0;
                for (int y = 191; y >= 0; y--) {
                    for (int x = 0; x < 256; x++) {
                        u8 b = file_data[ptr++];
                        u8 g = file_data[ptr++];
                        u8 r = file_data[ptr++];
                        
                        if (y >= 24 && y < 168 && x >= 16 && x < 240) {
                            offscreen[y * 256 + x] = RGB15(r >> 3, g >> 3, b >> 3) | BIT(15);
                        }
                    }
                }
                free(file_data);
            } else {
                fclose(file);
            }
        }
    }

    // 3. Draw Dynamic Name Text
    if (name_str) {
        char upper_name[64];
        int len = strlen(name_str);
        for(int i = 0; i < len; i++) upper_name[i] = toupper((unsigned char)name_str[i]);
        upper_name[len] = '\0';

        int text_width = len * 6;
        int start_x = (256 - text_width) / 2;
        if (start_x < 0) start_x = 0;

        print_string_embedded(upper_name, start_x, 12, offscreen);
    } else {
        const char* msg = "READY TO ADD NEW USER";
        int len = strlen(msg);
        int text_width = len * 6; 
        int start_x = (256 - text_width) / 2;
        
        print_string_embedded(msg, start_x, 12, offscreen);
    }

    // 4. Draw "HACK THE DEX"
    const char* dex_msg = "HACK THE DEX";
    int dex_len = strlen(dex_msg);
    int dex_x = 256 - (dex_len * 6) - 3; 
    int dex_y = 192 - 8 - 3;              
    
    print_string_embedded(dex_msg, dex_x, dex_y, offscreen);

    dmaCopy(offscreen, vram, 256 * 192 * 2);
    free(offscreen);
}

// ---------------------------------------------------------
// Bottom Screen Menu Renderer (Bitmap Mode)
// ---------------------------------------------------------
void update_bottom_screen(u16* vram) {
    u16* offscreen = (u16*)malloc(256 * 192 * 2);
    if (!offscreen) return;

    for (int i = 0; i < 256 * 192; i++) {
        offscreen[i] = GB_BG_COLOR | BIT(15);
    }

    int total_items = num_users + 1;
    int max_visible = 14; 
    int start_y = 12;
    int line_height = 12;

    for (int i = 0; i < max_visible; i++) {
        int item_idx = scroll_offset + i;
        if (item_idx >= total_items) break;

        char line_buf[64];
        const char* name_target = (item_idx == 0) ? "ADD NEW USER" : users[item_idx - 1].name;

        if (item_idx == selected_index) {
            snprintf(line_buf, sizeof(line_buf), "-> %s", name_target);
        } else {
            snprintf(line_buf, sizeof(line_buf), "   %s", name_target);
        }

        print_string_embedded(line_buf, 16, start_y + (i * line_height), offscreen);
    }

    dmaCopy(offscreen, vram, 256 * 192 * 2);
    free(offscreen);
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

    keysSetRepeat(25, 5); 
    load_users();
    int total_items = num_users + 1;

    while (1) {
        swiWaitForVBlank();
        scanKeys();
        
        int keys_repeat = keysDownRepeat();
        int keys_once = keysDown();

        if (keys_repeat & KEY_UP) selected_index--;
        if (keys_repeat & KEY_DOWN) selected_index++;

        if (selected_index < 0) selected_index = total_items - 1;
        if (selected_index >= total_items) selected_index = 0;

        if (selected_index < scroll_offset) scroll_offset = selected_index;
        else if (selected_index >= scroll_offset + 14) scroll_offset = selected_index - 14 + 1;

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

#ifdef EMU
                // Pre-create sd:/hackthedex/emulator in the SD image. MelonDS does not
                // reliably support runtime directory creation through its SD backend.
#else
                char dir_path[512];
                snprintf(dir_path, sizeof(dir_path), "%s/%s", ROOT_DIR, timestamp_str);

                if (!ensure_directory(ROOT_DIR) || !ensure_directory(dir_path)) {
                    continue;
                }
#endif

                // 3. Launch the Network Connection Screen (UI only - JSON will be downloaded here later)
                show_network_connection_screen(top_vram, bottom_vram, timestamp_str);

                // 4. Reload users so the folder appears on the home screen
                load_users();
                total_items = num_users + 1;

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
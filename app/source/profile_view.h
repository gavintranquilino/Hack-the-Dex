#ifndef PROFILE_VIEW_H
#define PROFILE_VIEW_H

#include <nds.h>

// Shared GameBoy Theme Colors
#define GB_BG_COLOR   RGB15(17, 21, 1)
#define GB_TEXT_COLOR RGB15(1, 7, 1)

typedef struct {
    char dir_path[256];
    char name[64];
    char email[64];
    char instagram[64];
    char discord[64];
    char linkedin[64];
    char photo_path[256];
    char signature_path[256];
} DexUser;

// Launches the blocking detailed view loop
void show_profile_view(DexUser* user, u16* top_vram);

#endif
#ifndef PROFILE_VIEW_H
#define PROFILE_VIEW_H

#include <nds.h>

typedef struct {
    char name[64];
    char pronouns[32];
    char instagram[64];
    char twitter[64];
    char linkedin[64];
    char discord[64];
    char photo_path[512];
    char signature_path[512];
    char dir_path[512];
} DexUser;

// Launches the blocking detailed view loop
void show_profile_view(DexUser* user, u16* top_vram);

#endif
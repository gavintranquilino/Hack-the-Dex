#ifndef OFFLINE_H
#define OFFLINE_H

#include <nds.h>
#include "profile_view.h"

int show_offline_credentials_screen(u16* top_vram, u16* bottom_vram,
                                     const char* timestamp_str);

/* Returns 0 after saving, 1 if cancelled. */
int show_edit_socials_screen(u16* top_vram, u16* bottom_vram, DexUser* user);

#endif

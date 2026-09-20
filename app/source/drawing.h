#ifndef DRAWING_H
#define DRAWING_H

#include <nds.h>

// Launches the signature capture interface. 
// Returns 1 if successfully saved, 0 if canceled or failed.
int show_drawing_capture(u16* top_vram, u16* bottom_vram, const char* signature_path);

#endif
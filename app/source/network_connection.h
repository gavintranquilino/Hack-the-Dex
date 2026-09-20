#ifndef NETWORK_CONNECTION_H
#define NETWORK_CONNECTION_H

#include <nds.h>

// Show the live outer-camera preview using the caller's bitmap VRAM buffers.
// The entry A press is ignored; release A, then press A to capture and fetch a
// profile. Failures resume the preview and allow retry.
// Returns 0 only after saving the unchanged profile JSON, or 1 on B cancel.
int show_network_connection_screen(u16* top_vram, u16* bottom_vram, const char* timestamp_str);

#endif

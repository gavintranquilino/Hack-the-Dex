# DS Application

This directory contains the main Hack the Dex Nintendo DS/DSi application,
built with BlocksDS.

- `source/` contains the application screens and behavior: camera capture,
  drawing, QR/profile networking, offline entry, and setup.
- `include/` contains the public headers shared by those modules, including `styles.h` for the shared UI theme and components.
- `source/styles.c` implements the shared palette, screen frame, cards, buttons, and page chrome used by each UI screen.
- `Makefile` defines the offline, emulator, no-SD emulator, and Hack the North build modes.
- `build/` contains generated build artifacts.

Build from this directory with `make`, `make EMU=1`, `make EMU-NO-SD=1`, or
`make HACKTHENORTH=1`. Final `.nds` ROMs are written to `../builds/roms`
(from the `app/` directory).
See the [root README](../README.md) for prerequisites and the complete build
matrix, [the project documentation](../docs/README.md) for the runtime
architecture, and the [DS UI Styles guide](../docs/STYLES.md) for shared
component usage.
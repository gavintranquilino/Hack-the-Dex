# DS Application

This directory contains the main Hack the Dex Nintendo DS/DSi application,
built with BlocksDS.

- `source/` contains the application screens and behavior: camera capture,
  drawing, QR/profile networking, offline entry, and setup.
- `include/` contains the public headers shared by those modules, including
  `styles.h` for the shared UI theme and components.
- `source/styles.c` implements the shared palette, screen frame, cards, buttons,
  and page chrome used by each UI screen.
- `Makefile` defines the available build modes.
- `build/` contains generated build artifacts.

Follow the [development environment guide](../.devcontainer/README.md) to set
up BlocksDS and build the application. Final `.nds` ROMs are written to
`app/build/roms/`.

See the [project documentation](../docs/README.md) for the runtime architecture
and [the DS UI Styles guide](../docs/STYLES.md) for shared component usage.

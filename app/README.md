# DS Application

This directory contains the main Hack the Dex Nintendo DS/DSi application,
built with BlocksDS.

- `source/` contains the application screens and behavior: camera capture,
  drawing, QR/profile networking, offline entry, and setup.
- `include/` contains the public headers shared by those modules.
- `Makefile` defines the online, emulator, and offline build flags.
- `build/` contains generated build artifacts.

Build from this directory with `make`, `make EMU=1`, or `make OFFLINE=1`.
See the [root README](../README.md) for prerequisites and the complete build
matrix, and [the project documentation](../docs/README.md) for the runtime
architecture.
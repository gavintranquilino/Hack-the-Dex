# Hack the Dex

Hack the Dex is a Nintendo DS/DSi friendship and profile-sharing project made
for Hack the North 2026. The application uses the DSi camera to scan a QR
code, retrieves a Hack the North profile through the companion web proxy, and
lets the user add a photo and drawing before saving the result to the SD card.

This repository is organized as a small monorepo:

- [`app/`](app/README.md) contains the main BlocksDS application.
- [`web/`](web/README.md) contains the local Python receiver and ngrok proxy.
- [`snippets/`](snippets/README.md) contains focused DS, camera, Wi-Fi, QR,
	and drawing experiments.
- [`docs/`](docs/README.md) contains the project documentation and architecture
	notes.

## Build the DS application

The application is built with [BlocksDS](https://blocksds.github.io/). Install
the BlocksDS toolchain and make sure its core is available at
`/opt/blocksds/core`, or set `BLOCKSDS` to your local installation path.

Build from the `app/` directory:

```bash
cd app
make
```

The default build is the offline SD-card build and produces
`app/hack_the_dex_offline.nds`.

### Build flags

The Makefile accepts these boolean flags:

| Command | Result |
| --- | --- |
| `make` | Offline SD-card build: `hack_the_dex_offline.nds` |
| `make EMU=1` | Offline emulator build with SD-card reads and writes: `hack_the_dex_emu.nds` |
| `make EMU-NO-SD=1` | Offline emulator build that performs no SD-card writes: `hack_the_dex_emu_no_sd.nds` |
| `make HACKTHENORTH=1` | Online Hack the North build: `hack_the_dex.nds` |

To use a BlocksDS installation elsewhere:

```bash
make BLOCKSDS=/path/to/blocksds/core
```

`EMU=1` runs the offline profile-entry flow while retaining SD-card reads and
writes. `EMU-NO-SD=1` is the emulator-safe variant: it uses a pre-created
`sd:/hackthedex/emulator` path and bypasses photo and drawing writes.
`HACKTHENORTH=1` enables the Wi-Fi and QR profile flow in
`network_connection.c`. Use at most one mode flag at a time; the flags are
compile-time options, not runtime settings.

## Run the web companion

The online build expects the local receiver and ngrok tunnel from
[`web/README.md`](web/README.md). Set up the Python dependencies, authenticate
ngrok, start the Chromium profile, and run the proxy before using the online
DS build.

## Project history

Hack the Dex was started on September 19, 2026 for Hack the North 2026. The
repository grew from a single hackathon project to a passion project
to use outside of the hackathon. 
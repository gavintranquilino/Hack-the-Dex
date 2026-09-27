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

The default build is the online Hack the North release and produces
`app/hack_the_dex.nds`.

### Build flags

The Makefile accepts these boolean flags:

| Command | Result |
| --- | --- |
| `make` | Online release build: `hack_the_dex.nds` |
| `make EMU=1` | Emulator build: `hack_the_dex_emu.nds` |
| `make OFFLINE=1` | Offline build: `hack_the_dex_offline.nds` |

To use a BlocksDS installation elsewhere:

```bash
make BLOCKSDS=/path/to/blocksds/core
```

`EMU=1` defines `EMU`, skips the live network setup, and uses emulator-safe
profile paths and camera file behavior. `OFFLINE=1` defines `OFFLINE`, skips
the Wi-Fi and QR profile flow, and opens the local profile-entry experience.
Use one mode flag at a time; the flags are compile-time options, not runtime
settings.

## Run the web companion

The online build expects the local receiver and ngrok tunnel from
[`web/README.md`](web/README.md). Set up the Python dependencies, authenticate
ngrok, start the Chromium profile, and run the proxy before using the online
DS build.

## Project history

Hack the Dex was started on September 19, 2026 for Hack the North 2026. The
repository grew during the event from BlocksDS and DSi connectivity tests into
the camera, QR, profile, drawing, and web-proxy workflow. Offline mode and the
web setup flow were added during the September 20-21 development pass.

## Planned additions

- Sending files through Download Play.
- A DS mode that removes or reduces the camera-specific content.
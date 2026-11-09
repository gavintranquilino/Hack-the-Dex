# Hack the Dex

Hack the Dex is a Nintendo DS/DSi friendship and profile-sharing project made
for Hack the North 2026. The application uses the DSi camera to scan a QR
code, retrieves a Hack the North profile through the companion web proxy, and
lets the user add a photo and drawing before saving the result to the SD card.

## Start here

1. Set up the project development environment using the [devcontainer guide](.devcontainer/README.md).
2. Build the Nintendo DS/DSi application by following that guide.
3. Choose an emulator or hardware device to run the resulting `.nds` ROM. Emulator setup is up to you; [melonDS](https://melonds.kuribo64.net/) is a recommended option.
4. The legacy web companion is deprecated and remains in this branch for now;
   its existing setup notes are in [`web/README.md`](web/README.md).

The development container provides BlocksDS and the Wonderful Toolchain. Docker
works on Linux, Windows, and macOS; Linux is the primary development target.

## Repository map

- [`app/`](app/README.md) contains the main BlocksDS application.
- [`web/`](web/README.md) contains the local Python receiver and ngrok proxy.
- [`snippets/`](snippets/README.md) contains focused DS, camera, Wi-Fi, QR,
  and drawing experiments.
- [`docs/`](docs/README.md) contains project documentation and architecture notes.

## Project history

Hack the Dex was started on September 19, 2026 for Hack the North 2026. The
repository grew from a single hackathon project to a passion project to use
outside of the hackathon.

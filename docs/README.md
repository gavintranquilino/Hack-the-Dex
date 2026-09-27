# Hack the Dex Documentation

Hack the Dex is a Hack the North 2026 project for Nintendo DS/DSi hardware.
It turns a DSi into a small social profile exchange: scan a friend's QR code,
retrieve their Hack the North profile, take a photo, draw a signature, and save
the resulting profile locally on the SD card.

## System overview

The project has two cooperating applications:

1. The DS application in [`app/`](../app/) runs on a DSi or emulator. It owns
   the screens, camera capture, drawing tools, QR scanning, Wi-Fi connection,
   profile validation, and SD-card storage.
2. The desktop companion in [`web/`](../web/) receives the camera frame over
   TCP, uses the Hack the North website to identify the scanned profile, and
   returns profile JSON and image data to the DS.

The online flow is:

```text
DSi camera -> QR decode -> Wi-Fi -> ngrok TCP tunnel -> web/main.py
                                               -> Hack the North website
DSi profile JSON and assets <- TCP response <- profile extraction
```

The web proxy uses Chromium over CDP so the Hack the North login session can
be completed in a normal browser. See [`web/README.md`](../web/README.md) for
setup, authentication, ngrok, and runtime instructions.

## Application modes

The BlocksDS Makefile in [`app/Makefile`](../app/Makefile) controls three
compile-time builds:

- The default build is the online release and starts with host and port setup.
- `EMU=1` builds an emulator-oriented version and avoids hardware-specific
  file behavior that is unsafe in the emulator.
- `OFFLINE=1` builds a local profile-entry version without Wi-Fi or QR profile
  retrieval.

The root [`README.md`](../README.md) contains the build commands and expected
`.nds` output names.

## Storage

Profiles are stored below `sd:/hackthedex`. Online profiles use timestamped
directories containing the profile JSON and captured assets. Emulator builds
use `sd:/hackthedex/emulator` so repeated tests do not depend on DSi storage
behavior.

## Repository layout

- [`app/`](../app/README.md): primary BlocksDS application, headers, and source.
- [`web/`](../web/README.md): Python TCP receiver, browser integration, and
  ngrok runner.
- [`snippets/`](../snippets/README.md): isolated experiments used while
  developing camera, Wi-Fi, QR, signature, and DSi behavior.
- [`app/build/`](../app/build/): generated dependency files and build output;
  do not edit these files by hand.

## History

The project began on September 19, 2026 during Hack the North 2026. Early
commits explored BlocksDS, DSi Wi-Fi, HTTP/TCP connectivity, and the camera.
The main workflow was assembled around the camera and drawing screens, then
the web proxy was integrated for profile lookup. Offline mode was added on
September 20, followed by web setup cleanup on September 21.

## Future work

- Send files through Download Play.
- Support a DS mode with camera-specific content removed or reduced.
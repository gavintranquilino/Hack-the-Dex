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

## Edit a friend's socials

Open a friend card and tap **EDIT SOCIALS**. The existing offline keyboard opens
with Discord, Instagram, Twitter, and LinkedIn prefilled. Use Up/Down to select a
social field, the touchscreen keyboard to type, and DEL to remove characters.
Tap **A SAVE** (or press A) to save; tap **B CANCEL** (or press B) to discard.
Fields retain the existing limit of 63 characters. Name and pronouns are not
editable here.

Saving updates `profile.json` in the friend's existing timestamp directory and
preserves other JSON members, the portrait, and the signature. A failed save
keeps the editor open for retry or cancel. On SD filesystems that cannot replace
an existing file with `rename`, the previous JSON is retained as
`profile.json.bak.1`, `.bak.2`, etc. No profile folders are deleted by editing.
If power is lost between the two SD rename operations, the previous JSON can be
restored from the latest numbered backup. A completed temporary write may also
remain as `profile.json.tmp` after a failure.

Run the storage regression tests on a host with a C compiler, from the repository
root:

```sh
gcc -std=c11 -Wall -Wextra -Werror -Iapp/include \
    app/tests/profile_storage_test.c app/source/profile_storage.c \
    app/source/cJSON.c -Wl,--wrap=rename -lm \
    -o /tmp/hackthedex-profile-storage-test
/tmp/hackthedex-profile-storage-test
```

The tests simulate the SD filesystem's rename restriction and check repeated
updates, preserved metadata, escaped characters, empty socials, malformed JSON,
write failures, and rollback after a failed replacement. For a device/emulator
check, use an SD-enabled build: edit a saved friend, reopen the card and restart
the app to verify persistence; then edit again and cancel to verify the saved
values stay intact. The no-SD build has no saved profiles to edit.

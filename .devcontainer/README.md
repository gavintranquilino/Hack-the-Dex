# Development environment

The devcontainer provides BlocksDS and the Wonderful Toolchain through the
official BlocksDS Docker image. Docker works on Linux, Windows, and macOS;
Linux is the primary target. Python and `uv` are available for manual use.

## Start the container

Install Docker and Node.js/npm, then install the Dev Container CLI:

```sh
npm install -g @devcontainers/cli
```

Run the following commands from the project root, the directory containing
`.devcontainer/`:

```sh
devcontainer up --workspace-folder .
devcontainer exec --workspace-folder . bash
```

The project files and build outputs are shared with the host. After changing
the devcontainer configuration, recreate it from the project root with:

```sh
devcontainer up --workspace-folder . --remove-existing-container
```

## Build the DS application

In the container, from the project root:

```sh
cd app
make
```

This builds the offline SD-card ROM in `app/build/roms/`. Optional build modes
(choose at most one) are:

```sh
make EMU=1           # emulator build with SD-card reads and writes
make EMU-NO-SD=1     # emulator build without SD-card writes
make HACKTHENORTH=1  # online Hack the North profile flow
```

Choose and configure an emulator on your host; melonDS is recommended. The
container only builds the ROM.

The legacy web companion is deprecated but remains in this branch. Its
instructions are in [`../web/README.md`](../web/README.md).

#!/usr/bin/env bash
set -euo pipefail

if [[ ! -f "${BLOCKSDS}/sys/default_makefiles/rom_arm9/Makefile" ]]; then
    echo "BlocksDS SDK not found at ${BLOCKSDS}" >&2
    echo "Check the BlocksDS image installation and rebuild the devcontainer." >&2
    exit 1
fi

echo "BlocksDS: ${BLOCKSDS}"
echo "uv: $(uv --version)"

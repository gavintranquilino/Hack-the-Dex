# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2024

BLOCKSDS ?= /opt/blocksds/core

# User config

NAME := hack_the_dex
GAME_TITLE := Hack the Dex
GAME_SUBTITLE := Hack the North 2026 Project

# Libraries

# Added -lfat for SD card access and -lm for cJSON math dependencies
LIBS := -ldswifi9 -lfat -lm -lnds9

# Added the libfat directory to the search paths
# Libraries

LIBS := -ldswifi9 -lm -lnds9

LIBDIRS := $(BLOCKSDS)/libs/dswifi \
           $(BLOCKSDS)/libs/libnds

# ARM7 core with DSWiFi support

ARM7ELF := $(BLOCKSDS)/sys/arm7/main_core/arm7_dswifi_maxmod.elf

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile
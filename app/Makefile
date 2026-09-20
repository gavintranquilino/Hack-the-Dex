# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2024

BLOCKSDS ?= /opt/blocksds/core

# User config

NAME := hack_the_dex
GAME_TITLE := Hack the Dex
GAME_SUBTITLE := Hack the North 2026 Project

# Emulator build
EMU ?= 0
OFFLINE ?= 0

SOURCEDIRS := source
INCLUDEDIRS := include

ifeq ($(EMU),1)
    CFLAGS += -DEMU
    NAME := hack_the_dex_emu
endif

ifeq ($(OFFLINE),1)
    NAME := hack_the_dex_offline
    GAME_TITLE := Hack the Dex [Offline]
    CFLAGS += -DOFFLINE
endif

# Libraries

LIBS := -ldswifi9 -lm -lnds9

LIBDIRS := $(BLOCKSDS)/libs/dswifi \
           $(BLOCKSDS)/libs/libnds

# ARM7 core with DSWiFi support

ARM7ELF := $(BLOCKSDS)/sys/arm7/main_core/arm7_dswifi_maxmod.elf

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile
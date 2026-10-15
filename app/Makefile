# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2024

BLOCKSDS ?= /opt/blocksds/core

# User config

NAME := hack_the_dex
GAME_TITLE := Hack the Dex
GAME_SUBTITLE := Hack the North 2026 Project

# Build modes. With no flag, build the offline SD-card version.
EMU ?= 0
EMU-NO-SD ?= 0
HACKTHENORTH ?= 0

SOURCEDIRS := source
INCLUDEDIRS := include

BUILD_MODE_COUNT := $(shell expr $(EMU) + $(EMU-NO-SD) + $(HACKTHENORTH))
ifneq ($(BUILD_MODE_COUNT),0)
ifneq ($(BUILD_MODE_COUNT),1)
    $(error Select at most one build mode: EMU=1, EMU-NO-SD=1, or HACKTHENORTH=1)
endif
endif

ifeq ($(HACKTHENORTH),1)
    NAME := hack_the_dex
    CFLAGS += -DHACKTHENORTH
else
    NAME := hack_the_dex_offline
    GAME_TITLE := Hack the Dex [Offline]
    CFLAGS += -DOFFLINE
endif

ifeq ($(EMU),1)
    NAME := hack_the_dex_emu
    GAME_TITLE := Hack the Dex [Emulator]
    CFLAGS += -DEMU
endif

ifeq ($(EMU-NO-SD),1)
    NAME := hack_the_dex_emu_no_sd
    GAME_TITLE := Hack the Dex [Emulator No SD]
    CFLAGS += -DEMU_NO_SD
endif

# Libraries

LIBS := -ldswifi9 -lm -lnds9

LIBDIRS := $(BLOCKSDS)/libs/dswifi \
           $(BLOCKSDS)/libs/libnds

# ARM7 core with DSWiFi support

ARM7ELF := $(BLOCKSDS)/sys/arm7/main_core/arm7_dswifi_maxmod.elf

# BlocksDS normally places the final ROM beside this Makefile. Keep all
# distributable ROM variants together at the repository level instead.
ROM_OUTPUT_DIR := ../app/build/roms
override ROM := $(ROM_OUTPUT_DIR)/$(NAME).nds

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile

$(ROM): | $(ROM_OUTPUT_DIR)

$(ROM_OUTPUT_DIR):
	mkdir -p $@
# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2024

BLOCKSDS ?= /opt/blocksds/core

NAME := ping
GAME_TITLE := Google Ping Test
GAME_SUBTITLE := Wi-Fi Connection Test

LIBS := -ldswifi9 -lnds9

LIBDIRS := $(BLOCKSDS)/libs/dswifi \
           $(BLOCKSDS)/libs/libnds

ARM7ELF := $(BLOCKSDS)/sys/arm7/main_core/arm7_dswifi_maxmod.elf

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile

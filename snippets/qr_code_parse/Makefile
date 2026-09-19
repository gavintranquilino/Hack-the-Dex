# User config
NAME          := qr_code_parser
GAME_TITLE    := QR Parse
GAME_SUBTITLE := Wi-Fi Demo

# CRITICAL: We must assign an ARM7 core that includes the Wi-Fi drivers
ARM7ELF       := $(BLOCKSDS)/sys/arm7/main_core/arm7_dswifi_maxmod.elf

# The libraries to link against
LIBS          := -ldswifi9 -lnds9

# The directories where the compiler should look for the libraries AND their .h headers
LIBDIRS       := $(BLOCKSDS)/libs/dswifi \
                 $(BLOCKSDS)/libs/libnds

# Include the core BlocksDS ARM9 build rules
include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile
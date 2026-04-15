TARGET = ChoirSATB
APP_TYPE = BOOT_QSPI

USE_DAISYSP_LGPL = 1
USE_DEBUG = 0
DAISY_PLATFORM = DAISY_SEED

# Library Locations
LIBDAISY_DIR ?= ../../libDaisy
DAISYSP_DIR ?= ../../DaisySP

MONKEY_DIR ?= ../../../Monkey
MONKEY_SRCS = \
	$(MONKEY_DIR)/Monkey.cpp \
	$(MONKEY_DIR)/Music.cpp \
	$(MONKEY_DIR)/Euclid.cpp

# Sources
CPP_SOURCES = \
	Main.cpp \
	Menu.cpp \
	Voices/Voice.cpp \
	$(MONKEY_SRCS)
	
C_DEFS += -DUSE_DEBUG=$(USE_DEBUG) -DDAISY_PLATFORM=$(DAISY_PLATFORM) -Wno-unused-variable
OPT = -Os
C_INCLUDES += -I$(MONKEY_DIR)


# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

show_size: # $(BUILD_DIR)/$(TARGET).elf
	@echo "Size of $(TARGET).elf:"
	@arm-none-eabi-nm --print-size --size-sort --reverse-sort $(BUILD_DIR)/$(TARGET).elf
	

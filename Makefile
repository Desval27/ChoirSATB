TARGET = ChoirSATB
#APP_TYPE = BOOT_QSPI

USE_DAISYSP_LGPL = 1
USE_DEBUG = 0
DAISY_PLATFORM = DAISY_SEED

# Library Locations
LIBDAISY_DIR ?= ../../libDaisy
DAISYSP_DIR ?= ../../DaisySP

MONKEY_DIR = ../../../Monkey
MONKEY_SRC = $(MONKEY_DIR)/src
MONKEY_INC = $(MONKEY_DIR)/include
MONKEY_CPP_SOURCES = $(wildcard $(MONKEY_SRC)/*.cpp)

# Sources
CPP_SOURCES = \
	Main.cpp \
	App.cpp \
	Voice.cpp \
	Pages/MainPage.cpp \
	Pages/MixerPage.cpp \
	Pages/VoicePage.cpp \
	$(MONKEY_CPP_SOURCES)
	
C_DEFS += -DDAISY_PLATFORM=$(DAISY_PLATFORM) -Wno-unused-variable
#OPT = -Og 
OPT = -Os
C_INCLUDES += -I. -I$(MONKEY_INC)


# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

show_size: # $(BUILD_DIR)/$(TARGET).elf
	@echo "Size of $(TARGET).elf:"
	@arm-none-eabi-nm --print-size --size-sort --reverse-sort $(BUILD_DIR)/$(TARGET).elf
	

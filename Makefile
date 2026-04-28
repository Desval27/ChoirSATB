TARGET = ChoirSATB
APP_TYPE = BOOT_QSPI

USE_DAISYSP_LGPL = 1
USE_DEBUG = 0
PLATFORM = DAISY_SEED

# Library Locations
LIBDAISY_DIR ?= ../../libDaisy
DAISYSP_DIR ?= ../../DaisySP

MONKEY_SEED_DIR = ..
MONKEY_SEED_SRC = $(MONKEY_SEED_DIR)/src
MONKEY_SEED_INC = $(MONKEY_SEED_DIR)/include
MONKEY_SEED_CPP_SOURCES = $(wildcard $(MONKEY_SEED_SRC)/*.cpp) 

MONKEY_DIR = ../../../Monkey
MONKEY_SRC = $(MONKEY_DIR)/src
MONKEY_INC = $(MONKEY_DIR)/include
MONKEY_CPP_SOURCES = $(wildcard $(MONKEY_SRC)/*.cpp) 

APP_SRC = src
APP_INC = include
APP_CPP_SOURCES := $(filter-out $(APP_SRC)/Main2.cpp, $(wildcard $(APP_SRC)/*.cpp)) $(wildcard $(APP_SRC)/Pages/*.cpp)
#APP_CPP_SOURCES := $(APP_SRC)/Main2.cpp

# Sources
CPP_SOURCES = \
	$(APP_CPP_SOURCES) \
	$(MONKEY_SEED_CPP_SOURCES) \
	$(MONKEY_CPP_SOURCES)
	
C_DEFS += -DDAISY_PLATFORM -DPLATFORM=$(PLATFORM) -Wno-unused-variable -Wno-unused-function
#OPT = -Og 
OPT = -Os
C_INCLUDES += -I$(APP_INC) -I$(MONKEY_SEED_INC) -I$(MONKEY_INC)

.PHONY: garp


# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

show_size: # $(BUILD_DIR)/$(TARGET).elf
	@echo "Size of $(TARGET).elf:"
	@arm-none-eabi-nm --print-size --size-sort --reverse-sort $(BUILD_DIR)/$(TARGET).elf
	
garp:
	@echo $(CPP_SOURCES)


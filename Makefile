TARGET = ChoirSATB

USE_DAISYSP_LGPL = 1
USE_DEBUG = 0

# Library Locations
LIBDAISY_DIR ?= ../../libDaisy
DAISYSP_DIR ?= ../../DaisySP

MONKEY_DIR ?= ../../../Monkey
MONKEY_SRCS = \
	$(MONKEY_DIR)/Monkey.cpp \
	$(MONKEY_DIR)/Music.cpp \
	$(MONKEY_DIR)/Euclid.cpp

# Sources
CPP_SOURCES = Main.cpp $(MONKEY_SRCS)
C_DEFS += -DUSE_DEBUG=$(USE_DEBUG)
C_INCLUDES += -I$(MONKEY_DIR)


# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
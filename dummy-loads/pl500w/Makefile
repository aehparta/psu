
# default target is x86
TARGET = X86

# enable log and display
USE += LOG DISPLAY GPIO
USE_X86 += I2C FTDI
USE_AVR += I2C
USE_RPI += I2C
USE_ESP32 += I2C

FONTS = all

LDFLAGS += -lm

# init
include $(LIBE_PATH)/init.mk

# our own sources etc
BUILD_BINS = display
PROGRAM_BIN = display
display_SRC = main.c $(libe_SRC)

# build
include $(LIBE_PATH)/build.mk

# programmable targets
include $(LIBE_PATH)/flash/program.mk

-include local.mk

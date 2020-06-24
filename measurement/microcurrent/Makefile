
TARGET = OPI
USE = LOG SPI GPIO DRIVER_MCP356X DISPLAY I2C

# init
include $(LIBE_PATH)/init.mk

# our own sources etc
BUILD_BINS = microcurrent
PROGRAM_BIN = microcurrent
microcurrent_SRC = main.c httpd.c $(libe_SRC)
microcurrent_SRC += $(LIBE_PATH)/libe/fonts/fontcmtt30.c $(LIBE_PATH)/libe/fonts/fontcmtt24.c $(LIBE_PATH)/libe/fonts/fontfixed6x8.c
LDFLAGS += -lmicrohttpd

# build
include $(LIBE_PATH)/build.mk

# programmable targets
include $(LIBE_PATH)/flash/program.mk

-include local.mk

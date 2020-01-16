
TARGET = AVR
MCU_AVR = atmega328p
F_CPU_AVR = 8000000
LDFLAGS_AVR += -Wl,-u,vfprintf -lprintf_flt

USE = I2C GPIO LOG ADC NVM AVR_CLOCK_TIMER1_100HZ
DEFINES = LOG_BAUD=38400

# init
include $(LIBE_PATH)/init.mk

# our own sources etc
BUILD_BINS = cla
PROGRAM_BIN = cla
cla_SRC = main.c $(libe_SRC)

# build
include $(LIBE_PATH)/build.mk

# programmable targets
include $(LIBE_PATH)/flash/program.mk

-include local.mk

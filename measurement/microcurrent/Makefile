
TARGET = OPI
USE = LOG LOG_COLORS SPI GPIO DRIVER_MCP356X DISPLAY I2C

# init
include $(LIBE_PATH)/init.mk

# our own sources etc
BUILD_BINS = microcurrent
PROGRAM_BIN = microcurrent
microcurrent_SRC = main.c httpd.c opt.c $(libe_SRC)
microcurrent_SRC += $(LIBE_PATH)/libe/fonts/fontcmtt30.c $(LIBE_PATH)/libe/fonts/fontcmtt24.c $(LIBE_PATH)/libe/fonts/fontfixed6x8.c
LDFLAGS += -lmicrohttpd

# build
include $(LIBE_PATH)/build.mk

# install
INSTALL_BASE_DIR = /opt/microcurrent
install: all
	sudo mkdir -p $(INSTALL_BASE_DIR)/bin/
	sudo mkdir -p $(INSTALL_BASE_DIR)/html/
	sudo cp $(PROGRAM_BIN)$(TARGET_EXT)$(BIN_EXT) $(INSTALL_BASE_DIR)/bin/microcurrent
	sudo cp -r ./web/* $(INSTALL_BASE_DIR)/html/
	sudo cp microcurrent.service /lib/systemd/system/microcurrent.service
	sudo systemctl enable microcurrent
	sudo service microcurrent start

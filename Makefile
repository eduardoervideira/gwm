CC = gcc
CFLAGS = -Wall -Wextra -g $(shell pkg-config --cflags xcb xcb-util xcb-icccm xcb-ewmh xcb-randr xcb-keysyms)
LDFLAGS = $(shell pkg-config --libs xcb xcb-util xcb-icccm xcb-ewmh xcb-randr xcb-keysyms) -lm

SRCDIR = src
BUILDDIR = build
TARGET = $(BUILDDIR)/gwm

SOURCES = $(wildcard $(SRCDIR)/*.c)

CONFIG_FILE = gwmrc

ifdef XDG_CONFIG_HOME
    CONFIG_DIR := $(XDG_CONFIG_HOME)
else
    CONFIG_DIR := $(HOME)/.config
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p $(BUILDDIR)
	@echo "compiling gwm"
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -rf $(BUILDDIR)/*

config:
	@mkdir -p $(CONFIG_DIR)/gwm
	@echo "creating config directory: $(CONFIG_DIR)/gwm"
	@cp $(CONFIG_FILE) $(CONFIG_DIR)/gwm/$(CONFIG_FILE)
	@echo "copying default config file: $(CONFIG_FILE) -> $(CONFIG_DIR)/gwm/$(CONFIG_FILE)"

install: $(TARGET)
	@cp $(TARGET) /usr/bin/gwm
	@echo "copying gwm binary: $(TARGET) -> /usr/bin/gwm"

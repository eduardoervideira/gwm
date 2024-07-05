CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lxcb -lxcb-util -lxcb-ewmh -lxcb-randr -lxcb-keysyms -lxcb-icccm -lm

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
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -rf $(BUILDDIR)/*

config:
	@mkdir -p $(CONFIG_DIR)/gwm
	@echo "creating config directory: $(CONFIG_DIR)/gwm"
	@cp $(CONFIG_FILE) $(CONFIG_DIR)/gwm/$(CONFIG_FILE)
	@echo "copying default config file: $(CONFIG_FILE) -> $(CONFIG_DIR)/gwm/$(CONFIG_FILE)"

install: $(TARGET)
	@cp $(TARGET) /usr/bin/gwm
	@echo "copying gwm binary: $(TARGET) -> /usr/bin/gwm"# DO NOT DELETE

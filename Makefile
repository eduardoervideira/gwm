CC = gcc
CFLAGS = -Wall -Wextra -g $(shell pkg-config --cflags xcb xcb-util xcb-icccm xcb-ewmh xcb-randr xcb-keysyms)
LDFLAGS = $(shell pkg-config --libs xcb xcb-util xcb-icccm xcb-ewmh xcb-randr xcb-keysyms) -lm

SRCDIR = src
BUILDDIR = build
TARGET = $(BUILDDIR)/gwm

SOURCES = $(wildcard $(SRCDIR)/*.c)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p $(BUILDDIR)
	@echo "compiling gwm"
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -rf $(BUILDDIR)/*
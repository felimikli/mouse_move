CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags libevdev)
LIBS = $(shell pkg-config --libs libevdev)
SRC = mouse_move.c
BIN = mouse_move

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
TARGET = $(BINDIR)/$(BIN)


.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(SRC) config.h
	$(CC) $(CFLAGS) -o $(BIN) $(SRC) $(LIBS)

clean:
	rm -f $(BIN)

install: all
	sudo install -Dm755 $(BIN) $(TARGET)
	@echo "Installed to $(TARGET)"

uninstall:
	sudo rm -f $(TARGET)
	@echo "Uninstalled from $(TARGET)"

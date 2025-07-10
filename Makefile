CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -levdev
SRC = mouse_move.c
BIN = mouse_move

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
TARGET = $(BINDIR)/$(BIN)

CAPABILITIES = cap_dac_read_search,cap_sys_admin=eip

.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(SRC) config.h
	$(CC) $(CFLAGS) -o $(BIN) $(SRC) $(LIBS)

clean:
	rm -f $(BIN)

install: all
	sudo install -Dm755 $(BIN) $(TARGET)
	sudo setcap "$(CAPABILITIES)" $(TARGET)
	@echo "Installed to $(TARGET)"
	@echo "Capabilities set: $(CAPABILITIES)"

uninstall:
	sudo rm -f $(TARGET)
	@echo "Uninstalled from $(TARGET)"

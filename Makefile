# Makefile for mouse_move.c

CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -levdev
SRC = mouse_move.c
BIN = mouse_move

.PHONY: all clean install setcap

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC) $(LIBS)

clean:
	rm -f $(BIN)

install: clean all
	sudo cp $(BIN) /usr/local/bin/$(BIN)
	sudo chown root:root /usr/local/bin/$(BIN)
	sudo chmod u+s /usr/local/bin/$(BIN)

setcap: $(BIN)
	sudo setcap cap_dac_read_search,cap_sys_rawio+ep ./$(BIN)

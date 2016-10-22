CC = gcc
SRC = src/utfconverter.c
INCLUDE = include
BIN = bin/utf
CFLAGS = -g -Wall -Werror -pedantic -Wextra 
REQ = $(SRC) include/utfconverter.h

all: $(BIN)

$(BIN): build/utfconverter.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -I $(INCLUDE) $(SRC) -o $(BIN)

build/utfconverter.o: $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) -I $(INCLUDE) $(REQ) -o $@

.PHONY: clean

debug: all

clean:
	rm -rf bin build


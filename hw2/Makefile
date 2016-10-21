CC = gcc
SRC = $(wildcard src/*.c)
INCLUDE = $(wildcard include/)
BIN = bin/utf
CFLAGS = -g -Wall -Werror -pedantic -Wextra 
REQ = $(SRC) $(wildcard include/*.h)

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


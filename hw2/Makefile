CC = gcc
SRC = $(wildcard src/*.c)
INCLUDE = $(wildcard include/)
BIN = utfconverter
CFLAGS = -g -Wall -Werror -pedantic -Wextra 
REQ = $(SRC) $(wildcard include/*.h)

all: $(BIN)

$(BIN): $(REQ)
	$(CC) $(CFLAGS) -I $(INCLUDE) $(SRC) -o $(BIN)

.PHONY: clean

clean:
	rm -f *.o $(BIN)


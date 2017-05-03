CC = gcc
SRC = ./src/
INCLUDE = ./include/
BIN = ./bin/
BUILD = ./build/
DEPS = utfconverter.c
OBJECTS = utfconverter.o
OUT = utf
CFLAGS = -g -c -Wall -Werror -pedantic -Wextra  

all: $(OUT)

rebuild: clean all

$(OBJECTS):
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -I$(INCLUDE) $(SRC)$(DEPS) -o $(BUILD)$@

$(OUT): $(OBJECTS)
	mkdir -p $(BIN)
	$(CC) -I$(INCLUDE) $(BUILD)$(OBJECTS) -o $(BIN)$(OUT)

.PHONY: clean

clean:
	rm -f -r $(BIN) $(BUILD)

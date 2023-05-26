CC=gcc
LD=ld

INCS+=-Iinclude
LIBS+=

CPPFLAGS?=
CPPFLAGS+=$(INCS)

CFLAGS?=-O2 -g
CFLAGS+=-Wall -Wextra -MD -std=c99

LDFLAGS?=
LDFLAGS+=$(LIBS)

SRC_C=$(wildcard src/*.c src/**/*.c)
OBJ=$(SRC_C:.c=.o)

BIN=emu6502

all: $(BIN)

clean:
	rm -f $(OBJ) $(BIN)

%_c.o: %.c
	@echo "CC	$(shell basename $@)"
	@$(CC) -o $@ -c $< $(CPPFLAGS) $(CFLAGS)

$(BIN): $(OBJ)
	@echo "LD	$(shell basename $@)"
	@$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

.PHONY: all clean

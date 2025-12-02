CC = gcc
CFLAGS = -Wall -g -Ilib -Iutilities/card
OBJ = lib/generic.o utilities/lavagna.o utilities/card/card.o

all: $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

.PHONY: all clean

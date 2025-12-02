CC = gcc
CFLAGS = -Wall -Wextra -Icard -Ilib
OBJ = lavagna.o card/card.o lib/generic.o
TARGET = lavagna

lavagna.o: lavagna.c card/card.h lib/generic.h lib/structure.h
	$(CC) $(CFLAGS) -c lavagna.c -o lavagna.o

card/card.o: card/card.c card/card.h
	$(CC) $(CFLAGS) -c card/card.c -o card/card.o

lib/generic.o: lib/generic.c lib/generic.h
	$(CC) $(CFLAGS) -c lib/generic.c -o lib/generic.o

clean:
	rm -f $(OBJ) $(TARGET)

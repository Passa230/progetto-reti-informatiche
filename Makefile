CC = gcc
CFLAGS = -Wall -Wextra -Ilib -Iutilities -Iutilities/card

OBJ = utilities/lavagna.o \
      utilities/card/card.o \
      lib/generic.o

TARGET = lavagna

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

utilities/lavagna.o: utilities/lavagna.c utilities/card/card.h lib/generic.h lib/structure.h
	$(CC) $(CFLAGS) -c utilities/lavagna.c -o utilities/lavagna.o

utilities/card/card.o: utilities/card/card.c utilities/card/card.h
	$(CC) $(CFLAGS) -c utilities/card/card.c -o utilities/card/card.o

lib/generic.o: lib/generic.c lib/generic.h lib/structure.h
	$(CC) $(CFLAGS) -c lib/generic.c -o lib/generic.o

clean:
	rm -f $(OBJ) $(TARGET)
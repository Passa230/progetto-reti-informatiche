CC = gcc
CFLAGS = -Wall -g -I. -Ilib -Iutilities
LDFLAGS = -pthread

SRC_DIR = src

OBJ_LIB = $(SRC_DIR)/generic.o
OBJ_SERVER_UTILS = $(SRC_DIR)/lavagna.o 

all: lavagna utente

# Compilazione del lavagna
lavagna: lavagna.o $(OBJ_LIB) $(OBJ_SERVER_UTILS)
	$(CC) $(CFLAGS) -o lavagna lavagna.o $(OBJ_LIB) $(OBJ_SERVER_UTILS) $(LDFLAGS)

# Compilazione del utente
utente: utente.o $(OBJ_LIB)
	$(CC) $(CFLAGS) -o utente utente.o $(OBJ_LIB) $(LDFLAGS)

# Regola generica per la creazione dei file oggetto
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia dei file generati
clean:
	rm -f lavagna utente *.o lib/*.o utilities/*.o
.PHONY: all clean

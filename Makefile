CC = gcc
CFLAGS = -Wall -g -I. -Ilib -Iutilities
LDFLAGS = -pthread

# Oggetti comuni (librerie)
OBJ_LIB = lib/generic.o

# Oggetti specifici per il server
OBJ_SERVER_UTILS = utilities/lavagna.o 

# Target principale
all: server client

# Compilazione del server
server: server.o $(OBJ_LIB) $(OBJ_SERVER_UTILS)
	$(CC) $(CFLAGS) -o server server.o $(OBJ_LIB) $(OBJ_SERVER_UTILS) $(LDFLAGS)

# Compilazione del client
client: client.o $(OBJ_LIB)
	$(CC) $(CFLAGS) -o client client.o $(OBJ_LIB) $(LDFLAGS)

# Regola generica per la creazione dei file oggetto
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia dei file generati
clean:
	rm -f server client *.o lib/*.o utilities/*.o
.PHONY: all clean

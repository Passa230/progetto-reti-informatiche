#ifndef GENERIC_H
#define GENERIC_H

#include <pthread.h>

// Gestione dei booleani
#define TRUE    1
#define FALSE   0

typedef unsigned short bool_t;


// Gestione degli stati di una card
enum colonna {TO_DO, DOING, DONE};

typedef enum colonna colonna_t;

// Dichiarazioni per la generazione degli id (definizioni in generic.c)
extern unsigned int NEXT_CARD_ID;
extern pthread_mutex_t semaforo_ID;
unsigned int get_new_id(void);

#endif /* GENERIC_H */
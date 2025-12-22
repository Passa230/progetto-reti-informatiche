#ifndef GENERIC_H
#define GENERIC_H

#include <pthread.h>

#define MAX_COLUMN          3
#define MAX_USER            100
#define MAX_BUF_SIZE        255
#define MAX_MSG_BUF_SIZE    501
#define MAX_SBUF_SIZE       20000
#define MAX_NOTIFICATIONS   70
#define MAX_NOT_CMD_SIZE    50
#define MAX_NOT_BUF_SIZE    MAX_BUF_SIZE + MAX_NOT_CMD_SIZE
#define MAX_PONG_TIME       30
#define PING_TIMEOUT        90

// Gestione dei booleani
#define TRUE                0   
#define FALSE               1

typedef unsigned short bool_t;


// Gestione degli stati di una card


enum colonna {TO_DO, DOING, DONE};

typedef enum colonna colonna_t;

// Dichiarazioni per la generazione degli id (definizioni in generic.c)

typedef unsigned int id_t;
id_t get_new_id(void);



#endif /* GENERIC_H */
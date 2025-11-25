#ifdef GENERIC_H
#define GENERIC_H

#define TRUE    1
#define FALSE   0

typedef unsigned short bool_t;

enum colonna {TO_DO, DOING, DONE};

typedef enum colonna colonna_t;


unsigned int NEXT_CARD_ID = 0;
unsigned int get_new_id();



#endif
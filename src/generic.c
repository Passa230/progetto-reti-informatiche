#include <include/generic.h>

/* Definizioni delle variabili dichiarate come extern in generic.h */
unsigned int NEXT_CARD_ID = 1;
pthread_mutex_t semaforo_ID;

id_t get_new_id(void){
    if (NEXT_CARD_ID == 1) {
        pthread_mutex_init(&semaforo_ID, NULL);
    }
    
    unsigned int id;
    pthread_mutex_lock(&semaforo_ID);
    id = NEXT_CARD_ID++;
    pthread_mutex_unlock(&semaforo_ID);
    return id;
}


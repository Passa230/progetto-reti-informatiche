#ifndef CARD_H
#define CARD_H

#include <generic.h>
#include <structure.h>

/**
 * Funzione che genera una nuova card. 
 * @param testo_attivita descrive l'attività relativa alla card.
 * @param utente_creatore utente che ha creato la card.
 * @return il puntatore alla nuova card o null in caso non sia stato possibile crearla
 */
card_t * card_create(const char* testo_attivita, uint16_t utente_creatore);

/**
 * @brief Funzione che permette di distruggere una card liberando l'area di memoria ad essa associata
 * @param card la card che deve essere eliminata
 * @return 0 se è andato tutto bene, un codice di errore altrimenti
 */
bool_t card_delete(card_t** card);

/**
 * @brief funzione che permette di cambiare lo stato di una carta in uno di quelli successivi a quello in cui si trovava
 * @param card la card di cui si vuole cambiare lo stato
 * @param new_state lo stato in cui si vuole portare la carta
 * @return 1 se è andato tutto bene, 0 altrimenti
 */
bool_t card_state_change(card_t* card, colonna_t new_state);

#endif

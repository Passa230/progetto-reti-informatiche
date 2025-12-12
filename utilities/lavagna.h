#ifndef LAVAGNA_H
#define LAVAGNA_H

#include <structure.h>

/**
 * @brief funzione che permette di inizializzare la lavagna del kanban
 */
bool_t lavagna_init();

/**
 * @brief funzione che crea una nuova card e la aggiunge in testa a una lista
 * 
 * @param testo_attivita descrive l'attività relativa alla card.
 * @param utente_creatore utente che ha creato la card.
 *  
 * @return ritorna 0 in caso di successo, ritorna 1 in caso di errore.
 */
bool_t lavagna_card_add(const char* testo_attivita, uint16_t utente_creatore);

/**
 * @brief Funzione che permette di rimuovere una card passando l'ID e la lista
 * in cui si trova
 * 
 * @param id: identificativo della card che si vuole eliminare.
 * @param queue: riferimento al primo elemento della lista dalla quale si vuole eliminare l'elemento
 * @return la card rimossa se la funzione è terminata correttamente, NULL altrimenti.
 */
card_t* lavagna_card_remove(id_t id, id_t list);

/**
 * @brief funzione che permette di cambiare lo stato di una card e di muoverla
 * da una colonna a un altra.
 * 
 * @param id id della card da muovere
 * @param stc colonna sorgente della card
 * @param dst colonna dove si vuole muovere la card
 */
void lavagna_card_change(id_t id, id_t src, id_t dest);

/**
 * @brief Stampa il contenuto della lavagna
 */
void lavagna_stampa();

/**
 * @brief Funzione che permette all'utente di registrarsi alla lavagna
 * 
 * @param port la porta del socket su cui l'utente vuole registrarsi
 * @return ritorna 0 in caso di successo, ritorna 1 in caso di errore.
*/
bool_t lavagna_hello(uint16_t port);

/**
 * @brief funzione che permette all'utente di uscire dalla kanbar. Se l'utente aveva delle carte
 * nella colonna doing allora tornano in todo.
 * 
 * @param port la porta alla quale l'utente si era registrato
 */
bool_t lavagna_quit(uint16_t port);

/**
 * @brief Muove una card in testa alla lista specificata
 * 
 * @param card puntatore alla card da muovere
 * @param col colonna di destinazione
 */
void lavagna_move_card_to_head(card_t* card, colonna_t col);

#endif /* LAVAGNA_H */

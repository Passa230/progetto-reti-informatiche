#ifndef LAVAGNA_H
#define LAVAGNA_H

#include <structure.h>
#include <generic.h>

extern lavagna_t lavagna;

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
 * 
 * @param buf il buffer in cui salvare la lavagna
 * @param max_len lunghezza massima del buffer
 */
void lavagna_stampa(char* buf, size_t max_len);

/**
 * @brief Funzione che permette all'utente di registrarsi alla lavagna
 * 
 * @param port la porta del socket su cui l'utente vuole registrarsi
 * @param sock_id identificatore del socket utilizzato per la comunicazione
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

/**
 * @brief Controlla se un utente è registrato alla lavagna
 * 
 * @param port la porta dell'utente da controllare
 * @return TRUE se l'utente è registrato, FALSE altrimenti
 */
bool_t lavagna_is_user_registerd(uint16_t port);

/**
 * @brief Restituisce una stringa contenente la lista degli utenti connessi
 * 
 * @param buffer Buffer dove scrivere la lista
 * @param max_len Dimensione massima del buffer
 * @return TRUE se l'operazione è riuscita, FALSE altrimenti
 */
int lavagna_user_list(char* buf, size_t max_len);

/**
 * @brief funzione che permette di trovare una carta passando come argomento 
 * l'ID dell'utente a cui è assegnata
 * 
 * @param id identificatore dell'utente a cui è assegnata la carte
 * @return un puntatore alla card o NULL se non vi è alcuna corrispondenza
 */
card_t* lavagna_trova_card_per_id(int id);


/**
 * @todo Compattare tutto in un'unica funzione
 * @brief funzione che permette di popolare un vettore di uint16_t con le porte
 * degli utenti attualmente registrati nel sistema.
 * 
 * @param buf il buffer all'interno del quale inserire la lista delle porte utente.
 */

int lavagna_user_list_to_vec(uint16_t* buf);

#endif /* LAVAGNA_H */

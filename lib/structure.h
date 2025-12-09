
#include "generic.h"
#include <time.h>
#include <stdint.h>


/**
 * @struct card
 * @brief Struttura che rappresenta una card del sistema Kanban.
 *
 * La struttura contiene tutte le informazioni necessarie per gestire
 * una card, inclusi identificatore, stato, descrizione e metadati
 * relativi agli utenti e alle modifiche.
 */
struct card {
    // @brief identificatore univoco della card
    id_t id;

    // @brief rappresenta lo stato della card, intesto come la fase (tra ToDo, Doing e Done) in cui si trova
    colonna_t colonna;

    // @brief rappresenta la descrizione dell'attività che deve essere portata a termine
    char* testo_attivita;

    // @brief numero di porta del socket relativo all'utente creatore della card
    uint16_t utente_creatore;

    // @brief numero di porta del socket relativo all'utente che deve portare a termine la card
    uint16_t utente_assegnatario;

    // @brief istante dell'ultimo aggiornamento della card.
    time_t ultimo_aggiornamento;
    
    // @brief implementazione della lista di card
    struct card* next_card;
}; 

typedef struct card card_t;

/**
 * @struct card
 * @brief Struttura che rappresenta una lavagna del sistema Kanban.
 * 
 * La lavagna contiene tutte le informazioni per la memorizzazione delle card di un sistema
 * kanban.
 */
struct lavagna{
    // @brief indentificatore della lavagna kanban
    id_t id;

    /**
     * @brief brief array di liste che permette di identificare le card in funzione del loro stato.
     * 
     * cards[0]: card con stato TO_DO.
     * cards[1]: card con stato DOING.
     * cards[2]: card con stato DONE.
     */
    card_t* cards[3];

    // @brief semafori per la gestione della mutua esclusione nell'aggiunta delle card.
    pthread_mutex_t sem_cards[3];

    // @brief contatore per gli utenti attualmente connessi
    int connected_users;
    pthread_mutex_t conn_user_sem;

    // @brief vettore che contiene gli utenti che sono attualmente iscritti alla lavagna
    uint16_t utenti_registrati[MAX_USER];
    pthread_mutex_t reg_user_sem;
};

typedef struct lavagna lavagna_t;




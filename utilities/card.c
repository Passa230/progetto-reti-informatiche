
// TODO: Creare il file generic.h e importare quello
#include <generic.h>
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
    int id;

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
    
}; 

// Ridefinizione del tipo stuct card così da semplificarne la scrittura
typedef struct card card_t;


/***
 * Funzione che genera una nuova card. 
 * @param testo_attivita descrive l'attività relativa alla card.
 * @param utente_creatore utente che ha creato la card.
 * @return il puntatore alla nuova card o null in caso non sia stato possibile crearla
 */
card_t * card_create(const char* testo_attivita, uint16_t utente_creatore) {
    card_t *nuova_card = (card_t *)malloc(sizeof(card_t));
    if (nuova_card == NULL) {
        return NULL; // Gestione errore allocazione memoria
    }
    nuova_card->id = get_new_id(); // Da decidere: utilizzare variabile globale o meno ?
    nuova_card->colonna = TO_DO;
    nuova_card->testo_attivita = strdup(testo_attivita);
    nuova_card->utente_creatore = utente_creatore;
    nuova_card->utente_assegnatario = NULL;
    nuova_card->ultimo_aggiornamento = time(NULL);
    return nuova_card;
}


/**
 * @brief Funzione che permette di distruggere una card liberando l'area di memoria ad essa associata
 * @param card la card che deve essere eliminata
 * @return 0 se è andato tutto bene, un codice di errore altrimenti
 */
int card_delete(card_t** card){
    if (*card == NULL) {
        printf("ERRORE: Non puoi passare un puntatore a null");
    }
    int signal = free(*card);
    *card = NULL;
    return signal;
}


/**
 * @brief funzione che permette di cambiare lo stato di una carta in uno di quelli successivi a quello in cui si trovava
 * @param card la card di cui si vuole cambiare lo stato
 * @param new_state lo stato in cui si vuole portare la carta
 * @return 1 se è andato tutto bene, 0 altrimenti
 */
bool_t card_state_change(card_t* card, colonna_t new_state){
    if (card == NULL || new_state == NULL) {
        printf("ERRORE: Non è possibile passare valori null\n");
    }
    
    if (card->colonna == new_state) {
        printf("La card è già nello stato\n");
        return FALSE;
    } 
    if (card->colonna > new_state){
        printf("Non puoi portare la card in uno stato precedente\n");
    }

    card->colonna = new_state;
    return TRUE;    
}




#include <structure.h>
#include <generic.h>
#include <card/card.h>
#include <time.h>
#include <stdint.h>


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
    nuova_card->utente_assegnatario = 0;
    nuova_card->next_card = NULL;
    nuova_card->ultimo_aggiornamento = time(NULL);
    return nuova_card;
}


/**
 * @brief Funzione che permette di distruggere una card liberando l'area di memoria ad essa associata
 * @param card la card che deve essere eliminata
 * @return 0 se è andato tutto bene, un codice di errore altrimenti
 */
bool_t card_delete(card_t** card){
    if (card == NULL || *card == NULL) {
        printf("ERRORE: La card non può essere nulla\n");
        return FALSE;
    }
    if ((*card)->testo_attivita != NULL) {
        free((*card)->testo_attivita);
    }
    free(*card);
    *card = NULL;
    return TRUE;
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









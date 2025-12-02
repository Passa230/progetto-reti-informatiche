#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <generic.h>
#include <structure.h>
#include <card.h>


lavagna_t lavagna;



/**
 * @brief funzione che permette di inizializzare la lavagna del kanban
 */
bool_t lavagna_init(){
    for (int i = 0; i < MAX_COLUMN; i++) {
        lavagna.cards[i] = NULL;
        pthread_mutex_init(&lavagna.sem_cards[i], NULL);
    }

    lavagna.connected_users = 0;
    return TRUE;
}



void lavagna_move_card_to_head(card_t* card, colonna_t col){
    pthread_mutex_lock(&lavagna.sem_cards[col]);
    card_t* tmp = lavagna.cards[col];
    lavagna.cards[col] = card;
    card->next_card = tmp;
    pthread_mutex_unlock(&lavagna.sem_cards[col]);
}


/**
 * @brief funzione che crea una nuova card e la aggiunge in testa a una lista
 * 
 * @param testo_attivita descrive l'attività relativa alla card.
 * @param utente_creatore utente che ha creato la card.
 *  
 * @return ritorna 0 in caso di successo, ritorna 0 in caso di errore.
 */
bool_t lavagna_card_add(const char* testo_attivita, uint16_t utente_creatore){
    if (testo_attivita == NULL) {
        printf("ERRORE: Il testo deve contenere qualcosa\n");
        return FALSE;
    }

    /**
     * Possibilità: aggiungere un controllo ulteriore anche sull'ID della card
     */

    card_t* card = card_create(testo_attivita, utente_creatore);

    lavagna_move_card_to_head(card, 0);
    return TRUE;  
}

/**
 * @brief Funzione che permette di rimuovere una card passando l'ID e la lista
 * in cui si trova
 * 
 * @param id: identificativo della card che si vuole eliminare.
 * @param queue: riferimento al primo elemento della lista dalla quale si vuole eliminare l'elemento
 * @return la card rimossa se la funzione è terminata correttamente, NULL altrimenti.
 */
card_t* lavagna_card_remove(id_t id, id_t list) {
    if (id < 0 || id >= NEXT_CARD_ID) {
        printf("ERRORE: La card non esiste\n");
        return NULL;
    }

    if (list < 0 || list >= MAX_COLUMN) {
        printf("ERRORE: L'inserimento deve essere fatto in una lista corretta\n");
        return NULL;
    }
    
    card_t* curr = lavagna.cards[list];
    card_t* prev = NULL;
    while (curr != NULL) {
        if (curr->id == id) {
            if (prev == NULL) {
                // rimozione della testa della lista
                lavagna.cards[list] = curr->next_card;
            } else {
                prev->next_card = curr->next_card;
            }
            //card_delete(&curr);
            return curr;
        }
        prev = curr;
        curr = curr->next_card;
    }

    return NULL;
}


/**
 * @brief funzione che permette di cambiare lo stato di una card e di muoverla
 * da una colonna a un altra.
 * 
 * @param id id della card da muovere
 * @param stc colonna sorgente della card
 * @param dst colonna dove si vuole muovere la card
 */
void lavagna_card_change(id_t id, id_t src, id_t dest){
    
    card_t* card = lavagna_card_remove(id, src);
    card_state_change(card, dest);    
    lavagna_move_card_to_head(card, dest);

}


void lavagna_stampa(){
    printf("\n%-30s | %-30s | %-30s\n", "TO DO", "DOING", "DONE");
    printf("-------------------------------+--------------------------------+------------------------------\n");

    card_t* curr[3];
    for(int i=0; i<3; i++) curr[i] = lavagna.cards[i];

    while(curr[0] != NULL || curr[1] != NULL || curr[2] != NULL) {
        // Print IDs
        for(int i=0; i<3; i++) {
            if(curr[i] != NULL) {
                printf("ID: %-26d", curr[i]->id);
            } else {
                printf("%-30s", "");
            }
            if(i < 2) printf(" | ");
        }
        printf("\n");

        // Print Descriptions
        for(int i=0; i<3; i++) {
            if(curr[i] != NULL) {
                char buffer[27];
                const char* text = curr[i]->testo_attivita ? curr[i]->testo_attivita : "";
                strncpy(buffer, text, 26);
                buffer[26] = '\0';
                if(strlen(text) > 26) {
                    buffer[23] = '.'; buffer[24] = '.'; buffer[25] = '.';
                }
                printf("%-30s", buffer);
            } else {
                printf("%-30s", "");
            }
            if(i < 2) printf(" | ");
        }
        printf("\n");
        
        printf("-------------------------------+--------------------------------+------------------------------\n");

        // Advance pointers
        for(int i=0; i<3; i++) {
            if(curr[i] != NULL) curr[i] = curr[i]->next_card;
        }
    }
}


int main(){
    int n_card = 3;

    lavagna_init();

    for (int i = 0; i < n_card; i++) {
        lavagna_card_add("Hello word", 10);
    }
    
    lavagna_stampa();
}
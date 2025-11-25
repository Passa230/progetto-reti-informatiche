#include "generic.c"
#include <time.h>
#include <stdint.h>

// Definizione della card 
struct card {
    // Campo ID: gestito in modo incrementale e identificato da un intero
    int id;

    // Campo Colonna: rappresenta lo stato della card, intesto come la fase (tra ToDo, Doing e Done) in cui si trova
    colonna_t colonna;

    // Campo testo attività: rappresenta la descrizione dell'attività che deve essere portata a termine
    char* testo_attivita;

    // Campo relativo agli utenti, cioè il loro numero di porta, che hanno implmentata e completata
    uint16_t utente_creatore;
    uint16_t utente_assegnatario;

    // Campo Timestamp: rappresenta il momento in cui la card è stata modificata l'ultima volta
    time_t ultimo_aggiornamento;
    
}; 

// Ridefinizione del tipo stuct card così da semplificarne la scrittura
typedef struct card card_t;


// Funzione per creare una nuova card
card_t * crea_card(int id, colonna_t colonna, const char* testo_attivita, uint16_t utente_creatore, uint16_t utente_assegnatario) {
    card_t *nuova_card = (card_t *)malloc(sizeof(card_t));
    if (nuova_card == NULL) {
        return NULL; // Gestione errore allocazione memoria
    }
    nuova_card->id = id;
    nuova_card->colonna = colonna;
    nuova_card->testo_attivita = strdup(testo_attivita);
    nuova_card->utente_creatore = utente_creatore;
    nuova_card->utente_assegnatario = utente_assegnatario;
    nuova_card->ultimo_aggiornamento = time(NULL);
    return nuova_card;
}


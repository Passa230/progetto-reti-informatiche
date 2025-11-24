#include "generic.c"

// Definizione della card 
struct {
    // Campo ID: gestito in modo incrementale e identificato da un intero
    int id;

    // Campo Colonna: rappresenta lo stato della card, intesto come la fase (tra ToDo, Doing e Done) in cui si trova
    colonna_t colonna;

    // Campo testo attività: rappresenta la descrizione dell'attività che deve essere portata a termine
    char* testo_attivita;

    // Campo relativo agli utenti, cioè il loro numero di porta, che hanno implmentata e completata
    int utente_creatore;
    int utente_assegnatario;

    // Campo Timestamp: rappresenta il momento in cui la card è stata modificata l'ultima volta
    // TODO --> Decidere il tipo da utilizzare
    /**
     * [YYYY-MM-DD HH:MM:SS] --> Formato standard per rappresentare data e ora
     * si potrebbe usare una struttura apposita con dei campi int e una funzione che permette
     * di formattare la data e l'ora in questo formato.
     */
    
} card; 

// Ridefinizione del tipo stuct card così da semplificarne la scrittura
typedef struct card card_t;


#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <lavagna.h>
#include <stdlib.h>
#include <include/color.h>
#include <unistd.h>


lavagna_t lavagna;


// FUNZIONI UTILITA CARD
// FUNZIONI UTILITA PER LA LAVAGNA


/**
 * @brief funzione che permette di comparare le porte degli utenti in modo
 * da riordinarli
 */
int compare_users(const void *a, const void *b) {
    user_t *userA = (user_t *)a;
    user_t *userB = (user_t *)b;
    
    // Ritorna la differenza tra le porte
    return (userA->port - userB->port);
}

/***
 * Funzione che genera una nuova card. 
 * @param testo_attivita descrive l'attività relativa alla card.
 * @param utente_creatore utente che ha creato la card.
 * @return il puntatore alla nuova card o null in caso non sia stato possibile crearla
 */
card_t * card_create(const char testo_attivita[], uint16_t utente_creatore) {
    card_t *nuova_card = (card_t *)malloc(sizeof(card_t));
    if (nuova_card == NULL) {
        return NULL; // Gestione errore allocazione memoria
    }
    nuova_card->id = get_new_id(); // Da decidere: utilizzare variabile globale o meno ?
    nuova_card->colonna = TO_DO;
    strncpy(nuova_card->testo_attivita, testo_attivita, sizeof(nuova_card->testo_attivita));
    nuova_card->testo_attivita[sizeof(nuova_card->testo_attivita) - 1] = '\0'; 
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
    if (card == NULL) {
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



/**
 * @brief funzione che permette di inizializzare la lavagna del kanban
 */
bool_t lavagna_init(){
    for (int i = 0; i < MAX_COLUMN; i++) {
        lavagna.cards[i] = NULL;
        pthread_mutex_init(&lavagna.sem_cards[i], NULL);
    }

    pthread_mutex_init(&lavagna.conn_user_sem, NULL);
    lavagna.connected_users = 0;
    memset(lavagna.utenti_registrati, 0, sizeof(lavagna.utenti_registrati));

    return TRUE;
}

/**
 * @brief logica della funzione move_card_to_head
 * @attention questa funzione non acquisisce lock di alcun tipo
 */
void _lavagna_move_card_to_head(card_t* card, colonna_t col){
    if (card == NULL) return;
    card->next_card = lavagna.cards[col];
    lavagna.cards[col] = card;
    card->colonna = col;
}

void lavagna_move_card_to_head(card_t* card, colonna_t col){
    pthread_mutex_lock(&lavagna.sem_cards[col]);
    _lavagna_move_card_to_head(card, col);
    pthread_mutex_unlock(&lavagna.sem_cards[col]);
}


/**
 * @brief funzione che crea una nuova card e la aggiunge in testa a una lista
 * 
 * @param testo_attivita descrive l'attività relativa alla card.
 * @param utente_creatore utente che ha creato la card.
 *  
 * @return ritorna 0 in caso di successo, ritorna 1 in caso di errore.
 */
bool_t lavagna_card_add(const char* testo_attivita, uint16_t utente_creatore){
    if (testo_attivita == NULL) {
        printf("ERRORE: Il testo deve contenere qualcosa\n");
        return FALSE;
    }

    card_t* card = card_create(testo_attivita, utente_creatore);

    lavagna_move_card_to_head(card, 0);
    return TRUE;  
}


/**
 * @brief Funzione che permette di rimuovere una card passando l'ID e la lista
 * in cui si trova
 * 
 * @attention questa funzione non acquisce alcun lock.
 * 
 * @param id: identificativo della card che si vuole eliminare.
 * @param queue: riferimento al primo elemento della lista dalla quale si vuole eliminare l'elemento
 * @return la card rimossa se la funzione è terminata correttamente, NULL altrimenti.
 */
card_t* _remove_card(id_t id, colonna_t list){
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
            //pthread_mutex_unlock(&lavagna.sem_cards[list]);
            return curr;
        }
        prev = curr;
        curr = curr->next_card;
    }
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
    if (list < 0 || list >= MAX_COLUMN) {
        printf("ERRORE: L'inserimento deve essere fatto in una lista corretta\n");
        return NULL;
    }
    
    pthread_mutex_lock(&lavagna.sem_cards[list]);
    card_t* card = _remove_card_unsafe(id, list);
    pthread_mutex_unlock(&lavagna.sem_cards[list]);
    return card;
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
    if (src == dest) return; // Nessuno spostamento necessario

    int first_lock = (src < dest) ? src:dest;
    int second_lock = (src < dest) ? dest:src;

    pthread_mutex_lock(&lavagna.sem_cards[first_lock]);
    pthread_mutex_lock(&lavagna.sem_cards[second_lock]);

    card_t* curr = lavagna.cards[src];
    card_t* prev = NULL;
    card_t* target = NULL;

    while (curr != NULL) {
        if (curr->id == id) {
            target = curr;
            if (prev == NULL) {
                lavagna.cards[src] = curr->next_card;
            } else {
                prev->next_card = curr->next_card;
            }
            break;
        }
        prev = curr;
        curr = curr->next_card;
    }

    if (target != NULL) {

        target->colonna = (colonna_t)dest;
        target->ultimo_aggiornamento = time(NULL);

        if (dest == 0) {
            target->utente_assegnatario = 0;
        }

        target->next_card = lavagna.cards[dest];
        lavagna.cards[dest] = target;
        
        printf(VERDE "[LOG] Card #%d spostata da %d a %d" RESET "\n", id, src, dest);
    } else {
        printf(ROSSO "[ERRORE] Card #%d non trovata nella colonna %d" RESET "\n", id, src);
    }

    pthread_mutex_unlock(&lavagna.sem_cards[second_lock]);
    pthread_mutex_unlock(&lavagna.sem_cards[first_lock]);
}


void lavagna_stampa(char* buf, size_t max_len){
    for (int i = 0; i < 3; i++) {
        pthread_mutex_lock(&lavagna.sem_cards[i]);
    }
    

    size_t used = 0;
    int written;

    // pulizia del buffer
    memset(buf, 0, max_len);

    written = snprintf(buf + used, max_len - used, 
        "\n" BIANCO "╔══════════════════════════════════════════════════════╗" RESET "\n"
             BIANCO "║                   " GRASSETTO "KANBAN BOARD" RESET BIANCO "                      ║" RESET "\n"
             BIANCO "╚══════════════════════════════════════════════════════╝" RESET "\n\n");
    used += written;

    
    const char* titoli[] = {"TO DO (Da fare)", "DOING (In corso)", "DONE (Completato)"};
    const char* colori[] = {ROSSO, GIALLO, VERDE};
    
    for (int i = 0; i < MAX_COLUMN; i++) {
        // Intestazione Colonna
        written = snprintf(buf + used, max_len - used, 
            "%s" GRASSETTO " [%s] " RESET "\n", 
            colori[i], titoli[i]);
        used += written;

        // Separatore sotto il titolo
        written = snprintf(buf + used, max_len - used, 
            "%s" "--------------------------------------------------------" RESET "\n", 
            colori[i]);
        used += written;

        card_t* curr = lavagna.cards[i];
        
        if (curr == NULL) {
            written = snprintf(buf + used, max_len - used, "  (Nessuna attività in questa colonna)\n\n");
            used += written;
        }

        // --- CICLO SULLE CARD DELLA COLONNA ---
        while (curr != NULL) {
            // Determina quale utente mostrare:
            // In TO DO: mostriamo chi l'ha creata (Creatore)
            // In DOING/DONE: mostriamo chi la sta facendo/fatta (Assegnatario)
            uint16_t utente_rif = (i == 0) ? curr->utente_creatore : curr->utente_assegnatario;
            char etichetta_utente[20];
            
            if (i == 0) strcpy(etichetta_utente, "Creato da");
            else if (i == 1) strcpy(etichetta_utente, "In carico a");
            else strcpy(etichetta_utente, "Finito da");

            // Stampa della CARD come un box
            written = snprintf(buf + used, max_len - used,
                "  +--------------------------------------------------+\n"
                "  | " GRASSETTO "ID: %-4d" RESET " | %-12s: " CIANO "%-5d" RESET "       |\n"
                "  |--------------------------------------------------|\n"
                "  | %-48.48s |\n"  // Taglia il testo a 48 caratteri per non rompere il box
                "  +--------------------------------------------------+\n",
                curr->id, 
                etichetta_utente, 
                utente_rif,
                curr->testo_attivita
            );
            
            // Controllo buffer overflow
            if (written < 0 || written >= max_len - used) break;
            used += written;

            curr = curr->next_card;
        }
        
        // Spazio extra tra le colonne
        written = snprintf(buf + used, max_len - used, "\n");
        used += written;
    }
    
    for (int i = 0; i < 3; i++) {
        pthread_mutex_unlock(&lavagna.sem_cards[i]);
    }

}


/**
 * @brief Funzione che permette all'utente di registrarsi alla lavagna
 * 
 * @param port la porta del socket su cui l'utente vuole registrarsi
 * @return ritorna 0 in caso di successo, ritorna 1 in caso di errore.
*/
bool_t lavagna_hello(uint16_t port){
    // la funzione prende entrambi il lock sul semaforo per gli utenti connessi
    pthread_mutex_lock(&lavagna.conn_user_sem);

    // Se ci sono troppi utenti registrati la funzione non va a buon fine
    if (lavagna.connected_users == MAX_USER) {
        pthread_mutex_unlock(&lavagna.conn_user_sem);
        printf(ROSSO "[ERRORE] Numero massimo di utenti raggiunto, attendere che qualcuno abbandoni e riprovare" RESET "\n");
        return FALSE;
    }
    
    // Si crea la connessione Server --> Client
    struct sockaddr_in async_addr;
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&async_addr, 0, sizeof(async_addr)); 
    async_addr.sin_family = AF_INET;
    async_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &async_addr.sin_addr);

    int ret = connect(sd, (struct sockaddr*)&async_addr, sizeof(async_addr));
    if (ret < 0) {
        close(sd);
        printf(ROSSO "[ERRORE] Non è stato possibile avviare una connessione con l'host %hd" RESET "\n", port);
        return FALSE;
    }
    

    // Fine della connessione

    lavagna.utenti_registrati[lavagna.connected_users].port = port;
    lavagna.utenti_registrati[lavagna.connected_users].sock_id = sd;
    lavagna.utenti_registrati[lavagna.connected_users].id = 0;
    lavagna.utenti_registrati[lavagna.connected_users].last_ping = 0;

    lavagna.connected_users++;

    // funzione di sorting 
    qsort(
        lavagna.utenti_registrati, 
        lavagna.connected_users, 
        sizeof(user_t), 
        compare_users
    );

    pthread_mutex_unlock(&lavagna.conn_user_sem);   
    return TRUE;
}

/**
 * @brief funzione che permette all'utente di uscire dalla kanbar. Se l'utente aveva delle carte
 * nella colonna doing allora tornano in todo.
 * 
 * @param port la porta alla quale l'utente si era registrato
 * return ritorna 0 in caso di successo, ritorna 1 in caso di errore.
 */
bool_t lavagna_quit(uint16_t port){
    pthread_mutex_lock(&lavagna.conn_user_sem);

    int found = -1;
    for (int i = 0; i < lavagna.connected_users; i++) {
        if (lavagna.utenti_registrati[i].port == port) {
            found = i;
            break;
        }
    }    

    if (found != -1) {
        close(lavagna.utenti_registrati[found].sock_id);

        for (int i = found; i < lavagna.connected_users - 1; i++) {
            lavagna.utenti_registrati[i] = lavagna.utenti_registrati[i + 1];
        }

        lavagna.connected_users--;
        lavagna.utenti_registrati[lavagna.connected_users].port = 0;
        printf(GIALLO "[LOG] Utente %d rimosso correttamente." RESET "\n", port);
    }
    pthread_mutex_unlock(&lavagna.conn_user_sem);

    // verifica sulla presenza di card doing con sincronizzazione corretta
    pthread_mutex_lock(&lavagna.sem_cards[0]); 
    pthread_mutex_lock(&lavagna.sem_cards[1]);
    card_t* curr = lavagna.cards[1];
    card_t* prev = NULL;

    while (curr != NULL) {
        if (curr->utente_assegnatario == port) {
            card_t* card_to_move = curr;

            if (prev == NULL) {
                lavagna.cards[1] = curr->next_card;
            } else {
                prev->next_card = curr->next_card;
            }           

            card_to_move->utente_assegnatario = 0;
            card_to_move->ultimo_aggiornamento = time(NULL);
            card_to_move->next_card = lavagna.cards[0];
            lavagna.cards[0] = card_to_move;

            printf(VERDE "[LOG] Card #%d riportata in TO DO" RESET "\n", card_to_move->id);
            break;
        } 
        prev = curr;
        curr = curr->next_card;
    }

    pthread_mutex_unlock(&lavagna.sem_cards[1]);
    pthread_mutex_unlock(&lavagna.sem_cards[0]);
    return TRUE;
}


/**
 * @brief funzione che verifica se una particolare porta, cioè un utente, è già 
 * registrato all'interno del sistema.
 * 
 * @param port porta di cui si vuole verificare la registrazione
 */
bool_t lavagna_is_user_registerd(uint16_t port){
    bool_t res = FALSE;
    pthread_mutex_lock(&lavagna.conn_user_sem);
    for (int i = 0; i < MAX_USER; i++) {
        res = (lavagna.utenti_registrati[i].port == port) ? TRUE: res;
    }
    pthread_mutex_unlock(&lavagna.conn_user_sem);
    return res;
}

/**
 * @brief Restituisce una stringa contenente la lista degli utenti connessi
 * 
 * @param buffer Buffer dove scrivere la lista
 * @param max_len Dimensione massima del buffer
 * @return TRUE se l'operazione è riuscita, FALSE altrimenti
 */

int lavagna_user_list(char* buf, size_t max_len){
    pthread_mutex_lock(&lavagna.conn_user_sem);

    size_t used = 0;
    int written;

    // Pulizia del buffer
    memset(buf, 0, max_len);

    // --- INTESTAZIONE TABELLA ---
    written = snprintf(buf + used, max_len - used,
        "\n" BLU "╔══════════════════════════════════════════════════╗" RESET "\n"
             BLU "║" RESET "                " GRASSETTO "UTENTI CONNESSI" RESET "                   " BLU "║" RESET "\n"
             BLU "╠══════════════╦═══════════════════════════════════╣" RESET "\n"
             BLU "║" RESET GRASSETTO "    PORTA     " RESET BLU "║" RESET GRASSETTO "            STATO              " RESET BLU "    ║" RESET "\n"
             BLU "╠══════════════╬═══════════════════════════════════╣" RESET "\n");
    used += written;

    int count = 0;
    for (int i = 0; i < MAX_USER; i++) {
        if (lavagna.utenti_registrati[i].port != 0) {
            count++;
            char stato[40];
            const char* colore_stato;

            // Determiniamo lo stato dell'utente
            if (lavagna.utenti_registrati[i].id == 0) {
                // Utente registrato ma senza card assegnata
                colore_stato = VERDE;
                snprintf(stato, sizeof(stato), "Libero (Pronto)");
            } else {
                // Utente che ha una card in "Doing" [cite: 37]
                colore_stato = GIALLO;
                snprintf(stato, sizeof(stato), "Lavorando su Card #%d", lavagna.utenti_registrati[i].id);
            }

            // Stampa della riga utente
            written = snprintf(buf + used, max_len - used,
                "║    %-8d  ║ %s%-31.31s " RESET BLU "  ║" RESET "\n",
                lavagna.utenti_registrati[i].port,
                colore_stato,
                stato
            );

            if (written < 0 || written >= max_len - used) break;
            used += written; 
        }
    }

    if (count == 0) {
        written = snprintf(buf + used, max_len - used,
            "║              (Nessun utente connesso)            ║\n");
        used += written;
    }

    // --- CHIUSURA TABELLA ---
    written = snprintf(buf + used, max_len - used,
        BLU "╚══════════════╩═══════════════════════════════════╝" RESET "\n");
    used += written;
    
    pthread_mutex_unlock(&lavagna.conn_user_sem);
    return (int)used;
}

// TODO --> Sostituire int ID con uint16_t id
card_t* lavagna_trova_card_per_id(int id){
    card_t* list = lavagna.cards[1];
    while (list != NULL) {
        if (list->utente_assegnatario == id) {
            return list;
        }
        list = list->next_card;
    }
    return NULL;
}


int lavagna_user_list_to_vec(uint16_t* buf){
    pthread_mutex_lock(&lavagna.conn_user_sem);

    for (int i = 0; i < lavagna.connected_users; i++) {
        buf[i] = lavagna.utenti_registrati[i].port;
    }
    
    int len = lavagna.connected_users;
    pthread_mutex_unlock(&lavagna.conn_user_sem);
    return len;
}
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
 * @param id: identificativo della card che si vuole eliminare.
 * @param queue: riferimento al primo elemento della lista dalla quale si vuole eliminare l'elemento
 * @return la card rimossa se la funzione è terminata correttamente, NULL altrimenti.
 */
card_t* lavagna_card_remove(id_t id, id_t list) {
    if (list < 0 || list >= MAX_COLUMN) {
        printf("ERRORE: L'inserimento deve essere fatto in una lista corretta\n");
        return NULL;
    }
    
    //pthread_mutex_lock(&lavagna.sem_cards[list]);
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

    //pthread_mutex_unlock(&lavagna.sem_cards[list]);
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

    pthread_mutex_lock(&lavagna.sem_cards[src]);
    card_t* card = lavagna_card_remove(id, src);
    card_state_change(card, dest);    
    lavagna_move_card_to_head(card, dest);
    pthread_mutex_unlock(&lavagna.sem_cards[src]);

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
             BIANCO "║                   " GRASSETTO "KANBAN BOARD" RESET BIANCO "                     ║" RESET "\n"
             BIANCO "╚══════════════════════════════════════════════════════╝" RESET "\n\n");
    used += written;

    // concatenazione messaggio
    written = snprintf(
        buf + used, 
        max_len - used, 
        "|------------------------------|\n|-------------TODO-------------|\n|------------------------------|\n"
    );

    used += written;

    card_t* list = lavagna.cards[0];
    while (list != NULL) {
        written = snprintf(
            buf + used,
            max_len -used,
            "  - %s",
            list->testo_attivita
        );
        used += written;
        list = list->next_card;
    }

    written = snprintf(
        buf + used, 
        max_len - used, 
        "|------------------------------|\n|-------------DOING------------|\n|------------------------------|\n"
    );

    used += written;

    list = lavagna.cards[1];
    while (list != NULL) {
        written = snprintf(
            buf + used,
            max_len -used,
            "  - %s",
            list->testo_attivita
        );
        used += written;
        list = list->next_card;
    }

    written = snprintf(
        buf + used, 
        max_len - used, 
        "|------------------------------|\n|-------------DONE-------------|\n|------------------------------|\n"
    );

    used += written;

    list = lavagna.cards[2];
    while (list != NULL) {
        written = snprintf(
            buf + used,
            max_len -used,
            "  - %s",
            list->testo_attivita
        );
        used += written;
        list = list->next_card;
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

    int i, found = -1;
    for (i = 0; i < lavagna.connected_users; i++) {
        if (lavagna.utenti_registrati[i].port == port) {
            found = i;
            break;
        }
    }    

    if (found != -1) {
        for (i = found; i < lavagna.connected_users - 1; i++) {
            lavagna.utenti_registrati[i] = lavagna.utenti_registrati[i + 1];
        }

        lavagna.connected_users--;
        lavagna.utenti_registrati[lavagna.connected_users].port = 0;
    }
    

    pthread_mutex_unlock(&lavagna.conn_user_sem);

    // verifica sulla presenza di card doing con sincronizzazione corretta
    pthread_mutex_lock(&lavagna.sem_cards[1]);
    card_t* list = lavagna.cards[1];

    while (list != NULL) {
        if (list->utente_assegnatario == port) {
            card_t* card_to_move = lavagna_card_remove(list->id, 1);

            pthread_mutex_unlock(&lavagna.sem_cards[1]);

            card_to_move->utente_assegnatario = 0;
            lavagna_move_card_to_head(card_to_move, 0);

            pthread_mutex_lock(&lavagna.sem_cards[1]);

            list = lavagna.cards[1];
        } else {
            list = list->next_card;
        }
    }

    

    pthread_mutex_unlock(&lavagna.sem_cards[1]);
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

    // lock sul semaforo degli utenti connessi
    pthread_mutex_lock(&lavagna.conn_user_sem);

    size_t used = 0;
    int written;

    // pulizia del buffer
    memset(buf, 0, max_len);

    // concatenazione messaggio
    written = snprintf(
        buf, 
        max_len, 
        "|-------------------------|\n|--- %s ---|\n|-------------------------|\n", 
        "UTENTI REGISTRATI"
    );

    if (written < 0) {
        pthread_mutex_unlock(&lavagna.conn_user_sem);
        printf(ROSSO "[ERRORE] Non è stato possibile stampare la lista degli utenti" RESET "\n");
        return -1;
    }

    used = written;

    for (int i = 0; i < MAX_USER; i++) {
        if (lavagna.utenti_registrati[i].port != 0) {
            written = snprintf(
                buf + used,
                max_len - used,
                "|---       %d        ---|\n",
                lavagna.utenti_registrati[i].port
            );

            if (written < 0 || written >= max_len - used)
                break;
            
            used += written;   
        }
    }
    
    pthread_mutex_unlock(&lavagna.conn_user_sem);
    return used;
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
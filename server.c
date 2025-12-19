#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <utilities/lavagna.h>
#include <string.h>

void* manage_request(void* arg);
void* card_handler(void* arg);

int main(){
    // inizializzazione della lavagna
    lavagna_init();

    // creazione del socket per la gestione dell'assegnamento delle card
    pthread_t scheduler_tid;
    pthread_create(&scheduler_tid, NULL, card_handler, NULL);
    pthread_detach(scheduler_tid);

    int ret, sd;
    socklen_t len;
    
    struct sockaddr_in my_addr, cl_addr;

    sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(5678);
    inet_pton(AF_INET, "127.0.0.1", &my_addr.sin_addr);

    ret = bind(sd, (struct sockaddr*) &my_addr, sizeof(my_addr));
    if (ret < 0) {
        perror("bind failed");
        return 1;
    }
    ret = listen(sd, 20);
    if (ret < 0) {
        perror("listen failed");
        return 1;
    }
    len = sizeof(cl_addr);   

    for (;;) {    
        // si accetta una richiesta
        int new_sd = accept(sd, (struct sockaddr*)&cl_addr, &len);
        if (new_sd < 0) {
            perror("accept failed");
            continue;
        }

        // Occorre capire se l'utente è registrato
        pthread_t t_id;

        pthread_create(&t_id, NULL, manage_request, (void*)(intptr_t) new_sd);
        pthread_detach(t_id);     

    }
    
}


/**
 * @attention I comandi disponibili sono: CREATE_CARD, SHOW_USR_LIST e SHOW_LAVAGNA, QUIT
 */
void* manage_request(void* arg){
    // va gestita la richiesta in funzione di quello che l'utente
    // chiede
    int user_sd = (int)(intptr_t) arg;
    char buf[MAX_BUF_SIZE], out_buf[MAX_BUF_SIZE];

    ssize_t size = recv(user_sd, buf, MAX_BUF_SIZE-1, 0);
    if (size <= 0) {
        close(user_sd);
        pthread_exit(NULL);
    }
    buf[size] = '\0';
    uint16_t port = atoi(buf);
    if (port == 0) {
        close(user_sd);
        pthread_exit(NULL);
    }
    lavagna_hello(port, user_sd);
    send(user_sd, "ok", strlen("ok") + 1, 0);
    printf("registrato utente alla porta %d\n", port);

    
    
    while (1) {        
        ssize_t size;
        size = recv(user_sd, buf, MAX_BUF_SIZE - 1, 0); 
        if (size <= 0) {
            lavagna_quit(port);
            close(user_sd);
            pthread_exit(NULL);
        }
        buf[size] = '\0';

        // GESTIRE LA LOGICA DEI COMANDI
        
        if (strcmp(buf, "CARD_CREATE\n") == 0) {
            memset(buf, 0, sizeof(buf));
            send(user_sd, "> INSERISCI DESCRIZIONE PER LA CARD [massimo 255 caratteri]", 60, 0);
            size = recv(user_sd, buf, MAX_BUF_SIZE - 1, 0);
            if (size <= 0) {
                printf("Errore ricezione descrizione card o connessione chiusa\n");
                lavagna_quit(port);
                close(user_sd);
                pthread_exit(NULL);
            }
            buf[size] = '\0';
            
            lavagna_card_add(buf, port);
            printf("creata card con testo %s", buf);
            send(user_sd, "CARD CREATA CON SUCCESSO!\n", 27, 0);
        } else if (strcmp(buf, "SHOW_USR_LIST\n") == 0) {
            lavagna_user_list(out_buf, MAX_BUF_SIZE);
            send(user_sd, out_buf, MAX_BUF_SIZE-1, 0);
        } else if (strcmp(buf, "SHOW_LAVAGNA\n") == 0) {
            char lavagna_buf[MAX_SBUF_SIZE];
            lavagna_stampa(lavagna_buf, MAX_SBUF_SIZE);
            send(user_sd, lavagna_buf, MAX_SBUF_SIZE-1, 0);
        } else if (strcmp(buf, "QUIT\n") == 0) {
            lavagna_quit(port);
            send(user_sd, "CANCELLAZIONE AVVENUTA CON SUCCESSO\n\0", 37 , 0);
            close(user_sd);
            pthread_exit(0);
        } else {
            send(user_sd, "ERRORE: Comando non valido!\n\0", 29 , 0);
        }

        memset(buf, 0, sizeof(buf));
    }
    
}


/**
 * @brief thread per la gestione delle card. Ogni k secondi il thread controlla se è possibile assegnare
 * delle nuove card a qualche utente, inoltre controlla se il tempo passato dall'ultimo assegnamento di una
 * particolare card supera un certo limite superiore e in tal caso manda un PING al client
 */
void* card_handler(void* arg){
    int port = 0;
    while (1) {
        // ASSEGNAMENTO DELLE CARD
        pthread_mutex_lock(&lavagna.sem_cards[0]);
        for (card_t* card = &lavagna.cards[0]; card != NULL; card = card->next_card) {
            if (card->utente_assegnatario == 0) {
                // Si deve cercare un utente libero
            }
            
        }
        

        // INCREMENTO DEI TIMER

        // NOTIFICA PING
        printf("Sto per andare a dormire\n");
        sleep(5);
    }
    
}
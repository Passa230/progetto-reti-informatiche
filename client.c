#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <generic.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <structure.h>


pthread_mutex_t queue_mutex;
notifica_t n_queue[MAX_NOTIFICATIONS];
int head = 0, tail = 0, count = 0;

void enqueue(char* nuovo_msg) {
    pthread_mutex_lock(&queue_mutex);
    if (count < MAX_NOTIFICATIONS) {
        strncpy(n_queue[tail].messagge, nuovo_msg, MAX_BUF_SIZE - 1);
        n_queue[tail].messagge[MAX_BUF_SIZE - 1] = '\0'; 

        tail = (tail + 1) % MAX_NOTIFICATIONS;
        count++;
    }
    pthread_mutex_unlock(&queue_mutex);
}


void* client_listener(void* arg);

int main(int argc, char **argv){
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    // Blocco della possibilità di fare CTRL + C all'utente
    // signal(SIGINT, SIG_IGN);
    uint16_t user_buf[MAX_USER];
    int ret, sd, user_len;
    struct sockaddr_in sv_addr;
    char buf[MAX_BUF_SIZE], lavagna_buf[MAX_SBUF_SIZE];
    char in_buf[MAX_BUF_SIZE];
    ssize_t size;

    if (atoi(argv[1]) < 5678) {
        printf("ERRORE: Non è possibile registrarsi a questa porta\n");        
        return 0;
    }

    pthread_mutex_init(&queue_mutex, NULL);
    pthread_t t_listener;
    pthread_create(&t_listener, NULL, client_listener, argv[1]);


    sd = socket(AF_INET, SOCK_STREAM, 0);
    printf("Qui ci arrivo\n");
    
    memset(&sv_addr, 0, sizeof(sv_addr)); 
    sv_addr.sin_family = AF_INET ;        
    sv_addr.sin_port = htons(5678);
    inet_pton(AF_INET, "127.0.0.1", &sv_addr.sin_addr);
    
    ret = connect(sd, (struct sockaddr*)&sv_addr, sizeof(sv_addr));
    size = send(sd, argv[1], strlen(argv[1]) + 1, 0);    // si aspetta la conferma della registrazione
    // printf("Qui ci arrivo\n");
    
    size = recv(sd, buf, MAX_BUF_SIZE-1, 0);
    if (size <= 0) {
        printf("Errore di ricezione o connessione chiusa\n");
        close(sd);
        return 1;
    }
    buf[size] = '\0';

    if (strcmp(buf, "ok") == 0) {
        printf("Registrazione avvenuta con successo\n");
    } else {
        printf("Errore di registrazione\n");
        close(sd);
        return 1;
    }    
    
    uint16_t connessione_attiva = 1;
    while (connessione_attiva == 1) {
        pthread_mutex_lock(&queue_mutex);
        while (count > 0) {
            printf("\n[NOTIFICA] %s\n", n_queue[head].messagge);
            head = (head + 1) % MAX_NOTIFICATIONS;
            count--;
        }
        pthread_mutex_unlock(&queue_mutex);
        
        printf(">>> ");
        fgets(in_buf, sizeof(in_buf), stdin);

        size = send(sd, in_buf, strlen(in_buf) + 1, 0);
        // si attende la risposta dal server 

        if (size <= 0) {
            printf("Connessione chiusa dal server\n");
            break;
        }


        if (strcmp(in_buf, "CARD_CREATE\n") == 0) {
            size = recv(sd, buf, MAX_BUF_SIZE, 0);
            printf("%s\n", buf);
            printf("> ");

            memset(in_buf, 0, sizeof(in_buf));
            fgets(in_buf, sizeof(in_buf), stdin);
            size = send(sd, in_buf, strlen(in_buf) + 1, 0);
            if (size <= 0) {
                printf("Connessione chiusa dal server\n");
                break;
            }
            size = recv(sd, buf, MAX_BUF_SIZE, 0);
            printf("%s\n", buf);
        } else if (strcmp(in_buf, "REVIEW_CARD\n") == 0) {
            // Da gestire la revisione, cioè far comunicare all'utente 
            /**
             * For che per ogni utente connesso invia UDP un messaggio
             * di richiesta approvazione.
             * 
             * Quando riceve una risposta del tipo "REVIEW_ACCEPT" allora 
             * manda al server un segnale del tipo "CARD_DONE"
             * 
             * I
             */
        
        } else if (strcmp(in_buf, "SHOW_USR_LIST\n") == 0){
            size = recv(sd, buf, MAX_BUF_SIZE, 0);
            printf("%s\n", buf);
            recv(sd, &user_len, sizeof(user_len), 0);
            int num_utenti = ntohl(user_len);
            recv(sd, user_buf, sizeof(uint16_t) * num_utenti, 0);
            printf("> [DATI] Utenti disponibile: %d", num_utenti);
            for (int i = 0; i < num_utenti; i++) {
                uint16_t porta_corretta = ntohs(user_buf[i]); // <--- CONVERSIONE QUI
                printf("  - Utente sulla porta: %d\n", porta_corretta);
                user_buf[i] = porta_corretta;
            }
        } else if (strcmp(in_buf, "SHOW_LAVAGNA\n") == 0) {
            size = recv(sd, lavagna_buf, MAX_SBUF_SIZE, 0);
            printf("%s\n", lavagna_buf);
        } else if(strcmp(in_buf, "QUIT\n") == 0){
            return 0;
        }

    }
    
}


/**
 * @todo aggiungere un card ACK per accettare l'assegnamento della card
 */
void* client_listener(void* arg){
    int port = atoi((char *)arg), ret, len;
    char buf[MAX_BUF_SIZE], async_buffer[MAX_NOT_BUF_SIZE];


    struct sockaddr_in async_addr, async_addr_to_server;
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&async_addr, 0, sizeof(async_addr)); 
    async_addr.sin_family = AF_INET;
    async_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &async_addr.sin_addr);


    ret = bind(sd, (struct sockaddr*) &async_addr, sizeof(async_addr));
    if (ret < 0) {
        perror("bind failed");
        close(sd);
        pthread_exit(NULL);
    }
    ret = listen(sd, 20);
    if (ret < 0) {
        perror("listen failed");
        close(sd);
        pthread_exit(NULL);
    }
    len = sizeof(async_addr);

    int new_sd;
    while ((new_sd = accept(sd, (struct sockaddr*) &async_addr_to_server, &len)) < 0) {
        perror("accept");
    }

    // Si deve mettere in ascolto sulla porta client
    while (1) {
        
        ssize_t size = recv(new_sd, buf, MAX_BUF_SIZE - 1, 0);
        if (size <= 0) break;
        buf[size] = '\0';

        int id; char testo[MAX_BUF_SIZE];

        if (strcmp(buf, "PING_USER") == 0) {
            send(new_sd, "PONG_LAVAGNA", 13, 0);
        } else if (sscanf(buf, "ASYNC: HANDLE_CARD %d %[^\n]", &id, testo) == 2){
            // Crea una send di ACK_CARD
            sprintf(async_buffer, "Card assegnata #%d: %s\n", id, testo);
            enqueue(async_buffer);
            memset(async_buffer, 0, sizeof(async_buffer));
        } else if (strcmp(buf, "REWEIUE")){

        }

    }
    
}
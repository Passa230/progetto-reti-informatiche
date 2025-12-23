#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
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

        if (strcmp(in_buf, "REVIEW_CARD\n") != 0){
            size = send(sd, in_buf, strlen(in_buf) + 1, 0);

            if (size <= 0) {
                printf("Connessione chiusa dal server\n");
                break;
            }
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
            if (user_len <= 0){
                printf("[ERRORE] Richiedi gli utenti prima di revisionare la card\n");
                continue;
            } else if (user_len == 1){
                printf("[ERRORE] Non ci sono abbastanza utenti, attendi che qualcuno si registri e riprova\n");
                continue;
            }

            int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (udp_sock < 0) {
                perror("Errore creazione socket UDP per invio");
                continue;
            }

            snprintf(in_buf, sizeof(in_buf), "REVIEW: L'utente sulla porta %s richiede revisione!", argv[1]);
            int sent_count = 0;

            for (int i = 0; i < user_len; i++) {
                if (user_buf[i] == atoi(argv[1])) continue;
                struct sockaddr_in dest_addr;
                memset(&dest_addr, 0, sizeof(dest_addr));
                dest_addr.sin_family = AF_INET;
                dest_addr.sin_port = htons(user_buf[i]); // Porta di destinazione
                inet_pton(AF_INET, "127.0.0.1", &dest_addr.sin_addr);

                
                ssize_t sent = sendto(udp_sock, in_buf, strlen(in_buf), 0, 
                              (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                
                if (sent < 0) {
                    perror("Errore invio UDP\n");
                } else {
                    sent_count++;
                }
                              
            }

            close(udp_sock);
            memset(user_buf, 0, sizeof(user_buf));
            user_len = 0;
            
        } else if (strcmp(in_buf, "SHOW_USR_LIST\n") == 0){
            int list_len = 0;
            size = recv(sd, &list_len, sizeof(list_len), 0);
            list_len = ntohl(list_len);

            memset(buf, 0, MAX_BUF_SIZE);
            size = recv(sd, buf, list_len, 0);
            printf("%s\n", buf);


            uint32_t net_len;
            recv(sd, &net_len, sizeof(uint32_t), 0);
            user_len = ntohl(net_len);
            printf("> [DATI] Utenti disponibile: %d\n", user_len);
            
            if (user_len > 0) {
                recv(sd, user_buf, sizeof(uint16_t) * user_len, 0);
                
                for (int i = 0; i < user_len; i++) {
                    uint16_t porta_corretta = ntohs(user_buf[i]); // <--- CONVERSIONE QUI
                    //printf("  - Utente sulla porta: %d\n", porta_corretta);
                    user_buf[i] = porta_corretta;
                }
            }
        } else if (strcmp(in_buf, "SHOW_LAVAGNA\n") == 0) {
            size = recv(sd, lavagna_buf, MAX_SBUF_SIZE, 0);
            printf("%s\n", lavagna_buf);
        } else if(strcmp(in_buf, "QUIT\n") == 0){
            return 0;
        } else {
            size = recv(sd, buf, MAX_BUF_SIZE, 0);
            printf("%s\n", buf);
        }

    }
    
}


/**
 * @todo aggiungere un card ACK per accettare l'assegnamento della card
 * Modifica, permettere al socket di accettare sia connessioni TCP che UDP
 */
void* client_listener(void* arg){
    int port = atoi((char *)arg), ret, len;
    char buf[MAX_BUF_SIZE], async_buffer[MAX_NOT_BUF_SIZE];
    int tcp_sd, udp_sd, max_sd, server_sd;
    fd_set read_fds;
    struct sockaddr_in async_addr, async_addr_to_server;
    
    // -----------------------------------------------------------
    // PREPARAZIONE SOCKET TCP (Per accogliere il Server)
    // -----------------------------------------------------------
    server_sd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&async_addr, 0, sizeof(async_addr)); 
    async_addr.sin_family = AF_INET;
    async_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &async_addr.sin_addr);


    ret = bind(server_sd, (struct sockaddr*) &async_addr, sizeof(async_addr));
    if (ret < 0) {
        perror("bind failed");
        close(server_sd);
        pthread_exit(NULL);
    }
    ret = listen(server_sd, 20);
    if (ret < 0) {
        perror("listen failed");
        close(server_sd);
        pthread_exit(NULL);
    }
    len = sizeof(async_addr);

    while ((tcp_sd = accept(server_sd, (struct sockaddr*) &async_addr_to_server, &len)) < 0) {
        perror("accept");
    }


    close(server_sd);

    // -----------------------------------------------------------
    // PREPARAZIONE SOCKET UDP (Per REVIEW peer-to-peer)
    // -----------------------------------------------------------
    udp_sd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (bind(udp_sd, (struct sockaddr*) &async_addr, sizeof(async_addr)) < 0){
        close(udp_sd);
        close(tcp_sd);
        pthread_exit(NULL);
    }
    
    // Si deve mettere in ascolto sulla porta client
    while (1) {
        // pulizia del set di descrittore
        FD_ZERO(&read_fds);
        
        // si aggiunge il socket UDP
        FD_SET(udp_sd, &read_fds);
        FD_SET(tcp_sd, &read_fds);
        max_sd = (udp_sd > tcp_sd) ? udp_sd : tcp_sd;;

        int activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
            break;
        }

        if (FD_ISSET(udp_sd, &read_fds)) {
            ssize_t n = recvfrom(udp_sd, buf, sizeof(buf) - 1, 0, NULL, NULL);
            buf[n] = '\0';
            if (strncmp(buf, "REVIEW:", 7) == 0) {
                printf("\r\033[K"); // Cancella riga corrente
                printf("[REVIEW RICHIESTA] %s\n", buf);
                printf(">>> ");
                fflush(stdout);
            }
            memset(async_buffer, 0, sizeof(async_buffer));
        }
        
        if (FD_ISSET(tcp_sd, &read_fds)) {
            ssize_t n = recv(tcp_sd, buf, MAX_BUF_SIZE - 1, 0);
    
            if (n <= 0) {
                // Il server ha chiuso la connessione (o crash)
                printf("[ERRORE] Connessione notifiche persa col server.\n");
                close(tcp_sd);
                close(udp_sd);
                // Usciamo dal thread o gestiamo la chiusura del client
                pthread_exit(NULL); 
            }

            buf[n] = '\0';

            int id; char testo[MAX_BUF_SIZE];

            if (strcmp(buf, "PING_USER") == 0) {
                send(tcp_sd, "PONG_LAVAGNA", 13, 0);
            } else if (sscanf(buf, "ASYNC: HANDLE_CARD %d %[^\n]", &id, testo) == 2){
                // Crea una send di ACK_CARD
                sprintf(async_buffer, "Card assegnata #%d: %s\n", id, testo);
                //enqueue(async_buffer);
                printf("\r\033[K");
                printf("[NOTIFICA ASINCRONA] %s\n", buf);
                printf(">>> ");
                fflush(stdout);

                memset(async_buffer, 0, sizeof(async_buffer));
            }
        }

    }
    
}
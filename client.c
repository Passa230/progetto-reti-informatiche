#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include <include/generic.h>
#include <include/structure.h>
#include <include/color.h>






bool_t is_review_complete = FALSE;
uint32_t review_received = 0, review_needed = 0;
uint16_t voters[MAX_USER];
time_t review_start_time = 0;
pthread_mutex_t review_mutex;

id_t assigned_card_id = -1;
char assigned_card_text[MAX_BUF_SIZE];
pthread_mutex_t assigned_card_mutex;


bool_t has_already_voted(uint16_t port){
    for (int i = 0; i < review_received; i++) {
        if (voters[i] == port) return TRUE;
    }
    return FALSE;
}


void stampa_menu(){
    printf("\n" GIALLO "======================= " RESET GRASSETTO "KANBAN BOARD" RESET GIALLO " =======================" RESET "\n");

    printf(VERDE "  > %-15s" RESET " : %s\n", "CARD_CREATE",        "Crea una nuova card");
    printf(VERDE "  > %-15s" RESET " : %s\n", "SHOW_USR_LIST",      "Mostra lista utenti e le loro porte");
    printf(VERDE "  > %-15s" RESET " : %s\n", "REVIEW_CARD",        "Richiedi revisione della tua card");
    printf(VERDE "  > %-15s" RESET " : %s\n", "CARD_DONE",          "Segnala al server che la card è completa");
    printf(VERDE "  > %-15s" RESET " : %s\n", "SHOW_LAVAGNA",       "Visualizza lo stato della lavagna");
    printf(VERDE "  > %-15s" RESET " : %s\n", "SHOW_CARD",          "Visualizza la card assegnata all'utente");
    printf(VERDE "  > %-15s" RESET " : %s\n", "OKAY_REVIEW port",   "Approva la revisione all'utente 'port'");
    printf(VERDE "  > %-15s" RESET " : %s\n", "CLEAR",              "Pulisci lo schermo"); 
    printf(VERDE "  > %-15s" RESET " : %s\n", "QUIT",               "Chiudi il client ed esci");

    printf(GIALLO "==============================================================" RESET "\n\n");
}





void* client_listener(void* arg);

int main(int argc, char **argv){
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    // Blocco della possibilità di fare CTRL + C all'utente
    signal(SIGINT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    uint16_t user_buf[MAX_USER];
    int ret, sd, user_len;
    uint16_t review_port;
    struct sockaddr_in sv_addr;
    char buf[MAX_BUF_SIZE], lavagna_buf[MAX_SBUF_SIZE];
    char in_buf[MAX_BUF_SIZE];
    ssize_t size;

    if (atoi(argv[1]) < 5678) {
        printf(ROSSO "[ERRORE] Non è possibile registrarsi a questa porta" RESET "\n");        
        return 0;
    }

    pthread_mutex_init(&review_mutex, NULL);
    pthread_mutex_init(&assigned_card_mutex, NULL);
    pthread_t t_listener;
    pthread_create(&t_listener, NULL, client_listener, argv[1]);


    sd = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&sv_addr, 0, sizeof(sv_addr)); 
    sv_addr.sin_family = AF_INET ;        
    sv_addr.sin_port = htons(5678);
    inet_pton(AF_INET, "127.0.0.1", &sv_addr.sin_addr);
    
    ret = connect(sd, (struct sockaddr*)&sv_addr, sizeof(sv_addr));
    if (ret < 0) {
        printf(ROSSO "[ERRORE] Non è stato possibile connettersi al server" RESET "\n");
        close(sd);
        return 0;
    }
    
    size = send(sd, argv[1], strlen(argv[1]) + 1, 0);    
    // printf("Qui ci arrivo\n");
    
    size = recv(sd, buf, MAX_BUF_SIZE-1, 0);
    if (size <= 0) {
        printf(ROSSO "[ERRORE] di ricezione o connessione chiusa"RESET"\n");
        close(sd);
        return 1;
    }
    buf[size] = '\0';

    if (strcmp(buf, "ok") == 0) {
        printf(VERDE "[SUCCESS] Registrazione avvenuta con successo" RESET "\n");
        stampa_menu();
    } else {
        printf(ROSSO "[ERRORE] Mancata conferma di registrazione del server" RESET "\n");
        close(sd);
        return 1;
    }    
    
    uint16_t connessione_attiva = 1;
    while (connessione_attiva == 1) {        
        printf(">>> ");
        fgets(in_buf, sizeof(in_buf), stdin);     


        if (strcmp(in_buf, "CARD_CREATE\n") == 0) {
            size = send(sd, in_buf, strlen(in_buf) + 1, 0);
            if (size <= 0) break; 
            size = recv(sd, buf, MAX_BUF_SIZE, 0);
            printf("%s\n", buf);
            printf("> ");

            memset(in_buf, 0, sizeof(in_buf));
            fgets(in_buf, sizeof(in_buf), stdin);
            size = send(sd, in_buf, strlen(in_buf) + 1, 0);
            if (size <= 0) {
                printf(ROSSO "[ERRORE] Connessione chiusa dal server"RESET"\n");
                break;
            }
            size = recv(sd, buf, MAX_BUF_SIZE, 0);
            printf("%s\n", buf);
        } else if (strcmp(in_buf, "REVIEW_CARD\n") == 0) {
            if (user_len <= 0){
                printf(ROSSO "[ERRORE] Richiedi gli utenti prima di revisionare la card"RESET"\n");
                continue;
            } else if (user_len == 1){
                printf(ROSSO "[ERRORE] Non ci sono abbastanza utenti, attendi che qualcuno si registri e riprova"RESET"\n");
                continue;
            } else if (assigned_card_id == -1){
                printf(ROSSO "[ERRORE] Nessuna card assegnata all'utente" RESET);
                continue;
            }

            int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (udp_sock < 0) {
                perror("Errore creazione socket UDP per invio");
                continue;
            }

            pthread_mutex_lock(&review_mutex);
            review_received = 0;
            review_needed = user_len - 1;
            is_review_complete = FALSE;
            review_start_time = time(NULL); 
            memset(voters, 0, sizeof(voters));
            pthread_mutex_unlock(&review_mutex);

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
            review_needed = user_len-1;
            user_len = 0;
            
        } else if (strcmp(in_buf, "SHOW_USR_LIST\n") == 0){
            size = send(sd, in_buf, strlen(in_buf) + 1, 0);
            if (size <= 0) break; 
            uint32_t list_len = 0;
            size = recv(sd, &list_len, sizeof(list_len), 0);
            list_len = ntohl(list_len);

            memset(lavagna_buf, 0, MAX_SBUF_SIZE);
            size = recv(sd, lavagna_buf, list_len, 0);
            printf("%s\n", lavagna_buf);


            uint32_t net_len;
            recv(sd, &net_len, sizeof(uint32_t), 0);
            user_len = ntohl(net_len);
            printf("> [DATI] Utenti disponibile: %d\n", user_len);
            
            if (user_len > 0) {
                recv(sd, user_buf, sizeof(uint16_t) * user_len, 0);
                
                for (int i = 0; i < user_len; i++) {
                    uint16_t porta_corretta = ntohs(user_buf[i]); // <--- CONVERSIONE QUI
                    user_buf[i] = porta_corretta;
                }
            }
        } else if (strcmp(in_buf, "SHOW_LAVAGNA\n") == 0) {
            size = send(sd, in_buf, strlen(in_buf) + 1, 0);
            if (size <= 0) break; 
            uint32_t n_lavagna_size, lavagna_size;
            size = recv(sd, &n_lavagna_size, sizeof(uint32_t), 0);
            lavagna_size = ntohl(n_lavagna_size);

            int letti = 0;
            while (letti < lavagna_size) {
                int r = recv(sd, lavagna_buf + letti, lavagna_size - letti, 0);
                if (r <= 0) break;
                letti += r;
            }
            lavagna_buf[lavagna_size] = '\0';
            printf("%s\n", lavagna_buf);
        } else if(strcmp(in_buf, "QUIT\n") == 0){
            size = send(sd, in_buf, strlen(in_buf) + 1, 0);
            if (size <= 0) break; 
            return 0;
        } else if(strcmp(in_buf, "CARD_DONE\n") == 0){
            if(is_review_complete == FALSE){
                printf(ROSSO "[ERRORE] Attendi che almeno un altro client revisioni il lavoro\n" RESET);
                continue;
            }
            size = send(sd, in_buf, strlen(in_buf) + 1, 0);
            if (size <= 0) break; 

            pthread_mutex_lock(&assigned_card_mutex);
            assigned_card_id = -1;
            memset(assigned_card_text, 0, sizeof(assigned_card_text));
            review_needed = review_received = 0;
            is_review_complete = FALSE;
            review_start_time = 0;
            pthread_mutex_unlock(&assigned_card_mutex);

            printf(VERDE "[SUCCESS] Card completata. A breve potresti ricevere una nuova card\n" RESET);

        } else if (strncmp(in_buf, "OKAY_REVIEW", 11) == 0){
            sscanf(in_buf, "OKAY_REVIEW %hd\n", &review_port);
            int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (udp_sock < 0) {
                perror("Socket UDP error");
                continue;
            }

            struct sockaddr_in dest_addr;
            memset(&dest_addr, 0, sizeof(dest_addr));
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(review_port); 
            inet_pton(AF_INET, "127.0.0.1", &dest_addr.sin_addr);

            char msg[MAX_BUF_SIZE];

            // l'utente manda anche la propria porta.
            snprintf(msg, sizeof(msg), "OKAY_REVIEW %d", atoi(argv[1]));
            ssize_t sent = sendto(udp_sock, msg, strlen(msg), 0, 
                                 (struct sockaddr*)&dest_addr, sizeof(dest_addr));
            if (sent < 0) {
                printf(ROSSO "[ERRORE] Errore nell'invio della conferma, riprovare" RESET);
            }
            
        } else if (strcmp(in_buf, "CLEAR\n") == 0){
            printf("\033[H\033[2J\033[3J");
            stampa_menu();
        } else if(strcmp(in_buf, "SHOW_CARD\n") == 0){
            pthread_mutex_lock(&assigned_card_mutex);
            if (assigned_card_id == -1) {
                printf(ROSSO "\n[ERRORE] Al momento non hai nessuna card in gestione.\n" RESET);
            } else {
                printf("\n" CIANO "╔══════════════════════════════════════════════════════╗" RESET "\n");
                printf(CIANO "║" RESET "               " GRASSETTO "DETTAGLI CARD ASSEGNATA" RESET "                " CIANO "║" RESET "\n");
                printf(CIANO "╠══════════════════════════════╦═══════════════════════╣" RESET "\n");
                
                printf(CIANO "║" RESET "  ID: " GRASSETTO "#%-23d" RESET CIANO "║" RESET "  STATO: " GIALLO "DOING" RESET "     " CIANO "    ║" RESET "\n", 
                    assigned_card_id);
                
                printf(CIANO "╠══════════════════════════════╩═══════════════════════╣" RESET "\n");
                printf(CIANO "║" RESET "  DESCRIZIONE ATTIVITÀ:                               " CIANO "║" RESET "\n");
                
                printf(CIANO "║" RESET "  %-52.52s" CIANO "║" RESET "\n", 
                    assigned_card_text);
                
                printf(CIANO "╚══════════════════════════════════════════════════════╝" RESET "\n\n");
            }
            pthread_mutex_unlock(&assigned_card_mutex);
        } else {
            printf(ROSSO "[ERRORE] Comando non valido!"RESET"\n");
        }

    }
    
}

void* client_listener(void* arg){
    uint16_t port = atoi((char *)arg), ret;
    socklen_t len;
    char buf[MAX_NOT_BUF_SIZE], async_buffer[MAX_NOT_BUF_SIZE];
    int tcp_sd, udp_sd, max_sd, server_sd;
    uint16_t review_send_port;
    fd_set read_fds;
    struct sockaddr_in async_addr, async_addr_to_server;
    
    // -----------------------------------------------------------
    // PREPARAZIONE SOCKET TCP 
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
    // PREPARAZIONE SOCKET UDP 
    // -----------------------------------------------------------
    udp_sd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (bind(udp_sd, (struct sockaddr*) &async_addr, sizeof(async_addr)) < 0){
        close(udp_sd);
        close(tcp_sd);
        pthread_exit(NULL);
    }
    
    // Si deve mettere in ascolto sulla porta client
    while (1) {
        // pulizia del set di descrittori
        FD_ZERO(&read_fds);
        
        // si aggiunge il socket UDP
        FD_SET(udp_sd, &read_fds);
        // si aggiunge il socket TCP
        FD_SET(tcp_sd, &read_fds);

        
        max_sd = (udp_sd > tcp_sd) ? udp_sd : tcp_sd;

        struct timeval timeout;
        timeout.tv_sec = 1;  // Si sveglia ogni 1 secondo
        timeout.tv_usec = 0;

        int activity = select(max_sd + 1, &read_fds, NULL, NULL, &timeout);
        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
            break;
        }

        // Controllo del timer per i processi a cui è stat chiesta la review
        pthread_mutex_lock(&review_mutex);
        if (review_needed > 0 && is_review_complete == FALSE) {
            time_t now = time(NULL);

            if (difftime(now, review_start_time) > 90) {
                review_needed = 0;
                review_received = 0;
                is_review_complete = FALSE;

                printf(ROSSO "[TIMEOUT] Tempo scaduto per la review! Nessuna risposta completa." RESET "\n");
                printf(GIALLO "[HINT] Rilancia il comando SHOW_USR_LIST per ottenere la lista degl utenti aggiornati\n e successivamente il comando REVIEW_CARD per riprovare." RESET "\n");
            }
            
        }
        pthread_mutex_unlock(&review_mutex);
        if (activity == 0) continue;
        
        

        if (FD_ISSET(udp_sd, &read_fds)) {
            ssize_t n = recvfrom(udp_sd, buf, sizeof(buf) - 1, 0, NULL, NULL);
            buf[n] = '\0';
            if (strncmp(buf, "REVIEW:", 7) == 0) {
                printf("\r\033[K"); // Cancella riga corrente
                printf("[REVIEW RICHIESTA] %s\n", buf);
                printf(">>> ");
                fflush(stdout);
            } else if (is_review_complete == FALSE && sscanf(buf, "OKAY_REVIEW %hd\n", &review_send_port) == 1) {
                pthread_mutex_lock(&review_mutex);
                if (has_already_voted(review_send_port) == TRUE || review_needed == 0) {
                    pthread_mutex_unlock(&review_mutex);
                    continue;
                }
                
                
                // devo incrementare un contatore globale
                voters[review_received] = review_send_port;
                review_received++;
                
                // si controlla se sono sufficienti
                if (review_received == review_needed) {
                    printf("[NOTIFICA] Review completata; adesso è possibile terminare la card\n");
                    is_review_complete = TRUE;
                }
                pthread_mutex_unlock(&review_mutex);
            }
            
            memset(async_buffer, 0, sizeof(async_buffer));
        }
        
        if (FD_ISSET(tcp_sd, &read_fds)) {
            ssize_t n = recv(tcp_sd, buf, MAX_NOT_BUF_SIZE - 1, 0);
    
            if (n <= 0) {
                // Il server ha chiuso la connessione (o crash)
                printf("[ERRORE] Connessione notifiche persa col server.\n");
                close(tcp_sd);
                close(udp_sd);
                // Usciamo dal thread o gestiamo la chiusura del client
                pthread_exit(NULL); 
            }

            buf[n] = '\0';

            if (strcmp(buf, "PING_USER") == 0) {
                send(tcp_sd, "PONG_LAVAGNA", 13, 0);
            } else if (strncmp(buf, "ASYNC: HANDLE_CARD", 18) == 0){

                char *ptr_msg = buf + 19; 
                char *text_start;

                id_t id = (id_t)strtoul(ptr_msg, &text_start, 10);
                
                if (*text_start == ' ') text_start++;
                
                uint32_t net_peer_count;
                int r = recv(tcp_sd, &net_peer_count, sizeof(uint32_t), 0);
                int peer_count = ntohl(net_peer_count);

                uint16_t raw_ports[MAX_USER];
                if (peer_count > 0) {
                    // Attendiamo esattamente i byte necessari
                    int expected_bytes = peer_count * sizeof(uint16_t);
                    int total_read = 0;
                    while(total_read < expected_bytes) {
                        r = recv(tcp_sd, (char*)raw_ports + total_read, expected_bytes - total_read, 0);
                        if (r <= 0) break; // Gestione errore disconnessione
                        total_read += r;
                    }
                }

                send(tcp_sd, "ACK_CARD\n", 9, 0);

                snprintf(async_buffer, sizeof(async_buffer), "Card assegnata #%d: %s (Peer per review: %d)\n", id, text_start, peer_count);
                async_buffer[strcspn(async_buffer, "\n")] = 0;
                
                printf("[NOTIFICA ASINCRONA] %s\n", async_buffer);
                fflush(stdout);

                
                pthread_mutex_lock(&review_mutex);
                is_review_complete = FALSE;
                review_received = 0;
                memset(voters, 0, sizeof(voters));
                pthread_mutex_unlock(&review_mutex);

                pthread_mutex_lock(&assigned_card_mutex);
                strncpy(assigned_card_text, text_start, MAX_BUF_SIZE - 1);
                assigned_card_text[MAX_BUF_SIZE - 1] = '\0';
                assigned_card_text[strcspn(assigned_card_text, "\r\n")] = 0;
                assigned_card_id = id; 
                pthread_mutex_unlock(&assigned_card_mutex);
                
                memset(async_buffer, 0, sizeof(async_buffer));
            }
        }

    }

    return 0;
}

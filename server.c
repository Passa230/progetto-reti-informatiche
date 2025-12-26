#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <utilities/lavagna.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <include/color.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>



void* manage_request(void* arg);
void* card_handler(void* arg);

int main(){
    // inizializzazione della lavagna
    lavagna_init();
    signal(SIGPIPE, SIG_IGN);

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
        perror(ROSSO "[ERRORE] Errore di binding" RESET);
        return 1;
    }
    ret = listen(sd, 20);
    if (ret < 0) {
        perror(ROSSO "[ERRORE] Errore di ascolto" RESET);
        return 1;
    }
    len = sizeof(cl_addr);   

    for (;;) {    
        // si accetta una richiesta
        int new_sd = accept(sd, (struct sockaddr*)&cl_addr, &len);
        if (new_sd < 0) {
            perror(ROSSO "[ERRORE] Errore di accept" RESET);
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
    lavagna_hello(port);
    send(user_sd, "ok", strlen("ok") + 1, 0);
    printf(VERDE "[LOG] Utente registrato alla porta %hd" RESET "\n", port);

    
    
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
                printf(ROSSO "[ERRORE] Errore ricezione descrizione card o connessione chiusa"RESET"\n");
                lavagna_quit(port);
                close(user_sd);
                pthread_exit(NULL);
            }
            buf[size] = '\0';
            
            lavagna_card_add(buf, port);
            printf(VERDE "[LOG] Creata card con testo %s"RESET"\n", buf);
            send(user_sd, "CARD CREATA CON SUCCESSO!\n", 27, 0);
        } else if (strcmp(buf, "SHOW_USR_LIST\n") == 0) {
            lavagna_user_list(out_buf, MAX_BUF_SIZE);
            uint32_t user_len = (uint32_t)(strlen(out_buf) + 1);
            uint32_t n_user_len = htonl(user_len);
            send(user_sd, &n_user_len, sizeof(user_len), 0);
            // CORREZIONE 1: Invia solo i byte della stringa + terminatore, non 1023 byte
            send(user_sd, out_buf, user_len, 0); 

            uint16_t user_buf[MAX_USER];
            memset(user_buf, 0, sizeof(user_buf));
            user_len = lavagna_user_list_to_vec(user_buf);
            
            // CORREZIONE 2: Converti ogni porta in Network Order (htons)
            for(int i = 0; i < user_len; i++) {
                user_buf[i] = htons(user_buf[i]);
            }

            // CORREZIONE 3: Converti la lunghezza in Network Order (htonl)
            int user_len_net = htonl(user_len);
            
            send(user_sd, &user_len_net, sizeof(user_len_net), 0);
            if (user_len > 0)
                send(user_sd, user_buf, sizeof(uint16_t) * user_len, 0);
        } else if (strcmp(buf, "SHOW_LAVAGNA\n") == 0) {
            char lavagna_buf[MAX_SBUF_SIZE];
            lavagna_stampa(lavagna_buf, MAX_SBUF_SIZE);
            send(user_sd, lavagna_buf, MAX_SBUF_SIZE-1, 0);
        } else if (strcmp(buf, "QUIT\n") == 0) {
            lavagna_quit(port);
            send(user_sd, "CANCELLAZIONE AVVENUTA CON SUCCESSO\n\0", 37 , 0);
            printf(GIALLO "[LOG] Utente uscito alla porta %hd" RESET "\n", port);
            close(user_sd);
            pthread_exit(0);
        } else if(strcmp(buf, "CARD_DONE\n") == 0){
            pthread_mutex_lock(&lavagna.sem_cards[1]);
            card_t* c = lavagna_trova_card_per_id(port);
            if (c == NULL) {
                pthread_mutex_unlock(&lavagna.sem_cards[1]);
                continue;
            }

            c = lavagna_card_remove(c->id, 1);
            if (c == NULL) {
                pthread_mutex_unlock(&lavagna.sem_cards[1]);
                printf("[LOG] Per qualche motivo la card è null");
                continue;
            }
            uint16_t p = c->utente_assegnatario;

            //printf("[LOG] Nessun Segmentation Fault\n");
            lavagna_move_card_to_head(c, 2);
            //printf("[LOG] Nessun Segmentation Fault\n");
            pthread_mutex_unlock(&lavagna.sem_cards[1]);

            pthread_mutex_lock(&lavagna.conn_user_sem);
            for (int i = 0; i < lavagna.connected_users; i++) {
                if (lavagna.utenti_registrati[i].port == p) {
                    lavagna.utenti_registrati[i].id = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&lavagna.conn_user_sem);
            

            //lavagna_stampa(lavagna_buf, MAX_SBUF_SIZE);
            //printf("%s", lavagna_buf);
            printf(VERDE "[LOG] Spostata card in DONE" RESET "\n");
        } else {
            printf("[LOG] Il comando non esiste\n");
            //send(user_sd, "ERRORE: Comando non valido!\n\0", 29 , 0);
        }

        memset(buf, 0, sizeof(buf));
    }
    
}


/**
 * @brief thread per la gestione delle card. Ogni k secondi il thread controlla se è possibile assegnare
 * delle nuove card a qualche utente, inoltre controlla se il tempo passato dall'ultimo assegnamento di una
 * particolare card supera un certo limite superiore e in tal caso manda un PING al client
 * @todo aggiungere altri messaggi di log se necessario
 * @todo implememntare meccanismo della stampa della lavagna ad ogni modifica
 * @todo  
 */
void* card_handler(void* arg){
    char msg[MAX_BUF_SIZE], buf[MAX_BUF_SIZE];
    

    memset(msg, 0, sizeof(msg));
    while (1) {
        pthread_mutex_lock(&lavagna.sem_cards[0]);
        card_t* card = lavagna.cards[0];
        while (card != NULL) {
            card_t* next = card->next_card;
            if (card->utente_assegnatario == 0 && card->utente_creatore != 0) {
                pthread_mutex_lock(&lavagna.conn_user_sem);
                for (int i = 0; i < lavagna.connected_users; i++) {
                    if (lavagna.utenti_registrati[i].id == 0) {

                        snprintf(msg, sizeof(msg), "ASYNC: HANDLE_CARD %d %s\n", 
                                        card->id, card->testo_attivita);
                        send(lavagna.utenti_registrati[i].sock_id, msg, strlen(msg) + 1, 0);

                        fd_set read_fds;
                        struct timeval timeout;  
                        FD_ZERO(&read_fds);
                        FD_SET(lavagna.utenti_registrati[i].sock_id, &read_fds);

                        timeout.tv_sec = 3;
                        timeout.tv_usec = 0;
                                      
                        printf(GIALLO "[LOG] Inviata proposta a %d. Attendo ACK..." RESET "\n", lavagna.utenti_registrati[i].port);

                        int activity = select(lavagna.utenti_registrati[i].sock_id + 1, &read_fds, NULL, NULL, &timeout);
                        if (activity > 0) {
                            int n = recv(lavagna.utenti_registrati[i].sock_id, buf, sizeof(buf)-1, 0);
                            buf[n] = '\0';
                            if (strncmp(buf, "ACK_CARD", 8) == 0) {
                                printf(VERDE "[LOG] ACK ricevuto! Sposto card in DOING." RESET "\n");
                                lavagna_card_remove(card->id, 0);
                                card->utente_assegnatario = lavagna.utenti_registrati[i].port;
                                lavagna.utenti_registrati[i].id = card->id;
                                lavagna_move_card_to_head(card, 1);
                                break;
                            }
                            
                        }
                    }
                }
                pthread_mutex_unlock(&lavagna.conn_user_sem);
            } 
            card = next;
        }
        pthread_mutex_unlock(&lavagna.sem_cards[0]);
         
        
        //printf("Sto per andare a dormire\n");
        sleep(5);

        // AGGIORNAMENTO DEI TIMER RELATIVI ALLA LAVAGNA
        time_t now = time(NULL);
        pthread_mutex_lock(&lavagna.conn_user_sem);
        for (int i = 0; i < lavagna.connected_users; i++) {
            user_t* u = &lavagna.utenti_registrati[i];

            if (u->id == 0) continue;

            pthread_mutex_lock(&lavagna.sem_cards[1]);
            card_t* c = lavagna_trova_card_per_id(u->port);
            
            

            // se l'utente non è mai stato pingato
            if (u->last_ping == 0) {
                // Si controlla se dall'ultimo aggiornamento è passato 1.30min
                if (difftime(now, c->ultimo_aggiornamento) >= PING_TIMEOUT) {
                    // In caso affermativo si manda un ping
                    send(u->sock_id, "PING_USER", 10, 0);
                    u->last_ping = now;
                    printf(GIALLO "[LOG] Inviato ping all'utente alla porta %d" RESET "\n", u->port);
                }
            } else {
                char tmp[16];
                // Si controlla se si è ricevuto un pong
                if (recv(u->sock_id, tmp, sizeof(tmp), MSG_DONTWAIT) > 0) {
                    if (strcmp(tmp, "PONG_LAVAGNA") == 0) {
                        u->last_ping = 0;
                        c->ultimo_aggiornamento = now;
                        printf(VERDE "[LOG] PONG dall'utente alla porta %hd "RESET"\n", u->port);
                    }
                } else if (difftime(now, u->last_ping) > 30){
                    printf(ROSSO "[ERRORE] TIMEOUT! L'utente %d non ha risposto. Sposto card in To Do"RESET"\n", u->port);
                    c->utente_assegnatario = 0;
                    pthread_mutex_unlock(&lavagna.conn_user_sem);
                                        
                    c = lavagna_card_remove(c->id, 1);

                    pthread_mutex_unlock(&lavagna.sem_cards[1]);

                    if (c != NULL) {
                        lavagna_move_card_to_head(c, 0); // Sposta in TODO

                        // Stampa (usa malloc per sicurezza come detto prima)
                        //char visualizza[MAX_SBUF_SIZE];
                        //lavagna_stampa(visualizza, MAX_SBUF_SIZE);
                        //printf("%s\n", visualizza);
                    }                    

                    pthread_mutex_lock(&lavagna.conn_user_sem);
                    u->id = 0;
                    u->last_ping = 0;
                    continue;
                }
            }     
            pthread_mutex_unlock(&lavagna.sem_cards[1]);
        }
        pthread_mutex_unlock(&lavagna.conn_user_sem);
    }
}
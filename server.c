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
void stampa_lavagna();

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
            buf[strcspn(buf, "\r\n")] = 0;

            lavagna_card_add(buf, port);
            printf(VERDE "[LOG] Creata card con testo %s"RESET"\n", buf);
            send(user_sd, "CARD CREATA CON SUCCESSO!\n", 27, 0);
            stampa_lavagna();
        } else if (strcmp(buf, "SHOW_USR_LIST\n") == 0) {
            int written = lavagna_user_list(out_buf, MAX_SBUF_SIZE);
            uint32_t user_len = (uint32_t)(strlen(out_buf) + 1);
            uint32_t n_user_len = htonl(user_len);

            send(user_sd, &n_user_len, sizeof(n_user_len), 0);
            // Manda la lunghezza della stringa
            send(user_sd, out_buf, user_len, 0); 

            uint16_t user_buf[MAX_USER];
            memset(user_buf, 0, sizeof(user_buf));
            user_len = lavagna_user_list_to_vec(user_buf);
            
            for(int i = 0; i < user_len; i++) {
                user_buf[i] = htons(user_buf[i]);
            }

            int user_len_net = htonl(user_len);
            
            send(user_sd, &user_len_net, sizeof(user_len_net), 0);
            if (user_len > 0)
                send(user_sd, user_buf, sizeof(uint16_t) * user_len, 0);
        } else if (strcmp(buf, "SHOW_LAVAGNA\n") == 0) {
            char lavagna_buf[MAX_SBUF_SIZE];
            lavagna_stampa(lavagna_buf, MAX_SBUF_SIZE);
            uint32_t real_len = (uint32_t)(strlen(lavagna_buf) + 1);
            uint32_t n_real_len = htonl(real_len);

            send(user_sd, &n_real_len, sizeof(n_real_len), 0);
            send(user_sd, lavagna_buf, real_len, 0);

        } else if (strcmp(buf, "QUIT\n") == 0) {
            lavagna_quit(port);
            send(user_sd, "CANCELLAZIONE AVVENUTA CON SUCCESSO\n\0", 37 , 0);
            printf(GIALLO "[LOG] Utente uscito alla porta %hd" RESET "\n", port);
            close(user_sd);
            pthread_exit(0);
        } else if(strcmp(buf, "CARD_DONE\n") == 0){
            pthread_mutex_lock(&lavagna.sem_cards[1]);
            card_t* c = lavagna_trova_card_per_id(port);
            id_t target_id = (c != NULL) ? c->id : -1;
            pthread_mutex_unlock(&lavagna.sem_cards[1]);

            if (target_id == -1) continue;
            

            c = lavagna_card_remove(c->id, 1);

            
            if (c != NULL)
            {
                uint16_t p = c->utente_assegnatario;

                c->utente_assegnatario = 0;
                lavagna_move_card_to_head(c, 2);

                pthread_mutex_lock(&lavagna.conn_user_sem);
                for (int i = 0; i < lavagna.connected_users; i++) {
                    if (lavagna.utenti_registrati[i].port == p) {
                        lavagna.utenti_registrati[i].id = 0;
                        break;
                    }
                }
                pthread_mutex_unlock(&lavagna.conn_user_sem);
                
            }
            
            printf(VERDE "[LOG] Spostata card in DONE" RESET "\n");
            stampa_lavagna();
        } else {
            printf("[LOG] Il comando non esiste\n");
        }

        memset(buf, 0, sizeof(buf));
    }
    
}


/**
 * @brief thread per la gestione delle card. Ogni k secondi il thread controlla se è possibile assegnare
 * delle nuove card a qualche utente, inoltre controlla se il tempo passato dall'ultimo assegnamento di una
 * particolare card supera un certo limite superiore e in tal caso manda un PING al client
 * @todo aggiungere altri messaggi di log se necessario
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

                        // Va inviata anche la lista degli utenti disponibili e il loro numero

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
                                card_t* card_to_move = lavagna_card_remove(card->id, 0);
                                if (card_to_move != NULL) {
                                    card_to_move->utente_assegnatario = lavagna.utenti_registrati[i].port;
                                    lavagna.utenti_registrati[i].id = card_to_move->id;
                                    lavagna_move_card_to_head(card, 1);

                                    /*pthread_mutex_unlock(&lavagna.sem_cards[0]);
                                    stampa_lavagna();
                                    pthread_mutex_lock(&lavagna.sem_cards[0]);*/

                                }
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
         
        sleep(HANDLER_SLEEP_TIME);

        // AGGIORNAMENTO DEI TIMER RELATIVI ALLA LAVAGNA
        time_t now = time(NULL);
        pthread_mutex_lock(&lavagna.conn_user_sem);

        for (int i = 0; i < lavagna.connected_users; i++) {
            user_t* u = &lavagna.utenti_registrati[i];

            if (u->id == 0) continue;

            pthread_mutex_lock(&lavagna.sem_cards[1]);
            card_t* c = lavagna_trova_card_per_id(u->port);
            
            bool_t is_timeout = FALSE;
            id_t id_to_remove = -1;

            // se l'utente non è mai stato pingato
            if (u->last_ping == 0) {
                // Si controlla se dall'ultimo aggiornamento è passato 1.30min
                if (c != NULL && difftime(now, c->ultimo_aggiornamento) >= PING_TIMEOUT) {
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
                        if (c != NULL) c->ultimo_aggiornamento = now;
                        
                        printf(VERDE "[LOG] PONG dall'utente alla porta %hd "RESET"\n", u->port);
                    }
                } else if (difftime(now, u->last_ping) > 30){
                    printf(ROSSO "[ERRORE] TIMEOUT! L'utente %d non ha risposto. Sposto card in To Do"RESET"\n", u->port);
                    
                    is_timeout = TRUE;
                    if (c != NULL) id_to_remove = c->id;
                }
            }     
            pthread_mutex_unlock(&lavagna.sem_cards[1]);

            if (is_timeout == TRUE) {
                printf(ROSSO "[ERRORE] TIMEOUT! L'utente %d non ha risposto. Sposto card in To Do"RESET"\n", u->port);

                if (id_to_remove != -1) {
                    card_t* removed = lavagna_card_remove(id_to_remove, 1);

                    if (remove != NULL) {
                        removed->utente_assegnatario = 0;
                        lavagna_move_card_to_head(removed, 0);
                        stampa_lavagna();
                    }
                    
                }

                u->id = 0;
                u->last_ping = 0;
                
            }
            
        }
        pthread_mutex_unlock(&lavagna.conn_user_sem);
    }
}

void stampa_lavagna(){
    // Utilizzando static il buffer viene allocato una sola volta
    // all'inizio del programma e riutilizzato ad ogni chiamata
    static char log_buf[MAX_SBUF_SIZE];
    memset(log_buf, 0, MAX_SBUF_SIZE);
    
    lavagna_stampa(log_buf, MAX_SBUF_SIZE);
    printf("%s", log_buf);
}
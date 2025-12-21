#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <generic.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>


char async_buffer[MAX_BUF_SIZE];
bool_t impeding_notify;

void* client_listener(void* arg);

int main(int argc, char **argv){
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    // Blocco della possibilità di fare CTRL + C all'utente
    // signal(SIGINT, SIG_IGN);
    int ret, sd;
    struct sockaddr_in sv_addr;
    char buf[MAX_BUF_SIZE], lavagna_buf[MAX_SBUF_SIZE];
    char in_buf[MAX_BUF_SIZE];
    ssize_t size;

    if (atoi(argv[1]) < 5678) {
        printf("ERRORE: Non è possibile registrarsi a questa porta\n");        
        return 0;
    }

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
        if (impeding_notify == TRUE) {
            printf("%s", async_buffer);
            impeding_notify = FALSE;
        }
        
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
        
        }       

        if (strcmp(in_buf, "SHOW_LAVAGNA\n") == 0) {
            size = recv(sd, lavagna_buf, MAX_SBUF_SIZE, 0);
            printf("%s\n", lavagna_buf);
        } else {
            size = recv(sd, buf, MAX_BUF_SIZE, 0);
            printf("%s\n", buf);
        }
        

        if(strcmp(in_buf, "QUIT\n") == 0){
            return 0;
        }


    }
    
}


void* client_listener(void* arg){
    int port = atoi((char *)arg), ret, len;
    char buf[MAX_BUF_SIZE];


    struct sockaddr_in async_addr;
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
    while ((new_sd = accept(sd, NULL, NULL)) < 0) {
        perror("accept");
    }

    // Si deve mettere in ascolto sulla porta client
    while (1) {
        
        ssize_t size = recv(new_sd, buf, MAX_BUF_SIZE - 1, 0);
        if (size <= 0) break;
        buf[size] = '\0';

        int id; char testo[MAX_BUF_SIZE];

        if (strcmp(buf, "PING") == 0) {
            // SI MANDA PONG --> Card attiva
        } else if (sscanf(buf, "ASYNC: HANDLE_CARD %d %[^\n]", &id, testo) == 2){
            sprintf(async_buffer, ">> NOTIFICA: Card assegnata #%d: %s\n", id, testo);
            impeding_notify = TRUE;
            //printf(">> NOTIFICA: Card assegnata #%d: %s\n", id, testo);
        }
        

        while (impeding_notify == TRUE){};

        /*
        QUATTRO POSSIBILI MESSAGGI ASINCRONI:
            - PING          --> Inviato dal server
            - ASSEGNAMENTO  --> Inviato dal server
            - ACK_REQUEST   --> Inviato da un client che vuole una conferma
            - OKAY          --> Inviato dai client per approvare la card dell'utente  
        */

    }
    
}
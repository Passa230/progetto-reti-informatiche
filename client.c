#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <generic.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>


int main(int argc, char **argv){
    // Blocco della possibilità di fare CTRL + C all'utente
    signal(SIGINT, SIG_IGN);

    int ret, sd;
    struct sockaddr_in sv_addr;
    char buf[MAX_BUF_SIZE], lavagna_buf[MAX_SBUF_SIZE];
    char in_buf[MAX_BUF_SIZE];
    ssize_t size;

    sd = socket(AF_INET, SOCK_STREAM, 0);
    printf("Qui ci arrivo\n");
    
    memset(&sv_addr, 0, sizeof(sv_addr)); 
    sv_addr.sin_family = AF_INET ;

    // Controllare che quella passata sia una porta e sia giusta
    if (atoi(argv[1]) < 5678) {
        printf("ERRORE: Non è possibile registrarsi a questa porta\n");        
        return 0;
    }
    
    printf("Qui ci arrivo\n");
    
    sv_addr.sin_port = htons(5678);

    inet_pton(AF_INET, "127.0.0.1", &sv_addr.sin_addr);
    ret = connect(sd, (struct sockaddr*)&sv_addr, sizeof(sv_addr));

    size = send(sd, argv[1], strlen(argv[1]) + 1, 0);    // si aspetta la conferma della registrazione
    printf("Qui ci arrivo\n");
    
    size = recv(sd, buf, MAX_BUF_SIZE-1, 0);
    // TODO: Gestire size < 0
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
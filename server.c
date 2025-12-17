#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <utilities/lavagna.h>
#include <string.h>

void* manage_request(void* arg);
void* reg_user_to_kanban(void* arg);

int main(){
    // inizializzazione della lavagna
    lavagna_init();

    int ret, sd;
    socklen_t len;
    
    struct sockaddr_in my_addr, cl_addr;

    sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(5678);
    inet_pton(AF_INET, "127.0.0.1", &my_addr.sin_addr);

    ret = bind(sd, (struct sockaddr*) &my_addr, sizeof(my_addr));
    // TODO: verificare errore di associazione
    ret = listen(sd, 20);
    // TODO: verificare errore sulla listen
    len = sizeof(cl_addr);   

    for (;;) {    
        // si accetta una richiesta
        int new_sd = accept(sd, (struct sockaddr*)&cl_addr, &len);

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
    uint16_t connection_active = 1, flag = 0;
    char buf[MAX_BUF_SIZE], out_buf[MAX_BUF_SIZE];

    ssize_t size = recv(user_sd, buf, MAX_BUF_SIZE-1, 0);
    uint16_t port = atoi(buf);
    lavagna_hello(port);
    send(user_sd, "ok", strlen("ok") + 1, 0);
    printf("registrato utente alla porta %d\n", port);

    
    
    while (connection_active == 1) {
        // dobbiamo eseguire
        flag = 0;
        
        ssize_t size;
        size = recv(user_sd, buf, MAX_BUF_SIZE, 0); 
        // TODO: Gestire la ricezione dell'errore

        // GESTIRE LA LOGICA DEI COMANDI
        
        if (strcmp(buf, "CREATE_CARD") == 0) {
            // TODO: Creare una nuova card

            flag = 1;
        }

        if (strcmp(buf, "SHOW_USR_LIST") == 0) {
            lavagna_user_list(out_buf, MAX_BUF_SIZE);
            send(user_sd, out_buf, MAX_BUF_SIZE-1, 0);
        
            flag = 1;
        }

        if (strcmp(buf, "SHOW_LAVAGNA") == 0) {
            // TODO: Mostrare la lavagna 
        
            flag = 1;
        }

        if (strcmp(buf, "QUIT") == 0) {
            lavagna_quit(port);
            send(user_sd, "CANCELLAZIONE AVVENUTA CON SUCCESSO\n\0", 37 , 0);
            pthread_exit(0);

            flag = 1;
        } 

        if (flag == 0){
            send(user_sd, "ERRORE: Comando non valido!\n\0", 29 , 0);
        }

        memset(buf, 0, sizeof(buf));
    }
    
}


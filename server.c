#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <generic.h>
#include <utilities/lavagna.h>

void* manage_request(void* arg);

int main(){
    // inizializzazione della lavagna
    lavagna_init();

    int ret, sd, len;
    
    struct sockaddr_in my_addr, cl_addr;

    sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(5678);
    inet_pton(AF_INET, "127.0.0.1", &my_addr.sin_addr);

    ret = bind(sd, (struct sockaddr*) &my_addr, sizeof(my_addr));
    ret = listen(sd, 20);
    len = sizeof(cl_addr);   

    for (;;) {    
        // si accetta una richiesta
        int new_sd = accept(sd, (struct sockaddr*)&cl_addr, &len);

        // SE HO RICEVUTO UN HELLO CONTROLLO CHE L'UTENTE NON SIA GIà REGISTRATO
        // CAMBIARE GESTIONE COMPLETAMENTE. 
        
        // si crea il thread che la gestisce
        pthread_t t_id;
        pthread_create(&t_id, NULL, manage_request, (void*) &cl_addr);

        // si fa in modo che al termine del thread venga pulita tutta la sua memoria
        pthread_detach(t_id);

    }
    
}


void* manage_request(void* arg){
    // va gestita la richiesta in funzione di quello che l'utente
    // chiede

    uint16_t connection_active;
    int user_sd = (int)(intptr_t) arg;
    char buf[MAX_BUF_SIZE];
    struct sockaddr_in cl_addr;
    /**
     * Idealmente la funzione dovrebbe entrare in un ciclo infinito che termina al 
     * momento del quit.
     */
    
    while (connection_active == 1) {
        // dobbiamo eseguire
        
        ssize_t size;
        size = recv(user_sd, buf, MAX_BUF_SIZE, 0); 
        if (size > 0) {
            if (strncmp(buf, "HELLO", 5) == 0) {
                printf("Comando ricevuto: HELLO da socket %d\n", user_sd);
                
            }
        } else {
            // Errore di ricezione --> da gestire
        }
        
        memset(buf, 0, sizeof(buf));
    }
    
}
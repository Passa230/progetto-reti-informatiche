#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <generic.h>
#include <stdio.h>
#include <string.h>


int main(int argc, char **argv){
    int ret, sd;
    struct sockaddr_in sv_addr;
    char buf[MAX_BUF_SIZE];

    sd = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&sv_addr, 0, sizeof(sv_addr)); 
    sv_addr.sin_family = AF_INET ;

    // Controllare che quella passata sia una porta e sia giusta
    sv_addr.sin_port = htons(atoi(argv[1]));

    inet_pton(AF_INET, "127.0.0.1", &sv_addr.sin_addr);
    ret = connect(sd, (struct sockaddr*)&sv_addr, sizeof(sv_addr));

    // si aspetta la conferma della registrazione
    ssize_t size;
    size = recv(sd, buf, MAX_BUF_SIZE, 0);

    if (strcmp(buf, "ok")) {
        printf("Registrazione avvenuta con successo\n");
    }
    
    
}
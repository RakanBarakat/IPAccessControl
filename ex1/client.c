#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>



int main (int argc, char ** argv) {

      if (argc < 4) {
        printf("Error!\n");
        return 1;
    }
    int required_args = 0;

    if(strcmp(argv[3], "R") == 0 || strcmp(argv[3], "L") == 0){
        required_args = 0;
    }else if(strcmp(argv[3], "A") == 0 || strcmp(argv[3], "D") == 0 || strcmp(argv[3], "C") == 0) {
        required_args = 2;
    }else{
        printf("Error!\n");
        return 1;
    }

    char buffer[256];
    if (required_args == 0) {
        snprintf(buffer, sizeof(buffer), "%s", argv[3]);
    } else if (required_args == 2) {
        snprintf(buffer, sizeof(buffer), "%s %s %s", argv[3], argv[4], argv[5]);
    }
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer2[4096] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    int port = atoi(argv[2]);
     if (port <= 0 || port > 65535) {
        printf("Error!");
        return 1;
        }
    serv_addr.sin_port = htons(port);

    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if(send(sock, buffer, strlen(buffer), 0) < 0){
        close(sock);
        return 0;
    }

    int bytes = read(sock, buffer2, sizeof(buffer2) - 1);
    if (bytes < 0) {
        close(sock);
        return 1;
    }
    buffer2[bytes] = '\0';
    printf("%s",buffer2);
    close(sock);
    return 0;
}


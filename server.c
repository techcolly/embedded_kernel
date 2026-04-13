#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "imgproto.h"

int main() {

    struct sockaddr_in lh_socket;
    char buf[5];

    lh_socket.sin_family = AF_INET;
    lh_socket.sin_port = htons(8080);
    inet_aton("127.0.0.1", (struct in_addr *)&lh_socket.sin_addr.s_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int bind_st = bind(sock, (struct sockaddr*)&lh_socket, sizeof(lh_socket));
    int lis_st = listen(sock, 0);
    
    int acc_sock = accept(sock, NULL, NULL);

    read(acc_sock, buf, 4);

    int w_status = write(acc_sock, "good", 4);

    close(acc_sock);

    printf("%s", buf);
    return 0;

}
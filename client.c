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
    
    int conn = connect(sock, (struct sockaddr *)&lh_socket, sizeof(lh_socket));

    write(sock, "test", 4);
    
    read(sock, buf, 4);

    close(sock);
    
    printf("%s", buf);
    return 0;
}
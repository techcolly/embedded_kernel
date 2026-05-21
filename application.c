#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stddef.h>

#include "imgproto.h"


int main() {
    char input[INPUT_BUF_SIZE];
    char *command;

    char *args[MAX_ARGS] = {0};

    struct sockaddr_in listening_socket;
    
    listening_socket.sin_family = AF_INET;
    listening_socket.sin_port = htons(0);

    int r = 1;

    int listening_sock = socket(AF_INET, SOCK_STREAM, 0);
    inet_aton("127.0.0.1", (struct in_addr *)&listening_socket.sin_addr.s_addr);
    setsockopt(listening_sock, SOL_SOCKET, SO_REUSEADDR, &r, sizeof(r));

    bind(listening_sock, (struct sockaddr*)&listening_socket, sizeof(listening_socket)); // will check return later
    
    struct sockaddr_in sending_socket;

    sending_socket.sin_family = AF_INET;
    sending_socket.sin_port = htons(0);
    inet_aton("127.0.0.1", (struct in_addr *)&sending_socket.sin_addr.s_addr);

    int sending_sock = socket(AF_INET, SOCK_STREAM, 0);
    bind(sending_sock, (struct sockaddr*)&sending_socket, sizeof(sending_socket)); // this is so we get assigned a port number

    socklen_t len = sizeof(struct sockaddr_in);

    getsockname(sending_sock, (struct sockaddr *)&sending_socket, &len);
    int sending_port = ntohs(sending_socket.sin_port);

    len = sizeof(struct sockaddr_in);
    getsockname(listening_sock, (struct sockaddr *)&listening_socket, &len);
    int listening_port = ntohs(listening_socket.sin_port);

    printf("\n\nSockets created. Sending Port #: %d, Listening Port #: %d", sending_port, listening_port);

    while(1) {
        memset(args, 0, sizeof(args));
        printf("\nSEP3> ");
        if(!fgets(input, sizeof(input), stdin)) break; // add proper error handling later

        input[strcspn(input, "\n")] = 0;

        command = strtok(input, " ");

        for (int i = 0; i < MAX_ARGS; i++)  {
            args[i] = strtok(NULL, " "); if (!args[i]) break;
        }
        
        if (!command) continue;
        int argc = count_args(args);

        if (strncmp(command, "listen_port", INPUT_BUF_SIZE) == 0) { // set listening port; 0 for random OS chosen port
            if (argc < 1) {
                printf("\nUsage: listen_port PORT");
                continue;
            }
            int newport = strtol(args[0], NULL, 10);
            listening_socket.sin_port = htons(newport);
        } 
        
        else if (strncmp(command, "sending_port", INPUT_BUF_SIZE) == 0) { // set sending port; 0 for random OS chosen port
            if (argc < 1) {
                printf("\nUsage: sending_port PORT");
                continue;
            }
            int newport = strtol(args[0], NULL, 10);
            sending_socket.sin_port = htons(newport);
        }

        else if (strncmp(command, "listen_for", INPUT_BUF_SIZE) == 0) { // IP address to listen for (NOT the address to listen ON)
            if (argc < 1) {
                printf("\nUsage: listen_for IP");
                continue;
            }
            inet_pton(AF_INET, args[0], &(listening_socket.sin_addr));
        }

        else if (strncmp(command, "send", INPUT_BUF_SIZE) == 0) {
            int chunks = 1;

            char rem[MAX_STRING_SIZE] = {0};
            char outfile_full[MAX_STRING_SIZE] = {0};
            char security_mode[MAX_STRING_SIZE] = {0};

            if (argc < 3) {
                printf("\nUsage: send IP PORT infile [chunks=N] [rem=RGB] [out=PATH] [encrypt|obfuscate]");
                continue;
            }

            for (int i = 3; i < MAX_ARGS; i++) {
                if (!args[i]) break;

                if (sscanf(args[i], "chunks=%d", &chunks) != 0) continue;
                else if (sscanf(args[i], "rem=%s", rem) != 0) continue;
                else if (sscanf(args[i], "out=%s", outfile_full) != 0) continue;
                else if (strncmp(args[i], "encrypt", INPUT_BUF_SIZE) == 0 ||
                        strncmp(args[i], "obfuscate", INPUT_BUF_SIZE) == 0) {
                    strcpy(security_mode, args[i]);
                    continue;
                }

                printf("\nInvalid Arguments");
            }

            send_image_file(
                args[0],          // destination IP
                args[1],          // destination port
                args[2],          // input image path
                outfile_full,     // remote output path
                rem,              // color removal mode
                chunks,
                &sending_sock
            );
            
            refreshSocket(&sending_sock, args[0], args[1]);

            send_status_packet(STATUS_OK, &sending_sock); // for now it will always send ok
        }   
        
        else if (strncmp(command, "listen", INPUT_BUF_SIZE) == 0) {
            uint8_t buf[PAYLOAD_SIZE + (HEADER_LEN * (MAX_CHUNKING_AMOUNT + 2*(1 + MAX_STRING_SIZE))) + 1]; // max string length 255
            struct sockaddr_in accepted_addr;
            socklen_t accepted_len = sizeof(accepted_addr);

            int lis_st = listen(listening_sock, 1);
            int acc_sock = accept(listening_sock, (struct sockaddr*)&accepted_addr, &accepted_len);
            
            if (acc_sock < 0) {
                perror("An error occured");
                continue;
            }

            int bytesRead = 0, packetsRead = 0;
            uint8_t numPackets;
            uint16_t filename_len, path_len, flags; 
            uint32_t payload_len;

            char *filename = NULL, *path = NULL;
            do {
                read_exact(acc_sock, buf + bytesRead, HEADER_LEN);

                memcpy(&numPackets, buf + bytesRead + MAGIC_LEN, 1);
                memcpy(&flags, buf + bytesRead + MAGIC_LEN + 1, 2);
                memcpy(&payload_len, buf + bytesRead + MAGIC_LEN + 3, 4);
                memcpy(&filename_len, buf + bytesRead + MAGIC_LEN + 7, 2);
                memcpy(&path_len, buf + bytesRead + MAGIC_LEN + 9, 2);

                bytesRead += HEADER_LEN;

                if (!packetsRead) {
                    filename = (char*)malloc(sizeof(char) * (1 + filename_len));
                    path = (char*)malloc(sizeof(char) * (1 + path_len));

                    filename[filename_len] = '\0';
                    path[path_len] = '\0';
                }

                //bytesRead += (int)read_exact(acc_sock, buf + bytesRead, payload_len + filename_len + path_len);
                
                bytesRead += (int)read_exact(acc_sock, buf + bytesRead, filename_len);
                if (!packetsRead) strncpy(filename, (char*)(buf + bytesRead - filename_len), filename_len);

                bytesRead += (int)read_exact(acc_sock, buf + bytesRead, path_len);
                if (!packetsRead) strncpy(path, (char*)(buf + bytesRead - path_len), path_len);

                bytesRead += (int)read_exact(acc_sock, buf + bytesRead, payload_len);

                packetsRead++;
            } while (packetsRead < (int)numPackets);
            
            
            for (int i = 0; i < bytesRead; i++) {
                printf("%02x ", buf[i]);
            }

            switch (flags & PMODE_MASK) {

                case PMODE_REQUESTING: { // will implement this later as we need to make sending a function first
                    char send[2]; int t = 0;
                    char outfile_req[MAX_STRING_SIZE] = {0};
                    char infile_req[MAX_STRING_SIZE] = {0};

                    char port_str[MAX_STRING_SIZE] = {0};
                    char ip_str[MAX_STRING_SIZE] = {0};

                    printf("\nImage request received.\n\nBytes Received: %d\nRequested Path: %s/%s",
                        bytesRead, path, filename);
                    do {
                        printf("\nSend (Y / N)? ");
                        scanf("%1s", send);
                        if (tolower(send[0]) == 'y') {
                            snprintf(outfile_req, MAX_STRING_SIZE, "./%s", filename);
                            snprintf(infile_req, MAX_STRING_SIZE, "%s/%s", path, filename);
                            snprintf(port_str, MAX_STRING_SIZE, "%d", ntohs(accepted_addr.sin_port));
                            inet_ntop(AF_INET, &accepted_addr.sin_addr, ip_str, INET_ADDRSTRLEN);

                            send_image_file(
                                ip_str,          // destination IP
                                port_str,         // destination port
                                infile_req,      // input image path
                                outfile_req,     // remote output path (only the default folder atm)
                                NULL,              // color removal mode (not supported here atm)
                                CHUNKS_DEFAULT, // cannot change this rn
                                &sending_sock
                            );
                            
                            refreshSocket(&sending_sock, args[0], args[1]);
                            
                            send_status_packet(STATUS_OK, &sending_sock); // for now it will always send ok
                            break;
                        } else if (tolower(send[0]) == 'n') {
                            break;
                        } else {
                            printf("\nInvalid Option, try again.");
                            t++;
                        }
                    } while(t < 5);

                    break;
                }

                case PMODE_SENDING: {
                    Image* read_image = wireToImage(buf, bytesRead, (int)filename_len, (int)path_len);
                    structToP3(path, filename, read_image);

                    free(read_image);

                    printf("\nImage received successfully.\n\nBytes Received: %d\nPath: %s/%s",
                        bytesRead, path, filename);

                    break;
                }

                case PMODE_STATUS: { // we'll deal with this in a bit

                }

                default:
                    printf("\nUnknown packet mode.\n\nFlags: 0x%04x\nBytes Received: %d",
                        flags, bytesRead);
                    break;
            }

            close(acc_sock);
            free(filename);
            free(path);
        }

        else if (strncmp(command, "request", INPUT_BUF_SIZE) == 0) { // request an image request X.X.X.X PORT ./image.p3
            char reqPath[MAX_STRING_SIZE] = {0};
            char reqName[MAX_STRING_SIZE] = {0};

            uint16_t FLAGS = PMODE_REQUESTING;

            if (argc < 3) {
                printf("\nUsage: request IP PORT ./image.p3");
                continue;
            }

            char *requested_full = args[2];
            char *last_slash = strrchr(requested_full, '/');

            if (last_slash) {
                int dir_len = last_slash - requested_full;

                strncpy(reqPath, requested_full, dir_len);
                reqPath[dir_len] = '\0';

                strcpy(reqName, last_slash + 1);
            } else {
                strcpy(reqPath, ".");
                strcpy(reqName, requested_full);
            }

            struct sockaddr_in dest;

            dest.sin_family = AF_INET;
            dest.sin_port = htons((uint16_t)strtol(args[1], NULL, 10));
            inet_pton(AF_INET, args[0], &dest.sin_addr);

            Packet* reqP = requestPacket(FLAGS, reqName, reqPath);

            uint8_t *sendingCurrently = malloc(HEADER_LEN + strlen(reqName) + strlen(reqPath) + 1);

            int currentLen = 0;
            serializePacket(reqP, sendingCurrently, &currentLen);

            int conn = connect(sending_sock, (struct sockaddr *)&dest, sizeof(dest));

            write_exact(sending_sock, sendingCurrently, (size_t)currentLen);

            close(sending_sock);
            sending_sock = socket(AF_INET, SOCK_STREAM, 0);

            free(sendingCurrently);
            free_packet(reqP);
            
            break;
        }

        else if (strncmp(command, "exit", INPUT_BUF_SIZE) == 0) {
            break;
        }

        else {
            printf("\nUnknown Command");
        }
    }

    close(listening_sock);
    close(sending_sock); 
    free(args);
    return 0;
}
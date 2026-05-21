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

const char MAGIC_BYTES[MAGIC_LEN + 1] = "SEP3";

Image* p3ToStruct(const char* path) {
    FILE* ptr = NULL;
    Image* image = NULL;

    int r, g, b;
    char magic[3];
    int width, height, max_color;

    ptr = fopen(path, "r");
    if (!ptr) {
        perror("\nError opening file");
        goto fail;
    }

    if (fscanf(ptr, "%2s", magic) != 1 || strcmp(magic, "P3") != 0) {
        printf("\nInvalid File Format");
        goto fail;
    }

    if (fscanf(ptr, "%d %d", &width, &height) != 2 || width != IMG_W || height != IMG_H) {
        printf("\nInvalid File Format");
        goto fail;
    }

    if (fscanf(ptr, "%d", &max_color) != 1 || max_color != 255) {
        printf("\nInvalid File Format");
        goto fail;
    }

    image = (Image*)malloc(sizeof(Image));
    if (!image) {
        perror("\nAn error occurred");
        goto fail;
    }

    for (int y = 0; y < IMG_H; y++) {
        for (int x = 0; x < IMG_W; x++) {
            if (fscanf(ptr, "%d %d %d", &r, &g, &b) != 3) {
                printf("\nError scanning pixels into image struct");
                goto fail;
            }

            if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
                printf("\nInvalid pixel value");
                goto fail;
            }

            image->bmp_img[y][x].red   = (uint8_t)r;
            image->bmp_img[y][x].green = (uint8_t)g;
            image->bmp_img[y][x].blue  = (uint8_t)b;
        }
    }

    fclose(ptr);
    return image;

    fail:
        if (ptr) fclose(ptr);
        free(image);
        return NULL;
}

int structToP3(const char* path, const char* filename, const Image* image) {
    if (!path || !filename || !image) return S2P3_BAD_ARGS;

    char file_path[512];

    int n = snprintf(file_path, sizeof(file_path), "%s/%s", path, filename);

    if (n < 0) {
        printf("Unknown Error with output path");
        return GENERIC_FAILURE;
    } else if ((size_t)n >= sizeof(file_path)) {
        printf("\nOutput path too long");
        return S2P3_PATH_TOO_LONG;
    }

    FILE* ptr = fopen(file_path, "w");
    if (!ptr) {
        perror("\nError opening file");
        return S2P3_FOPEN_ERR;
    }

    fprintf(ptr, "P3\n128 128\n255");

    for (int y = 0; y < IMG_H; y++) {
        fprintf(ptr, "\n");

        for (int x = 0; x < IMG_W; x++) {
            int r = (int)image->bmp_img[y][x].red;
            int g = (int)image->bmp_img[y][x].green;
            int b = (int)image->bmp_img[y][x].blue;

            fprintf(ptr, "%d %d %d", r, g, b);
            if (x < IMG_W - 1) fprintf(ptr, " ");
        }
    }

    if (fclose(ptr) != 0) {
        perror("\nError closing output file");
        return S2P3_FCLOSE_ERR;
    }

    return SUCCESS;
}

int applyColorMode(uint16_t flags, Image* image) {

    int remove_R = (flags & CMODE_NO_R) != 0;
    int remove_G = (flags & CMODE_NO_G) != 0;
    int remove_B = (flags & CMODE_NO_B) != 0;

    if (!remove_R && !remove_G && !remove_B) return 0;

    for (int y = 0; y < IMG_H; y++) {
        for (int x = 0; x < IMG_W; x++) { // we can optimize this later with offsetof() and using the set bits to know the offset before looping
            if (remove_R) image->bmp_img[y][x].red = 0; 
            if (remove_G) image->bmp_img[y][x].green = 0;
            if (remove_B) image->bmp_img[y][x].blue = 0;
        }
    }

    return 1; 
}

int imageToPayload(int lines, int y_offset, const Image* image, uint8_t* out_payload, uint32_t* out_len) {
    if (!image || !out_payload || !out_len) return 1;

    *out_len = 0;
    int localCounter = 0;

    for (int y = y_offset; y < y_offset + lines; y++) { // encryption will be done here later
        for (int x = 0; x < IMG_W; x++) {
            out_payload[localCounter++] = image->bmp_img[y][x].red;
            out_payload[localCounter++] = image->bmp_img[y][x].green;
            out_payload[localCounter++] = image->bmp_img[y][x].blue;
        }
    }

    *out_len = localCounter;
    return 0;
}

PacketList* createP3Packets(uint16_t flags, const char* filename, const char* path, const Image* image, int num_chunks) {
    if (num_chunks < 1) return NULL; // probably an error case
    if (num_chunks >= MAX_CHUNKING_AMOUNT) num_chunks = MAX_CHUNKING_AMOUNT; // probably not a catastrophic error we can continue
    while (IMG_H % num_chunks != 0) num_chunks--; // probably not a catastrophic error we can continue

    PacketList* packetList = (PacketList*)calloc(1, sizeof(PacketList));
    if (!packetList) return NULL;

    Packet* packets = (Packet*)calloc((size_t)num_chunks, sizeof(Packet));
    if (!packets) {
        free(packetList);
        return NULL;
    }

    packetList->count = num_chunks;
    packetList->packets = packets;

    for (int i = 0; i < num_chunks; i++) {
        Packet *p = &packets[i];

        memcpy(p->header.magic, MAGIC_BYTES, MAGIC_LEN);
        p->header.num_chunks = (uint8_t)num_chunks;
        p->header.flags = flags;
        p->header.filename_len = strlen(filename);
        p->header.path_len = strlen(path);

        p->filename = strdup(filename);
        p->path = strdup(path);
        p->payload = (uint8_t*)malloc(PAYLOAD_SIZE / num_chunks);

        if (!p->filename || !p->path || !p->payload) {
            free_packet_list(packetList);
            return NULL;
        }

        if (imageToPayload(
                IMG_H / num_chunks,
                i * (IMG_H / num_chunks),
                image,
                p->payload,
                &p->header.payload_len
            ) != 0) {
            free_packet_list(packetList);
            return NULL;
        }
    }

    return packetList;
}

Packet* requestPacket(uint16_t FLAGS, const char* reqName, const char* reqPath) {
    Packet* p = (Packet*)calloc(1, sizeof(Packet));
    if (!p) return NULL;
    memcpy(p->header.magic, MAGIC_BYTES, MAGIC_LEN);

    p->header.num_chunks = 1;
    p->header.flags = FLAGS;
    p->header.filename_len = strlen(reqName);
    p->header.path_len = strlen(reqPath);

    p->filename = strdup(reqName);
    p->path = strdup(reqPath);
    p->payload = NULL;

    if (!p->filename || !p->path) {
        free_packet(p);
    }

    return p;
}

void free_packet(Packet *p) {
    if (!p) return;
    free(p->filename);
    free(p->path);
    free(p->payload);
    free(p);
}

void free_packet_list(PacketList *pl) {
    if (!pl) return;

    if (pl->packets) {
        for (int i = 0; i < pl->count; i++) {
            free(pl->packets[i].filename);
            free(pl->packets[i].path);
            free(pl->packets[i].payload);
        }

        free(pl->packets);
    }

    free(pl);
}

int count_args(char **args) {
    int n = 0;
    while (n < MAX_ARGS && args[n]) n++;
    return n;
}

int refreshSocket(int *sock, const char *ip, const char *port_str) {
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));

    dest.sin_family = AF_INET;
    dest.sin_port = htons((uint16_t)strtol(port_str, NULL, 10));
    inet_pton(AF_INET, ip, &dest.sin_addr);    
    int conn = connect(*sock, (struct sockaddr *)&dest, sizeof(dest));
}

int send_image_file(
    const char *ip,
    const char *port_str,
    const char *infile,
    const char *outfile_full,
    const char *rem,
    int chunks,
    int *sending_sock
) {
    int rc = 0;
    char outPath[MAX_STRING_SIZE] = {0};
    char outName[MAX_STRING_SIZE] = {0};

    uint16_t FLAGS = PMODE_SENDING;

    if (rem) {
        if (strcmp(rem, "RGB") == 0) {
            FLAGS |= CMODE_GRAYSCALE;
        } else {
            if (strstr(rem, "R")) FLAGS |= CMODE_NO_R;
            if (strstr(rem, "G")) FLAGS |= CMODE_NO_G;
            if (strstr(rem, "B")) FLAGS |= CMODE_NO_B;
        }
    }

    char *last_slash = strrchr(outfile_full, '/');

    if (last_slash) {
        int dir_len = last_slash - outfile_full;

        strncpy(outPath, outfile_full, dir_len);
        outPath[dir_len] = '\0';

        strcpy(outName, last_slash + 1);
    } else {
        strcpy(outPath, ".");
        strcpy(outName, outfile_full);
    }

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));

    dest.sin_family = AF_INET;
    dest.sin_port = htons((uint16_t)strtol(port_str, NULL, 10));
    inet_pton(AF_INET, ip, &dest.sin_addr);

    Image *image = p3ToStruct(infile);

    applyColorMode(FLAGS, image);

    PacketList *packetsToSend = createP3Packets(FLAGS, outName, outPath, image, chunks);

    int conn = connect(*sending_sock, (struct sockaddr *)&dest, sizeof(dest));

    for (int i = 0; i < packetsToSend->count; i++) {
        uint8_t *sendingCurrently =
            (uint8_t*)malloc((PAYLOAD_SIZE + HEADER_LEN + 1) * sizeof(uint8_t));

        int currentLen = 0;
        serializePacket(&packetsToSend->packets[i], sendingCurrently, &currentLen);

        for (int j = 0; j < currentLen; j++) {
            printf("%02x ", sendingCurrently[j]);
        }

        write_exact(*sending_sock, sendingCurrently, (size_t)currentLen);

        free(sendingCurrently);
    }

    cleanup: // will polish this later
        free_packet_list(packetsToSend);
        free(image);
        return rc;
}

int send_status_packet(int status_code, int *sending_sock) {
    uint16_t FLAGS = PMODE_STATUS;

    switch (status_code) {
        case STATUS_OK:
        case STATUS_MALFORMED_PACKET:
        case STATUS_PACKET_TOO_LONG:
        case STATUS_INVALID_FILE_OR_PATH:
        case STATUS_REQUEST_DENIED:
            FLAGS |= status_code;
            break;

        default:
            return -1;
    }

    Packet statusPacket;
    memset(&statusPacket, 0, sizeof(statusPacket));

    memcpy(statusPacket.header.magic, MAGIC_BYTES, MAGIC_LEN);

    statusPacket.header.num_chunks = 1;
    statusPacket.header.flags = FLAGS;
    statusPacket.header.payload_len = 0;
    statusPacket.header.filename_len = 0;
    statusPacket.header.path_len = 0;

    uint8_t sendingCurrently[HEADER_LEN];
    int currentLen = 0;

    serializePacket(&statusPacket, sendingCurrently, &currentLen);

    write_exact(*sending_sock, sendingCurrently, (size_t)currentLen);

    return 0;
}

int serializePacket(const Packet* packet, uint8_t* serialized_payload, int* serialized_len) {
    if (!packet) return NO_VALID_PACKET; // no valid packet
    if (!serialized_len || !serialized_payload) return INVALID_SERIALIZATION_PTR; // invalid serialization pointer(s)

    int buffer_ptr = 0;
    
    uint8_t numChunks = packet->header.num_chunks;
    uint16_t flags = packet->header.flags;
    uint32_t len = packet->header.payload_len;
    uint16_t filename_len = packet->header.filename_len;
    uint16_t path_len = packet->header.path_len;
    
    if(filename_len && !packet->filename) return NULL_FILENAME_NONZERO_LENGTH; // null filename with nonzero length
    if(path_len && !packet->path) return NULL_PATH_NONZERO_LENGTH; // null path with nonzero length

    memcpy(serialized_payload, packet->header.magic, MAGIC_LEN);
    buffer_ptr += MAGIC_LEN;

    serialized_payload[buffer_ptr++] = numChunks;

    serialized_payload[buffer_ptr++] = (uint8_t)(flags);
    serialized_payload[buffer_ptr++] = (uint8_t)(flags >> 8);

    serialized_payload[buffer_ptr++] = (uint8_t)(len);
    serialized_payload[buffer_ptr++] = (uint8_t)(len >> 8);
    serialized_payload[buffer_ptr++] = (uint8_t)(len >> 16);
    serialized_payload[buffer_ptr++] = (uint8_t)(len >> 24);

    serialized_payload[buffer_ptr++] = (uint8_t)(filename_len);
    serialized_payload[buffer_ptr++] = (uint8_t)(filename_len >> 8);

    serialized_payload[buffer_ptr++] = (uint8_t)(path_len);
    serialized_payload[buffer_ptr++] = (uint8_t)(path_len >> 8);

    // payload begins : this includes filename, path, and the actual image payload

    for (int i = 0; i < filename_len; i++) {
        serialized_payload[buffer_ptr++] = (uint8_t)(packet->filename[i]);
    }

    for (int i = 0; i < path_len; i++) {
        serialized_payload[buffer_ptr++] = (uint8_t)(packet->path[i]);
    }

    if (packet->payload && len > 0) {
        memcpy(&serialized_payload[buffer_ptr], packet->payload, (size_t)len);
        buffer_ptr += len;
    }

    *serialized_len = buffer_ptr;
    return SUCCESS;

}

Image* wireToImage(uint8_t* buffer, const int bytesRead, const int filename_len, const int path_len) {
    Image* img = (Image*)malloc(sizeof(Image));
    int p_idx = 0;
    
    for (int i = 0; i < bytesRead; ) {
        if (i <= (bytesRead - MAGIC_LEN) && *((uint32_t*)&buffer[i]) == htonl((0x53455033))) { // 0x53455033 = "SEP3"  
            i += (HEADER_LEN + filename_len + path_len);
            continue;
        }

        if (i + 2 < bytesRead && p_idx < (IMG_H * IMG_W)) {
            int r = p_idx / IMG_W;
            int c = p_idx % IMG_W;

            img->bmp_img[r][c].red   = buffer[i++];
            img->bmp_img[r][c].green = buffer[i++];
            img->bmp_img[r][c].blue  = buffer[i++];
            
            p_idx++;
        } else {
            break;
        }
    }

    return img;
}

ssize_t read_exact(int fd, void *buf, size_t n) {
    size_t off = 0;

    while (off < n) {
        ssize_t r = recv(fd, (uint8_t*)buf + off, n - off, 0);
        if (r <= 0) return r;  // 0=EOF, -1=error
        off += (size_t)r;
    }

    return (ssize_t)off;
}

ssize_t write_exact(int fd, const void *buf, size_t n) {
  size_t off = 0;
  while (off < n) {
    ssize_t w = send(fd, (const char*)buf + off, n - off, 0);
    if (w <= 0) return w;
    off += (size_t)w;
  }
  return (ssize_t)off;
}
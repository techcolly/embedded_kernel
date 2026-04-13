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

const char MAGIC_BYTES[MAGIC_LEN + 1] = "TCP3";

Image* p3ToStruct(const char* path) {
    FILE* ptr = fopen(path, "r");

    if(!ptr) {
        printf("\nError opening file");
        return NULL;
    }

    char magic[3]; 

    if (fscanf(ptr, "%2s", magic) != 1 || strcmp(magic, "P3") != 0) {
        printf("\nInvalid File Format");
        return NULL;
    }

    int width, height, max_color;

    if (fscanf(ptr, "%d %d", &width, &height) != 2 || width != 128 || height != 128) {
        printf("\nInvalid File Format");
        return NULL;
    }

    if (fscanf(ptr, "%d", &max_color) != 1 || max_color != 255) {
        printf("\nInvalid File Format");
        return NULL;
    }


    int y = 0, r, g, b;
    char current_line[1030]; // it's guaranteed not to be longer than this
    Image* image = (Image*)malloc(sizeof(Image));
    
    if(!image) {
        printf("\nAn error occured allocating the image");
        return NULL;
    }

    for (int y = 0; y < IMG_H; y++) {
        for (int x = 0; x < IMG_W; x++) {
            if (fscanf(ptr, "%d %d %d", &r, &g, &b) != 3) {
                free(image);
                printf("\nError scanning pixels into image struct");
                return NULL;
            }
            image->bmp_img[y][x].red   = (uint8_t)r;
            image->bmp_img[y][x].green = (uint8_t)g;
            image->bmp_img[y][x].blue  = (uint8_t)b;
        }
    }

    return image;
}

int structToP3(const char* path, const Image* image) {
    FILE* ptr = fopen(path, "w");

    if(!ptr) {
        printf("\nError opening file");
        return 1;
    }

    fprintf(ptr, "P3\n128 128\n255");

    int r, g, b;
    for (int y = 0; y < IMG_H; y++) {
        fprintf(ptr, "\n");
        for (int x = 0; x < IMG_W; x++) {
            
            r = (int)image->bmp_img[y][x].red;
            g = (int)image->bmp_img[y][x].green;
            b = (int)image->bmp_img[y][x].blue;

            fprintf(ptr, "%d %d %d", r, g, b);
            if (x < IMG_W - 1) fprintf(ptr, " ");
        }
    }

    return 0;
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

int imageToPayload(uint16_t flags, int lines, int y_offset, const Image* image, uint8_t* out_payload, uint32_t* out_len) {
    *out_len = 0;
}

PacketList* createP3Packets(uint16_t flags, const char* filename, const char* path, const Image* image, int num_chunks) {
    if (num_chunks < 1) num_chunks = 1;
    if (num_chunks >= MAX_CHUNKING_AMOUNT) num_chunks = MAX_CHUNKING_AMOUNT;
    while (IMG_H % num_chunks != 0) num_chunks--;

    Header* headers = (Header*)malloc(num_chunks * sizeof(Header));

    uint8_t** payloads = (uint8_t**)malloc(num_chunks * sizeof(uint8_t*));

    Packet* packets = (Packet*)malloc(num_chunks * sizeof(Packet));

    PacketList* packetList = (PacketList*)malloc(sizeof(PacketList));
    packetList->count = num_chunks;
    packetList->packets = packets;

    for (int i = 0; i < num_chunks; i++) {

        memcpy(headers[i].magic, MAGIC_BYTES, 4);
        headers[i].flags = flags;
        headers[i].filename_len = strlen(filename);
        headers[i].path_len = strlen(path);

        payloads[i] = (uint8_t*)malloc((49152 / num_chunks + 400) * sizeof(uint8_t));

        imageToPayload(
            headers[i].flags, 
            IMG_H / num_chunks, 
            i * (IMG_H / num_chunks), 
            image, 
            payloads[i], 
            &headers[i].payload_len
        );

        packets[i].header = headers[i];
        packets[i].filename = filename;
        packets[i].path = path;
        packets[i].payload = payloads[i];
    }

    return packetList;
}
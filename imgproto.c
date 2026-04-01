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
    
}
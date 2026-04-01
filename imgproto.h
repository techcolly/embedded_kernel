#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>

#define IMG_H 128
#define IMG_W 128
#define MAGIC_LEN 4
#define RGB_BYTES 3

#define PAYLOAD_SIZE (IMG_H * IMG_W * RGB_BYTES)

typedef struct Pixel {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} Pixel;

typedef struct Image { // when processing an image we use this its NOT part of the protocol
    Pixel bmp_img[IMG_H][IMG_W];
} Image;

typedef struct Header {
    uint8_t magic[MAGIC_LEN];
    uint8_t flags; 
    /*
        bit 0 : use encryption or obfuscation (set 0 for encryption, set 1 for obfuscation)
        bit 1 : is this a status message or image (set 0 for status, set 1 for error)
        bits 2-3 : error reason (if applicable set 0 otherwise)
        bits 4-6 : sending mode (if applicable set 0 otherwise)
        bit 7 : is this the final image being sent (if applicable set 0 otherwise)
    */
    uint32_t payload_len;
    uint16_t filename_len;
    uint16_t path_len;
} Header;

typedef struct Packet {
    Header header;
    char* filename;
    char* path;
    uint8_t payload[PAYLOAD_SIZE];
} Packet;

Image* p3ToStruct(const char* path); // this will just return NULL if it didn't work
int structToP3(const char* path, const Image* image); // will return 0 or 1 depending on if it worked or not

Packet* createP3Packet(
    uint8_t flags, 
    const char* filename, 
    const char* path, 
    const uint8_t* payload,
    uint32_t payload_len
); // will return NULL if it didn't work


// add image to payload and payload to imagew later
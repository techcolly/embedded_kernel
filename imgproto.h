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
#define HEADER_LEN 88
#define MAX_CHUNKING_AMOUNT 50
#define SECURITY_MODE (1 << 0)
#define PMODE_SENDING (0b00 << 1)
#define PMODE_REQUESTING (0b01 << 1)
#define PMODE_STATUS (0b10 << 1)

#define STATUS_OK (0b000 << 3)
#define STATUS_MALFORMED_PACKET (0b001 << 3)
#define STATUS_PACKET_TOO_LONG (0b010 << 3)
#define STATUS_INVALID_FILE_OR_PATH (0b011 << 3)

#define CMODE_NO_R (1 << 8)
#define CMODE_NO_G (1 << 7)
#define CMODE_NO_B (1 << 6)

#define MASK_CMODE (0b111 << 6)

extern const char MAGIC_BYTES[MAGIC_LEN];

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
    uint16_t flags; 
    /*
        bit 0 : use encryption or obfuscation (set 0 for encryption, set 1 for obfuscation)
        bit 1-2 : sending an image, requesting an image, sending status message
        bits 3-5 : status code (if applicable set 0 otherwise)
        bits 6-8 : color mode (if applicable set 0 otherwise)
        bits 9-15 : unused for now
    */
    uint32_t payload_len;
    uint16_t filename_len;
    uint16_t path_len;
} Header;

typedef struct Packet {
    Header header;
    char* filename;
    char* path;
    uint8_t* payload;
} Packet;

typedef struct PacketList {
    Packet* packets;
    int count;
} PacketList;

// ----------------------------------------------- client-side functions --------------------------------------------------- //

Image* p3ToStruct(const char* path); // this will just return NULL if it didn't work

int applyColorMode(uint16_t flags, Image* image); // this one actually can modify the image

int imageToPayload(
    uint16_t flags, 
    int lines, 
    int y_offset, 
    const Image* image, 
    uint8_t* out_payload, 
    uint32_t* out_len
); // createP3Packets can call this when chunking

PacketList* createP3Packets(
    uint16_t flags, 
    const char* filename, 
    const char* path, 
    const Image* image,
    int num_chunks
); 

int serializePacket(
    const Packet* packet, 
    uint8_t* serialized_payload, 
    int serialized_len
); // will encrypt later

// ----------------------------------------------- server-side functions --------------------------------------------------- //

int structToP3(const char* path, const Image* image); // will return 0 or 1 depending on if it worked or not
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <stddef.h>

#define IMG_H 128
#define IMG_W 128
#define MAGIC_LEN 4
#define RGB_BYTES 3
#define MAX_STRING_SIZE 255
#define MAX_ARGS 16
#define INPUT_BUF_SIZE 2*MAX_STRING_SIZE + 20

#define PAYLOAD_SIZE (IMG_H * IMG_W * RGB_BYTES)
#define HEADER_LEN (MAGIC_LEN + 11)
#define MAX_CHUNKING_AMOUNT 50
#define CHUNKS_DEFAULT 32

#define PMODE_SENDING (0b00 << 1)
#define PMODE_REQUESTING (0b01 << 1)
#define PMODE_STATUS (0b10 << 1)
#define PMODE_MASK (0b11 << 1)

#define STATUS_OK (0b000 << 3)
#define STATUS_MALFORMED_PACKET (0b001 << 3)
#define STATUS_PACKET_TOO_LONG (0b010 << 3)
#define STATUS_INVALID_FILE_OR_PATH (0b011 << 3)
#define STATUS_REQUEST_DENIED (0b100 << 3)

#define USE_ENC (1 << 10)
#define USE_OB (1 << 9)
#define CMODE_NO_R (1 << 8)
#define CMODE_NO_G (1 << 7)
#define CMODE_NO_B (1 << 6)

#define CMODE_GRAYSCALE (0b111 << 6)
#define MASK_CMODE (0b111 << 6)

// return codes for the program not the protocol; will make this an enum later
#define SUCCESS 0
#define GENERIC_FAILURE 1
#define NO_VALID_PACKET 2
#define NULL_FILENAME_NONZERO_LENGTH 3
#define NULL_PATH_NONZERO_LENGTH 4
#define INVALID_SERIALIZATION_PTR 5
#define S2P3_PATH_TOO_LONG 6
#define S2P3_FOPEN_ERR 7
#define S2P3_FCLOSE_ERR 8
#define S2P3_BAD_ARGS 9

extern const char MAGIC_BYTES[MAGIC_LEN + 1];

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
    uint8_t num_chunks;
    uint16_t flags; 
    /*
        bit 0 : unused
        bit 1-2 : sending an image, requesting an image, sending status message
        bits 3-5 : status code (if applicable set 0 otherwise)
        bits 6-8 : color mode (if applicable set 0 otherwise) ---> 111 for grayscale
        bits 9-10 : use encryption or obfuscation or none (0: none, 1: obfuscation, 2: encryption, 3: unused)
        bits 11-15: unused
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

// ----------------------------------------------- utility functions --------------------------------------------------- //

ssize_t read_exact(int fd, void *buf, size_t n);
ssize_t write_exact(int fd, const void *buf, size_t n);

// ----------------------------------------------- sending functions --------------------------------------------------- //

Image* p3ToStruct(const char* path); // this will just return NULL if it didn't work

int applyColorMode(uint16_t flags, Image* image); // this one actually can modify the image

int imageToPayload(
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
    int* serialized_len
); // will encrypt later

// ----------------------------------------------- recieving functions --------------------------------------------------- //

Image* wireToImage(uint8_t* buffer, const int bytesRead, const int filename_len, const int path_len);
int structToP3(const char* path, const char* filename, const Image* image); // will return 0 or 1 depending on if it worked or not  




Packet* requestPacket(uint16_t FLAGS, const char* reqName, const char* reqPath);

int send_image_file(
    const char *ip,
    const char *port_str,
    const char *infile,
    const char *outfile_full,
    const char *rem,
    int chunks,
    int *sending_sock
);

int send_status_packet(int status_code, int *sending_sock);

int refreshSocket(int *sock, const char *ip, const char *port_str); 

int count_args(char **args);

void free_packet(Packet *p);

void free_packet_list(PacketList *pl);